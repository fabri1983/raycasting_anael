// MIT License - Copyright (c) 2024 Anaël Seghezzi
// Raymarching rendering with precomputed tiles interleaved in both planes at 60fps.
// Full screen in mode 256x224 with 4 pixels wide column (effective resolution 64x224),
// It's possible to use 60+1 colors, but only 32+1 are used here.
// Reference for raymarching: https://lodev.org/cgtutor/raycasting.html

/*
fabri1983's notes (since Aug/18/2024)
-------------------------------------
* Ray Casting: https://lodev.org/cgtutor/raycasting.html
* DDA = Digital Differential Analysis.
* Modified to display full screen in 320x224.
* Only 6 button pad supported at the moment.
* Rendered columns are 4 pixels wide, then effectively 256p/4=64 or 320p/4=80 "pixels" columns.
* Using RAM_FIXED_xxx regions to store arrays. This way the setup for DMA operations can be prepared in fewer instructions.
  You must only use MEM_pack() before any resource is setup/reset that uses RAM_FIXED_xxx regions, and also ensure SGDK's inner 
  MEM_xxx() functions do too.
* Moved clear_buffer() into inline ASM to take advantage of compiler optimizations => %1 saved in cpu usage.
* clear_buffer() using SP (Stack Pointer) to point end of buffer => 60 cycles saved if PLANE_COLUMNS=32, and 
  150 cycles saved if PLANE_COLUMNS=64.
* If clear_buffer() is moved into VBlank callback => %1 saved in cpu usage, but runs into next display period.
* Replaced   if ((joy & BUTTON_LEFT) || (joy & BUTTON_RIGHT))
  by         if (joy & (BUTTON_LEFT | BUTTON_RIGHT))
  Same with BUTTON_UP and BUTTON_DOWN.
* Moved write_vline_full() into write_vline() and translated into inline ASM => ~1% saved in cpu usage.
* write_vline(): translated into inline ASM => 2% saved in cpu usage.
* write_vline(): optimized accesses for top and bottom tiles of current column made in ASM => 4% saved in cpu usage.
* write_vline(): exposing as global the column pointer instead of sending it as an argument. This made the 
  frame_buffer_pxcolumn[] useless, so no need for render_loadPlaneDisplacements() neither offset before 
  write_vline() => ~1% saved in cpu usage depending on the inline ASM constraints.
* Replaced dirX and dirY calculation in relation to the angle by two tables defined in tab_dir_xy.h => some cycles saved.
* Replaced DMA_doDmaFast() by DMA_queueDmaFast() => 1% saved in cpu usage but consumes all VBlank period and runs 
  into next display period.
* Changes in tab_wall_div.h so the start of the vertical line to be written is calculated ahead of time => 1% saved in cpu usage.
* Replaced variable d used for color calculation by two tables defined in tab_color_d8_1.h => 2% saved in cpu usage.
  There is also a replacement for tab_color_d8_1[] by using tables in tab_color_d8_pals_shft.h in asm which is slightly faster.
* Replaced mulu(sideDistX_l0, deltaDistX) >> FS by a table => ~4% saved in cpu usage, though rom size increased 
  460 KB (+ padding to 128KB boundary).
* DMA framebuffer row by row using ASM -> 3% saved in cpu usage (thanks Shannon Birt).
* Render bottom half of framebuffer and then mirror it into top region using multi scanline HInts.
  This reduce DMA presure during VBlank up to a 50%, but adds more CPU pressure during top half active display period.
* SGDK's SPR_update() function was modified to handle DMA for specific cases, and also removed unused features.
  See comments with tag fabri1983 at spr_eng_override.c => ~1% saved in cpu usage.
* sega.s: use _VINT_vtimer instead of SGDK's _VINT to only increment vtimer, since we are calling the vint callback manually
* SGDK's SYS_doVBlankProcessEx() function was modified to remove unwanted logic. See render.c => ~1% saved in cpu usage.
* SGDK's JOY_update() and JOY_readJoypad(JOY_1) functions were copied and modified to handle only 6 button joypad => 2% saved in cpu usage.
  at render_SYS_doVBlankProcessEx_ON_VBLANK() => some cycles saved.
* Commented out the #pragma directives for loop unrolling => ~1% saved in cpu usage. It may vary according the 
  use/abuse of FORCE_INLINE.
* Manual unrolling of 2 (or 4) iterations for column processing => 2% saved in cpu usage. It may vary according
  the use/abuse of FORCE_INLINE.

fabri1983's resources notes:
----------------------------
* HUD: the hud is treated as an image. The resource definition in hud_res.res file expects a map base attribute value which is 
  calculated before hand. See HUD_BASE_TILE_ATTRIB in hud.h.
* HUD: if resource is compressed then also set const HUD_TILEMAP_COMPRESSED in hud.h.
* WEAPONS: the location of sprite tiles is given by methods like weapon_getVRAMLocation(). Required for SPR_addSpriteEx().
* Additional free VRAM: Planes A and B bottom region covered by the HUD, which is in Window Plane, leaves unused VRAM.
  Window Plane leaves unused VRAM from where it begins down to the address where the HUD is displayed.
  See PB_FREE_VRAM_AT, PW_FREE_VRAM_AT, PA_FREE_VRAM_AT, and LAST_FREE_VRAM_AT at consts_ext.h.
*/

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
#include "consts_ext.h"
#include "frame_buffer.h"
#include "render.h"
#include "game_loop.h"
#include "vint_callback.h"
#include "hint_callback.h"
#include "hud.h"
#include "weapon.h"
#if DISPLAY_LOGOS_AT_START
#include "teddyBearLogo.h"
#endif
#if DISPLAY_TITLE_SCREEN
#include "title.h"
#endif

