#ifndef _WEAPONS_H_
#define _WEAPONS_H_

#include <types.h>
#include "weapon_consts.h"

u16 weapon_biggerAnimTileNum ();

u16 weapon_getVRAMLocation ();

void weapon_resetState ();

void weapon_free_pals_buffer ();

void weapon_select (u8 weaponId);

void weapon_addAmmo (u8 weaponId, u16 amnt);

/// @brief Loads next or previous available weapon in the inventory.
/// @param sign 1: next weapon, -1: previous weapon
void weapon_next (u8 sign);

void weapon_updateSway (bool _isMoving);

void weapon_fire ();

void weapon_update ();

#endif // _WEAPONS_H_