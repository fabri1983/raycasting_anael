#ifndef _CONSTS_H_
#define _CONSTS_H_

#define SHOW_TEXCOORD FALSE // Show texture coords? This disabled the use of USE_TAB_DELTAS_3.
#define USE_CLEAR_FRAMEBUFFER_WITH_SP TRUE
#define USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS FALSE

// Use mega big table with pre calculated results for: (op1*op2) >> FS.
// It requires before the loop of pixel columns: a *= PIXEL_COLUMNS; // offset to use as: tab_mulu_dist_div256[sideDistX|Y][a+column]
// Not working due to table size incurring rom size > 4096 KB plus the table seems to only works for one of: (sideDistX_l0,sideDistX_l1) or (sideDistY_l0,sideDistY_l1).
#define USE_TAB_MULU_DIST_256_SHFT_FS FALSE

#define FS 8 // Fixed Point size in bits
#define FP (1<<FS) // Fixed Precision
#define AP 128 // Angle Precision (optimal for a rotation step of 8 : 1024/8 = 128)
#define STEP_COUNT 15 // View distance depth. (STEP_COUNT+1 should be a power of two)
#define STEP_COUNT_LOOP STEP_COUNT // If lower than STEP_COUNT then far distant attempts are ignored => less cpu usage but less view depth

// 224 px display height / 8 = 28. Tiles are 8 pixels in height.
// The HUD takes the bottom 32px / 8 = 4 tiles => 28-4=24
#define VERTICAL_ROWS 24

// 320/8=40. 256/8=32.
#define TILEMAP_COLUMNS 40

// 64 for 320p. 32 for 256p.
#define PLANE_COLUMNS 64

// 320/4=80. 256/4=64.
#define PIXEL_COLUMNS 80

#define PB_ADDR 0xC000 // Default Plane B address set in VDP_setPlaneSize(), and starting at 0,0
#define PA_ADDR 0xE000 // Default Plane A address set in VDP_setPlaneSize(), and starting at 0,0
#define PW_ADDR PLANE_COLUMNS == 64 ? 0xD000 + 0x0C00 : 0xC800 + 0x0E00 // As set in VDP_setPlaneSize() depending on the chosen plane size, plus HUD_BASE_XP and HUD_BASE_YP
#define HALF_PLANE_ADDR_OFFSET 0x0600 // In case we split in 2 the DMA of any plane and need to set the correct offset
#define QUARTER_PLANE_ADDR_OFFSET 0x0300 // In case we split in 4 the DMA of any plane and need to set the correct offset

#define MAP_SIZE 16
#define MAP_FRACTION 64
#define MIN_POS_XY (FP + MAP_FRACTION)
#define MAX_POS_XY (FP*(MAP_SIZE-1) - MAP_FRACTION)
#define MAP_HIT_MASK_MAPXY 16-1
#define MAP_HIT_MASK_SIDEDISTXY 4096-1
#define MAP_HIT_OFFSET_MAPXY 0
#define MAP_HIT_OFFSET_SIDEDISTXY 4

#define ANGLE_DIR_NORMALIZATION 24

#endif // _CONSTS_H_