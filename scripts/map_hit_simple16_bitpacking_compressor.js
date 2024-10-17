const fs = require('fs');
// Check correct values of constants before script execution. See consts.h.
const { PIXEL_COLUMNS } = require('./consts');

const inputFile = 'tab_map_hit_OUTPUT.txt';

const CASES = [
    { count: 28, bits: [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1] },
    { count: 21, bits: [2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0] },
    { count: 21, bits: [1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0] },
    { count: 21, bits: [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0] },
    { count: 14, bits: [2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0] },
    { count: 9, bits: [4, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0] },
    { count: 8, bits: [3, 4, 4, 4, 4, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0] },
    { count: 7, bits: [4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0] },
    { count: 6, bits: [5, 5, 5, 5, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0] },
    { count: 6, bits: [4, 4, 5, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0] },
    { count: 5, bits: [6, 6, 6, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0] },
    { count: 5, bits: [5, 5, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0] },
    { count: 4, bits: [7, 7, 7, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0] },
    { count: 3, bits: [10, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0] },
    { count: 2, bits: [14, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0] },
    { count: 1, bits: [28, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0] }
];

class ValueOutOfRange extends Error {
    constructor() {
        super('Value out of range for simple16. Maximum value is 268435455');
        this.name = 'ValueOutOfRange';
    }
}

const MAX = 268435455;

class Simple16 {
    static check(data) {
        for (let value of data) {
            if (value > MAX) {
                throw new ValueOutOfRange();
            }
        }
    }

    static as(value) {
        return value;
    }
}

class U32Simple16 extends Simple16 {
    static as(value) {
        return value & 0xFFFFFFFF; // Ensure unsigned 32-bit integer
    }
}

class U64Simple16 extends Simple16 {
    static as(value) {
        return Number(value) >>> 0;
    }
}

class U16Simple16 extends Simple16 {
    static as(value) {
        return value & 0xFFFF;
    }
}

class U8Simple16 extends Simple16 {
    static as(value) {
        return value & 0xFF;
    }
}

function consume(values, valFrom, valTo) {
    let selector = 0;
    while (true) {
        let continueNextSelector = false;
        const { count: caseCount, bits } = CASES[selector];
        let count = Math.min(caseCount, valTo - valFrom);
        for (let j = 0; j < count; j++) {
            const valuesJ = U16Simple16.as(values[valFrom + j]);
            if (valuesJ >= (1 << bits[j])) {
                selector++;
                continueNextSelector = true;
                break;
            }
        }
        if (!continueNextSelector)
            return count;
    }
}

function calculateSizeInBytes(data) {
    Simple16.check(data);
    return calculateSizeInBytesUnchecked(data);
}

function calculateSizeInBytesUnchecked(data) {
    let size = 0;
    let valFrom = 0;
    const valTo = data.length;

    while (valFrom < valTo) {
        const count = consume(data, valFrom, valTo);
        valFrom += count;
        size += 4;
    }
    return size;
}

function deltaEncode(values) {
    const deltas = [];
    deltas.push(values[0]); // First value remains unchanged
    for (let i = 1; i < values.length; i++) {
        deltas.push(values[i] - values[i - 1]); // Store the difference between consecutive values
    }
    return deltas;
}

function deltaDecode(deltas) {
    const values = [];
    values.push(deltas[0]);  // The first value is unchanged
    for (let i = 1; i < deltas.length; i++) {
        values.push(values[i - 1] + deltas[i]);  // Add the delta to the previous value
    }
    return values;
}

class HuffmanNode {
    constructor(value, frequency) {
        this.value = value;          // Value or character
        this.frequency = frequency;  // Frequency of occurrence
        this.left = null;           // Left child
        this.right = null;          // Right child
    }
}

function buildHuffmanTree(frequencies) {
    const nodes = [];
    for (const [value, freq] of frequencies) {
        nodes.push(new HuffmanNode(value, freq));
    }
    //const nodes = frequencies.entries.map(([value, freq]) => new HuffmanNode(value, freq));
    
    while (nodes.length > 1) {
        nodes.sort((a, b) => a.frequency - b.frequency);
        
        const left = nodes.shift();   // Get the two nodes with the lowest frequency
        const right = nodes.shift();
        
        const merged = new HuffmanNode(null, left.frequency + right.frequency);
        merged.left = left;
        merged.right = right;
        
        nodes.push(merged);           // Add merged node back to the list
    }
    
    return nodes[0];  // Return the root of the tree
}

