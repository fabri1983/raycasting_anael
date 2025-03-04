#include "frame_buffer.h"
#include "consts.h"
#include "consts_ext.h"
#include "utils.h"
#include <vdp_tile.h>
#include <memory.h>

u16* frame_buffer;
u16* column_ptr;

void fb_allocate_frame_buffer ()
{
    frame_buffer = (u16*) FRAME_BUFFER_ADDRESS;

    // Do not use clear_buffer() here because it doesn't save registers in the stack and at this moment in the execution they are actually being used
    memsetU32((u32*)frame_buffer, 0, (VERTICAL_ROWS*TILEMAP_COLUMNS*2)/2);
}

void fb_free_frame_buffer ()
{
    memsetU32((u32*)frame_buffer, 0, (VERTICAL_ROWS*TILEMAP_COLUMNS*2)/2);
}

void clear_buffer_no_usp ()
{
	// We need to clear only first TILEMAP_COLUMNS columns from each row from the framebuffer.
	__asm volatile (
		// Save all registers (except scratch pad). NOTE: no need to save them since this is executed at the beginning of display loop
		//"    movem.l %%d2-%%d7/%%a2-%%a6,-(%%sp)\n"
		// Makes a0 points to the memory location at the framebuffer's end 
        "    move.l  %[frame_buffer_end],%%a0\n" // frame_buffer end address
		// Clear registers
		"    moveq   #0,%%d0\n" // tile index 0 with all attributes in 0
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
        // Iterate over all rows
        ".set regs, 14\n"
		".rept %c[_VERTICAL_ROWS]*2\n"
		    // Clear all the bytes of current row by using regs registers with long word (4 bytes) access.
        "    .rept (%c[TILEMAP_COLUMNS_BYTES] / (regs*4))\n"
        "    movem.l %%d0-%%d7/%%a1-%%a6,-(%%a0)\n"
        "    .endr\n"
        // NOTE: if reminder from the division isn't 0 you need to add the missing operations.
        "    .if ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) > 0 && ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) < 3\n"
        "       .rept ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4)\n"
        "       move.l  %%d0,-(%%a0)\n"
        "       .endr\n"
        "    .endif\n"
        "    .if ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) == 3\n"
        "       movem.l %%d0-%%d2,-(%%a0)\n"
        "    .endif\n"
        "    .if ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) == 4\n"
        "       movem.l %%d0-%%d3,-(%%a0)\n"
        "    .endif\n"
        "    .if ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) == 5\n"
        "       movem.l %%d0-%%d4,-(%%a0)\n"
        "    .endif\n"
        "    .if ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) == 6\n"
        "       movem.l %%d0-%%d5,-(%%a0)\n"
        "    .endif\n"
        "    .if ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) == 7\n"
        "       movem.l %%d0-%%d6,-(%%a0)\n"
        "    .endif\n"
        "    .if ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) == 8\n"
        "       movem.l %%d0-%%d7,-(%%a0)\n"
        "    .endif\n"
        // Remaining conditions (up to regs-1) should be added here and adjusted according the available registers
		".endr\n"
		// Restore all saved registers
		//"    movem.l (%%sp)+,%%d2-%%d7/%%a2-%%a6\n"
		:
		: [frame_buffer_end] "i" (FRAME_BUFFER_ADDRESS + (VERTICAL_ROWS*TILEMAP_COLUMNS*2)*2), 
		  [TILEMAP_COLUMNS_BYTES] "i" (TILEMAP_COLUMNS*2), [_VERTICAL_ROWS] "i" (VERTICAL_ROWS)
		: "memory"
	);
}

