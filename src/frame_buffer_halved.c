#include "frame_buffer.h"
#include "consts.h"
#include "consts_ext.h"
#include "utils.h"
#include <vdp_tile.h>

void clear_buffer_halved_no_usp ()
{
    // We need to clear only first TILEMAP_COLUMNS columns from each row from the framebuffer.
    // Only half Plane A and half Plane B.
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
        // frame_buffer's Plane B region (actually at the end)
        // Iterate over bottom half rows - 1
		".rept (%c[_VERTICAL_ROWS]/2 - 1)\n"
		    // Clear all the bytes of current row by using 14 registers with long word (4 bytes) access.
		"    .rept (%c[TILEMAP_COLUMNS_BYTES] / (14*4))\n"
    	"    movem.l %%d0-%%d7/%%a1-%%a6,-(%%a0)\n"
    	"    .endr\n"
		    // NOTE: if reminder from the division isn't 0 you need to add the missing operations.
		"    .rept ((%c[TILEMAP_COLUMNS_BYTES] %% (14*4)) / 4)\n"
		"    move.l  %%d0,-(%%a0)\n"
		"    .endr\n"
		    // Skip the non displayed data
		"    lea     -%c[NON_DISPLAYED_BYTES_PER_ROW](%%a0),%%a0\n"
		".endr\n"
		// One more iteration without the last lea instruction
		"    .rept (%c[TILEMAP_COLUMNS_BYTES] / (14*4))\n"
    	"    movem.l %%d0-%%d7/%%a1-%%a6,-(%%a0)\n"
    	"    .endr\n"
		"    .rept ((%c[TILEMAP_COLUMNS_BYTES] %% (14*4)) / 4)\n"
		"    move.l  %%d0,-(%%a0)\n"
		"    .endr\n"
        // Accomodate to locate at the end of frame_buffer's Plane A region
        "    lea     -%c[NON_DISPLAYED_BYTES_PER_ROW]-%c[PLANE_COLUMNS_BYTES]*%c[_VERTICAL_ROWS]/2(%%a0),%%a0\n"
        // frame_buffer's Plane A region (actually at the end)
        // Iterate over bottom half rows - 1
        ".rept (%c[_VERTICAL_ROWS]/2 - 1)\n"
		    // Clear all the bytes of current row by using 14 registers with long word (4 bytes) access.
		"    .rept (%c[TILEMAP_COLUMNS_BYTES] / (14*4))\n"
    	"    movem.l %%d0-%%d7/%%a1-%%a6,-(%%a0)\n"
    	"    .endr\n"
		    // NOTE: if reminder from the division isn't 0 you need to add the missing operations.
		"    .rept ((%c[TILEMAP_COLUMNS_BYTES] %% (14*4)) / 4)\n"
		"    move.l  %%d0,-(%%a0)\n"
		"    .endr\n"
		    // Skip the non displayed data
		"    lea     -%c[NON_DISPLAYED_BYTES_PER_ROW](%%a0),%%a0\n"
		".endr\n"
		// One more iteration without the last lea instruction
		"    .rept (%c[TILEMAP_COLUMNS_BYTES] / (14*4))\n"
    	"    movem.l %%d0-%%d7/%%a1-%%a6,-(%%a0)\n"
    	"    .endr\n"
		"    .rept ((%c[TILEMAP_COLUMNS_BYTES] %% (14*4)) / 4)\n"
		"    move.l  %%d0,-(%%a0)\n"
		"    .endr\n"
		// Restore all saved registers
		//"    movem.l (%%sp)+,%%d2-%%d7/%%a2-%%a6\n"
		:
		: [frame_buffer_end] "i" (FRAME_BUFFER_ADDRESS + VERTICAL_ROWS*PLANE_COLUMNS*2*2 - (PLANE_COLUMNS-TILEMAP_COLUMNS)*2), 
		  [TILEMAP_COLUMNS_BYTES] "i" (TILEMAP_COLUMNS*2), [_VERTICAL_ROWS] "i" (VERTICAL_ROWS), 
		  [NON_DISPLAYED_BYTES_PER_ROW] "i" ((PLANE_COLUMNS-TILEMAP_COLUMNS)*2),
          [PLANE_COLUMNS_BYTES] "i" (PLANE_COLUMNS*2)
		: "memory"
	);
}

