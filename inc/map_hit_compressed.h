#ifndef _TAB_MAP_HIT_H_
#define _TAB_MAP_HIT_H_

#include <types.h>
#include "consts.h"

/**
 * First you need to generate the hit map with script tab_map_hit_generator.js. Check correct values of constants before script execution.
 * As per last script execution:
 *   min calculated index:  174080  <-- define in consts.h as MAP_HIT_MIN_CALCULATED_INDEX
 *   max calculated index: 2447359
 *   final array size: (2447359 - 174080) + 1 = 2273280 elems
 * 
 * Then run script map_hit_block_turbopfor_16_compressor.js. Check correct values of constants before script execution.
 *
 * In order to correctly access the table you need to calculate the offsets and also use the MAP_HIT_MIN_CALCULATED_INDEX:
 *   u32 calculatedIndex = (((mapX * MAP_SIZE + mapY) * (1024/(1024/AP)) + a) * PIXEL_COLUMNS + column) - MAP_HIT_MIN_CALCULATED_INDEX;
 *   u16 value = map_hit_decompressAt(calculatedIndex);
 * Then decompose the value:
 *   u16 hit_mapXY = (value >> MAP_HIT_OFFSET_MAPXY) & MAP_HIT_MASK_MAPXY;
 *   u16 hit_mapXY = (value >> MAP_HIT_OFFSET_SIDEDISTXY) & MAP_HIT_MASK_SIDEDISTXY;
 * 
 * How to tell which side X or Y does hit_mapXY go?
 *   if (hit_mapXY < sideDistY)
 *       ...
 *   else
 *       ...
 */

void map_hit_reset_vars ();
void map_hit_setRow (u16 posX, u16 posY, u16 a);
void map_hit_setIndexForStartingColumn (u16 column);
void map_hit_incrementColumn ();
u16 map_hit_decompressAt ();

#endif // _TAB_MAP_HIT_H_