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

HINTERRUPT_CALLBACK hint_callback ();

#endif // _HINT_CALLBACK_H_