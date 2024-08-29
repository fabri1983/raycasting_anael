/**
Expects a file named tab_deltas.txt with next format:
	260, 256, 252, 256,
	655, 256, 100, 256,
	1092, 256, 60, 256,
	1260, 256, 52, 256,
	16384, 256, 4, 256,
	16384, 256, 65532, 256,
	...
	5461, 256, 65524, 256,
*/

const fs = require('fs');
const readline = require('readline');

const inputFile = 'tab_deltas.txt';
const outputFile = 'tab_deltas_output.txt';

// Function to get the sign of a 16-bit integer
function getSign(num) {
    // Check if the most significant bit is set (bit 15)
    return (num & 0x8000) ? -1 : 1;
}

// Function to convert a number to a 16-bit signed integer
function to16BitSigned(num) {
    // Ensure the number is within the 16-bit signed range
    num = num & 0xFFFF;
    // If the number is negative (bit 15 is set), extend the sign
    return (num & 0x8000) ? (num | 0xFFFF0000) : num;
}

// Function to process a line
function processLine(line) {
	if (line.trim().lenght == 0)
		return '';

    // Remove any trailing comma and split by comma or space
    const numbers = line.trim().replace(/,\s*$/, '').split(/[,\s]+/).map(n => parseInt(n));
    if (numbers.length !== 4) {
        throw new Error(`Invalid line format: ${line}`);
    }

    const result1 = to16BitSigned(numbers[0] * getSign(numbers[2]));
    const result2 = to16BitSigned(numbers[1] * getSign(numbers[3]));

    return `${result1},${result2},`;
}

async function processFile() {
    const fileStream = fs.createReadStream(inputFile);
    const rl = readline.createInterface({
        input: fileStream,
        crlfDelay: Infinity
    });

    let output = '';

    for await (const line of rl) {
        try {
            output += processLine(line) + '\n';
        } catch (error) {
            console.error(`Error processing line: ${line}`);
            console.error(error.message);
        }
    }

    fs.writeFileSync(outputFile, output);
    console.log(`Processing complete. Output saved to ${outputFile}`);
}

processFile().catch(error => {
    console.error(`An error occurred: ${error.message}`);
});