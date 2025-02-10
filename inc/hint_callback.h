#ifndef _HINT_CALLBACK_H_
#define _HINT_CALLBACK_H_

#include <types.h>
#include "consts.h"
#include "utils.h"

void hint_reset ();

bool canDMAinHint (u16 lenInWord);

void hint_enqueueWeaponPal (u16* pal);

void hint_setPalToRestore (u16* pal);

void hint_enqueueHudTilemap ();

void hint_enqueueTiles (void *from, u16 toIndex, u16 lenInWord);

void hint_enqueueTilesBuffered (u16 toIndex, u16 lenInWord);

void hint_enqueueVdpSpriteCache (u16 lenInWord);

void hint_reset_change_bg_state ();

HINTERRUPT_CALLBACK hint_change_bg_callback ();

HINTERRUPT_CALLBACK hint_load_hud_pals_callback ();

void hint_reset_mirror_planes_state ();

HINTERRUPT_CALLBACK hint_mirror_planes_callback ();

#endif // _HINT_CALLBACK_H_