const fs = require('fs');
const { PIXEL_COLUMNS } = require('./consts');

const inputFile = 'tab_map_hit_OUTPUT.txt';
const outputFile = 'map_hit_rle_OUTPUT.txt';

class RLEEntry {
    constructor(value, count) {
        this.value = value;
        this.count = count;
    }
}

class RLEMatrix {
    constructor(rows, cols, data, rowOffsets, totalEntries) {
        this.rows = rows;
        this.cols = cols;
        this.data = data;
        this.rowOffsets = rowOffsets;
        this.totalEntries = totalEntries;
    }
}

function compressMatrix(matrix, rows, cols) {
    const compressed = new RLEMatrix(rows, cols, [], [], 0);
    let dataIndex = 0;

    for (let i = 0; i < rows; i++) {
        compressed.rowOffsets[i] = dataIndex;
        let currentValue = matrix[i * cols + 0];
        let count = 1;

        for (let j = 1; j < cols; j++) {
            if (matrix[i * cols + j] === currentValue && count < 65535) {
                count++;
            } else {
                compressed.data[dataIndex] = new RLEEntry(currentValue, count);
                dataIndex++;
                currentValue = matrix[i * cols + j];
                count = 1; // Changed to regular number
            }
        }

        compressed.data[dataIndex] = new RLEEntry(currentValue, count);
        dataIndex++;
    }

    compressed.rowOffsets[rows] = dataIndex;
    compressed.totalEntries = dataIndex;

    return compressed;
}

function decompressElement(matrix, row, col) {
    const start = matrix.rowOffsets[row];
    const end = matrix.rowOffsets[row + 1];
    let currentCol = 0;

    for (let i = start; i < end; i++) {
        if ((currentCol + matrix.data[i].count) > col) {
            return matrix.data[i].value;
        }
        currentCol += matrix.data[i].count;
    }

    console.error("Error: Column index not found");
    return 0;
}

function readMatrixFromFile(filename) {
    try {
        const content = fs.readFileSync(filename, 'utf8');
        const rows = content.trim().split('\n');
        return rows.flatMap(row => row.split(',').filter(x => x !== '' && !isNaN(x)).map(Number));
    } catch (err) {
        console.error('Error reading file:', err);
        process.exit(1);
    }
}

function saveToHeader(matrix, filename) {
    let output = '';
    const variableName = 'map_hit_rle';

    // Write RLEEntry array
    output += `static const RLEEntry ${variableName}_data[] = {\n`;
    for (let i = 0; i < matrix.totalEntries; i++) {
        output += `    {${matrix.data[i].value}, ${matrix.data[i].count}},\n`;
    }
    output += '};\n\n';

    // Write row_offsets array
    output += `static const uint32_t ${variableName}_row_offsets[] = {\n`;
    for (let i = 0; i <= matrix.rows; i++) {
        output += `    ${matrix.rowOffsets[i]},\n`;
    }
    output += '};\n\n';

    // Write RLEMatrix structure
    output += `static const RLEMatrix ${variableName} = {\n`;
    output += `    .rows = ${matrix.rows},\n`;
    output += `    .cols = ${matrix.cols},\n`;
    output += `    .data = (RLEEntry*)${variableName}_data,\n`;
    output += `    .row_offsets = (uint32_t*)${variableName}_row_offsets,\n`;
    output += `    .total_entries = ${matrix.totalEntries}\n`;
    output += '};';

    fs.writeFileSync(filename, output);
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
    const matrix = readMatrixFromFile(inputFile);
    console.log(`Matrix has ${matrix.length} elems`);

    const rows = Math.floor(matrix.length / PIXEL_COLUMNS);
    const cols = PIXEL_COLUMNS;
    const compressedMatrix = compressMatrix(matrix, rows, cols);

    // Calculate sizes in bytes
    const originalSizeBytes = rows * cols * 2; // Assuming each element is 2 bytes (uint16_t)
    const compressedMatrixSizeBytes = compressedMatrix.totalEntries * 4 + (compressedMatrix.rowOffsets.length * 4);
    const compression = (1 - (compressedMatrixSizeBytes / originalSizeBytes)) * 100;

    // Calculate the maximum width needed for the integer part
    const maxIntegerWidth = Math.max(
        originalSizeBytes.toString().length,
        compressedMatrixSizeBytes.toString().length,
        Math.floor(compression).toString().length
    );

    console.log(`${'Original size:'.padEnd(42)}${formatNumber(originalSizeBytes, maxIntegerWidth)} bytes`);
    console.log(`${'Compressed size:'.padEnd(42)}${formatNumber(compressedMatrixSizeBytes, maxIntegerWidth)} bytes`);
    console.log(`${'Compression:'.padEnd(42)}${formatNumber(compression, maxIntegerWidth, 2)}%`);

    // Print some additional information about the compressed matrix
    console.log(`Total entries in compressed matrix: ${compressedMatrix.totalEntries}`);
    console.log(`Number of row offsets: ${compressedMatrix.rowOffsets.length}`);

    let mismatchFound = false;
    for (let row = 0; row < (matrix.length / PIXEL_COLUMNS) && !mismatchFound; row++) {
        for (let col = 0; col < PIXEL_COLUMNS && !mismatchFound; col++) {
            const index = row * PIXEL_COLUMNS + col;
            const originalValue = matrix[index];
            const decompressedValue = decompressElement(compressedMatrix, row, col);

            if (originalValue !== decompressedValue) {
                console.error(`ERROR: Mismatch at row ${row}, column ${col} (index ${index}) -> Original: ${originalValue}, Decompressed: ${decompressedValue}`);
                mismatchFound = true;
            }
        }
    }

    if (!mismatchFound) {
        saveToHeader(compressedMatrix, outputFile);
        console.log('Processing complete. Output saved to ' + outputFile);
    }
}

main();
