#ifndef _RENDER_H_
#define _RENDER_H_

#include <types.h>
#include "consts.h"

/// @brief Loads render tiles in VRAM. 
/// 8 set of 8 tiles each plus 1 empty tile => 73 tiles in total.
/// Additionally, we add another set of 8 tiles using colors in palette from 8 to 14. This way walls are painted using 2 palettes.
/// Starts locating tiles at index 0. 
/// Returns next available tile index in VRAM which must be HUD_VRAM_START_INDEX.
/// IMPORTANT: if amount of generated tiles is changed then update resource file and the constants involved too.
u16 render_loadTiles ();

/// @brief Loads the palettes as we expected them to work for the walls.
/// It assumes palettes grey and red are already loaded, then it loads the chunks for green and blue palettes.
void render_loadWallPalettes ();

/// @brief See original Z80_setBusProtection() method.
/// NOTE: This implementation doesn't disable interrupts because at the moment it's called no interrupt is expected to occur.
/// NOTE: This implementation assumes the Z80 bus was not already requested, and requests it immediatelly.
/// @param value TRUE enables protection, FALSE disables it.
void render_Z80_setBusProtection (bool value);

/// @brief This implementation differs from DMA_flushQueue() in which:
/// - it doesn't check if transfer size exceeds capacity because we know before hand the max capacity.
/// - it assumes Z80 bus wasn't requested and hence request it.
void render_DMA_flushQueue ();

/// @brief Custom SYS_doVBlankProcessEx() impl only for VBlankProcessTime ON_VBLANK, in conjunction with a faster _VINT_doom in sega.s.
/// Please compare it with original SYS_doVBlankProcessEx() at vdp.c to see what has been removed.
void render_SYS_doVBlankProcessEx_ON_VBLANK ();

void render_DMA_enqueue_framebuffer ();
void render_DMA_row_by_row_framebuffer ();

void render_DMA_halved_mirror_planes ();
void render_copy_top_entries_in_VRAM ();

#endif // _RENDER_H_