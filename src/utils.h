#ifndef _UTILS_H_
#define _UTILS_H_

#include <types.h>
#include <vdp.h>

#ifdef __GNUC__
#define HINTERRUPT_CALLBACK     __attribute__ ((interrupt)) void
#elif defined(_MSC_VER)
#define HINTERRUPT_CALLBACK     void
#endif

#ifdef __GNUC__
#define FORCE_INLINE            inline __attribute__ ((always_inline))
#elif defined(_MSC_VER)
#define FORCE_INLINE            inline __forceinline
#endif

#ifdef __GNUC__
#define NO_INLINE               __attribute__ ((noinline))
#elif defined(_MSC_VER)
#define NO_INLINE               __declspec(noinline)
#endif

/// @brief Set bit 6 (64 decimal, 0x40 hexa) of reg 1.
/// @param reg01 VDP's Reg 1 holds other bits than just VDP ON/OFF status, so we need its current value.
FORCE_INLINE void turnOffVDP (u8 reg01) {
    //reg01 &= ~0x40;
    //*(vu16*) VDP_CTRL_PORT = 0x8100 | reg01;
    *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
}

/// @brief Set bit 6 (64 decimal, 0x40 hexa) of reg 1.
/// @param reg01 VDP's Reg 1 holds other bits than just VDP ON/OFF status, so we need its current value.
FORCE_INLINE void turnOnVDP (u8 reg01) {
    //reg01 |= 0x40;
    //*(vu16*) VDP_CTRL_PORT = 0x8100 | reg01;
    *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);
}

/**
 * Wait until HCounter 0xC00009 reaches nth position (actually the (n*2)th pixel since the VDP counts by 2).
*/
FORCE_INLINE void waitHCounter (u8 n) {
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

/**
 * \brief Writes into VDP_CTRL_PORT (0xC00004) the setup for DMA (length and source address). 
 * Optimizations may apply manually if you know the source address is only 8 bits or 12 bits, and same for the length parameter.
 * \param len How many colors to move.
 * \param fromAddr Must be >> 1 (shifted to right).
*/
void NO_INLINE setupDMAForPals (u16 len, u32 fromAddr) {
    // Uncomment if you previously change it to 1 (CPU access to VRAM is 1 byte length, and 2 bytes length for CRAM and VSRAM)
    //VDP_setAutoInc(2);

    vu16* palDmaPtr = (vu16*) VDP_CTRL_PORT;

    // Setup DMA length (in word here): low and high
    *palDmaPtr = 0x9300 | (len & 0xff);
    *palDmaPtr = 0x9400 | ((len >> 8) & 0xff); // This step is useless if the length has only set first 8 bits

    // Setup DMA address
    *palDmaPtr = 0x9500 | (fromAddr & 0xff);
    *palDmaPtr = 0x9600 | ((fromAddr >> 8) & 0xff); // This step is useless if the address has only set first 8 bits
    *palDmaPtr = 0x9700 | ((fromAddr >> 16) & 0x7f); // This step is useless if the address has only set first 12 bits
}

#endif // _UTILS_H_