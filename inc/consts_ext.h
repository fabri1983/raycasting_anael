#ifndef _CONSTS_EXT_H_
#define _CONSTS_EXT_H_

#include "consts.h"
#include "hud_consts.h"
#include "weapon_consts.h"

#define PB_ADDR 0xC000 // Default Plane B address set in VDP_setPlaneSize(), and starting at 0,0
#define PW_ADDR_AT_HUD (PLANE_COLUMNS == 64 ? 0xD000+0x0C00 : 0xC800+0x0E00) // As set in VDP_setPlaneSize() depending on the chosen plane size, plus HUD_XP and HUD_YP offsets
#define PA_ADDR 0xE000 // Default Plane A address set in VDP_setPlaneSize(), and starting at 0,0
#define VDP_SPRITE_LIST_ADDR 0x0000F400 // Defined on VDP_init() and also updated whenever VDP's reg 05 is updated.
#define HALF_PLANE_ADDR_OFFSET_BYTES (VERTICAL_ROWS*PLANE_COLUMNS) // In case we split in 2 chunks the DMA of any plane we need to use appropriate offset
#define QUARTER_PLANE_ADDR_OFFSET_BYTES (HALF_PLANE_ADDR_OFFSET/2) // In case we split in 4 chunks the DMA of any plane we need to use the appropriate offset
#define EIGHTH_PLANE_ADDR_OFFSET (HALF_PLANE_ADDR_OFFSET/4) // In case we split in 8 chunks the DMA of any plane we need to use the appropriate offset

// Free VRAM region at Plane B.
#define PB_FREE_VRAM_AT (PB_ADDR + (PLANE_COLUMNS*VERTICAL_ROWS - (PLANE_COLUMNS-TILEMAP_COLUMNS))*2 + (PLANE_COLUMNS == 64 ? 16 : 0))
// If PLANE_COLUMNS = 64 then Plane B ends at PW_ADDR_AT_HUD => PW_ADDR_AT_HUD - PB_FREE_VRAM_AT = 0x1020 = 4128 bytes (129 tiles).
// If PLANE_COLUMNS = 32 then Plane B ends at PW_ADDR_AT_HUD => PW_ADDR_AT_HUD - PB_FREE_VRAM_AT = 0x1000 = 4096 bytes (128 tiles).
#define PB_FREE_BYTES_LENGTH (PW_ADDR_AT_HUD - PB_FREE_VRAM_AT)

// Free VRAM region at Window Plane.
#define PW_FREE_VRAM_AT (PW_ADDR_AT_HUD + (HUD_HEIGHT*PLANE_COLUMNS*2))
// If PLANE_COLUMNS = 64 then Window Plane ends at PA_ADDR => 0xE000 - PW_FREE_VRAM_AT = 0x200 = 512 bytes (16 tiles)
// If PLANE_COLUMNS = 32 then Window Plane ends at PA_ADDR => 0xE000 - PW_FREE_VRAM_AT = 0x7F2 = 2034 bytes (63 tiles)
#define PW_FREE_BYTES_LENGTH (PA_ADDR - PW_FREE_VRAM_AT)

// Free VRAM region at Plane A.
#define PA_FREE_VRAM_AT (PA_ADDR + (PLANE_COLUMNS*VERTICAL_ROWS - (PLANE_COLUMNS-TILEMAP_COLUMNS))*2 + (PLANE_COLUMNS == 64 ? 16 : 0))
// If PLANE_COLUMNS = 64 then Plane A ends at 0xF000 => 0xF000 - PA_FREE_VRAM_AT = 0x420 = 1056 bytes (33 tiles).
// If PLANE_COLUMNS = 32 then Plane A ends at 0xE800 => 0xE800 - PA_FREE_VRAM_AT = 0x200 = 512 bytes (16 tiles).
#define PA_FREE_BYTES_LENGTH ((PLANE_COLUMNS == 64 ? 0xF000 : 0xE800) - PA_FREE_VRAM_AT)

// Free VRAM location depending on Plane size.
// NOTE: we can start earlier depending on max SAT size. Ie: if we only use up to N sprites, then we can (80-N)*8 bytes earlier.
#define LAST_FREE_VRAM_AT ((PLANE_COLUMNS == 64 ? 0xF700 : 0xD000) - ((80-80)*8))
// If PLANE_COLUMNS = 64 then 2303 bytes + 1 tile (73 tiles or more).
// If PLANE_COLUMNS = 32 then 4096 bytes + 1 tile (129 tiles or more).
#define LAST_FREE_BYTES_LENGTH ((PLANE_COLUMNS == 64 ? 0xFFFF : 0xDFFF) - LAST_FREE_VRAM_AT + 32)

#include <memory_base.h>

// This is the fixed RAM address for the frame_buffer array, before the end of the heap.
#define RAM_FIXED_FRAME_BUFFER_ADDRESS (MEMORY_HIGH - (VERTICAL_ROWS*TILEMAP_COLUMNS*2)*2)
#if PLANE_COLUMNS == 64
#include "hud_320.h"
#else
#include "hud_256.h"
#endif

// This address comes from VDPSprite vdpSpriteCache variable at vdp_spr.c and may change if the SGDK lib or our code changes.
// We have a check at the beginning of the game. Take a look at it if game doesn't show.
#if DISPLAY_LOGOS_AT_START == F && DISPLAY_TITLE_SCREEN == F
#define RAM_FIXED_VDP_SPRITE_CACHE_ADDRESS 0xE0FF022A
#elif DISPLAY_LOGOS_AT_START == T && DISPLAY_TITLE_SCREEN == F
#define RAM_FIXED_VDP_SPRITE_CACHE_ADDRESS 0xE0FF02AC
#elif DISPLAY_LOGOS_AT_START == F && DISPLAY_TITLE_SCREEN == T
#define RAM_FIXED_VDP_SPRITE_CACHE_ADDRESS 0xE0FF02B2
#else
#define RAM_FIXED_VDP_SPRITE_CACHE_ADDRESS 0xE0FF027C
#endif

// This is the fixed RAM address for the hud_tilemap_src array.
#define RAM_FIXED_HUD_TILEMAP_SRC_ADDRESS (RAM_FIXED_FRAME_BUFFER_ADDRESS - (HUD_SOURCE_IMAGE_W*HUD_SOURCE_IMAGE_H)*2)

// This is the fixed RAM address for the hud_tilemap_dst array.
#define RAM_FIXED_HUD_TILEMAP_DST_ADDRESS (RAM_FIXED_HUD_TILEMAP_SRC_ADDRESS - (TILEMAP_COLUMNS*HUD_BG_H)*2)

// This is the fixed RAM address for the HUD palettes data.
#define RAM_FIXED_HUD_PALETTES_ADDRESS (RAM_FIXED_HUD_TILEMAP_DST_ADDRESS - (16*HUD_USED_PALS)*2)

// This is the fixed RAM address for the WEAPON palettes data.
#define RAM_FIXED_WEAPON_PALETTES_ADDRESS (RAM_FIXED_HUD_PALETTES_ADDRESS - (16*WEAPON_USED_PALS)*2)

#endif // _CONSTS_EXT_H_