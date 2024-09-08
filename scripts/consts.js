const FS = 8;  // fixed point size in bits
const FP = (1 << FS);  // fixed precision
const AP = 128;  // angle precision (optimal for a rotation step of 8 : 1024/8 = 128)
const STEP_COUNT = 15;  // (STEP_COUNT+1 should be a power of two)

// 224 px display height / 8 = 28. Tiles are 8 pixels in height.
// The HUD takes the bottom 32px / 8 = 4 tiles => 28-4=24
const VERTICAL_COLUMNS = 24;

// 320/8=40. 256/8=32.
const TILEMAP_COLUMNS = 40;

// 320/4=80. 256/4=64.
const PIXEL_COLUMNS = 80;

const MAP_SIZE = 16;

exports.FS = FS
exports.FP = FP
exports.AP = AP
exports.STEP_COUNT = STEP_COUNT
exports.VERTICAL_COLUMNS = VERTICAL_COLUMNS
exports.TILEMAP_COLUMNS = TILEMAP_COLUMNS
exports.PIXEL_COLUMNS = PIXEL_COLUMNS
exports.MAP_SIZE = MAP_SIZE