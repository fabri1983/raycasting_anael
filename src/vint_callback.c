#include "vint_callback.h"
#include <vdp.h>
#include <vdp_bg.h>
#include <vdp_spr.h>
#include <pal.h>
#include <memory.h>
#include "consts.h"
#include "consts_ext.h"
#include "hud.h"
#include "hud_320.h"
#include "weapons.h"
#include "utils.h"
#include "render.h"
#include "hint_callback.h"

#if HUD_RELOAD_OVERRIDEN_PALETTES_AT_HINT == FALSE
u32 restorePalA_addrForDMA;
u32 restorePalB_addrForDMA;
#endif

#if DMA_ENQUEUE_HUD_TILEMAP_TO_FLUSH_AT_VINT
u8 hud_tilemap;
#endif

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

#if DMA_ENQUEUE_VDP_SPRITE_CACHE_TO_FLUSH_AT_VINT
static u16 vdpSpriteCache_lenInWord;
#endif

void vint_reset ()
{
    #if HUD_RELOAD_OVERRIDEN_PALETTES_AT_HINT == FALSE
    restorePalA_addrForDMA = 0;
    restorePalB_addrForDMA = 0;
    #endif

    #if DMA_ENQUEUE_HUD_TILEMAP_TO_FLUSH_AT_VINT
    hud_tilemap = 0;
    #endif

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

    #if DMA_ENQUEUE_VDP_SPRITE_CACHE_TO_FLUSH_AT_VINT
    vdpSpriteCache_lenInWord = 0;
    #endif
}

FORCE_INLINE void vint_enqueueHudTilemap ()
{
    #if DMA_ENQUEUE_HUD_TILEMAP_TO_FLUSH_AT_VINT
    hud_tilemap = 1;
    #endif
}

FORCE_INLINE void vint_setPalToRestore (u16* pal)
{
    #if HUD_RELOAD_OVERRIDEN_PALETTES_AT_HINT == FALSE
    restorePalA_addrForDMA = (u32) (pal + 1) >> 1;
    //restorePalB_addrForDMA = (u32) (pal + 16 + 1) >> 1;
    #endif
}

void vint_enqueueTiles (void* from, u16 toIndex, u16 lenInWord)
{
    tiles_from[tiles_elems] = from;
    tiles_toIndex[tiles_elems] = toIndex;
    tiles_lenInWord[tiles_elems] = lenInWord;
    ++tiles_elems;
}

void vint_enqueueTilesBuffered (u16 toIndex, u16 lenInWord)
{
    #if DMA_ALLOW_BUFFERED_SPRITE_TILES
    tiles_buf_toIndex[tiles_buf_elems] = toIndex;
    tiles_buf_lenInWord[tiles_buf_elems] = lenInWord;
    ++tiles_buf_elems;
    tiles_buf_dmaBufPtr += lenInWord;
    #endif
}

void vint_enqueueVdpSpriteCache (u16 lenInWord)
{
    #if DMA_ENQUEUE_VDP_SPRITE_CACHE_TO_FLUSH_AT_VINT
    vdpSpriteCache_lenInWord = lenInWord;
    #endif
}

void vint_callback ()
{
    #if RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT || RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT_MULTI_CALLBACKS
    hint_reset_mirror_planes_state();
    #elif HUD_SET_FLOOR_AND_ROOF_COLORS_ON_HINT
    hint_reset_change_bg_state();
    #endif

	turnOffVDP(0x74);

    render_Z80_setBusProtection(TRUE);
	//waitSubTick_(10); // Z80 delay --> wait a bit (10 ticks) to improve PCM playback (test on SOR2)

    render_DMA_flushQueue();

    #if DMA_FRAMEBUFFER_ROW_BY_ROW
    render_DMA_row_by_row_framebuffer();
    #endif

	render_Z80_setBusProtection(FALSE);

    #if HUD_RELOAD_OVERRIDEN_PALETTES_AT_HINT == FALSE
    vu32* vdpCtrl_ptr_l = (vu32*) VDP_CTRL_PORT;
	// Reload the palettes that were overriden by the HUD palettes
    setupDMAForPals(15, restorePalA_addrForDMA);
	u32 palCmd_restore = VDP_DMA_CRAM_ADDR(((WEAPON_BASE_PAL+0) * 16 + 1) * 2); // target starting color index multiplied by 2
    *vdpCtrl_ptr_l = palCmd_restore; // trigger DMA transfer
    // setupDMAForPals(15, restorePalB_addrForDMA);
	// palCmd_restore = VDP_DMA_CRAM_ADDR(((WEAPON_BASE_PAL+1) * 16 + 1) * 2); // target starting color index multiplied by 2
    // *vdpCtrl_ptr_l = palCmd_restore; // trigger DMA transfer
    #endif

    #if DMA_ENQUEUE_HUD_TILEMAP_TO_FLUSH_AT_VINT
    // Have any hud tilemaps to DMA?
    if (hud_tilemap) {
        hud_tilemap = 0;
        // PW_ADDR comes with the correct base position in screen
        //DMA_doDmaFast(DMA_VRAM, hud_getTilemap(), PW_ADDR, (PLANE_COLUMNS*HUD_BG_H) - (PLANE_COLUMNS-TILEMAP_COLUMNS), -1);
        u32 fromAddr = HUD_TILEMAP_DST_ADDRESS;
        #pragma GCC unroll 4 // Always set the max number since it does not accept defines
        for (u8 i=0; i < HUD_BG_H; ++i) {
            doDMAfast_fixed_args(vdpCtrl_ptr_l, fromAddr + i*PLANE_COLUMNS*2, VDP_DMA_VRAM_ADDR(PW_ADDR + i*PLANE_COLUMNS*2), TILEMAP_COLUMNS);
        }
    }
    #endif

    // Have any tiles to DMA?
    while (tiles_elems) {
        --tiles_elems;
        u16 lenInWord = tiles_lenInWord[tiles_elems];
        DMA_doDma(DMA_VRAM, tiles_from[tiles_elems], tiles_toIndex[tiles_elems], lenInWord, -1);
    }

    #if DMA_ENQUEUE_VDP_SPRITE_CACHE_TO_FLUSH_AT_VINT
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
        tiles_buf_dmaBufPtr -= lenInWord;
        u16 toIndex = tiles_buf_toIndex[tiles_buf_elems];
        DMA_doDmaFast(DMA_VRAM, tiles_buf_dmaBufPtr, toIndex, lenInWord, -1);
        DMA_releaseTemp(lenInWord);
    }
    #endif

    #if RENDER_MIRROR_PLANES_USING_VDP_VRAM
    render_DMA_halved_mirror_planes();
    #endif

    turnOnVDP(0x74);

    #if RENDER_MIRROR_PLANES_USING_VDP_VRAM
    // Called once the other half planes were effectively DMAed into VRAM.
    // Do this after the display is turned on, because is CPU and VDP intense and we don't want any black scanlines leak into active display.
    render_copy_top_entries_in_VRAM();
    #endif
}