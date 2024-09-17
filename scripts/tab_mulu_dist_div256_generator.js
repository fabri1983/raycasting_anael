const fs = require('fs');
const { Worker, isMainThread, parentPort, workerData } = require('worker_threads');
const os = require('os');
const utils = require('./utils');

const tabDeltasFile = '../inc/tab_deltas.h';
const mapMatrixFile = '../src/map_matrix.c';
const outputFile = 'tab_mulu_dist_div256_PARTIAL.txt';

// Check correct values of constants before script execution. See consts.h.
const { FS, FP, AP, PIXEL_COLUMNS, MAP_SIZE, MAP_FRACTION, MIN_POS_XY, MAX_POS_XY } = require('./consts');

// Progress tracking
const totalIterations = (MAX_POS_XY - MIN_POS_XY + 1) * (MAX_POS_XY - MIN_POS_XY + 1) * (1024 / 8);
let completedIterations = 0;

function displayProgress () {
    const progress = (completedIterations / totalIterations) * 100;
    const progressBar = '='.repeat(Math.floor(progress / 2)) + '-'.repeat(50 - Math.floor(progress / 2));
    process.stdout.write(`\r[${progressBar}] ${progress.toFixed(2)}%`);
}

// Processing function to be run inside the worker thread
function processTabDeltasChunk (workerId, startPosX, endPosX, tab_deltas, map) {
    const outputMap = new Map();

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
            else if (x == 0 || map[y*MAP_SIZE + x-1] || map[ytop*MAP_SIZE + x-1] || map[ybottom*MAP_SIZE + x-1]) {
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
            else if (y == 0 || map[(y-1)*MAP_SIZE + x] || map[(y-1)*MAP_SIZE + xleft] || map[(y-1)*MAP_SIZE + xright]) {
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
                const a_for_matrix = a * PIXEL_COLUMNS;

                for (let c = 0; c < PIXEL_COLUMNS; ++c) {

                    // instead of: const u16 *delta_a_ptr = tab_deltas + (a * PIXEL_COLUMNS * 4);
                    // we use the offset directly when accessing tab_deltas
                    const deltaDistX = tab_deltas[(a * PIXEL_COLUMNS * 4) + c*4 + 0];
                    const deltaDistY = tab_deltas[(a * PIXEL_COLUMNS * 4) + c*4 + 1];
                    const rayDirX = utils.toSignedFrom16b(tab_deltas[(a * PIXEL_COLUMNS * 4) + c*4 + 2]);
                    const rayDirY = utils.toSignedFrom16b(tab_deltas[(a * PIXEL_COLUMNS * 4) + c*4 + 3]);

                    let sideDistX, stepX;
                    let keyX = null;

                    if (rayDirX < 0) {
                        stepX = -1;
                        sideDistX = utils.toUnsigned16Bit((sideDistX_l0 * deltaDistX) >> FS);
                        keyX = `[${sideDistX_l0}][${a_for_matrix + c}]`;
                    } else {
                        stepX = 1;
                        sideDistX = utils.toUnsigned16Bit((sideDistX_l1 * deltaDistX) >> FS);
                        keyX = `[${sideDistX_l1}][${a_for_matrix + c}]`;
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
                        sideDistY = utils.toUnsigned16Bit((sideDistY_l0 * deltaDistY) >> FS);
                        //keyY = `[${sideDistY_l0}][${a_for_matrix + c}]`;
                    } else {
                        stepY = 1;
                        stepYMS = MAP_SIZE;
                        sideDistY = utils.toUnsigned16Bit((sideDistY_l1 * deltaDistY) >> FS);
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

            // Send progress update to main thread
            parentPort.postMessage({ type: 'progress', value: 1024/8 });
        }
    }

    return outputMap;
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
    const progressInterval = setInterval(displayProgress, 2000);

    Promise.all(workers)
        .then(results => {
            clearInterval(progressInterval); // Stop progress display
            displayProgress(); // Last update with updated counter
            process.stdout.write('\n'); // Move to the next line after progress bar

            const finalMap = new Map();
            results.forEach(result => {
                result.forEach(([key, value]) => {
                    // NOTE: not sure why I need to do this, something is wrong in the way the keys is generated.
                    // Keep the line with biggest value
                    //if (!finalMap.has(key) || finalMap.get(key) < value)
                        finalMap.set(key, value);
                });
            });

            // Sort by key in ASC order, then save key=value into an array
            const sortedOutputLines = Array.from(finalMap.entries()).sort((a, b) => {
                    const [, N1, M1] = a[0].match(/\[(\d+)\]\[(\d+)\]/);
                    const [, N2, M2] = b[0].match(/\[(\d+)\]\[(\d+)\]/);
                    if (parseInt(N1) !== parseInt(N2)) {
                        return parseInt(N1) - parseInt(N2);
                    }
                    return parseInt(M1) - parseInt(M2);
                })
                .map(entry => `${entry[0]}=${entry[1]}`);

            // Write output to file
            fs.writeFileSync(outputFile, sortedOutputLines.join('\n'));
            console.log('Processing complete. Output saved to ' + outputFile);

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
    const result = processTabDeltasChunk(workerId, startPosX, endPosX, tab_deltas, map);
    parentPort.postMessage(Array.from(result.entries()));
}