void clear_buffer_halved ()
{
	// We need to clear only first TILEMAP_COLUMNS columns from each row from the framebuffer.
    // Only half Plane A and half Plane B.
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
        // frame_buffer's Plane B region (actually at the end)
        // Iterate over bottom half rows - 1
		".rept (%c[_VERTICAL_ROWS]/2 - 1)\n"
		    // Clear all the bytes of current row by using 15 registers with long word (4 bytes) access.
		"    .rept (%c[TILEMAP_COLUMNS_BYTES] / (15*4))\n"
    	"    movem.l %%d0-%%d7/%%a1-%%a7,-(%%a0)\n"
    	"    .endr\n"
		    // NOTE: if reminder from the division isn't 0 you need to add the missing operations.
		"    .rept ((%c[TILEMAP_COLUMNS_BYTES] %% (15*4)) / 4)\n"
		"    move.l  %%d0,-(%%a0)\n"
		"    .endr\n"
		    // Skip the non displayed data
		"    lea     -%c[NON_DISPLAYED_BYTES_PER_ROW](%%a0),%%a0\n"
		".endr\n"
		// One more iteration without the last lea instruction
		"    .rept (%c[TILEMAP_COLUMNS_BYTES] / (15*4))\n"
    	"    movem.l %%d0-%%d7/%%a1-%%a7,-(%%a0)\n"
    	"    .endr\n"
		"    .rept ((%c[TILEMAP_COLUMNS_BYTES] %% (15*4)) / 4)\n"
		"    move.l  %%d0,-(%%a0)\n"
		"    .endr\n"
        // Accomodate to locate at the end of frame_buffer's Plane A region
        "    lea     -%c[NON_DISPLAYED_BYTES_PER_ROW]-%c[PLANE_COLUMNS_BYTES]*%c[_VERTICAL_ROWS]/2(%%a0),%%a0\n"
        // frame_buffer's Plane A region (actually at the end)
        // Iterate over bottom half rows - 1
        ".rept (%c[_VERTICAL_ROWS]/2 - 1)\n"
		    // Clear all the bytes of current row by using 15 registers with long word (4 bytes) access.
		"    .rept (%c[TILEMAP_COLUMNS_BYTES] / (15*4))\n"
    	"    movem.l %%d0-%%d7/%%a1-%%a7,-(%%a0)\n"
    	"    .endr\n"
		    // NOTE: if reminder from the division isn't 0 you need to add the missing operations.
		"    .rept ((%c[TILEMAP_COLUMNS_BYTES] %% (15*4)) / 4)\n"
		"    move.l  %%d0,-(%%a0)\n"
		"    .endr\n"
		    // Skip the non displayed data
		"    lea     -%c[NON_DISPLAYED_BYTES_PER_ROW](%%a0),%%a0\n"
		".endr\n"
		// One more iteration without the last lea instruction
		"    .rept (%c[TILEMAP_COLUMNS_BYTES] / (15*4))\n"
    	"    movem.l %%d0-%%d7/%%a1-%%a7,-(%%a0)\n"
    	"    .endr\n"
		"    .rept ((%c[TILEMAP_COLUMNS_BYTES] %% (15*4)) / 4)\n"
		"    move.l  %%d0,-(%%a0)\n"
		"    .endr\n"
        // Restore SP
		"    move.l  %%usp,%%sp\n"
		// Restore all saved registers
		//"    movem.l (%%sp)+,%%d2-%%d7/%%a2-%%a6\n"
		:
		: [frame_buffer_end] "i" (FRAME_BUFFER_ADDRESS + VERTICAL_ROWS*PLANE_COLUMNS*2*2 - (PLANE_COLUMNS-TILEMAP_COLUMNS)*2), 
		  [TILEMAP_COLUMNS_BYTES] "i" (TILEMAP_COLUMNS*2), [_VERTICAL_ROWS] "i" (VERTICAL_ROWS), 
		  [NON_DISPLAYED_BYTES_PER_ROW] "i" ((PLANE_COLUMNS-TILEMAP_COLUMNS)*2),
          [PLANE_COLUMNS_BYTES] "i" (PLANE_COLUMNS*2)
		: "memory"
	);
}

