#ifndef _TAB_MAP_HIT_H_
#define _TAB_MAP_HIT_H_

#include <types.h>
#include "consts.h"

/**
 * Table constent is generated with script tab_map_hit_generator.js.
 * 
 * Final size is also calculated by the script. So you can use that size or leave the brackets empty.
 * Max theorically size is: MAP_SIZE * MAP_SIZE * (1024/(1024/AP)) * PIXEL_COLUMNS.
 * 
 * In order to correctly access the table you need to calculate the offsets and also use the minIndex:
 *   u16 calculatedIndex = ((mapX * MAP_SIZE + mapY) * (1024/(1024/AP)) + a) * PIXEL_COLUMNS + column;
 *   u16 value = tab_map_hit[calculatedIndex - minIndex];
 * Then decompose the value:
 *   u16 mapXY = (value >> MAP_HIT_OFFSET_MAPXY) & MAP_HIT_MASK_MAPXY;
 *   u16 sideDistXY = (value >> MAP_HIT_OFFSET_SIDEDISTXY) & MAP_HIT_MASK_SIDEDISTXY;
 * How to tell which side X or Y is the decomposed value for?
 *   if (stored_sideDistX < sideDistY)
 *       work here
 *   else
 *       work here
 */
const u16 tab_map_hit[] = {
0
};

#endif // _TAB_MAP_HIT_H_