#include "clear_buffer.h"
#include "utils.h"
#include "consts.h"

FORCE_INLINE void clear_buffer (u8* frame_buffer_ptr) {
#if PLANE_COLUMNS == 32
	// We need to clear all the tilemap
	__asm volatile (
		"    lea     %c[_TOTAL_BYTES](%%a0),%%a0\n"    // a0: buffer end
		// Save all registers (except scratch pad)
		//"    movem.l %%d2-%%d7/%%a2-%%a6,-(%%sp)\n"
		// Clear registers
		"    moveq   #0,%%d0\n" // tile index 0
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
		: [_TOTAL_BYTES] "i" ((VERTICAL_COLUMNS*PLANE_COLUMNS*2 - (PLANE_COLUMNS-TILEMAP_COLUMNS))*sizeof(u16))
		:
	);
#elif PLANE_COLUMNS == 64
	// We need to clear only first PIXEL_COLUMNS columns of each row from the tilemap
	__asm volatile (
		".set end_offset,(%c[_TOTAL_BYTES] - %c[_NON_DISPLAYED_BYTES])\n"
		"    lea     end_offset(%%a0),%%a0\n"    // a0: buffer end minus the non displayed data
		// Save all registers (except scratch pad)
		//"    movem.l %%d2-%%d7/%%a2-%%a6,-(%%sp)\n"
		// Clear registers
		"    moveq   #0,%%d0\n" // tile index 0
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
		".rept (%c[_VERTICAL_COLUMNS]*2 - 1)\n"
		// Clear all the bytes of current row by using 14 registers with long word access.
		"    .rept (%c[_TILEMAP_COLUMNS_BYTES] / (14*4))\n"
    	"    movem.l %%d0-%%d7/%%a1-%%a6,-(%%a0)\n"
    	"    .endr\n"
		// NOTE: if reminder from the division isn't 0 you need to add the missing operations.
		"    .rept ((%c[_TILEMAP_COLUMNS_BYTES] %% (14*4)) / 4)\n"
		"    move.l  %%d0,-(%%a0)\n"
		"    .endr\n"
		// Skip the non displayed data
		"    lea     -%c[_NON_DISPLAYED_BYTES](%%a0),%%a0\n"
		".endr\n"
		// One iteration more without the last lea instruction
		"    .rept (%c[_TILEMAP_COLUMNS_BYTES] / (14*4))\n"
    	"    movem.l %%d0-%%d7/%%a1-%%a6,-(%%a0)\n"
    	"    .endr\n"
		"    .rept ((%c[_TILEMAP_COLUMNS_BYTES] %% (14*4)) / 4)\n"
		"    move.l  %%d0,-(%%a0)\n"
		"    .endr\n"
		// Restore all saved registers
		//"    movem.l (%%sp)+,%%d2-%%d7/%%a2-%%a6\n"
		: "+a" (frame_buffer_ptr)
		: [_TOTAL_BYTES] "i" ((VERTICAL_COLUMNS*PLANE_COLUMNS*2)*sizeof(u16)), [_TILEMAP_COLUMNS_BYTES] "i" (TILEMAP_COLUMNS*sizeof(u16)), 
		[_NON_DISPLAYED_BYTES] "i" ((PLANE_COLUMNS-TILEMAP_COLUMNS)*sizeof(u16)), [_VERTICAL_COLUMNS] "i" (VERTICAL_COLUMNS)
		:
	);
#endif
}