void clear_buffer_halved_sp ()
{
    // We need to clear only first TILEMAP_COLUMNS columns from each row from the framebuffer.
    // Only half Plane A and half Plane B.
	// Here we load the framebuffer address into the SP, previously backed up, to gain 1 more register.
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
        // frame_buffer's Plane B region (actually at the end)
        // Iterate over bottom half rows - 1
		".rept (%c[_VERTICAL_ROWS]/2 - 1)\n"
		    // Clear all the bytes of current row by using 15 registers with long word (4 bytes) access.
		"    .rept (%c[TILEMAP_COLUMNS_BYTES] / (15*4))\n"
    	"    movem.l %%d0-%%d7/%%a0-%%a6,-(%%sp)\n"
    	"    .endr\n"
		    // NOTE: if reminder from the division isn't 0 you need to add the missing operations.
		"    .rept ((%c[TILEMAP_COLUMNS_BYTES] %% (15*4)) / 4)\n"
		"    move.l  %%d0,-(%%sp)\n"
		"    .endr\n"
		    // Skip the non displayed data
		"    lea     -%c[NON_DISPLAYED_BYTES_PER_ROW](%%sp),%%sp\n"
		".endr\n"
		// One more iteration without the last lea instruction
		"    .rept (%c[TILEMAP_COLUMNS_BYTES] / (15*4))\n"
    	"    movem.l %%d0-%%d7/%%a0-%%a6,-(%%sp)\n"
    	"    .endr\n"
		"    .rept ((%c[TILEMAP_COLUMNS_BYTES] %% (15*4)) / 4)\n"
		"    move.l  %%d0,-(%%sp)\n"
		"    .endr\n"
        // Accomodate to locate at the end of frame_buffer's Plane A region
        "    lea     -%c[NON_DISPLAYED_BYTES_PER_ROW]-%c[PLANE_COLUMNS_BYTES]*%c[_VERTICAL_ROWS]/2(%%sp),%%sp\n"
        // frame_buffer's Plane A region (actually at the end)
        // Iterate over bottom half rows - 1
        ".rept (%c[_VERTICAL_ROWS]/2 - 1)\n"
		    // Clear all the bytes of current row by using 15 registers with long word (4 bytes) access.
		"    .rept (%c[TILEMAP_COLUMNS_BYTES] / (15*4))\n"
    	"    movem.l %%d0-%%d7/%%a0-%%a6,-(%%sp)\n"
    	"    .endr\n"
		    // NOTE: if reminder from the division isn't 0 you need to add the missing operations.
		"    .rept ((%c[TILEMAP_COLUMNS_BYTES] %% (15*4)) / 4)\n"
		"    move.l  %%d0,-(%%sp)\n"
		"    .endr\n"
		    // Skip the non displayed data
		"    lea     -%c[NON_DISPLAYED_BYTES_PER_ROW](%%sp),%%sp\n"
		".endr\n"
		// One more iteration without the last lea instruction
		"    .rept (%c[TILEMAP_COLUMNS_BYTES] / (15*4))\n"
    	"    movem.l %%d0-%%d7/%%a0-%%a6,-(%%sp)\n"
    	"    .endr\n"
		"    .rept ((%c[TILEMAP_COLUMNS_BYTES] %% (15*4)) / 4)\n"
		"    move.l  %%d0,-(%%sp)\n"
		"    .endr\n"
		// Restore SP
		"    move.l  %%usp,%%sp\n"
		// Restore all saved registers
		//"    movem.l (%%sp)+,%%d2-%%d7/%%a2-%%a6\n"
		: 
		: [frame_buffer_end] "i" (FRAME_BUFFER_ADDRESS + VERTICAL_ROWS*PLANE_COLUMNS*2*2 - (PLANE_COLUMNS-TILEMAP_COLUMNS)*2),
		  [TILEMAP_COLUMNS_BYTES] "i" (TILEMAP_COLUMNS*2), [_VERTICAL_ROWS] "i" (VERTICAL_ROWS), 
		  [NON_DISPLAYED_BYTES_PER_ROW] "i" ((PLANE_COLUMNS-TILEMAP_COLUMNS)*2),
          [PLANE_COLUMNS_BYTES] "i" (PLANE_COLUMNS*2)
		:
	);
}

#if RENDER_MIRROR_PLANES_USING_CPU_RAM || RENDER_MIRROR_PLANES_USING_VDP_VRAM
// Stores top tilemap entry value followed by h2 row value, for evey processed column.
static u16 top_entries[2*PIXEL_COLUMNS];
// Keep track of current column this way, otherwise the code breaks.
static u8 top_entries_current_col;
#endif

