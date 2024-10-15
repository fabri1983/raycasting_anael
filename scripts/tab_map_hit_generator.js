const fs = require('fs');
const { Worker, isMainThread, parentPort, workerData } = require('worker_threads');
const os = require('os');
const utils = require('./utils');
// Check correct values of constants before script execution. See consts.h.
const {
    FS, FP, AP, STEP_COUNT, PIXEL_COLUMNS, MAP_SIZE, MAP_FRACTION, MIN_POS_XY, MAX_POS_XY, 
    MAP_HIT_MASK_MAPXY, MAP_HIT_MASK_SIDEDISTXY, MAP_HIT_OFFSET_MAPXY, MAP_HIT_OFFSET_SIDEDISTXY, 
    MAX_U32
} = require('./consts');

const tabDeltasFile = '../inc/tab_deltas.h';
const mapMatrixFile = '../src/map_matrix.c';
const outputFile = 'tab_map_hit_OUTPUT.txt';

const MAX_JOBS = 256;

// Progress tracking
const totalIterations = (MAX_POS_XY - MIN_POS_XY + 1) * (MAX_POS_XY - MIN_POS_XY + 1) * (1024/(1024/AP));
let completedIterations = 0;

function displayProgress () {
    const progress = (completedIterations / totalIterations) * 100;
    const progressBar = '='.repeat(Math.floor(progress / 2)) + '-'.repeat(50 - Math.floor(progress / 2));
    process.stdout.write(`\r[${progressBar}] ${progress.toFixed(2)}%`);
}

function calculateHitMapEntry (mapX, mapY, a, column, sideDistXY, mapXY) {
    const key = `[${mapX}][${mapY}][${a}][${column}]`;
    // At this point we know sideDistXY < 4096, mapXY < MAP_SIZE
    // The value is calculated in next layout: 
    //   16 bits:  mmmm    dddddddddddd
    //             mapXY    sideDistXY
    const value = ((sideDistXY & MAP_HIT_MASK_SIDEDISTXY) << MAP_HIT_OFFSET_SIDEDISTXY) 
                    | ((mapXY & MAP_HIT_MASK_MAPXY) << MAP_HIT_OFFSET_MAPXY) 
    return { key, value };
}

