// MIT License - Copyright (c) 2024 Anaël Seghezzi
// raymarching rendering with precomputed tiles interleaved in both planes at 60fps
// full screen in mode 256x224 with 4 pixels wide column (effective resolution 64x224)
// it's possible to use 60+1 colors, but only 32+1 are used here
// reference for raymarching: https://lodev.org/cgtutor/raycasting.html

// fabri1983's notes (since Aug/18/2024)
// -----------------`
// * Ray Casting: https://lodev.org/cgtutor/raycasting.html
// * Rendered columns are 4 pixels wide, then effectively 256p/4=64 or 320p/4=80 "pixels" columns.
// * Moved clear_buffer() into inline ASM to take advantage of compiler optimizations => %1 saved in cpu usage.
// * clear_buffer() using SP (Stack Pointer) to point end of buffer => 60 cycles saved if PLANE_COLUMNS=32, 150 cycles saved if PLANE_COLUMNS=64.
// * If clear_buffer() is moved into VBlank callback => %1 saved in cpu usage, but runs into next display period.
// * Moved write_vline_full() into write_vline() and translated into inline ASM => ~1% saved in cpu usage.
// * write_vline(): translated into inline ASM => 2% saved in cpu usage.
// * write_vline(): access optimization for top and bottom tiles of current column in ASM => 4% saved in cpu usage.
// * write_vline(): exposing globally the column pointer instead of sending it as an argument. This made the frame_buffer_pxcolumn[] useless,
//   so no need for loadPlaneDisplacements() neither offset before write_vline() => ~1% saved in cpu usage depending on the inline ASM constraints.
// * Replaced   if ((joy & BUTTON_LEFT) || (joy & BUTTON_RIGHT))
//   by         if (joy & (BUTTON_LEFT | BUTTON_RIGHT))
//   Same with BUTTON_UP and BUTTON_DOWN.
// * Replaced dirX and dirY calculation in relation to the angle by two tables defined in tab_dir_xy.h => some cycles saved.
// * Replaced DMA_doDmaFast() by DMA_queueDmaFast() => 1% saved in cpu usage but consumes all VBlank period and runs into next display period.
// * Changes in tab_wall_div.h so the start of the vertical line to be written is calculated ahead of time => 1% saved in cpu usage.
// * Replaced variable d used for color calculation by two tables defined in tab_color_d8.h => 2% saved in cpu usage.
// * Replaced color calculation out of tab_color_d8[] by using tables in tab_color_d8_pals_shft.h => 1% saved in cpu usage.
// * Replaced (mulu(sideDistX_l0, deltaDistX) >> FS) by a table => ~4% saved in cpu usage, though rom size increased 460 KB (+ padding to 128KB boundary).
// * sega.s: use _VINT_lean instead of _VINT to enter vint callback as soon as possible => some cycles saved.
// * Commented out the #pragma directives for loop unrolling => ~1% saved in cpu usage. It may vary according the use of FORCE_INLINE.
// * Manual unrolling of 2 (or 4) iterations for column processing => 2% saved in cpu usage. It may vary according the use of FORCE_INLINE.

#include <genesis.h>
#include "utils.h"
#include "consts.h"
#include "clear_buffer.h"
#include "frame_buffer.h"

#include "vint_callback.h"
#include "hint_callback.h"
#include "hud.h"
#include "weapons.h"

// the code is optimised further using GCC's automatic unrolling
#pragma GCC push_options
//#pragma GCC optimize ("unroll-loops")
#pragma GCC optimize ("no-unroll-loops")

// Load render tiles in VRAM. 9 set of 8 tiles each => 72 tiles in total.
// Returns next available tile index in VRAM.
// IMPORTANT: if tiles num is changed then update resource file and the constants involved too.
static u16 loadRenderingTiles ()
{
	// Create a buffer tile
	u8* tile = MEM_alloc(32); // 32 bytes per tile, layout: tile[4][8]
	memset(tile, 0, 32); // clear the tile with color index 0

	// 9 possible tile heights

	// Tile with height 0 is at index 0
	VDP_loadTileData((u32*)tile, 0, 1, CPU);

	// Remaining 8 possible tile heights, distributed in 8 sets

	// 8 tiles per set
	for (u16 i = 1; i <= 8; i++) {
		memset(tile, 0, 32); // clear the tile with color index 0
		// 8 sets
		for (u16 c = 0; c < 8; c++) {
			// visit the rows of each tile in current set
			for (u16 j = i-1; j < 8; j++) {
				// visit the columns of current row. Tile width: 4 pixels = 2 bytes
				for (u16 b = 0; b < 2; b++) {
					tile[4*j + b] = (c+0) | ((c+1) << 4);
				}
			}
			VDP_loadTileData((u32*)tile, i + c*8, 1, CPU);
		}
	}

	MEM_free(tile);

	// Returns next available tile index in VRAM
	return HUD_VRAM_START_INDEX; // 9*8
}