function generateCodes(node, prefix = '', codes = {}) {
    if (!node) return;
    if (node.value !== null) {
        codes[node.value] = prefix;  // Leaf node, store the code
    }
    generateCodes(node.left, prefix + '0', codes);
    generateCodes(node.right, prefix + '1', codes);
    return codes;
}

function encodeWithHuffman(values, codes) {
    let encoded = '';
    for (const value of values) {
        encoded += codes[value];  // Append the code for each value
    }
    return encoded;  // Return the concatenated string of bits
}

function packAndHuffman(values, valFrom, valTo, huffmanCodes) {
    let bestSelector = -1;
    let bestCount = 0;
    let bestValue = 0;
    let bestHuffmanEncodedValue = [];

    for (let selector = 0; selector < CASES.length; selector++) {
        let value = U32Simple16.as(selector << 28);
        const { count: caseCount, bits } = CASES[selector];
        let count = Math.min(caseCount, valTo - valFrom);
        let packed = 0;
        let packedValuesForHuffman = [];
        let continueNextSelector = false;

        for (let j = 0; j < count; j++) {
            const v = U16Simple16.as(values[valFrom + j]);
            const bitsJ = bits[j];
            if (v >= (1 << bitsJ)) {
                // If value doesn't fit in current selector, skip to the next one
                continueNextSelector = true;
                break;
            }
            value |= U32Simple16.as(v << packed);
            packed += bitsJ;
            packedValuesForHuffman.push(v);  // Store packed values for Huffman encoding
        }

        // Apply Huffman encoding to packed values
        const huffmanEncodedValue = encodeWithHuffman(packedValuesForHuffman, huffmanCodes);

        // If we successfully packed the values, check if it's better than the current best case
        if (!continueNextSelector && count > bestCount) {
            bestSelector = selector;
            bestCount = count;
            bestValue = value;
            bestHuffmanEncodedValue = huffmanEncodedValue;
        }
    }

    if (bestSelector === -1) {
        throw new Error("No suitable selector found for packing.");
    }

    return { value: bestHuffmanEncodedValue, count: bestCount };
}

function pack(values, valFrom, valTo) {
    let bestSelector = -1;
    let bestCount = 0;
    let bestValue = 0;
    
    for (let selector = 0; selector < CASES.length; selector++) {
        let value = U32Simple16.as(selector << 28);
        const { count: caseCount, bits } = CASES[selector];
        let count = Math.min(caseCount, valTo - valFrom);
        let packed = 0;
        let continueNextSelector = false;

        for (let j = 0; j < count; j++) {
            const v = U16Simple16.as(values[valFrom + j]);
            const bitsJ = bits[j];
            if (v >= (1 << bitsJ)) {
                // If value doesn't fit in current selector, skip to the next one
                continueNextSelector = true;
                break;
            }
            value |= U32Simple16.as(v << packed);
            packed += bitsJ;
        }

        // If we successfully packed the values, check if it's better than the current best case
        if (!continueNextSelector && count > bestCount) {
            bestSelector = selector;
            bestCount = count;
            bestValue = value;
        }
    }

    if (bestSelector === -1) {
        throw new Error("No suitable selector found for packing.");
    }

    return { value: bestValue, count: bestCount };
}

function compressUnchecked(values) {
    const out = [];
    let valFrom = 0;
    const valTo = values.length;

    while (valFrom < valTo) {
        const { value, count } = pack(values, valFrom, valTo);
        valFrom += count;
        out.push(value);
    }

    return out;
}

function compressWithHuffmanUnchecked(values) {
    const out = [];
    let valFrom = 0;
    const valTo = values.length;

    // Perform frequency analysis
    const frequencies = new Map();
    for (const value of values) {
        const freq = (frequencies.get(value) || 0) + 1;  // Count frequency of each value
        frequencies.set(value, freq);
    }

    // Build Huffman Tree and generate codes
    const huffmanTree = buildHuffmanTree(frequencies);
    const huffmanCodes = generateCodes(huffmanTree);

    while (valFrom < valTo) {
        const { value, count } = packAndHuffman(values, valFrom, valTo, huffmanCodes);
        valFrom += count;
        out.push(value);
    }

    return out;
}

function compressAsBytesUnchecked(values) {
    const out = [];
    let valFrom = 0;
    const valTo = values.length;

    while (valFrom < valTo) {
        const { value, count } = pack(values, valFrom, valTo);
        valFrom += count;
        out.push((value & 0xff000000) >> 24);
        out.push((value & 0x00ff0000) >> 16);
        out.push((value & 0x0000ff00) >> 8);
        out.push((value & 0x000000ff));
    }
    return out;
}

