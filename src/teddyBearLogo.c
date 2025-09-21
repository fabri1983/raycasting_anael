/**
 * Teddy Bear animation author: fabri1983@gmail.com
 * https://github.com/fabri1983
*/

#include <types.h>
#include <vdp.h>
#include <vdp_bg.h>
#include <vdp_tile.h>
#include <memory.h>
#include <joy.h>
#include <sys.h>
#include <sprite_eng.h>
#include "teddyBearLogo.h"
#include "utils.h"
#include "logos_res.h"

#define STR_VERSION "v2.12 (Aug 2025)"
#define STR_VERSION_LEN 17 // String version length including \0

#define TEDDY_BEAR_LOGO_FADE_TO_BLACK_STEPS 7 // How many steps needs to be applied as much to reach black color. Max is 7.
#define TEDDY_BEAR_LOGO_FADE_TO_BLACK_STEP_FREQ 3 // Every N frames we apply one fade to black step.

static void fadeToBlack_logo ()
{
	u16* palsBuffer = MEM_alloc(16 * sizeof(u16));
    memcpy(palsBuffer, sprDefTeddyBearAnim.palette->data, 16 * sizeof(u16));

	// Always multiple of FADE_TO_BLACK_STEPS since a uniform fade to black effect lasts FADE_TO_BLACK_STEPS steps.
	s16 loopFrames = TEDDY_BEAR_LOGO_FADE_TO_BLACK_STEPS * TEDDY_BEAR_LOGO_FADE_TO_BLACK_STEP_FREQ;

	while (loopFrames >= 0) {
		if ((loopFrames-- % TEDDY_BEAR_LOGO_FADE_TO_BLACK_STEP_FREQ) == 0) {
			u16* pals_ptr = palsBuffer;
			for (u16 i=16; i--;) {
                u16 d = *pals_ptr - 0x222; // decrement 1 unit on every component
                switch (d & 0b1000100010000) {
					case 0b0000000010000: d &= ~0b0000000011110; break; // red overflows? then zero it
					case 0b0000100010000: d &= ~0b0000111111110; break; // red and green overflow? then zero them
					case 0b0000100000000: d &= ~0b0000111100000; break; // green overflows? then zero it
					case 0b1000000010000: d &= ~0b1111000011110; break; // red and blue overflow? then zero them
					case 0b1000000000000: d &= ~0b1111000000000; break; // blue overflows? then zero it
					case 0b1000100000000: d &= ~0b1111111100000; break; // green and blue overflow? then zero them
					case 0b1000100010000: d = 0; break; // all colors overflow, then zero them
					default: break;
                }
                *pals_ptr++ = d;
            }
		}

        DMA_queueDmaFast(DMA_CRAM, palsBuffer, PAL3 * 16 * sizeof(u16), 16, 2);

        SYS_doVBlankProcess();
	}

    MEM_free(palsBuffer);
}

void displayTeddyBearLogo ()
{
    //
    // Setup SGDK Teddy Bear animation
    //

    // Init Sprites engine with fixed vram size (in tiles)
    SPR_initEx(sprDefTeddyBearAnim.maxNumTile);

    u16 tileIndexNext = TILE_USER_INDEX;
    u16 teddyBearAnimTileAttribsBase = TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, tileIndexNext);
    u16 sprX = (screenWidth - sprDefTeddyBearAnim.w) / 2;
    u16 sprY = (screenHeight - sprDefTeddyBearAnim.h) / 2;
    Sprite* teddyBearAnimSpr = SPR_addSpriteEx(&sprDefTeddyBearAnim, sprX, sprY, teddyBearAnimTileAttribsBase,
        SPR_FLAG_AUTO_TILE_UPLOAD | SPR_FLAG_DISABLE_ANIMATION_LOOP);
    tileIndexNext += sprDefTeddyBearAnim.maxNumTile;
    
    // Draw first frame with only black palette to later do the fade in
    PAL_setPalette(PAL3, palette_black, CPU);
    SPR_update();

    //
    // Fade In sprite and text
    //

    // Draw SGDK version string
    VDP_setTextPalette(PAL3);
    VDP_drawText((const char*) STR_VERSION, screenWidth/8 - STR_VERSION_LEN, screenHeight/8 - 1);

    // Fade in to Sprite palette (previously loaded at PAL3)
    PAL_fadeIn(PAL3*16, PAL3*16 + (16-1), sprDefTeddyBearAnim.palette->data, 30, TRUE);
    // process fading immediately
    do {
        SYS_doVBlankProcess();
        const u16 joyState = JOY_readJoypad(JOY_1);
        if (joyState & BUTTON_START)
            break;
    }
    while (PAL_doFadeStep());
    // final update
    SYS_doVBlankProcess();

    //
    // Display loop for SGDK Teddy Bear animation
    //

    for (;;)
    {
        const u16 joyState = JOY_readJoypad(JOY_1);
        if (joyState & BUTTON_START)
            break;

        if (SPR_isAnimationDone(teddyBearAnimSpr)) {
            // Leave some time the last animation frame in the screen
            waitMs_(250);
            break;
        }

        SPR_update();

        SYS_doVBlankProcess();
    }

    // Fade out all graphics to Black
    // NOTE: SKIP THE FADE OUT TO AVOID ADDRESS ERROR WITH THE RAY CASTER
    //PAL_fadeOutAll(30, FALSE);
    fadeToBlack_logo();

    SPR_end();
    SYS_doVBlankProcess();

    // restore planes
    VDP_clearPlane(BG_A, TRUE);
    // Restore SGDK's font palette (VDP_resetScreen() doesn't restore it accordingly)
    VDP_setTextPalette(PAL0);
    // restore SGDK's default palettes
    PAL_setPalette(PAL3, palette_blue, CPU);
}