#include "hint_callback.h"
#include <vdp_bg.h>
#include <vdp.h>
#include <vdp_spr.h>
#include <pal.h>
#include <memory.h>
#include <sys.h>
#include "consts.h"
#include "consts_ext.h"
#include "hud.h"
#if PLANE_COLUMNS == 64
#include "hud_320.h"
#else
#include "hud_256.h"
#endif
#include "weapons.h"
#include "frame_buffer.h"

u32 hudPalA_addrForDMA;
u32 hudPalB_addrForDMA;

u32 weaponPalA_addrForDMA;
u32 weaponPalB_addrForDMA;

#if HUD_RELOAD_OVERRIDEN_PALETTES_AT_HINT
u32 restorePalA_addrForDMA;
u32 restorePalB_addrForDMA;
#endif

#if DMA_ENQUEUE_HUD_TILEMAP_TO_FLUSH_AT_HINT
u8 hud_tilemap;
#endif

u16 tilesLenInWordTotalToDMA;

static u8 tiles_elems;
static void* tiles_from[DMA_MAX_QUEUE_CAPACITY] = {0};
static u16 tiles_toIndex[DMA_MAX_QUEUE_CAPACITY] = {0};
static u16 tiles_lenInWord[DMA_MAX_QUEUE_CAPACITY] = {0};

#if DMA_ALLOW_BUFFERED_SPRITE_TILES
static u8 tiles_buf_elems;
static u16 tiles_buf_toIndex[DMA_MAX_QUEUE_CAPACITY] = {0};
static u16 tiles_buf_lenInWord[DMA_MAX_QUEUE_CAPACITY] = {0};
static u16* tiles_buf_dmaBufPtr;
#endif

#if DMA_ENQUEUE_VDP_SPRITE_CACHE_TO_FLUSH_AT_HINT
static u16 vdpSpriteCache_lenInWord;
#endif

void hint_reset ()
{
    hud_setup_hint_pals(&hudPalA_addrForDMA, &hudPalB_addrForDMA);

    weaponPalA_addrForDMA = 0;
    weaponPalB_addrForDMA = 0;

    #if HUD_RELOAD_OVERRIDEN_PALETTES_AT_HINT
    restorePalA_addrForDMA = 0;
    restorePalB_addrForDMA = 0;
    #endif

    #if DMA_ENQUEUE_HUD_TILEMAP_TO_FLUSH_AT_HINT
    hud_tilemap = 0;
    #endif

    tilesLenInWordTotalToDMA = 0;

    memset(tiles_from, 0, DMA_MAX_QUEUE_CAPACITY);
    memsetU16(tiles_toIndex, 0, DMA_MAX_QUEUE_CAPACITY);
    memsetU16(tiles_lenInWord, 0, DMA_MAX_QUEUE_CAPACITY);
    tiles_elems = 0;

    #if DMA_ALLOW_BUFFERED_SPRITE_TILES
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
    return (tilesLenInWordTotalToDMA + lenInWord) <= DMA_LENGTH_IN_WORD_THRESHOLD_FOR_HINT;
}

FORCE_INLINE void hint_enqueueWeaponPal (u16* pal)
{
    // This was the old way
    // PAL_setColors(WEAPON_BASE_PAL*16 + 1, pal + 1, 16, DMA_QUEUE);
    // PAL_setColors((WEAPON_BASE_PAL+1)*16 + 1, pal + 16 + 1, 16, DMA_QUEUE);

    weaponPalA_addrForDMA = (u32) (pal + 1) >> 1;
    //weaponPalB_addrForDMA = (u32) (pal + 16 + 1) >> 1;
}

FORCE_INLINE void hint_setPalToRestore (u16* pal)
{
    #if HUD_RELOAD_OVERRIDEN_PALETTES_AT_HINT
    restorePalA_addrForDMA = (u32) (pal + 1) >> 1;
    //restorePalB_addrForDMA = (u32) (pal + 16 + 1) >> 1;
    #endif
}

FORCE_INLINE void hint_enqueueHudTilemap ()
{
    #if DMA_ENQUEUE_HUD_TILEMAP_TO_FLUSH_AT_HINT
    hud_tilemap = 1;
    #endif
}

void hint_enqueueTiles (void* from, u16 toIndex, u16 lenInWord)
{
    tiles_from[tiles_elems] = from;
    tiles_toIndex[tiles_elems] = toIndex;
    tiles_lenInWord[tiles_elems] = lenInWord;
    ++tiles_elems;
    tilesLenInWordTotalToDMA += lenInWord;
}

