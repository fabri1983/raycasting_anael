#ifndef _VINT_CALLBACK_H_
#define _VINT_CALLBACK_H_

#include <types.h>
#include "consts.h"

void vint_reset ();

void vint_enqueueTiles (void *from, u16 toIndex, u16 lenInWord);

void vint_enqueueTilesBuffered (u16 toIndex, u16 lenInWord);

void vint_enqueueVdpSpriteCache (u16 lenInWord);

void vint_callback ();

#endif // _VINT_CALLBACK_H_