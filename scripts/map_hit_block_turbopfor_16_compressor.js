/**
 * TurboPFor block-based delta encoding with bit packing.
 * 
 * Data is divided into blocks of size PIXEL_COLUMNS * 4.
 * For each block:
 *  - A base value (minimum in the block) is stored.
 *  - Differences from the base are calculated.
 *  - The optimal number of bits to represent these differences is determined.
 *  - Differences are bit-packed using this optimal bit width.
 * Lookup table to access each compressed block into the compressed array.
 * Duplicate blocks are removed to save space and lookup table updated accordingly.
 */

const fs = require('fs');
// Check correct values of constants before script execution. See consts.h.
const { MAX_U16, PIXEL_COLUMNS } = require('./consts');

const BLOCK_SIZE = PIXEL_COLUMNS*4; // BLOCK_SIZE >= PIXEL_COLUMNS otherwise we need to adjust map_hit_compressed.c
const MAX_BITS = 16; // Maximum bits for a 16-bit unsigned integer

const inputFile = 'tab_map_hit_OUTPUT.txt';
const outputFile = 'map_hit_block_turbopfor_OUTPUT.txt';

function removeDuplicatedBlocks(compressedMatrix, blockLookupIndex) {
    const uniqueBlocks = new Map();
    const newCompressedMatrix = [];
    const newBlockLookupIndex = [];

    for (let i = 0; i < blockLookupIndex.length; i++) {
        const start = blockLookupIndex[i];
        const end = i < blockLookupIndex.length - 1 ? blockLookupIndex[i + 1] : compressedMatrix.length;
        const block = compressedMatrix.slice(start, end);
        const blockKey = block.join(',');

        if (uniqueBlocks.has(blockKey)) {
            newBlockLookupIndex.push(uniqueBlocks.get(blockKey));
        } else {
            const newStart = newCompressedMatrix.length;
            newCompressedMatrix.push(...block);
            newBlockLookupIndex.push(newStart);
            uniqueBlocks.set(blockKey, newStart);
        }
    }

    return { compressedMatrix: newCompressedMatrix, blockLookupIndex: newBlockLookupIndex };
}

function determineOptimalBits(block) {
    let maxValue = 0;
    for (const val of block) {
        if (val > maxValue) maxValue = val;
    }

    for (let bits = 1; bits <= MAX_BITS; bits++) {
        if (maxValue < (1 << bits)) {
            return bits;
        }
    }
    return MAX_BITS;
}

function compressMatrix(matrix) {
    const compressedMatrix = [];
    const blockLookupIndex = [];
    const numBlocks = Math.ceil(matrix.length / BLOCK_SIZE);

    for (let i = 0; i < numBlocks; i++) {
        const start = i * BLOCK_SIZE;
        const end = Math.min(start + BLOCK_SIZE, matrix.length);
        const block = matrix.slice(start, end);

        // Saves the location in compressedMatrix where this blocks starts at
        blockLookupIndex.push(compressedMatrix.length);

        // Gets the smaller value of the block
        let base = block[0];
        for (const val of block) {
            if (val < base) base = val;
        }
        const bits = determineOptimalBits(block.map(v => v - base));

        compressedMatrix.push(base);
        compressedMatrix.push(bits);

        let compressed = 0;
        let usedBits = 0;

        block.forEach((value, index) => {
            const diff = value - base;
            compressed |= (diff << usedBits);
            usedBits += bits;

            // Write the value if the bits span over 16 or we've reached the last item in the block
            if (usedBits >= 16 || index === block.length - 1) {
                compressedMatrix.push(compressed & 0xFFFF); // Push lower 16 bits
                compressed >>>= 16;
                usedBits -= 16;
            }
        });

        // If there's any leftover compressed data, push it
        if (usedBits > 0) {
            compressedMatrix.push(compressed & 0xFFFF);
        }
    }

    const { compressedMatrix: dedupMatrix, blockLookupIndex: dedupLookupIndex } = 
        removeDuplicatedBlocks(compressedMatrix, blockLookupIndex);

    return { compressedMatrix: dedupMatrix, blockLookupIndex: dedupLookupIndex };
}

