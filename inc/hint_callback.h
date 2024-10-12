#ifndef _HINT_CALLBACK_H_
#define _HINT_CALLBACK_H_

#include <types.h>
#include "consts.h"
#include "utils.h"

void hint_setupPals ();

void hint_enqueueWeaponPal (u16* pal);

HINTERRUPT_CALLBACK hint_callback ();

#endif // _HINT_CALLBACK_H_