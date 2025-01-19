#ifndef _UTILS_H_
#define _UTILS_H_

#include <types.h>
#include <vdp.h>

#ifdef __GNUC__
#define HINTERRUPT_CALLBACK __attribute__((interrupt)) void
#elif defined(_MSC_VER)
// Declare function for the hint callback (generate a RTE to return from interrupt instead of RTS)
#define HINTERRUPT_CALLBACK void
#endif

#ifdef __GNUC__
#define FORCE_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
// To force method inlining (not sure that GCC does actually care of it)
#define FORCE_INLINE inline __forceinline
#endif

#ifdef __GNUC__
#define NO_INLINE __attribute__((noinline))
#elif defined(_MSC_VER)
// To force no inlining for this method
#define NO_INLINE __declspec(noinline)
#endif

#ifdef __GNUC__
#define VOID_OR_CHAR void
#elif defined(_MSC_VER)
#define VOID_OR_CHAR char
#endif

#define MEMORY_BARRIER() __asm volatile ("" : : : "memory")

#define CLAMP(x, minimum, maximum) ( min(max((x),(minimum)),(maximum)) )

#define SIGN(x) ( (x > 0) - (x < 0) )

// Calculates the position/index of the highest bit of n which corresponds to the power of 2 that 
// is the closest integer logarithm base 2 of n. The result is thus FLOOR(LOG2(n))
#define LOG2(n) (((sizeof(u32) * 8) - 1) - (__builtin_clz((n))))

/// Blastem-nightly builds supports KDebug integration and there's a built-in 68K cycle counter. 
/// Just write to unused to VDP register to start/stop this counter.
/// See also Stef's tools.h BLASTEM_PROFIL_START and BLASTEM_PROFIL_END.
#define STOPWATCH_68K_CYCLES_START() __asm volatile ("move.w  #0x9FC0, (0xC00004).l\n" :::"memory")
#define STOPWATCH_68K_CYCLES_STOP() __asm volatile ("move.w  #0x9F00, (0xC00004).l\n" :::"memory")

/// @brief Set bit 6 (64 decimal, 0x40 hexa) of reg 1.
/// @param reg01 VDP's Reg 1 holds other bits than just VDP ON/OFF status, so we need its current value.
void turnOffVDP (u8 reg01);

/// @brief Set bit 6 (64 decimal, 0x40 hexa) of reg 1.
/// @param reg01 VDP's Reg 1 holds other bits than just VDP ON/OFF status, so we need its current value.
void turnOnVDP (u8 reg01);

/**
 * Wait until HCounter 0xC00009 reaches nth position (actually the (n*2)th pixel since the VDP counts by 2).
*/
void waitHCounter_opt1 (u8 n);

/**
 * Wait until HCounter 0xC00009 reaches nth position (actually the (n*2)th pixel since the VDP counts by 2).
*/
void waitHCounter_opt2 (u8 n);

/**
 * Wait until VCounter 0xC00008 reaches nth scanline position. Parameter n is loaded into a register.
 * The docs straight up say to not trust the value of the V counter during vblank, in that case use VDP_getAdjustedVCounter().
*/
void waitVCounterReg (u16 n);

/**
 * \brief Writes into VDP_CTRL_PORT (0xC00004) the setup for DMA (length and source address).
 * \param len How many colors to move.
 * \param fromAddr Must come >> 1 (shifted to right) already.
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

/// @brief Waits for a certain amount of millisecond (~3.33 ms based timer when wait is >= 100ms). 
/// Lightweight implementation without calling SYS_doVBlankProcess().
/// This method CAN NOT be called from V-Int callback or when V-Int is disabled.
/// @param ms >= 100ms, otherwise use waitMs() from timer.h
void waitMs_ (u32 ms);

/// Wait for a certain amount of subticks. ONLY values < 150.
void waitSubTick_ (u32 subtick);

void unpackSelector (u16 compression, u8* src, u8* dest);

#endif // _UTILS_H_