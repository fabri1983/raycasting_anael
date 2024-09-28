const fs = require('fs');

// Check correct values of constants before script execution. See consts.h.
const { AP, PIXEL_COLUMNS, MAP_SIZE } = require('./consts');

const utils = {

    isInteger (value) {
        return !isNaN(parseInt(value)) && isFinite(value);
    },

    /**
     * Read and parse tab_deltas definition file.
     * @param {*} tabDeltasFile 
     * @returns An Array.
     */
    readTabDeltas (tabDeltasFile) {
        const content = fs.readFileSync(tabDeltasFile, 'utf8');
        const arrName = 'tab_deltas';

        const regex = new RegExp(`const\\s+u16\\s+${arrName}\\s*\\[.*?\\]\\s*=\\s*\\{([^}]+)\\}`, 's');
        const match = content.match(regex);
        if (!match) {
            throw new Error(`Failed to find array ${arrName} in input file.`);
        }
        const tab_deltas = match[1].split(',')
            .map(x => x.trim())
            .filter(x => x !== '' && !isNaN(x) && this.isInteger(x))
            .map(x => parseInt(x, 10));

        if (tab_deltas.length !== AP * PIXEL_COLUMNS * 4) {
            throw new Error(`Invalid number of elements in tab_deltas (expected ${AP * PIXEL_COLUMNS * 4}): ${tab_deltas.length}`);
        }

        return tab_deltas;
    },

    /**
     * Read and parse the map definition file.
     * @param {*} mapMatrixFile 
     * @returns An Array.
     */
    readMapMatrix (mapMatrixFile) {
        const content = fs.readFileSync(mapMatrixFile, 'utf8');
        const lines = content.split('\n');
        const map_matrix = [];

        let inMapDefinition = false;

        for (const line of lines) {
            if (line.includes('const u8 map[MAP_SIZE][MAP_SIZE] =')) {
                inMapDefinition = true;
                continue;
            }

            if (inMapDefinition) {
                if (line.includes('};')) {
                    break; // End of map definition
                }

                const numbers = line.trim()
                    .replace(/^{/, '') // Remove opening brace
                    .replace(/},?$/, '') // Remove closing brace and optional comma
                    .split(',')
                    .map(n => parseInt(n.trim(), 10))
                    .filter(n => !isNaN(n));

                if (numbers.length > 0) {
                    map_matrix.push(...numbers); // use spread operator
                }
            }
        }

        if (map_matrix.length !== (MAP_SIZE * MAP_SIZE)) {
            throw new Error(`Invalid map_matrix dimensions (expected ${MAP_SIZE} * ${MAP_SIZE})`);
        }

        return map_matrix;
    },

    /**
     * Read and parse the tables tab_color_d8_x and tab_color_d8_y from a definition file.
     * @param {*} tabColorD8File 
     * @returns An object containing the 2 arrays: { tab_color_d8_x, tab_color_d8_y }
     */
    readTabColorD8Tables (tabColorD8File) {
        const content = fs.readFileSync(tabColorD8File, 'utf8');
        
        function parseArray (name) {
            const regex = new RegExp(`const\\s+u16\\s+${name}\\s*\\[.*?\\]\\s*=\\s*\\{([^}]+)\\}`, 's');
            const match = content.match(regex);
            if (!match) {
                throw new Error(`Failed to find array ${name} in input file.`);
            }
            return match[1].split(',')
                .map(x => x.trim())
                .filter(x => x !== '')
                .map(x => parseInt(x, 10));
        }

        return {
            tab_color_d8_x: parseArray('tab_color_d8_x'),
            tab_color_d8_y: parseArray('tab_color_d8_y')
        };
    },

    /**
     * Convert 16-bit unsigned value to a signed number.
     * @param {*} value 
     * @returns signed number.
     */
    toSignedFrom16b (value) {
        return value >= 32768 ? value - 65536 : value;
    },

    /**
     * Keeps the lowest 16 bits.
     * @param {*} value 
     * @returns 16-bit unsigned.
     */
    toUnsigned16Bit (value) {
        return value & 0xFFFF;
    },

    /**
     * Get the sign of a 16-bit integer.
     * @param {*} num 
     * @returns -1 if bit 16th is set, 0 if not.
     */
    get16BitSign (num) {
        // Check if the most significant bit is set (bit 15)
        return (num & 0x8000) ? -1 : 1;
    }
};

module.exports = utils;