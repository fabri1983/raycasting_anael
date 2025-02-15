#ifndef _CONSTS_EXT_H_
#define _CONSTS_EXT_H_

#include "consts.h"

#include <memory_base.h>
// This is the fixed address of the frame_buffer array, before the end of the heap.
#define FRAME_BUFFER_ADDRESS (MEMORY_HIGH - VERTICAL_ROWS*PLANE_COLUMNS*2*2 - (PLANE_COLUMNS-TILEMAP_COLUMNS)*2)
#if PLANE_COLUMNS == 64
#include "hud_320.h"
#else
#include "hud_256.h"
#endif
// This is the fixed address of the hud_tilemap_dst array, before the frame_buffer.
#define HUD_TILEMAP_DST_ADDRESS (FRAME_BUFFER_ADDRESS - (PLANE_COLUMNS*HUD_BG_H)*2)

#endif // _CONSTS_EXT_H_