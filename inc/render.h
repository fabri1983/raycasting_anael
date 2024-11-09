#ifndef _RENDER_H_
#define _RENDER_H_

#include <types.h>
#include "consts.h"

// Loads render tiles in VRAM. 8 set of 8 tiles each plus 1 empty tile => 73 tiles in total.
// Additionally, we add another set of 8 tiles using colors in palette from 8 to 14. This way walls are painted with 2 palettes.
// Starts locating tiles at index 0. Returns next available tile index in VRAM which must be HUD_VRAM_START_INDEX.
// IMPORTANT: if amount of generated tiles is changed then update resource file and the constants involved too.
u16 render_loadTiles ();

// Calculates the offset to access the column for both planes defined in the framebuffer.
void render_loadPlaneDisplacements ();

// Loads the palettes as we expected them to work for the walls.
// It assumes palettes grey and red are already loaded, then it loads the chunks for green and blue palettes.
void render_loadWallPalettes ();

// Custom SYS_doVBlankProcessEx() impl only for VBlankProcessTime ON_VBLANK.
// Please compare it with original SYS_doVBlankProcessEx() at vdp.c to see what has been removed.
void render_SYS_doVBlankProcessEx_ON_VBLANK ();

#endif // _RENDER_H_