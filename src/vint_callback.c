#include "vint_callback.h"
#include <vdp.h>
#include <vdp_bg.h>
#include <vdp_spr.h>
#include <pal.h>
#include <memory.h>
#include "hud.h"
#include "hud_320.h"
#include "weapons.h"
#include "utils.h"

#if HUD_RELOAD_OVERRIDEN_PALETTES_AT_HINT == FALSE
u32 restorePalA_addrForDMA;
u32 restorePalB_addrForDMA;
#endif

#if DMA_ENQUEUE_HUD_TILEMPS_IN_VINT
u8 hud_tilemaps;
#endif

static u8 tiles_elems;
static void* tiles_from[DMA_MAX_QUEUE_CAPACITY] = {0};
static u16 tiles_toIndex[DMA_MAX_QUEUE_CAPACITY] = {0};
static u16 tiles_lenInWord[DMA_MAX_QUEUE_CAPACITY] = {0};

#if DMA_ALLOW_BUFFERED_TILES
static u8 tiles_buf_elems;
static u16 tiles_buf_toIndex[DMA_MAX_QUEUE_CAPACITY] = {0};
static u16 tiles_buf_lenInWord[DMA_MAX_QUEUE_CAPACITY] = {0};
static u16* tiles_buf_dmaBufPtr;
#endif

#if DMA_ENQUEUE_VDP_SPRITE_CACHE_IN_VINT
static u16 vdpSpriteCache_lenInWord;
#endif

void vint_reset ()
{
    #if HUD_RELOAD_OVERRIDEN_PALETTES_AT_HINT == FALSE
    restorePalA_addrForDMA = 0;
    restorePalB_addrForDMA = 0;
    #endif

    #if DMA_ENQUEUE_HUD_TILEMPS_IN_VINT
    hud_tilemaps = 0;
    #endif

    memset(tiles_from, 0, DMA_MAX_QUEUE_CAPACITY);
    memsetU16(tiles_toIndex, 0, DMA_MAX_QUEUE_CAPACITY);
    memsetU16(tiles_lenInWord, 0, DMA_MAX_QUEUE_CAPACITY);
    tiles_elems = 0;

    #if DMA_ALLOW_BUFFERED_TILES
    memsetU16(tiles_buf_toIndex, 0, DMA_MAX_QUEUE_CAPACITY);
    memsetU16(tiles_buf_lenInWord, 0, DMA_MAX_QUEUE_CAPACITY);
    tiles_buf_elems = 0;
    tiles_buf_dmaBufPtr = dmaDataBuffer;
    #endif

    #if DMA_ENQUEUE_VDP_SPRITE_CACHE_IN_VINT
    vdpSpriteCache_lenInWord = 0;
    #endif
}

FORCE_INLINE void vint_enqueueHudTilemap ()
{
    #if DMA_ENQUEUE_HUD_TILEMPS_IN_VINT
    hud_tilemaps = 1;
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
    #if DMA_ALLOW_BUFFERED_TILES
    tiles_buf_toIndex[tiles_buf_elems] = toIndex;
    tiles_buf_lenInWord[tiles_buf_elems] = lenInWord;
    ++tiles_buf_elems;
    tiles_buf_dmaBufPtr += lenInWord;
    #endif
}

void vint_enqueueVdpSpriteCache (u16 lenInWord)
{
    #if DMA_ENQUEUE_VDP_SPRITE_CACHE_IN_VINT
    vdpSpriteCache_lenInWord = lenInWord;
    #endif
}

void vint_callback ()
{
	// NOTE: AT THIS POINT THE SGDK's DMA QUEUE HAS BEEN FLUSHED

    #if HUD_RELOAD_OVERRIDEN_PALETTES_AT_HINT == FALSE
	// Reload the 2 palettes that were overriden by the HUD palettes
	u32 palCmd_restore = VDP_DMA_CRAM_ADDR(((WEAPON_BASE_PAL+0) * 16 + 1) * 2); // target starting color index multiplied by 2
    turnOffVDP(0x74);
    setupDMAForPals(15, restorePalA_addrForDMA);
    *((vu32*) VDP_CTRL_PORT) = palCmd_restore; // trigger DMA transfer
	// palCmd_restore = VDP_DMA_CRAM_ADDR(((WEAPON_BASE_PAL+1) * 16 + 1) * 2); // target starting color index multiplied by 2
    // setupDMAForPals(15, restorePalB_addrForDMA);
    // *((vu32*) VDP_CTRL_PORT) = palCmd_restore; // trigger DMA transfer
	turnOnVDP(0x74);
    #endif

	// clear the frame buffer
	#if RENDER_CLEAR_FRAMEBUFFER_WITH_SP
	//clear_buffer_sp(frame_buffer);
	#else
	//clear_buffer(frame_buffer);
	#endif

    #if DMA_ENQUEUE_HUD_TILEMPS_IN_VINT
    // Have any hud tilemaps to DMA?
    if (hud_tilemaps) {
        // PW_ADDR comes with the correct base position in screen
        DMA_doDmaFast(DMA_VRAM, hud_getTilemap(), PW_ADDR, (PLANE_COLUMNS*HUD_BG_H) - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
        hud_tilemaps = 0;
    }
    #endif

    // Have any tiles to DMA?
    while (tiles_elems) {
        --tiles_elems;
        u16 lenInWord = tiles_lenInWord[tiles_elems];
        DMA_doDma(DMA_VRAM, tiles_from[tiles_elems], tiles_toIndex[tiles_elems], lenInWord, 2);
    }

    #if DMA_ENQUEUE_VDP_SPRITE_CACHE_IN_VINT
    // Have any update for vdp sprite cache?
    if (vdpSpriteCache_lenInWord) {
        DMA_doDmaFast(DMA_VRAM, vdpSpriteCache, VDP_SPRITE_TABLE, vdpSpriteCache_lenInWord, 2);
        vdpSpriteCache_lenInWord = 0;
    }
    #endif

    #if DMA_ALLOW_BUFFERED_TILES
    // Have any buffered tiles to DMA?
    while (tiles_buf_elems) {
        --tiles_buf_elems;
        u16 lenInWord = tiles_buf_lenInWord[tiles_buf_elems];
        tiles_buf_dmaBufPtr -= lenInWord;
        u16 toIndex = tiles_buf_toIndex[tiles_buf_elems];
        DMA_doDmaFast(DMA_VRAM, tiles_buf_dmaBufPtr, toIndex, lenInWord, 2);
        DMA_releaseTemp(lenInWord);
    }
    #endif
}