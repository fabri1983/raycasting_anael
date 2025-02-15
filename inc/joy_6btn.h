#ifndef _JOY_6BTN_H_
#define _JOY_6BTN_H_

#include <types.h>

// Call this method whenever a joy button is pressed on logo screen.
void joy_setRandomSeed ();

void joy_update_6btn ();

u16 joy_readJoypad_joy1 ();

#endif // _JOY_6BTN_H_