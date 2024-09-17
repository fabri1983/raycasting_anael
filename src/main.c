// MIT License - Copyright (c) 2024 Anaël Seghezzi
// raymarching rendering with precomputed tiles interleaved in both planes at 60fps
// full screen in mode 256x224 with 4 pixels wide column (effective resolution 64x224)
// it's possible to use 60+1 colors, but only 32+1 are used here
// reference for raymarching: https://lodev.org/cgtutor/raycasting.html

// fabri1983's notes (since Aug/18/2024)
// -----------------
// * Rendered columns are 4 pixels wide, then effectively 256p/4=64 or 320p/4=80 "pixels" columns.
// * Moved clear_buffer() into inline ASM to take advantage of compiler optimizations => %1 saved in cpu usage.
// * If clear_buffer() is moved into VBlank callback => %1 saved in cpu usage, but runs into next display period.
// * Moved write_vline_full() into write_vline() and translated into inline ASM.
// * write_vline() translated into inline ASM => 2% saved in cpu usage.
// * Replaced   if ((joy & BUTTON_LEFT) || (joy & BUTTON_RIGHT))
//   by         if (joy & (BUTTON_LEFT | BUTTON_RIGHT))
//   Same with BUTTON_UP and BUTTON_DOWN.
// * Replaced dirX and dirY calculation in relation to the angle by two tables defined in tab_dir_xy.h => some cycles saved.
// * Replaced DMA_doDmaFast() by DMA_queueDmaFast() => 1% saved in cpu usage.
// * Changes in tab_wall_div.h so the start of the vertical line to be written is calculated ahead of time => 1% saved in cpu usage.
// * Replaced d for color calculation by two tables defined in tab_color_d8.h => 2% saved in cpu usage.
// * Commented out the #pragma directives for loop unrolling => ~1% saved in cpu usage.
// * Manual unrolling of 4 iterations for column processing => 2% saved in cpu usage.

// the code is optimised further using GCC's automatic unrolling
//#pragma GCC push_options
//#pragma GCC optimize ("unroll-loops")

#include <genesis.h>
#include "hud_res.h"
#include "utils.h"
#include "consts.h"
#include "clear_buffer.h"
#include "frame_buffer.h"

// Load render tiles in VRAM. 9 set of 8 tiles each => 72 tiles in total.
// Returns next available tile index in VRAM.
static u16 loadRenderingTiles () {
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
				// visit the columns of each row. Tile width: 4 pixels = 2 bytes
				for (u16 b = 0; b < 2; b++) {
					tile[4*j + b] = (c+0) + ((c+1) << 4);
				}
			}
			VDP_loadTileData((u32*)tile, i + c*8, 1, CPU);
		}
	}

	MEM_free(tile);

	// Returns next available tile index in VRAM
	return 9*8;
}

static void loadHUD (u16 currentTileIndex) {
	const u16 baseTileAttrib = TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, currentTileIndex);
	VDP_loadTileSet(img_hud.tileset, baseTileAttrib & TILE_INDEX_MASK, DMA);

	const u16 yPos = PLANE_COLUMNS == 64 ? 24 : 12;
    VDP_setTileMapEx(BG_A, img_hud.tilemap, baseTileAttrib, 0, yPos, 0, 0, TILEMAP_COLUMNS, 4, DMA);
}

#define HINT_SCANLINE_CHANGE_BG_COLOR 95

