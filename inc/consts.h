#ifndef _CONSTS_H_
#define _CONSTS_H_

#define SHOW_TEXCOORD FALSE // show texture coords?
#define USE_TAB_DELTAS_3 TRUE // Use smaller table with 3 columns where ray direction is encoded
#define USE_TAB_MULU_DIST_DIV256 FALSE // Use mega big table for mulu pre calculation. This only works for 1 of sideDistX_l0, sideDistX_l1, etc

#define FS 8 // fixed point size in bits
#define FP (1<<FS) // fixed precision
#define STEP_COUNT 15 // (STEP_COUNT+1 should be a power of two)
#define AP 128 // angle precision (optimal for a rotation step of 8 : 1024/8 = 128)

#define PB_ADDR 0xC000 // default Plane B address set in VDP_setPlaneSize()
#define PA_ADDR 0xE000 // default Plane A address set in VDP_setPlaneSize()
#define HALF_PLANE_ADDR 0x0600 // In case we need to split in half the DMA of any plane

// 224 px display height / 8 = 28. Tiles are 8 pixels in height.
// The HUD takes the bottom 4 tiles => 28-4=24
#define VERTICAL_COLUMNS 24

// 320/8=40. 256/8=32
#define TILEMAP_COLUMNS 40

// 64 for 320p. 32 for 256p
#define PLANE_COLUMNS 64

// 320/4=80. 256/4=64
#define PIXEL_COLUMNS 80

#define MAP_SIZE 16

#endif // _CONSTS_H_