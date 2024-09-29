const fs = require('fs');
const path = require('path');
const utils = require('./utils');
// Check correct values of constants before script execution. See consts.h.
const { FS } = require('./consts');

const MPH_VALUES_DELTADIST_NKEYS = 915;  // Number of keys (must be 915)
const OP1_MAX_VALUE = 256;

const tabDeltasFile = '../inc/tab_deltas.h';
const outputFile = 'perf_hash_mulu_256_shft_FS_OUTPUT.txt';

// Export the constants, table, and function for use in other scripts
module.exports = {
    MPH_VALUES_DELTADIST_NKEYS,
    loadDeltaDists,
    generateKeyToIndexMap
};

function loadDeltaDists (filePath) {
    const tab_deltas = utils.readTabDeltas(filePath);

    // Read only every 4 columns since that's where the deltaDistX is located (deltaDistY has same values)
    const result = new Set();
    for (let i = 0; i < tab_deltas.length; i += 4) {
        result.add(tab_deltas[i]);
    }
    
    const deltaDists = Array.from(result).sort((a, b) => a - b);
    if (deltaDists.length !== MPH_VALUES_DELTADIST_NKEYS) {
        throw new Error(`File has ${deltaDists.length} values, expected ${MPH_VALUES_DELTADIST_NKEYS}`);
    }

    return deltaDists;
}

/**
 * Create a Map where the key is the key from the sortedKeys and the value is the index.
 * The resulting map acts as a Minimal Perfect Hash function.
 * map[key] guaranties a value from 0 to MPH_VALUES_DELTADIST_NKEYS - 1.
 * @param {*} sortedKeys 
 * @returns a map that acts as a Minimal Perfect Hash function.
 */
function generateKeyToIndexMap (sortedKeys) {
    const keyToIndexMap = new Map();
    for (let i = 0; i < sortedKeys.length; i++) {
        keyToIndexMap.set(sortedKeys[i], i);
    }
    return keyToIndexMap;
}

function generateProducts () {
    const deltaDists = loadDeltaDists(tabDeltasFile);
    const keyToIndex = generateKeyToIndexMap(deltaDists);
    let expectedLength = (OP1_MAX_VALUE + 1) * MPH_VALUES_DELTADIST_NKEYS;
    const products = new Array(expectedLength);

    for (let op1 = 0; op1 <= OP1_MAX_VALUE; op1++) {
        for (let i = 0; i < deltaDists.length; ++i) {
            let op2 = deltaDists[i];
            let product = op1 * op2;
            let shiftedProduct = utils.toUnsigned16Bit(product >> FS);
            let op2_index = keyToIndex.get(op2); // this is indeed the minimal perfect hash function
            let prod_index = (op1 * MPH_VALUES_DELTADIST_NKEYS) + op2_index;
            products[prod_index] = shiftedProduct;
        }
    }

    // Set undefined positions in the products array to 0
    for (let i = 0; i < products.length; i++) {
        if (products[i] === undefined) {
            products[i] = 0;
        }
    }

    // If this happens then is because the perf() function is giving numbers outside the range [0, MPH_VALUES_DELTADIST_NKEYS]
    if (products.length != expectedLength)
        throw new Error(`products[] has ${products.length} values, expected ${expectedLength}`);

    return products;
}

function checkCorrectness (products) {
    const deltaDists = loadDeltaDists(tabDeltasFile);
    const keyToIndex = generateKeyToIndexMap(deltaDists);

    for (let op1 = 0; op1 <= OP1_MAX_VALUE; op1++) {
        for (let i = 0; i < deltaDists.length; ++i) {
            let op2 = deltaDists[i];
            let product = op1 * op2;
            let shiftedProduct = utils.toUnsigned16Bit(product >> FS);
            let op2_index = keyToIndex.get(op2); // this is indeed the minimal perfect hash function
            let prod_index = (op1 * MPH_VALUES_DELTADIST_NKEYS) + op2_index;
            if (products[prod_index] != shiftedProduct)
                throw new Error(`products[]: incorrect value ${products[prod_index]} for (op1*op2) >> FS at location (${op1} * ${MPH_VALUES_DELTADIST_NKEYS}) + ${op2_index}, expected ${shiftedProduct}`);
        }
    }
}

/*function saveProductsToFile (products, outputFile) {
    const filePath = path.resolve(outputFile);
    const fileStream = fs.createWriteStream(filePath);

    const chunkSize = Math.ceil(products.length / 4);

    for (let chunk = 0; chunk < 4; chunk++) {
        const start = chunk * chunkSize;
        const end = Math.min((chunk + 1) * chunkSize, products.length);

        fileStream.write('{\n');

        for (let i = start; i < end; i++) {
            let line = '';
            for (let j = 0; j < MPH_VALUES_DELTADIST_NKEYS && i < end; j++, i++) {
                line += products[i].toString() + ',';
            }
            fileStream.write(line + '\n');
            i--; // Adjust for the extra increment in the inner loop
        }

        fileStream.write('}\n\n');
    }

    fileStream.end();
    console.log(`products[] saved to file in 4 chunks: ${outputFile}`);
}*/

function saveProductsToFile (products, outputFile) {
    const filePath = path.resolve(outputFile);
    const fileStream = fs.createWriteStream(filePath);

    let line = '';
    for (let i = 0; i < products.length; i++) {
        line += products[i].toString() + ',';
        if ((i + 1) % MPH_VALUES_DELTADIST_NKEYS === 0) {
            fileStream.write(line + '\n');
            line = '';
        }
    }

    fileStream.end();
    console.log(`products[] saved to file: ${outputFile}`);
}

async function calculateOutput () {
    try {
        const products = generateProducts();
        checkCorrectness(products);
        saveProductsToFile(products, outputFile);
    } catch (err) {
        console.error('Error:', err.message);
    }
}

// Conditionally run the main function if the script is executed directly
if (require.main === module) {
    calculateOutput().catch(error => {
        console.error(`An error occurred: ${error.message}`);
    });
}