FORCE_INLINE void fb_set_top_entries_column (u8 pixel_column)
{
    #if RENDER_MIRROR_PLANES_USING_CPU_RAM || RENDER_MIRROR_PLANES_USING_VDP_VRAM
    top_entries_current_col = pixel_column;
    #endif
}

FORCE_INLINE void fb_increment_entries_column ()
{
    // NOTE: we must do this here otherwise if done in write_vline_halved() it breaks the system
    #if RENDER_MIRROR_PLANES_USING_CPU_RAM || RENDER_MIRROR_PLANES_USING_VDP_VRAM
    top_entries_current_col += 2; // we advance 2 positions since we store val and he together
    #endif
}

void write_vline_halved (u16 h2, u16 tileAttrib)
{
	// Tilemap width in tiles.

	// Draw HALF a solid vertical line: from BOTTOM to TOP
	if (h2 == 0) {
		// C version
		//for (u16 y = 0; y < VERTICAL_ROWS*PLANE_COLUMNS/2; y+=PLANE_COLUMNS) {
		// 	column_ptr[y] = tileAttrib;
		//}

		// ASM version
		__asm volatile (
			".set off,(%c[_VERTICAL_ROWS]/2)*(%c[_PLANE_COLUMNS]*2)\n" // *2 for byte addressing
			".rept %c[_VERTICAL_ROWS]/2\n"
			"    move.w  %[tileAttrib],off(%[tilemap])\n"
			"    .set off,off+%c[_PLANE_COLUMNS]*2\n" // *2 for byte addressing
			".endr\n"
			: [tileAttrib] "+d" (tileAttrib)
			: [tilemap] "a" (column_ptr), 
              [_VERTICAL_ROWS] "i" (VERTICAL_ROWS), [_PLANE_COLUMNS] "i" (PLANE_COLUMNS)
			:
		);

        return;
	}

    // ASM version.
    // This block of code sets tileAttrib which points to a colored tile.
    // This block of code sets bottom tilemap entry and save the top value into an array for later use.
    u16 h2_aux2 = h2;
    #if RENDER_MIRROR_PLANES_USING_CPU_RAM || RENDER_MIRROR_PLANES_USING_VDP_VRAM
    u16* top_entries_ptr = top_entries + top_entries_current_col;
    #endif
    __asm volatile (
        // Offset h2 comes already multiplied by 8, great, but we need to clear the first 3 bits so 
        // we can use it as a multiple of the block size (4 bytes) we'll jump into
        "    andi.w  %[CLEAR_BITS_OFFSET],%[h2]\n" // (h2 & ~(8-1))
        "    lsr.w   #1,%[h2]\n" // make it multiple of 4
        // Jump into table using h2
        "    jmp     .wvl_table_%=(%%pc,%[h2].w)\n"
        // Assignment ranges:
        //  from [(VERTICAL_ROWS-2)*PLANE_COLUMNS] down to [(((VERTICAL_ROWS-2)/2)+1)*PLANE_COLUMNS]
        // Eg for VERTICAL_ROWS=28: 
        //  from [26*PLANE_COLUMNS] down to [14*PLANE_COLUMNS]
        // Eg for VERTICAL_ROWS=24: 
        //  from [22*PLANE_COLUMNS] down to [12*PLANE_COLUMNS]
        ".wvl_table_%=:\n"
        ".set offdown, (%c[_VERTICAL_ROWS] - 2) * %c[_PLANE_COLUMNS] * 2\n" // *2 for byte convertion
        ".rept (%c[_VERTICAL_ROWS] - 2) / 2\n"
        "    move.w  %[tileAttrib],offdown(%[tilemap])\n"
        "    .set offdown, offdown - %c[_PLANE_COLUMNS] * 2\n" // *2 for byte convertion
        ".endr\n"

        // Setup for top and bottom tilemap entries
        "    andi.w  #7,%[h2_aux]\n" // h2_aux = (h2 & 7)
        "    add.w   %[h2_aux],%[tileAttrib]\n" // tileAttrib += (h2 & 7);

        // Top tilemap entry (here we only care that the TILE_ATTR_VFLIP_MASK is set)
        "    lsl.w   %[_SHIFT_TOP_COLUMN_BYTES],%[h2]\n" // h2 = (h2 & ~(8-1)) << (LOG2(PLANE_COLUMNS) - LOG2(8))
        #if RENDER_MIRROR_PLANES_USING_CPU_RAM || RENDER_MIRROR_PLANES_USING_VDP_VRAM
        //"    move.w  %[tileAttrib],(%[tilemap],%[h2])\n" // WE DON'T NEED THIS ANYMORE NOW THAT WE STORE IT IN top_entries[]
        "    move.w  %[tileAttrib],(%[top_entries_ptr])\n" // Stores the value for top tilemap entry
        "    move.w  %[h2],2(%[top_entries_ptr])\n" // Stores the row for top tilemap entry
        #endif

        // Bottom tilemap entry
        "    or.w    %[_TILE_ATTR_VFLIP_MASK],%[tileAttrib]\n" // tileAttrib = (tileAttrib + (h2 & 7)) | TILE_ATTR_VFLIP_MASK;
        "    sub.w   %[h2],%[h2_bottom]\n" // h2_bottom = (VERTICAL_ROWS-1)*PLANE_COLUMNS - ((h2 & ~(8-1)) << (LOG2(PLANE_COLUMNS) - LOG2(8)))
        "    move.w  %[tileAttrib],(%[tilemap],%[h2_bottom])\n"

        : [h2] "+d" (h2), [tileAttrib] "+d" (tileAttrib), [h2_aux] "+d" (h2_aux2)
          #if RENDER_MIRROR_PLANES_USING_CPU_RAM || RENDER_MIRROR_PLANES_USING_VDP_VRAM
          , [top_entries_ptr] "+a" (top_entries_ptr)
          #endif
        : [tilemap] "a" (column_ptr), [CLEAR_BITS_OFFSET] "i" (~(8-1)), [_VERTICAL_ROWS] "i" (VERTICAL_ROWS), [_PLANE_COLUMNS] "i" (PLANE_COLUMNS),
          [_TILE_ATTR_VFLIP_MASK] "i" (TILE_ATTR_VFLIP_MASK), 
          [_SHIFT_TOP_COLUMN_BYTES] "i" (LOG2(PLANE_COLUMNS) - LOG2(8) + 1 + 1), // +1 due to division by 2, +1 due to initial lsr.w #1
          [h2_bottom] "d" ((VERTICAL_ROWS-1)*PLANE_COLUMNS*2),// *2 for byte convertion
          [_PIXEL_COLUMNS] "i" (PIXEL_COLUMNS)
        :
    );
}

