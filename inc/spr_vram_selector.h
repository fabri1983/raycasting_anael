#ifndef _SPR_VRAM_SELECTOR_H_
#define _SPR_VRAM_SELECTOR_H_

#include <types.h>

#define SPR_VRAM_WEAPON_RES_ID 1

u16 spr_vram_getTotalSize ();
u16 spr_vram_getIndex (u16 resId);

#endif // _SPR_VRAM_SELECTOR_H_