const fs = require('fs');
const utils = require('./utils');
// Check correct values of constants before script execution. See consts.h.
const { PAL0, PAL1, PAL2, PAL3, TILE_ATTR_PALETTE_SFT } = require('./consts');

const ELEMENTS_PER_LINE = 512;

const outputFileX = 'tab_color_d8_1_X_pals_shft_OUTPUT.txt';
const outputFileY = 'tab_color_d8_1_Y_pals_shft_OUTPUT.txt';

// First half contains elems with func palFunc(0), other half contains elems with palFunc(1)
function transformArray (inputArray, palFunc) {
    const firstHalf = inputArray.map(elem => palFunc(0, elem));
    const secondHalf = inputArray.map(elem => palFunc(1, elem));
    return [...firstHalf, ...secondHalf];
}

// Interleave the elements of first half with the elems of second half
function interleaveArray (inputArray) {
    const halfLength = inputArray.length / 2;
    const result = [];

    for (let i = 0; i < halfLength; i++) {
        result.push(inputArray[i]);
        result.push(inputArray[i + halfLength]);
    }

    return result;
}

// Write output to file
function writeArray (filename, inputArray) {
    const lines = [];
    for (let i = 0; i < inputArray.length; i += ELEMENTS_PER_LINE) {
        lines.push(`${inputArray.slice(i, i + ELEMENTS_PER_LINE).join(',')},`);
    }
    let linesContent = lines.join('\n');

    fs.writeFileSync(filename, linesContent);
    console.log(`File ${filename} created.`);
}

// Main execution
try {
    const tab_color_d8_1 = utils.generateTabColor_d8_1();

    // This is what we are pre calculating:
    //    u16 d8_1 = tab_color_d8_1[sideDistX];
    //    u16 color;
    //    if (mapY&1) color = (PAL0 << TILE_ATTR_PALETTE_SFT) + d8_1 + (8*8); // use the tiles that point to second half of wall's palette
    //    else color = (PAL0 << TILE_ATTR_PALETTE_SFT) + d8_1;
    const tab_color_d8_1_X_pals_shft = transformArray(tab_color_d8_1, 
        (n,d8_1) => (PAL0 << TILE_ATTR_PALETTE_SFT) + d8_1 + ((n & 1) === 1 ? 8*8 : 0));
    
    // This is what we are pre calculating:
    //    u16 d8_1 = tab_color_d8_1[sideDistY];
    //    u16 color;
    //    if (mapX&1) color = (PAL1 << TILE_ATTR_PALETTE_SFT) + d8_1 + (8*8); // use the tiles that point to second half of wall's palette
    //    else color = (PAL1 << TILE_ATTR_PALETTE_SFT) + d8_1;
    const tab_color_d8_1_Y_pals_shft = transformArray(tab_color_d8_1, 
        (n,d8_1) => (PAL1 << TILE_ATTR_PALETTE_SFT) + d8_1 + ((n & 1) === 1 ? 8*8 : 0));

    const interleaved_X = interleaveArray(tab_color_d8_1_X_pals_shft);
    const interleaved_Y = interleaveArray(tab_color_d8_1_Y_pals_shft);

    writeArray(outputFileX, interleaved_X);
    writeArray(outputFileY, interleaved_Y);

    console.log('Processing completed successfully.');
} catch (error) {
    console.error('An error occurred:', error.message);
}