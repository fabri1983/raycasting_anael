#include "hint_callback.h"
#include <vdp_bg.h>
#include <vdp.h>
#include <vdp_spr.h>
#include <pal.h>
#include <memory.h>
#include <sys.h>
#include "consts.h"
#include "consts_ext.h"
#include "weapon_consts.h"
#include "hud.h"
#if PLANE_COLUMNS == 64
#include "hud_320.h"
#else
#include "hud_256.h"
#endif
#include "frame_buffer.h"

#if DMA_ENQUEUE_HUD_TILEMAP_TO_FLUSH_AT_HINT
bool hud_tilemap_set;
#endif

u16 tilesLenInWordTotalToDMA;

static u16 tiles_elems;
static void* tiles_from[DMA_MAX_QUEUE_CAPACITY] = {0};
static u16 tiles_toIndex[DMA_MAX_QUEUE_CAPACITY] = {0};
static u16 tiles_lenInWord[DMA_MAX_QUEUE_CAPACITY] = {0};

#if DMA_ALLOW_COMPRESSED_SPRITE_TILES
static u16 tiles_buf_elems;
static u16 tiles_buf_toIndex[DMA_MAX_QUEUE_CAPACITY] = {0};
static u16 tiles_buf_lenInWord[DMA_MAX_QUEUE_CAPACITY] = {0};
static u16* tiles_buf_dmaBufPtr;
#endif

#if DMA_ENQUEUE_VDP_SPRITE_CACHE_TO_FLUSH_AT_HINT
static u16 vdpSpriteCache_lenInWord;
#endif

void hint_reset ()
{
    #if DMA_ENQUEUE_HUD_TILEMAP_TO_FLUSH_AT_HINT
    hud_tilemap_set = FALSE;
    #endif

    tilesLenInWordTotalToDMA = 0;

    memset(tiles_from, 0, DMA_MAX_QUEUE_CAPACITY);
    memsetU16(tiles_toIndex, 0, DMA_MAX_QUEUE_CAPACITY);
    memsetU16(tiles_lenInWord, 0, DMA_MAX_QUEUE_CAPACITY);
    tiles_elems = 0;

    #if DMA_ALLOW_COMPRESSED_SPRITE_TILES
    memsetU16(tiles_buf_toIndex, 0, DMA_MAX_QUEUE_CAPACITY);
    memsetU16(tiles_buf_lenInWord, 0, DMA_MAX_QUEUE_CAPACITY);
    tiles_buf_elems = 0;
    tiles_buf_dmaBufPtr = dmaDataBuffer;
    #endif

    #if DMA_ENQUEUE_VDP_SPRITE_CACHE_TO_FLUSH_AT_HINT
    vdpSpriteCache_lenInWord = 0;
    #endif
}

FORCE_INLINE bool canDMAinHint (u16 lenInWord)
{
    u16 sum = tilesLenInWordTotalToDMA + lenInWord;
    return sum <= (u16)DMA_LENGTH_IN_WORD_THRESHOLD_FOR_HINT;
}

FORCE_INLINE void hint_enqueueHudTilemap ()
{
    #if DMA_ENQUEUE_HUD_TILEMAP_TO_FLUSH_AT_HINT
    hud_tilemap_set = TRUE;
    #endif
}

FORCE_INLINE void hint_enqueueTiles (void* from, u16 toIndex, u16 lenInWord)
{
    u16 prev = tiles_elems;
    ++tiles_elems;
    tiles_toIndex[prev] = toIndex;
    tiles_lenInWord[prev] = lenInWord;
    tilesLenInWordTotalToDMA += lenInWord;
    tiles_from[prev] = from;
}

FORCE_INLINE void hint_enqueueTilesBuffered (u16 toIndex, u16 lenInWord)
{
    #if DMA_ALLOW_COMPRESSED_SPRITE_TILES
    u16 prev = tiles_buf_elems;
    ++tiles_buf_elems;
    tiles_buf_toIndex[prev] = toIndex;
    tiles_buf_lenInWord[prev] = lenInWord;
    tiles_buf_dmaBufPtr += lenInWord;
    tilesLenInWordTotalToDMA += lenInWord;
    #endif
}

