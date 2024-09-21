#include "write_vline.h"
#include <vdp_tile.h>
#include "utils.h"
#include "consts.h"

FORCE_INLINE void write_vline (u16 *tilemap, u16 h2, u16 color)
{
	// Tilemap width in tiles.

	// draw a solid vertical line
	if (h2 == 0) {
		// C version
		//for (u16 y = 0; y < VERTICAL_COLUMNS*PLANE_COLUMNS; y+=PLANE_COLUMNS) {
		// 	tilemap[y] = color;
		//}

		// inline ASM version
		__asm volatile (
			".set off,0\n"
			".rept %c[_VERTICAL_COLUMNS]\n"
			"    move.w  %[_color],off(%[_tilemap])\n"
			"    .set off,off+%c[_PLANE_COLUMNS]*2\n"
			".endr\n"
			: [_tilemap] "+a" (tilemap), [_color] "+d" (color)
			: [_VERTICAL_COLUMNS] "i" (VERTICAL_COLUMNS), [_PLANE_COLUMNS] "i" (PLANE_COLUMNS)
			:
		);
	}
	else {
		//u16 ta = (h2 / 8); // vertical tilemap entry position
		// top tilemap entry
		//tilemap[ta*PLANE_COLUMNS] = color + (h2 & 7); // offsets the color by the halved pixel height modulo 8
		// bottom tilemap entry (with flipped attribute)
		//tilemap[((VERTICAL_COLUMNS-1)-ta)*PLANE_COLUMNS] = (color + (h2 & 7)) | TILE_ATTR_VFLIP_MASK;

		// THIS is slightly faster:
		// top tilemap entry
		//tilemap[(h2 & ~(8-1)) << (LOG2(PLANE_COLUMNS) - LOG2(8))] = color + (h2 & 7); // offsets the color by the halved pixel height modulo 8
		// bottom tilemap entry (with flipped attribute)
		//tilemap[(VERTICAL_COLUMNS-1)*PLANE_COLUMNS - ((h2 & ~(8-1)) << (LOG2(PLANE_COLUMNS) - LOG2(8)))] = (color + (h2 & 7)) | TILE_ATTR_VFLIP_MASK;

		// VERTICAL_COLUMNS-2 remaining vertical tilemap entries

		// C version (for 28 vertical tiles)
		/*switch (ta) {
			case 0:		tilemap[1*PLANE_COLUMNS] = color;
						tilemap[26*PLANE_COLUMNS] = color; // fallthru
			case 1:		tilemap[2*PLANE_COLUMNS] = color;
						tilemap[25*PLANE_COLUMNS] = color; // fallthru
			case 2:		tilemap[3*PLANE_COLUMNS] = color;
						tilemap[24*PLANE_COLUMNS] = color; // fallthru
			case 3:		tilemap[4*PLANE_COLUMNS] = color;
						tilemap[23*PLANE_COLUMNS] = color; // fallthru
			case 4:		tilemap[5*PLANE_COLUMNS] = color;
						tilemap[22*PLANE_COLUMNS] = color; // fallthru
			case 5:		tilemap[6*PLANE_COLUMNS] = color;
						tilemap[21*PLANE_COLUMNS] = color; // fallthru
			case 6:		tilemap[7*PLANE_COLUMNS] = color;
						tilemap[20*PLANE_COLUMNS] = color; // fallthru
			case 7:		tilemap[8*PLANE_COLUMNS] = color;
						tilemap[19*PLANE_COLUMNS] = color; // fallthru
			case 8:		tilemap[9*PLANE_COLUMNS] = color;
						tilemap[18*PLANE_COLUMNS] = color; // fallthru
			case 9:		tilemap[10*PLANE_COLUMNS] = color;
						tilemap[17*PLANE_COLUMNS] = color; // fallthru
			case 10:	tilemap[11*PLANE_COLUMNS] = color;
						tilemap[16*PLANE_COLUMNS] = color; // fallthru
			case 11:	tilemap[12*PLANE_COLUMNS] = color;
						tilemap[15*PLANE_COLUMNS] = color; // fallthru
			case 12:	tilemap[13*PLANE_COLUMNS] = color;
						tilemap[14*PLANE_COLUMNS] = color; // fallthru
						break;
		}*/

		// inline ASM version
		u16 h2_aux = h2, color_aux = color;
		__asm volatile (
			"    andi.w  #7,%[_h2_aux]\n" // _h2_aux = (h2 & 7)
			"    add.w   %[_h2_aux],%[_color_aux]\n" // _color_aux = color + (h2 & 7);

			// offset h2 comes already multiplied by 8, but we need to clear the first 3 bits so 
			// we can use it as a multiple of the jump block size (8 bytes)
			"    andi.w  %[_clearBitsOffset],%[_h2]\n" // (h2 & ~(8-1))

			// top tilemap entry
			"    move.w  %[_h2],%[_h2_aux]\n" // _h2_aux = (h2 & ~(8-1))
			"    lsl.w   %[_SHIFT_TOP_COLUMN_BYTES],%[_h2_aux]\n" // _h2_aux = (h2 & ~(8-1)) << (LOG2(PLANE_COLUMNS) - LOG2(8))
			"    move.w  %[_color_aux],(%[_tilemap],%[_h2_aux])\n"

			// bottom tilemap entry
			"    or.w    %[_TILE_ATTR_VFLIP_MASK],%[_color_aux]\n" // _color_aux = (color + (h2 & 7)) | TILE_ATTR_VFLIP_MASK;
			"    neg.w   %[_h2_aux]\n"
			"    addi.w  %[_BOTTOM_COLUMN_BYTES],%[_h2_aux]\n" // _h2_aux = (VERTICAL_COLUMNS-1)*PLANE_COLUMNS - ((h2 & ~(8-1)) << (LOG2(PLANE_COLUMNS) - LOG2(8)))
			"    move.w  %[_color_aux],(%[_tilemap],%[_h2_aux])\n"

			// jump into table using with _h2
			"    jmp     .wvl_table_%=(%%pc,%[_h2].w)\n"
			// color assignment table: 
			//  from tile[1*PLANE_COLUMNS] up to [((VERTICAL_COLUMNS-2)/2)*PLANE_COLUMNS]
			//  from tile[(VERTICAL_COLUMNS-2)*PLANE_COLUMNS] down to [(((VERTICAL_COLUMNS-2)/2)+1)*PLANE_COLUMNS]
			// Eg for VERTICAL_COLUMNS=28: 
			//  from tile[1*PLANE_COLUMNS] up to [13*PLANE_COLUMNS] and from tile[26*PLANE_COLUMNS] down to [14*PLANE_COLUMNS]
			// Eg for VERTICAL_COLUMNS=24: 
			//  from tile[1*PLANE_COLUMNS] up to [11*PLANE_COLUMNS] and from tile[22*PLANE_COLUMNS] down to [12*PLANE_COLUMNS]
			".wvl_table_%=:\n"
			".set offup,1 * %c[_PLANE_COLUMNS] * 2\n"
			".set offdown,(%c[_VERTICAL_COLUMNS] - 2) * %c[_PLANE_COLUMNS]*2\n"
			".rept (%c[_VERTICAL_COLUMNS] - 2) / 2\n"
			"    move.w  %[_color],offup(%[_tilemap])\n"
			"    move.w  %[_color],offdown(%[_tilemap])\n"
			"    .set offup,offup+%c[_PLANE_COLUMNS]*2\n"
			"    .set offdown,offdown-%c[_PLANE_COLUMNS]*2\n"
			".endr\n"
			: [_tilemap] "+a" (tilemap), [_h2] "+d" (h2), [_color] "+d" (color), [_h2_aux] "+d" (h2_aux), [_color_aux] "+d" (color_aux)
			: [_clearBitsOffset] "i" (~(8-1)), [_VERTICAL_COLUMNS] "i" (VERTICAL_COLUMNS), [_PLANE_COLUMNS] "i" (PLANE_COLUMNS),
			[_TILE_ATTR_VFLIP_MASK] "i" (TILE_ATTR_VFLIP_MASK), [_SHIFT_TOP_COLUMN_BYTES] "i" (LOG2(PLANE_COLUMNS) - LOG2(8) + 1),
			[_BOTTOM_COLUMN_BYTES] "i" ((VERTICAL_COLUMNS-1)*PLANE_COLUMNS*2)
			:
		);
	}
}