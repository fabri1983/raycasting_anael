// MIT License - Copyright (c) 2024 Anaël Seghezzi
// raymarching rendering with precomputed tiles interleaved in both planes at 60fps
// full screen in mode 256x224 with 4 pixels wide column (effective resolution 64x224)
// it's possible to use 60+1 colors, but only 32+1 are used here
// reference for raymarching: https://lodev.org/cgtutor/raycasting.html

// fabri1983's notes (since Aug/18/2024)
// -----------------
// * Rendered columns are 4 pixels wide, then 256p/4=64 or 320p/4=80 "pixels" columns.
// * write_vline_full() moved into write_vline() so we save 1 condition before the call decision to both methods.
// * write_vline() translated into inline asm => 2% saved in cpu usage.
// * Replaced   if ((joy & BUTTON_LEFT) || (joy & BUTTON_RIGHT))
//   by         if (joy & (BUTTON_LEFT | BUTTON_RIGHT))
//   Same with BUTTON_UP and BUTTON_DOWN.
// * Replaced dirX and dirY calculation in relation to the angle by two tables defined in tab_dir_xy.h => some cycles saved.
// * Replaced DMA_doDmaFast() by DMA_queueDmaFast() => 1% saved in cpu usage.
// * clear_buffer() replaced by inline asm to take advantage of compiler optimizations => %1 saved in cpu usage.
// * If clear_buffer() is moved into VBlank callback => %1 saved in cpu usage, but runs into next display period.
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
#include "map_matrix.h"
#include "frame_buffer.h"
#include "process_column.h"
#include "tab_dir_xy.h"
#if USE_TAB_DELTAS_3 & !SHOW_TEXCOORD
#include "tab_deltas_3.h"
#else
#include "tab_deltas.h"
#endif

