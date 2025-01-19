#include "utils.h"
#include <vdp.h>
#include <vdp_bg.h>
#include <sys.h>
#include <tools.h>
#include <timer.h>
#include "consts.h"

FORCE_INLINE void turnOffVDP (u8 reg01)
{
    //reg01 &= ~0x40;
    //*(vu16*) VDP_CTRL_PORT = 0x8100 | reg01;
    *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
}

FORCE_INLINE void turnOnVDP (u8 reg01)
{
    //reg01 |= 0x40;
    //*(vu16*) VDP_CTRL_PORT = 0x8100 | reg01;
    *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);
}

FORCE_INLINE void waitHCounter_opt1 (u8 n)
{
    u32 regA = VDP_HVCOUNTER_PORT + 1; // HCounter address is 0xC00009
    __asm volatile (
        "1:\n" 
        "    cmp.b     (%0),%1\n"         // cmp: n - (0xC00009). Compares byte because hcLimit won't be > 160 for our practical cases
        "    bhi.s     1b\n"              // loop back if n is higher than (0xC00009)
            // bhi is for unsigned comparisons
        :
        : "a" (regA), "d" (n)
        : "cc"
    );
}

FORCE_INLINE void waitHCounter_opt2 (u8 n)
{
    u32* regA=0; // placeholder used to indicate the use of an An register
    __asm volatile (
        "    move.l    #0xC00009,%0\n"    // Load HCounter (VDP_HVCOUNTER_PORT + 1 = 0xC00009) into an An register
        "1:\n" 
        "    cmp.b     (%0),%1\n"         // cmp: n - (0xC00009). Compares byte because hcLimit won't be > 160 for our practical cases
        "    bhi.s     1b\n"              // loop back if n is higher than (0xC00009)
            // bhi is for unsigned comparisons
        : "+a" (regA)
        : "d" (n)
        : "cc"
    );
}

FORCE_INLINE void waitVCounterReg (u16 n)
{
    // The docs straight up say to not trust the value of the V counter during vblank, in that case use VDP_getAdjustedVCounter().
    // - Sik: on PAL Systems it jumps from 0x102 (258) to 0x1CA (458) (assuming V28).
    // - Sik: Note that the 9th bit is not visible through the VCounter, so what happens from the 68000's viewpoint is that it reaches 0xFF (255), 
    // then counts from 0x00 to 0x02, then from 0xCA (202) to 0xFF (255), and finally starts back at 0x00.
    // - Stef: on PAL System the VCounter rollback occurs at 0xCA (202) so probably better to use n up to 202 to avoid that edge case.

    u32 regA = VDP_HVCOUNTER_PORT; // VCounter address is 0xC00008
    __asm volatile (
        "1:\n"
        "    cmp.w     (%0),%1\n"         // cmp: n - (0xC00008)
        "    bgt.s     1b\n"              // loop back if n is higher than (0xC00008)
            // bhi is for unsigned comparisons, 
            // bge/bgt are for signed comparisons in case n comes already smaller than value in VDP_HVCOUNTER_PORT memory
        :
        : "a" (regA), "d" (n << 8) // (n << 8) | 0xFF
        : "cc"
    );
}

FORCE_INLINE void setupDMAForPals (u16 len, u32 fromAddr)
{
    // Uncomment if you previously change it to 1 (CPU access to VRAM is 1 byte length, and 2 bytes length for CRAM and VSRAM)
    //VDP_setAutoInc(2);

    // Setup DMA length (in long word here): low at higher word, high at lower word
    vu32* dmaPtr_l = (vu32*) VDP_CTRL_PORT;
    *dmaPtr_l = ((0x9300 | (u8)len) << 16) | (0x9400 | (u8)(len >> 8));

    // Setup DMA address
    vu16* dmaPtr_w = (vu16*) VDP_CTRL_PORT;
    *dmaPtr_w = 0x9500 | (u8)fromAddr; // low
    *dmaPtr_w = 0x9600 | (u8)(fromAddr >> 8); // mid
    *dmaPtr_w = 0x9700 | ((fromAddr >> 16) & 0x7f); // high
}

FORCE_INLINE u16 mulu_shft_FS (u16 op1, u16 op2)
{
    __asm volatile (
        "mulu.w  %1, %0\n\t"     // mulu.w op2,op1
        "lsr.l   %[_FS], %0\n\t" // lsr.l FS, op1
        : "+d" (op1)
        : "d" (op2), [_FS] "i" (FS)
        :
    );
    return op1;
}