function compressWithLookupUnchecked(values) {
    const compressedData = [];
    const rowLookupTable = []; // Maps each row to its compressed block index
    const offsetTable = []; // Stores bit offsets for each value within a compressed block

    let valFrom = 0;
    const valTo = values.length;

    // Step through each block of PIXEL_COLUMNS values
    while (valFrom < valTo) {
        // Pack the values starting from valFrom
        const { value, count } = pack(values, valFrom, valTo);

        // Store the starting compressed block index for the current row
        if (valFrom % PIXEL_COLUMNS === 0) {
            rowLookupTable.push(compressedData.length);
        }

        // Add the packed value to the compressed data array
        compressedData.push(value);

        // Step 2: Calculate bit offsets for each packed value within this block
        const selector = (value >>> 28);
        const { bits } = CASES[selector];
        const offsets = [];
        let packedOffset = 0;

        // Calculate the offset for each packed value based on its bit size
        for (let j = 0; j < bits.length; j++) {
            offsets.push(packedOffset);
            packedOffset += bits[j];
        }

        // Store the calculated offsets for this block
        offsetTable.push(offsets);

        // Move to the next set of values
        valFrom += count;
    }

    return { compressed: compressedData, rowLookupTable, offsetTable };
}

function compress(values) {
    Simple16.check(values);
    return compressUnchecked(values);
}

function compressWithDelta(values) {
    Simple16.check(values);
    const deltas = deltaEncode(values);
    return compressUnchecked(deltas);
}

function compressAsBytes(values) {
    Simple16.check(values);
    return compressAsBytesUnchecked(values);
}

function compressWithLookup(values) {
    Simple16.check(values);
    return compressWithLookupUnchecked(values);
}

function decompress(compressed) {
    const out = []
    let offset = 0;

    while (offset < compressed.length) {
        const next = compressed[offset];
        ++offset;
        const selector = (next >>> 28);
        const { count, bits } = CASES[selector];
        let unpacked = 0;
        for (let j = 0; j < count; j++) {
            const bitsJ = bits[j];
            const mask = 0xFFFFFFFF >>> (32 - bitsJ);
            const value = (next >> unpacked) & mask;
            out.push(value);
            unpacked += bitsJ;
        }
    }

    return out;
}

function decompressWithDelta(compressed) {
    const decompressedValues = decompress(compressed);
    return deltaDecode(decompressedValues);
}

function decompressFromBytes(bytes) {
    const out = [];
    if (bytes.length % 4 !== 0) {
        throw new Error('Invalid byte array length');
    }

    let offset = 0;
    while (offset < bytes.length) {
        const next = bytes.readUInt32LE(offset);
        offset += 4;
        const selector = (next >>> 28);
        const { count, bits } = CASES[selector];
        let unpacked = 0;
        for (let j = 0; j < count; j++) {
            const bitsJ = bits[j];
            const mask = 0xFFFFFFFF >>> (32 - bitsJ);
            const value = (next >> unpacked) & mask;
            out.push(value);
            unpacked += bitsJ;
        }
    }

    return out;
}

function decompressElementA(compressedData, rowLookupTable, offsetTable, row, col) {
    // Find the compressed block that contains this row
    const blockIndex = rowLookupTable[row];
    const compressedBlock = compressedData[blockIndex];

    // Find the bit offset for the value within this compressed block
    const selector = (compressedBlock >>> 28);
    const { bits } = CASES[selector];

    // Get the corresponding offset for the column
    const bitOffset = offsetTable[blockIndex][col];
    const bitsForValue = bits[col];

    // Unpack the value at the bit offset
    const mask = 0xFFFFFFFF >>> (32 - bitsForValue);
    const value = (compressedBlock >>> bitOffset) & mask;

    return U16Simple16.as(value);
}

function decompressElementB(compressed, rowLookupTable, offsetTable, row, col) {
    // Get the compressed block for the given row
    const blockIndex = rowLookupTable[row];
    const block = compressed[blockIndex];

    // Get the bit offset for the value at the specified column
    const bitOffset = offsetTable[row][col];

    // Extract the selector from the compressed block
    const selector = block >>> 28;
    const { bits } = CASES[selector];

    // Find the bit size for this column's value
    const bitsForValue = bits[col];

    // Create a mask to extract the value
    const mask = (1 << bitsForValue) - 1;

    // Shift the block to the correct bit offset and apply the mask
    const value = (block >>> bitOffset) & mask;

    return value;
}

function loadMatrix(filename) {
    const content = fs.readFileSync(filename, 'utf8');
    const rows = content.trim().split('\n');
    return rows.flatMap(row => row.split(',').filter(x => x !== '' && !isNaN(x)).map(Number));
}