#define copy_bottom_half_into_top_half(srcAddr,dstAddr) \
    __asm volatile ( \
        /* Save all registers (except scratch pad) */ \
		"movem.l %%d2-%%d7/%%a2-%%a6,-(%%sp)\n\t" \
        /* Save current SP value in USP. Make sure you are not using SGDK's multitasking feature */ \
		"move.l  %%sp,%%usp\n\t" \
        /* Load source and destiny addresses */ \
        "move.l  %[_srcAddr],%%a0\n\t" \
        "move.l  %[_dstAddr],%%a1\n\t" \
        /* Iterate VERTICAL_ROWS/2 - 1 so we can ommit last lea instructions */ \
        ".rept %c[_VERTICAL_ROWS]/2 - 1\n\t" \
            /* Copy long words to 14 registers, then copy them into target */ \
            ".rept (%c[TILEMAP_COLUMNS_BYTES] / (14*4))\n\t" \
            "movem.l (%%a0)+,%%d0-%%d7/%%a2-%%a7\n\t" \
            "movem.l %%d0-%%d7/%%a2-%%a7,(%%a1)\n\t" \
            "lea     14*4(%%a1),%%a1\n\t" \
            ".endr\n\t" \
            /* NOTE: if reminder from the division isn't 0 you need to add the missing operations. */ \
            ".rept ((%c[TILEMAP_COLUMNS_BYTES] %% (14*4)) / 4)\n\t" \
            "move.l  (%%a0)+,(%%a1)+\n\t" \
            ".endr\n\t" \
            /* Re accommodate for next iteration */ \
            "lea     2*(%c[_PLANE_COLUMNS]-%c[_TILEMAP_COLUMNS])(%%a0),%%a0\n\t" \
            "lea     -2*(%c[_PLANE_COLUMNS]+%c[_TILEMAP_COLUMNS])(%%a1),%%a1\n\t" \
        ".endr\n\t" \
        /* One last iteration without the lea instructions */ \
            /* Copy long words to 14 registers, then copy them into target */ \
            ".rept (%c[TILEMAP_COLUMNS_BYTES] / (14*4))\n\t" \
            "movem.l (%%a0)+,%%d0-%%d7/%%a2-%%a7\n\t" \
            "movem.l %%d0-%%d7/%%a2-%%a7,(%%a1)\n\t" \
            "lea     14*4(%%a1),%%a1\n\t" \
            ".endr\n\t" \
            /* NOTE: if reminder from the division isn't 0 you need to add the missing operations. */ \
            ".rept ((%c[TILEMAP_COLUMNS_BYTES] %% (14*4)) / 4)\n\t" \
            "move.l  (%%a0)+,(%%a1)+\n\t" \
            ".endr\n\t" \
        /* Restore SP */ \
		"move.l  %%usp,%%sp\n\t" \
		/* Restore all saved registers */ \
		"movem.l (%%sp)+,%%d2-%%d7/%%a2-%%a6" \
        : \
        : [_srcAddr] "i" (srcAddr), [_dstAddr] "i" (dstAddr), \
          [_TILEMAP_COLUMNS] "i" (TILEMAP_COLUMNS), [_PLANE_COLUMNS] "i" (PLANE_COLUMNS), \
          [_VERTICAL_ROWS] "i" (VERTICAL_ROWS), [TILEMAP_COLUMNS_BYTES] "i" (TILEMAP_COLUMNS*2) \
        : \
    )

