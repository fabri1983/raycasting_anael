#include "hint_callback.h"
#include <vdp_bg.h>
#include <vdp.h>
#include <vdp_spr.h>
#include <pal.h>
#include <memory.h>
#include "hud.h"
#include "hud_320.h"
#include "weapons.h"

u32 hudPalA_addrForDMA;
u32 hudPalB_addrForDMA;
u32 restorePalA_addrForDMA;
u32 restorePalB_addrForDMA;

u32 weaponPalA_addrForDMA;
u32 weaponPalB_addrForDMA;

u8 hud_tilemaps;

u16 tilesLenInWordTotalToDMA;

u8 tiles_elems;
void* tiles_from[DMA_MAX_WORDS_QUEUE] = {0};
u16 tiles_toIndex[DMA_MAX_WORDS_QUEUE] = {0};
u16 tiles_lenInWord[DMA_MAX_WORDS_QUEUE] = {0};

u8 tiles_buf_elems;
u16 tiles_buf_toIndex[DMA_MAX_WORDS_QUEUE] = {0};
u16 tiles_buf_lenInWord[DMA_MAX_WORDS_QUEUE] = {0};
u16* tiles_buf_dmaBufPtr;

static u16 vdpSpriteCache_lenInWord;

void hint_reset ()
{
    weaponPalA_addrForDMA = NULL;
    weaponPalB_addrForDMA = NULL;

    hud_tilemaps = 0;

    tilesLenInWordTotalToDMA = 0;

    memset(tiles_from, 0, DMA_MAX_WORDS_QUEUE);
    memsetU16(tiles_toIndex, 0, DMA_MAX_WORDS_QUEUE);
    memsetU16(tiles_lenInWord, 0, DMA_MAX_WORDS_QUEUE);
    tiles_elems = 0;

    memsetU16(tiles_buf_toIndex, 0, DMA_MAX_WORDS_QUEUE);
    memsetU16(tiles_buf_lenInWord, 0, DMA_MAX_WORDS_QUEUE);
    tiles_buf_elems = 0;
    tiles_buf_dmaBufPtr = dmaDataBuffer;

    vdpSpriteCache_lenInWord = 0;
}

FORCE_INLINE void hint_setupPals ()
{
    hud_setup_hint_pals(&hudPalA_addrForDMA, &hudPalB_addrForDMA);
    restorePalA_addrForDMA = (u32) (palette_green + 1) >> 1;
    restorePalB_addrForDMA = (u32) (palette_blue + 1) >> 1;
}

FORCE_INLINE bool canDMAinHint (u16 lenInWord)
{
    return tilesLenInWordTotalToDMA + lenInWord <= DMA_LENGTH_IN_WORD_THRESHOLD_FOR_HINT;
}

void hint_enqueueWeaponPal (u16* pal)
{
    // This was the old way
    // PAL_setColors(WEAPON_BASE_PAL*16 + 8, pal + 8, 8, DMA_QUEUE);
    // PAL_setColors((WEAPON_BASE_PAL+1)*16 + 9, pal + 16 + 8, 8, DMA_QUEUE);

    weaponPalA_addrForDMA = (u32) (pal +  0 + 8) >> 1;
    //weaponPalB_addrForDMA = (u32) (pal + 16 + 8) >> 1;
}

void hint_enqueueHudTilemap ()
{
    hud_tilemaps = 1;
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
    tiles_buf_toIndex[tiles_buf_elems] = toIndex;
    tiles_buf_lenInWord[tiles_buf_elems] = lenInWord;
    ++tiles_buf_elems;
    tiles_buf_dmaBufPtr += lenInWord;
    tilesLenInWordTotalToDMA += lenInWord;
}

void hint_enqueueVdpSpriteCache (u16 lenInWord)
{
    vdpSpriteCache_lenInWord = lenInWord;
}