int main (bool hardReset)
{
	// On soft reset do like a hard reset
	if (!hardReset) {
		VDP_waitDMACompletion(); // avoids some glitches as per Genesis Manual's Addendum section
		SYS_hardReset();
	}

	VDP_setEnable(FALSE);

	// Basic game setup
	//loadPlaneDisplacements();
	u16 currentTileIndex = loadRenderingTiles();
	currentTileIndex = hud_loadInitialState(currentTileIndex);
    hint_setupPals();
    SPR_initEx(weapon_biggerSummationAnimTiles());
    weapon_resetState();
    hud_addWeapon(WEAPON_FIST);
    hud_addWeapon(WEAPON_PISTOL);
    hud_addHealthUnits(100); // replace by health_add(100) which inside calls hud_addHealthUnits(100)
    weapon_select(WEAPON_PISTOL);
    weapon_addAmmo(WEAPON_PISTOL, 50);

	MEM_pack();

	// Setup VDP
	VDP_setScreenWidth320();
	VDP_setPlaneSize(PLANE_COLUMNS, 32, TRUE);
	VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
	VDP_setHorizontalScroll(BG_A, 0);
	VDP_setHorizontalScroll(BG_B, 4); // offset second plane by 4 pixels
    VDP_setWindowHPos(FALSE, HUD_XP);
    VDP_setWindowVPos(TRUE, HUD_YP);
	//VDP_setBackgroundColor(1); // this set grey as bg color so floor and roof are the same color
	PAL_setColor(0, 0x0222); //palette_grey[1]

    VDP_setEnable(TRUE);

	SYS_disableInts();
	{
		SYS_setVBlankCallback(vint_callback);
        #if USE_DIF_FLOOR_AND_ROOF_COLORS
		// Scanline location for the HUD is (224-32)-2 (2 scanlines earlier to prepare dma and complete the first palette burst).
		// The color change between roof and floor has to be made at (224-32)/2 of framebuffer but at a scanline multiple of HUD location.
		// 95 is approx at mid framebbufer, and 95*2 = (224-32)-2 which is the start of HUD loading palettes logic.
		VDP_setHIntCounter(HUD_HINT_SCANLINE_CHANGE_BG_COLOR-1); // -1 since hintcounter is 0 based
        #else
        VDP_setHIntCounter((224-32)-2);
        #endif
    	SYS_setHIntCallback(hint_callback);
    	VDP_setHInterrupt(TRUE);
	}
	SYS_enableInts();

    // In case we enqueued some DMA ops
	SYS_doVBlankProcess();

	// For some unknown reason if we use a game_loop.h and declare gameLoop() function, later when defining 
	// the body of the function in its own game_loop.c unit it crashes the emulator/console with an illegal instruction.
	// So solution is: remove game_loop.h, declare gameLoop() as extern in game_loop.c, and just call it here.
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
	gameLoop();
	#pragma GCC diagnostic pop

	SYS_disableInts();
	{
		SYS_setVBlankCallback(NULL);
		VDP_setHInterrupt(FALSE);
    	SYS_setHIntCallback(NULL);
	}
	SYS_enableInts();

    SPR_reset();

	PAL_setColor(0, 0x000); // Black BG color
	VDP_clearPlane(BG_B, TRUE);
	VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(WINDOW, TRUE);
	VDP_setHorizontalScroll(BG_A, 0);
	VDP_setHorizontalScroll(BG_B, 0);
    VDP_setWindowHPos(FALSE, 0);
    VDP_setWindowVPos(TRUE, 0);

	return 0;
}

#pragma GCC pop_options