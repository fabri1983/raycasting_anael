#include "write_vline.h"
#include <vdp_tile.h>
#include "utils.h"
#include "consts.h"

// Points to the start of the column in first row for each cycle of the for-loop of columns
u16* column_ptr = NULL;

FORCE_INLINE void write_vline (u16 h2, u16 tileAttrib)
{
	// Tilemap width in tiles.

	// Draw a solid vertical line
	if (h2 == 0) {
		// C version.
		//for (u16 y = 0; y < VERTICAL_ROWS*PLANE_COLUMNS; y+=PLANE_COLUMNS) {
		// 	column_ptr[y] = tileAttrib;
		//}

		// Inline ASM version.
		__asm volatile (
			".set off,0\n"
			".rept %c[_VERTICAL_ROWS]\n"
			"    move.w  %[tileAttrib],off(%[tilemap])\n"
			"    .set off,off+%c[_PLANE_COLUMNS]*2\n"
			".endr\n"
			: [tileAttrib] "+d" (tileAttrib)
			: [tilemap] "a" (column_ptr), 
              [_VERTICAL_ROWS] "i" (VERTICAL_ROWS), [_PLANE_COLUMNS] "i" (PLANE_COLUMNS)
			: "cc"
		);
	}
	else {
        // TOP AND BOTTOM TILEMAP ENTRIES
        
		//u16 ta = (h2 / 8); // vertical tilemap entry position
		// top tilemap entry
		//column_ptr[ta*PLANE_COLUMNS] = tileAttrib + (h2 & 7); // offsets the tileAttrib by the halved pixel height modulo 8
		// bottom tilemap entry (with flipped attribute)
		//column_ptr[((VERTICAL_ROWS-1)-ta)*PLANE_COLUMNS] = (tileAttrib + (h2 & 7)) | TILE_ATTR_VFLIP_MASK;

		// THIS is slightly faster in C (but is super fast when implemented in the inline ASM block, as done below)
		// top tilemap entry
		//column_ptr[(h2 & ~(8-1)) << (LOG2(PLANE_COLUMNS) - LOG2(8))] = tileAttrib + (h2 & 7); // offsets the tileAttrib by the halved pixel height modulo 8
		// bottom tilemap entry (with flipped attribute)
		//column_ptr[(VERTICAL_ROWS-1)*PLANE_COLUMNS - ((h2 & ~(8-1)) << (LOG2(PLANE_COLUMNS) - LOG2(8)))] = (tileAttrib + (h2 & 7)) | TILE_ATTR_VFLIP_MASK;

		// Remaining VERTICAL_ROWS-2 tilemap entries

        // HERE WE TRY TO CLEAR THE TILEMAP ENTRIES FOR THE CURRENT COLUMN. THIS IS SLOWER THAN USING clear_buffer().
        // C version (for VERTICAL_ROWS = 24).
        // Set tileAttribTile0 which points to tile 0.
        /*u16 tileAttribTile0 = tileAttrib & ~TILE_INDEX_MASK;
        switch (ta) {
            case 11:	column_ptr[(11-1)*PLANE_COLUMNS] = tileAttribTile0;
						column_ptr[(12+1)*PLANE_COLUMNS] = tileAttribTile0; // fallthru
			case 10:	column_ptr[(10-1)*PLANE_COLUMNS] = tileAttribTile0;
						column_ptr[(13+1)*PLANE_COLUMNS] = tileAttribTile0; // fallthru
			case 9:		column_ptr[(9-1)*PLANE_COLUMNS] = tileAttribTile0;
						column_ptr[(14+1)*PLANE_COLUMNS] = tileAttribTile0; // fallthru
			case 8:		column_ptr[(8-1)*PLANE_COLUMNS] = tileAttribTile0;
						column_ptr[(15+1)*PLANE_COLUMNS] = tileAttribTile0; // fallthru
			case 7:		column_ptr[(7-1)*PLANE_COLUMNS] = tileAttribTile0;
						column_ptr[(16+1)*PLANE_COLUMNS] = tileAttribTile0; // fallthru
			case 6:		column_ptr[(6-1)*PLANE_COLUMNS] = tileAttribTile0;
						column_ptr[(17+1)*PLANE_COLUMNS] = tileAttribTile0; // fallthru
			case 5:		column_ptr[(5-1)*PLANE_COLUMNS] = tileAttribTile0;
						column_ptr[(18+1)*PLANE_COLUMNS] = tileAttribTile0; // fallthru
			case 4:		column_ptr[(4-1)*PLANE_COLUMNS] = tileAttribTile0;
						column_ptr[(19+1)*PLANE_COLUMNS] = tileAttribTile0; // fallthru
			case 3:		column_ptr[(3-1)*PLANE_COLUMNS] = tileAttribTile0;
						column_ptr[(20+1)*PLANE_COLUMNS] = tileAttribTile0; // fallthru
			case 2:		column_ptr[(2-1)*PLANE_COLUMNS] = tileAttribTile0;
						column_ptr[(21+1)*PLANE_COLUMNS] = tileAttribTile0; // fallthru
            case 1:		column_ptr[(1-1)*PLANE_COLUMNS] = tileAttribTile0;
						column_ptr[(22+1)*PLANE_COLUMNS] = tileAttribTile0; // fallthru
            case 0:     break;
		}*/

        // HERE WE TRY TO CLEAR THE TILEMAP ENTRIES FOR THE CURRENT COLUMN. THIS IS SLOWER THAN USING clear_buffer().
        // Inline ASM version.
        // This block sets tileAttribTile0 which points to tile 0.
        /*u16 jump = ((VERTICAL_ROWS-2)/2) * 8; // (multiplied by jump block size)
        u16 h2_aux1 = h2;
        u16 tileAttribTile0 = tileAttrib & ~TILE_INDEX_MASK;
        __asm volatile (
            // Offset h2 comes already multiplied by 8, great, but we need to clear the first 3 bits so 
			// we can use it as a multiple of the jump block size (8 bytes)
			"    andi.w  %[CLEAR_BITS_OFFSET],%[h2]\n" // (h2 & ~(8-1))
            // To calculate the jump we take the complement to (VERTICAL_ROWS-2)/2
            "    sub.w   %[h2],%[jump]\n" // jump = ((VERTICAL_ROWS-2)/2)*8 - h2
            // Jump into table using h2
			"    jmp     .wvl_clear_table_%=(%%pc,%[jump].w)\n"
            // Assignment table:
            //  from [((VERTICAL_ROWS-2)/2 - 1) * PLANE_COLUMNS] down to [1*PLANE_COLUMNS]
			//  from [((VERTICAL_ROWS-2)/2 + 1 + 1) * PLANE_COLUMNS] up to [(VERTICAL_ROWS-2)*PLANE_COLUMNS]
            ".wvl_clear_table_%=:\n"
            ".set offdown, (((%c[_VERTICAL_ROWS] - 2) / 2) - 1) * %c[_PLANE_COLUMNS] * 2\n" // *2 for byte convertion
			".set offup, (((%c[_VERTICAL_ROWS] - 2) / 2) + 1 + 1) * %c[_PLANE_COLUMNS] * 2\n" // *2 for byte convertion
			".rept ((%c[_VERTICAL_ROWS] - 2) / 2)\n"
			"    move.w  %[tileAttribTile0],offdown(%[tilemap])\n"
			"    move.w  %[tileAttribTile0],offup(%[tilemap])\n"
			"    .set offdown, offdown - %c[_PLANE_COLUMNS] * 2\n" // *2 for byte convertion
			"    .set offup, offup + %c[_PLANE_COLUMNS] * 2\n" // *2 for byte convertion
			".endr\n"
            // Case 0 (the complement to (VERTICAL_ROWS-2)/2) falls here
            : [h2] "+d" (h2_aux1), [tileAttribTile0] "+d" (tileAttribTile0), [jump] "+d" (jump)
			: [tilemap] "a" (column_ptr), [CLEAR_BITS_OFFSET] "i" (~(8-1)), 
              [_VERTICAL_ROWS] "i" (VERTICAL_ROWS), [_PLANE_COLUMNS] "i" (PLANE_COLUMNS)
			: "cc"
		);*/

        // C version (for VERTICAL_ROWS = 28 which was the original implementation).
        // Set tileAttrib which points to a colored tile.
		/*switch (ta) {
			case 0:		column_ptr[1*PLANE_COLUMNS] = tileAttrib;
						column_ptr[26*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 1:		column_ptr[2*PLANE_COLUMNS] = tileAttrib;
						column_ptr[25*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 2:		column_ptr[3*PLANE_COLUMNS] = tileAttrib;
						column_ptr[24*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 3:		column_ptr[4*PLANE_COLUMNS] = tileAttrib;
						column_ptr[23*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 4:		column_ptr[5*PLANE_COLUMNS] = tileAttrib;
						column_ptr[22*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 5:		column_ptr[6*PLANE_COLUMNS] = tileAttrib;
						column_ptr[21*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 6:		column_ptr[7*PLANE_COLUMNS] = tileAttrib;
						column_ptr[20*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 7:		column_ptr[8*PLANE_COLUMNS] = tileAttrib;
						column_ptr[19*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 8:		column_ptr[9*PLANE_COLUMNS] = tileAttrib;
						column_ptr[18*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 9:		column_ptr[10*PLANE_COLUMNS] = tileAttrib;
						column_ptr[17*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 10:	column_ptr[11*PLANE_COLUMNS] = tileAttrib;
						column_ptr[16*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 11:	column_ptr[12*PLANE_COLUMNS] = tileAttrib;
						column_ptr[15*PLANE_COLUMNS] = tileAttrib; // fallthru
			case 12:	column_ptr[13*PLANE_COLUMNS] = tileAttrib;
						column_ptr[14*PLANE_COLUMNS] = tileAttrib; // fallthru
						break;
		}*/

		// Inline ASM version.
        // This block sets tileAttrib which points to a colored tile.
        // This block sets top and bottom tilemap entries.
        u16 h2_aux2 = h2;
		__asm volatile (
			// Offset h2 comes already multiplied by 8, great, but we need to clear the first 3 bits so 
			// we can use it as a multiple of the jump block size (8 bytes)
			"    andi.w  %[CLEAR_BITS_OFFSET],%[h2]\n" // (h2 & ~(8-1))
			// Jump into table using h2
			"    jmp     .wvl_table_%=(%%pc,%[h2].w)\n"
			// Assignment table:
			//  from [1*PLANE_COLUMNS] up to [((VERTICAL_ROWS-2)/2)*PLANE_COLUMNS]
			//  from [(VERTICAL_ROWS-2)*PLANE_COLUMNS] down to [(((VERTICAL_ROWS-2)/2)+1)*PLANE_COLUMNS]
			// Eg for VERTICAL_ROWS=28: 
			//  from [1*PLANE_COLUMNS] up to [13*PLANE_COLUMNS] and from [26*PLANE_COLUMNS] down to [14*PLANE_COLUMNS]
			// Eg for VERTICAL_ROWS=24: 
			//  from [1*PLANE_COLUMNS] up to [11*PLANE_COLUMNS] and from [22*PLANE_COLUMNS] down to [12*PLANE_COLUMNS]
			".wvl_table_%=:\n"
			".set offup, 1 * %c[_PLANE_COLUMNS] * 2\n" // *2 for byte convertion
			".set offdown, (%c[_VERTICAL_ROWS] - 2) * %c[_PLANE_COLUMNS] * 2\n" // *2 for byte convertion
			".rept (%c[_VERTICAL_ROWS] - 2) / 2\n"
			"    move.w  %[tileAttrib],offup(%[tilemap])\n"
			"    move.w  %[tileAttrib],offdown(%[tilemap])\n"
			"    .set offup, offup + %c[_PLANE_COLUMNS] * 2\n" // *2 for byte convertion
			"    .set offdown, offdown - %c[_PLANE_COLUMNS] * 2\n" // *2 for byte convertion
			".endr\n"

            // Setup for top and bottom tilemap entries
			"    andi.w  #7,%[h2_aux]\n" // h2_aux = (h2 & 7)
			"    add.w   %[h2_aux],%[tileAttrib]\n" // tileAttrib += (h2 & 7);

			// Top tilemap entry
			"    lsl.w   %[_SHIFT_TOP_COLUMN_BYTES],%[h2]\n" // h2 = (h2 & ~(8-1)) << (LOG2(PLANE_COLUMNS) - LOG2(8))
			"    move.w  %[tileAttrib],(%[tilemap],%[h2])\n"

			// Bottom tilemap entry
			"    or.w    %[_TILE_ATTR_VFLIP_MASK],%[tileAttrib]\n" // tileAttrib_aux = (tileAttrib + (h2 & 7)) | TILE_ATTR_VFLIP_MASK;
			"    sub.w   %[h2],%[h2_bottom]\n" // h2_bottom = (VERTICAL_ROWS-1)*PLANE_COLUMNS - ((h2 & ~(8-1)) << (LOG2(PLANE_COLUMNS) - LOG2(8)))
			"    move.w  %[tileAttrib],(%[tilemap],%[h2_bottom])\n"

			: [h2] "+d" (h2), [tileAttrib] "+d" (tileAttrib), [h2_aux] "+d" (h2_aux2)
			: [tilemap] "a" (column_ptr), [CLEAR_BITS_OFFSET] "i" (~(8-1)), [_VERTICAL_ROWS] "i" (VERTICAL_ROWS), [_PLANE_COLUMNS] "i" (PLANE_COLUMNS),
			  [_TILE_ATTR_VFLIP_MASK] "i" (TILE_ATTR_VFLIP_MASK), [_SHIFT_TOP_COLUMN_BYTES] "i" (LOG2(PLANE_COLUMNS) - LOG2(8) + 1), /* +1 due to division by 2 */
			  [h2_bottom] "d" ((VERTICAL_ROWS-1)*PLANE_COLUMNS*2) /* *2 for byte convertion */
			: "cc"
		);
	}
}