HINTERRUPT_CALLBACK hint_callback ()
{
    #if HUD_USE_DIF_FLOOR_AND_ROOF_COLORS
	if (GET_VCOUNTER <= HUD_HINT_SCANLINE_CHANGE_ROOF_BG_COLOR) {
		// set background color used for the floor
		waitHCounter_DMA(156);
        const u16 addr = 0 * 2; // CRAM index 0
        *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_CRAM_ADDR((u32)addr);
        *((vu16*) VDP_DATA_PORT) = 0x0444; //palette_grey[2]; // floor color
		return;
	}
    #endif

	// Prepare DMA cmd and source address for first palette
    u32 palCmd = VDP_DMA_CRAM_ADDR(((HUD_PAL+0) * 16 + 1) * 2); // target starting color index multiplied by 2

	// We are one scanline earlier so wait until entering the HBlank region
	waitHCounter_DMA(152);

    //turnOffVDP(0x74);
    setupDMAForPals(15, hudPalA_addrForDMA);
    //turnOnVDP(0x74);

    // At this moment we are at the middle/end of the scanline due to the previous DMA setup.
    // So we need to wait for next HBlank (indeed some pixels before to absorb some overhead)
    waitHCounter_DMA(152);

    //turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
    //turnOnVDP(0x74);

	// Prepare DMA cmd and source address for second palette
    palCmd = VDP_DMA_CRAM_ADDR(((HUD_PAL+1) * 16 + 1) * 2); // target starting color index multiplied by 2
    //turnOffVDP(0x74);
    setupDMAForPals(15, hudPalB_addrForDMA);
    //turnOnVDP(0x74);

	// At this moment we are at the middle/end of the scanline due to the previous DMA setup.
    // So we need to wait for next HBlank (indeed some pixels before to absorb some overhead)
    waitHCounter_DMA(152);

    //turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
    //turnOnVDP(0x74);

    #if HUD_USE_DIF_FLOOR_AND_ROOF_COLORS
	// set background color used for the roof
	waitHCounter_DMA(156);
	u32 addr = 0 * 2; // color index 0
    *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_CRAM_ADDR(addr);
    *((vu16*) VDP_DATA_PORT) = 0x0222; //palette_grey[1]; // roof color
    #endif

    // Have any hud tilemaps to DMA?
    if (hud_tilemaps) {
        DMA_doDmaFast(DMA_VRAM, hud_getTilemap(), PW_ADDR, (PLANE_COLUMNS*HUD_BG_H) - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
        hud_tilemaps = 0;
    }

    // Have any tiles to DMA?
    while (tiles_elems) {
        --tiles_elems;
        u16 lenInWord = tiles_lenInWord[tiles_buf_elems];
        tilesLenInWordTotalToDMA -= lenInWord;
        DMA_doDma(DMA_VRAM, tiles_from[tiles_elems], tiles_toIndex[tiles_elems], lenInWord, 2);
    }

    // Havy any buffered tiles to DMA?
    while (tiles_buf_elems) {
        --tiles_buf_elems;
        u16 lenInWord = tiles_buf_lenInWord[tiles_buf_elems];
        tilesLenInWordTotalToDMA -= lenInWord;
        tiles_buf_dmaBufPtr -= lenInWord;
        u16 toIndex = tiles_buf_toIndex[tiles_buf_elems];
        DMA_doDmaFast(DMA_VRAM, tiles_buf_dmaBufPtr, toIndex, lenInWord, 2);
        DMA_releaseTemp(lenInWord);
    }

    // Have any update for vdp sprite cache?
    if (vdpSpriteCache_lenInWord) {
        DMA_doDmaFast(DMA_VRAM, vdpSpriteCache, VDP_SPRITE_TABLE, vdpSpriteCache_lenInWord, 2);
        vdpSpriteCache_lenInWord = 0;
    }

    // Have any weapon palette to DMA?
    if (weaponPalA_addrForDMA) {

        palCmd = VDP_DMA_CRAM_ADDR(((WEAPON_BASE_PAL+0) * 16 + 8) * 2); // target starting color index multiplied by 2
        waitHCounter_DMA(152);
        setupDMAForPals(8, weaponPalA_addrForDMA);
        waitHCounter_DMA(152);
        turnOffVDP(0x74);
        *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
        turnOnVDP(0x74);

        // palCmd = VDP_DMA_CRAM_ADDR(((WEAPON_BASE_PAL+1) * 16 + 8) * 2); // target starting color index multiplied by 2
        // waitHCounter_DMA(152);
        // setupDMAForPals(8, weaponPalB_addrForDMA);
        // waitHCounter_DMA(152);
        // turnOffVDP(0x74);
        // *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
        // turnOnVDP(0x74);

        weaponPalA_addrForDMA = NULL;
    }

    // USE THIS TO SEE HOW MUCH DEEP IN THE HUD IMAGE ALL THE TILES DMA GO
    // turnOffVDP(0x74);
    // waitHCounter_DMA(160);
    // turnOnVDP(0x74);

	// Send first 1/2 of frame_buffer Plane A
	//DMA_doDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, (VERTICAL_ROWS*PLANE_COLUMNS)/2 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
	// Send first 1/4 of frame_buffer Plane A
	//DMA_doDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, (VERTICAL_ROWS*PLANE_COLUMNS)/4 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);

    #if HUD_RELOAD_OVERRIDEN_PALETTES_AT_HINT
	// Reload the 2 palettes that were overriden by the HUD palettes
	palCmd = VDP_DMA_CRAM_ADDR(((HUD_PAL+0) * 16 + 1) * 2); // target starting color index multiplied by 2

	waitVCounterReg(224);
	
    turnOffVDP(0x74);
    setupDMAForPals(7, restorePalA_addrForDMA);
    *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
    
	palCmd = VDP_DMA_CRAM_ADDR(((HUD_PAL+1) * 16 + 1) * 2); // target starting color index multiplied by 2
    setupDMAForPals(7, restorePalB_addrForDMA);
    *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
	turnOnVDP(0x74);
    #endif
}