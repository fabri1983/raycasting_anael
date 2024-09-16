const fs = require('fs');
const utils = require('./utils');

const inputFile = '../inc/tab_color_d8.h';
const outputFileX = 'tab_color_d8_x_with_pal_OUTPUT.txt';
const outputFileY = 'tab_color_d8_y_with_pal_OUTPUT.txt';

// Check correct values of constants before script execution. See consts.h.
const { PAL0, PAL1, PAL2, PAL3, TILE_ATTR_PALETTE_SFT } = require('./consts');

const ELEMENTS_PER_LINE = 512;

// Create 2D matrix with palette
function create2DMatrixWithPal (inputArray, palFunc) {
    return Array(2).fill().map((_, n) =>
        inputArray.map(elem => elem | (palFunc(n) << TILE_ATTR_PALETTE_SFT))
    );
}

// Write output to file
function writeAsArray (filename, matrix) {
    const content = matrix.map(row => {
        const lines = [];
        for (let i = 0; i < row.length; i += ELEMENTS_PER_LINE) {
            lines.push(`${row.slice(i, i + ELEMENTS_PER_LINE).join(',')},`);
        }
        let lineContent = lines.join('\n');
        return `${lineContent}`
    }).join('\n');
    fs.writeFileSync(filename, content);
}

// Main execution
try {
    const { tab_color_d8_x, tab_color_d8_y } = utils.readTabColorD8Tables(inputFile);

    const tab_color_d8_x_with_pal = create2DMatrixWithPal(tab_color_d8_x, n => n & 1 === 1 ? PAL2 : PAL0);
    const tab_color_d8_y_with_pal = create2DMatrixWithPal(tab_color_d8_y, n => n & 1 === 1 ? PAL3 : PAL1);

    writeAsArray(outputFileX, tab_color_d8_x_with_pal);
    writeAsArray(outputFileY, tab_color_d8_y_with_pal);

    console.log('Processing completed successfully.');
} catch (error) {
    console.error('An error occurred:', error.message);
}