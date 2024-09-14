const fs = require('fs');
const { Worker, isMainThread, parentPort, workerData } = require('worker_threads');
const os = require('os');

const tabDeltasFile = '../inc/tab_deltas.h';
const mapMatrixFile = '../src/map_matrix.c';
const outputFile = 'tab_map_hit_OUTPUT.txt';

// Check correct values of constants before script execution. See consts.h.
const {
    FS, FP, AP, STEP_COUNT, PIXEL_COLUMNS, MAP_SIZE, MIN_POS_XY, MAX_POS_XY, 
    MAP_HIT_MASK_MAPXY, MAP_HIT_MASK_SIDEDISTXY, MAP_HIT_MASK_SIDE, 
    MAP_HIT_OFFSET_MAPXY, MAP_HIT_OFFSET_SIDEDIST
} = require('./consts');

function isInteger (value) {
    return !isNaN(parseInt(value)) && isFinite(value);
}

// Read and parse tab_deltas.h
function readTabDeltas () {
    const content = fs.readFileSync(tabDeltasFile, 'utf8');
    const lines = content.split('\n');
    const tab_deltas = [];

    for (const line of lines) {
        const numbers = line.trim().replace(/,\s*$/, '').split(/[,\s]+/)
            .filter(n => !isNaN(n) && isInteger(n))
            .map(n => parseInt(n));
        if (numbers.length >= 4) {
            tab_deltas.push(numbers[0]);
            tab_deltas.push(numbers[1]);
            tab_deltas.push(numbers[2]);
            tab_deltas.push(numbers[3]);
        }
    }

    if (tab_deltas.length !== AP * PIXEL_COLUMNS * 4) {
        throw new Error(`Invalid number of elements in tab_deltas (expected ${AP * PIXEL_COLUMNS * 4}): ${tab_deltas.length}`);
    }

    return tab_deltas;
}

// Read and parse map_matrix.c
function readMapMatrix () {
    const content = fs.readFileSync(mapMatrixFile, 'utf8');
    const lines = content.split('\n');
    const map_matrix = [];

    let inMapDefinition = false;

    for (const line of lines) {
        if (line.includes('const u8 map[MAP_SIZE][MAP_SIZE] =')) {
            inMapDefinition = true;
            continue;
        }

        if (inMapDefinition) {
            if (line.includes('};')) {
                break; // End of map definition
            }

            const numbers = line.trim()
                .replace(/^{/, '') // Remove opening brace
                .replace(/},?$/, '') // Remove closing brace and optional comma
                .split(',')
                .map(n => parseInt(n.trim(), 10))
                .filter(n => !isNaN(n));

            if (numbers.length > 0) {
                map_matrix.push(...numbers); // use spread operator
            }
        }
    }

    if (map_matrix.length !== (MAP_SIZE * MAP_SIZE)) {
        throw new Error(`Invalid map_matrix dimensions (expected ${MAP_SIZE} * ${MAP_SIZE})`);
    }

    return map_matrix;
}

// Convert 16-bit unsigned to 16-bit signed
function toSigned16Bit (value) {
    return value >= 32768 ? value - 65536 : value;
}

function calculateOutputHitMapEntry (posX, posY, a, column, mapXY, side, sideDistXY) {
    const key = `[${Math.floor(posX/FP)}][${Math.floor(posY/FP)}][${a}][${column}]`;
    // At this point we know sideDistXY < 4096, mapXY < MAP_SIZE, and side is [0,1]
    const value = ((mapXY & MAP_HIT_MASK_MAPXY) << MAP_HIT_OFFSET_MAPXY) 
                | ((sideDistXY & MAP_HIT_MASK_SIDEDISTXY) << MAP_HIT_OFFSET_SIDEDIST) 
                | (side & MAP_HIT_MASK_SIDE);
    return { key, value };
}