void clear_buffer ()
{
	// We need to clear only first TILEMAP_COLUMNS columns from each row from the framebuffer.
	// 9842 cycles according Blastem cycle counter.
	__asm volatile (
		// Save all registers (except scratch pad). NOTE: no need to save them since this is executed at the beginning of display loop
		//"    movem.l %%d2-%%d7/%%a2-%%a6,-(%%sp)\n"
        // Save current SP value in USP. Make sure you are not using SGDK's multitasking feature
		"    move.l  %%sp,%%usp\n"
		// Makes a0 points to the memory location at the framebuffer's end 
        "    move.l  %[frame_buffer_end],%%a0\n" // frame_buffer end address
		// Clear registers
		"    moveq   #0,%%d0\n" // tile index 0 with all attributes in 0
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
        "    move.l  %%d0,%%a7\n"
        // Iterate over all rows
        ".set regs, 15\n"
		".rept %c[_VERTICAL_ROWS]*2\n"
		    // Clear all the bytes of current row by using regs registers with long word (4 bytes) access.
        "    .rept (%c[TILEMAP_COLUMNS_BYTES] / (regs*4))\n"
        "    movem.l %%d0-%%d7/%%a1-%%a7,-(%%a0)\n"
        "    .endr\n"
        // NOTE: if reminder from the division isn't 0 you need to add the missing operations.
        "    .if ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) > 0 && ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) < 3\n"
        "       .rept ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4)\n"
        "       move.l  %%d0,-(%%a0)\n"
        "       .endr\n"
        "    .endif\n"
        "    .if ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) == 3\n"
        "       movem.l %%d0-%%d2,-(%%a0)\n"
        "    .endif\n"
        "    .if ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) == 4\n"
        "       movem.l %%d0-%%d3,-(%%a0)\n"
        "    .endif\n"
        "    .if ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) == 5\n"
        "       movem.l %%d0-%%d4,-(%%a0)\n"
        "    .endif\n"
        "    .if ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) == 6\n"
        "       movem.l %%d0-%%d5,-(%%a0)\n"
        "    .endif\n"
        "    .if ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) == 7\n"
        "       movem.l %%d0-%%d6,-(%%a0)\n"
        "    .endif\n"
        "    .if ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) == 8\n"
        "       movem.l %%d0-%%d7,-(%%a0)\n"
        "    .endif\n"
        // Remaining conditions (up to regs-1) should be added here and adjusted according the available registers
		".endr\n"
        // Restore SP
		"    move.l  %%usp,%%sp\n"
		// Restore all saved registers
		//"    movem.l (%%sp)+,%%d2-%%d7/%%a2-%%a6\n"
		:
		: [frame_buffer_end] "i" (FRAME_BUFFER_ADDRESS + (VERTICAL_ROWS*TILEMAP_COLUMNS*2)*2), 
		  [TILEMAP_COLUMNS_BYTES] "i" (TILEMAP_COLUMNS*2), [_VERTICAL_ROWS] "i" (VERTICAL_ROWS)
		: "memory"
	);
}

void clear_buffer_sp ()
{
	// We need to clear only first TILEMAP_COLUMNS columns from each row from the framebuffer.
	// Here we load the framebuffer address into the SP, previously backed up, to gain 1 more register.
	// 9693 cycles according Blastem cycle counter.
	__asm volatile (
        // Save all registers (except scratch pad). NOTE: no need to save them since this is executed at the beginning of display loop
		//"    movem.l %%d2-%%d7/%%a2-%%a6,-(%%sp)\n"
		// Save current SP value in USP. Make sure you are not using SGDK's multitasking feature
		"    move.l  %%sp,%%usp\n"
		// Makes SP points to the memory location at the framebuffer's end 
        "    move.l  %[frame_buffer_end],%%sp\n" // frame_buffer end address
		// Clear registers
		"    moveq   #0,%%d0\n" // tile index 0 with all attributes in 0
		"    move.l  %%d0,%%d1\n"
		"    move.l  %%d0,%%d2\n"
		"    move.l  %%d0,%%d3\n"
		"    move.l  %%d0,%%d4\n"
		"    move.l  %%d0,%%d5\n"
		"    move.l  %%d0,%%d6\n"
		"    move.l  %%d0,%%d7\n"
		"    move.l  %%d0,%%a0\n"
		"    move.l  %%d0,%%a1\n"
		"    move.l  %%d0,%%a2\n"
		"    move.l  %%d0,%%a3\n"
		"    move.l  %%d0,%%a4\n"
		"    move.l  %%d0,%%a5\n"
		"    move.l  %%d0,%%a6\n"
        // Iterate over all rows (both planes)
        ".set regs, 15\n"
		".rept %c[_VERTICAL_ROWS]*2\n"
		    // Clear all the bytes of current row by using regs registers with long word (4 bytes) access.
        "    .rept (%c[TILEMAP_COLUMNS_BYTES] / (regs*4))\n"
        "    movem.l %%d0-%%d7/%%a0-%%a6,-(%%sp)\n"
        "    .endr\n"
        // NOTE: if reminder from the division isn't 0 you need to add the missing operations.
        "    .if ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) > 0 && ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) < 3\n"
        "       .rept ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4)\n"
        "       move.l  %%d0,-(%%sp)\n"
        "       .endr\n"
        "    .endif\n"
        "    .if ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) == 3\n"
        "       movem.l %%d0-%%d2,-(%%sp)\n"
        "    .endif\n"
        "    .if ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) == 4\n"
        "       movem.l %%d0-%%d3,-(%%sp)\n"
        "    .endif\n"
        "    .if ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) == 5\n"
        "       movem.l %%d0-%%d4,-(%%sp)\n"
        "    .endif\n"
        "    .if ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) == 6\n"
        "       movem.l %%d0-%%d5,-(%%sp)\n"
        "    .endif\n"
        "    .if ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) == 7\n"
        "       movem.l %%d0-%%d6,-(%%sp)\n"
        "    .endif\n"
        "    .if ((%c[TILEMAP_COLUMNS_BYTES] %% (regs*4)) / 4) == 8\n"
        "       movem.l %%d0-%%d7,-(%%sp)\n"
        "    .endif\n"
        // Remaining conditions (up to regs-1) should be added here and adjusted according the available registers
		".endr\n"
		// Restore SP
		"    move.l  %%usp,%%sp\n"
		// Restore all saved registers
		//"    movem.l (%%sp)+,%%d2-%%d7/%%a2-%%a6\n"
		:
		: [frame_buffer_end] "i" (FRAME_BUFFER_ADDRESS + (VERTICAL_ROWS*TILEMAP_COLUMNS*2)*2),
		  [TILEMAP_COLUMNS_BYTES] "i" (TILEMAP_COLUMNS*2), [_VERTICAL_ROWS] "i" (VERTICAL_ROWS)
		:
	);
}

