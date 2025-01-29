const fs = require('fs');
const { Worker, isMainThread, parentPort, workerData } = require('worker_threads');
const os = require('os');
const utils = require('./utils');
// Check correct values of constants before script execution. See consts.h.
const {
    FS, FP, AP, STEP_COUNT_LOOP, PIXEL_COLUMNS, TILEMAP_COLUMNS, VERTICAL_ROWS, MAP_SIZE, MAP_FRACTION, 
    MIN_POS_XY, MAX_POS_XY, TILE_ATTR_PALETTE_SFT, PAL0, PAL1 } = require('./consts');

const tabDeltasFile = '../inc/tab_deltas.h';
const mapMatrixFile = '../src/map_matrix.c';
const outputFile = 'tiles_pair_OUTPUT.txt';

const MAX_JOBS = 256;

// Progress tracking
const totalIterations = (MAX_POS_XY - MIN_POS_XY + 1) * (MAX_POS_XY - MIN_POS_XY + 1) * (1024/(1024/AP));
let completedIterations = 0;

function displayProgress () {
    const progress = (completedIterations / totalIterations) * 100;
    const progressBar = '='.repeat(Math.floor(progress / 2)) + '-'.repeat(50 - Math.floor(progress / 2));
    process.stdout.write(`\r[${progressBar}] ${progress.toFixed(2)}%`);
}

function trackTilePairsBetweenPlanes (framebuffer_planeA, framebuffer_planeB, tilePairMap) {
    for (let i = 0; i < VERTICAL_ROWS*TILEMAP_COLUMNS; i++) {
        // Construct the key using the bitmask 0x7FF which only keeps the tile index data of the framebuffer plane entry
        const key = `${framebuffer_planeA[i] & 0x7FF}-${framebuffer_planeB[i] & 0x7FF}`;
        tilePairMap.set(key, 1);
    }
}

// Processing function to be run inside the worker thread
function processGameChunk (workerId, startPosX, endPosX, tab_deltas, tab_wall_div, tab_color_d8_1, map) {
    const framebuffer_planeA = new Uint16Array(VERTICAL_ROWS*TILEMAP_COLUMNS);
    const framebuffer_planeB = new Uint16Array(VERTICAL_ROWS*TILEMAP_COLUMNS);
    const tilePairMap = new Map();
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
                    // Move one block of map: (FP + 2*MAP_FRACTION) = 384 units. The block size is FP, but we account for a safe distant to avoid clipping.
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
            //         // Move one block of map: (FP + 2*MAP_FRACTION) = 384 units. The block size is FP, but we account for a safe distant to avoid clipping.
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
                    // Move one block of map: (FP + 2*MAP_FRACTION) = 384 units. The block size is FP, but we account for a safe distant to avoid clipping.
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
            //         // Move one block of map: (FP + 2*MAP_FRACTION) = 384 units. The block size is FP, but we account for a safe distant to avoid clipping.
            //         posY -= (FP + 2*MAP_FRACTION) + posStepping;
            //         // Send progress update to main thread
            //         parentPort.postMessage({ type: 'progress', value: (1024/(1024/AP)) });
            //         // Continue with next Y until it gets outside the collision
            //         continue;
            //     }
            // }

            for (let angle = 0; angle < 1024; angle += (1024/AP)) {

                utils.clean_framebuffer(framebuffer_planeA);
                utils.clean_framebuffer(framebuffer_planeB);

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

                    for (let n = 0; n < STEP_COUNT_LOOP; ++n) {
                        // side X
                        if (sideDistX < sideDistY) {
                            mapX += stepX;
                            const hit = map[mapY * MAP_SIZE + mapX];
                            if (hit != 0) {
                                const h2 = tab_wall_div[sideDistX]; // height halved

                                const d8_1 = tab_color_d8_1[sideDistX]; // the bigger the distant the darker the color is
                                let tileAttrib;
                                if (mapY&1) tileAttrib = (PAL0 << TILE_ATTR_PALETTE_SFT) | (d8_1 + (8*8)); // use the tiles that point to second half of wall's palette
                                else tileAttrib = (PAL0 << TILE_ATTR_PALETTE_SFT) | d8_1;

                                let framebuffer = framebuffer_planeA;
                                if ((column % 2) == 1)
                                    framebuffer = framebuffer_planeB;
                                utils.write_vline(h2, tileAttrib, framebuffer, column/2);

                                break;
                            }
                            sideDistX += deltaDistX;
                        }
                        // side Y
                        else {
                            mapY += stepY;
                            const hit = map[mapY * MAP_SIZE + mapX];
                            if (hit != 0) {
                                const h2 = tab_wall_div[sideDistY]; // height halved

                                const d8_1 = tab_color_d8_1[sideDistY]; // the bigger the distant the darker the color is
                                let tileAttrib;
                                if (mapX&1) tileAttrib = (PAL1 << TILE_ATTR_PALETTE_SFT) + d8_1 + (8*8); // use the tiles that point to second half of wall's palette
                                else tileAttrib = (PAL1 << TILE_ATTR_PALETTE_SFT) + d8_1;

                                let framebuffer = framebuffer_planeA;
                                if ((column % 2) == 1)
                                    framebuffer = framebuffer_planeB;
                                utils.write_vline(h2, tileAttrib, framebuffer, column/2);

                                break;
                            }
                            sideDistY += deltaDistY;
                        }
                    }
                }

                // traverse both framebuffers and track the generated pairs between each entry
                trackTilePairsBetweenPlanes(framebuffer_planeA, framebuffer_planeB, tilePairMap);
            }

            //global.gc(false); // Not sure if false sets a Minor GC call but it works fine.

            // Send progress update to main thread
            parentPort.postMessage({ type: 'progress', value: (1024/(1024/AP)) });
        }
    }

    return tilePairMap;
}

function writeOutputToFile (finalMap, outputFile) {
    const mapEntries = Array.from(finalMap);

    // Sort the entries by key (numberA-numberB)
    mapEntries.sort((a, b) => {
        // Split the keys into numberA and numberB
        const [aNumA, aNumB] = a[0].split('-').map(Number);
        const [bNumA, bNumB] = b[0].split('-').map(Number);

        // Compare numberA first
        if (aNumA !== bNumA) {
            return aNumA - bNumA; // Sort by numberA ascending
        }
        // If numberA is the same, compare numberB
        return aNumB - bNumB; // Sort by numberB ascending
    });

    let outputString = '';
    for (const [key, value] of mapEntries) {
        outputString += `${key}\n`; // Format: "key"
    }

    fs.writeFileSync(outputFile, outputString, 'utf8');
}

// Function to run parallel workers
function runWorkers () {
    return new Promise((resolve, reject) => {

        const numCores = Math.min(16, os.cpus().length);
        const tab_deltas = utils.readTabDeltas(tabDeltasFile);
        const tab_wall_div = utils.generateTabWallDiv();
        const tab_color_d8_1 = utils.generateTabColor_d8_1();
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
                workerData: { workerId, ...job, tab_deltas, tab_wall_div, tab_color_d8_1, map }
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
            // Process all the maps gathered from each execution of processGameChunk()
            const finalMap = new Map();
            results.forEach(resultMap => {
                for (const [key, value] of resultMap) {
                    finalMap.set(key, value);
                }
            });

            // Write output to file
            writeOutputToFile(finalMap, outputFile);
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
    const { workerId, startPosX, endPosX, tab_deltas, tab_wall_div, tab_color_d8_1, map } = workerData;
    const result = processGameChunk(workerId, startPosX, endPosX, tab_deltas, tab_wall_div, tab_color_d8_1, map);
    parentPort.postMessage(result);
}