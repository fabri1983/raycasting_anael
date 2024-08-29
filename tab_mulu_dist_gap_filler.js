/*
Expects a file with next format:
	[N][M]=x
	Eg:
	[0][32]=1265
	[0][33]=6855
	[0][34]=45
	...
	[0][7175]=7853
	[1][32]=12
	[1][32]=256
	...
	[1][7175]=4901
*/

const fs = require('fs');
const readline = require('readline');

const inputFile = 'tab_mulu_dist.txt';
const outputFile = 'tab_mulu_dist_output.txt';

const expectedCount = 7176;
const maxM = expectedCount - 1;
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

    // Sort lines
    lines.sort((a, b) => {
        const [, nA, mA] = a.match(/\[(\d+)\]\[(\d+)\]/);
        const [, nB, mB] = b.match(/\[(\d+)\]\[(\d+)\]/);
        if (nA !== nB) return parseInt(nA) - parseInt(nB);
        return parseInt(mA) - parseInt(mB);
    });

    let output = '';
    let prevN = null;
    let prevM = -1;
    let currentNCount = 0;

    function checkConsecutiveM(n, m) {
        if (m === prevM) {
            throw new Error(`Sanity check failed: N=${n} has consecutive M values: ${m}`);
        }
    }

    for (const line of lines) {
        const [, N, M, value] = line.match(/\[(\d+)\]\[(\d+)\]=(.+)/); // [N][M]=number
        const currentN = parseInt(N);
        const currentM = parseInt(M);

        if (prevN !== null && currentN !== prevN) {
            // Fill remaining M values for previous N up to 6159
            for (let missingM = prevM + 1; missingM <= maxM; missingM++) {
                output += `[${prevN}][${missingM}]=0\n`;
                currentNCount++;
                checkConsecutiveM(prevN, missingM);
                prevM = missingM;
            }
            // Sanity check for previous N
            if (currentNCount !== expectedCount) {
                throw new Error(`Sanity check failed: N=${prevN} has ${currentNCount} M values instead of ${expectedCount}`);
            }
            currentNCount = 0;
            prevM = -1;
        }

        if (prevN !== currentN) {
            // Fill initial M values for new N from 0 to currentM - 1
            for (let missingM = minM; missingM < currentM; missingM++) {
                output += `[${currentN}][${missingM}]=0\n`;
                currentNCount++;
                checkConsecutiveM(currentN, missingM);
                prevM = missingM;
            }
        } else {
            // Fill gap within the same N
            for (let missingM = prevM + 1; missingM < currentM; missingM++) {
                output += `[${currentN}][${missingM}]=0\n`;
                currentNCount++;
                checkConsecutiveM(currentN, missingM);
                prevM = missingM;
            }
        }

        output += `${line}\n`;
        currentNCount++;
        checkConsecutiveM(currentN, currentM);
        prevN = currentN;
        prevM = currentM;
    }

    // Fill final sequence if necessary
    if (prevN !== null && prevM < maxM) {
        for (let missingM = prevM + 1; missingM <= maxM; missingM++) {
            output += `[${prevN}][${missingM}]=0\n`;
            currentNCount++;
            checkConsecutiveM(prevN, missingM);
            prevM = missingM;
        }
    }

    // Final sanity check
    if (currentNCount !== expectedCount) {
        throw new Error(`Sanity check failed: Last N=${prevN} has ${currentNCount} M values instead of ${expectedCount}`);
    }

    fs.writeFileSync(outputFile, output);
    console.log(`Processing complete. Output saved to ${outputFile}`);
    console.log(`Sanity checks passed: All N sequences have exactly ${expectedCount} M values and no consecutive M values are equal.`);
}

processFile().catch(error => {
    console.error(`An error occurred: ${error.message}`);
});