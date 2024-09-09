const fs = require('fs');

const inputFile = '../inc/tab_color_d8.h';
const outputFileX = 'tab_color_d8_x_with_pal_OUTPUT.txt';
const outputFileY = 'tab_color_d8_y_with_pal_OUTPUT.txt';

// Check correct values of constants before script execution. See consts.h.
const { PAL0, PAL1, PAL2, PAL3, TILE_ATTR_PALETTE_SFT } = require('./consts');

const ELEMENTS_PER_LINE = 512;

// Read and parse the input file
function readInputFile(filename) {
    const content = fs.readFileSync(filename, 'utf8');
    
    function parseArray(name) {
        const regex = new RegExp(`const\\s+u16\\s+${name}\\s*\\[.*?\\]\\s*=\\s*\\{([^}]+)\\}`, 's');
        const match = content.match(regex);
        if (!match) {
            throw new Error(`Failed to find array ${name} in input file`);
        }
        return match[1].split(',')
            .map(x => x.trim())
            .filter(x => x !== '')
            .map(x => parseInt(x, 10));
    }

    return {
        tab_color_d8_x: parseArray('tab_color_d8_x'),
        tab_color_d8_y: parseArray('tab_color_d8_y')
    };
}

// Create 2D matrix with palette
function create2DMatrixWithPal(inputArray, palFunc) {
    return Array(2).fill().map((_, n) =>
        inputArray.map(elem => elem | (palFunc(n) << TILE_ATTR_PALETTE_SFT))
    );
}

// Write output to file
function writeOutputFile(filename, matrix) {
    const content = matrix.map(row => {
        const lines = [];
        for (let i = 0; i < row.length; i += ELEMENTS_PER_LINE) {
            lines.push(`${row.slice(i, i + ELEMENTS_PER_LINE).join(',')},`);
        }
        let lineContent = lines.join('\n');
        return `{${lineContent}},`
    }).join('\n');
    fs.writeFileSync(filename, content);
}

// Main execution
try {
    const { tab_color_d8_x, tab_color_d8_y } = readInputFile(inputFile);

    const tab_color_d8_x_with_pal = create2DMatrixWithPal(tab_color_d8_x, n => n & 1 === 1 ? PAL1 : PAL0);
    const tab_color_d8_y_with_pal = create2DMatrixWithPal(tab_color_d8_y, n => n & 1 === 1 ? PAL3 : PAL2);

    writeOutputFile(outputFileX, tab_color_d8_x_with_pal);
    writeOutputFile(outputFileY, tab_color_d8_y_with_pal);

    console.log('Processing completed successfully.');
} catch (error) {
    console.error('An error occurred:', error.message);
}