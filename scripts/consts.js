const FS = 8;  // fixed point size in bits
const FP = (1 << FS);  // fixed precision
const AP = 128;  // angle precision (optimal for a rotation step of 8 : 1024/8 = 128)
const STEP_COUNT = 15;  // View distance depth. (STEP_COUNT+1 should be a power of two)

// 224 px display height / 8 = 28. Tiles are 8 pixels in height.
// The HUD takes the bottom 32px / 8 = 4 tiles => 28-4=24
const VERTICAL_COLUMNS = 24;

// 320/8=40. 256/8=32.
const TILEMAP_COLUMNS = 40;

// 320/4=80. 256/4=64.
const PIXEL_COLUMNS = 80;

const MAP_SIZE = 16;
const MAP_FRACTION = 64;
const MIN_POS_XY = (FP + MAP_FRACTION);
const MAX_POS_XY = (FP*(MAP_SIZE-1) - MAP_FRACTION);
const MAP_HIT_MASK_SIDE = 1;
const MAP_HIT_MASK_SIDEDISTXY = 4096-1;
const MAP_HIT_MASK_MAPXY = 16-1;
const MAP_HIT_OFFSET_SIDEDIST = 1;
const MAP_HIT_OFFSET_MAPXY = 12 + 1;

//---------------------
// SGDK constants
//---------------------
const PAL0 = 0;
const PAL1 = 1;
const PAL2 = 2;
const PAL3 = 3;
const TILE_ATTR_PALETTE_SFT = 13;
const M_PI = Math.PI;
const MAX_U8 = 255;  // Assuming MAX_U8 as 255 since it's an 8-bit value
const MAX_U16 = 0xFFFF;

// Export all of them
exports.FS = FS
exports.FP = FP
exports.AP = AP
exports.STEP_COUNT = STEP_COUNT
exports.VERTICAL_COLUMNS = VERTICAL_COLUMNS
exports.TILEMAP_COLUMNS = TILEMAP_COLUMNS
exports.PIXEL_COLUMNS = PIXEL_COLUMNS
exports.MAP_SIZE = MAP_SIZE
exports.MIN_POS_XY = MIN_POS_XY
exports.MAX_POS_XY = MAX_POS_XY
exports.MAP_HIT_MASK_SIDE = MAP_HIT_MASK_SIDE;
exports.MAP_HIT_MASK_SIDEDISTXY = MAP_HIT_MASK_SIDEDISTXY;
exports.MAP_HIT_MASK_MAPXY = MAP_HIT_MASK_MAPXY;
exports.MAP_HIT_OFFSET_SIDEDIST = MAP_HIT_OFFSET_SIDEDIST;
exports.MAP_HIT_OFFSET_MAPXY = MAP_HIT_OFFSET_MAPXY;

exports.PAL0 = PAL0
exports.PAL1 = PAL1
exports.PAL2 = PAL2
exports.PAL3 = PAL3
exports.TILE_ATTR_PALETTE_SFT = TILE_ATTR_PALETTE_SFT
exports.M_PI = M_PI
exports.MAX_U8 = MAX_U8
exports.MAX_U16 = MAX_U16
