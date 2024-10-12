#include "write_vline.h"
#include <vdp_tile.h>
#include "utils.h"
#include "consts.h"

u16* column_ptr = NULL;

FORCE_INLINE void write_vline (u16 h2, u16 tileAttrib)
{
	// Tilemap width in tiles.

	// draw a solid vertical line
	if (h2 == 0) {
		// C version
		//for (u16 y = 0; y < VERTICAL_ROWS*PLANE_COLUMNS; y+=PLANE_COLUMNS) {
		// 	column_ptr[y] = tileAttrib;
		//}

		// inline ASM version
		__asm volatile (
			".set off,0\n"
			".rept %c[_VERTICAL_COLUMNS]\n"
			"    move.w  %[tileAttrib],off(%[tilemap])\n"
			"    .set off,off+%c[_PLANE_COLUMNS]*2\n"
			".endr\n"
			: [tileAttrib] "+d" (tileAttrib)
			: [tilemap] "a" (column_ptr), 
              [_VERTICAL_COLUMNS] "i" (VERTICAL_ROWS), [_PLANE_COLUMNS] "i" (PLANE_COLUMNS)
			: "cc"
		);
	}
	else {
		//u16 ta = (h2 / 8); // vertical tilemap entry position
		// top tilemap entry
		//tilemap[ta*PLANE_COLUMNS] = tileAttrib + (h2 & 7); // offsets the tileAttrib by the halved pixel height modulo 8
		// bottom tilemap entry (with flipped attribute)
		//tilemap[((VERTICAL_ROWS-1)-ta)*PLANE_COLUMNS] = (tileAttrib + (h2 & 7)) | TILE_ATTR_VFLIP_MASK;

		// THIS is slightly faster in C, but super fast when implemented in the inline ASM block:
		// top tilemap entry
		//tilemap[(h2 & ~(8-1)) << (LOG2(PLANE_COLUMNS) - LOG2(8))] = tileAttrib + (h2 & 7); // offsets the tileAttrib by the halved pixel height modulo 8
		// bottom tilemap entry (with flipped attribute)
		//tilemap[(VERTICAL_ROWS-1)*PLANE_COLUMNS - ((h2 & ~(8-1)) << (LOG2(PLANE_COLUMNS) - LOG2(8)))] = (tileAttrib + (h2 & 7)) | TILE_ATTR_VFLIP_MASK;

		// VERTICAL_ROWS-2 remaining vertical tilemap entries

		// C version (for 28 vertical tiles)
		/*switch (ta) {
			case 0:		tilemap[1*PLANE_COLUMNS] = tileAttrib;
						tilemap[26*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 1:		tilemap[2*PLANE_COLUMNS] = tileAttrib;
						tilemap[25*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 2:		tilemap[3*PLANE_COLUMNS] = tileAttrib;
						tilemap[24*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 3:		tilemap[4*PLANE_COLUMNS] = tileAttrib;
						tilemap[23*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 4:		tilemap[5*PLANE_COLUMNS] = tileAttrib;
						tilemap[22*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 5:		tilemap[6*PLANE_COLUMNS] = tileAttrib;
						tilemap[21*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 6:		tilemap[7*PLANE_COLUMNS] = tileAttrib;
						tilemap[20*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 7:		tilemap[8*PLANE_COLUMNS] = tileAttrib;
						tilemap[19*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 8:		tilemap[9*PLANE_COLUMNS] = tileAttrib;
						tilemap[18*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 9:		tilemap[10*PLANE_COLUMNS] = tileAttrib;
						tilemap[17*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 10:	tilemap[11*PLANE_COLUMNS] = tileAttrib;
						tilemap[16*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 11:	tilemap[12*PLANE_COLUMNS] = tileAttrib;
						tilemap[15*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 12:	tilemap[13*PLANE_COLUMNS] = tileAttrib;
						tilemap[14*PLANE_COLUMNS] = tileAttrib; // fallthru
						break;
		}*/

		// inline ASM version
		u16 h2_aux = h2;
		__asm volatile (
			// offset h2 comes already multiplied by 8, but we need to clear the first 3 bits so 
			// we can use it as a multiple of the jump block size (8 bytes)
			"    andi.w  %[CLEAR_BITS_OFFSET],%[h2]\n" // (h2 & ~(8-1))
			// jump into table using with h2
			"    jmp     .wvl_table_%=(%%pc,%[h2].w)\n"
			// tileAttrib assignment table: 
			//  from tile[1*PLANE_COLUMNS] up to [((VERTICAL_ROWS-2)/2)*PLANE_COLUMNS]
			//  from tile[(VERTICAL_ROWS-2)*PLANE_COLUMNS] down to [(((VERTICAL_ROWS-2)/2)+1)*PLANE_COLUMNS]
			// Eg for VERTICAL_ROWS=28: 
			//  from tile[1*PLANE_COLUMNS] up to [13*PLANE_COLUMNS] and from tile[26*PLANE_COLUMNS] down to [14*PLANE_COLUMNS]
			// Eg for VERTICAL_ROWS=24: 
			//  from tile[1*PLANE_COLUMNS] up to [11*PLANE_COLUMNS] and from tile[22*PLANE_COLUMNS] down to [12*PLANE_COLUMNS]
			".wvl_table_%=:\n"
			".set offup,1 * %c[_PLANE_COLUMNS] * 2\n"
			".set offdown,(%c[_VERTICAL_COLUMNS] - 2) * %c[_PLANE_COLUMNS]*2\n"
			".rept (%c[_VERTICAL_COLUMNS] - 2) / 2\n"
			"    move.w  %[tileAttrib],offup(%[tilemap])\n"
			"    move.w  %[tileAttrib],offdown(%[tilemap])\n"
			"    .set offup,offup+%c[_PLANE_COLUMNS]*2\n"
			"    .set offdown,offdown-%c[_PLANE_COLUMNS]*2\n"
			".endr\n"

            // setup for top and bottom tilemap entries
			"    andi.w  #7,%[h2_aux]\n" // h2_aux = (h2 & 7)
			"    add.w   %[h2_aux],%[tileAttrib]\n" // tileAttrib += (h2 & 7);

			// top tilemap entry
			"    lsl.w   %[_SHIFT_TOP_COLUMN_BYTES],%[h2]\n" // h2 = (h2 & ~(8-1)) << (LOG2(PLANE_COLUMNS) - LOG2(8))
			"    move.w  %[tileAttrib],(%[tilemap],%[h2])\n"

			// bottom tilemap entry
			"    or.w    %[_TILE_ATTR_VFLIP_MASK],%[tileAttrib]\n" // tileAttrib_aux = (tileAttrib + (h2 & 7)) | TILE_ATTR_VFLIP_MASK;
			"    neg.w   %[h2]\n"
			"    addi.w  %[_BOTTOM_COLUMN_BYTES],%[h2]\n" // h2 = (VERTICAL_ROWS-1)*PLANE_COLUMNS - ((h2 & ~(8-1)) << (LOG2(PLANE_COLUMNS) - LOG2(8)))
			"    move.w  %[tileAttrib],(%[tilemap],%[h2])\n"

			: [h2] "+d" (h2), [tileAttrib] "+d" (tileAttrib), [h2_aux] "+d" (h2_aux)
			: [tilemap] "a" (column_ptr), [CLEAR_BITS_OFFSET] "i" (~(8-1)), [_VERTICAL_COLUMNS] "i" (VERTICAL_ROWS), [_PLANE_COLUMNS] "i" (PLANE_COLUMNS),
			  [_TILE_ATTR_VFLIP_MASK] "i" (TILE_ATTR_VFLIP_MASK), [_SHIFT_TOP_COLUMN_BYTES] "i" (LOG2(PLANE_COLUMNS) - LOG2(8) + 1), /* +1 due to division by 2 */
			  [_BOTTOM_COLUMN_BYTES] "i" ((VERTICAL_ROWS-1)*PLANE_COLUMNS*2)
			: "cc"
		);
	}
}