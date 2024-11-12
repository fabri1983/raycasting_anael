// MIT License - Copyright (c) 2024 Anaël Seghezzi
// raymarching rendering with precomputed tiles interleaved in both planes at 60fps
// full screen in mode 256x224 with 4 pixels wide column (effective resolution 64x224)
// it's possible to use 60+1 colors, but only 32+1 are used here
// reference for raymarching: https://lodev.org/cgtutor/raycasting.html

// fabri1983's notes (since Aug/18/2024)
// -------------------------------------
// * Ray Casting: https://lodev.org/cgtutor/raycasting.html
// * DDA = Digital Differential Analysis.
// * Modified to display full screen in mode 320x224.
// * Only 6 button pad supported.
// * Rendered columns are 4 pixels wide, then effectively 256p/4=64 or 320p/4=80 "pixels" columns.
// * Moved clear_buffer() into inline ASM to take advantage of compiler optimizations => %1 saved in cpu usage.
// * clear_buffer() using SP (Stack Pointer) to point end of buffer => 60 cycles saved if PLANE_COLUMNS=32, and 
//   150 cycles saved if PLANE_COLUMNS=64.
// * If clear_buffer() is moved into VBlank callback => %1 saved in cpu usage, but runs into next display period.
// * Moved write_vline_full() into write_vline() and translated into inline ASM => ~1% saved in cpu usage.
// * write_vline(): translated into inline ASM => 2% saved in cpu usage.
// * write_vline(): access optimization for top and bottom tiles of current column in ASM => 4% saved in cpu usage.
// * write_vline(): exposing globally the column pointer instead of sending it as an argument. This made the 
//   frame_buffer_pxcolumn[] useless, so no need for render_loadPlaneDisplacements() neither offset before 
//   write_vline() => ~1% saved in cpu usage depending on the inline ASM constraints.
// * Replaced   if ((joy & BUTTON_LEFT) || (joy & BUTTON_RIGHT))
//   by         if (joy & (BUTTON_LEFT | BUTTON_RIGHT))
//   Same with BUTTON_UP and BUTTON_DOWN.
// * Replaced dirX and dirY calculation in relation to the angle by two tables defined in tab_dir_xy.h => some cycles saved.
// * Replaced DMA_doDmaFast() by DMA_queueDmaFast() => 1% saved in cpu usage but consumes all VBlank period and runs 
//   into next display period.
// * Changes in tab_wall_div.h so the start of the vertical line to be written is calculated ahead of time => 1% saved in cpu usage.
// * Replaced variable d used for color calculation by two tables defined in tab_color_d8.h => 2% saved in cpu usage.
// * Replaced color calculation out of tab_color_d8[] by using tables in tab_color_d8_pals_shft.h => 1% saved in cpu usage.
// * Replaced (mulu(sideDistX_l0, deltaDistX) >> FS) by a table => ~4% saved in cpu usage, though rom size increased 
//   460 KB (+ padding to 128KB boundary).
// * sega.s: use _VINT_lean instead of _VINT to enter vint callback as soon as possible => some cycles saved.
// * Commented out the #pragma directives for loop unrolling => ~1% saved in cpu usage. It may vary according the 
//   use/abuse of FORCE_INLINE.
// * Manual unrolling of 2 (or 4) iterations for column processing => 2% saved in cpu usage. It may vary according the 
//   use/abuse of FORCE_INLINE.
// * SGDK's SPR_update() function was modified to handle DMA for specific cases, and also cut off unused features.
//   See comments with tag fabri1983 at spr_eng_override.c => ~1% saved in cpu usage.
// * SGDK's SYS_doVBlankProcessEx() function was modified to cut off unwanted logic. See render.c => ~1% saved in cpu usage.
// * SGDK's JOY_update() and JOY_readJoypad(JOY_1) functions were modified to handle only 6 button joypad => 2% saved in cpu usage.

// fabri1983's resources notes:
// ----------------------------
// * HUD: the hud is treated as an image. The resource definition in .res file expects a map base attribute value which is 
//   calculated before hand. See HUD_BASE_TILE_ATTRIB in hud.h.
// * HUD: if resource is compressed then also set const HUD_TILEMAP_COMPRESSED in hud.h.
// * WEAPONS: the calculation of max tiles used is method weapon_biggerAnimTileNum(). Once the value is known, you can 
//   replace the calculation instructions by just a fixed value.
// * WEAPONS: the location of sprite tiles is given by methods like weapon_getVRAMLocation(). Required for SPR_addSpriteEx().

