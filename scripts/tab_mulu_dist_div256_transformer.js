/*
Expects a file named generated from tab_mulu_dist_div256_generator.js, with next format:
	[N][M]=x
	Eg:
	[0][32]=1265
	[0][33]=6855
	[0][34]=45
	...
	[1][0]=12
	[1][1]=256
	...
*/

const fs = require('fs');
const readline = require('readline');

const inputFile = 'tab_mulu_dist_div256_OUTPUT.txt';
const outputFile = 'tab_mulu_dist_div256_FULL.txt';

// Check correct values of constants before script execution. See consts.h.
const { AP, PIXEL_COLUMNS } = require('./consts');

const expectedCountM = (1024/(1024/AP))*PIXEL_COLUMNS;
const maxM = expectedCountM - 1;
const minM = 0;

async function processFile() {
    const fileStream = fs.createReadStream(inputFile);
    const rl = readline.createInterface({
        input: fileStream,
        crlfDelay: Infinity
    });

    let lines = [];

    // Read and store non-empty lines
    for await (const line of rl) {
        if (line.trim() !== '') {
            lines.push(line);
        }
    }

    // Sort lines in ASC order (just in case they weren't aready sorted)
    lines.sort((a, b) => {
        const [, N1, M1, value1] = a.match(/\[(\d+)\]\[(\d+)\]=(.+)/); // [N][M]=number
        const [, N2, M2, value2] = b.match(/\[(\d+)\]\[(\d+)\]=(.+)/); // [N][M]=number
        // Not same N then sort according N
        if (parseInt(N1) !== parseInt(N2)) {
            return parseInt(N1) - parseInt(N2);
        }
        // Same N then sort according M
        // If not same M then sort according M
        if (parseInt(M1) !== parseInt(M2)) {
            return parseInt(M1) - parseInt(M2);
        }
        // Same M then sort according value
        return parseInt(value1) - parseInt(value2);
    });

    const outputLines = [];
    let prevN = null;
    let prevM = -1;
    let currentNCount = 0;

    function checkConsecutiveM(n, m) {
        if (m === prevM) {
            throw new Error(`Sanity check failed: N=${n} has equal consecutive M: ${m}`);
        }
    }

    for (const line of lines) {
        const [, N, M, value] = line.match(/\[(\d+)\]\[(\d+)\]=(.+)/); // [N][M]=number
        const currentN = parseInt(N);
        const currentM = parseInt(M);

        if (prevN !== null && currentN !== prevN) {
            // Fill remaining M values for previous N up to maxM
            for (let missingM = prevM + 1; missingM <= maxM; missingM++) {
                outputLines.push(`[${prevN}][${missingM}]=0`);
                currentNCount++;
                checkConsecutiveM(prevN, missingM);
                prevM = missingM;
            }
            // Sanity check for previous N
            if (currentNCount !== expectedCountM) {
                throw new Error(`Sanity check failed: N=${prevN} has ${currentNCount} M values instead of ${expectedCountM}`);
            }
            currentNCount = 0;
            prevM = -1;
        }

        if (prevN !== currentN) {
            // Fill initial M values for new N from 0 to currentM - 1
            for (let missingM = minM; missingM < currentM; missingM++) {
                outputLines.push(`[${currentN}][${missingM}]=0`);
                currentNCount++;
                checkConsecutiveM(currentN, missingM);
                prevM = missingM;
            }
        } else {
            // Fill gap within the same N
            for (let missingM = prevM + 1; missingM < currentM; missingM++) {
                outputLines.push(`[${currentN}][${missingM}]=0`);
                currentNCount++;
                checkConsecutiveM(currentN, missingM);
                prevM = missingM;
            }
        }

        outputLines.push(`${line}`);
        currentNCount++;
        checkConsecutiveM(currentN, currentM);
        prevN = currentN;
        prevM = currentM;
    }

    // Fill final sequence if necessary
    if (prevN !== null && prevM < maxM) {
        for (let missingM = prevM + 1; missingM <= maxM; missingM++) {
            outputLines.push(`[${prevN}][${missingM}]=0`);
            currentNCount++;
            checkConsecutiveM(prevN, missingM);
            prevM = missingM;
        }
    }

    // Final sanity check
    if (currentNCount !== expectedCountM) {
        throw new Error(`Sanity check failed: Last N=${prevN} has ${currentNCount} M values instead of ${expectedCountM}`);
    }

    const result = outputLines.reduce((acc, line) => {
        const [, N, M, value] = line.match(/\[(\d+)\]\[(\d+)\]=(.+)/); // [N][M]=number
        if (N !== null) {
            if (!acc[N]) {
                acc[N] = [];
            }
            acc[N].push(value);
        }
        return acc;
    }, [])
        .filter(Boolean) // Remove any undefined entries
        .map(group => `{${group.join(',')}}`);

    // Write output to file
    fs.writeFileSync(outputFile, result.join(',\n'));
    console.log('Processing complete. Output saved to ' + outputFile);
    console.log(`Sanity checks passed: All N sequences have exactly ${expectedCountM} M values and no consecutive M values are equal.`);
}

console.log('Execution in progress...');

processFile().catch(error => {
    console.error(`An error occurred: ${error.message}`);
});