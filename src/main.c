// MIT License - Copyright (c) 2024 Anaël Seghezzi
// Ray marching rendering with precomputed tiles interleaved in both planes at 60fps.
// Full screen in mode 256x224 with 4 pixels wide column (effective resolution 64x224),
// It's possible to use 60+1 colors, but only 32+1 are used here.
// Reference for ray marching: https://lodev.org/cgtutor/raycasting.html

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

#include <tools.h>

int main (bool hardReset)
{
	// On soft reset we do a hard reset
	if (!hardReset) {
		VDP_waitDMACompletion(); // avoids some glitches as per Genesis Manual's Addendum section
		SYS_hardReset();
	}

    // Ensure our constants has correct values
    if (((u32)vdpSpriteCache) != RAM_FIXED_VDP_SPRITE_CACHE_ADDRESS) {
        KLog_U2("RAM_FIXED_VDP_SPRITE_CACHE_ADDRESS mismatch: ", RAM_FIXED_VDP_SPRITE_CACHE_ADDRESS, " ", (u32)vdpSpriteCache);
        return 0;
    }
    //acá: check correct values for PB_ADDR, PW_ADDR_AT_HUD, PA_ADDR, VDP_SPRITE_LIST_ADDR

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
    SPR_initEx(weapon_biggestAnimTileNum()); // NOTE: + others xxx_biggerAnimTileNum()
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
	PAL_setColor(0, 0x0444); // palette_grey[2]=0x0444 roof color
    #elif RENDER_SET_ROOF_COLOR_RAMP_ONLY_HINT_MULTI_CALLBACKS
    PAL_setColor(0, 0x0666); // palette_grey[3]=0x0666 floor color
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