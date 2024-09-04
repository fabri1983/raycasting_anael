// MIT License - Copyright (c) 2024 Anaël Seghezzi
// raymarching rendering with precomputed tiles interleaved in both planes at 60fps
// full screen in mode 256x224 with 4 pixels wide column (effective resolution 64x224)
// it's possible to use 60+1 colors, but only 32+1 are used here
// reference for raymarching: https://lodev.org/cgtutor/raycasting.html

// fabri1983's notes (since Aug/18/2024)
// -----------------
// * Rendered columns are 4 pixels wide, then 256/4=64 "pixels", leaving us with an effective resolution of 64x224=14336 pixels.
// * write_vline_full() moved into write_vline() so we save 1 condition before the call decision to both methods.
//   Now is commented out given that the difference with write_vline() is the calculation of the top and bottom tiles.
// * write_vline() translated into inline asm => 2% saved in cpu usage.
// * Changed:
//   if ((joy & BUTTON_LEFT) || (joy & BUTTON_RIGHT))
//   by
//   if (joy & (BUTTON_LEFT | BUTTON_RIGHT))
//   Same with BUTTON_UP and BUTTON_DOWN.
// * Replaced dirX and dirY calculation in relation to the angle by two tables defined in tab_dir_xy.h => some cycles saved.
// * Moved clear_buffer() into VBlank callback => no visible gain but we take advantage of the VBlank period.
// * Replaced DMA_doDmaFast() by DMA_queueDmaFast() => 1% saved in cpu usage.
// * Changes in tab_wall_div.h so the start of the vertical line to be written is calculated ahead of time => 1% saved in cpu usage.
// * Replaced d for color calculation by two tables defined in tab_color_d8.h => 2% saved in cpu usage.
// * Commented out the #pragma directives for loop unrolling => ~1% saved in cpu usage.

// the code is optimised further using GCC's automatic unrolling
//#pragma GCC push_options
//#pragma GCC optimize ("unroll-loops")

#include <genesis.h>
#include "hud_res.h"
#include "utils.h"

#define SHOW_TEXCOORD FALSE // show texture coords?
#define USE_TAB_DELTAS_3 TRUE // Use smaller table with 3 columns where ray direction is encoded
#define USE_TAB_MULU_DIST_DIV256 FALSE // Use mega big table for mulu pre calculation

#define FS 8 // fixed point size in bits
#define FP (1<<FS) // fixed precision
#define STEP_COUNT 15 // (STEP_COUNT+1 should be a power of two)
#define AP 128 // angle precision (optimal for a rotation step of 8 : 1024/8 = 128)

// 224 px display height / 8 = 28. Remember that tiles are 8 pixels wide.
// The HUD we use the bottom 4 tiles.
#define VERTICAL_TILES 24

#include "tab_wall_div.h"
#if USE_TAB_DELTAS_3
#include "tab_deltas_3.h"
#else
#include "tab_deltas.h"
#endif
#include "tab_dir_xy.h"
#include "tab_color_d8.h"
#if USE_TAB_MULU_DIST_DIV256
#include "tab_mulu_dist_div256.h"
#endif

