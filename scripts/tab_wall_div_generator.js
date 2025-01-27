const fs = require('fs');

const outputFile = 'tab_wall_div_OUTPUT.txt';
// Check correct values of constants before script execution. See consts.h.
const utils = require('./utils');

const tab_wall_div = utils.generateTabWallDiv();

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