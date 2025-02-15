#ifndef _HUD_CONSTS_H_
#define _HUD_CONSTS_H_

#include "consts.h"

#define HUD_HEIGHT 4 // In tiles unit

#define HUD_HINT_SCANLINE_START_PAL_SWAP (224-(HUD_HEIGHT*8))

#define HUD_RELOAD_OVERRIDEN_PALETTES_AT_HINT FALSE // If TRUE then reload happens at HInt, otherwise at VInt.

#define HUD_SET_FLOOR_AND_ROOF_COLORS_ON_HINT TRUE // If TRUE then it will change from floor to roof bg color at HInt. If FALSE then we use single bg color.

// ((224-32)-2)/2 = 95. Is -2 scanlines due to the setup of dma and its transfer time for the HUD palettes into VRAM.
#define HUD_HINT_SCANLINE_MID_SCREEN ((HUD_HINT_SCANLINE_START_PAL_SWAP-2)/2)

#define HUD_VRAM_START_INDEX (1 + 8*8 + 8*8) // If change this value you'll have to update map_base parameter in resource file
#define HUD_PAL PAL2 // If change this value you'll have to update map_base parameter in resource file
// This value is the parameter map_base that has to go in the resource file if you modify one of the above. Currently is 16456.
#define HUD_BASE_TILE_ATTRIB TILE_ATTR_FULL(HUD_PAL, 1, FALSE, FALSE, HUD_VRAM_START_INDEX)

#define HUD_TILEMAP_COMPRESSED TRUE // If TRUE then we decompress it into a buffer. Otherwise we use the uncompressed data from ROM.

#define HUD_XP 0
#define HUD_YP (PLANE_COLUMNS == 64 ? 24 : 12)

#define WEAPON_FIST 0
#define WEAPON_PISTOL 1
#define WEAPON_SHOTGUN 2
#define WEAPON_MACHINE_GUN 3
#define WEAPON_ROCKET 4
#define WEAPON_PLASMA 5
#define WEAPON_BFG 6
#define WEAPON_MAX_COUNT 7

#define KEY_NONE 0
#define KEY_CARD_BLUE 1
#define KEY_CARD_YELLOW 2
#define KEY_CARD_RED 3
#define KEY_SKULL_BLUE 4
#define KEY_SKULL_YELLOW 5
#define KEY_SKULL_RED 6

#define FACE_EXPRESSION_LEFT 0
#define FACE_EXPRESSION_CENTERED 1
#define FACE_EXPRESSION_RIGHT 2
#define FACE_EXPRESSION_SMILE 3
#define FACE_EXPRESSION_HIT 4

#define UPDATE_FLAG_AMMO 0
#define UPDATE_FLAG_HEALTH 1
#define UPDATE_FLAG_WEAPON 2
#define UPDATE_FLAG_ARMOR 3
#define UPDATE_FLAG_KEY 4
#define UPDATE_FLAG_FACE 5

#endif // _HUD_CONSTS_H_