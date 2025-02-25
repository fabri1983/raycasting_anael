const fs = require('fs');

// Check correct values of constants before script execution. See consts.h.
const { FP, AP, PIXEL_COLUMNS, TILEMAP_COLUMNS, VERTICAL_ROWS, 
        MAP_SIZE, STEP_COUNT, MAX_U8, TILE_ATTR_VFLIP_MASK } = require('./consts');

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

    generateTabColor_d8_1 () {
        const result = new Array(FP * (STEP_COUNT + 1)).fill(0);
        for (let sideDist = 0; sideDist < (FP * (STEP_COUNT + 1)); ++sideDist) {
            let d = 7 - Math.min(7, Math.floor(sideDist / FP)); // the bigger the distant the darker the color is
            result[sideDist] = 1 + d*8;
        }
        return result;
    },

    /**
     * Generates tab_wall_div array.
     * @returns An Array.
     */
    generateTabWallDiv () {
        // Vertical height calculation starts at the center
        const WALL_H2 = (VERTICAL_ROWS * 8) / 2;

        // Initialize the tab_wall_div array
        const tab_wall_div = new Array(FP * (STEP_COUNT + 1));

        // Populate the tab_wall_div array
        for (let i = 0; i < tab_wall_div.length; i++) {
            let v = (TILEMAP_COLUMNS * FP) / (i + 1);
            let div = Math.round(Math.min(v, MAX_U8));
            if (div >= WALL_H2) {
                tab_wall_div[i] = 0;
            } else {
                tab_wall_div[i] = WALL_H2 - div;
            }
        }

        return tab_wall_div;
    },

    clean_framebuffer (framebuffer) {
        for (let i = 0; i < VERTICAL_ROWS*TILEMAP_COLUMNS; ++i)
            framebuffer[i] = 0; // tile 0 with all attributes in 0
    },

    write_vline (h2, tileAttrib, framebuffer, column) {
        // Draw a solid vertical line
        if (h2 == 0) {
            for (let y = 0; y < VERTICAL_ROWS*TILEMAP_COLUMNS; y+=TILEMAP_COLUMNS) {
            	framebuffer[y + column] = tileAttrib;
            }
            return;
        }

        const ta = Math.floor(h2 / 8); // vertical tilemap entry position
        // top tilemap entry
        framebuffer[ta*TILEMAP_COLUMNS + column] = tileAttrib + (h2 & 7); // offsets the tileAttrib by the halved pixel height modulo 8
        // bottom tilemap entry (with flipped attribute)
        framebuffer[((VERTICAL_ROWS-1)-ta)*TILEMAP_COLUMNS + column] = (tileAttrib + (h2 & 7)) | TILE_ATTR_VFLIP_MASK;

        // Version for VERTICAL_ROWS = 24
        // Set tileAttrib which points to a colored tile.
        switch (ta) {
            case 0:		framebuffer[1*TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[22*TILEMAP_COLUMNS + column] = tileAttrib; // fallthru
            case 1:		framebuffer[2*TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[21*TILEMAP_COLUMNS + column] = tileAttrib; // fallthru
            case 2:		framebuffer[3*TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[20*TILEMAP_COLUMNS + column] = tileAttrib; // fallthru
            case 3:		framebuffer[4*TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[19*TILEMAP_COLUMNS + column] = tileAttrib; // fallthru
            case 4:		framebuffer[5*TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[18*TILEMAP_COLUMNS + column] = tileAttrib; // fallthru
            case 5:		framebuffer[6*TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[17*TILEMAP_COLUMNS + column] = tileAttrib; // fallthru
            case 6:		framebuffer[7*TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[16*TILEMAP_COLUMNS + column] = tileAttrib; // fallthru
            case 7:		framebuffer[8*TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[15*TILEMAP_COLUMNS + column] = tileAttrib; // fallthru
            case 8:		framebuffer[9*TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[14*TILEMAP_COLUMNS + column] = tileAttrib; // fallthru
            case 9:		framebuffer[10*TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[13*TILEMAP_COLUMNS + column] = tileAttrib; // fallthru
            case 10:	framebuffer[11*TILEMAP_COLUMNS + column] = tileAttrib;
                        framebuffer[12*TILEMAP_COLUMNS + column] = tileAttrib; // fallthru
                        break;
        }
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