HINTERRUPT_CALLBACK hIntCallback () {
	if (GET_VCOUNTER <= HINT_SCANLINE_CHANGE_BG_COLOR) {
		waitHCounter(156);
		// set background color used for the floor
		PAL_setColor(0, palette_grey[2]);
		return;
	}
	
	// We are one scanline earlier so wait until entering th HBlank region
	waitHCounter(152);

	// Prepare DMA cmd and source address for first palette
    u32 palCmd = VDP_DMA_CRAM_ADDR((PAL0 * 16 + 1) * 2); // target starting color index multiplied by 2
    u32 fromAddrForDMA = (u32) (img_hud.palette->data + 1) >> 1; // TODO: this can be set outside the HInt

    // This seems to take an entire scanline, if you turn off VDP then it will draw black scanline (or whatever the BG color is) as a way off measure.
    setupDMAForPals(15, fromAddrForDMA);

    // At this moment we are at the middle/end of the scanline due to the previous DMA setup.
    // So we need to wait for next HBlank (indeed some pixels before to absorb some overhead)
    waitHCounter(152);

    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
    turnOnVDP(0x74);

	// Prepare DMA cmd and source address for second palette
    palCmd = VDP_DMA_CRAM_ADDR((PAL1 * 16 + 1) * 2); // target starting color index multiplied by 2
    fromAddrForDMA = (u32) (img_hud.palette->data + 16 + 1) >> 1; // TODO: this can be set outside the HInt

	// This seems to take an entire scanline, if you turn off VDP then it will draw black scanline (or whatever the BG color is) as a way off measure.
    setupDMAForPals(15, fromAddrForDMA);

	// At this moment we are at the middle/end of the scanline due to the previous DMA setup.
    // So we need to wait for next HBlank (indeed some pixels before to absorb some overhead)
    waitHCounter(152);

    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
    turnOnVDP(0x74);

	// set background color used for the roof
	// THIS INTRODUCES A COLOR DOT GLITCH BARELY NOTICABLE. SOLUTION IS TO DO THIS IN VBLANK CALLBACK
	//PAL_setColor(0, palette_grey[1]);

	// Send 1/2 of the PA
	//DMA_doDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, (VERTICAL_COLUMNS*PLANE_COLUMNS)/2 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
	// Send 1/4 of the PA
	//DMA_doDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, (VERTICAL_COLUMNS*PLANE_COLUMNS)/4 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);

	// Reload the first 2 palettes that were overriden by the HUD palettes
	// waitVCounterReg(224);
	// // grey palette
	// palCmd = VDP_DMA_CRAM_ADDR((PAL0 * 16 + 1) * 2); // target starting color index multiplied by 2
    // fromAddrForDMA = (u32) (palette_grey + 1) >> 1; // TODO: this can be set outside the HInt (or maybe the compiler does it already)
	// turnOffVDP(0x74);
    // setupDMAForPals(8, fromAddrForDMA);
    // *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
	// // red palette
	// palCmd = VDP_DMA_CRAM_ADDR((PAL1 * 16 + 1) * 2); // target starting color index multiplied by 2
    // fromAddrForDMA = (u32) (palette_red + 1) >> 1; // TODO: this can be set outside the HInt (or maybe the compiler does it already)
    // setupDMAForPals(8, fromAddrForDMA);
    // *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
	// turnOnVDP(0x74);
}

void vBlankCallback () {
	// NOTE: AT THIS POINT THE DMA QUEUE HAS BEEN FLUSHED

	// Reload the first 2 palettes that were overriden by the HUD palettes
	// grey palette
	u32 palCmd = VDP_DMA_CRAM_ADDR((PAL0 * 16 + 1) * 2); // target starting color index multiplied by 2
    u32 fromAddrForDMA = (u32) (palette_grey + 1) >> 1; // TODO: this can be set outside the HInt (or maybe the compiler does it already)
	turnOffVDP(0x74);
    setupDMAForPals(8, fromAddrForDMA);
    *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
	// red palette
	palCmd = VDP_DMA_CRAM_ADDR((PAL1 * 16 + 1) * 2); // target starting color index multiplied by 2
    fromAddrForDMA = (u32) (palette_red + 1) >> 1; // TODO: this can be set outside the HInt (or maybe the compiler does it already)
    setupDMAForPals(8, fromAddrForDMA);
    *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer

	// set background color used for the roof
	PAL_setColor(0, palette_grey[1]);

	turnOnVDP(0x74);

	// clear the frame buffer
	//clear_buffer((u8*)frame_buffer);
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
	loadPlaneDisplacements();
	u16 currentTileIndex = loadRenderingTiles();
	loadHUD(currentTileIndex);

	MEM_pack();

	SYS_disableInts();
	{
		SYS_setVBlankCallback(vBlankCallback);
		// Scanline location for the HUD is (224-32)-2 (2 scanlines earlier to prepare dma and complete the first palette burst).
		// The color change between roof and floor has to be made at (224-32)/2 of framebuffer but at a scanline multiple of HUD location.
		// 95 is approx at mid framebbufer, and 95*2 = (224-32)-2 which is the start of HUD loading palettes logic.
		VDP_setHIntCounter(HINT_SCANLINE_CHANGE_BG_COLOR-1); // -1 since hintcounter is 0 based
    	SYS_setHIntCallback(hIntCallback);
    	VDP_setHInterrupt(TRUE);
	}
	SYS_enableInts();

	// Setup VDP
	VDP_setScreenWidth320();
	VDP_setPlaneSize(PLANE_COLUMNS, 32, TRUE);
	VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
	VDP_setHorizontalScroll(BG_A, 0);
	VDP_setHorizontalScroll(BG_B, 4); // offset second plane by 4 pixels
	//VDP_setBackgroundColor(1); // this set grey as bg color so floor and roof are the same color
	// set background color used for the roof
	PAL_setColor(0, palette_grey[1]);

	VDP_setEnable(TRUE);

	SYS_doVBlankProcess();

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
	gameLoop();
	//gameLoopAuto();
	#pragma GCC diagnostic pop

	SYS_disableInts();
	{
		SYS_setVBlankCallback(NULL);
		VDP_setHInterrupt(FALSE);
    	SYS_setHIntCallback(NULL);
	}
	SYS_enableInts();

	PAL_setColor(0, 0x000); // Black BG color
	VDP_clearPlane(BG_B, TRUE);
	VDP_clearPlane(BG_A, TRUE);
	VDP_setHorizontalScroll(BG_A, 0);
	VDP_setHorizontalScroll(BG_B, 0);
	VDP_drawText("Game Over", (TILEMAP_COLUMNS - 10)/2, (224/8)/2);

	return 0;
}