FORCE_INLINE void hint_enqueueVdpSpriteCache (u16 lenInWord)
{
    #if DMA_ENQUEUE_VDP_SPRITE_CACHE_TO_FLUSH_AT_HINT
    vdpSpriteCache_lenInWord = lenInWord;
    #endif
}

typedef union
{
    u8 code[6];
    struct
    {
        u16 jmpInst;
        VoidCallback* addr;
    };
} InterruptCaller;

extern InterruptCaller hintCaller; // Declared in sys.c

FORCE_INLINE void hint_reset_change_bg_state ()
{
    // C version
    // Change the hint callback to the one that changes the BG color. This takes effect immediatelly.
    //hintCaller.addr = hint_change_bg_callback; //SYS_setHIntCallback(hint_change_bg_callback);

    // ASM version
    __asm volatile (
        // Change the hint callback to the one that changes the BG color. This takes effect immediatelly.
        "move.w  %[_hint_callback],%[_hintCaller]+4\n\t" // SYS_setHIntCallback(hint_change_bg_callback);
        :
        : [_hint_callback] "s" (hint_change_bg_callback), [_hintCaller] "m" (hintCaller)
        :
    );
}

HINTERRUPT_CALLBACK hint_change_bg_callback ()
{
    /*
    // C version
    // Change the hint callback to the normal one. This takes effect immediatelly.
    hintCaller.addr = hint_load_hud_pals_callback; //SYS_setHIntCallback(hint_load_hud_pals_callback);
    // Set BG to the floor color
    waitHCounter_opt1(156); // NOTE: using waitHCounter_opt1() or _opt2() avoids a flickering
    *(vu32*)VDP_CTRL_PORT = VDP_WRITE_CRAM_ADDR(0 * 2); // CRAM index 0
    *(vu16*)VDP_DATA_PORT = 0x0666; // palette_grey[3]=0x0666 floor color
    */

    // ASM version (register free so no push/pop from stack)
    __asm volatile (
        // Set BG to the floor color
        "move.l  %[_CRAM_CMD],(0xC00004)\n\t"      // *(vu32*)VDP_CTRL_PORT = VDP_WRITE_CRAM_ADDR(0 * 2); // CRAM index 0
        "move.w  %[floor_color],(0xC00000)\n\t"    // *(vu16*)VDP_DATA_PORT = 0x0666; //palette_grey[3]=0x0666 floor color
        // Change the hint callback to the normal one. This takes effect immediatelly.
        "move.w  %[_hint_callback],%[_hintCaller]+4" // SYS_setHIntCallback(hint_load_hud_pals_callback);
        :
        : [_CRAM_CMD] "i" (VDP_WRITE_CRAM_ADDR(0 * 2)), [floor_color] "i" (0x0666),
          [_hint_callback] "s" (hint_load_hud_pals_callback), [_hintCaller] "m" (hintCaller)
        : "cc"
    );
}

