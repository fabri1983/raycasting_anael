#include "vint_callback.h"
#include <vdp.h>
#include <vdp_bg.h>
#include <vdp_spr.h>
#include <pal.h>
#include <memory.h>
#include "hud.h"
#include "utils.h"

static void* tiles_data;
static u16 tiles_toIndex;
static u16 tiles_lenInWord;

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
    tiles_data = NULL;
    tiles_toIndex = 0;
    tiles_lenInWord = 0;

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

void vint_enqueueTiles (void *from, u16 toIndex, u16 lenInWord)
{
    tiles_data = from;
    tiles_toIndex = toIndex;
    tiles_lenInWord = lenInWord;
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
	u32 palCmd = VDP_DMA_CRAM_ADDR(((HUD_PAL+0) * 16 + 1) * 2); // target starting color index multiplied by 2
    u32 fromAddrForDMA = (u32) (palette_green + 1) >> 1; // TODO: this can be set outside the HInt (or maybe the compiler does it already)
	turnOffVDP(0x74);
    setupDMAForPals(7, fromAddrForDMA);
    *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
	palCmd = VDP_DMA_CRAM_ADDR(((HUD_PAL+1) * 16 + 1) * 2); // target starting color index multiplied by 2
    fromAddrForDMA = (u32) (palette_blue + 1) >> 1; // TODO: this can be set outside the HInt (or maybe the compiler does it already)
    setupDMAForPals(7, fromAddrForDMA);
    *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
    turnOnVDP(0x74);
    #endif

	// clear the frame buffer
	#if RENDER_CLEAR_FRAMEBUFFER_WITH_SP
	//clear_buffer_sp(frame_buffer);
	#else
	//clear_buffer(frame_buffer);
	#endif

    // Have any tiles to DMA?
    if (tiles_data) {
        DMA_doDma(DMA_VRAM, tiles_data, tiles_toIndex, tiles_lenInWord, 2);
        tiles_data = NULL;
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