#ifndef _HUD_H_
#define _HUD_H_

#include <types.h>
#include <vdp_bg.h>

#define HUD_HINT_SCANLINE_START_PAL_SWAP (224-32)
#define HUD_RELOAD_OVERRIDEN_PALETTES_AT_HINT TRUE // If TRUE then reload happens at HInt, otherwise at VInt.
#define HUD_USE_DIF_FLOOR_AND_ROOF_COLORS FALSE
#define HUD_HINT_SCANLINE_CHANGE_ROOF_BG_COLOR 95 // Only meaningful if HUD_USE_DIF_FLOOR_AND_ROOF_COLORS is set to TRUE

#define HUD_VRAM_START_INDEX 72 // If change this value you'll have to update map_base parameter in resource file
#define HUD_PAL PAL2 // If change this value you'll have to update map_base parameter in resource file
// This value is the parameter map_base that has to go in the resource file if you modify one of the above. Currently is 16456.
#define HUD_BASE_TILE_ATTRIB TILE_ATTR_FULL(HUD_PAL, 0, FALSE, FALSE, HUD_VRAM_START_INDEX)

#define HUD_TILEMAP_COMPRESSED TRUE // If TRUE then we decompress it into a buffer. Otherwise we use the data from ROM which saves some RAM.

#define HUD_XP 0
#define HUD_YP (PLANE_COLUMNS == 64 ? 24 : 12)
#define HUD_HEIGHT 4

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

typedef struct {
    u8 hundrs;
    u8 tens;
    u8 ones;
} Digits;

u16 hud_loadInitialState (u16 currentTileIndex);
void hud_setup_hint_pals (u32* palA_addr, u32* palB_addr);
u16* hud_getTilemap ();

void hud_resetAmmo ();
void hud_setAmmo (u8 hundreds, u8 tens, u8 ones);
void hud_addAmmoUnits (u16 amnt);
void hud_subAmmoUnits (u16 amnt);
void hud_resetHealth ();
void hud_setHealth (u8 hundreds, u8 tens, u8 ones);
void hud_addHealthUnits (u16 amnt);
void hud_subHealthUnits (u16 amnt);
void hud_resetArmor ();
void hud_setArmor (u8 hundreds, u8 tens, u8 ones);
void hud_addArmorUnits (u16 amnt);
void hud_subArmorUnits (u16 amnt);
void hud_resetWeapons ();
void hud_addWeapon (u8 weapon);
void hud_resetKeys ();
void hud_addKey (u8 key);
void hud_resetFaceExpression ();
void hud_setFaceExpression (u8 expr, u8 timer);

bool hud_hasWeaponInInventory (u8 weapon);
bool hud_hasKeyInInventory (u8 key);
bool hud_isDead ();

void hud_update ();

#endif // _HUD_H_