static FORCE_INLINE void copy_top_entries_in_RAM ()
{
    #if RENDER_MIRROR_PLANES_USING_CPU_RAM

    // C version: set tilemap top entries (already inverted)
    /*u16* entries_ptr = top_entries;
    #pragma GCC unroll 80 // Always set the max number since it does not accept defines
    for (u8 i=0; i < PIXEL_COLUMNS/2; ++i) {
        u16 val = *entries_ptr++;
        u16 h2 = *entries_ptr++;
        frame_buffer[i + h2] = val; // h2 comes in byte addressing
        val = *entries_ptr++;
        h2 = *entries_ptr++;
        frame_buffer[VERTICAL_ROWS*PLANE_COLUMNS + i + h2] = val; // h2 comes in byte addressing
    }*/

    // ASM version: set tilemap top entries (already inverted)
    u32* entries_ptr = (u32*) top_entries;
    u32 entry; // h2 in higher word, and val in lower word
    u16 offset;
    __asm volatile (
        ".set col, 0\n\t"
        ".rept %c[_PIXEL_COLUMNS]/2\n\t"
            "move.l  (%[entries_ptr])+,%[entry]\n\t" // val in higher word, h2 in lower word
            "move.w  %[entry],%[offset]\n\t" // copy h2 into offset
            "swap    %[entry]\n\t"
            "move.w  %[entry],col*2(%[fb],%[offset])\n\t" // *2 for byte addressing

            "move.l  (%[entries_ptr])+,%[entry]\n\t" // val in higher word, h2 in lower word
            "move.w  %[entry],%[offset]\n\t" // copy h2 into offset
            "addi.w  #%c[_VERTICAL_ROWS]*%c[_PLANE_COLUMNS]*2,%[offset]\n\t" // *2 for byte addressing
            "swap    %[entry]\n\t"
            "move.w  %[entry],col*2(%[fb],%[offset])\n\t" // *2 for byte addressing
            ".set col, col + 1\n\t"
        ".endr\n\t"
        : [entry] "=d" (entry), [offset] "=d" (offset)
        : [entries_ptr] "a" (entries_ptr), [fb] "a" (frame_buffer),
          [_PIXEL_COLUMNS] "i" (PIXEL_COLUMNS), [_PLANE_COLUMNS] "i" (PLANE_COLUMNS), [_VERTICAL_ROWS] "i" (VERTICAL_ROWS)
        :
    );

    #endif
}

