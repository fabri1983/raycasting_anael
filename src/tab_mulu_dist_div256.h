/*
    This table is created out of mulu(sideDistX_l0|l1, deltaDistX) >> FS (and the same for Y).
    sideDistX_l0|l1 has max value 256.
    deltaDistX is obtained from a = angle/(1024/AP), ie 128 posible rotation values for each X position in the map.
    "a" is then used to get the tab_deltas[] entry which gives us access to the 64 values for deltaDistX.
    So (1024/(1024/AP))=128, then 128*64 possible entries to get into tab_deltas[], then values for deltaDistX are obtained from x=0 up to x=64-1.
    I noticed if instead of dimension 128*64 we use 128*56 then the lose in values is barely noticable.
 */
static const u16 tab_mulu_dist_div256[256+1][(1024/(1024/AP) - 1)*56 + 64] = {
0
};

// static /*const*/ u16 tab_mulu_dist_y_div256[256+1][1024] = {
// 0
// };