void hint_enqueueTilesBuffered (u16 toIndex, u16 lenInWord)
{
    #if DMA_ALLOW_BUFFERED_SPRITE_TILES
    tiles_buf_toIndex[tiles_buf_elems] = toIndex;
    tiles_buf_lenInWord[tiles_buf_elems] = lenInWord;
    ++tiles_buf_elems;
    tiles_buf_dmaBufPtr += lenInWord;
    tilesLenInWordTotalToDMA += lenInWord;
    #endif
}

void hint_enqueueVdpSpriteCache (u16 lenInWord)
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

extern InterruptCaller hintCaller;

FORCE_INLINE void hint_reset_change_bg_state ()
{
    // C version
    // Change the hint callback to the one that changes the BG color. This takes effect immediatelly.
    //hintCaller.addr = hint_change_bg_callback; //SYS_setHIntCallback(hint_change_bg_callback);

    // ASM version
    __asm volatile (
        // Change the hint callback to the one that changes the BG color. This takes effect immediatelly.
        "move.w  %[hint_callback],%[hintCaller]+4\n\t" // SYS_setHIntCallback(hint_change_bg_callback);
        :
        : [hint_callback] "s" (hint_change_bg_callback), [hintCaller] "m" (hintCaller)
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
    *(vu16*)VDP_DATA_PORT = 0x0444; //palette_grey[2]; // floor color
    */

    // ASM version.
    __asm volatile (
        // Change the hint callback to the normal one. This takes effect immediatelly.
        "move.w  %[hint_callback],%[hintCaller]+4\n\t" // SYS_setHIntCallback(hint_load_hud_pals_callback);
        // Set BG to the floor color
        "1:\n\t"
        "cmp.b   %c[_HCOUNTER_PORT],%[hcLimit]\n\t" // cmp: n - (0xC00009). Compares byte because hcLimit won't be > 160 for our practical cases
        "bhi.s   1b\n\t"                            // loop back if n is lower than (0xC00009)
        "move.l  %[_CRAM_CMD],(0xC00004)\n\t"       // *(vu32*)VDP_CTRL_PORT = VDP_WRITE_CRAM_ADDR(0 * 2); // CRAM index 0
        "move.w  %[floor_color],(0xC00000)"         // *(vu16*)VDP_DATA_PORT = 0x0444; //palette_grey[2]; // floor color
        :
        : [hcLimit] "d" (156), [_HCOUNTER_PORT] "i" (VDP_HVCOUNTER_PORT + 1), // HCounter address is 0xC00009
          [_CRAM_CMD] "i" (VDP_WRITE_CRAM_ADDR(0 * 2)), [floor_color] "i" (0x0444),
          [hint_callback] "s" (hint_load_hud_pals_callback), [hintCaller] "m" (hintCaller)
        :
    );
}

HINTERRUPT_CALLBACK hint_load_hud_pals_callback ()
{
    vu32* vdpCtrl_ptr_l = (vu32*) VDP_CTRL_PORT;

	// Prepare DMA source address for the 2 HUD palettes
    //waitHCounter_opt3(vdpCtrl_ptr_l, 152);
    //turnOffVDP_m(vdpCtrl_ptr_l, 0x74);
    setupDMAForPals(15 + 15 + 1, hudPalA_addrForDMA);
    //turnOnVDP_m(vdpCtrl_ptr_l, 0x74);

    waitHCounter_opt3(vdpCtrl_ptr_l, 152);
    //turnOffVDP_m(vdpCtrl_ptr_l, 0x74);
    // Trigger DMA command
    *vdpCtrl_ptr_l = VDP_DMA_CRAM_ADDR(((HUD_PAL+0) * 16 + 1) * 2); // trigger DMA transfer
    //turnOnVDP_m(vdpCtrl_ptr_l, 0x74);

    // If case applies, change BG color to ceiling color
    #if HUD_SET_FLOOR_AND_ROOF_COLORS_ON_HINT
	//waitHCounter_opt3(vdpCtrl_ptr_l, 156); // We can avoid the waiting here since this happens in the HUD region so any CRAM dot is barely noticeable
    *vdpCtrl_ptr_l = VDP_WRITE_CRAM_ADDR(0 * 2); // color index 0;
    *(vu16*)VDP_DATA_PORT = 0x0222; //palette_grey[1]; // roof color
    #endif

    #if DMA_ENQUEUE_HUD_TILEMAP_TO_FLUSH_AT_HINT
    // Have any hud tilemaps to DMA?
    if (hud_tilemap) {
        hud_tilemap = 0;
        // PW_ADDR comes with the correct base position in screen
        //DMA_doDmaFast(DMA_VRAM, hud_getTilemap(), PW_ADDR, (PLANE_COLUMNS*HUD_BG_H) - (PLANE_COLUMNS-TILEMAP_COLUMNS), -1);
        u32 fromAddr = HUD_TILEMAP_DST_ADDRESS;
        #pragma GCC unroll 4 // Always set the max number since it does not accept defines
        for (u8 i=0; i < HUD_BG_H; ++i) {
            // it relies on vdpCtrl_ptr_l
            doDMAfast_fixed_args(vdpCtrl_ptr_l, fromAddr + i*PLANE_COLUMNS*2, VDP_DMA_VRAM_ADDR(PW_ADDR + i*PLANE_COLUMNS*2), TILEMAP_COLUMNS);
        }
    }
    #endif

    // Have any tiles to DMA?
    while (tiles_elems) {
        --tiles_elems;
        u16 lenInWord = tiles_lenInWord[tiles_elems];
        tilesLenInWordTotalToDMA -= lenInWord;
        DMA_doDma(DMA_VRAM, tiles_from[tiles_elems], tiles_toIndex[tiles_elems], lenInWord, -1);
    }

    #if DMA_ENQUEUE_VDP_SPRITE_CACHE_TO_FLUSH_AT_HINT
    // Have any update for vdp sprite cache?
    if (vdpSpriteCache_lenInWord) {
        DMA_doDmaFast(DMA_VRAM, vdpSpriteCache, VDP_SPRITE_TABLE, vdpSpriteCache_lenInWord, -1);
        vdpSpriteCache_lenInWord = 0;
    }
    #endif

    #if DMA_ALLOW_BUFFERED_SPRITE_TILES
    // Have any buffered tiles to DMA?
    while (tiles_buf_elems) {
        --tiles_buf_elems;
        u16 lenInWord = tiles_buf_lenInWord[tiles_buf_elems];
        tilesLenInWordTotalToDMA -= lenInWord;
        tiles_buf_dmaBufPtr -= lenInWord;
        u16 toIndex = tiles_buf_toIndex[tiles_buf_elems];
        DMA_doDmaFast(DMA_VRAM, tiles_buf_dmaBufPtr, toIndex, lenInWord, -1);
        DMA_releaseTemp(lenInWord);
    }
    #endif

    // Have any weapon palette to DMA?
    /*if (weaponPalA_addrForDMA) {

        u32 palCmd_weapon = VDP_DMA_CRAM_ADDR(((WEAPON_BASE_PAL+0) * 16 + 1) * 2); // target starting color index multiplied by 2
        waitHCounter_opt3(vdpCtrl_ptr_l, 152);
        setupDMAForPals(15, weaponPalA_addrForDMA);
        waitHCounter_opt3(vdpCtrl_ptr_l, 152);
        turnOffVDP_m(vdpCtrl_ptr_l, 0x74);
        *vdpCtrl_ptr_l = palCmd_weapon; // trigger DMA transfer
        turnOnVDP_m(vdpCtrl_ptr_l, 0x74);

        // palCmd_weapon = VDP_DMA_CRAM_ADDR(((WEAPON_BASE_PAL+1) * 16 + 1) * 2); // target starting color index multiplied by 2
        // waitHCounter_opt3(vdpCtrl_ptr_l, 152);
        // setupDMAForPals(15, weaponPalB_addrForDMA);
        // waitHCounter_opt3(vdpCtrl_ptr_l, 152);
        // turnOffVDP_m(vdpCtrl_ptr_l, 0x74);
        // *vdpCtrl_ptr_l = palCmd_weapon; // trigger DMA transfer
        // turnOnVDP_m(vdpCtrl_ptr_l, 0x74);

        weaponPalA_addrForDMA = NULL;
    }*/

    #if DMA_FRAMEBUFFER_A_EIGHT_CHUNKS_ON_DISPLAY_PERIOD_AND_HINT && DMA_FRAMEBUFFER_ROW_BY_ROW == FALSE
    // Send next 1/8 of frame_buffer Plane A
    DMA_doDmaFast(DMA_VRAM, frame_buffer + (VERTICAL_ROWS*PLANE_COLUMNS)/8, PA_ADDR + EIGHTH_PLANE_ADDR_OFFSET_BYTES, 
        (VERTICAL_ROWS*PLANE_COLUMNS)/8 - (PLANE_COLUMNS-TILEMAP_COLUMNS), -1);
    #elif DMA_FRAMEBUFFER_A_FIRST_QUARTER_ON_HINT && DMA_FRAMEBUFFER_ROW_BY_ROW == FALSE
	// Send first 1/4 of frame_buffer Plane A
	DMA_doDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, (VERTICAL_ROWS*PLANE_COLUMNS)/4 - (PLANE_COLUMNS-TILEMAP_COLUMNS), -1);
    #elif DMA_FRAMEBUFFER_A_FIRST_HALF_ON_HINT && DMA_FRAMEBUFFER_ROW_BY_ROW == FALSE
	// Send first 1/2 of frame_buffer Plane A
	DMA_doDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, (VERTICAL_ROWS*PLANE_COLUMNS)/2 - (PLANE_COLUMNS-TILEMAP_COLUMNS), -1);
    #else
    #endif

    // USE THIS TO SEE HOW MUCH DEEP IN THE HUD IMAGE ALL THE DMA GOES
    // turnOffVDP_m(vdpCtrl_ptr_l, 0x74);
    // waitHCounter_opt3(vdpCtrl_ptr_l, 160);
    // turnOnVDP_m(vdpCtrl_ptr_l, 0x74);

    #if HUD_RELOAD_OVERRIDEN_PALETTES_AT_HINT
	// Reload the palettes that were overriden by the HUD palettes
	u32 palCmd_restore = VDP_DMA_CRAM_ADDR(((WEAPON_BASE_PAL+0) * 16 + 1) * 2); // target starting color index multiplied by 2
    setupDMAForPals(15, restorePalA_addrForDMA);
	waitVCounterReg(224);
    turnOffVDP_m(vdpCtrl_ptr_l, 0x74);
    *vdpCtrl_ptr_l = palCmd_restore; // trigger DMA transfer
	// palCmd_restore = VDP_DMA_CRAM_ADDR(((WEAPON_BASE_PAL+1) * 16 + 1) * 2); // target starting color index multiplied by 2
    // setupDMAForPals(15, restorePalB_addrForDMA);
    // *vdpCtrl_ptr_l = palCmd_restore; // trigger DMA transfer
	turnOnVDP_m(vdpCtrl_ptr_l, 0x74);
    #endif
}

