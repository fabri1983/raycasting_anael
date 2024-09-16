#include <types.h>
#include <vdp_tile.h>
#include "process_column.h"
#include "frame_buffer.h"
#include "consts.h"
#include "utils.h"
#include "map_matrix.h"
#include "write_vline.h"
#include "tab_wall_div.h"
#include "tab_color_d8.h"
#include "tab_color_d8_with_pal.h"
#include "tab_mulu_dist_div256.h"

FORCE_INLINE void process_column (u8 column, u16** delta_a_ptr, u16 a, u16 posX, u16 posY, u16 sideDistX_l0, u16 sideDistX_l1, u16 sideDistY_l0, u16 sideDistY_l1) {

#if USE_TAB_DELTAS_3 & !SHOW_TEXCOORD
	u16 deltaDistX = *(*delta_a_ptr)++; // value up to 65536-1
	u16 deltaDistY = *(*delta_a_ptr)++; // value up to 65536-1
	u16 rayDir = *(*delta_a_ptr)++; // jump block size multiple of 12

	u16 sideDistX, sideDistY; // effective value goes up to 4096-1 once in the stepping loop due to condition control
	s16 stepX, stepY, stepYMS;

	// rayDix < 0 and rayDirY < 0
	if (rayDir == 0) {
		stepX = -1;
		stepY = -1;
		stepYMS = -MAP_SIZE;
		sideDistX = sideDistX_l0;
		sideDistY = sideDistY_l0;
	}
	// rayDix < 0 and rayDirY > 0
	else if (rayDir == 12) {
		stepX = -1;
		stepY = 1;
		stepYMS = MAP_SIZE;
		sideDistX = sideDistX_l0;
		sideDistY = sideDistY_l1;
	}
	// rayDix > 0 and rayDirY < 0
	else if (rayDir == 24) {
		stepX = 1;
		stepY = -1;
		stepYMS = -MAP_SIZE;
		sideDistX = sideDistX_l1;
		sideDistY = sideDistY_l0;
	}
	// rayDix > 0 and rayDirY > 0
	else {
		stepX = 1;
		stepY = 1;
		stepYMS = MAP_SIZE;
		sideDistX = sideDistX_l1;
		sideDistY = sideDistY_l1;
	}

	#if USE_TAB_MULU_DIST_DIV256
	sideDistX = tab_mulu_dist_div256[sideDistX][(a+column)];
	sideDistY = tab_mulu_dist_div256[sideDistY][(a+column)];
	#else
	sideDistX = mulu_shft_FS(sideDistX, deltaDistX); //(u16)(mulu(sideDistX, deltaDistX) >> FS);
	sideDistY = mulu_shft_FS(sideDistY, deltaDistY); //(u16)(mulu(sideDistY, deltaDistY) >> FS);
	#endif

	/*
    __asm volatile (
		"jmp     (%%pc,%[_rayDir].w)\n\t" // rayDir comes with the offset multiple of block size

        "0:\n\t"  // rayDix < 0 and rayDirY < 0
        "moveq   #-1, %[_sideDistX_l1]\n\t" // stepX
        "moveq   #-1, %[_sideDistY_l1]\n\t" // stepY
        "moveq   #-%c[_MAP_SIZE], %[_rayDir]\n\t" // stepYMS
        "bra.s   4f\n\t"
		"nop\n\t"
		"nop\n\t"

        "1:\n\t"  // rayDix < 0 and rayDirY > 0
        "move.w  %[_sideDistY_l1], %[_sideDistY_l0]\n\t"
		"moveq   #-1, %[_sideDistX_l1]\n\t" // stepX
        "moveq   #1, %[_sideDistY_l1]\n\t" // stepY
        "moveq   #%c[_MAP_SIZE], %[_rayDir]\n\t" // stepYMS
        "bra.s   4f\n\t"
		"nop\n\t"

        "2:\n\t"  // rayDix > 0 and rayDirY < 0
        "move.w  %[_sideDistX_l1], %[_sideDistX_l0]\n\t"
		"moveq   #1, %[_sideDistX_l1]\n\t" // stepX
        "moveq   #-1, %[_sideDistY_l1]\n\t" // stepY
        "moveq   #-%c[_MAP_SIZE], %[_rayDir]\n\t" // stepYMS
        "bra.s   4f\n\t"
		"nop\n\t"

        "3:\n\t"  // rayDix > 0 and rayDirY > 0
        "move.w  %[_sideDistX_l1], %[_sideDistX_l0]\n\t" // sideDistX
        "move.w  %[_sideDistY_l1], %[_sideDistY_l0]\n\t" // sideDisty
		"moveq   #1, %[_sideDistX_l1]\n\t" // stepX
        "moveq   #1, %[_sideDistY_l1]\n\t" // stepY
		"moveq   #%c[_MAP_SIZE], %[_rayDir]\n\t" // stepYMS

        "4:\n\t"
		#if USE_TAB_MULU_DIST_DIV256
		Need to complete this;
		#else
		"mulu.w  %[_deltaDistX], %[_sideDistX_l0]\n\t" // sideDistX
        "mulu.w  %[_deltaDistY], %[_sideDistY_l0]\n\t" // sideDistY
		"lsr.l   %[_FS], %[_sideDistX_l0]\n\t" // sideDistX >> FS
        "lsr.l   %[_FS], %[_sideDistY_l0]\n\t" // sideDistY >> FS
		#endif
        : [_sideDistX_l0] "+d" (sideDistX_l0), [_sideDistY_l0] "+d" (sideDistY_l0),
          [_sideDistX_l1] "+d" (sideDistX_l1), [_sideDistY_l1] "+d" (sideDistY_l1),
		  [_rayDir] "+d" (rayDir)
        : [_deltaDistX] "d" (deltaDistX), [_deltaDistY] "d" (deltaDistY),
          [_MAP_SIZE] "i" (MAP_SIZE), [_FS] "i" (FS)
        : "cc"
    );

	// Previous inlined asm block stored results in the variables used as input/output
	sideDistX = sideDistX_l0;
    sideDistY = sideDistY_l0;
	stepX = sideDistX_l1;
    stepY = sideDistY_l1;
    stepYMS = rayDir;
	*/
#else
	u16 deltaDistX = *(*delta_a_ptr)++; // value up to 65536-1
	u16 deltaDistY = *(*delta_a_ptr)++; // value up to 65536-1
	const s16 rayDirX = (s16) *(*delta_a_ptr)++;
	const s16 rayDirY = (s16) *(*delta_a_ptr)++;

	u16 sideDistX, sideDistY; // effective value goes up to 4096-1 once in the stepping loop due to condition control
	s16 stepX, stepY, stepYMS;

	if (rayDirX < 0) {
		stepX = -1;
		sideDistX = sideDistX_l0;
	}
	else {
		stepX = 1;
		sideDistX = sideDistX_l1;
	}

	if (rayDirY < 0) {
		stepY = -1;
		stepYMS = -MAP_SIZE;
		sideDistY = sideDistY_l0;
	}
	else {
		stepY = 1;
		stepYMS = MAP_SIZE;
		sideDistY = sideDistY_l1;
	}

	#if USE_TAB_MULU_DIST_DIV256
	sideDistX = tab_mulu_dist_div256[sideDistX][(a+column)];
	sideDistY = tab_mulu_dist_div256[sideDistY][(a+column)];
	#else
	sideDistX = mulu_shft_FS(sideDistX, deltaDistX); //(u16)(mulu(sideDistX, deltaDistX) >> FS);
	sideDistY = mulu_shft_FS(sideDistY, deltaDistY); //(u16)(mulu(sideDistY, deltaDistY) >> FS);
	#endif
#endif

	u16 mapX = posX / FP;
	u16 mapY = posY / FP;
	const u8 *map_ptr = &map[mapY][mapX];

	for (u16 n = 0; n < STEP_COUNT_LOOP; ++n) {

		// side X
		if (sideDistX < sideDistY) {

			mapX += stepX;
			map_ptr += stepX;

			u16 hit = *map_ptr; // map[mapY][mapX];
			if (hit) {

				u16 h2 = tab_wall_div[sideDistX];

				#if SHOW_TEXCOORD
				u16 d = (7 - min(7, sideDistX / FP));
				u16 wallY = posY + (muls(sideDistX, rayDirY) >> FS);
				//wallY = ((wallY * 8) >> FS) & 7; // faster
				wallY = max((wallY - mapY*FP) * 8 / FP, 0); // cleaner
				u16 color = ((0 + 2*(mapY&1)) << TILE_ATTR_PALETTE_SFT) + 1 + min(d, wallY)*8;
				#else
				u16 d8 = tab_color_d8_x[sideDistX];
				u16 color;
				if (mapY&1) color = d8 | (PAL2 << TILE_ATTR_PALETTE_SFT);
				else color = d8 | (PAL0 << TILE_ATTR_PALETTE_SFT);
				// Next try out is indeed slower than all previous calculations
				// u16 cIdx = (FP * (STEP_COUNT + 1)) * (mapY&1) + sideDistX;
				// u16 color = tab_color_d8_x_with_pal[cIdx];
				#endif

				u16 *column_ptr = frame_buffer + frame_buffer_column[column];
				write_vline(column_ptr, h2, color);
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

				u16 h2 = tab_wall_div[sideDistY];

				#if SHOW_TEXCOORD
				u16 d = (7 - min(7, sideDistY / FP));
				u16 wallX = posX + (muls(sideDistY, rayDirX) >> FS);
				//wallX = ((wallX * 8) >> FS) & 7; // faster
				wallX = max((wallX - mapX*FP) * 8 / FP, 0); // cleaner
				u16 color = ((1 + 2*(mapX&1)) << TILE_ATTR_PALETTE_SFT) + 1 + min(d, wallX)*8;
				#else
				u16 d8 = tab_color_d8_y[sideDistY];
				u16 color;
				if (mapX&1) color = d8 | (PAL3 << TILE_ATTR_PALETTE_SFT);
				else color = d8 |= (PAL1 << TILE_ATTR_PALETTE_SFT);
				// Next try out is indeed slower than all previous calculations
				// u16 cIdx = (FP * (STEP_COUNT + 1)) * (mapX&1) + sideDistY;
				// u16 color = tab_color_d8_y_with_pal[cIdx];
				#endif

				u16 *column_ptr = frame_buffer + frame_buffer_column[column];
				write_vline(column_ptr, h2, color);
				break;
			}

			sideDistY += deltaDistY;
		}
	}
}