// Processing function to be run inside the worker thread
function processGameChunk (startPosX, endPosX, tab_deltas, map) {
    const outputHitMap = new Map();

    for (let posX = startPosX; posX <= endPosX; posX += 1) {
        for (let posY = MIN_POS_XY; posY <= MAX_POS_XY; posY += 1) {
            for (let angle = 0; angle < 1024; angle += 8) {

                const sideDistX_l0 = posX - (Math.floor(posX / FP) * FP);
                const sideDistX_l1 = (Math.floor(posX / FP) + 1) * FP - posX;
                const sideDistY_l0 = posY - (Math.floor(posY / FP) * FP);
                const sideDistY_l1 = (Math.floor(posY / FP) + 1) * FP - posY;

                let a = Math.floor(angle / (1024 / AP));

                for (let column = 0; column < PIXEL_COLUMNS; ++column) {

                    // instead of: const u16 *delta_a_ptr = tab_deltas + (a * PIXEL_COLUMNS * 4);
                    // we use the offset directly when accessing tab_deltas
                    const deltaDistX = tab_deltas[(a * PIXEL_COLUMNS * 4) + column*4 + 0];
                    const deltaDistY = tab_deltas[(a * PIXEL_COLUMNS * 4) + column*4 + 1];
                    const rayDirX = toSigned16Bit(tab_deltas[(a * PIXEL_COLUMNS * 4) + column*4 + 2]);
                    const rayDirY = toSigned16Bit(tab_deltas[(a * PIXEL_COLUMNS * 4) + column*4 + 3]);

                    let sideDistX, stepX;

                    if (rayDirX < 0) {
                        stepX = -1;
                        sideDistX = ((sideDistX_l0 * deltaDistX) >> FS) & 0xFFFF;
                    } else {
                        stepX = 1;
                        sideDistX = ((sideDistX_l1 * deltaDistX) >> FS) & 0xFFFF;
                    }

                    let sideDistY, stepY, stepYMS;

                    if (rayDirY < 0) {
                        stepY = -1;
                        stepYMS = -MAP_SIZE;
                        sideDistY = ((sideDistY_l0 * deltaDistY) >> FS) & 0xFFFF;
                    } else {
                        stepY = 1;
                        stepYMS = MAP_SIZE;
                        sideDistY = ((sideDistY_l1 * deltaDistY) >> FS) & 0xFFFF;
                    }

                    let mapX = Math.floor(posX / FP);
                    let mapY = Math.floor(posY / FP);
                    
                    let hitInLoop = false;

                    for (let n = 0; n < STEP_COUNT; ++n) {
                        // side X
                        if (sideDistX < sideDistY) {
                            mapX += stepX;
                            let hit = map[mapY * MAP_SIZE + mapX];
                            if (hit != 0) {
                                hitInLoop = true;
                                // Here we found a hit for side X when user is at (posX/FP,posY/FP,a,column).
                                // Store in outputHitMap at key [${posX/FP}][${posY/FP}][${a}] the value X,${mapY},${sideDistX}
                                const entry = calculateOutputHitMapEntry(posX, posY, a, column, 0, mapY, sideDistX);
                                outputHitMap[entry.key] = entry.value;
console.log(entry);
                                break;
                            }
                            sideDistX += deltaDistX;
                        }
                        // side Y
                        else {
                            mapY += stepY;
                            let hit = map[mapY * MAP_SIZE + mapX];
                            if (hit != 0) {
                                hitInLoop = true;
                                // Here we found a hit for side Y when user is at (posX/FP,posY/FP,a,column).
                                // Store in outputHitMap at key [${posX/FP}][${posY/FP}][${a}] the value Y,${mapX},${sideDistY}
                                const entry = calculateOutputHitMapEntry(posX, posY, a, column, 1, mapX, sideDistY);
                                outputHitMap[entry.key] = entry.value;
console.log(entry);
                                break;
                            }
                            sideDistY += deltaDistY;
                        }
                    }

                    // no hit inside the loop? Just in case
                    if (hitInLoop == false) {
                        // Store in outputHitMap at key [${posX/FP}][${posY/FP}][${a}] the value 0
                        const entry = calculateOutputHitMapEntry(posX, posY, a, column, 0, 0, 0);
                        outputHitMap[entry.key] = entry.value;
                    }
                }
            }
        }
    }

    return outputHitMap;
}

