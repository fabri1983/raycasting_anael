#ifndef _WRITE_VLINE_H_
#define _WRITE_VLINE_H_

#include <types.h>
#include <vdp_tile.h>
#include "utils.h"
#include "consts.h"

FORCE_INLINE void write_vline (u16 *tilemap, u16 h2, u16 color)
{
	// Tilemap width in tiles.

	// draw a solid vertical line
	/*if (h2 == 0) {
		// C version
		//for (u16 y = 0; y < VERTICAL_TILES*TILEMAP_COLUMNS; y+=TILEMAP_COLUMNS) {
		// 	tilemap[y] = color;
		//}

		// inline ASM version
		__asm volatile (
			".set off,0\n"
			".rept %c[_VERTICAL_TILES]\n"
			"    move.w  %[_color],off(%[_tilemap])\n"
			"    .set off,off+%c[_TILEMAP_COLUMNS]*2\n"
			".endr\n"
			: [_tilemap] "+a" (tilemap)
			: [_color] "d" (color), [_VERTICAL_TILES] "i" (VERTICAL_TILES), [_TILEMAP_COLUMNS] "i" (TILEMAP_COLUMNS)
			:
		);
		return;
	}*/

	u16 ta = (h2 / 8); // vertical tilemap entry position

	// top tilemap entry
	tilemap[ta*TILEMAP_COLUMNS] = color + (h2 & 7); // offsets the color by the halved pixel height modulo 8
	// bottom tilemap entry (with flipped attribute)
	tilemap[((VERTICAL_TILES-1)-ta)*TILEMAP_COLUMNS] = (color + (h2 & 7)) | (1 << TILE_ATTR_VFLIP_SFT);

	// VERTICAL_TILES-2 remaining vertical tilemap entries

	// C version (for 28 vertical tiles and 32 tilemap columns)
	/*switch (ta) {
		case 0:		tilemap[1*TILEMAP_COLUMNS] = color;
					tilemap[26*32] = color; // fallthru
		case 1:		tilemap[2*32] = color;
					tilemap[25*32] = color; // fallthru
		case 2:		tilemap[3*32] = color;
					tilemap[24*32] = color; // fallthru
		case 3:		tilemap[4*32] = color;
					tilemap[23*32] = color; // fallthru
		case 4:		tilemap[5*32] = color;
					tilemap[22*32] = color; // fallthru
		case 5:		tilemap[6*32] = color;
					tilemap[21*32] = color; // fallthru
		case 6:		tilemap[7*32] = color;
					tilemap[20*32] = color; // fallthru
		case 7:		tilemap[8*32] = color;
					tilemap[19*32] = color; // fallthru
		case 8:		tilemap[9*32] = color;
					tilemap[18*32] = color; // fallthru
		case 9:		tilemap[10*32] = color;
					tilemap[17*32] = color; // fallthru
		case 10:	tilemap[11*32] = color;
					tilemap[16*32] = color; // fallthru
		case 11:	tilemap[12*32] = color;
					tilemap[15*32] = color; // fallthru
		case 12:	tilemap[13*32] = color;
					tilemap[14*32] = color; // fallthru
					break;
	}*/

	// inline ASM version
	__asm volatile (
		// offset comes already multiplied by 8 but we need its first 3 bits to be cleared
		"    andi.b  %[_clearBitsOffset],%[_offset]\n"
		// jump into table using with _offset
		"    jmp     .wvl_table_%=(%%pc,%[_offset].w)\n"
		// color assignment table from tile[1*TILEMAP_COLUMNS] up to [((VERTICAL_TILES-2)/2)*32] and from tile[(VERTICAL_TILES-2)*32] down to [(((VERTICAL_TILES-2)/2)+1)*32]
		// Eg for VERTICAL_TILES=28: 
		//  from tile[1*32] up to [13*32] and from tile[26*32] down to [14*32]
		// Eg for VERTICAL_TILES=24: 
		//  from tile[1*32] up to [11*32] and from tile[22*32] down to [12*32]
		".wvl_table_%=:\n"
		".set offup,1 * %c[_TILEMAP_COLUMNS] * 2\n"
		".set offdown,(%c[_VERTICAL_TILES] - 2) * %c[_TILEMAP_COLUMNS]*2\n"
		".rept (%c[_VERTICAL_TILES] - 2) / 2\n"
		"    move.w  %[_color],offup(%[_tilemap])\n"
		"    move.w  %[_color],offdown(%[_tilemap])\n"
		"    .set offup,offup+%c[_TILEMAP_COLUMNS]*2\n"
		"    .set offdown,offdown-%c[_TILEMAP_COLUMNS]*2\n"
		".endr\n"
		: [_tilemap] "+a" (tilemap)
		: [_color] "d" (color), [_offset] "d" (h2), [_clearBitsOffset] "i" (~(8-1)), 
		[_VERTICAL_TILES] "i" (VERTICAL_TILES), [_TILEMAP_COLUMNS] "i" (TILEMAP_COLUMNS)
		:
	);
}

#endif // _WRITE_VLINE_H_