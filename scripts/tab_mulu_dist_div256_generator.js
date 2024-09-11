const fs = require('fs');
const { Worker, isMainThread, parentPort, workerData } = require('worker_threads');
const os = require('os');

const inputFile = '../inc/tab_deltas.h';
const outputFile = 'tab_mulu_dist_div256_OUTPUT.txt';

// Check correct values of constants before script execution. See consts.h.
const { FS, FP, AP, PIXEL_COLUMNS, MAP_SIZE } = require('./consts');

function isInteger(value) {
    return !isNaN(parseInt(value)) && isFinite(value);
}

// Read and parse tab_deltas.h
function readTabDeltas() {
    const content = fs.readFileSync(inputFile, 'utf8');
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

// Convert 16-bit unsigned to 16-bit signed
function toSigned16Bit(value) {
    return value >= 32768 ? value - 65536 : value;
}

// Multiplication function (simulating mulu from C)
function mulu(a, b) {
    return Math.imul(a, b);
}

// Processing function to be run inside the worker thread
function processTabDeltasChunk(startPosX, endPosX, tab_deltas) {
    const outputMap = new Map(); // Using a Map to keep track of the largest value for each [N][M]

    for (let posX = startPosX; posX <= endPosX; posX += 1) {
        for (let posY = 0; posY <= FP; posY += 1) {
            for (let angle = 0; angle < 1024; angle += 8) {

                const sideDistX_l0 = (posX - Math.floor(posX / FP) * FP);
                const sideDistX_l1 = ((Math.floor(posX / FP) + 1) * FP - posX);
                const sideDistY_l0 = (posY - Math.floor(posY / FP) * FP);
                const sideDistY_l1 = ((Math.floor(posY / FP) + 1) * FP - posY);

                let a = Math.floor(angle / (1024 / AP));
                const a_for_matrix = a * PIXEL_COLUMNS;

                for (let c = 0; c < PIXEL_COLUMNS; ++c) {

                    // instead of: const u16 *delta_a_ptr = tab_deltas + (a * PIXEL_COLUMNS * 4);
                    // we use the offset directly when accessing tab_deltas
                    const deltaDistX = tab_deltas[(a * PIXEL_COLUMNS * 4) + c*4 + 0];
                    const deltaDistY = tab_deltas[(a * PIXEL_COLUMNS * 4) + c*4 + 1];
                    const rayDirX = toSigned16Bit(tab_deltas[(a * PIXEL_COLUMNS * 4) + c*4 + 2]);
                    const rayDirY = toSigned16Bit(tab_deltas[(a * PIXEL_COLUMNS * 4) + c*4 + 3]);

                    let sideDistX, stepX;
                    let keyX = null;

                    if (rayDirX < 0) {
                        stepX = -1;
                        sideDistX = (mulu(sideDistX_l0, deltaDistX) >> FS) & 0xFFFF;
                        keyX = `[${sideDistX_l0}][${a_for_matrix + c}]`;
                    } else {
                        stepX = 1;
                        sideDistX = (mulu(sideDistX_l1, deltaDistX) >> FS) & 0xFFFF;
                        //keyX = `[${sideDistX_l1}][${a_for_matrix + c}]`;
                    }

                    if (keyX != null) {
                        // NOTE: not sure why I need to do this, something is wrong in the way the keys is generated.
                        // Keep the line with biggest value
                        //if (!outputMap.has(keyX) || outputMap.get(keyX) < sideDistX)
                            outputMap.set(keyX, sideDistX);
                    }

                    let sideDistY, stepY, stepYMS;
                    let keyY;

                    if (rayDirY < 0) {
                        stepY = -1;
                        stepYMS = -MAP_SIZE;
                        sideDistY = (mulu(sideDistY_l0, deltaDistY) >> FS) & 0xFFFF;
                        //keyY = `[${sideDistY_l0}][${a_for_matrix + c}]`;
                    } else {
                        stepY = 1;
                        stepYMS = MAP_SIZE;
                        sideDistY = (mulu(sideDistY_l1, deltaDistY) >> FS) & 0xFFFF;
                        //keyY = `[${sideDistY_l1}][${a_for_matrix + c}]`;
                    }

                    if (keyY != null) {
                        // NOTE: not sure why I need to do this, something is wrong in the way the keys is generated.
                        // Keep the line with biggest value
                        //if (!outputMap.has(keyY) || outputMap.get(keyY) < sideDistY)
                            outputMap.set(keyY, sideDistY);
                    }
                }
            }
        }
    }

    return outputMap;
}

// Function to run parallel workers
function runWorkers() {
    const numCores = Math.min(8, os.cpus().length); // otherwise it fails with out of heap memory exception
    const tab_deltas = readTabDeltas();
    const workers = [];
    const chunkSize = Math.ceil((FP + 1) / numCores);

    console.log(`Utilizing ${numCores} cores for processing.`);

    for (let i = 0; i < numCores; i++) {
        const startPosX = i * chunkSize;
        const endPosX = Math.min((i + 1) * chunkSize - 1, FP);

        workers.push(
            new Promise((resolve, reject) => {
                const worker = new Worker(__filename, {
                    workerData: { startPosX, endPosX, tab_deltas }
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
                result.forEach(([key, value]) => {
                    // NOTE: not sure why I need to do this, something is wrong in the way the keys is generated.
                    // Keep the line with biggest value
                    //if (!finalMap.has(key) || finalMap.get(key) < value)
                        finalMap.set(key, value);
                });
            });

            // Convert Map to Array and sort in ASC order
            const sortedOutputLines = Array.from(finalMap.entries()).sort((a, b) => {
                const [, N1, M1] = a[0].match(/\[(\d+)\]\[(\d+)\]/);
                const [, N2, M2] = b[0].match(/\[(\d+)\]\[(\d+)\]/);
                if (parseInt(N1) !== parseInt(N2)) {
                    return parseInt(N1) - parseInt(N2);
                }
                return parseInt(M1) - parseInt(M2);
            }).map(entry => `${entry[0]}=${entry[1]}`);

            // Write output to file
            fs.writeFileSync(outputFile, sortedOutputLines.join('\n'));
            console.log('Processing complete. Output saved to ' + outputFile);
        })
        .catch(error => {
            console.error('An error occurred:', error);
        });
}

if (isMainThread) {
    runWorkers();
} else {
    const { startPosX, endPosX, tab_deltas } = workerData;
    const result = processTabDeltasChunk(startPosX, endPosX, tab_deltas);
    parentPort.postMessage(Array.from(result.entries()));
}
