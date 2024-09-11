#ifndef _TAB_MULU_DIST_DIV_256_H_
#define _TAB_MULU_DIST_DIV_256_H_

#include <types.h>
#include "consts.h"

/*
    This table is created out of mulu(sideDistX_l0|l1, deltaDistX) >> FS (and the same for Y).
    sideDistX_l0|l1 has max value 256. Same apply for sideDistY_l0|l1.
    deltaDistX is obtained from a = angle/(1024/AP), ie 128 posible rotation values for each X position in the map.
    "a" is then used to get the tab_deltas[] entry which gives us access to the PIXEL_COLUMNS values for deltaDistX and deltaDistY.
    So (1024/(1024/AP))=128, then 128*PIXEL_COLUMNS possible entries to get into tab_deltas[], then values for deltaDistX and deltaDistY 
    are obtained from x=0 up to x=PIXEL_COLUMNS-1.
 */

// Table constent is generated with scripts tab_mulu_dist_div256_generator.js and tab_mulu_dist_div256_transformer.js.
const u16 tab_mulu_dist_div256[256+1][(1024/(1024/AP))*PIXEL_COLUMNS] = {
0
};

#endif // _TAB_MULU_DIST_DIV_256_H_