#include <types.h>
#include <dma.h>
#include <sys.h>
#include <memory.h>
#include <vdp.h>
#include <vdp_bg.h>
#include <pal.h>
#include <sprite_eng.h>
#include "utils.h"
#include "consts.h"
#include "frame_buffer.h"
#include "render.h"
#include "game_loop.h"
#include "vint_callback.h"
#include "hint_callback.h"
#include "hud.h"
#include "weapons.h"
#if DISPLAY_LOGOS_AT_START
#include "segaLogo.h"
#include "teddyBearLogo.h"
#endif

// the code is optimised further using GCC's automatic unrolling, but might not be true if too much inlining is used (or whatever reason).
#pragma GCC push_options
//#pragma GCC optimize ("unroll-loops")
#pragma GCC optimize ("no-unroll-loops")

int main (bool hardReset)
{
	// On soft reset we do like a hard reset
	if (!hardReset) {
		VDP_waitDMACompletion(); // avoids some glitches as per Genesis Manual's Addendum section
        SPR_end();
		SYS_hardReset();
	}

    #if DISPLAY_LOGOS_AT_START
    displaySegaLogo();
    waitMs_(200);
    displayTeddyBearLogo();
    waitMs_(200);
    #endif

    // Restart DMA with this settings
    DMA_initEx(20, 8192, 8192);

	VDP_setEnable(FALSE);

	// Basic game setup

	//render_loadPlaneDisplacements(); // not needed anymore, see details above in description
    render_loadWallPalettes();
    vint_reset();
    hint_reset();
	u16 currentTileIndex = render_loadTiles();
	currentTileIndex = hud_loadInitialState(currentTileIndex);
    SPR_initEx(weapon_biggerAnimTileNum()); // + others xxx_biggerAnimTileNum()
    weapon_resetState();

    hud_addWeapon(WEAPON_FIST);
    hud_addWeapon(WEAPON_PISTOL);
    hud_addWeapon(WEAPON_SHOTGUN);
    hud_addHealthUnits(100); // replace by health_add(100), from which it will call hud_addHealthUnits(100)
    weapon_select(WEAPON_PISTOL);
    weapon_addAmmo(WEAPON_PISTOL, 50);
    weapon_addAmmo(WEAPON_SHOTGUN, 50);

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
    #if HUD_SET_FLOOR_AND_ROOF_COLORS_ON_HINT && !HUD_SET_FLOOR_AND_ROOF_COLORS_ON_WRITE_VLINE
	PAL_setColor(0, 0x0222); //palette_grey[1] // roof color
    #else
    PAL_setColor(0, 0x0444); //palette_grey[2] // roof color
    #endif

    VDP_setEnable(TRUE);

	SYS_disableInts();
	{
        // It has no effect because we are manually calling it on render_SYS_doVBlankProcessEx_ON_VBLANK() at render.c.
        // We leave it in case we start using SGDK's SYS_doVBlankProcessEx().
		SYS_setVBlankCallback(vint_callback);

        #if HUD_SET_FLOOR_AND_ROOF_COLORS_ON_HINT && !HUD_SET_FLOOR_AND_ROOF_COLORS_ON_WRITE_VLINE
		// Scanline location for the HUD is (224-32)-2 (2 scanlines earlier to prepare dma and complete the first palette burst).
		// The color change between roof and floor has to be made at (224-32)/2 of framebuffer but at a scanline multiple of HUD location.
		// 95 is approx at mid framebbufer, and 95*2 = (224-32)-2 which is the start of HUD loading palettes logic.
		VDP_setHIntCounter(HUD_HINT_SCANLINE_CHANGE_ROOF_BG_COLOR-1); // -1 since hintcounter is 0 based
        #else
        VDP_setHIntCounter(HUD_HINT_SCANLINE_START_PAL_SWAP-2); // 2 scanlines earlier so we have enough time for the DMA of palettes
        #endif
    	SYS_setHIntCallback(hint_callback);
    	VDP_setHInterrupt(TRUE);
	}
	SYS_enableInts();

    // In case we enqueued some DMA ops
	SYS_doVBlankProcess();

	game_loop();

	SYS_disableInts();
	{
		SYS_setVBlankCallback(NULL);
		VDP_setHInterrupt(FALSE);
    	SYS_setHIntCallback(NULL);
	}
	SYS_enableInts();

    SPR_end();

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