function printMinAndMaxValuesWithSizeBits(array) {
    let min = array[0];
    let max = array[0];

    for (let i = 1; i < array.length; i++) {
        if (array[i] < min) {
            min = array[i];
        }
        if (array[i] > max) {
            max = array[i];
        }
    }

    const getBitsRequired = (num) => {
    if (num === 0) return 1; // 0 requires 1 bit to represent
        return Math.floor(Math.log2(Math.abs(num))) + 1 + (num < 0 ? 1 : 0);
    };

    const minBits = getBitsRequired(min);
    const maxBits = getBitsRequired(max);

    console.log(`Minimum value: ${min} (${minBits} bits)`);
    console.log(`Maximum value: ${max} (${maxBits} bits)`);
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
    printMinAndMaxValuesWithSizeBits(matrix);

    // Compress the matrix
    const { compressed: compressedMatrix, rowLookupTable, offsetTable } = compressWithLookup(matrix);

    // Calculate and print compression ratio
    const originalSizeBytes = matrix.length * 2; // 2 bytes per 16-bit value
    const compressedMatrixSizeBytes = compressedMatrix.length * 4; // 4 bytes
    const rowLookupTableSizeBytes = rowLookupTable.length * 2; // 2 bytes
    const offsetTableSizeBytes = offsetTable.length * 1; // 1 byte
    const compression = (1 - (compressedMatrixSizeBytes + rowLookupTableSizeBytes + offsetTableSizeBytes) / originalSizeBytes) * 100;

    // Calculate the maximum width needed for the integer part
    const maxIntegerWidth = Math.max(
        originalSizeBytes.toString().length,
        compressedMatrixSizeBytes.toString().length,
        rowLookupTableSizeBytes.toString().length,
        offsetTableSizeBytes.toString().length,
        Math.floor(compression).toString().length
    );

    console.log(`${'Original size:'.padEnd(42)}${formatNumber(originalSizeBytes, maxIntegerWidth)} bytes`);
    console.log(`${'CompressedMatrix size:'.padEnd(42)}${formatNumber(compressedMatrixSizeBytes, maxIntegerWidth)} bytes`);
    console.log(`${'RowLookupTable size:'.padEnd(42)}${formatNumber(rowLookupTableSizeBytes, maxIntegerWidth)} bytes`);
    console.log(`${'OffsetTable size:'.padEnd(42)}${formatNumber(offsetTableSizeBytes, maxIntegerWidth)} bytes`);
    console.log(`${'Compressed size:'.padEnd(42)}${formatNumber(compressedMatrixSizeBytes + rowLookupTableSizeBytes + offsetTableSizeBytes, maxIntegerWidth)} bytes`);
    console.log(`${'Compression:'.padEnd(42)}${formatNumber(compression, maxIntegerWidth, 2)}%`);

    console.log("Checking for any mismatch between original matrix and decompressed matrix ...");

    const uncompressedMatrix = decompress(compressedMatrix);
    let mismatchFound = false;
    for (let row = 0; row < Math.floor(matrix.length / PIXEL_COLUMNS) && !mismatchFound; row++) {
        for (let col = 0; col < PIXEL_COLUMNS && !mismatchFound; col++) {
            const index = row * PIXEL_COLUMNS + col;
            const originalValue = matrix[index];
            const decompressedValue = uncompressedMatrix[index];

            if (originalValue !== decompressedValue) {
                console.error(`ERROR: Mismatch at row ${row}, column ${col} (index ${index}) -> Original: ${originalValue}, Decompressed: ${decompressedValue}`);
                mismatchFound = true;
            }
        }
    }

    if (!mismatchFound)
        console.log("Done.");

    console.log("Checking index by index for any mismatch between original matrix and decompressed matrix ...");

    mismatchFound = false;
    for (let row = 0; row < Math.floor(matrix.length / PIXEL_COLUMNS) && !mismatchFound; row++) {
        for (let col = 0; col < PIXEL_COLUMNS && !mismatchFound; col++) {
            const index = row * PIXEL_COLUMNS + col;
            const originalValue = matrix[index];
            const decompressedValue = decompressElementB(compressedMatrix, rowLookupTable, offsetTable, row, col);

            if (originalValue !== decompressedValue) {
                console.error(`ERROR: Mismatch at row ${row}, column ${col} (index ${index}) -> Original: ${originalValue}, Decompressed: ${decompressedValue}`);
                mismatchFound = true;
            }
        }
    }

    if (!mismatchFound)
        console.log("Done.");
}

main();