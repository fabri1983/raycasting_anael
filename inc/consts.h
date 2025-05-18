#ifndef _CONSTS_H_
#define _CONSTS_H_

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef F
#define F 0
#endif
#ifndef T
#define T 1
#endif

#define DISPLAY_LOGOS_AT_START F
#define DISPLAY_TITLE_SCREEN F

#define RENDER_SHOW_TEXCOORD F // Show texture coords? Is not optimized though

// If TRUE then it will change from floor to roof bg color at HInt. If FALSE then we use single bg color (unless other switch is used).
#define RENDER_SET_FLOOR_AND_ROOF_COLORS_ON_HINT T
// If TRUE then it changes the BG color as a grey ramp effect. Only if hint multi callbacks siwtch is enabled.
#define RENDER_SET_ROOF_COLOR_RAMP_ONLY_HINT_MULTI_CALLBACKS F

#define RENDER_MIRROR_PLANES_USING_CPU_RAM F // Mirror bottom half of planes into top hal, by using CPU and RAM
#define RENDER_MIRROR_PLANES_USING_VDP_VRAM F // Mirror bottom half of planes into top hal, by using VRAM to VRAM copy
#define RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT F // Mirror bottom half of planes into top hal, by using VSCROLL table manipulation at HINT
#define RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT_MULTI_CALLBACKS T // Mirror bottom half of planes into top half, by using VSCROLL table manipulation at multiple HINTs (fully optimized)
#define HMC_START_OFFSET_FACTOR 1 // Only for hints using VSCroll, this marks the initial offset factor
// Render only half bottom region of both planes to later mirror them using one of the many available strategies.
#define RENDER_HALVED_PLANES RENDER_MIRROR_PLANES_USING_CPU_RAM | RENDER_MIRROR_PLANES_USING_VDP_VRAM | RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT | RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT_MULTI_CALLBACKS

// Doesn't save registers in the stack so call it at the begin of game loop. It doesn't use the USP.
#define RENDER_CLEAR_FRAMEBUFFER_NO_USP F
// Doesn't save registers in the stack so call it at the begin of game loop. Overwrites USP so be sure is not used by any interruption callback.
#define RENDER_CLEAR_FRAMEBUFFER F
// Slightly faster with the use of SP as pointer. Doesn't save registers in the stack so call it at the begin of game loop. Overwrites USP so be sure is not used by any interruption callback.
#define RENDER_CLEAR_FRAMEBUFFER_WITH_SP T

#define RENDER_USE_TAB_COLOR_D8_1_PALS_SHIFTED T
#define RENDER_USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS T
#define RENDER_USE_MAP_HIT_COMPRESSED F
#define RENDER_COLUMNS_UNROLL 2 // Use only multiple of 2. Supported values: 1, 2, 4. Glitches appear with 4, dang!
#define RENDER_ENABLE_FRAME_LOAD_CALCULATION T

#define DMA_ALLOW_BUFFERED_SPRITE_TILES F // Set to TRUE if you have compressed sprites, otherwise FALSE.
#define DMA_MAX_QUEUE_CAPACITY 8 // How many objects we can hold without crashing the system due to array out of bound access.
#define DMA_TILES_THRESHOLD_FOR_HINT 200 // when this number of tiles is exceeded we move the exceeding tiles to VInt queue.
#define DMA_LENGTH_IN_WORD_THRESHOLD_FOR_HINT ((DMA_TILES_THRESHOLD_FOR_HINT * 32) / 2)

#define DMA_ENQUEUE_HUD_TILEMAP_TO_FLUSH_AT_HINT T
#define DMA_ENQUEUE_HUD_TILEMAP_TO_FLUSH_AT_VINT F
#define DMA_HUD_TILEMAP_IMMEDIATELY F

#define DMA_ENQUEUE_VDP_SPRITE_CACHE_TO_FLUSH_AT_HINT T
#define DMA_ENQUEUE_VDP_SPRITE_CACHE_TO_FLUSH_AT_VINT F
#define DMA_ENQUEUE_VDP_SPRITE_CACHE_FOR_SGDK_QUEUE F
#define DMA_ENQUEUE_VDP_SPRITE_CACHE_ON_CUSTOM_SPR_QUEUE F

// If change this value then update map_base parameter in hud_res.res file using value from HUD_BASE_TILE_ATTRIB
#define VRAM_INDEX_AFTER_TILES (1 + 8*8 + 8*8)

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
#define PIXEL_COLUMNS TILEMAP_COLUMNS*2

#define MAP_SIZE 16
#define MAP_FRACTION 32 // How much we allow the player to be close to any wall
#define MIN_POS_XY (FP + MAP_FRACTION)
#define MAX_POS_XY (FP*(MAP_SIZE-1) - MAP_FRACTION)

#define ANGLE_DIR_NORMALIZATION 24

#define MAP_HIT_MASK_MAPXY (16-1) // 4 bits
#define MAP_HIT_MASK_SIDEDISTXY (4096-1) // 12 bits
#define MAP_HIT_OFFSET_MAPXY 0
#define MAP_HIT_OFFSET_SIDEDISTXY 4
#define MAP_HIT_MIN_CALCULATED_INDEX 174080

#endif // _CONSTS_H_