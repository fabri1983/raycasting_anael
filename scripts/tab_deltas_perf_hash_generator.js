const fs = require('fs');
const utils = require('./utils');
const { loadDeltaDists, generateMinPerHashMap } = require('./perf_hash_mulu_256_shft_FS_generator');

const inputFile = '../inc/tab_deltas.h';
const outputFile = 'tab_deltas_perf_hash_OUTPUT.txt';

async function processFile () {
    const tab_deltas = utils.readTabDeltas(inputFile);
    const deltaDists = loadDeltaDists(inputFile);
    const minPerfHashMap = generateMinPerHashMap(deltaDists);
    const outputLines = [];
    const asmFactor = 'ASM_PERF_HASH_JUMP_BLOCK_SIZE_BYTES';

    for (let c = 0; c < tab_deltas.length; c+=4) {
        let ddxph = minPerfHashMap.get(tab_deltas[c+0]); // deltaDistX_perf_hash
        let ddyph = minPerfHashMap.get(tab_deltas[c+1]); // deltaDistY_perf_hash
        outputLines.push(
            `${tab_deltas[c+0]}, ${tab_deltas[c+1]}, ${tab_deltas[c+2]}, ${tab_deltas[c+3]}, ${ddxph}*${asmFactor}, ${ddyph}*${asmFactor},`);
    }

    // Write output to file
    fs.writeFileSync(outputFile, outputLines.join('\n'));
    console.log('Processing complete. Output saved to ' + outputFile);
}

processFile().catch(error => {
    console.error(`An error occurred: ${error.message}`);
});