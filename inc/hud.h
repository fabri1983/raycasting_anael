#ifndef _HUD_H_
#define _HUD_H_

#include <types.h>
#include <vdp_bg.h>
#include "hud_consts.h"

typedef struct {
    u16 hundrs;
    u16 tens;
    u16 ones;
} Digits;

u16 hud_loadInitialState (u16 currentTileIndex);
void hud_free_src_buffer ();
void hud_free_dst_buffer ();
void hud_free_pals_buffer ();

void hud_resetAmmo ();
void hud_setAmmo (u16 hundreds, u16 tens, u16 ones);
void hud_addAmmoUnits (u16 amnt);
void hud_subAmmoUnits (u16 amnt);
void hud_resetHealth ();
void hud_setHealth (u16 hundreds, u16 tens, u16 ones);
void hud_addHealthUnits (u16 amnt);
void hud_subHealthUnits (u16 amnt);
void hud_resetArmor ();
void hud_setArmor (u16 hundreds, u16 tens, u16 ones);
void hud_addArmorUnits (u16 amnt);
void hud_subArmorUnits (u16 amnt);
void hud_resetWeapons ();
void hud_addWeapon (u16 weapon);
void hud_resetKeys ();
void hud_addKey (u16 key);
void hud_resetFaceExpression ();
void hud_setFaceExpression (u16 expr, u16 timer);

bool hud_hasWeaponInInventory (u16 weapon);
bool hud_hasKeyInInventory (u16 key);
bool hud_isDead ();

void hud_update ();

#endif // _HUD_H_