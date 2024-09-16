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
void turnOffVDP (u8 reg01);

/// @brief Set bit 6 (64 decimal, 0x40 hexa) of reg 1.
/// @param reg01 VDP's Reg 1 holds other bits than just VDP ON/OFF status, so we need its current value.
void turnOnVDP (u8 reg01);

/**
 * Wait until HCounter 0xC00009 reaches nth position (actually the (n*2)th pixel since the VDP counts by 2).
*/
void waitHCounter (u8 n);

/**
 * Wait until VCounter 0xC00008 reaches nth scanline position. Parameter n is loaded into a register.
*/
void waitVCounterReg (u16 n);

/**
 * \brief Writes into VDP_CTRL_PORT (0xC00004) the setup for DMA (length and source address). 
 * Optimizations may apply manually if you know before hand the source address is only 8 bits or 12 bits, and same for the length parameter.
 * \param len How many colors to move.
 * \param fromAddr Must be >> 1 (shifted to right).
*/
void setupDMAForPals (u16 len, u32 fromAddr);

/// @brief Multiply and shift by FS using asm to directly return u16 data type.
/// @param op1 
/// @param op2 
/// @return u16 data type
u16 mulu_shft_FS (u16 op1, u16 op2);

/// @brief Same than VDP_showCPULoad() but optimized. Show values up to 255%. Otherwise it crashes or gives innacurate data.
/// @param xPos screen X position in tiles
/// @param yPos screen Y position in tiles
void showCPULoad (u16 xPos, u16 yPos);

/// @brief Same than VDP_showFPS(FALSE) but optimized. Show values up to 255. Otherwise it crashes or gives innacurate data.
/// @param xPos screen X position in tiles
/// @param yPos screen Y position in tiles
void showFPS (u16 xPos, u16 yPos);

#endif // _UTILS_H_