const unsigned char div_100[] = {
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
    '1','1','1','1','1','1','1','1','1','1',
    '1','1','1','1','1','1','1','1','1','1',
    '1','1','1','1','1','1','1','1','1','1',
    '1','1','1','1','1','1','1','1','1','1',
    '1','1','1','1','1','1','1','1','1','1',
    '1','1','1','1','1','1','1','1','1','1',
    '1','1','1','1','1','1','1','1','1','1',
    '1','1','1','1','1','1','1','1','1','1',
    '1','1','1','1','1','1','1','1','1','1',
    '1','1','1','1','1','1','1','1','1','1',
    '2','2','2','2','2','2','2','2','2','2',
    '2','2','2','2','2','2','2','2','2','2',
    '2','2','2','2','2','2','2','2','2','2',
    '2','2','2','2','2','2','2','2','2','2',
    '2','2','2','2','2','2','2','2','2','2',
    '2','2','2','2','2','2',
};

const unsigned char div_10_mod_10[] = {
    '0','0','0','0','0','0','0','0','0','0',
    '1','1','1','1','1','1','1','1','1','1',
    '2','2','2','2','2','2','2','2','2','2',
    '3','3','3','3','3','3','3','3','3','3',
    '4','4','4','4','4','4','4','4','4','4',
    '5','5','5','5','5','5','5','5','5','5',
    '6','6','6','6','6','6','6','6','6','6',
    '7','7','7','7','7','7','7','7','7','7',
    '8','8','8','8','8','8','8','8','8','8',
    '9','9','9','9','9','9','9','9','9','9',
    '0','0','0','0','0','0','0','0','0','0',
    '1','1','1','1','1','1','1','1','1','1',
    '2','2','2','2','2','2','2','2','2','2',
    '3','3','3','3','3','3','3','3','3','3',
    '4','4','4','4','4','4','4','4','4','4',
    '5','5','5','5','5','5','5','5','5','5',
    '6','6','6','6','6','6','6','6','6','6',
    '7','7','7','7','7','7','7','7','7','7',
    '8','8','8','8','8','8','8','8','8','8',
    '9','9','9','9','9','9','9','9','9','9',
    '0','0','0','0','0','0','0','0','0','0',
    '1','1','1','1','1','1','1','1','1','1',
    '2','2','2','2','2','2','2','2','2','2',
    '3','3','3','3','3','3','3','3','3','3',
    '4','4','4','4','4','4','4','4','4','4',
    '5','5','5','5','5','5',
};

const unsigned char mod_10[] = {
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5','6','7','8','9',
    '0','1','2','3','4','5',
};

static char str_cpu_load[] = "   %"; // \0 is implicitelly added

void showCPULoad (u16 xPos, u16 yPos)
{
	u16 num = SYS_getCPULoad();
	str_cpu_load[0] = div_100[num];
    str_cpu_load[1] = div_10_mod_10[num];
    str_cpu_load[2] = mod_10[num];
    str_cpu_load[3] = '%'; // overrides garbage data from mod_10[num] when num > 255
    str_cpu_load[4] = '\0'; // overrides garbage data from mod_10[num] when num > 255
	VDP_clearText(xPos, yPos, 3);
	VDP_drawTextBG(BG_A, str_cpu_load, xPos, yPos);
    //VDP_drawTextEx(BG_A, str_cpu_load, TILE_ATTR_FULL(0, 1, FALSE, FALSE, TILE_FONT_INDEX), xPos, yPos, DMA_QUEUE);
}

static char str_fps[] = "    "; // \0 is implicitelly added

void showFPS (u16 xPos, u16 yPos)
{
    u16 num = SYS_getFPS();
	str_fps[0] = div_100[num];
    str_fps[1] = div_10_mod_10[num];
    str_fps[2] = mod_10[num];
    str_fps[3] = ' '; // overrides garbage data from mod_10[num] when num > 255
    str_fps[4] = '\0'; // overrides garbage data from mod_10[num] when num > 255
    VDP_clearText(xPos, yPos, 3);
	VDP_drawTextBG(BG_A, str_fps, xPos, yPos);
}

void waitMs_ (u32 ms)
{
	u32 tick = (ms * TICKPERSECOND) / 1000;
	const u32 start = getTick();
    u32 max = start + tick;
    u32 current;

    // need to check for overflow
    if (max < start) max = 0xFFFFFFFF;

    // wait until we reached subtick
    do {
        current = getTick();
    }
    while (current < max);
}

FORCE_INLINE void waitSubTick_ (u32 subtick)
{
	if (subtick == 0)
		return;

    u32 tmp = subtick*7;
    // Seems that every 7 loops it simulates a tick.
    // TODO: use cycle accurate wait loop in asm (about 100 cycles for 1 subtick)
    __asm volatile (
        "1:\n\t"
        "dbra   %0, 1b" // dbf/dbra: test if not zero, then decrement register dN and branch back (b) to label 1
        : "+d" (tmp)
        :
        : "cc"
	);
}

FORCE_INLINE void unpackSelector (u16 compression, u8* src, u8* dest)
{
    switch (compression) {
        case COMPRESSION_APLIB:
            aplib_unpack(src, dest); // SGDK
            break;
        case COMPRESSION_LZ4W:
            lz4w_unpack(src, dest); // SGDK
            break;
    }
}