function decompressElement(compressedMatrix, blockLookupIndex, index) {
    const blockIndex = Math.floor(index / BLOCK_SIZE);
    const elementIndex = index % BLOCK_SIZE;

    const blockStart = blockLookupIndex[blockIndex];
    const base = compressedMatrix[blockStart];
    const bits = compressedMatrix[blockStart + 1];

    const totalBits = elementIndex * bits;
    const startWord = Math.floor(totalBits / 16) + blockStart + 2;
    const bitOffset = totalBits % 16;

    let value = (compressedMatrix[startWord] >>> bitOffset);
    if ((bitOffset + bits) > 16) {
        value |= compressedMatrix[startWord + 1] << (16 - bitOffset);
    }
    value &= (1 << bits) - 1;

    return base + (value & 0xFFFF);
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

function saveArraysToFile(compressedMatrix, blockLookupIndex, lookupByteFactor, filename) {
    let fileContent = '';
    fileContent += `#define MAP_HIT_COMPRESSED_BLOCK_SIZE (PIXEL_COLUMNS*${Math.floor(BLOCK_SIZE/PIXEL_COLUMNS)})\n`;
    fileContent += '\n';
    fileContent += 'const u16 map_hit_compressed[] = {\n';
    
    let nextBlockStart = 1;
    // Save compressedMatrix
    for (let i = 0; i < compressedMatrix.length; i++) {
        fileContent += compressedMatrix[i].toString();
        fileContent += ',';
        if (i === compressedMatrix.length - 1 
                || (nextBlockStart < blockLookupIndex.length && i == (blockLookupIndex[nextBlockStart] - 1))) {
            fileContent += '\n';
            ++nextBlockStart;
        }
    }

    fileContent += '};\n';
    fileContent += '\n';

    fileContent += '// values grouped by same delta (might be useful for further compression?)\n';
    if (lookupByteFactor === 2) {
        fileContent += 'const u16 map_hit_lookup[] = {\n';
    } else if (lookupByteFactor === 4) {
        fileContent += 'const u32 map_hit_lookup[] = {\n';
    } else {
        throw new Error('Invalid lookupByteFactor. Must be 2 or 4.');
    }

    // Save blockLookupIndex
    let currentDelta = null;
    let lineContent = '';

    function appendValue(value) {
        if (lookupByteFactor === 2) {
            // Ensure 16-bit unsigned integer
            value &= 0xFFFF;
        } else if (lookupByteFactor === 4) {
            // Ensure 32-bit unsigned integer
            value &= 0xFFFFFFFF;
        } else {
            throw new Error('Invalid lookupByteFactor. Must be 2 or 4.');
        }
        return value.toString() + ', ';
    }

    for (let i = 0; i < blockLookupIndex.length; i++) {
        let value = blockLookupIndex[i];
        
        if (i > 0) {
            let delta = value - blockLookupIndex[i - 1];
            
            if (delta !== currentDelta) {
                if (lineContent) {
                    fileContent += lineContent.trim() + '\n';
                }
                lineContent = '';
                currentDelta = delta;
            }
        }
        
        lineContent += appendValue(value);
    }

    if (lineContent) {
        fileContent += lineContent.trim() + '\n';
    }

    fileContent += '};';

    // Write to file
    fs.writeFileSync(filename, fileContent, 'utf8');
    console.log(`Content saved to ${filename}`);
}

function main() {
    // Load the matrix
    const matrix = loadMatrix(inputFile);
    console.log(`Matrix has ${matrix.length} elems`);

    // Compress the matrix
    const { compressedMatrix, blockLookupIndex } = compressMatrix(matrix);

    // Calculate and print compression ratio
    const originalSizeBytes = matrix.length * 2; // 2 bytes
    const compressedMatrixSizeBytes = compressedMatrix.length * 2; // 2 bytes
    const biggerLookupValue = Math.max(...blockLookupIndex);
    console.log(`Bigger LookupIndex table value: ${biggerLookupValue}`)
    let lookupByteFactor = 2;
    if (biggerLookupValue > MAX_U16)
        lookupByteFactor = 4;
    const blockLookupIndexSizeBytes = blockLookupIndex.length * lookupByteFactor; // 2 or 4 bytes
    const compression = (1 - (compressedMatrixSizeBytes + blockLookupIndexSizeBytes) / originalSizeBytes) * 100;

    // Calculate the maximum width needed for the integer part
    const maxIntegerWidth = Math.max(
        originalSizeBytes.toString().length,
        compressedMatrixSizeBytes.toString().length,
        blockLookupIndexSizeBytes.toString().length,
        Math.floor(compression).toString().length
    );

    console.log(`${'Original size:'.padEnd(42)}${formatNumber(originalSizeBytes, maxIntegerWidth)} bytes`);
    console.log(`${'CompressedMatrix size:'.padEnd(42)}${formatNumber(compressedMatrixSizeBytes, maxIntegerWidth)} bytes`);
    console.log(`${'Block Lookup Index size:'.padEnd(42)}${formatNumber(blockLookupIndexSizeBytes, maxIntegerWidth)} bytes`);
    console.log(`${'Compressed size + Block Lookup Index:'.padEnd(42)}${formatNumber(compressedMatrixSizeBytes + blockLookupIndexSizeBytes, maxIntegerWidth)} bytes`);
    console.log(`${'Compression:'.padEnd(42)}${formatNumber(compression, maxIntegerWidth, 2)}%`);

    let mismatchFound = false;
    for (let row = 0; row < Math.floor(matrix.length / BLOCK_SIZE) && !mismatchFound; row++) {
        for (let col = 0; col < BLOCK_SIZE && !mismatchFound; col++) {
            const index = row * BLOCK_SIZE + col;
            const originalValue = matrix[index];
            const decompressedValue = decompressElement(compressedMatrix, blockLookupIndex, index);

            if (originalValue !== decompressedValue) {
                console.error(`ERROR: Mismatch at row ${row}, column ${col} (index ${index}) -> Original: ${originalValue}, Decompressed: ${decompressedValue}`);
                mismatchFound = true;
            }
        }
    }

    if (!mismatchFound)
        saveArraysToFile(compressedMatrix, blockLookupIndex, lookupByteFactor, outputFile);
}

main();