#ifndef _HUD_H_
#define _HUD_H_

#include <types.h>
#include <vdp_bg.h>
#include "hud_consts.h"

typedef struct {
    u8 hundrs;
    u8 tens;
    u8 ones;
} Digits;

u16 hud_loadInitialState (u16 currentTileIndex);
void hud_free_dst_buffer();
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