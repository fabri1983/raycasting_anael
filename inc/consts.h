#ifndef _CONSTS_H_
#define _CONSTS_H_

#define DISPLAY_LOGOS_AT_START TRUE
#define DISPLAY_TITLE_SCREEN TRUE

#define RENDER_SHOW_TEXCOORD FALSE // Show texture coords?

#define RENDER_CLEAR_FRAMEBUFFER_WITH_SP TRUE
#define RENDER_CLEAR_FRAMEBUFFER FALSE

#define RENDER_FRAMEBUFER_DMA_QUEUE_SIZE_ALWAYS_2 TRUE

#define RENDER_USE_TAB_COLOR_D8_1_PALS_SHIFTED FALSE
#define RENDER_USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS TRUE
#define RENDER_USE_MAP_HIT_COMPRESSED FALSE
#define RENDER_COLUMNS_UNROLL 2 // Use only multiple of 2. Supported values: 1, 2, 4. Glitches appear with 4.
#define RENDER_ENABLE_FRAME_LOAD_CALCULATION TRUE

#define DMA_FRAMEBUFFER_A_EIGHT_CHUNKS_ON_DISPLAY_PERIOD_AND_HINT FALSE
#define DMA_FRAMEBUFFER_A_FIRST_QUARTER_ON_HINT FALSE
#define DMA_FRAMEBUFFER_A_FIRST_HALF_ON_HINT FALSE

#define DMA_ALLOW_BUFFERED_TILES FALSE // Set to TRUE if you have compressed sprites, otherwise FALSE.
#define DMA_MAX_QUEUE_CAPACITY 8 // How many objects we can hold without crashing the system due to array out of bound access.
#define DMA_TILES_THRESHOLD_FOR_HINT 384 // when this number of tiles is exceeded we move tiles to VInt queue.
#define DMA_LENGTH_IN_WORD_THRESHOLD_FOR_HINT ((DMA_TILES_THRESHOLD_FOR_HINT * 32) / 2)

#define DMA_ENQUEUE_HUD_TILEMPS_IN_HINT TRUE
#define DMA_ENQUEUE_HUD_TILEMPS_IN_VINT FALSE
#define DMA_ENQUEUE_HUD_TILEMPS FALSE

#define DMA_ENQUEUE_VDP_SPRITE_CACHE_IN_HINT TRUE
#define DMA_ENQUEUE_VDP_SPRITE_CACHE_IN_VINT FALSE

#define FS 8 // Fixed Point size in bits
#define FP (1<<FS) // Fixed Precision
#define AP 128 // Angle Precision (optimal for a rotation step of 8 : 1024/8 = 128)
#define STEP_COUNT 15 // View distance depth. STEP_COUNT+1 should be a power of two.
#define STEP_COUNT_LOOP 15 // >= 12 were the values that work without any glitch.

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
#define HALF_PLANE_ADDR_OFFSET 0x0600 // In case we split in 2 chunks the DMA of any plane we need to use appropriate offset
#define QUARTER_PLANE_ADDR_OFFSET 0x0300 // In case we split in 4 chunks the DMA of any plane we need to use the appropriate offset
#define EIGHTH_PLANE_ADDR_OFFSET 0x0180 // In case we split in 8 chunks the DMA of any plane we need to use the appropriate offset

#define MAP_SIZE 16
#define MAP_FRACTION 32 // How much we allow the player to be close to any wall
#define MIN_POS_XY (FP + MAP_FRACTION)
#define MAX_POS_XY (FP*(MAP_SIZE-1) - MAP_FRACTION)
#define MAP_HIT_MASK_MAPXY (16-1)
#define MAP_HIT_MASK_SIDEDISTXY (4096-1)
#define MAP_HIT_OFFSET_MAPXY 0
#define MAP_HIT_OFFSET_SIDEDISTXY 4
#define MAP_HIT_MIN_CALCULATED_INDEX 174080

#define ANGLE_DIR_NORMALIZATION 24

#endif // _CONSTS_H_