function checkFinalSize (arr) {
    const expectedCount = MAP_SIZE * MAP_SIZE * (1024/(1024/AP)) * PIXEL_COLUMNS;
    if (arr.length != expectedCount)
        throw new Error(`Sanity check failed: hit map size is ${arr.length} instead of ${expectedCount}`);
}

function writeOutputToFile (arr, outputFile) {
    // Take groups of 16 elems
    const lines = [];
    for (let i = 0; i < arr.length; i += 16) {
        const line = arr.slice(i, i + 16).join(',');
        lines.push(line);
    }

    // Join lines with newline character
    const content = lines.join(',\n');

    fs.writeFileSync(outputFile, content);
    console.log('Processing complete. Output saved to ' + outputFile);
}

// Function to run parallel workers
function runWorkers () {
    const numCores = Math.min(16, os.cpus().length);
    const tab_deltas = readTabDeltas();
    const map = readMapMatrix();
    const workers = [];
    const chunkSize = Math.ceil((MAX_POS_XY + 1) / numCores);

    console.log('(Invoke with: node --max-old-space-size=5120 <script.js> to avoid running out of mem)');
    console.log(`Utilizing ${numCores} cores for processing.`);

    for (let i = 0; i < numCores; i++) {
        const startPosX = Math.max(i * chunkSize, MIN_POS_XY);
        const endPosX = Math.min((i + 1) * chunkSize, MAX_POS_XY);

        workers.push(
            new Promise((resolve, reject) => {
                const worker = new Worker(__filename, {
                    workerData: { startPosX, endPosX, tab_deltas, map }
                });

                worker.on('message', resolve);
                worker.on('error', reject);
            })
        );
    }

    console.log('Main thread: Waiting for all workers to complete...');

    Promise.all(workers)
        .then(results => {
            const finalMap = new Map();
            results.forEach(result => {
console.log(result);
                result.forEach(([key, value]) => {
                    finalMap.set(key, value);
                });
            });
console.log(finalMap);
            // Convert Map to Array and sort in ASC order
            const sortedOutput = Array.from(finalMap.entries()).sort((a, b) => {
                const [, posX_FP_1, posY_FP_1, angle_1, col_1] = a[0].match(/\[(\d+)\]\[(\d+)\]\[(\d+)\]\[(\d+)\]/);
                const [, posX_FP_2, posY_FP_2, angle_2, col_2] = b[0].match(/\[(\d+)\]\[(\d+)\]\[(\d+)\]\[(\d+)\]/);
                if (parseInt(posX_FP_1) !== parseInt(posX_FP_2)) {
                    return parseInt(posX_FP_1) - parseInt(posX_FP_2);
                }
                if (parseInt(posY_FP_1) !== parseInt(posY_FP_2)) {
                    return parseInt(posY_FP_1) - parseInt(posY_FP_2);
                }
                if (parseInt(angle_1) !== parseInt(angle_2)) {
                    return parseInt(angle_1) - parseInt(angle_2);
                }
                return parseInt(col_1) - parseInt(col_2);
            }).map(entry => entry[1]);

            // Sanity check on size
            checkFinalSize(sortedOutput);

            // Write output to file
            writeOutputToFile(sortedOutput, outputFile);
        })
        .catch(error => {
            console.error('An error occurred:', error);
        });
}

if (isMainThread) {
    runWorkers();
} else {
    const { startPosX, endPosX, tab_deltas, map } = workerData;
    const result = processGameChunk(startPosX, endPosX, tab_deltas, map);
    parentPort.postMessage(Array.from(result.entries()));
}
