#include "clear_buffer.h"
#include "utils.h"
#include "consts.h"
#include <tools.h>

#if USE_CLEAR_FRAMEBUFFER_WITH_SP == FALSE

FORCE_INLINE void clear_buffer (u16* frame_buffer_ptr) {
#if PLANE_COLUMNS == 32
	// We need to clear all the tilemap
	__asm volatile (
		"    lea     %c[TOTAL_BYTES](%%a0),%%a0\n" // frame_buffer_ptr + TOTAL_BYTES: buffer end
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
		"    .rept (%c[TOTAL_BYTES] / (14*4))\n"
    	"    movem.l %%d0-%%d7/%%a1-%%a6,-(%%a0)\n"
    	"    .endr\n"
		// NOTE: if reminder from the division isn't 0 you need to add the missing operations.
		"    .rept ((%c[TOTAL_BYTES] %% (14*4)) / 4)\n"
		"    move.l  %%d0,-(%%a0)\n"
		"    .endr\n"
		// Restore all saved registers
		//"    movem.l (%%sp)+,%%d2-%%d7/%%a2-%%a6\n"
		:
		: "a" (frame_buffer_ptr), 
		  [TOTAL_BYTES] "i" ((VERTICAL_COLUMNS*PLANE_COLUMNS*2 - (PLANE_COLUMNS-TILEMAP_COLUMNS))*sizeof(u16))
		:
	);
#elif PLANE_COLUMNS == 64
	// We need to clear only first PIXEL_COLUMNS columns of each row from the tilemap
	// 9842 cycles according Blastem cycle counter
	__asm volatile (
		// Makes SP points to the memory location of the framebuffer's end 
		"    lea     %c[TOTAL_BYTES](%%a0),%%a0\n" // frame_buffer_ptr + TOTAL_BYTES: buffer end
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
		// One iteration more without the last lea instruction
		"    .rept (%c[TILEMAP_COLUMNS_BYTES] / (14*4))\n"
    	"    movem.l %%d0-%%d7/%%a1-%%a6,-(%%a0)\n"
    	"    .endr\n"
		"    .rept ((%c[TILEMAP_COLUMNS_BYTES] %% (14*4)) / 4)\n"
		"    move.l  %%d0,-(%%a0)\n"
		"    .endr\n"
		// Restore all saved registers
		//"    movem.l (%%sp)+,%%d2-%%d7/%%a2-%%a6\n"
		:
		: "a" (frame_buffer_ptr), 
		  [TOTAL_BYTES] "i" ((VERTICAL_COLUMNS*PLANE_COLUMNS*2 - (PLANE_COLUMNS-TILEMAP_COLUMNS))*sizeof(u16)), 
		  [TILEMAP_COLUMNS_BYTES] "i" (TILEMAP_COLUMNS*sizeof(u16)), [_VERTICAL_COLUMNS] "i" (VERTICAL_COLUMNS), 
		  [NON_DISPLAYED_BYTES_PER_ROW] "i" ((PLANE_COLUMNS-TILEMAP_COLUMNS)*sizeof(u16))
		:
	);
#endif
}

#endif

#if USE_CLEAR_FRAMEBUFFER_WITH_SP

// *2 because we have 2 planes. /2 because every time we read from the inverted buffer we do it by long word (twice u16).
#define INVERTED_BUFFER_SIZE ((VERTICAL_COLUMNS*TILEMAP_COLUMNS*2)/2)

u32 spBackup[1] = {0};

FORCE_INLINE void clear_buffer_sp (u16* frame_buffer_ptr) {
#if PLANE_COLUMNS == 32
	// We need to clear all the tilemap
	__asm volatile (
		// Save current SP value in a 4 bytes region dedicated for it
		"    move.l  %%sp,%[mem_spBackup]\n"
		// Makes SP points to the memory location of the framebuffer's end 
		"    lea     %c[TOTAL_BYTES](%%a0),%%sp\n" // frame_buffer_ptr + TOTAL_BYTES: buffer end
		// Clear registers
		"    moveq   #0,%%d0\n" // tile index 0
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
		// Clear all the bytes by using 15 registers with long word (4 bytes) access.
		"    .rept (%c[TOTAL_BYTES] / (15*4))\n"
    	"    movem.l %%d0-%%d7/%%a0-%%a6,-(%%sp)\n"
    	"    .endr\n"
		// NOTE: if reminder from the division isn't 0 you need to add the missing operations.
		"    .rept ((%c[TOTAL_BYTES] %% (15*4)) / 4)\n"
		"    move.l  %%d0,-(%%sp)\n"
		"    .endr\n"
		// Restore SP
		"    move.l  %[mem_spBackup],%%sp\n"
		: [mem_spBackup] "+m" (spBackup)
		: "a" (frame_buffer_ptr),
		  [TOTAL_BYTES] "i" ((VERTICAL_COLUMNS*PLANE_COLUMNS*2 - (PLANE_COLUMNS-TILEMAP_COLUMNS))*sizeof(u16))
		:
	);
#elif PLANE_COLUMNS == 64
	// We need to clear the tilemap following the addresses loaded into the SP
	// 9693 cycles according Blastem cycle counter
	__asm volatile (
		// Save current SP value in a 4 bytes region dedicated for it
		"    move.l  %%sp,%[mem_spBackup]\n"
		// Makes SP points to the memory location of the framebuffer's end 
		"    lea     %c[TOTAL_BYTES](%%a0),%%sp\n" // frame_buffer_ptr + TOTAL_BYTES: buffer end
		// Clear registers
		"    moveq   #0,%%d0\n" // tile index 0
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
		".rept (%c[_VERTICAL_COLUMNS]*2 - 1)\n"
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
		// One iteration more without the last lea instruction
		"    .rept (%c[TILEMAP_COLUMNS_BYTES] / (15*4))\n"
    	"    movem.l %%d0-%%d7/%%a0-%%a6,-(%%sp)\n"
    	"    .endr\n"
		"    .rept ((%c[TILEMAP_COLUMNS_BYTES] %% (15*4)) / 4)\n"
		"    move.l  %%d0,-(%%sp)\n"
		"    .endr\n"
		// Restore SP
		"    move.l  %[mem_spBackup],%%sp\n"
		: [mem_spBackup] "+m" (spBackup)
		: "a" (frame_buffer_ptr), 
		  [TOTAL_BYTES] "i" ((VERTICAL_COLUMNS*PLANE_COLUMNS*2 - (PLANE_COLUMNS-TILEMAP_COLUMNS))*sizeof(u16)), 
		  [TILEMAP_COLUMNS_BYTES] "i" (TILEMAP_COLUMNS*sizeof(u16)), [_VERTICAL_COLUMNS] "i" (VERTICAL_COLUMNS), 
		  [NON_DISPLAYED_BYTES_PER_ROW] "i" ((PLANE_COLUMNS-TILEMAP_COLUMNS)*sizeof(u16))
		:
	);
#endif
}

#endif