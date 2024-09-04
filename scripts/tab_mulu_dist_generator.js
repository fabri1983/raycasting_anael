const fs = require('fs');

const inputFile = '../src/tab_deltas.h';
const outputFile = 'tab_mulu_dist.txt';

// Constants
const FS = 8; // fixed point size in bits
const FP = (1 << FS); // fixed precision
const AP = 128; // angle precision
const MAP_SIZE = 16;
const DELTA_DIST_VALUES = 60; // this value same than the one in tab_mulu_dist_div256.h

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

    if (tab_deltas.length !== AP * 64 * 4) {
        throw new Error(`Invalid number of elements in tab_deltas (expected ${AP * 64 * 4}): ${tab_deltas.length}`);
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

// Main processing function
function processTabDeltas() {
    const tab_deltas = readTabDeltas();
    const outputMap = new Map(); // Using a Map to keep track of the largest value for each [N][M]

    for (let posX = 0; posX <= FP; posX += 1) {
        for (let angle = 0; angle < 1024; angle += 8) {
			
			var posY = posX;

            const sideDistX_l0 = (posX - Math.floor(posX / FP) * FP);
            const sideDistX_l1 = ((Math.floor(posX / FP) + 1) * FP - posX);
			const sideDistY_l0 = (posY - Math.floor(posY / FP) * FP);
            const sideDistY_l1 = ((Math.floor(posY / FP) + 1) * FP - posY);

            let a = Math.floor(angle / (1024 / AP));
			const aa = a * DELTA_DIST_VALUES;
            a *= 256;

            for (let c = 0; c < 64; ++c) {
                const deltaDistX = tab_deltas[a + c*4 + 0];
                const deltaDistY = tab_deltas[a + c*4 + 1];
                const rayDirX = toSigned16Bit(tab_deltas[a + c*4 + 2]);
                const rayDirY = toSigned16Bit(tab_deltas[a + c*4 + 3]);

                let sideDistX, stepX;
                let keyX = null;

                if (rayDirX < 0) {
                    stepX = -1;
                    sideDistX = (mulu(sideDistX_l0, deltaDistX) >> FS) & 0xFFFF;
                    keyX = `[${sideDistX_l0}][${aa + c}]`;
                } else {
                    stepX = 1;
                    sideDistX = (mulu(sideDistX_l1, deltaDistX) >> FS) & 0xFFFF;
                    keyX = `[${sideDistX_l1}][${aa + c}]`;
                }

                // keep the line with biggest value
                if (keyX != null && (!outputMap.has(keyX) || outputMap.get(keyX) < sideDistX)) {
                    outputMap.set(keyX, sideDistX);
                }

                let sideDistY, stepY, stepYMS;
                let keyY;

                if (rayDirY < 0) {
                    stepY = -1;
                    stepYMS = -MAP_SIZE; // Note: MAP_SIZE is not defined in the provided context
                    sideDistY = (mulu(sideDistY_l0, deltaDistY) >> FS) & 0xFFFF;
					keyY = `[${sideDistY_l0}][${aa + c}]`;
                } else {
                    stepY = 1;
                    stepYMS = MAP_SIZE; // Note: MAP_SIZE is not defined in the provided context
                    sideDistY = (mulu(sideDistY_l1, deltaDistY) >> FS) & 0xFFFF;
					keyY = `[${sideDistY_l1}][${aa + c}]`;
                }

                // keep the line with biggest value
                if (keyY != null && (!outputMap.has(keyY) || outputMap.get(keyY) < sideDistY)) {
                    outputMap.set(keyY, sideDistY);
                }
            }
        }
    }

    // Convert Map to Array and sort in ASC order
    const sortedOutputLines = Array.from(outputMap.entries()).sort((a, b) => {
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
}

// Run the processing
try {
    processTabDeltas();
} catch (error) {
    console.error('An error occurred:', error.message);
}