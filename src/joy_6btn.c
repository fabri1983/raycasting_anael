#include "joy_6btn.h"
#include <joy.h>
#include <z80_ctrl.h>
#include <tools.h>
#include <timer.h>

#define JOY_TYPE_SHIFT          12

#if (HALT_Z80_ON_IO != 0)
    #define Z80_STATE_DECL      u16 z80state;
    #define Z80_STATE_SAVE      z80state = Z80_getAndRequestBus(TRUE);
    #define Z80_STATE_RESTORE   if (!z80state) Z80_releaseBus();
#else
    #define Z80_STATE_DECL
    #define Z80_STATE_SAVE
    #define Z80_STATE_RESTORE
#endif

static vu16 joyState_joy1;

static u16 TH_CONTROL_PHASE(vu8 *pb)
{
    u16 val;

    *pb = 0x00; /* TH (select) low */
    __asm volatile ("nop");
    __asm volatile ("nop");
    val = *pb;

    *pb = 0x40; /* TH (select) high */
    val <<= 8;
    val |= *pb;

    return val;
}

static u16 read6Btn_port1 ()
{
    vu8 *pb;
    u16 val, v1, v2;
    Z80_STATE_DECL

    pb = (vu8 *)0xa10003 + PORT_1*2;

    Z80_STATE_SAVE
    v1 = TH_CONTROL_PHASE(pb);                    /* - 0 s a 0 0 d u - 1 c b r l d u */
    val = TH_CONTROL_PHASE(pb);                   /* - 0 s a 0 0 d u - 1 c b r l d u */
    v2 = TH_CONTROL_PHASE(pb);                    /* - 0 s a 0 0 0 0 - 1 c b m x y z */
    val = TH_CONTROL_PHASE(pb);                   /* - 0 s a 1 1 x x - 1 c b r l d u */
                                                  /* x should be read as 1 on a 6 button controller but in some case we read 0 so take care of that */
    Z80_STATE_RESTORE

    // On a six-button controller, bits 0-3 of the high byte will always read 0.
    // This corresponds to up + down on a 3-button controller, on some controller or emulator that is a possible combination so we try
    // to compensante using last TH low read which should return 1111 on 6 buttons controller (not 100% though).
    // if (((v2 & 0x0F00) != 0x0000) || ((val & 0x0C00) == 0x0000))
    //     v2 = (JOY_TYPE_PAD3 << JOY_TYPE_SHIFT) | 0x0F00; /* three button pad */
    // else
        v2 = (JOY_TYPE_PAD6 << JOY_TYPE_SHIFT) | ((v2 & 0x000F) << 8); /* six button pad */

    val = ((v1 & 0x3000) >> 6) | (v1 & 0x003F);   /* 0 0 0 0 0 0 0 0 s a c b r l d u  */
    val |= v2;                                    /* 0 0 0 1 m x y z s a c b r l d u  or  0 0 0 0 1 1 1 1 s a c b r l d u */
    val ^= 0x0FFF;                                /* 0 0 0 1 M X Y Z S A C B R L D U  or  0 0 0 0 0 0 0 0 S A C B R L D U */

    return val;
}

void joy_setRandomSeed ()
{
    u16 tick = getTick();
    setRandomSeed((tick << 0) ^ (tick << 8));
}

void joy_update_6btn ()
{
    //SYS_disableInts();

    u16 val = read6Btn_port1();
    u16 newstate = val & BUTTON_ALL;
    //u16 change = joyState_joy1 ^ newstate;
    joyState_joy1 = newstate;

    //SYS_enableInts();
}

u16 joy_readJoypad_joy1 ()
{
    return joyState_joy1;
}