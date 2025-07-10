#include "vint_callback.h"
#include <vdp.h>
#include <vdp_bg.h>
#include <vdp_spr.h>
#include <pal.h>
#include <memory.h>
#include <z80_ctrl.h>
#include "consts.h"
#include "consts_ext.h"
#include "hud.h"
#include "weapon_consts.h"
#if PLANE_COLUMNS == 64
#include "hud_320.h"
#else
#include "hud_256.h"
#endif
#include "utils.h"
#include "render.h"
#include "hint_callback.h"

#if DMA_ENQUEUE_HUD_TILEMAP_TO_FLUSH_AT_VINT
u8 hud_tilemap_set;
#endif

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

#if DMA_ENQUEUE_VDP_SPRITE_CACHE_TO_FLUSH_AT_VINT
static u16 vdpSpriteCache_lenInWord;
#endif

void vint_reset ()
{
    #if DMA_ENQUEUE_HUD_TILEMAP_TO_FLUSH_AT_VINT
    hud_tilemap_set= 0;
    #endif

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

    #if DMA_ENQUEUE_VDP_SPRITE_CACHE_TO_FLUSH_AT_VINT
    vdpSpriteCache_lenInWord = 0;
    #endif
}

void vint_enqueueHudTilemap ()
{
    #if DMA_ENQUEUE_HUD_TILEMAP_TO_FLUSH_AT_VINT
    hud_tilemap_set = 1;
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
    #if DMA_ALLOW_COMPRESSED_SPRITE_TILES
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
    vu32* vdpCtrl_ptr_l = (vu32*) VDP_CTRL_PORT;
	turnOffVDP_m(vdpCtrl_ptr_l, 0x74);

    #if RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT | RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT_MULTI_CALLBACKS
    hint_reset_mirror_planes_state();
    #endif

    render_Z80_setBusProtection(TRUE);
    // delay enabled ? --> wait a bit (10 ticks) to improve PCM playback (test on SOR2)
    if (Z80_getForceDelayDMA())
	    waitSubTick_(10);

    render_DMA_flushQueue();

    render_DMA_row_by_row_framebuffer();

	render_Z80_setBusProtection(FALSE);

    #if HUD_RELOAD_WEAPON_PALS_AT_VINT
	// DMA the weapon pals that were overriden gy the HUD pals
    doDMAfast_fixed_args(vdpCtrl_ptr_l, RAM_FIXED_WEAPON_PALETTES_ADDRESS + 1*2, VDP_DMA_CRAM_ADDR((WEAPON_BASE_PAL*16 + 1) * 2), 16*WEAPON_USED_PALS - 1);
    #endif

    #if DMA_ENQUEUE_HUD_TILEMAP_TO_FLUSH_AT_VINT
    // Have any hud tilemaps to DMA?
    if (hud_tilemap_set) {
        hud_tilemap_set = 0;
        #pragma GCC unroll 256 // Always set a big number since it does not accept defines
        for (u8 i=0; i < HUD_BG_H; ++i) {
            doDMAfast_fixed_args(vdpCtrl_ptr_l, RAM_FIXED_HUD_TILEMAP_DST_ADDRESS + i*TILEMAP_COLUMNS*2, VDP_DMA_VRAM_ADDR(PW_ADDR_AT_HUD + i*PLANE_COLUMNS*2), TILEMAP_COLUMNS);
        }
    }
    #endif

    // Have any tiles to DMA?
    while (tiles_elems) {
        --tiles_elems;
        u16 lenInWord = tiles_lenInWord[tiles_elems];
        // NOTE: this should be DMA_doDma() because tiles might be in in a bank > 4MB
        DMA_doDmaFast(DMA_VRAM, tiles_from[tiles_elems], tiles_toIndex[tiles_elems], lenInWord, -1);
    }

    #if DMA_ENQUEUE_VDP_SPRITE_CACHE_TO_FLUSH_AT_VINT
    // Have any update for vdp sprite cache?
    if (vdpSpriteCache_lenInWord) {
        DMA_doDmaFast(DMA_VRAM, (void*) RAM_FIXED_VDP_SPRITE_CACHE_ADDRESS, VDP_SPRITE_LIST_ADDR, vdpSpriteCache_lenInWord, -1);
        vdpSpriteCache_lenInWord = 0;
    }
    #endif

    #if DMA_ALLOW_COMPRESSED_SPRITE_TILES
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
    render_mirror_planes_in_VRAM();
    #endif

    turnOnVDP_m(vdpCtrl_ptr_l, 0x74);

    #if RENDER_MIRROR_PLANES_USING_VDP_VRAM
    // Called once the other half planes were effectively DMAed into VRAM.
    // Do this after the display is turned on, because is CPU and VDP intense and we don't want any black scanlines leak into active display.
    render_copy_top_entries_in_VRAM();
    #endif

    #if RENDER_SET_FLOOR_AND_ROOF_COLORS_ON_HINT & !(RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT | RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT_MULTI_CALLBACKS)
    hint_reset_change_bg_state();
    #endif
}