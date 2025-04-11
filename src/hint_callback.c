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

#if DMA_ALLOW_COMPRESSED_SPRITE_TILES
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

bool canDMAinHint (u16 lenInWord)
{
    return (tilesLenInWordTotalToDMA + lenInWord) <= DMA_LENGTH_IN_WORD_THRESHOLD_FOR_HINT;
}

void hint_enqueueWeaponPal (u16* pal)
{
    // This was the old way
    // PAL_setColors(WEAPON_BASE_PAL*16 + 1, pal + 1, 16, DMA_QUEUE);
    // PAL_setColors((WEAPON_BASE_PAL+1)*16 + 1, pal + 16 + 1, 16, DMA_QUEUE);

    weaponPalA_addrForDMA = (u32) (pal + 1) >> 1;
    //weaponPalB_addrForDMA = (u32) (pal + 16 + 1) >> 1;
}

void hint_setPalToRestore (u16* pal)
{
    #if HUD_RELOAD_OVERRIDEN_PALETTES_AT_HINT
    restorePalA_addrForDMA = (u32) (pal + 1) >> 1;
    //restorePalB_addrForDMA = (u32) (pal + 16 + 1) >> 1;
    #endif
}

void hint_enqueueHudTilemap ()
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
    #if DMA_ALLOW_COMPRESSED_SPRITE_TILES
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

extern InterruptCaller hintCaller; // Declared in sys.c

void hint_reset_change_bg_state ()
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
    *(vu16*)VDP_DATA_PORT = 0x0444; // palette_grey[2]=0x0444 floor color
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
        "move.w  %[floor_color],(0xC00000)"         // *(vu16*)VDP_DATA_PORT = 0x0444; //palette_grey[2]=0x0444 floor color
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
    #if RENDER_SET_FLOOR_AND_ROOF_COLORS_ON_HINT
	//waitHCounter_opt3(vdpCtrl_ptr_l, 156); // We can avoid the waiting here since this happens in the HUD region so any CRAM dot is barely noticeable
    *vdpCtrl_ptr_l = VDP_WRITE_CRAM_ADDR(0 * 2); // color index 0;
    *(vu16*)VDP_DATA_PORT = 0x0222; // palette_grey[1]=0x0222 roof color
    #endif

    #if DMA_ENQUEUE_HUD_TILEMAP_TO_FLUSH_AT_HINT
    // Have any hud tilemaps to DMA?
    if (hud_tilemap) {
        hud_tilemap = 0;
        // PW_ADDR_AT_HUD comes with the correct base position in screen
        u32 fromAddr = HUD_TILEMAP_DST_ADDRESS;
        #pragma GCC unroll 4 // Always set the max number since it does not accept defines
        for (u8 i=0; i < HUD_BG_H; ++i) {
            // it relies on vdpCtrl_ptr_l
            doDMAfast_fixed_args(vdpCtrl_ptr_l, fromAddr + i*TILEMAP_COLUMNS*2, VDP_DMA_VRAM_ADDR(PW_ADDR_AT_HUD + i*PLANE_COLUMNS*2), TILEMAP_COLUMNS);
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

    #if DMA_ALLOW_COMPRESSED_SPRITE_TILES
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

void hint_reset_mirror_planes_state ()
{
    #if RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT_MULTI_CALLBACKS
    // Change the hint callback to one that mirror halved planes. This takes effect immediatelly.
    hintCaller.addr = hint_mirror_planes_callback_asm_0; //SYS_setHIntCallback(hint_mirror_planes_callback_asm_0);
    #elif RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT
    mirror_offset_rows = (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - HMC_START_OFFSET_FACTOR*((2 << 16) | 2);
    vCounterManual = HINT_SCANLINE_MID_SCREEN + 1; // +1 because we do --vCounterManual before condition check
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
        *(vu16*)ctrl_l = 0x8A00 | (HINT_SCANLINE_MID_SCREEN - 1); //VDP_setHIntCounter(HINT_SCANLINE_MID_SCREEN - 1);
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
        "move.w  %[_HINT_COUNTER],(0xC00004)\n\t" // VDP_setHIntCounter(HINT_SCANLINE_MID_SCREEN-3);
        "move.w  %[hint_callback],%[hintCaller]+4\n\t" // SYS_setHIntCallback(hint_mirror_planes_last_scanline_callback);
        "1:"
        : [mirror_offset_rows] "+m" (mirror_offset_rows), [vCounterManual] "+m" (vCounterManual)
        : [_VSRAM_CMD] "i" (VDP_WRITE_VSRAM_ADDR(0)), [NEXT_ROW_OFFSET] "i" ((2 << 16) | 2),
          [_HINT_COUNTER] "i" (0x8A00 | (HINT_SCANLINE_MID_SCREEN - 1)),
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
    #if RENDER_SET_FLOOR_AND_ROOF_COLORS_ON_HINT
    waitHCounter_opt1(156);
    *ctrl_l = VDP_WRITE_CRAM_ADDR(0 * 2); // CRAM index 0
    *(vu16*)data_l = 0x0444; // palette_grey[2]=0x0444 floor color
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
        #if RENDER_SET_FLOOR_AND_ROOF_COLORS_ON_HINT
        "1:\n\t"
        "cmpi.b  %[hcLimit],%c[_HCOUNTER_PORT]\n\t" // cmpi: (0xC00009) - n. Compares byte because hcLimit won't be > 160 for our practical cases
        "blo.s   1b\n\t"                            // loop back if n is lower than (0xC00009)
        "move.l  %[_CRAM_CMD],(0xC00004)\n\t" // *ctrl_l = VDP_WRITE_CRAM_ADDR(0 * 2); // CRAM index 0
        "move.w  %[floor_color],(0xC00000)"   // *(vu16*)data_l = 0x0444; // palette_grey[2]=0x0444 floor color
        #endif
        :
        : [hcLimit] "i" (156), [_HCOUNTER_PORT] "i" (VDP_HVCOUNTER_PORT + 1), // HCounter address is 0xC00009
          [_CRAM_CMD] "i" (VDP_WRITE_CRAM_ADDR(0 * 2)), [floor_color] "i" (0x0444),
          [_VSRAM_CMD] "i" (VDP_WRITE_VSRAM_ADDR(0)), [_HINT_COUNTER] "i" (0x8A00 | 255),
          [hint_callback] "s" (hint_load_hud_pals_callback), [hintCaller] "m" (hintCaller)
        : "cc", "memory"
    );
}