// Processing function to be run inside the worker thread
function processGameChunk (workerId, startPosX, endPosX, tab_deltas, map) {
    const outputHitMap = new Map();
    const posStepping = 1;

    // NOTE: here we are moving from the most UPPER-LEFT position of the map[][] layout, 
    // stepping DOWN into Y Axis, and RIGHT into X Axis, where in each position we do a full rotation.
    // Therefore we only interesting in collisions with x+1 and y+1.

    for (let posX = startPosX; posX <= endPosX; posX += posStepping) {

        for (let posY = MIN_POS_XY; posY <= MAX_POS_XY; posY += posStepping) {

            // Current location normalized
            let x = Math.floor(posX / FP);
            let y = Math.floor(posY / FP);

            // Limit Y axis location normalized
            const ytop = Math.floor((posY - (MAP_FRACTION-1)) / FP);
            const ybottom = Math.floor((posY + (MAP_FRACTION-1)) / FP);

            // Check X axis collision
            // Moving right as per map[][] layout?
            if (map[y*MAP_SIZE + (x+1)] || map[ytop*MAP_SIZE + (x+1)] || map[ybottom*MAP_SIZE + (x+1)]) {
                if (posX > ((x+1)*FP - MAP_FRACTION)) {
                    // Move one block of map: (FP + 2*MAP_FRACTION) = 384 units. The block sizes FP, but we account for a safe distant to avoid clipping.
                    posX += (FP + 2*MAP_FRACTION) - posStepping;
                    // Send progress update to main thread
                    parentPort.postMessage({ type: 'progress', value: (MAX_POS_XY - posY + 1) * (1024/(1024/AP)) });
                    // Stop current Y and continue with next X until it gets outside the collision
                    break;
                }
            }
            // Moving left as per map[][] layout?
            // else if (map[y*MAP_SIZE + (x-1)] || map[ytop*MAP_SIZE + (x-1)] || map[ybottom*MAP_SIZE + (x-1)]) {
            //     if (posX < (x*FP + MAP_FRACTION)) {
            //         // Move one block of map: (FP + 2*MAP_FRACTION) = 384 units. The block sizes FP, but we account for a safe distant to avoid clipping.
            //         posX -= (FP + 2*MAP_FRACTION) + posStepping;
            //         // Send progress update to main thread
            //         parentPort.postMessage({ type: 'progress', value: (MAX_POS_XY - posY + 1) * (1024/(1024/AP)) });
            //         // Stop current Y and continue with next X until it gets outside the collision
            //         break;
            //     }
            // }

            // Limit X axis location normalized
            const xleft = Math.floor((posX - (MAP_FRACTION-1)) / FP);
            const xright = Math.floor((posX + (MAP_FRACTION-1)) / FP);

            // Check Y axis collision
            // Moving down as per map[][] layout?
            if (map[(y+1)*MAP_SIZE + x] || map[(y+1)*MAP_SIZE + xleft] || map[(y+1)*MAP_SIZE + xright]) {
                if (posY > ((y+1)*FP - MAP_FRACTION)) {
                    // Move one block of map: (FP + 2*MAP_FRACTION) = 384 units. The block sizes FP, but we account for a safe distant to avoid clipping.
                    posY += (FP + 2*MAP_FRACTION) - posStepping;
                    // Send progress update to main thread
                    parentPort.postMessage({ type: 'progress', value: (1024/(1024/AP)) });
                    // Continue with next Y until it gets outside the collision
                    continue;
                }
            }
            // Moving up as per map[][] layout?
            // else if (map[(y-1)*MAP_SIZE + x] || map[(y-1)*MAP_SIZE + xleft] || map[(y-1)*MAP_SIZE + xright]) {
            //     if (posY < (y*FP + MAP_FRACTION)) {
            //         // Move one block of map: (FP + 2*MAP_FRACTION) = 384 units. The block sizes FP, but we account for a safe distant to avoid clipping.
            //         posY -= (FP + 2*MAP_FRACTION) + posStepping;
            //         // Send progress update to main thread
            //         parentPort.postMessage({ type: 'progress', value: (1024/(1024/AP)) });
            //         // Continue with next Y until it gets outside the collision
            //         continue;
            //     }
            // }

            for (let angle = 0; angle < 1024; angle += (1024/AP)) {

                // DDA (Digital Differential Analyzer)

                const sideDistX_l0 = posX - (Math.floor(posX / FP) * FP);
                const sideDistX_l1 = (Math.floor(posX / FP) + 1) * FP - posX;
                const sideDistY_l0 = posY - (Math.floor(posY / FP) * FP);
                const sideDistY_l1 = (Math.floor(posY / FP) + 1) * FP - posY;

                let a = Math.floor(angle / (1024/AP));

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

                    for (let n = 0; n < STEP_COUNT; ++n) {
                        // side X
                        if (sideDistX < sideDistY) {
                            mapX += stepX;
                            let hit = map[mapY * MAP_SIZE + mapX];
                            if (hit != 0) {
                                // Here we found a hit for side X when user is at (posX/FP,posY/FP,a,column).
                                const entry = calculateHitMapEntry(Math.floor(posX/FP), Math.floor(posY/FP), a, column, sideDistX, mapY);
                                outputHitMap.set(entry.key, entry.value);
                                break;
                            }
                            sideDistX += deltaDistX;
                        }
                        // side Y
                        else {
                            mapY += stepY;
                            let hit = map[mapY * MAP_SIZE + mapX];
                            if (hit != 0) {
                                // Here we found a hit for side Y when user is at (posX/FP,posY/FP,a,column).
                                const entry = calculateHitMapEntry(Math.floor(posX/FP), Math.floor(posY/FP), a, column, sideDistY, mapX);
                                outputHitMap.set(entry.key, entry.value);
                                break;
                            }
                            sideDistY += deltaDistY;
                        }
                    }
                }
            }

            //global.gc(false); // Not sure if false sets a Minor GC call but it works fine.

            // Send progress update to main thread
            parentPort.postMessage({ type: 'progress', value: (1024/(1024/AP)) });
        }
    }

    return outputHitMap;
}

function checkFinalSize (map) {
    const expectedCount = MAP_SIZE * MAP_SIZE * (1024/(1024/AP)) * PIXEL_COLUMNS;
    if (map.size != expectedCount)
        throw new Error(`Sanity check failed: hit map size is ${map.size} instead of ${expectedCount}`);
}

function toIndex (key) {
    const match = key.match(/\[(\d+)\]\[(\d+)\]\[(\d+)\]\[(\d+)\]/);
    if (!match) {
        throw new Error('Invalid key format');
    }

    const [, mapX, mapY, a, column] = match.map(Number);

    if (mapX < 0 || mapX >= MAP_SIZE ||
            mapY < 0 || mapY >= MAP_SIZE ||
            a < 0 || a >= (1024/(1024/AP)) ||
            column < 0 || column >= PIXEL_COLUMNS) {
        throw new Error('Invalid key format or values out of range. key: ' + key);
    }

    const index = ((mapX * MAP_SIZE + mapY) * (1024/(1024/AP)) + a) * PIXEL_COLUMNS + column;
    return index;
}

function getMinIndex (map) {
    let minIndex = MAX_U32;
    for (const [key, value] of map.entries()) {
        const index = toIndex(key);
        if (index < minIndex)
            minIndex = index;
    }
    return minIndex;
}

function getMaxIndex (map) {
    let maxIndex = 0;
    for (const [key, value] of map.entries()) {
        const index = toIndex(key);
        if (index > maxIndex)
            maxIndex = index;
    }
    return maxIndex;
}

