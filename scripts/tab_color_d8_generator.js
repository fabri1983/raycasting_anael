const fs = require('fs');
const utils = require('./utils');
// Check correct values of constants before script execution. See consts.h.
const {  } = require('./consts');

const ELEMENTS_PER_LINE = 8;

const outputFileX = 'tab_color_d8_OUTPUT.txt';

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
    const tab_color_d8 = utils.generateTabColorD8();
    writeArray(outputFileX, tab_color_d8);

    console.log('Processing completed successfully.');
} catch (error) {
    console.error('An error occurred:', error.message);
}