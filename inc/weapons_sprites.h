#ifndef _WEAPONS_SPRITES_H_
#define _WEAPONS_SPRITES_H_

#include <types.h>
#include "consts.h"

u16 weaponsCreateSprites (u16 currTileIndex);

void weaponPistol_load ();

void weapon_fire ();

#endif // _WEAPONS_SPRITES_H_