#define H2_FOR_TOP_ENTRY (TILEMAP_COLUMNS/8)*2 // *2 for byte convertion

void write_vline (u16 h2, u16 tileAttrib)
{
	// Tilemap width in tiles.

	// Draw a solid vertical line
	if (h2 == 0) {
		// C version
		/*for (u16 y = 0; y < VERTICAL_ROWS*TILEMAP_COLUMNS; y+=TILEMAP_COLUMNS) {
			column_ptr[y] = tileAttrib;
		}*/

		// ASM version
		__asm volatile (
			".set off,0\n"
			".rept %c[_VERTICAL_ROWS]\n"
			"    move.w  %[tileAttrib],off(%[tilemap])\n"
			"    .set off,off+%c[_TILEMAP_COLUMNS]*2\n" // *2 for byte addressing
			".endr\n"
			: [tileAttrib] "+d" (tileAttrib)
			: [tilemap] "a" (column_ptr), 
              [_VERTICAL_ROWS] "i" (VERTICAL_ROWS), [_TILEMAP_COLUMNS] "i" (TILEMAP_COLUMNS)
			:
		);

        return;
	}

    // TOP AND BOTTOM TILEMAP ENTRIES
    
    //u16 ta = (h2 / 8); // vertical tilemap entry position

    // top tilemap entry
    //column_ptr[ta*TILEMAP_COLUMNS] = tileAttrib + (h2 & 7); // offsets the tileAttrib by the halved pixel height modulo 8
    // bottom tilemap entry (with flipped attribute)
    //column_ptr[((VERTICAL_ROWS-1)-ta)*TILEMAP_COLUMNS] = (tileAttrib + (h2 & 7)) | TILE_ATTR_VFLIP_MASK;

    // THIS is faster when implemented in the inline ASM block, as done below
    // top tilemap entry
    //column_ptr[((h2 & ~(8-1)) * (TILEMAP_COLUMNS/8))] = tileAttrib + (h2 & 7); // offsets the tileAttrib by the halved pixel height modulo 8
    // bottom tilemap entry (with flipped attribute)
    //column_ptr[(VERTICAL_ROWS-1)*TILEMAP_COLUMNS - ((h2 & ~(8-1))*(TILEMAP_COLUMNS/8))] = (tileAttrib + (h2 & 7)) | TILE_ATTR_VFLIP_MASK;

    // Remaining VERTICAL_ROWS-2 tilemap entries

    // HERE WE TRY TO CLEAR THE TILEMAP ENTRIES FOR THE CURRENT COLUMN. THIS IS SLOWER THAN USING clear_buffer().
    // C version (for VERTICAL_ROWS = 24).
    // Set tileAttrib_tile0 which points to tile 0.
    /*u16 tileAttrib_tile0 = tileAttrib & ~TILE_INDEX_MASK;
    switch (ta) {
        case 11:	column_ptr[(11-1)*TILEMAP_COLUMNS] = tileAttrib_tile0;
                    column_ptr[(12+1)*TILEMAP_COLUMNS] = tileAttrib_tile0; // fallthru
        case 10:	column_ptr[(10-1)*TILEMAP_COLUMNS] = tileAttrib_tile0;
                    column_ptr[(13+1)*TILEMAP_COLUMNS] = tileAttrib_tile0; // fallthru
        case 9:		column_ptr[(9-1)*TILEMAP_COLUMNS] = tileAttrib_tile0;
                    column_ptr[(14+1)*TILEMAP_COLUMNS] = tileAttrib_tile0; // fallthru
        case 8:		column_ptr[(8-1)*TILEMAP_COLUMNS] = tileAttrib_tile0;
                    column_ptr[(15+1)*TILEMAP_COLUMNS] = tileAttrib_tile0; // fallthru
        case 7:		column_ptr[(7-1)*TILEMAP_COLUMNS] = tileAttrib_tile0;
                    column_ptr[(16+1)*TILEMAP_COLUMNS] = tileAttrib_tile0; // fallthru
        case 6:		column_ptr[(6-1)*TILEMAP_COLUMNS] = tileAttrib_tile0;
                    column_ptr[(17+1)*TILEMAP_COLUMNS] = tileAttrib_tile0; // fallthru
        case 5:		column_ptr[(5-1)*TILEMAP_COLUMNS] = tileAttrib_tile0;
                    column_ptr[(18+1)*TILEMAP_COLUMNS] = tileAttrib_tile0; // fallthru
        case 4:		column_ptr[(4-1)*TILEMAP_COLUMNS] = tileAttrib_tile0;
                    column_ptr[(19+1)*TILEMAP_COLUMNS] = tileAttrib_tile0; // fallthru
        case 3:		column_ptr[(3-1)*TILEMAP_COLUMNS] = tileAttrib_tile0;
                    column_ptr[(20+1)*TILEMAP_COLUMNS] = tileAttrib_tile0; // fallthru
        case 2:		column_ptr[(2-1)*TILEMAP_COLUMNS] = tileAttrib_tile0;
                    column_ptr[(21+1)*TILEMAP_COLUMNS] = tileAttrib_tile0; // fallthru
        case 1:		column_ptr[(1-1)*TILEMAP_COLUMNS] = tileAttrib_tile0;
                    column_ptr[(22+1)*TILEMAP_COLUMNS] = tileAttrib_tile0; // fallthru
        case 0:     break;
    }*/

    // HERE WE TRY TO CLEAR THE TILEMAP ENTRIES FOR THE CURRENT COLUMN. THIS IS SLOWER THAN USING clear_buffer().
    // ASM version.
    // This block sets tileAttrib_tile0 which points to tile 0.
    /*u16 jump = ((VERTICAL_ROWS-2)/2) * 8; // (multiplied by jump block size)
    u16 h2_aux1 = h2;
    u16 tileAttrib_tile0 = tileAttrib & ~TILE_INDEX_MASK;
    __asm volatile (
        // Offset h2 comes already multiplied by 8, great, but we need to clear the first 3 bits so 
        // we can use it as a multiple of the jump block size (8 bytes)
        "    andi.w  %[CLEAR_BITS_OFFSET],%[h2]\n" // (h2 & ~(8-1))
        // To calculate the jump we take the complement to (VERTICAL_ROWS-2)/2
        "    sub.w   %[h2],%[jump]\n" // jump = ((VERTICAL_ROWS-2)/2)*8 - h2
        // Jump into table using h2
        "    jmp     .wvl_clear_table_%=(%%pc,%[jump].w)\n"
        // Assignment table:
        //  from [((VERTICAL_ROWS-2)/2 - 1) * TILEMAP_COLUMNS] down to [1*TILEMAP_COLUMNS]
        //  from [((VERTICAL_ROWS-2)/2 + 1 + 1) * TILEMAP_COLUMNS] up to [(VERTICAL_ROWS-2)*TILEMAP_COLUMNS]
        ".wvl_clear_table_%=:\n"
        ".set offdown, (((%c[_VERTICAL_ROWS] - 2) / 2) - 1) * %c[_TILEMAP_COLUMNS] * 2\n" // *2 for byte convertion
        ".set offup, (((%c[_VERTICAL_ROWS] - 2) / 2) + 1 + 1) * %c[_TILEMAP_COLUMNS] * 2\n" // *2 for byte convertion
        ".rept ((%c[_VERTICAL_ROWS] - 2) / 2)\n"
        "    move.w  %[tileAttrib_tile0],offdown(%[tilemap])\n"
        "    move.w  %[tileAttrib_tile0],offup(%[tilemap])\n"
        "    .set offdown, offdown - %c[_TILEMAP_COLUMNS] * 2\n" // *2 for byte convertion
        "    .set offup, offup + %c[_TILEMAP_COLUMNS] * 2\n" // *2 for byte convertion
        ".endr\n"
        // Case 0 (the complement to (VERTICAL_ROWS-2)/2) falls here
        : [h2] "+d" (h2_aux1), [tileAttrib_tile0] "+d" (tileAttrib_tile0), [jump] "+d" (jump)
        : [tilemap] "a" (column_ptr), [CLEAR_BITS_OFFSET] "i" (~(8-1)), 
          [_VERTICAL_ROWS] "i" (VERTICAL_ROWS), [_TILEMAP_COLUMNS] "i" (TILEMAP_COLUMNS)
        :
    );*/

    // C version (for VERTICAL_ROWS = 24).
    // Set tileAttrib which points to a colored tile.
    /*switch (ta) {
        case 0:		column_ptr[1*TILEMAP_COLUMNS] = tileAttrib;
                    column_ptr[22*TILEMAP_COLUMNS] = tileAttrib; // fallthru
        case 1:		column_ptr[2*TILEMAP_COLUMNS] = tileAttrib;
                    column_ptr[21*TILEMAP_COLUMNS] = tileAttrib; // fallthru
        case 2:		column_ptr[3*TILEMAP_COLUMNS] = tileAttrib;
                    column_ptr[20*TILEMAP_COLUMNS] = tileAttrib; // fallthru
        case 3:		column_ptr[4*TILEMAP_COLUMNS] = tileAttrib;
                    column_ptr[19*TILEMAP_COLUMNS] = tileAttrib; // fallthru
        case 4:		column_ptr[5*TILEMAP_COLUMNS] = tileAttrib;
                    column_ptr[18*TILEMAP_COLUMNS] = tileAttrib; // fallthru
        case 5:		column_ptr[6*TILEMAP_COLUMNS] = tileAttrib;
                    column_ptr[17*TILEMAP_COLUMNS] = tileAttrib; // fallthru
        case 6:		column_ptr[7*TILEMAP_COLUMNS] = tileAttrib;
                    column_ptr[16*TILEMAP_COLUMNS] = tileAttrib; // fallthru
        case 7:		column_ptr[8*TILEMAP_COLUMNS] = tileAttrib;
                    column_ptr[15*TILEMAP_COLUMNS] = tileAttrib; // fallthru
        case 8:		column_ptr[9*TILEMAP_COLUMNS] = tileAttrib;
                    column_ptr[14*TILEMAP_COLUMNS] = tileAttrib; // fallthru
        case 9:		column_ptr[10*TILEMAP_COLUMNS] = tileAttrib;
                    column_ptr[13*TILEMAP_COLUMNS] = tileAttrib; // fallthru
        case 10:	column_ptr[11*TILEMAP_COLUMNS] = tileAttrib;
                    column_ptr[12*TILEMAP_COLUMNS] = tileAttrib; // fallthru
                    break;
    }*/

    // ASM version.
    // This block of code sets tileAttrib which points to a colored tile.
    // This block of code sets top and bottom tilemap entries.
    u16 h2_aux2 = h2;
    __asm volatile (
        // Offset h2 comes already multiplied by 8, great, but we need to clear the first 3 bits so 
        // we can use it as a multiple of the block size (8 bytes) we'll jump into
        "    andi.w  %[CLEAR_BITS_OFFSET],%[h2]\n" // (h2 & ~(8-1))
        // Jump into table using h2
        "    jmp     .wvl_table_%=(%%pc,%[h2].w)\n"
        // Assignment ranges:
        //  from [1*TILEMAP_COLUMNS] up to [((VERTICAL_ROWS-2)/2)*TILEMAP_COLUMNS]
        //  from [(VERTICAL_ROWS-2)*TILEMAP_COLUMNS] down to [(((VERTICAL_ROWS-2)/2)+1)*TILEMAP_COLUMNS]
        // Eg for VERTICAL_ROWS=28: 
        //  from [1*TILEMAP_COLUMNS] up to [13*TILEMAP_COLUMNS] and from [26*TILEMAP_COLUMNS] down to [14*TILEMAP_COLUMNS]
        // Eg for VERTICAL_ROWS=24: 
        //  from [1*TILEMAP_COLUMNS] up to [11*TILEMAP_COLUMNS] and from [22*TILEMAP_COLUMNS] down to [12*TILEMAP_COLUMNS]
        ".wvl_table_%=:\n"
        ".set offup, 1 * %c[_TILEMAP_COLUMNS] * 2\n" // *2 for byte convertion
        ".set offdown, (%c[_VERTICAL_ROWS] - 2) * %c[_TILEMAP_COLUMNS] * 2\n" // *2 for byte convertion
        ".rept (%c[_VERTICAL_ROWS] - 2) / 2\n"
        "    move.w  %[tileAttrib],offup(%[tilemap])\n"
        "    move.w  %[tileAttrib],offdown(%[tilemap])\n"
        "    .set offup, offup + %c[_TILEMAP_COLUMNS] * 2\n" // *2 for byte convertion
        "    .set offdown, offdown - (%c[_TILEMAP_COLUMNS] * 2)\n" // *2 for byte convertion
        ".endr\n"

        // Setup for top and bottom tilemap entries
        "    andi.w  #7,%[h2_aux]\n" // h2_aux = (h2 & 7)
        "    add.w   %[h2_aux],%[tileAttrib]\n" // tileAttrib += (h2 & 7);

        // Top tilemap entry
        // #if H2_FOR_TOP_ENTRY == 10
        // "    move.w	%[h2],%[h2_aux]\n"
        // "    add.w	%[h2],%[h2]\n"
        // "    add.w	%[h2],%[h2]\n"
        // "    add.w	%[h2_aux],%[h2]\n"
        // "    add.w	%[h2],%[h2]\n"
        // #elif H2_FOR_TOP_ENTRY == 8
        // "    lsl.w   #3,%[h2]\n"
        // #else
        "    mulu.w  %[H2_TOP],%[h2]\n" // h2 = ((h2 & ~(8-1)) * (TILEMAP_COLUMNS/8))
        // #endif
        "    move.w  %[tileAttrib],(%[tilemap],%[h2].w)\n"

        // Bottom tilemap entry
        "    or.w    %[_TILE_ATTR_VFLIP_MASK],%[tileAttrib]\n" // tileAttrib = (tileAttrib + (h2 & 7)) | TILE_ATTR_VFLIP_MASK;
        "    neg.w   %[h2]\n"
        "    addi.w  %[H2_BOTTOM],%[h2]\n" // h2 = (VERTICAL_ROWS-1)*TILEMAP_COLUMNS - ((h2 & ~(8-1))*(TILEMAP_COLUMNS/8))
        "    move.w  %[tileAttrib],(%[tilemap],%[h2].w)"

        : [h2] "+d" (h2), [tileAttrib] "+d" (tileAttrib), [h2_aux] "+d" (h2_aux2)
        : [tilemap] "a" (column_ptr), [CLEAR_BITS_OFFSET] "i" (~(8-1)), 
          [_VERTICAL_ROWS] "i" (VERTICAL_ROWS), [_TILEMAP_COLUMNS] "i" (TILEMAP_COLUMNS), 
          [H2_TOP] "i" (H2_FOR_TOP_ENTRY), [H2_BOTTOM] "i" ((VERTICAL_ROWS-1)*TILEMAP_COLUMNS*2), // *2 for byte convertion
          [_TILE_ATTR_VFLIP_MASK] "i" (TILE_ATTR_VFLIP_MASK)
        :
    );
}