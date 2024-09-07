#ifndef _CONSTS_H_
#define _CONSTS_H_

#define SHOW_TEXCOORD FALSE // show texture coords?
#define USE_TAB_DELTAS_3 TRUE // Use smaller table with 3 columns where ray direction is encoded
#define USE_TAB_MULU_DIST_DIV256 FALSE // Use mega big table for mulu pre calculation. This only works for 1 of sideDistX_l0, sideDistX_l1, etc

#define FS 8 // fixed point size in bits
#define FP (1<<FS) // fixed precision
#define STEP_COUNT 15 // (STEP_COUNT+1 should be a power of two)
#define AP 128 // angle precision (optimal for a rotation step of 8 : 1024/8 = 128)

// 224 px display height / 8 = 28. Remember that tiles are 8 pixels wide.
// The HUD takes the bottom 4 tiles => 28-4=24
#define VERTICAL_COLUMNS 24

// 32 for 256p, 40 for 320p
#define TILEMAP_COLUMNS 40

// 32 for 256p, 64 for 320p
#define PLANE_COLUMNS 64

// 64 for 256p, 80 for 320p
#define PIXEL_COLUMNS 80

#define MAP_SIZE 16

#endif // _CONSTS_H_