// Load render tiles in VRAM. 9 set of 8 tiles each => 72 tiles in total.
// Returns next available tile index in VRAM.
static u16 loadRenderTiles () {
	// Create a buffer tile
	u8* tile = MEM_alloc(32); // 32 bytes per tile, layout: tile[4][8]
	memset(tile, 0, 32);

	// 9 possible tile heights

	// Tile with height 0 is at index 0
	VDP_loadTileData((u32*)tile, 0, 1, CPU);

	// Remaining 8 possible tile heights, distributed in 8 sets

	// 8 tiles per set
	for (u16 i = 1; i <= 8; i++) {
		memset(tile, 0, 32); // clear the tile
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

HINTERRUPT_CALLBACK hintCallbackHUD () {
	// Prepare DMA cmd and source address for first palette
    u32 palCmd = VDP_DMA_CRAM_ADDR((PAL0 * 16 + 1) * 2); // target starting color index multiplied by 2
    u32 fromAddrForDMA = (u32) (img_hud.palette->data + 1) >> 1; // TODO: this can be set outside the HInt

    // This seems to take an entire scanline, if you turn off VDP then it will draw black scanline (or whatever the BG color is) as a way off measure.
    setupDMAForPals(15, fromAddrForDMA);

    // At this moment we are at the middle/end of the scanline due to the previous DMA setup.
    // So we need to wait for next HBlank (indeed some pixels before to absorb some overhead)
    waitHCounter(146);

    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
    turnOnVDP(0x74);

	// Prepare DMA cmd and source address for second palette
    palCmd = VDP_DMA_CRAM_ADDR((PAL1 * 16 + 1) * 2); // target starting color index multiplied by 2
    fromAddrForDMA = (u32) (img_hud.palette->data + 17) >> 1; // TODO: this can be set outside the HInt

	// This seems to take an entire scanline, if you turn off VDP then it will draw black scanline (or whatever the BG color is) as a way off measure.
    setupDMAForPals(15, fromAddrForDMA);

	// At this moment we are at the middle/end of the scanline due to the previous DMA setup.
    // So we need to wait for next HBlank (indeed some pixels before to absorb some overhead)
    waitHCounter(146);

    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
    turnOnVDP(0x74);

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
	u16 currentTileIndex = loadRenderTiles();
	loadHUD(currentTileIndex);

	MEM_pack();

	SYS_disableInts();
	{
		SYS_setVBlankCallback(vBlankCallback);
		VDP_setHIntCounter((224-32)-2); // scanline location for the HUD
    	SYS_setHIntCallback(hintCallbackHUD);
    	VDP_setHInterrupt(TRUE);
	}
	SYS_enableInts();

	// Setup VDP
	VDP_setScreenWidth320();
	VDP_setPlaneSize(PLANE_COLUMNS, 32, TRUE);
	VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
	VDP_setHorizontalScroll(BG_A, 0);
	VDP_setHorizontalScroll(BG_B, 4); // offset second plane by 4 pixels
	PAL_setColor(0, 0x0222); // palette_grey[1]
	//VDP_setBackgroundColor(1);

	VDP_setEnable(TRUE);

	SYS_doVBlankProcess();

	u16 posX = 2 * FP, posY = 2 * FP;
	u16 angle = 0; // angle max value is 1023 and is updated in +/- 8 units

	for (;;)
	{
		// clear the frame buffer
		clear_buffer((u8*)frame_buffer);

		// handle inputs
		u16 joy = JOY_readJoypad(JOY_1);
		if (joy) {

			// if (joy & BUTTON_START) break;

			// movement and collisions
			if (joy & (BUTTON_UP | BUTTON_DOWN)) {

				s16 dx = tab_dir_x_div24[angle];
				s16 dy = tab_dir_y_div24[angle];
				if (joy & BUTTON_DOWN) {
					dx = -dx;
					dy = -dy;
				}

				u16 x = posX / FP;
				u16 y = posY / FP;
				const u16 yt = (posY-63) / FP;
				const u16 yb = (posY+63) / FP;
				posX += dx;
				posY += dy;

				// x collision
				if (dx > 0) {
					if (map[y][x+1] || map[yt][x+1] || map[yb][x+1]) {
						posX = min(posX, (x+1)*FP-64);
						x = posX / FP;
					}
				}
				else {
					if (x == 0 || map[y][x-1] || map[yt][x-1] || map[yb][x-1]) {
						posX = max(posX, x*FP+64);
						x = posX / FP;
					}
				}

				const u16 xl = (posX-63) / FP;
				const u16 xr = (posX+63) / FP;

				// y collision
				if (dy > 0) {
					if (map[y+1][x] || map[y+1][xl] || map[y+1][xr])
						posY = min(posY, (y+1)*FP-64);
				}
				else {
					if (y == 0 || map[y-1][x] || map[y-1][xl] || map[y-1][xr])
						posY = max(posY, y*FP+64);
				}
			}

			// rotation
			if (joy & BUTTON_LEFT)
				angle = (angle + 8) & 1023;
			if (joy & BUTTON_RIGHT)
				angle = (angle - 8) & 1023;
		}

		// DDA
		{
			// Value goes from 0 up to 256 (FP)
			u16 sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1;
			sideDistX_l0 = posX & (FP - 1); // (posX - (posX/FP)*FP);
			sideDistX_l1 = FP - sideDistX_l0; // (((posX/FP) + 1)*FP - posX);
			sideDistY_l0 = posY & (FP - 1); // (posY - (posY/FP)*FP);
			sideDistY_l1 = FP - sideDistY_l0; // (((posY/FP) + 1)*FP - posY);

			u16 a = angle / (1024 / AP); // a range is [0, 128)
			#if USE_TAB_DELTAS_3 & !SHOW_TEXCOORD
			u16* delta_a_ptr = (u16*) (tab_deltas + (a * PIXEL_COLUMNS * 3));
			#else
			u16* delta_a_ptr = (u16*) (tab_deltas + (a * PIXEL_COLUMNS * 4));
			#endif
			#if USE_TAB_MULU_DIST_DIV256
			a *= PIXEL_COLUMNS; // offset into tab_mulu_dist_div256[...][a]
			#endif

			// 256p or 320p width but 4 "pixels" wide column => effectively 256/4=64 or 320/4=80 pixels width.
			for (u8 column = 0; column < PIXEL_COLUMNS; column += 4) {
				process_column(column+0, &delta_a_ptr, a, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
				process_column(column+1, &delta_a_ptr, a, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
				process_column(column+2, &delta_a_ptr, a, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
				process_column(column+3, &delta_a_ptr, a, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
			}
		}

		// Enqueue the frame buffer for DMA during VBlank period
		// Remaining 1/2 of PA
		//DMA_queueDmaFast(DMA_VRAM, frame_buffer + (VERTICAL_COLUMNS*PLANE_COLUMNS)/2, PA_ADDR + HALF_PLANE_ADDR_OFFSET, (VERTICAL_COLUMNS*PLANE_COLUMNS)/2 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
		// Remaining 3/4 of PA
		//DMA_queueDmaFast(DMA_VRAM, frame_buffer + (VERTICAL_COLUMNS*PLANE_COLUMNS)/4, PA_ADDR + QUARTER_PLANE_ADDR_OFFSET, ((VERTICAL_COLUMNS*PLANE_COLUMNS)/4)*3 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
		DMA_queueDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, VERTICAL_COLUMNS*PLANE_COLUMNS - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
		DMA_queueDmaFast(DMA_VRAM, frame_buffer + VERTICAL_COLUMNS*PLANE_COLUMNS, PB_ADDR, VERTICAL_COLUMNS*PLANE_COLUMNS - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);

		SYS_doVBlankProcessEx(ON_VBLANK_START);

		//showFPS(0, 1);
		showCPULoad(0, 1);
	}

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