void fb_mirror_planes_in_RAM ()
{
    u32 pA_bottom_half_start = FRAME_BUFFER_ADDRESS + ((VERTICAL_ROWS*PLANE_COLUMNS)/2)*2;
    u32 pA_top_half_end = FRAME_BUFFER_ADDRESS + ((VERTICAL_ROWS*PLANE_COLUMNS)/2 - PLANE_COLUMNS)*2;
    copy_bottom_half_into_top_half(pA_bottom_half_start, pA_top_half_end);

    u32 pB_bottom_half_start = FRAME_BUFFER_ADDRESS + ((VERTICAL_ROWS*PLANE_COLUMNS) + (VERTICAL_ROWS*PLANE_COLUMNS)/2)*2;
    u32 pB_top_half_end = FRAME_BUFFER_ADDRESS + ((VERTICAL_ROWS*PLANE_COLUMNS) + (VERTICAL_ROWS*PLANE_COLUMNS)/2 - PLANE_COLUMNS)*2;
    copy_bottom_half_into_top_half(pB_bottom_half_start, pB_top_half_end);

    copy_top_entries_in_RAM();
}

#define doDMA_VRAM_COPY_fixed_args(ctrl_port,fromAddr,cmdAddr,len) \
    __asm volatile ( \
        /* Setup DMA length (in long word here) */ \
        "move.l  %[_len_low_high],(%[_ctrl_port])\n\t" /* *((vu32*) VDP_CTRL_PORT) = ((0x9300 | (u8)len) << 16) | (0x9400 | (u8)(len >> 8)); */ \
        /* Setup DMA address low and mid */ \
        "move.l  %[_addr_low_mid],(%[_ctrl_port])\n\t" /* *((vu32*) VDP_CTRL_PORT) = ((0x9500 | ((fromAddr >> 1) & 0xff)) << 16) | (0x9600 | ((fromAddr >> 9) & 0xff)); */ \
        /* Setup DMA address high */ \
        "move.w  %[_addr_high],(%[_ctrl_port])\n\t" /* *((vu32*) VDP_CTRL_PORT) = 0x97C0; // VRAM COPY operation */ \
        /* Trigger DMA */ \
        "move.l  %[_cmdAddr],(%[_ctrl_port])\n\t" /* *((vu32*) VDP_CTRL_PORT) = cmdAddr; */ \
        : [_ctrl_port] "+a" (ctrl_port) \
        : [_len_low_high] "i" (((0x9300 | ( (len) & 0xff)) << 16) | (0x9400 | (((len) >> 8) & 0xff)) ), \
          [_addr_low_mid] "i" (((0x9500 | ( (fromAddr) & 0xff)) << 16) | (0x9600 | (((fromAddr) >> 8) & 0xff)) ), \
          [_addr_high] "i" (0x97C0), /* VRAM COPY operation */ \
          /* If you want to add VDP stepping then use: addr_high_step = (0x97C0 << 16) | (0x8F00 | ((step) & 0xff)); \ */ \
          [_cmdAddr] "i" ((cmdAddr)) \
        : \
    )

static void clear_buffer_row (u32* ptr)
{
    // C version
    /*u32 zero = 0;
    #pragma GCC unroll 40 // Always set the max number since it does not accept defines
    for (u8 i = 0; i < TILEMAP_COLUMNS/2; ++i) { // /2 because we are using a u32* pointer, so updating long words
        *ptr++ = zero;
    }*/

    // ASM version
    u32 zero = 0;
    u16 countDbra = (TILEMAP_COLUMNS/2)/2 - 1; // /2 because we are using a u32* pointer, so updating long words, and additional /2 for the 2 move.l
    __asm volatile (
        "1:\n\t"
        "move.l  %2,(%0)+\n\t"
        "move.l  %2,(%0)+\n\t"
        "dbra    %1,1b\n"
        : "+a" (ptr), "+d" (countDbra)
        : "d" (zero)
        :
    );
}

