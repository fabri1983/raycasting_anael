#ifndef _HUD_320_H_
#define _HUD_320_H_

#include <types.h>
#include "utils.h"

#define HUD_VRAM_START_INDEX 72
#define HUD_PAL PAL0
// This value is the one that has to go in the resources file
#define HUD_BASE_TILE_ATTRIB TILE_ATTR_FULL(HUD_PAL, 0, FALSE, FALSE, HUD_VRAM_START_INDEX)

#define HUD_IMAGE_W 42 // In tile units
#define HUD_IMAGE_H 20 // In tile units
#define HUD_TILEMAP_COMPRESSED TRUE // If TRUE then we decompress it into a buffer. Otherwise we use the data from ROM which saves some RAM.

#define HUD_BASE_XP 0
#define HUD_BASE_YP PLANE_COLUMNS == 64 ? 24 : 12

#define HUD_BG_X 0
#define HUD_BG_Y 0
#define HUD_BG_W TILEMAP_COLUMNS
#define HUD_BG_H 4
#define HUD_BG_XP 0
#define HUD_BG_YP 0

#define HUD_NUMS_X 0
#define HUD_NUMS_Y 4
#define HUD_NUMS_W 2
#define HUD_NUMS_H 3

#define HUD_AMMO_XP 0
#define HUD_AMMO_YP 0

#define HUD_HEALTH_XP 6
#define HUD_HEALTH_YP 0

#define HUD_ARMOR_XP 22
#define HUD_ARMOR_YP 0

#define WEAPON_PISTOL 2
#define WEAPON_SHOTGUN 3
#define WEAPON_MACHINE_GUN 4
#define WEAPON_ROCKET 5
#define WEAPON_PLASMA 6
#define WEAPON_BFG 7

#define HUD_WEAPON_X 0
#define HUD_WEAPON_Y 15
#define HUD_WEAPON_HIGH_XP 15
#define HUD_WEAPON_HIGH_YP 0
#define HUD_WEAPON_LOW_XP 13
#define HUD_WEAPON_LOW_YP 1

#define KEY_NONE 0
#define KEY_CARD_BLUE 1
#define KEY_CARD_YELLOW 2
#define KEY_CARD_RED 3
#define KEY_SKULL_BLUE 4
#define KEY_SKULL_YELLOW 5
#define KEY_SKULL_RED 6

#define HUD_KEY_X 0
#define HUD_KEY_Y 13
#define HUD_KEY_XP 29
#define HUD_KEY_YP 0

#define FACE_EXPRESSION_LEFT 0
#define FACE_EXPRESSION_CENTERED 1
#define FACE_EXPRESSION_RIGHT 2
#define FACE_EXPRESSION_SMILE 3
#define FACE_EXPRESSION_HIT 4

#define HUD_FACE_X 22
#define HUD_FACE_Y 4
#define HUD_FACE_DEAD_X 18
#define HUD_FACE_DEAD_Y 16
#define HUD_FACE_W 4
#define HUD_FACE_H 4
#define HUD_FACE_XP 18
#define HUD_FACE_YP 0

#define UPDATE_FLAG_AMMO 0
#define UPDATE_FLAG_HEALTH 1
#define UPDATE_FLAG_WEAPON 2
#define UPDATE_FLAG_ARMOR 3
#define UPDATE_FLAG_KEY 4
#define UPDATE_FLAG_FACE 5

typedef struct {
    u8 hundrs;
    u8 tens;
    u8 ones;
} Digits;

int loadInitialHUD (u16 currentTileIndex);

void resetAmmo ();
void setAmmo (u8 hundreds, u8 tens, u8 ones);
void resetHealth ();
void setHealth (u8 hundreds, u8 tens, u8 ones);
void resetArmor ();
void setArmor (u8 hundreds, u8 tens, u8 ones);
void resetWeapons ();
void addWeapon (u8 weapon);
void resetKeys ();
void addKey (u8 key);
void resetFaceExpression ();
void setFaceExpression (u8 expr, u8 timer);

bool isDead ();

void updateHUD ();

void prepareHUDPalAddresses ();

#define HINT_SCANLINE_CHANGE_BG_COLOR 95

HINTERRUPT_CALLBACK hIntCallbackHud ();

#endif // _HUD_320_H_