// The code is optimised further using GCC's automatic unrolling, but might not be the case if too much inlining is used (or whatever other reason).
#pragma GCC push_options
//#pragma GCC optimize ("unroll-loops")
#pragma GCC optimize ("no-unroll-loops")

int main (bool hardReset)
{
	// On soft reset we do a hard reset
	if (!hardReset) {
		VDP_waitDMACompletion(); // avoids some glitches as per Genesis Manual's Addendum section
        SPR_end();
		SYS_hardReset();
	}

    // Ensure our constants has correct values
    if (((u32)vdpSpriteCache) != RAM_FIXED_VDP_SPRITE_CACHE_ADDRESS)
        return 0;
    //acá: check for correct values for PB_ADDR, PW_ADDR_AT_HUD, PA_ADDR, VDP_SPRITE_LIST_ADDR

    #if DISPLAY_LOGOS_AT_START
    displayTeddyBearLogo();
    #endif

    #if DISPLAY_TITLE_SCREEN
    title_vscroll_2_planes_show();
    #endif

    // Restart DMA with this settings
    DMA_initEx(DMA_QUEUE_SIZE_MIN, 0, 0);

    // ----------------------
	// Basic game setup
    // ----------------------

    fb_allocate_frame_buffer();
    render_loadWallPalettes();
    vint_reset();
    hint_reset();
	u16 currentTileIndex = render_loadTiles();
	currentTileIndex = hud_loadInitialState(currentTileIndex);
    SPR_initEx(weapon_biggerAnimTileNum()); // NOTE: + others xxx_biggerAnimTileNum()
    weapon_resetState();

    hud_addWeapon(WEAPON_FIST);
    hud_addWeapon(WEAPON_PISTOL);
    hud_addWeapon(WEAPON_SHOTGUN);
    hud_addHealthUnits(100); // NOTE: needs to be replaced by health_add(100), from which it will call hud_addHealthUnits(100)
    weapon_select(WEAPON_PISTOL);
    weapon_addAmmo(WEAPON_PISTOL, 50);
    weapon_addAmmo(WEAPON_SHOTGUN, 50);

    // ----------------------
	// Setup VDP
    // ----------------------

	VDP_setScreenWidth320();
	VDP_setPlaneSize(PLANE_COLUMNS, 32, TRUE);
	VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
	VDP_setHorizontalScroll(BG_A, 0);
	VDP_setHorizontalScroll(BG_B, 4); // offset plane by 4 pixels
    VDP_setWindowHPos(FALSE, HUD_XP);
    VDP_setWindowVPos(TRUE, HUD_YP);
    #if RENDER_SET_FLOOR_AND_ROOF_COLORS_ON_HINT
	PAL_setColor(0, 0x0222); // palette_grey[1]=0x0222 roof color
    #elif RENDER_SET_ROOF_COLOR_RAMP_ONLY_HINT_MULTI_CALLBACKS
    PAL_setColor(0, 0x0666); // palette_grey[2]=0x0444 floor color
    #else
    PAL_setColor(0, 0x0444); // palette_grey[2]=0x0444 floor color
    #endif

	SYS_disableInts();

    #if RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT | RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT_MULTI_CALLBACKS
    hint_reset_mirror_planes_state();
    #elif RENDER_SET_FLOOR_AND_ROOF_COLORS_ON_HINT
    hint_reset_change_bg_state();
    #endif
    SYS_setVIntCallback(vint_callback);

    #if RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT_MULTI_CALLBACKS
    VDP_setHIntCounter(0); // every scanline
    SYS_setHIntCallback(hint_mirror_planes_callback_asm_0);
    #elif RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT
    VDP_setHIntCounter(0); // every scanline
    SYS_setHIntCallback(hint_mirror_planes_callback);
    #elif RENDER_SET_FLOOR_AND_ROOF_COLORS_ON_HINT
    VDP_setHIntCounter(HINT_SCANLINE_MID_SCREEN - 1); // -1 because scanline counter is 0-based
    SYS_setHIntCallback(hint_change_bg_callback);
    #else
    VDP_setHIntCounter(HINT_SCANLINE_START_PAL_SWAP - 1); // -1 because scanline counter is 0-based
    SYS_setHIntCallback(hint_load_hud_pals_callback);
    #endif
    VDP_setHInterrupt(TRUE);

	SYS_enableInts();

    // ----------------------
    // Game Loop
    // ----------------------

	game_loop();

    // ----------------------
    // Clear used VRAM
    // ----------------------

    fb_free_frame_buffer();
    hud_free_src_buffer();
    hud_free_dst_buffer();
    hud_free_pals_buffer();
    weapon_free_pals_buffer();

	return 0;
}

#pragma GCC pop_options