HINTERRUPT_CALLBACK hint_load_hud_pals_callback ()
{
    vu32* vdpCtrl_ptr_l = (vu32*) VDP_CTRL_PORT;

    // DMA the 2 HUD palettes immediately
    doDMAfast_fixed_args(vdpCtrl_ptr_l, RAM_FIXED_HUD_PALETTES_ADDRESS + 1*2, VDP_DMA_CRAM_ADDR((HUD_BASE_PAL*16 + 1) * 2), 16*HUD_USED_PALS - 1);

    // If case applies, change BG color to ceiling color
    #if RENDER_SET_FLOOR_AND_ROOF_COLORS_ON_HINT
	//waitHCounter_opt3(vdpCtrl_ptr_l, 156); // We can avoid the waiting here since this happens in the HUD region so any CRAM dot is barely noticeable
    *vdpCtrl_ptr_l = VDP_WRITE_CRAM_ADDR(0 * 2); // color index 0;
    //*(vu16*)VDP_DATA_PORT = 0x0444; // palette_grey[2]=0x0444 roof color
    __asm volatile (
        "move.w  #0x0444,-4(%0)"   // 4 cycles faster than move.w #0x0444,(VDP_DATA_PORT)
        :
        : "a" (vdpCtrl_ptr_l)
        : "cc"
    );
    #endif

    #if DMA_ENQUEUE_HUD_TILEMAP_TO_FLUSH_AT_HINT
    // Have any hud tilemaps to DMA?
    if (hud_tilemap_set) {
        hud_tilemap_set = FALSE;

        // Setup DMA address ONLY ONCE
        u32 from = RAM_FIXED_HUD_TILEMAP_DST_ADDRESS + 0*TILEMAP_COLUMNS*2;
        from >>= 1;
        *(vu16*)vdpCtrl_ptr_l = 0x9500 + (from & 0xff); // low
        from >>= 8;
        *(vu16*)vdpCtrl_ptr_l = 0x9600 + (from & 0xff); // mid
        from >>= 8;
        *(vu16*)vdpCtrl_ptr_l = 0x9700 + (from & 0x7f); // high

        // Setup DMA length high ONLY ONCE. Length in words because DMA RAM/ROM to VRAM moves 2 bytes per VDP cycle op
        *(vu16*)vdpCtrl_ptr_l = 0x9400 | ((TILEMAP_COLUMNS >> 8) & 0xff); // DMA length high
        // DMA length low has to be set every time before triggering the DMA command

        #pragma GCC unroll 256 // Always set a big number since it does not accept defines
        for (u16 i=0; i < HUD_BG_H; ++i) {
            doDMAfast_fixed_args_loop_ready(vdpCtrl_ptr_l, VDP_DMA_VRAM_ADDR(PW_ADDR_AT_HUD + i*PLANE_COLUMNS*2), TILEMAP_COLUMNS);
        }
    }
    #endif

    // Have any tiles to DMA?
    while (tiles_elems) {
        --tiles_elems;
        u16 lenInWord = tiles_lenInWord[tiles_elems];
        tilesLenInWordTotalToDMA -= lenInWord;
        // NOTE: this should be DMA_doDma() because tiles might be in in a bank > 4MB
        void* from = tiles_from[tiles_elems];
        u16 to = tiles_toIndex[tiles_elems];
        DMA_doDmaFast(DMA_VRAM, from, to, lenInWord, (s16)-1);
    }

    #if DMA_ENQUEUE_VDP_SPRITE_CACHE_TO_FLUSH_AT_HINT
    // Have any update for vdp sprite cache?
    if (vdpSpriteCache_lenInWord) {
        u16 len = vdpSpriteCache_lenInWord;
        vdpSpriteCache_lenInWord = 0;
        DMA_doDmaFast(DMA_VRAM, (void*) RAM_FIXED_VDP_SPRITE_CACHE_ADDRESS, VDP_SPRITE_LIST_ADDR, len, (s16)-1);
    }
    #endif

    #if DMA_ALLOW_COMPRESSED_SPRITE_TILES
    // Have any buffered tiles to DMA?
    while (tiles_buf_elems) {
        --tiles_buf_elems;
        u16 lenInWord = tiles_buf_lenInWord[tiles_buf_elems];
        tilesLenInWordTotalToDMA -= lenInWord;
        tiles_buf_dmaBufPtr -= lenInWord;
        u16 toIndex = tiles_buf_toIndex[tiles_buf_elems];
        DMA_doDmaFast(DMA_VRAM, tiles_buf_dmaBufPtr, toIndex, lenInWord, (s16)-1);
        DMA_releaseTemp(lenInWord);
    }
    #endif

    // USE THIS TO SEE HOW MUCH DEEP IN THE HUD IMAGE ALL THE DMA GOES
    // turnOffVDP_m(vdpCtrl_ptr_l, 0x74);
    // waitHCounter_opt3(vdpCtrl_ptr_l, 160);
    // turnOnVDP_m(vdpCtrl_ptr_l, 0x74);

    #if HUD_RELOAD_WEAPON_PALS_AT_HINT
    waitVCounterReg(224);
    // DMA the weapon pals that were overriden gy the HUD pals
    doDMAfast_fixed_args(vdpCtrl_ptr_l, RAM_FIXED_WEAPON_PALETTES_ADDRESS + 1*2, VDP_DMA_CRAM_ADDR((WEAPON_BASE_PAL*16 + 1) * 2), 16*WEAPON_USED_PALS - 1);
    #endif
}

