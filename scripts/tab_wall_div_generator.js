const fs = require('fs');

const outputFile = 'tab_wall_div_OUTPUT.txt';
// Check correct values of constants before script execution. See consts.h.
const { FP, STEP_COUNT, VERTICAL_ROWS, TILEMAP_COLUMNS, MAX_U8 } = require('./consts');

// Vertical height calculation starts at the center
const WALL_H2 = (VERTICAL_ROWS * 8) / 2;

// Initialize the tab_wall_div array
let tab_wall_div = new Array(FP * (STEP_COUNT + 1));

// Populate the tab_wall_div array
for (let i = 0; i < tab_wall_div.length; i++) {
    let v = (TILEMAP_COLUMNS * FP) / (i + 1);
    let div = Math.round(Math.min(v, MAX_U8));
    if (div >= WALL_H2) {
        tab_wall_div[i] = 0;
    } else {
        tab_wall_div[i] = WALL_H2 - div;
    }
}

// Generate content for tab_wall_div.txt with lines of up to 8 elements
let fileContent = '';
for (let i = 0; i < tab_wall_div.length; i++) {
    fileContent += tab_wall_div[i];
    if ((i + 1) % 8 === 0) {
        fileContent += ',\n';  // Newline after every 8 elements
    } else if (i !== tab_wall_div.length - 1) {
        fileContent += ', ';  // Comma between elements
    }
}

fs.writeFile(outputFile, fileContent, (err) => {
    if (err) throw err;
    console.log(outputFile + ' has been saved!');
});