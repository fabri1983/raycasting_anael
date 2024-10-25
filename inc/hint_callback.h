#ifndef _HINT_CALLBACK_H_
#define _HINT_CALLBACK_H_

#include <types.h>
#include "consts.h"
#include "utils.h"

#define DMA_MAX_WORDS_QUEUE 8
#define DMA_TILES_DMA_THRESHOLD_FOR_HINT 384
#define DMA_LENGTH_IN_WORD_THRESHOLD_FOR_HINT ((DMA_TILES_DMA_THRESHOLD_FOR_HINT * 32) / 2)

void hint_reset ();

void hint_setupPals ();

bool canDMAinHint (u16 lenInWord);

void hint_enqueueWeaponPal (u16* pal);

void hint_enqueueHudTilemap ();

void hint_enqueueTiles (void *from, u16 toIndex, u16 lenInWord);

void hint_enqueueTilesBuffered (u16 toIndex, u16 lenInWord);

void hint_enqueueVdpSpriteCache (u16 lenInWord);

HINTERRUPT_CALLBACK hint_callback ();

#endif // _HINT_CALLBACK_H_