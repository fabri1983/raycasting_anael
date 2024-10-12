#include "hint_callback.h"
#include <vdp_bg.h>
#include <vdp.h>
#include <pal.h>
#include "hud.h"
#include "hud_res.h"
#include "weapons.h"

static u32 hudPalA_addrForDMA;
static u32 hudPalB_addrForDMA;
static u32 weaponPalA_addrForDMA;
static u32 weaponPalB_addrForDMA;

void hint_setupPals ()
{
    hudPalA_addrForDMA = (u32) (img_hud_spritesheet.palette->data +  0 + 1) >> 1;
    hudPalB_addrForDMA = (u32) (img_hud_spritesheet.palette->data + 16 + 1) >> 1;
    weaponPalA_addrForDMA = NULL;
    weaponPalB_addrForDMA = NULL;
}

FORCE_INLINE void hint_enqueueWeaponPal (u16* pal)
{
    // PAL_setColors(WEAPON_BASE_PAL*16 + 9, pal + 9, 7, DMA_QUEUE);
    // PAL_setColors((WEAPON_BASE_PAL+1)*16 + 9, pal + 16 + 9, 7, DMA_QUEUE);
    weaponPalA_addrForDMA = (u32) (pal +  0 + 9) >> 1;
    weaponPalB_addrForDMA = (u32) (pal + 16 + 9) >> 1;
}

HINTERRUPT_CALLBACK hint_callback ()
{
    #if USE_DIF_FLOOR_AND_ROOF_COLORS
	if (GET_VCOUNTER <= HUD_HINT_SCANLINE_CHANGE_BG_COLOR) {
		waitHCounter_DMA(156);
		// set background color used for the floor
        const u16 addr = 0 * 2;
        *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_CRAM_ADDR((u32)addr);
        *((vu16*) VDP_DATA_PORT) = 0x0444; //palette_grey[2];
		return;
	}
    #endif

	// We are one scanline earlier so wait until entering the HBlank region
	waitHCounter_DMA(152);

	// Prepare DMA cmd and source address for first palette
    u32 palCmd = VDP_DMA_CRAM_ADDR(((HUD_PAL+0) * 16 + 1) * 2); // target starting color index multiplied by 2
    turnOffVDP(0x74);
    setupDMAForPals(15, hudPalA_addrForDMA);

    // At this moment we are at the middle/end of the scanline due to the previous DMA setup.
    // So we need to wait for next HBlank (indeed some pixels before to absorb some overhead)
    waitHCounter_DMA(152);

    *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
    turnOnVDP(0x74);

	// Prepare DMA cmd and source address for second palette
    palCmd = VDP_DMA_CRAM_ADDR(((HUD_PAL+1) * 16 + 1) * 2); // target starting color index multiplied by 2
    turnOffVDP(0x74);
    setupDMAForPals(15, hudPalB_addrForDMA);

	// At this moment we are at the middle/end of the scanline due to the previous DMA setup.
    // So we need to wait for next HBlank (indeed some pixels before to absorb some overhead)
    waitHCounter_DMA(152);

    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
    turnOnVDP(0x74);

    #if USE_DIF_FLOOR_AND_ROOF_COLORS
	// set background color used for the roof
	waitHCounter_DMA(152);
	u32 addr = 0 * 2; // color index 0
    *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_CRAM_ADDR(addr);
    *((vu16*) VDP_DATA_PORT) = 0x0222; //palette_grey[1];
    #endif

    // Have any weapon palette to DMA
    if (weaponPalA_addrForDMA != NULL) {

        palCmd = VDP_DMA_CRAM_ADDR(((WEAPON_BASE_PAL+0) * 16 + 9) * 2); // target starting color index multiplied by 2
        waitHCounter_DMA(152);
        setupDMAForPals(7, weaponPalA_addrForDMA);
        waitHCounter_DMA(152);
        *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer

        palCmd = VDP_DMA_CRAM_ADDR(((WEAPON_BASE_PAL+1) * 16 + 9) * 2); // target starting color index multiplied by 2
        waitHCounter_DMA(152);
        setupDMAForPals(7, weaponPalB_addrForDMA);
        waitHCounter_DMA(152);
        *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer

        weaponPalA_addrForDMA = NULL;
    }

	// Send 1/2 of the PA
	//DMA_doDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, (VERTICAL_ROWS*PLANE_COLUMNS)/2 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
	// Send 1/4 of the PA
	//DMA_doDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, (VERTICAL_ROWS*PLANE_COLUMNS)/4 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);

	// Reload the 2 palettes that were overriden by the HUD palettes
	// waitVCounterReg(224);
	// palCmd = VDP_DMA_CRAM_ADDR(((HUD_PAL+0) * 16 + 1) * 2); // target starting color index multiplied by 2
    // u32 fromAddrForDMA = (u32) (palette_green + 1) >> 1; // TODO: this can be set outside the HInt (or maybe the compiler does it already)
	// turnOffVDP(0x74);
    // setupDMAForPals(8, fromAddrForDMA);
    // *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
	// palCmd = VDP_DMA_CRAM_ADDR(((HUD_PAL+1) * 16 + 1) * 2); // target starting color index multiplied by 2
    // fromAddrForDMA = (u32) (palette_blue + 1) >> 1; // TODO: this can be set outside the HInt (or maybe the compiler does it already)
    // setupDMAForPals(8, fromAddrForDMA);
    // *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
	// turnOnVDP(0x74);
}