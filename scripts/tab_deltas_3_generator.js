const fs = require('fs');

const inputFile = '../inc/tab_deltas.h';
const outputFile = 'tab_deltas_3_OUTPUT.txt';

// Check correct values of constants before script execution. See consts.h.
const { AP, PIXEL_COLUMNS } = require('./consts');

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

// Function to get the sign of a 16-bit integer
function get16BitSign(num) {
    // Check if the most significant bit is set (bit 15)
    return (num & 0x8000) ? -1 : 1;
}

function calculateRayDirection(rayDirX, rayDirY) {
    // rayDix < 0 and rayDirY < 0
    if (get16BitSign(rayDirX) < 0 && get16BitSign(rayDirY) < 0)
        return 0;
    // rayDix < 0 and rayDirY > 0
    if (get16BitSign(rayDirX) < 0 && get16BitSign(rayDirY) > 0)
        return 1;
    // rayDix > 0 and rayDirY < 0
    if (get16BitSign(rayDirX) > 0 && get16BitSign(rayDirY) < 0)
        return 2;
    // rayDix > 0 and rayDirY > 0
    if (get16BitSign(rayDirX) > 0 && get16BitSign(rayDirY) > 0)
        return 3;
    return -1
}

async function processFile() {
    const tab_deltas = readTabDeltas();
    const outputLines = [];

    for (let c = 0; c < tab_deltas.length; c+=4) {
        var rayDirection = calculateRayDirection(tab_deltas[c+2], tab_deltas[c+3]);
        outputLines.push(`${tab_deltas[c+0]},${tab_deltas[c+1]},${rayDirection},`);
    }

    // Write output to file
    fs.writeFileSync(outputFile, outputLines.join('\n'));
    console.log('Processing complete. Output saved to ' + outputFile);
}

processFile().catch(error => {
    console.error(`An error occurred: ${error.message}`);
});