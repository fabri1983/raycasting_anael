#ifndef _CONSTS_EXT_H_
#define _CONSTS_EXT_H_

#include "consts.h"
#include "hud_consts.h"

#define PB_ADDR 0xC000 // Default Plane B address set in VDP_setPlaneSize(), and starting at 0,0
#define PW_ADDR_AT_HUD (PLANE_COLUMNS == 64 ? 0xD000+0x0C00 : 0xC800+0x0E00) // As set in VDP_setPlaneSize() depending on the chosen plane size, plus HUD_XP and HUD_YP offsets
#define PA_ADDR 0xE000 // Default Plane A address set in VDP_setPlaneSize(), and starting at 0,0
#define HALF_PLANE_ADDR_OFFSET_BYTES (VERTICAL_ROWS*PLANE_COLUMNS) // In case we split in 2 chunks the DMA of any plane we need to use appropriate offset
#define QUARTER_PLANE_ADDR_OFFSET_BYTES (HALF_PLANE_ADDR_OFFSET/2) // In case we split in 4 chunks the DMA of any plane we need to use the appropriate offset
#define EIGHTH_PLANE_ADDR_OFFSET (HALF_PLANE_ADDR_OFFSET/4) // In case we split in 8 chunks the DMA of any plane we need to use the appropriate offset

// Free VRAM region at Plane B.
#define PB_FREE_VRAM_AT (PB_ADDR + (PLANE_COLUMNS*VERTICAL_ROWS - (PLANE_COLUMNS-TILEMAP_COLUMNS))*2 + (PLANE_COLUMNS == 64 ? 16 : 0))
// If PLANE_COLUMNS = 64 then Plane B ends at PW_ADDR_AT_HUD => PW_ADDR_AT_HUD - PB_FREE_VRAM_AT = 0x1020 = 4128 bytes (129 tiles).
// If PLANE_COLUMNS = 32 then Plane B ends at PW_ADDR_AT_HUD => PW_ADDR_AT_HUD - PB_FREE_VRAM_AT = 0x1000 = 4096 bytes (128 tiles).
#define PB_FREE_BYTES (PW_ADDR_AT_HUD - PB_FREE_VRAM_AT)

// Free VRAM region at Window Plane.
#define PW_FREE_VRAM_AT (PW_ADDR_AT_HUD + (HUD_HEIGHT*PLANE_COLUMNS*2))
// If PLANE_COLUMNS = 64 then Window Plane ends at PA_ADDR => 0xE000 - PW_FREE_VRAM_AT = 0x200 = 512 bytes (16 tiles)
// If PLANE_COLUMNS = 32 then Window Plane ends at PA_ADDR => 0xE000 - PW_FREE_VRAM_AT = 0x7F2 = 2034 bytes (63 tiles)
#define PW_FREE_BYTES (PA_ADDR - PW_FREE_VRAM_AT)

// Free VRAM region at Plane A.
#define PA_FREE_VRAM_AT (PA_ADDR + (PLANE_COLUMNS*VERTICAL_ROWS - (PLANE_COLUMNS-TILEMAP_COLUMNS))*2 + (PLANE_COLUMNS == 64 ? 16 : 0))
// If PLANE_COLUMNS = 64 then Plane A ends at 0xF000 => 0xF000 - PA_FREE_VRAM_AT = 0x420 = 1056 bytes (33 tiles).
// If PLANE_COLUMNS = 32 then Plane A ends at 0xE800 => 0xE800 - PA_FREE_VRAM_AT = 0x200 = 512 bytes (16 tiles).
#define PA_FREE_BYTES ((PLANE_COLUMNS == 64 ? 0xF000 : 0xE800) - PA_FREE_VRAM_AT)

// Free VRAM location depending on Plane size.
// NOTE: we can start earlier depending on max SAT size. Ie: if we only use up to N sprites, then we can (80-N)*8 bytes earlier.
#define LAST_FREE_VRAM_AT ((PLANE_COLUMNS == 64 ? 0xF700 : 0xD000) - (0*8))
// If PLANE_COLUMNS = 64 then 2303 bytes + 1 tile (73 tiles).
// If PLANE_COLUMNS = 32 then 4096 bytes + 1 tile (129 tiles).
#define LAST_FREE_BYTES ((PLANE_COLUMNS == 64 ? 0xFFFF : 0xDFFF) - LAST_FREE_VRAM_AT + 32)

#include <memory_base.h>
// This is the fixed address of the frame_buffer array, before the end of the heap.
#define FRAME_BUFFER_ADDRESS (MEMORY_HIGH - (VERTICAL_ROWS*PLANE_COLUMNS*2)*2 - (PLANE_COLUMNS-TILEMAP_COLUMNS)*2)
#if PLANE_COLUMNS == 64
#include "hud_320.h"
#else
#include "hud_256.h"
#endif
// This is the fixed address of the hud_tilemap_dst array, before the frame_buffer.
#define HUD_TILEMAP_DST_ADDRESS (FRAME_BUFFER_ADDRESS - (PLANE_COLUMNS*HUD_BG_H)*2)

#endif // _CONSTS_EXT_H_