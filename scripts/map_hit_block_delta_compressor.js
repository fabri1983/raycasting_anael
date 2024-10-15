const fs = require('fs');
const { PIXEL_COLUMNS } = require('./consts');

const BLOCK_SIZE = PIXEL_COLUMNS; // Number of elements per block

const inputFile = 'tab_map_hit_OUTPUT.txt';

function compressMatrix(matrix) {
    const compressedMatrix = [];
    const blocks = Math.ceil(matrix.length / BLOCK_SIZE);

    for (let i = 0; i < blocks; i++) {
        const start = i * BLOCK_SIZE;
        const end = Math.min(start + BLOCK_SIZE, matrix.length);
        const block = matrix.slice(start, end);

        // Store the first value of each block as-is
        compressedMatrix.push(block[0]);

        // Delta encode the rest of the block
        for (let j = 1; j < block.length; j++) {
            const delta = block[j] - block[j - 1];
            compressedMatrix.push(delta);
        }
    }

    return compressedMatrix;
}

function decompressElement(compressedMatrix, index) {
    const blockIndex = Math.floor(index / BLOCK_SIZE);
    const blockStart = blockIndex * BLOCK_SIZE;
    const elementIndex = index % BLOCK_SIZE;

    let value = compressedMatrix[blockStart];
    for (let i = 1; i <= elementIndex; i++) {
        value += compressedMatrix[blockStart + i];
    }

    return value;
}

function loadMatrix(filename) {
    const content = fs.readFileSync(filename, 'utf8');
    const rows = content.trim().split('\n');
    return rows.flatMap(row => row.split(',').filter(x => x !== '' && !isNaN(x)).map(Number));
}

// Function to format number with right alignment and decimal alignment
function formatNumber(num, integerWidth, decimalPlaces = 0) {
    const [integerPart, decimalPart] = num.toFixed(decimalPlaces).split('.');
    const formattedInteger = integerPart.padStart(integerWidth);
    if (decimalPlaces > 0) {
        return `${formattedInteger}.${decimalPart}`;
    }
    return formattedInteger;
} 

function main() {
    // Load the matrix
    const matrix = loadMatrix(inputFile);
    console.log(`Matrix has ${matrix.length} elems`);

    // Compress the matrix
    const compressedMatrix = compressMatrix(matrix);

    // Calculate and print compression ratio
    const originalSizeBytes = matrix.length * 2; // 2 bytes per 16-bit value
    const compressedMatrixSizeBytes = compressedMatrix.length * 2;
    const compressionRatio = (1 - compressedMatrixSizeBytes / originalSizeBytes) * 100;

    // Calculate the maximum width needed for the integer part
    const maxIntegerWidth = Math.max(
        originalSizeBytes.toString().length,
        compressedMatrixSizeBytes.toString().length,
        Math.floor(compressionRatio).toString().length
    );

    console.log(`${'Original size:'.padEnd(42)}${formatNumber(originalSizeBytes, maxIntegerWidth)} bytes`);
    console.log(`${'CompressedMatrix size:'.padEnd(42)}${formatNumber(compressedMatrixSizeBytes, maxIntegerWidth)} bytes`);
    console.log(`${'Compression ratio:'.padEnd(42)}${formatNumber(compressionRatio, maxIntegerWidth, 2)}%`);

    let mismatchFound = false;
    for (let row = 0; row < Math.floor(matrix.length / PIXEL_COLUMNS) && !mismatchFound; row++) {
        for (let col = 0; col < PIXEL_COLUMNS && !mismatchFound; col++) {
            const index = row * PIXEL_COLUMNS + col;
            const originalValue = matrix[index];
            const decompressedValue = decompressElement(compressedMatrix, index);

            if (originalValue !== decompressedValue) {
                console.error(`ERROR: Mismatch at row ${row}, column ${col} (index ${index}) -> Original: ${originalValue}, Decompressed: ${decompressedValue}`);
                mismatchFound = true;
            }
        }
    }
}

main();