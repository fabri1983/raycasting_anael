const fs = require('fs');
const { Worker, isMainThread, parentPort, workerData } = require('worker_threads');
const os = require('os');
const utils = require('./utils');

const tabDeltasFile = '../inc/tab_deltas.h';
const mapMatrixFile = '../src/map_matrix.c';
const outputFile = 'tab_map_hit_OUTPUT.txt';

// Check correct values of constants before script execution. See consts.h.
const {
    FS, FP, AP, STEP_COUNT, PIXEL_COLUMNS, MAP_SIZE, MAP_FRACTION, MIN_POS_XY, MAX_POS_XY, 
    MAP_HIT_MASK_MAPXY, MAP_HIT_MASK_SIDEDISTXY, MAP_HIT_MASK_SIDE, 
    MAP_HIT_OFFSET_MAPXY, MAP_HIT_OFFSET_SIDEDIST
} = require('./consts');

// Progress tracking
const totalIterations = (MAX_POS_XY - MIN_POS_XY + 1) * (MAX_POS_XY - MIN_POS_XY + 1) * (1024 / 8);
let completedIterations = 0;

function displayProgress () {
    const progress = (completedIterations / totalIterations) * 100;
    const progressBar = '='.repeat(Math.floor(progress / 2)) + '-'.repeat(50 - Math.floor(progress / 2));
    process.stdout.write(`\r[${progressBar}] ${progress.toFixed(2)}%`);
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
function processGameChunk (workerId, startPosX, endPosX, tab_deltas, map) {
    const outputHitMap = new Map();

    for (let posX = startPosX; posX <= endPosX; posX += 1) {

        for (let posY = MIN_POS_XY; posY <= MAX_POS_XY; posY += 1) {

            // Current location normalized
            let x = Math.floor(posX / FP);
            let y = Math.floor(posY / FP);

            // Limit Y axis location normalized
            const ytop = Math.floor((posY - (MAP_FRACTION-1)) / FP);
            const ybottom = Math.floor((posY + (MAP_FRACTION-1)) / FP);

            // Check X axis collision
            if (map[y*MAP_SIZE + x+1] || map[ytop*MAP_SIZE + x+1] || map[ybottom*MAP_SIZE + x+1]) {
                // Send progress update to main thread
                parentPort.postMessage({ type: 'progress', value: (MAX_POS_XY - posY + 1) * (1024 / 8) });
                // Stop current Y and continue with next X until it gets outside the collision
                break;
            }
            if (x == 0 || map[y*MAP_SIZE + x-1] || map[ytop*MAP_SIZE + x-1] || map[ybottom*MAP_SIZE + x-1]) {
                // Send progress update to main thread
                parentPort.postMessage({ type: 'progress', value: (MAX_POS_XY - posY + 1) * (1024 / 8) });
                // Stop current Y and continue with next X until it gets outside the collision
                break;
            }

            // Limit X axis location normalized
            const xleft = Math.floor((posX - (MAP_FRACTION-1)) / FP);
            const xright = Math.floor((posX + (MAP_FRACTION-1)) / FP);

            // Check Y axis collision
            if (map[(y+1)*MAP_SIZE + x] || map[(y+1)*MAP_SIZE + xleft] || map[(y+1)*MAP_SIZE + xright]) {
                // Send progress update to main thread
                parentPort.postMessage({ type: 'progress', value: 1024 / 8 });
                // Continue with next Y until it gets outside the collision
                continue;
            }
            if (y == 0 || map[(y-1)*MAP_SIZE + x] || map[(y-1)*MAP_SIZE + xleft] || map[(y-1)*MAP_SIZE + xright]) {
                // Send progress update to main thread
                parentPort.postMessage({ type: 'progress', value: 1024 / 8 });
                // Continue with next Y until it gets outside the collision
                continue;
            }

            for (let angle = 0; angle < 1024; angle += 8) {

                // DDA (Digital Differential Analyzer)

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
                    const rayDirX = utils.toSignedFrom16b(tab_deltas[(a * PIXEL_COLUMNS * 4) + column*4 + 2]);
                    const rayDirY = utils.toSignedFrom16b(tab_deltas[(a * PIXEL_COLUMNS * 4) + column*4 + 3]);

                    let sideDistX, stepX;

                    if (rayDirX < 0) {
                        stepX = -1;
                        sideDistX = utils.toUnsigned16Bit((sideDistX_l0 * deltaDistX) >> FS);
                    } else {
                        stepX = 1;
                        sideDistX = utils.toUnsigned16Bit((sideDistX_l1 * deltaDistX) >> FS);
                    }

                    let sideDistY, stepY, stepYMS;

                    if (rayDirY < 0) {
                        stepY = -1;
                        stepYMS = -MAP_SIZE;
                        sideDistY = utils.toUnsigned16Bit((sideDistY_l0 * deltaDistY) >> FS);
                    } else {
                        stepY = 1;
                        stepYMS = MAP_SIZE;
                        sideDistY = utils.toUnsigned16Bit((sideDistY_l1 * deltaDistY) >> FS);
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

            // Send progress update to main thread
            parentPort.postMessage({ type: 'progress', value: 1024/8 });
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
    const tab_deltas = utils.readTabDeltas(tabDeltasFile);
    const map = utils.readMapMatrix(mapMatrixFile);
    const workers = [];
    const chunkSize = Math.ceil((MAX_POS_XY + 1) / numCores);

    console.log('(Invoke with: node --max-old-space-size=5120 <script.js> to avoid running out of mem)');
    console.log(`Utilizing ${numCores} cores for processing.`);
    let startTimeStr = new Date().toLocaleString('en-US', { hour12: false });
    console.log(startTimeStr);

    // Reset iterations counter
    completedIterations = 0;

    for (let i=0, startPosX=MIN_POS_XY; i < numCores && startPosX < MAX_POS_XY; i++, startPosX+=chunkSize) {
        const endPosX = Math.min(startPosX + chunkSize - 1, MAX_POS_XY);
        const workerId = `[${startPosX}-${endPosX}]`;

        workers.push(
            new Promise((resolve, reject) => {
                const worker = new Worker(__filename, {
                    workerData: { workerId, startPosX, endPosX, tab_deltas, map }
                });

                worker.on('message', (message) => {
                    if (message.type === 'progress')
                        completedIterations += message.value;
                    else
                        resolve(message);
                });

                worker.on('error', reject);
            })
        );
    }

    console.log('Main thread: Waiting for all workers to complete...');

    displayProgress(); // Initial progress display
    // Set up progress display interval
    const progressInterval = setInterval(displayProgress, 4000);

    Promise.all(workers)
        .then(results => {
            clearInterval(progressInterval); // Stop progress display
            displayProgress(); // Last update with updated counter
            process.stdout.write('\n'); // Move to the next line after progress bar

            const finalMap = new Map();
            results.forEach(result => {
                result.forEach(([key, value]) => {
                    finalMap.set(key, value);
                });
            });

            // Sort by key in ASC order, then save value into an array
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
                })
                .map(entry => entry[1]); // keep the value

            // Sanity check on size
            checkFinalSize(sortedOutput);

            // Write output to file
            writeOutputToFile(sortedOutput, outputFile);

            let endTimeStr = new Date().toLocaleString('en-US', { hour12: false });
            console.log(endTimeStr);
        })
        .catch(error => {
            process.stdout.write('\n'); // Move to the next line after progress bar
            console.error('An error occurred:', error);

            let endTimeStr = new Date().toLocaleString('en-US', { hour12: false });
            console.log(endTimeStr);
        });
}

if (isMainThread) {
    runWorkers();
} else {
    const { workerId, startPosX, endPosX, tab_deltas, map } = workerData;
    const result = processGameChunk(workerId, startPosX, endPosX, tab_deltas, map);
    parentPort.postMessage(Array.from(result.entries()));
}
