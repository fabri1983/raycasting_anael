#include "vint_callback.h"
#include <vdp.h>
#include <vdp_bg.h>
#include <pal.h>
#include "hud.h"
#include "utils.h"

static void* tiles_data;
static u16 tiles_toIndex;
static u16 tiles_lenInWord;

void vint_reset ()
{
    tiles_data = NULL;
    tiles_toIndex = 0;
    tiles_lenInWord = 0;
}

void vint_enqueueTiles (void *from, u16 toIndex, u16 lenInWord)
{
    tiles_data = from;
    tiles_toIndex = toIndex;
    tiles_lenInWord = lenInWord;
}

void vint_callback ()
{
	// NOTE: AT THIS POINT THE DMA QUEUE HAS BEEN FLUSHED

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
	#if USE_CLEAR_FRAMEBUFFER_WITH_SP
	//clear_buffer_sp(frame_buffer);
	#else
	//clear_buffer(frame_buffer);
	#endif

    // Have any tiles to DMA?
    if (tiles_data) {
        DMA_doDma(DMA_VRAM, tiles_data, tiles_toIndex, tiles_lenInWord, 2);
        tiles_data = NULL;
    }
}