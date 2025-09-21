#ifndef _WEAPONS_H_
#define _WEAPONS_H_

#include <types.h>
#include "weapon_consts.h"

u16 weapon_biggestAnimTileNum ();

u16 weapon_getVRAMLocation ();

void weapon_resetState ();

void weapon_free_pals_buffer ();

void weapon_select (u16 weaponId);

void weapon_addAmmo (u16 weaponId, u16 amnt);

/// @brief Loads next or previous available weapon in the inventory.
/// @param dir 1: next weapon, -1: previous weapon
void weapon_next (s16 dir);

void weapon_updateSway (bool _isMoving);

void weapon_fire ();

void weapon_update ();

#endif // _WEAPONS_H_