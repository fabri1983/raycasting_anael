#ifndef _TITLE_H_
#define _TITLE_H_

#define TITLE_LOGO_HEIGHT_TILES (152/8)
#define TITLE_LOGO_WIDTH_TILES (256/8)
#define TITLE_LOGO_X_POS_TILES 4 // Plane X position (in tile)
#define TITLE_LOGO_Y_POS_TILES 0 // Plane Y position (in tile)

#define TITLE_256C_STRIPS_COUNT 28
#define TITLE_256C_WIDTH 320 // In pixels
#define TITLE_256C_HEIGHT 224 // In pixels
#define TITLE_256C_STRIP_HEIGHT 8
#define TITLE_256C_COLORS_PER_STRIP 32
// in case you were to split any calculation over the colors of strip by an odd divisor
#define TITLE_256C_COLORS_PER_STRIP_REMAINDER(n) (TITLE_256C_COLORS_PER_STRIP % n)

#define MELTING_OFFSET_STEPPING 4

#define PLANE_B_ADDR 0xC000 // SGDK's default Plane B location for a screen size 64*32
#define PLANE_A_ADDR 0xE000 // SGDK's default Plane A location for a screen size 64*32

#include <memory_base.h>

// Interleaved half Tilemap A fixed address (in bytes), just before the end of the heap.
#define TITLE_HALVED_TILEMAP_A_ADDRESS (MEMORY_HIGH - ((TITLE_256C_HEIGHT/8)*((TITLE_256C_WIDTH/8)/2)*2))
// Interleaved half Tilemap B fixed address (in bytes), just before the end of the first halved tilemap.
#define TITLE_HALVED_TILEMAP_B_ADDRESS (TITLE_HALVED_TILEMAP_A_ADDRESS - ((TITLE_256C_HEIGHT/8)*((TITLE_256C_WIDTH/8)/2)*2))

// Tilemap fixed address (in bytes), just before the end of the heap.
#define TITLE_FULL_TILEMAP_ADDRESS (MEMORY_HIGH - ((TITLE_256C_HEIGHT/8)*(TITLE_256C_WIDTH/8)*2))
// Tilemap fixed address (in bytes), just before the end of the previous tilemap.
#define TITLE_MELTDOWN_TILEMAP_ADDRESS (TITLE_FULL_TILEMAP_ADDRESS - ((TITLE_256C_HEIGHT/8)*(TITLE_256C_WIDTH/8)*2))

void title_vscroll_2_planes_show ();

void title_vscroll_1_plane_show ();

#endif // _TITLE_H_