#ifndef _WEAPONS_H_
#define _WEAPONS_H_

#include <types.h>
#include "consts.h"
#include "hud.h"

#define WEAPON_BASE_PAL PAL0

#define WEAPON_SPRITE_X_CENTERED (TILEMAP_COLUMNS/2)*8
#define WEAPON_SPRITE_Y_ABOVE_HUD (VERTICAL_ROWS)*8 - 2 // -2 scanlines used for hint DMA hud palettes

#define WEAPON_FIST_ANIM_WIDTH 20
#define WEAPON_FIST_ANIM_HEIGHT 10
#define WEAPON_PISTOL_ANIM_WIDTH 11
#define WEAPON_PISTOL_ANIM_HEIGHT 13

#define WEAPON_START_HIT_FRAME 1 // All sprites fire animations start at the same frame
#define WEAPON_FIST_ANIM_READY_TO_HIT_AGAIN_FRAME 1
#define WEAPON_PISTOL_ANIM_READY_TO_HIT_AGAIN_FRAME 3

#define WEAPON_CHANGE_COOLDOWN_TIMER 8 // This has to be bigger than the effect of scrolling down current weapon and scrolling up new weapon
#define WEAPON_RESET_TO_IDLE_TIMER 2*60 // Always >= than largest animation length multiplied by the frequency of animation frames
#define WEAPON_FIST_FIRE_COOLDOWN_TIMER 28
#define WEAPON_PISTOL_FIRE_COOLDOWN_TIMER 8

#define WEAPON_PISTOL_MAX_AMMO 400
#define WEAPON_SHOTGUN_MAX_AMMO 100
#define WEAPON_MACHINE_GUN_MAX_AMMO 400
#define WEAPON_ROCKET_MAX_AMMO 100
#define WEAPON_PLASMA_MAX_AMMO 600
#define WEAPON_BFG_MAX_AMMO 600

u16 weapon_biggerSummationAnimTiles ();

void weapon_resetState ();

void weapon_select (u8 weaponId);

void weapon_addAmmo (u8 weaponId, u16 amnt);

/// @brief Loads next or previous available weapon in the inventory.
/// @param sign 1: next weapon, -1: previous weapon
void weapon_next (u8 sign);

void weapon_fire ();

void weapon_update ();

#endif // _WEAPONS_H_