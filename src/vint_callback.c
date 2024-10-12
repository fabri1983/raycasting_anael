#include "vint_callback.h"
#include <vdp.h>
#include <vdp_bg.h>
#include <pal.h>
#include "hud.h"
#include "hud_res.h"
#include "utils.h"

void vint_callback ()
{
	// NOTE: AT THIS POINT THE DMA QUEUE HAS BEEN FLUSHED

	// Reload the 2 palettes that were overriden by the HUD palettes
	u32 palCmd = VDP_DMA_CRAM_ADDR(((HUD_PAL+0) * 16 + 1) * 2); // target starting color index multiplied by 2
    u32 fromAddrForDMA = (u32) (palette_green + 1) >> 1; // TODO: this can be set outside the HInt (or maybe the compiler does it already)
	turnOffVDP(0x74);
    setupDMAForPals(8, fromAddrForDMA);
    *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
	palCmd = VDP_DMA_CRAM_ADDR(((HUD_PAL+1) * 16 + 1) * 2); // target starting color index multiplied by 2
    fromAddrForDMA = (u32) (palette_blue + 1) >> 1; // TODO: this can be set outside the HInt (or maybe the compiler does it already)
    setupDMAForPals(8, fromAddrForDMA);
    *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
    turnOnVDP(0x74);

	// clear the frame buffer
	#if USE_CLEAR_FRAMEBUFFER_WITH_SP
	//clear_buffer_sp(frame_buffer);
	#else
	//clear_buffer(frame_buffer);
	#endif
}