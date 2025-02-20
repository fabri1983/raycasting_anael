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
/// @param ctrl_port a variable defined as (vu32*)VDP_CTRL_PORT.
/// @param reg01 VDP's Reg 1 holds other bits than just VDP ON/OFF status, so we need its current value.
#define turnOffVDP_m(ctrl_port,reg01) \
    __asm volatile ( \
        "move.w  %[_reg01],(%[_ctrl_port])" \
        : \
        : [_ctrl_port] "a" (ctrl_port), [_reg01] "i" (0x8100 | (reg01 & ~0x40)) \
        : \
    )

/// @brief Set bit 6 (64 decimal, 0x40 hexa) of reg 1.
/// @param ctrl_port a variable defines as (vu32*)VDP_CTRL_PORT.
/// @param reg01 VDP's Reg 1 holds other bits than just VDP ON/OFF status, so we need its current value.
#define turnOnVDP_m(ctrl_port,reg01) \
__asm volatile ( \
    "move.w  %[_reg01],(%[_ctrl_port])" \
    : \
    : [_ctrl_port] "a" (ctrl_port), [_reg01] "i" (0x8100 | (reg01 | 0x40)) \
    : \
)

/// @brief Set bit 6 (64 decimal, 0x40 hexa) of reg 1.
/// @param reg01 VDP's Reg 1 holds other bits than just VDP ON/OFF status, so we need its current value.
void turnOffVDP (u8 reg01);

/// @brief Set bit 6 (64 decimal, 0x40 hexa) of reg 1.
/// @param reg01 VDP's Reg 1 holds other bits than just VDP ON/OFF status, so we need its current value.
void turnOnVDP (u8 reg01);

/// @brief Wait until HCounter 0xC00009 reaches nth position (actually the (n*2)th pixel since the VDP counts by 2).
/// This implementation is register free, uses 0xC00009 as immediate for accessing its memory location.
/// @param n
#define waitHCounter_imm(n) \
    __asm volatile ( \
        "1:\n\t" \
        "cmpi.b  %[_n],%c[_HCOUNTER_PORT]\n\t" /* cmpi: (0xC00009) - n. Compares byte because hcLimit won't be > 160 for our practical cases */ \
        "blo.s   1b"                           /* loop back if n is lower than (0xC00009) */ \
        : \
        : [_n] "i" (n), [_HCOUNTER_PORT] "i" (VDP_HVCOUNTER_PORT + 1) /* HCounter address is 0xC00009 */ \
        : \
    )

/**
 * Wait until HCounter 0xC00009 reaches nth position (actually the (n*2)th pixel since the VDP counts by 2).
*/
void waitHCounter_opt1 (u8 n);

/**
 * Wait until HCounter 0xC00009 reaches nth position (actually the (n*2)th pixel since the VDP counts by 2).
*/
void waitHCounter_opt2 (u8 n);

/// @brief Wait until HCounter 0xC00009 reaches nth position (actually the (n*2)th pixel since the VDP counts by 2).
/// This implementation depends on already existing ctrl_port variable to address 0xC00004 + 5 = 0xC00009.
/// @param ctrl_port a variable defines as (vu32*)VDP_CTRL_PORT.
/// @param n
#define waitHCounter_opt3(ctrl_port,n) \
    __asm volatile ( \
        "1:\n\t" \
        "cmpi.b  %[_n],5(%[_ctrl_port])\n\t" /* cmpi: (0xC00009) - n. Compares byte because hcLimit won't be > 160 for our practical cases */ \
        "blo.s   1b"                         /* loop back if n is lower than (0xC00009) */ \
        : \
        : [_n] "i" (n), [_ctrl_port] "a" (ctrl_port) \
        : \
    )

/**
 * Wait until VCounter 0xC00008 reaches nth scanline position. Parameter n is loaded into a register.
 * The docs straight up say to not trust the value of the V counter during vblank, in that case use VDP_getAdjustedVCounter().
*/
void waitVCounterReg (u16 n);

/// @brief Writes into VDP_CTRL_PORT (0xC00004) the setup for DMA (length and source address) and writes the command too.
/// Assumes the 4 arguments are known values at compile time, and the VDP stepping was already set.
/// @param ctrl_port a variable defined as (vu32*)VDP_CTRL_PORT.
/// @param fromAddr source VRAM address in u32 format.
/// @param cmdAddr destinaiton address as a command. One of: VDP_DMA_VRAM_ADDR, VDP_DMA_CRAM_ADDR, VDP_DMA_VSRAM_ADDR.
/// @param len words to move (is words bcause DMA RAM/ROM to VRAM move 2 bytes per cycle op).
#define doDMAfast_fixed_args(ctrl_port,fromAddr,cmdAddr,len) \
    __asm volatile ( \
        /* Setup DMA length (in long word here) */ \
        "move.l  %[_len_low_high],(%[_ctrl_port])\n\t" /* *((vu32*) VDP_CTRL_PORT) = ((0x9300 | (u8)len) << 16) | (0x9400 | (u8)(len >> 8)); */ \
        /* Setup DMA address low and mid */ \
        "move.l  %[_addr_low_mid],(%[_ctrl_port])\n\t" /* *((vu32*) VDP_CTRL_PORT) = ((0x9500 | ((fromAddr >> 1) & 0xff)) << 16) | (0x9600 | ((fromAddr >> 9) & 0xff)); */ \
        /* Setup DMA address high */ \
        "move.w  %[_addr_high],(%[_ctrl_port])\n\t" /* *((vu32*) VDP_CTRL_PORT) = (0x9700 | ((fromAddr >> 17) & 0x7f)); */ \
        /* Trigger DMA */ \
        /* NOTE: this should be done as Stef does with two .w writes from memory. See DMA_doDmaFast() */ \
        "move.l  %[_cmdAddr],(%[_ctrl_port])" /* *((vu32*) VDP_CTRL_PORT) = cmdAddr; */ \
        : \
        : [_ctrl_port] "a" (ctrl_port), \
          [_len_low_high] "i" ( ((0x9300 | ((len) & 0xff)) << 16) | (0x9400 | (((len) >> 8) & 0xff)) ), \
          [_addr_low_mid] "i" ( ((0x9500 | (((fromAddr) >> 1) & 0xff)) << 16) | (0x9600 | (((fromAddr) >> 9) & 0xff)) ), \
          [_addr_high] "i" ( (0x9700 | (((fromAddr) >> 17) & 0x7f)) ), /* VRAM COPY operation with high address */ \
          /* If you want to add VDP stepping then use: ((0x9700 | (((fromAddr) >> 17) & 0x7f)) << 16) | (0x8F00 | ((step) & 0xff)) \ */ \
          [_cmdAddr] "i" ((u32)(cmdAddr)) \
        : \
    )

/**
 * \brief Writes into VDP_CTRL_PORT (0xC00004) the setup for DMA (length and source address). Assumes fromAddr is a dynamic var.
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