static u16 vCounterManual;
static u32 mirror_offset_rows; // positive numbers move planes upward

FORCE_INLINE void hint_reset_mirror_planes_state ()
{
    #if RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT_MULTI_CALLBACKS
    // Change the hint callback to one that mirror halved planes. This takes effect immediatelly.
    hintCaller.addr = hint_mirror_planes_callback_asm_0; //SYS_setHIntCallback(hint_mirror_planes_callback_asm_0);
    #elif RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT
    mirror_offset_rows = (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - HMC_START_OFFSET_FACTOR*((2 << 16) | 2);
    vCounterManual = HINT_SCANLINE_MID_SCREEN;
    // Change the hint callback to one that mirror halved planes. This takes effect immediatelly.
    hintCaller.addr = hint_mirror_planes_callback; //SYS_setHIntCallback(hint_mirror_planes_callback);
    #endif

    // Change the HInt counter to start from every scanline again. This takes effect next VDP's hint assertion.
    *(vu16*)VDP_CTRL_PORT = 0x8A00 | 0; //VDP_setHIntCounter(0);
}

HINTERRUPT_CALLBACK hint_mirror_planes_callback ()
{
    /*
    // C version:
    vu32* data_l = (u32*) VDP_DATA_PORT;
    vu32* ctrl_l = (u32*) VDP_CTRL_PORT;

    // Apply VSCROLL on both planes (writing in one go on both planes)
    *ctrl_l = VDP_WRITE_VSRAM_ADDR(0); // 0: Plane A, 2: Plane B
    *data_l = mirror_offset_rows; // writes on both planes
    mirror_offset_rows -= (2 << 16) | 2; // -1 to move down the screen, and other -1 to compensate for the already advanced scanline

    --vCounterManual;
    if (vCounterManual == 0) {
        // Change the HInt counter to the HUD region. This takes effect next VDP's hint assertion.
        *(vu16*)ctrl_l = 0x8A00 | (HINT_SCANLINE_MID_SCREEN - 2); //VDP_setHIntCounter(HINT_SCANLINE_MID_SCREEN - 2);
        // Change to the hint callback with last operations
        hintCaller.addr = hint_mirror_planes_last_scanline_callback; //SYS_setHIntCallback(hint_mirror_planes_last_scanline_callback);
    }
    */

    // ASM version (register free so no push/pop from stack)
    __asm volatile (
        // Apply VSCROLL on both planes (writing in one go on both planes)
        "move.l  %[_VSRAM_CMD],(0xC00004)\n\t" // VDP_CTRL_PORT: 0: Plane A, 2: Plane B
        "move.l  %[mirror_offset_rows],(0xC00000)\n\t" // VDP_DATA_PORT: writes on both planes
        "subi.l  %[NEXT_ROW_OFFSET],%[mirror_offset_rows]\n\t" // mirror_offset_rows -= (2 << 16) | 2;
        "subq.w  #1,%[vCounterManual]\n\t" // --vCounterManual;
        "bne.s   1f\n\t"
        // Change the HInt counter to the HUD region. This takes effect next VDP's hint assertion.
        "move.w  %[_HINT_COUNTER],(0xC00004)\n\t" // VDP_setHIntCounter(HINT_SCANLINE_MID_SCREEN - 2);
        "move.w  %[_hint_callback],%[_hintCaller]+4\n\t" // SYS_setHIntCallback(hint_mirror_planes_last_scanline_callback);
        "1:"
        : [mirror_offset_rows] "+m" (mirror_offset_rows), [vCounterManual] "+m" (vCounterManual)
        : [_VSRAM_CMD] "i" (VDP_WRITE_VSRAM_ADDR(0)), [NEXT_ROW_OFFSET] "i" ((2 << 16) | 2),
          [_HINT_COUNTER] "i" (0x8A00 | (HINT_SCANLINE_MID_SCREEN - 2)),
          [_hint_callback] "s" (hint_mirror_planes_last_scanline_callback), [_hintCaller] "m" (hintCaller)
        : "cc"
    );
}

HINTERRUPT_CALLBACK hint_mirror_planes_last_scanline_callback ()
{
    /*
    // C version:
    vu32* data_l = (u32*) VDP_DATA_PORT;
    vu32* ctrl_l = (u32*) VDP_CTRL_PORT;

    // Restore VSCROLL to 0 on both planes (writing in one go on both planes)
    *ctrl_l = VDP_WRITE_VSRAM_ADDR(0); // 0: Plane A, 2: Plane B
    *data_l = 0; // writes on both planes

    // Disable the HInt by setting the max positive value as the HintCounter. This takes effect next VDP's hint assertion.
    *(vu16*)ctrl_l = 0x8A00 | 255; //VDP_setHIntCounter(255);
    // Change the hint callback to the normal one. This takes effect immediatelly.
    hintCaller.addr = hint_load_hud_pals_callback; //SYS_setHIntCallback(hint_load_hud_pals_callback);

    // If case applies, change BG color to floor color
    #if RENDER_SET_FLOOR_AND_ROOF_COLORS_ON_HINT & RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT
    waitHCounter_opt1(156);
    *ctrl_l = VDP_WRITE_CRAM_ADDR(0 * 2); // CRAM index 0
    *(vu16*)data_l = 0x0666; // palette_grey[3]=0x0666 floor color
    #endif
    */

    // ASM version (register free so no push/pop from stack)
    __asm volatile (
        // If case applies, change BG color to floor color
        #if RENDER_SET_FLOOR_AND_ROOF_COLORS_ON_HINT & RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT
        "move.l  %[_CRAM_CMD],(0xC00004)\n\t" // *ctrl_l = VDP_WRITE_CRAM_ADDR(0 * 2); // CRAM index 0
        "move.w  %[floor_color],(0xC00000)\n\t"   // *(vu16*)data_l = 0x0666; // palette_grey[3]=0x0666 floor color
        #endif
        // Restore VSCROLL to 0 on both planes (writing in one go on both planes)
        "move.l  %[_VSRAM_CMD],(0xC00004)\n\t" // VDP_CTRL_PORT: 0: Plane A, 2: Plane B
        "move.l  #0,(0xC00000)\n\t" // VDP_DATA_PORT: writes on both planes
        // Disable the HInt by setting the max positive value as the HintCounter. This takes effect next VDP's hint assertion.
        "move.w  %[_HINT_COUNTER],(0xC00004)\n\t" // VDP_setHIntCounter(255);
        "move.w  %[_hint_callback],%[_hintCaller]+4" // SYS_setHIntCallback(hint_load_hud_pals_callback);
        :
        : [_CRAM_CMD] "i" (VDP_WRITE_CRAM_ADDR(0 * 2)), [floor_color] "i" (0x0666),
          [_VSRAM_CMD] "i" (VDP_WRITE_VSRAM_ADDR(0)), [_HINT_COUNTER] "i" (0x8A00 | 255),
          [_hint_callback] "s" (hint_load_hud_pals_callback), [_hintCaller] "m" (hintCaller)
        : "cc"
    );
}