void fb_mirror_planes_in_VRAM ()
{
    // Mirror bottom half Plane A and B region into top half Plane A and B
    // Arguments must be in byte addressing mode

    // C version
    /*VDP_setAutoInc(1);
    #pragma GCC unroll 24 // Always set the max number since it does not accept defines
    for (u8 i=0; i < VERTICAL_ROWS/2; ++i) {
        DMA_doVRamCopy(PA_ADDR + HALF_PLANE_ADDR_OFFSET_BYTES + i*PLANE_COLUMNS*2, PA_ADDR + HALF_PLANE_ADDR_OFFSET_BYTES - i*PLANE_COLUMNS*2, TILEMAP_COLUMNS*2, -1);
        clear_buffer_row(pA_bottom_half_start);
        pA_bottom_half_start += (PLANE_COLUMNS)/2; // jump into next row
        while (GET_VDP_STATUS(VDP_DMABUSY_FLAG)); // wait DMA completion

        DMA_doVRamCopy(PB_ADDR + HALF_PLANE_ADDR_OFFSET_BYTES + i*PLANE_COLUMNS*2, PB_ADDR + HALF_PLANE_ADDR_OFFSET_BYTES - i*PLANE_COLUMNS*2, TILEMAP_COLUMNS*2, -1);
        clear_buffer_row(pB_bottom_half_start);
        pB_bottom_half_start += (PLANE_COLUMNS)/2; // jump into next row
        while(GET_VDP_STATUS(VDP_DMABUSY_FLAG)); // wait DMA completion
    }
    VDP_setAutoInc(2);*/

    // ASM version
    u32* pA_bottom_half_start = (u32*)(FRAME_BUFFER_ADDRESS + ((VERTICAL_ROWS*PLANE_COLUMNS)/2)*2);
    u32* pB_bottom_half_start = (u32*)(FRAME_BUFFER_ADDRESS + ((VERTICAL_ROWS*PLANE_COLUMNS) + (VERTICAL_ROWS*PLANE_COLUMNS)/2)*2);
    vu32* vdpCtrl_ptr_l = (vu32*) VDP_CTRL_PORT;
    *(vu16*)vdpCtrl_ptr_l = 0x8F00 | 1; // Set VDP stepping to 1

    #pragma GCC unroll 24 // Always set the max number since it does not accept defines
    for (u8 i=0; i < VERTICAL_ROWS/2; ++i) {
        doDMA_VRAM_COPY_fixed_args(vdpCtrl_ptr_l, PA_ADDR + HALF_PLANE_ADDR_OFFSET_BYTES + i*PLANE_COLUMNS*2, 
            VDP_DMA_VRAMCOPY_ADDR(PA_ADDR + HALF_PLANE_ADDR_OFFSET_BYTES - i*PLANE_COLUMNS*2), TILEMAP_COLUMNS*2);
        clear_buffer_row(pA_bottom_half_start);
        pA_bottom_half_start += (PLANE_COLUMNS)/2; // jump into next row. Is /2 because we are manipulating a u32* pointer
        while (GET_VDP_STATUS(VDP_DMABUSY_FLAG)); // wait DMA completion

        doDMA_VRAM_COPY_fixed_args(vdpCtrl_ptr_l, PB_ADDR + HALF_PLANE_ADDR_OFFSET_BYTES + i*PLANE_COLUMNS*2, 
            VDP_DMA_VRAMCOPY_ADDR(PB_ADDR + HALF_PLANE_ADDR_OFFSET_BYTES - i*PLANE_COLUMNS*2), TILEMAP_COLUMNS*2);
        clear_buffer_row(pB_bottom_half_start);
        pB_bottom_half_start += (PLANE_COLUMNS)/2; // jump into next row. Is /2 because we are manipulating a u32* pointer
        while(GET_VDP_STATUS(VDP_DMABUSY_FLAG)); // wait DMA completion
    }

    *(vu16*)vdpCtrl_ptr_l = 0x8F00 | 2; // Set VDP stepping back to 2
}

void fb_copy_top_entries_in_VRAM ()
{
    #if RENDER_MIRROR_PLANES_USING_VDP_VRAM

    vu32* vdpCtrl_ptr_l = (vu32*) VDP_CTRL_PORT;
    vu16* vdpData_ptr_w = (vu16*) VDP_DATA_PORT;

    // C version: set tilemap top entries (already inverted)
    u16* entries_ptr = top_entries;
    #pragma GCC unroll 80 // Always set the max number since it does not accept defines
    for (u8 i=0; i < PIXEL_COLUMNS/2; ++i) {
        u16 val = (*entries_ptr++);
        u16 h2 = (*entries_ptr++);
        *vdpCtrl_ptr_l = VDP_WRITE_VRAM_ADDR(PA_ADDR + 2*i + h2); // h2 is already in byte addressing
        *vdpData_ptr_w = val;
        val = (*entries_ptr++);
        h2 = (*entries_ptr++);
        *vdpCtrl_ptr_l = VDP_WRITE_VRAM_ADDR(PB_ADDR + 2*i + h2); // h2 is already in byte addressing
        *vdpData_ptr_w = val;
    }

    // ASM version: set tilemap top entries (already inverted)
    // TODO: use same snippet than in copy_top_entries_in_RAM() but replacing %[fb] by ctrl_port and data_port

    #endif
}