#define MAP_SIZE 16
static const u8 map[MAP_SIZE][MAP_SIZE] = {
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	{1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1},
	{1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1},
	{1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1},
	{1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
	{1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1},
	{1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1},
	{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1},
	{1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1},
	{1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1},
	{1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1},
	{1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1},
	{1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};

static FORCE_INLINE void write_vline (u16 *tilemap, u16 h2, u16 color)
{
	// Tilemap width in tiles.

	// draw a solid vertical line
	/*if (h2 == 0) {
		// C version
		//for (u16 y = 0; y < VERTICAL_TILES*32; y+=32) {
		// 	tilemap[y] = color;
		//}

		// inline ASM version
		__asm volatile (
			".set off,0\n"
			".rept %c[_VERTICAL_TILES]\n"
			"    move.w  %[_color],off(%[_tilemap])\n"
			"    .set off,off+32*2\n"
			".endr\n"
			: [_tilemap] "+a" (tilemap)
			: [_color] "d" (color), [_VERTICAL_TILES] "i" (VERTICAL_TILES)
			:
		);
		return;
	}*/

	u16 ta = (h2 / 8); // vertical tilemap entry position

	// top tilemap entry
	tilemap[ta*32] = color + (h2 & 7); // offset color by the halved pixel height modulo 8
	// bottom tilemap entry (with flipped attribute)
	tilemap[((VERTICAL_TILES-1)-ta)*32] = (color + (h2 & 7)) | (1 << TILE_ATTR_VFLIP_SFT);

	// VERTICAL_TILES-2 remaining vertical tilemap entries

	// C version (for 28 vertical tiles)
	/*switch (ta) {
		case 0:		tilemap[1*32] = color;  // fallthru
					tilemap[26*32] = color; // fallthru
		case 1:		tilemap[2*32] = color;  // fallthru
					tilemap[25*32] = color; // fallthru
		case 2:		tilemap[3*32] = color;  // fallthru
					tilemap[24*32] = color; // fallthru
		case 3:		tilemap[4*32] = color;  // fallthru
					tilemap[23*32] = color; // fallthru
		case 4:		tilemap[5*32] = color;  // fallthru
					tilemap[22*32] = color; // fallthru
		case 5:		tilemap[6*32] = color;  // fallthru
					tilemap[21*32] = color; // fallthru
		case 6:		tilemap[7*32] = color;  // fallthru
					tilemap[20*32] = color; // fallthru
		case 7:		tilemap[8*32] = color;  // fallthru
					tilemap[19*32] = color; // fallthru
		case 8:		tilemap[9*32] = color;  // fallthru
					tilemap[18*32] = color; // fallthru
		case 9:		tilemap[10*32] = color; // fallthru
					tilemap[17*32] = color; // fallthru
		case 10:	tilemap[11*32] = color; // fallthru
					tilemap[16*32] = color; // fallthru
		case 11:	tilemap[12*32] = color; // fallthru
					tilemap[15*32] = color; // fallthru
		case 12:	tilemap[13*32] = color; // fallthru
					tilemap[14*32] = color; // fallthru
					break;
	}*/

	// inline ASM version
	__asm volatile (
		// offset comes already multiplied by 8 but we need its first 3 bits to be cleared
		"    andi.b  %[_clearBitsOffset],%[_offset]\n"
		// jump into table using with _offset
		"    jmp     .wvl_table_%=(%%pc,%[_offset].w)\n"
		// color assignment table from tile[1*32] up to [((VERTICAL_TILES-2)/2)*32] and from tile[26*32] down to [(((VERTICAL_TILES-2)/2)+1)*32]
		// Eg for VERTICAL_TILES=28: 
		//  from tile[1*32] up to [13*32]
		//  from tile[26*32] down to [14*32]
		".wvl_table_%=:\n"
		".set offup,1*32*2\n"
		".set offdown,(%c[_VERTICAL_TILES]-2)*32*2\n"
		".rept (%c[_VERTICAL_TILES]-2)/2\n"
		"    move.w  %[_color],offup(%[_tilemap])\n"
		"    move.w  %[_color],offdown(%[_tilemap])\n"
		"    .set offup,offup+32*2\n"
		"    .set offdown,offdown-32*2\n"
		".endr\n"
		: [_tilemap] "+a" (tilemap)
		: [_color] "d" (color), [_offset] "d" (h2), [_clearBitsOffset] "i" (~(8-1)), [_VERTICAL_TILES] "i" (VERTICAL_TILES)
		:
	);
}

HINTERRUPT_CALLBACK hintCallback () {
	*(vu16*) VDP_CTRL_PORT = 0x8C00 | 0x81; // H40 mode
	//VDP_setScreenWidth320(); // H40 mode
	VDP_setPlaneSize(64, 32, FALSE);

	// Prepare DMA cmd and source address for first palette
    u32 palCmd = VDP_DMA_CRAM_ADDR((PAL0 * 16 + 1) * 2); // target starting color index multiplied by 2
    u32 fromAddrForDMA = (u32) (img_hud.palette->data + 1) >> 1; // TODO: this can be set outside the HInt

    //waitHCounter(136);

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
}

// 224 px display height, but we use only VERTICAL_TILES height.
// 64 display columns divided in 2 planes => 64/2=32
// If VERTICAL_TILES=28 => VERTICAL_TILES*32=896 pixels
// If VERTICAL_TILES=24 => VERTICAL_TILES*32=768 pixels
// Multiplied by 2 because is shared between the 2 planes BG_A and BG_B
static u16 frame_buffer[VERTICAL_TILES*32*2];
static u8* frame_buffer_ptr = (u8*)frame_buffer;

// For every display column we calculate the shift to access one of the 2 planes used in the framebuffer.
static u16 frame_buffer_xid[64];

void vintCallback () {
	// Reload the first 2 palettes that were overriden by the HUD palettes
	u32 palCmd = VDP_DMA_CRAM_ADDR((PAL0 * 16 + 0) * 2); // target starting color index multiplied by 2
    u32 fromAddrForDMA = (u32) (palette_grey + 0) >> 1; // TODO: this can be set outside the HInt
    setupDMAForPals(16, fromAddrForDMA);
    *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer

	palCmd = VDP_DMA_CRAM_ADDR((PAL1 * 16 + 0) * 2); // target starting color index multiplied by 2
    fromAddrForDMA = (u32) (palette_red + 0) >> 1; // TODO: this can be set outside the HInt
    setupDMAForPals(16, fromAddrForDMA);
    *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer

	*(vu16*) VDP_CTRL_PORT = 0x8C00 & ~0x81; // H32 mode
	//VDP_setScreenWidth256(); // H32 mode
	VDP_setPlaneSize(32, 32, FALSE);

	// clear the frame buffer
	__asm volatile (
		"    lea     %c[_TOTAL_BYTES](%%a0),%%a0\n"    // a0: buffer end
		// Save all registers (except scratch pad)
		//"    movem.l %%d2-%%d7/%%a2-%%a6,-(%%sp)\n"
		// Clear registers
		"    moveq   #0,%%d0\n"
		"    move.l  %%d0,%%d1\n"
		"    move.l  %%d0,%%d2\n"
		"    move.l  %%d0,%%d3\n"
		"    move.l  %%d0,%%d4\n"
		"    move.l  %%d0,%%d5\n"
		"    move.l  %%d0,%%d6\n"
		"    move.l  %%d0,%%d7\n"
		"    move.l  %%d0,%%a1\n"
		"    move.l  %%d0,%%a2\n"
		"    move.l  %%d0,%%a3\n"
		"    move.l  %%d0,%%a4\n"
		"    move.l  %%d0,%%a5\n"
		"    move.l  %%d0,%%a6\n"
		// Clear all the bytes by using 14 registers with long word access.
		"    .rept (%c[_TOTAL_BYTES] / (14*4))\n"
    	"    movem.l %%d0-%%d7/%%a1-%%a6,-(%%a0)\n"
    	"    .endr\n"
		// NOTE: if reminder from the division isn't 0 you need to add the missing operations.
		"    .rept ((%c[_TOTAL_BYTES] %% (14*4)) / 4)\n"
		"    move.l  %%d0,-(%%a0)\n"
		"    .endr\n"
		// Restore all saved registers
		//"    movem.l (%%sp)+,%%d2-%%d7/%%a2-%%a6\n"
		: "+a" (frame_buffer_ptr)
		: [_TOTAL_BYTES] "i" ((VERTICAL_TILES*32*2)*2)
		: 
	);
}

static void loadPlaneDisplacements () {
	for (u16 i = 0; i < 64; i++) {
		frame_buffer_xid[i] = i&1 ? VERTICAL_TILES*32 + i/2 : i/2;
	}
}

/// @brief Load render tiles in VRAM. 72 tiles in total.
static void loadRenderTiles () {
	// Create a tile
	u8* tile = MEM_alloc(32); // 32 bytes per tile, layout: tile[4][8]
	memset(tile, 0, 32);

	// 9 possible tile heights

	// Height 0 is the tile zero
	VDP_loadTileData((u32*)tile, 0, 1, CPU);

	// All remaining 8 possible tile heights (1 to 8, tile zero already loaded)
	for (u16 i = 1; i <= 8; i++) {
		memset(tile, 0, 32);
		for (u16 c = 0; c < 8; c++) { // 8 values to create the depth gradient
			for (u16 j = i-1; j < 8; j++) { // tile rows
				for (u16 b = 0; b < 2; b++) { // tile width (4 pixels = 2 bytes)
					tile[4*j + b] = (c+0) + ((c+1) << 4);
				}
			}
			VDP_loadTileData((u32*)tile, i + c*8, 1, CPU);
		}
	}

	MEM_free(tile);
}

static void loadHUD () {
	u16 baseTileAttrib = TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, 72);
	VDP_loadTileSet(img_hud.tileset, baseTileAttrib & TILE_INDEX_MASK, DMA);
	VDP_waitDMACompletion();

    VDP_setTileMapEx(BG_A, img_hud.tilemap, baseTileAttrib, 0, 24, 0, 0, 40, 4, DMA);
	VDP_waitDMACompletion();

	// Load a tilemap having the extended 64 tiles width already set
    // TileMap* tilemap = img_hud.tilemap;
	// u16 tilemapAddrInPlane = VDP_getPlaneAddress(BG_A, 0, 24);
	// const u16 numTiles = 64 * 4;
    // if (tilemap->compression != COMPRESSION_NONE) {
    //  TileMap *m = unpackTileMap(tilemap, NULL);
	// 	VDP_setTileMapData(tilemapAddrInPlane, m->tilemap, baseTileAttrib & TILE_INDEX_MASK, numTiles, 2, DMA);
	// 	VDP_waitDMACompletion();
    //  MEM_free(m);
    // }
    // else {
	// 	VDP_setTileMapData(tilemapAddrInPlane, FAR_SAFE(tilemap->tilemap, numTiles*8*2), baseTileAttrib & TILE_INDEX_MASK, numTiles, 2, DMA);
	// 	VDP_waitDMACompletion();
	// }
}

int main (bool hardReset)
{
	// On soft reset do like a hard reset
	if (!hardReset) {
		VDP_waitDMACompletion(); // avoids some glitches as per Genesis Manual's Addendum section
		SYS_hardReset();
	}

	VDP_setEnable(FALSE);

	loadPlaneDisplacements();

	loadRenderTiles();

	loadHUD();

	MEM_pack();

	SYS_disableInts();
	{
		SYS_setVBlankCallback(vintCallback);
		VDP_setHIntCounter(224-32-2); // scanline location for the HUD
    	SYS_setHIntCallback(hintCallback);
    	VDP_setHInterrupt(TRUE);
	}
	SYS_enableInts();

	// Setup VDP
	VDP_setScreenWidth256(); // H32 mode
	VDP_setPlaneSize(32, 32, TRUE);
	VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
	VDP_setHorizontalScroll(BG_A, 0);
	VDP_setHorizontalScroll(BG_B, 4); // offset second plane by 4 pixels
	VDP_setBackgroundColor(1);
	const u16 PAA = 57344; //VDP_getPlaneAddress(BG_A, 0, 0);
	const u16 PBA = 49152; //VDP_getPlaneAddress(BG_B, 0, 0);

	SYS_doVBlankProcess();

	VDP_setEnable(TRUE);

	u16 posX = 2 * FP, posY = 2 * FP;
	u16 angle = 0; // angle max value is 1023 and is updated in +/- 8 units

	for (;;)
	{
		// handle inputs
		u16 joy = JOY_readJoypad(JOY_1);
		if (joy) {

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
			u16 sideDistX_l0 = (posX - (posX/FP)*FP);
			u16 sideDistX_l1 = (((posX/FP) + 1)*FP - posX);
			u16 sideDistY_l0 = (posY - (posY/FP)*FP);
			u16 sideDistY_l1 = (((posY/FP) + 1)*FP - posY);

			u16 a = angle / (1024 / AP);
			#if USE_TAB_DELTAS_3
			const u16 *delta_a_ptr = tab_deltas + (a * (256-64)); // substract 64 because the table now has 1 column less
			#else
			const u16 *delta_a_ptr = tab_deltas + (a * 256);
			#endif
			#if USE_TAB_MULU_DIST_DIV256
			a *= DELTA_DIST_VALUES; // offset into tab_mulu_dist_div256[...][a]
			#endif

			// 256 pixels width but 4 "pixels" wide column => effectively 256/4=64 pixels width.
			for (u16 c = 0; c < 64; ++c) {
				#if USE_TAB_DELTAS_3
				const u16* delta_ptr = delta_a_ptr + c*3;
				const u16 deltaDistX = delta_ptr[0];
				const u16 deltaDistY = delta_ptr[1];
				const s16 rayDir = (s16)delta_ptr[2];
				#else
				const u16* delta_ptr = delta_a_ptr + c*4;
				const u16 deltaDistX = delta_ptr[0];
				const u16 deltaDistY = delta_ptr[1];
				const s16 rayDirX = (s16)delta_ptr[2];
				const s16 rayDirY = (s16)delta_ptr[3];
				#endif

				u16 sideDistX, sideDistY;
				s16 stepX, stepY, stepYMS;

			#if USE_TAB_DELTAS_3
				// rayDix < 0 and rayDirY < 0
				if (rayDir == 0) {
					stepX = -1;
					stepY = -1;
					stepYMS = -MAP_SIZE;
					#if USE_TAB_MULU_DIST_DIV256
					sideDistX = tab_mulu_dist_div256[sideDistX_l0][(a+c)];
					sideDistY = tab_mulu_dist_div256[sideDistY_l0][(a+c)];
					#else
					sideDistX = (u16)(mulu(sideDistX_l0, deltaDistX) >> FS);
					sideDistY = (u16)(mulu(sideDistY_l0, deltaDistY) >> FS);
					#endif
				}
				// rayDix < 0 and rayDirY > 0
				else if (rayDir == 1) {
					stepX = -1;
					stepY = 1;
					stepYMS = MAP_SIZE;
					#if USE_TAB_MULU_DIST_DIV256
					sideDistX = tab_mulu_dist_div256[sideDistX_l0][(a+c)];
					sideDistY = tab_mulu_dist_div256[sideDistY_l1][(a+c)];
					#else
					sideDistX = (u16)(mulu(sideDistX_l0, deltaDistX) >> FS);
					sideDistY = (u16)(mulu(sideDistY_l1, deltaDistY) >> FS);
					#endif
				}
				// rayDix > 0 and rayDirY < 0
				else if (rayDir == 2) {
					stepX = 1;
					stepY = -1;
					stepYMS = -MAP_SIZE;
					#if USE_TAB_MULU_DIST_DIV256
					sideDistX = tab_mulu_dist_div256[sideDistX_l1][(a+c)];
					sideDistY = tab_mulu_dist_div256[sideDistY_l0][(a+c)];
					#else
					sideDistX = (u16)(mulu(sideDistX_l1, deltaDistX) >> FS);
					sideDistY = (u16)(mulu(sideDistY_l0, deltaDistY) >> FS);
					#endif
				}
				// rayDix > 0 and rayDirY > 0
				else {
					stepX = 1;
					stepY = 1;
					stepYMS = MAP_SIZE;
					#if USE_TAB_MULU_DIST_DIV256
					sideDistX = tab_mulu_dist_div256[sideDistX_l1][(a+c)];
					sideDistY = tab_mulu_dist_div256[sideDistY_l1][(a+c)];
					#else
					sideDistX = (u16)(mulu(sideDistX_l1, deltaDistX) >> FS);
					sideDistY = (u16)(mulu(sideDistY_l1, deltaDistY) >> FS);
					#endif
				}
			#else
				if (rayDirX < 0) {
					stepX = -1;
					#if USE_TAB_MULU_DIST_DIV256
					sideDistX = tab_mulu_dist_div256[sideDistX_l0][(a+c)];
					#else
					sideDistX = (u16)(mulu(sideDistX_l0, deltaDistX) >> FS);
					#endif
				}
				else {
					stepX = 1;
					#if USE_TAB_MULU_DIST_DIV256
					sideDistX = tab_mulu_dist_div256[sideDistX_l1][(a+c)];
					#else
					sideDistX = (u16)(mulu(sideDistX_l1, deltaDistX) >> FS);
					#endif
				}

				if (rayDirY < 0) {
					stepY = -1;
					stepYMS = -MAP_SIZE;
					#if USE_TAB_MULU_DIST_DIV256
					sideDistY = tab_mulu_dist_div256[sideDistY_l0][(a+c)];
					#else
					sideDistY = (u16)(mulu(sideDistY_l0, deltaDistY) >> FS);
					#endif
				}
				else {
					stepY = 1;
					stepYMS = MAP_SIZE;
					#if USE_TAB_MULU_DIST_DIV256
					sideDistY = tab_mulu_dist_div256[sideDistY_l1][(a+c)];
					#else
					sideDistY = (u16)(mulu(sideDistY_l1, deltaDistY) >> FS);
					#endif
				}
			#endif

				u16 mapX = posX / FP;
				u16 mapY = posY / FP;
				const u8 *map_ptr = &map[mapY][mapX];

				for (u16 n = STEP_COUNT; n--;) {

					// side X
					if (sideDistX < sideDistY) {

						mapX += stepX;
						map_ptr += stepX;

						u16 hit = *map_ptr; // map[mapY][mapX];
						if (hit) {

							u16 t = tab_wall_div[sideDistX];
							u16 d8 = tab_color_d8_x[sideDistX]; // it was: (7 - min(7, sideDistX / FP))*8 + 1;

						#if SHOW_TEXCOORD
							u16 wallY = posY + (muls(sideDistX, rayDirY) >> FS);
							//wallY = ((wallY * 8) >> FS) & 7; // faster
							wallY = max((wallY - mapY*FP) * 8 / FP, 0); // cleaner
							// if tab_color_d8_x[] optimization is removed then use min(d8, wallY)*8 at the end
							u16 color = ((0 + 2*(mapY&1)) << TILE_ATTR_PALETTE_SFT) + 1 + min(d8-1, wallY*8);
						#else
							u16 color = d8; // PAL0 by default
							if (mapY&1)
								color |= (PAL2 << TILE_ATTR_PALETTE_SFT);
						#endif

							u16 *idata = frame_buffer + frame_buffer_xid[c];
							write_vline(idata, t, color);
							break;
						}

						sideDistX += deltaDistX;
					}
					// side Y
					else {

						mapY += stepY;
						map_ptr += stepYMS;

						u16 hit = *map_ptr; // map[mapY][mapX];
						if (hit) {

							u16 t = tab_wall_div[sideDistY];
							u16 d8 = tab_color_d8_y[sideDistY]; // it was: (7 - min(7, sideDistY / FP))*8 + 1;

						#if SHOW_TEXCOORD
							u16 wallX = posX + (muls(sideDistY, rayDirX) >> FS);
							//wallX = ((wallX * 8) >> FS) & 7; // faster
							wallX = max((wallX - mapX*FP) * 8 / FP, 0); // cleaner
							// if tab_color_d8_y[] optimization is removed then use min(d8, wallX)*8 at the end
							u16 color = ((1 + 2*(mapX&1)) << TILE_ATTR_PALETTE_SFT) + 1 + min(d8-1, wallX*8);
						#else
							u16 color = d8 |= (PAL1 << TILE_ATTR_PALETTE_SFT); // PAL1 by default
							if (mapX&1)
								color |= (PAL3 << TILE_ATTR_PALETTE_SFT);
						#endif

							u16 *idata = frame_buffer + frame_buffer_xid[c];
							write_vline(idata, t, color);
							break;
						}

						sideDistY += deltaDistY;
					}
				}
			}
		}

		// end of the frame and dma copy
		DMA_queueDmaFast(DMA_VRAM, frame_buffer, PAA, VERTICAL_TILES*32, 2);
		DMA_queueDmaFast(DMA_VRAM, frame_buffer + VERTICAL_TILES*32, PBA, VERTICAL_TILES*32, 2);

		SYS_doVBlankProcess();

		//VDP_showFPS(TRUE);
		VDP_showCPULoad();
	}

	SYS_disableInts();
	{
		SYS_setVBlankCallback(NULL);
		VDP_setHInterrupt(FALSE);
    	SYS_setHIntCallback(NULL);
	}
	SYS_enableInts();

	VDP_setBackgroundColor(0);
	VDP_clearPlane(BG_B, TRUE);
	VDP_clearPlane(BG_A, TRUE);
	VDP_drawText("Game Over", (256/8 - 10)/2, (224/8)/2);

	return 0;
}