#include "utils.h"
#include <vdp.h>
#include <vdp_bg.h>
#include <sys.h>
#include <tools.h>
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

FORCE_INLINE void waitHCounter_CPU (u8 n)
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

FORCE_INLINE void waitHCounter_DMA (u8 n)
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
    u32 regA = VDP_HVCOUNTER_PORT; // VCounter address is 0xC00008
    __asm volatile (
        "1:\n"
        "    cmp.w     (%0),%1\n"         // cmp: n - (0xC00008)
        "    bhi.s     1b\n"              // loop back if n is higher than (0xC00008)
            // bhi is for unsigned comparisons
        :
        : "a" (regA), "d" (n << 8) // (n << 8) | 0xFF
        : "cc"
    );
}

FORCE_INLINE void setupDMAForPals (u16 len, u32 fromAddr)
{
    // Uncomment if you previously change it to 1 (CPU access to VRAM is 1 byte length, and 2 bytes length for CRAM and VSRAM)
    //VDP_setAutoInc(2);

    vu16* dmaPtr = (vu16*) VDP_CTRL_PORT;

    // Setup DMA length (in word here): low and high
    *dmaPtr = 0x9300 | (len & 0xff);
    *dmaPtr = 0x9400 | ((len >> 8) & 0xff); // This step is useless if the length has only set first 8 bits

    // Setup DMA address
    *dmaPtr = 0x9500 | (fromAddr & 0xff);
    *dmaPtr = 0x9600 | ((fromAddr >> 8) & 0xff); // This step is useless if the address has only set first 8 bits
    *dmaPtr = 0x9700 | ((fromAddr >> 16) & 0x7f); // This step is useless if the address has only set first 12 bits
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
    str_cpu_load[3] = '%'; // leave it so it overrides garbage data from mod_10[num] when num > 255
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
    str_fps[3] = ' '; // leave it so it overrides garbage data from mod_10[num] when num > 255
    VDP_clearText(xPos, yPos, 3);
	VDP_drawTextBG(BG_A, str_fps, xPos, yPos);
}

void unpackSelector (u16 compression, u8* src, u8* dest)
{
    switch (compression) {
        case COMPRESSION_APLIB:
            aplib_unpack(src, dest); // SGDK
            break;
        case COMPRESSION_LZ4W:
            lz4w_unpack(src, dest); // SGDK
            break;
        default: break;
    }
}