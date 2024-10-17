#ifndef _TAB_MAP_HIT_H_
#define _TAB_MAP_HIT_H_

#include <types.h>
#include "consts.h"

/**
 * Table content is generated with script tab_map_hit_generator.js. Check correct values of constants before script execution.
 * 
 * Final size is also calculated by the script. So you can use that size or leave the brackets empty.
 * Max theorically size is: MAP_SIZE * MAP_SIZE * (1024/(1024/AP)) * PIXEL_COLUMNS.
 * But we are better off using the calculated size.
 * As per last script execution:
 *   min calculated index:  174080  <-- defined in consts.h as MAP_HIT_MIN_CALCULATED_INDEX
 *   max calculated index: 2447359
 *   final array size: (2447359 - 174080) + 1 = 2273280 elems
 * 
 * In order to correctly access the table you need to calculate the offsets and also use the MAP_HIT_MIN_CALCULATED_INDEX:
 *   u32 calculatedIndex = (((mapX * MAP_SIZE + mapY) * (1024/(1024/AP)) + a) * PIXEL_COLUMNS + column) - MAP_HIT_MIN_CALCULATED_INDEX;
 *   u16 value = tab_map_hit[calculatedIndex];
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
const u16 tab_map_hit[] = {
0
};

#endif // _TAB_MAP_HIT_H_