static u16 vCounterManual;
static u32 mirror_offset_rows; // positive numbers move planes upward

FORCE_INLINE void hint_reset_mirror_planes_state ()
{
    #if RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT_MULTI_CALLBACKS
    // Change the hint callback to one that mirror halved planes. This takes effect immediatelly.
    #if HMC_USE_ASM_UNIT
    hintCaller.addr = hint_mirror_planes_callback_asm_0; //SYS_setHIntCallback(hint_mirror_planes_callback_asm_0);
    #else
    hintCaller.addr = hint_mirror_planes_callback_0; //SYS_setHIntCallback(hint_mirror_planes_callback_0);
    #endif
    #elif RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT
    mirror_offset_rows = (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - HMC_START_OFFSET_FACTOR*((2 << 16) | 2);
    vCounterManual = HUD_HINT_SCANLINE_MID_SCREEN + 1; // +1 because we do --vCounterManual before condition check
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
        *(vu16*)ctrl_l = 0x8A00 | (HUD_HINT_SCANLINE_MID_SCREEN - 1); //VDP_setHIntCounter(HUD_HINT_SCANLINE_MID_SCREEN - 1);
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
        "move.w  %[_HINT_COUNTER],(0xC00004)\n\t" // VDP_setHIntCounter(HUD_HINT_SCANLINE_MID_SCREEN-3);
        "move.w  %[hint_callback],%[hintCaller]+4\n\t" // SYS_setHIntCallback(hint_mirror_planes_last_scanline_callback);
        "1:"
        : [mirror_offset_rows] "+m" (mirror_offset_rows), [vCounterManual] "+m" (vCounterManual)
        : [_VSRAM_CMD] "i" (VDP_WRITE_VSRAM_ADDR(0)), [NEXT_ROW_OFFSET] "i" ((2 << 16) | 2),
          [_HINT_COUNTER] "i" (0x8A00 | (HUD_HINT_SCANLINE_MID_SCREEN - 1)),
          [hint_callback] "s" (hint_mirror_planes_last_scanline_callback), [hintCaller] "m" (hintCaller)
        : "cc", "memory"
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
    #if HUD_SET_FLOOR_AND_ROOF_COLORS_ON_HINT
    waitHCounter_opt1(156);
    *ctrl_l = VDP_WRITE_CRAM_ADDR(0 * 2); // CRAM index 0
    *(vu16*)data_l = 0x0444; //palette_grey[2]; // floor color
    #endif
    */

    // ASM version (register free so no push/pop from stack)
    __asm volatile (
        // Restore VSCROLL to 0 on both planes (writing in one go on both planes)
        "move.l  %[_VSRAM_CMD],(0xC00004)\n\t" // VDP_CTRL_PORT: 0: Plane A, 2: Plane B
        "move.l  #0,(0xC00000)\n\t" // VDP_DATA_PORT: writes on both planes
        // Disable the HInt by setting the max positive value as the HintCounter. This takes effect next VDP's hint assertion.
        "move.w  %[_HINT_COUNTER],(0xC00004)\n\t" // VDP_setHIntCounter(255);
        "move.w  %[hint_callback],%[hintCaller]+4\n\t" // SYS_setHIntCallback(hint_load_hud_pals_callback);
        // If case applies, change BG color to floor color
        #if HUD_SET_FLOOR_AND_ROOF_COLORS_ON_HINT
        "1:\n\t"
        "cmpi.b  %[hcLimit],%c[_HCOUNTER_PORT]\n\t" // cmpi: (0xC00009) - n. Compares byte because hcLimit won't be > 160 for our practical cases
        "blo.s   1b\n\t"                            // loop back if n is lower than (0xC00009)
        "move.l  %[_CRAM_CMD],(0xC00004)\n\t" // *ctrl_l = VDP_WRITE_CRAM_ADDR(0 * 2); // CRAM index 0
        "move.w  %[floor_color],(0xC00000)"   // *(vu16*)data_l = 0x0444; //palette_grey[2]; // floor color
        #endif
        :
        : [hcLimit] "i" (156), [_HCOUNTER_PORT] "i" (VDP_HVCOUNTER_PORT + 1), // HCounter address is 0xC00009
          [_CRAM_CMD] "i" (VDP_WRITE_CRAM_ADDR(0 * 2)), [floor_color] "i" (0x0444),
          [_VSRAM_CMD] "i" (VDP_WRITE_VSRAM_ADDR(0)), [_HINT_COUNTER] "i" (0x8A00 | 255),
          [hint_callback] "s" (hint_load_hud_pals_callback), [hintCaller] "m" (hintCaller)
        : "cc", "memory"
    );
}

#define hint_mirror_planes_template(n,rowOffset) \
    __asm volatile ( \
        /* Apply VSCROLL on both planes (writing in one go on both planes) */ \
        "move.l  %[_VSRAM_CMD],(0xC00004)\n\t" /* VDP_CTRL_PORT: 0: Plane A, 2: Plane B */ \
        "move.l  %[_ROW_OFFSET],(0xC00000)\n\t" /* VDP_DATA_PORT: writes on both planes */ \
        "move.w  %[hint_callback],%[hintCaller]+4" /* SYS_setHIntCallback(hint_mirror_planes_callback_N); */ \
        : \
        : [_VSRAM_CMD] "i" (VDP_WRITE_VSRAM_ADDR(0)), [_ROW_OFFSET] "i" (rowOffset), \
          [hint_callback] "s" (hint_mirror_planes_callback_##n), [hintCaller] "m" (hintCaller) \
        : "cc", "memory" \
    )

#define hint_mirror_planes_jump_to_hud_template(rowOffset) \
    __asm volatile ( \
        /* Apply VSCROLL on both planes (writing in one go on both planes) */ \
        "move.l  %[_VSRAM_CMD],(0xC00004)\n\t" /* VDP_CTRL_PORT: 0: Plane A, 2: Plane B */ \
        "move.l  %[_ROW_OFFSET],(0xC00000)\n\t" /* VDP_DATA_PORT: writes on both planes */ \
        /* Change the HInt counter to the amount of scanlines we want to jump from here. This takes effect next VDP's hint assertion. */ \
        "move.w  %[_HINT_COUNTER],(0xC00004)\n\t" /* VDP_setHIntCounter(HUD_HINT_SCANLINE_MID_SCREEN - 1); */ \
        "move.w  %[hint_callback],%[hintCaller]+4" /* SYS_setHIntCallback(hint_mirror_planes_last_scanline_callback); */ \
        : \
        : [_VSRAM_CMD] "i" (VDP_WRITE_VSRAM_ADDR(0)), [_ROW_OFFSET] "i" (rowOffset), \
          [_HINT_COUNTER] "i" (0x8A00 | (HUD_HINT_SCANLINE_MID_SCREEN - 1)), \
          [hint_callback] "s" (hint_mirror_planes_last_scanline_callback), [hintCaller] "m" (hintCaller) \
        : "cc", "memory" \
    )

// Forward declarations

HINTERRUPT_CALLBACK hint_mirror_planes_callback_0 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_1 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_2 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_3 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_4 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_5 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_6 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_7 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_8 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_9 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_10 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_11 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_12 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_13 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_14 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_15 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_16 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_17 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_18 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_19 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_20 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_21 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_22 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_23 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_24 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_25 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_26 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_27 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_28 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_29 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_30 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_31 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_32 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_33 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_34 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_35 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_36 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_37 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_38 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_39 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_40 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_41 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_42 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_43 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_44 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_45 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_46 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_47 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_48 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_49 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_50 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_51 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_52 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_53 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_54 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_55 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_56 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_57 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_58 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_59 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_60 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_61 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_62 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_63 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_64 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_65 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_66 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_67 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_68 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_69 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_70 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_71 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_72 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_73 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_74 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_75 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_76 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_77 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_78 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_79 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_80 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_81 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_82 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_83 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_84 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_85 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_86 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_87 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_88 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_89 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_90 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_91 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_92 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_93 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_94 ();
HINTERRUPT_CALLBACK hint_mirror_planes_callback_95 ();

// Repeat from 0 up to HUD_HINT_SCANLINE_MID_SCREEN - 1

HINTERRUPT_CALLBACK hint_mirror_planes_callback_0 () {
    hint_mirror_planes_template(1, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 0)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_1 () {
    hint_mirror_planes_template(2, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 1)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_2 () {
    hint_mirror_planes_template(3, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 2)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_3 () {
    hint_mirror_planes_template(4, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 3)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_4 () {
    hint_mirror_planes_template(5, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 4)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_5 () {
    hint_mirror_planes_template(6, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 5)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_6 () {
    hint_mirror_planes_template(7, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 6)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_7 () {
    hint_mirror_planes_template(8, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 7)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_8 () {
    hint_mirror_planes_template(9, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 8)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_9 () {
    hint_mirror_planes_template(10, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 9)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_10 () {
    hint_mirror_planes_template(11, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 10)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_11 () {
    hint_mirror_planes_template(12, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 11)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_12 () {
    hint_mirror_planes_template(13, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 12)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_13 () {
    hint_mirror_planes_template(14, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 13)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_14 () {
    hint_mirror_planes_template(15, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 14)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_15 () {
    hint_mirror_planes_template(16, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 15)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_16 () {
    hint_mirror_planes_template(17, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 16)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_17 () {
    hint_mirror_planes_template(18, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 17)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_18 () {
    hint_mirror_planes_template(19, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 18)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_19 () {
    hint_mirror_planes_template(20, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 19)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_20 () {
    hint_mirror_planes_template(21, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 20)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_21 () {
    hint_mirror_planes_template(22, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 21)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_22 () {
    hint_mirror_planes_template(23, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 22)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_23 () {
    hint_mirror_planes_template(24, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 23)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_24 () {
    hint_mirror_planes_template(25, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 24)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_25 () {
    hint_mirror_planes_template(26, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 25)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_26 () {
    hint_mirror_planes_template(27, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 26)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_27 () {
    hint_mirror_planes_template(28, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 27)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_28 () {
    hint_mirror_planes_template(29, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 28)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_29 () {
    hint_mirror_planes_template(30, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 29)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_30 () {
    hint_mirror_planes_template(31, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 30)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_31 () {
    hint_mirror_planes_template(32, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 31)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_32 () {
    hint_mirror_planes_template(33, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 32)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_33 () {
    hint_mirror_planes_template(34, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 33)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_34 () {
    hint_mirror_planes_template(35, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 34)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_35 () {
    hint_mirror_planes_template(36, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 35)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_36 () {
    hint_mirror_planes_template(37, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 36)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_37 () {
    hint_mirror_planes_template(38, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 37)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_38 () {
    hint_mirror_planes_template(39, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 38)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_39 () {
    hint_mirror_planes_template(40, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 39)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_40 () {
    hint_mirror_planes_template(41, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 40)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_41 () {
    hint_mirror_planes_template(42, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 41)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_42 () {
    hint_mirror_planes_template(43, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 42)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_43 () {
    hint_mirror_planes_template(44, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 43)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_44 () {
    hint_mirror_planes_template(45, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 44)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_45 () {
    hint_mirror_planes_template(46, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 45)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_46 () {
    hint_mirror_planes_template(47, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 46)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_47 () {
    hint_mirror_planes_template(48, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 47)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_48 () {
    hint_mirror_planes_template(49, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 48)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_49 () {
    hint_mirror_planes_template(50, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 49)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_50 () {
    hint_mirror_planes_template(51, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 50)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_51 () {
    hint_mirror_planes_template(52, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 51)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_52 () {
    hint_mirror_planes_template(53, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 52)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_53 () {
    hint_mirror_planes_template(54, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 53)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_54 () {
    hint_mirror_planes_template(55, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 54)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_55 () {
    hint_mirror_planes_template(56, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 55)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_56 () {
    hint_mirror_planes_template(57, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 56)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_57 () {
    hint_mirror_planes_template(58, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 57)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_58 () {
    hint_mirror_planes_template(59, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 58)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_59 () {
    hint_mirror_planes_template(60, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 59)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_60 () {
    hint_mirror_planes_template(61, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 60)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_61 () {
    hint_mirror_planes_template(62, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 61)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_62 () {
    hint_mirror_planes_template(63, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 62)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_63 () {
    hint_mirror_planes_template(64, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 63)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_64 () {
    hint_mirror_planes_template(65, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 64)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_65 () {
    hint_mirror_planes_template(66, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 65)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_66 () {
    hint_mirror_planes_template(67, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 66)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_67 () {
    hint_mirror_planes_template(68, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 67)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_68 () {
    hint_mirror_planes_template(69, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 68)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_69 () {
    hint_mirror_planes_template(70, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 69)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_70 () {
    hint_mirror_planes_template(71, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 70)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_71 () {
    hint_mirror_planes_template(72, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 71)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_72 () {
    hint_mirror_planes_template(73, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 72)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_73 () {
    hint_mirror_planes_template(74, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 73)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_74 () {
    hint_mirror_planes_template(75, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 74)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_75 () {
    hint_mirror_planes_template(76, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 75)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_76 () {
    hint_mirror_planes_template(77, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 76)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_77 () {
    hint_mirror_planes_template(78, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 77)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_78 () {
    hint_mirror_planes_template(79, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 78)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_79 () {
    hint_mirror_planes_template(80, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 79)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_80 () {
    hint_mirror_planes_template(81, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 80)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_81 () {
    hint_mirror_planes_template(82, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 81)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_82 () {
    hint_mirror_planes_template(83, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 82)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_83 () {
    hint_mirror_planes_template(84, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 83)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_84 () {
    hint_mirror_planes_template(85, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 84)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_85 () {
    hint_mirror_planes_template(86, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 85)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_86 () {
    hint_mirror_planes_template(87, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 86)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_87 () {
    hint_mirror_planes_template(88, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 87)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_88 () {
    hint_mirror_planes_template(89, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 88)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_89 () {
    hint_mirror_planes_template(90, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 89)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_90 () {
    hint_mirror_planes_template(91, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 90)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_91 () {
    hint_mirror_planes_template(92, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 91)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_92 () {
    hint_mirror_planes_template(93, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 92)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_93 () {
    hint_mirror_planes_template(94, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 93)*((2 << 16) | 2));
}
HINTERRUPT_CALLBACK hint_mirror_planes_callback_94 () {
    hint_mirror_planes_template(95, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 94)*((2 << 16) | 2));
}

// Hint callback that changes the HintCounter to jump into the HUD scanline region
HINTERRUPT_CALLBACK hint_mirror_planes_callback_95 () {
    hint_mirror_planes_jump_to_hud_template((((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - (HMC_START_OFFSET_FACTOR + 95)*((2 << 16) | 2));
}