function convertToFullArray (map) {
    const minIndex = getMinIndex(map);
    const maxIndex = getMaxIndex(map);
    const finalArraySize = (maxIndex - minIndex) + 1;
    console.log(`min calculated index: ${minIndex}`);
    console.log(`max calculated index: ${maxIndex}`);
    console.log(`final array size: (${maxIndex} - ${minIndex}) + 1 = ${finalArraySize}`);
    const fullArray = new Array(finalArraySize).fill(0);

    for (const [key, value] of map.entries()) {
        const index = toIndex(key);
        fullArray[index - minIndex] = value;
    }

    return fullArray;
}

function writeOutputToFile (arr, outputFile) {
    const lines = [];

    // Take groups of PIXEL_COLUMNS elems
    for (let i = 0; i < arr.length; i += PIXEL_COLUMNS) {
        const limit = Math.min(i + PIXEL_COLUMNS, arr.length);
        const line = arr.slice(i, limit).join(',');
        lines.push(line);
    }

    // Join lines with newline character
    const content = lines.join(',\n');

    fs.writeFileSync(outputFile, content);
}

// Function to run parallel workers
function runWorkers () {
    return new Promise((resolve, reject) => {

        const numCores = Math.min(16, os.cpus().length);
        const tab_deltas = utils.readTabDeltas(tabDeltasFile);
        const map = utils.readMapMatrix(mapMatrixFile);
        const jobQueue = [];
        const activeWorkers = new Set();
        let completedJobs = 0;
        const results = [];

        console.log('(Invoke with: node --max-old-space-size=4092 <script.js> to avoid running out of mem)');
        console.log(`Utilizing ${numCores} cores for processing multiple jobs.`);
        let startTimeStr = new Date().toLocaleString('en-US', { hour12: false });
        console.log(startTimeStr);

        // Create job queue
        const chunkSize = Math.ceil((MAX_POS_XY - MIN_POS_XY + 1) / MAX_JOBS);
        for (let startPosX = MIN_POS_XY; startPosX <= MAX_POS_XY; startPosX += chunkSize) {
            const endPosX = Math.min(startPosX + chunkSize - 1, MAX_POS_XY);
            jobQueue.push({ startPosX, endPosX });
        }

        // Total jobs to complete
        const totalJobs = jobQueue.length;
        console.log(`Main thread: Total jobs to complete: ${totalJobs}`);
        
        console.log('Main thread: Waiting for all workers to complete their jobs...');

        // Reset iterations counter
        completedIterations = 0;

        // Initial progress display
        displayProgress();
        // Set up progress display interval
        const progressInterval = setInterval(displayProgress, 4000);

        function startWorker() {
            if (jobQueue.length === 0 || activeWorkers.size >= numCores) return;

            const job = jobQueue.shift();
            const workerId = `[${job.startPosX}-${job.endPosX}]`;

            const worker = new Worker(__filename, {
                workerData: { workerId, ...job, tab_deltas, map }
            });

            activeWorkers.add(worker);

            worker.on('message', (message) => {
                if (message.type === 'progress') {
                    completedIterations += message.value;
                } else {
                    // Job completed
                    activeWorkers.delete(worker);
                    completedJobs++;

                    results.push(message);
                    
                    if (completedJobs === totalJobs) {
                        finishProcessing();
                    } else {
                        startWorker(); // Start a new job if available
                    }
                }
            });

            worker.on('error', (error) => {
                process.stdout.write('\n'); // Move to the next line after progress bar
                console.error(`Worker ${workerId} error:`, error);
                activeWorkers.delete(worker);
                reject(error);
            });
        }

        function finishProcessing() {
            clearInterval(progressInterval); // Stop progress display
            displayProgress(); // Last update with updated counter
            process.stdout.write('\n'); // Move to the next line after progress bar
            resolve(results);
        }

        // Start initial batch of workers
        for (let i = 0; i < numCores; i++) {
            startWorker();
        }
    });
}

// if (!global.gc) {
//     console.log('Garbage collection unavailable. Pass --expose-gc when launching node to enable forced garbage collection.');
//     return;
// }

if (isMainThread) {
    runWorkers()
        .then((results) => {
            const finalMap = new Map();
            results.forEach(resultMap => {
                for (const [key, value] of resultMap) {
                    finalMap.set(key, value);
                }
            });

            // Sanity check on size
            //checkFinalSize(finalMap);

            // Complete the array of values with the missing elements (those unaccessible map regions)
            const completeOutputArray = convertToFullArray(finalMap);

            // Write output to file
            writeOutputToFile(completeOutputArray, outputFile);
            console.log('Processing complete. Output saved to ' + outputFile);

            let endTimeStr = new Date().toLocaleString('en-US', { hour12: false });
            console.log(endTimeStr);
        })
        .catch((error) => {
            process.stdout.write('\n'); // Move to the next line after progress bar
            console.error('An error occurred:', error);

            let endTimeStr = new Date().toLocaleString('en-US', { hour12: false });
            console.log(endTimeStr);
        });
} else {
    const { workerId, startPosX, endPosX, tab_deltas, map } = workerData;
    const result = processGameChunk(workerId, startPosX, endPosX, tab_deltas, map);
    parentPort.postMessage(result);
}