#include "hint_callback.h"
#include <vdp_bg.h>
#include <vdp.h>
#include <vdp_spr.h>
#include <pal.h>
#include <memory.h>
#include "hud.h"
#include "hud_320.h"
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

#if DMA_ENQUEUE_HUD_TILEMPS_IN_HINT
u8 hud_tilemaps;
#endif

u16 tilesLenInWordTotalToDMA;

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

#if DMA_ENQUEUE_VDP_SPRITE_CACHE_IN_HINT
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

    #if DMA_ENQUEUE_HUD_TILEMPS_IN_HINT
    hud_tilemaps = 0;
    #endif

    tilesLenInWordTotalToDMA = 0;

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

    #if DMA_ENQUEUE_VDP_SPRITE_CACHE_IN_HINT
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
    #if DMA_ENQUEUE_HUD_TILEMPS_IN_HINT
    hud_tilemaps = 1;
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
    #if DMA_ALLOW_BUFFERED_TILES
    tiles_buf_toIndex[tiles_buf_elems] = toIndex;
    tiles_buf_lenInWord[tiles_buf_elems] = lenInWord;
    ++tiles_buf_elems;
    tiles_buf_dmaBufPtr += lenInWord;
    tilesLenInWordTotalToDMA += lenInWord;
    #endif
}

void hint_enqueueVdpSpriteCache (u16 lenInWord)
{
    #if DMA_ENQUEUE_VDP_SPRITE_CACHE_IN_HINT
    vdpSpriteCache_lenInWord = lenInWord;
    #endif
}

HINTERRUPT_CALLBACK hint_callback ()
{
    #if HUD_SET_FLOOR_AND_ROOF_COLORS_ON_HINT && !HUD_SET_FLOOR_AND_ROOF_COLORS_ON_WRITE_VLINE
	if (GET_VCOUNTER <= HUD_HINT_SCANLINE_CHANGE_ROOF_BG_COLOR) {
		// set background color used for the floor
		waitHCounter_opt2(156);
        const u16 addr = 0 * 2; // CRAM index 0
        *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_CRAM_ADDR((u32)addr);
        *((vu16*) VDP_DATA_PORT) = 0x0444; //palette_grey[2]; // floor color
		return;
	}
    #endif

	// Prepare DMA cmd and source address for first palette
    u32 palCmd = VDP_DMA_CRAM_ADDR(((HUD_PAL+0) * 16 + 1) * 2); // target starting color index multiplied by 2

	// We are one scanline earlier so wait until entering the HBlank region
	waitHCounter_opt2(152);

    //turnOffVDP(0x74);
    setupDMAForPals(15, hudPalA_addrForDMA);
    //turnOnVDP(0x74);

    // At this moment we are at the middle/end of the scanline due to the previous DMA setup.
    // So we need to wait for next HBlank (indeed some pixels before to absorb some overhead)
    waitHCounter_opt2(152);

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
    waitHCounter_opt2(152);

    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
    turnOnVDP(0x74);

    #if HUD_SET_FLOOR_AND_ROOF_COLORS_ON_HINT && !HUD_SET_FLOOR_AND_ROOF_COLORS_ON_WRITE_VLINE
	// set background color used for the roof
	waitHCounter_opt2(156);
	u32 addr = 0 * 2; // color index 0
    *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_CRAM_ADDR(addr);
    *((vu16*) VDP_DATA_PORT) = 0x0222; //palette_grey[1]; // roof color
    #endif

    #if DMA_ENQUEUE_HUD_TILEMPS_IN_HINT
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
        tilesLenInWordTotalToDMA -= lenInWord;
        DMA_doDma(DMA_VRAM, tiles_from[tiles_elems], tiles_toIndex[tiles_elems], lenInWord, 2);
    }

    #if DMA_ENQUEUE_VDP_SPRITE_CACHE_IN_HINT
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
        tilesLenInWordTotalToDMA -= lenInWord;
        tiles_buf_dmaBufPtr -= lenInWord;
        u16 toIndex = tiles_buf_toIndex[tiles_buf_elems];
        DMA_doDmaFast(DMA_VRAM, tiles_buf_dmaBufPtr, toIndex, lenInWord, 2);
        DMA_releaseTemp(lenInWord);
    }
    #endif

    // Have any weapon palette to DMA?
    /*if (weaponPalA_addrForDMA) {

        u32 palCmd_weapon = VDP_DMA_CRAM_ADDR(((WEAPON_BASE_PAL+0) * 16 + 1) * 2); // target starting color index multiplied by 2
        waitHCounter_opt2(152);
        setupDMAForPals(15, weaponPalA_addrForDMA);
        waitHCounter_opt2(152);
        turnOffVDP(0x74);
        *((vu32*) VDP_CTRL_PORT) = palCmd_weapon; // trigger DMA transfer
        turnOnVDP(0x74);

        // palCmd_weapon = VDP_DMA_CRAM_ADDR(((WEAPON_BASE_PAL+1) * 16 + 1) * 2); // target starting color index multiplied by 2
        // waitHCounter_opt2(152);
        // setupDMAForPals(15, weaponPalB_addrForDMA);
        // waitHCounter_opt2(152);
        // turnOffVDP(0x74);
        // *((vu32*) VDP_CTRL_PORT) = palCmd_weapon; // trigger DMA transfer
        // turnOnVDP(0x74);

        weaponPalA_addrForDMA = NULL;
    }*/

    #if DMA_FRAMEBUFFER_A_EIGHT_CHUNKS_ON_DISPLAY_PERIOD_AND_HINT
    // Send next 1/8 of frame_buffer Plane A
    DMA_doDmaFast(DMA_VRAM, frame_buffer + (VERTICAL_ROWS*PLANE_COLUMNS)/8, PA_ADDR + EIGHTH_PLANE_ADDR_OFFSET, 
        (VERTICAL_ROWS*PLANE_COLUMNS)/8 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
    #elif DMA_FRAMEBUFFER_A_FIRST_QUARTER_ON_HINT
	// Send first 1/4 of frame_buffer Plane A
	DMA_doDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, (VERTICAL_ROWS*PLANE_COLUMNS)/4 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
    #elif DMA_FRAMEBUFFER_A_FIRST_HALF_ON_HINT
	// Send first 1/2 of frame_buffer Plane A
	DMA_doDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, (VERTICAL_ROWS*PLANE_COLUMNS)/2 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
    #else
    #endif

    // USE THIS TO SEE HOW MUCH DEEP IN THE HUD IMAGE ALL THE DMA GOES
    // turnOffVDP(0x74);
    // waitHCounter_opt2(160);
    // turnOnVDP(0x74);

    #if HUD_RELOAD_OVERRIDEN_PALETTES_AT_HINT
	// Reload the 2 palettes that were overriden by the HUD palettes
	u32 palCmd_restore = VDP_DMA_CRAM_ADDR(((WEAPON_BASE_PAL+0) * 16 + 1) * 2); // target starting color index multiplied by 2
    setupDMAForPals(15, restorePalA_addrForDMA);
	waitVCounterReg(224);
    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = palCmd_restore; // trigger DMA transfer
	// palCmd_restore = VDP_DMA_CRAM_ADDR(((WEAPON_BASE_PAL+1) * 16 + 1) * 2); // target starting color index multiplied by 2
    // setupDMAForPals(15, restorePalB_addrForDMA);
    // *((vu32*) VDP_CTRL_PORT) = palCmd_restore; // trigger DMA transfer
	turnOnVDP(0x74);
    #endif
}