#ifndef _HINT_CALLBACK_H_
#define _HINT_CALLBACK_H_

#include <types.h>
#include "utils.h"

void hint_reset ();

bool canDMAinHint (u16 lenInWord);

void hint_enqueueHudTilemap ();

extern void hint_enqueueTiles (void *from, u16 toIndex, u16 lenInWord);

void hint_enqueueTilesBuffered (u16 toIndex, u16 lenInWord);

void hint_enqueueVdpSpriteCache (u16 lenInWord);

void hint_reset_change_bg_state ();

HINTERRUPT_CALLBACK hint_change_bg_callback ();

HINTERRUPT_CALLBACK hint_load_hud_pals_callback ();

void hint_reset_mirror_planes_state ();

HINTERRUPT_CALLBACK hint_mirror_planes_callback ();

HINTERRUPT_CALLBACK hint_mirror_planes_last_scanline_callback ();

// Defined in hint_callback_.s
extern HINTERRUPT_CALLBACK hint_mirror_planes_callback_asm_0 ();

#endif // _HINT_CALLBACK_H_