#include "weapons_sprites.h"
#include <vdp_tile.h>
#include <sprite_eng.h>
#include "weapons_res.h"
#include "hud_320.h"

#define WEAPON_PISTOL_ANIM_WIDTH 11
#define WEAPON_PISTOL_ANIM_HEIGHT 13

static Sprite* spr_weapon_pistol_anim;
static Sprite* spr_currentWeapon;

u16 weaponsCreateSprites (u16 currTileIndex)
{
    u16 baseTileAttribs = TILE_ATTR_FULL(PAL3, 0, FALSE, FALSE, currTileIndex);
    currTileIndex += sprDef_weapon_pistol_anim.maxNumTile;
    spr_weapon_pistol_anim = SPR_addSpriteExSafe(&sprDef_weapon_pistol_anim, 0, 0, baseTileAttribs, 
        SPR_FLAG_AUTO_TILE_UPLOAD | SPR_FLAG_AUTO_VRAM_ALLOC | SPR_FLAG_DISABLE_ANIMATION_LOOP);
    SPR_setVisibility(spr_weapon_pistol_anim, HIDDEN);

    spr_currentWeapon = spr_weapon_pistol_anim;

    return currTileIndex;
}


FORCE_INLINE void weaponPistol_load ()
{
    SPR_setVisibility(spr_currentWeapon, HIDDEN);

    SPR_setVisibility(spr_weapon_pistol_anim, VISIBLE);
    SPR_setPosition(spr_weapon_pistol_anim, (TILEMAP_COLUMNS/2 - (WEAPON_PISTOL_ANIM_WIDTH+1)/2)*8 , (HUD_BASE_YP - WEAPON_PISTOL_ANIM_HEIGHT)*8);
    spr_currentWeapon = spr_weapon_pistol_anim;
}

FORCE_INLINE void weapon_fire ()
{
    SPR_setFrame(spr_currentWeapon, 1); // reset animation to 2nd frame which is after the iddle frame
    spr_currentWeapon->status &= ~0x0010; // set NOT STATE_ANIMATION_DONE from sprite_eng.c
}