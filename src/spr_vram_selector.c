#include <types.h>
#include <sprite_eng.h>
#include "spr_vram_selector.h"
#include "weapon.h"

u16 spr_vram_getTotalSize ()
{
    // TODO: + others xxx_biggerAnimTileNum()
    return weapon_biggestAnimTileNum();
}

u16 spr_vram_getIndex (u16 resId)
{
    if (resId == SPR_VRAM_WEAPON_RES_ID)
        return TILE_MAX_NUM - weapon_biggestAnimTileNum();
    // Fallback to 0 so we can quickly detect something is odd
    return 0;
}