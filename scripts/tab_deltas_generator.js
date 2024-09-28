const fs = require('fs');
// Check correct values of constants before script execution. See consts.h.
const { FP, AP, PIXEL_COLUMNS, M_PI, MAX_U16 } = require('./consts');

const outputFile = 'tab_deltas_OUTPUT.txt';

let tabDeltas = new Uint16Array(AP * PIXEL_COLUMNS * 4);
let deltaPtr = 0;

// precompute ray deltas for all angles, one axis is enough because of symmetry
for (let i = 0; i < AP; i++) {
    let a = (i * M_PI * 2) / AP;
    let sina = Math.sin(a);
    let cosa = Math.cos(a);
    let dx = sina * FP;
    let dy = cosa * FP;

    let rx1 = dx + dy;
    let ry1 = dy - dx;
    let rx2 = dx - dy;
    let ry2 = dy + dx;

    // interpolate rayDir and fill the delta table
    for (let x = 0; x < PIXEL_COLUMNS; x++) {
        let rayDirAngleX = rx1 + (rx2 - rx1) * (x + 0.5) / PIXEL_COLUMNS;
        let rayDirAngleY = ry1 + (ry2 - ry1) * (x + 0.5) / PIXEL_COLUMNS;
        let d, divisor;

        divisor = Math.abs(rayDirAngleX);
        d = (FP * FP) / Math.max(1, divisor);
        // from 182 up to 65535, but only 915 different values
        tabDeltas[deltaPtr] = Math.round(Math.min(d, MAX_U16)); 

        divisor = Math.abs(rayDirAngleY);
        d = (FP * FP) / Math.max(1, divisor);
        // from 182 up to 65535, but only 915 different values
        tabDeltas[deltaPtr + 1] = Math.round(Math.min(d, MAX_U16));

        // from 0 up to 65535, which actually are 717 signed different values: [-360, 360] (except few values due to precision lack)
        tabDeltas[deltaPtr + 2] = Math.round(rayDirAngleX) & 0xFFFF;
        tabDeltas[deltaPtr + 3] = Math.round(rayDirAngleY) & 0xFFFF;

        deltaPtr += 4;
    }
}

// Convert the Uint16Array to a string with the new format
let content = '';
for (let i = 0; i < tabDeltas.length; i += 4) {
    content += `${tabDeltas[i+0]}, ${tabDeltas[i+1]}, ${tabDeltas[i+2]}, ${tabDeltas[i+3]},\n`;
}

// Write to file
fs.writeFileSync(outputFile, content);

console.log('File "' + outputFile + '" has been created.');