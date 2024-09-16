const fs = require('fs');
const utils = require('./utils');

const inputFile = '../inc/tab_deltas.h';
const outputFile = 'tab_deltas_3_OUTPUT.txt';

function calculateRayDirection (rayDirX, rayDirY) {
    // rayDix < 0 and rayDirY < 0
    if (utils.get16BitSign(rayDirX) < 0 && utils.get16BitSign(rayDirY) < 0)
        return 0;
    // rayDix < 0 and rayDirY > 0
    if (utils.get16BitSign(rayDirX) < 0 && utils.get16BitSign(rayDirY) > 0)
        return 1;
    // rayDix > 0 and rayDirY < 0
    if (utils.get16BitSign(rayDirX) > 0 && utils.get16BitSign(rayDirY) < 0)
        return 2;
    // rayDix > 0 and rayDirY > 0
    if (utils.get16BitSign(rayDirX) > 0 && utils.get16BitSign(rayDirY) > 0)
        return 3;
    return -1
}

async function processFile () {
    const tab_deltas = utils.readTabDeltas(inputFile);
    const outputLines = [];
    let asm_block_size_factor = 12; // jump block size wise

    for (let c = 0; c < tab_deltas.length; c+=4) {
        var rayDirection = calculateRayDirection(tab_deltas[c+2], tab_deltas[c+3]);
        outputLines.push(`${tab_deltas[c+0]}, ${tab_deltas[c+1]}, ${rayDirection}*${asm_block_size_factor},`);
    }

    // Write output to file
    fs.writeFileSync(outputFile, outputLines.join('\n'));
    console.log('Processing complete. Output saved to ' + outputFile);
}

processFile().catch(error => {
    console.error(`An error occurred: ${error.message}`);
});