#ifndef _HUD_CONSTS_H_
#define _HUD_CONSTS_H_

#include "consts.h"

#define HUD_HEIGHT 4 // In tiles unit

#define HINT_SCANLINE_START_PAL_SWAP (224 - (HUD_HEIGHT*8)) // This formula is not 0-based.
#define HINT_SCANLINE_MID_SCREEN (HINT_SCANLINE_START_PAL_SWAP/2) // Eg: ((224-32)-2)/2 = 96. This formula is not 0-based.

#define HUD_RELOAD_WEAPON_PALS_AT_HINT F
#define HUD_RELOAD_WEAPON_PALS_AT_VINT T

#define HUD_BASE_PAL PAL2 // If change this value you'll have to update map_base parameter in resource file
#define HUD_USED_PALS 2
// This value is the parameter map_base that has to go in the resource file if you modify one of the above. Currently is 16456.
#define HUD_BASE_TILE_ATTRIB TILE_ATTR_FULL(HUD_BASE_PAL, 1, FALSE, FALSE, VRAM_INDEX_AFTER_TILES)

#define HUD_TILEMAP_COMPRESSED T // If TRUE then we decompress it into a buffer. Otherwise we use the uncompressed data from ROM.

// X tile position in Window Plane
#define HUD_XP 0
// Y tile position in Window Plane depending on PLANE_COLUMNS
#define HUD_YP (PLANE_COLUMNS == 64 ? (224/8 - HUD_HEIGHT) : (224/8 - HUD_HEIGHT)/2)

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