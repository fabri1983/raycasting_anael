#include <types.h>
#include <vdp_tile.h>
#include <dma.h>
#include <sys.h>
#include <memory.h>
#include <vdp.h>
#include <vdp_bg.h>
#include <pal.h>
#include <tools.h>
#include <mapper.h>
#include <joy.h>
#include <libres.h>
#include "title.h"
#include "title_res.h"
#include "utils.h"

#define TITLE_256C_STRIPS_COUNT 28
#define TITLE_256C_WIDTH 320 // In pixels
#define TITLE_256C_HEIGHT 224 // In pixels
#define TITLE_256C_STRIP_HEIGHT 8
#define TITLE_256C_COLORS_PER_STRIP 32
// in case you were to split any calculation over the colors of strip by an odd divisor
#define TITLE_256C_COLORS_PER_STRIP_REMAINDER(n) (TITLE_256C_COLORS_PER_STRIP % n)

// Clears a VDPPlane only the region where the DOOM logo appears, and width extended to 64 columns so only one DMA command is issued.
static void clearPlaneFromLogo (VDPPlane plane)
{
    const u16 doomLogoHeightTiles = 19;
    u16* planeAllWithTile0 = (u16*) MEM_alloc(doomLogoHeightTiles * 64 * sizeof(u16));
    memsetU32((u32*) planeAllWithTile0, 0, (doomLogoHeightTiles * 64 * sizeof(u16)) / 4);

    // Enqueue a tilemap with only 0's (tile 0) of size same as plane dimensions 64*28 (columns x rows) in tiles.
    switch(plane)
    {
        case BG_A:
            DMA_queueDmaFast(DMA_VRAM, planeAllWithTile0, VDP_BG_A, doomLogoHeightTiles*64 - (64-(320/8)), 2);
            break;
        case BG_B:
            DMA_queueDmaFast(DMA_VRAM, planeAllWithTile0, VDP_BG_B, doomLogoHeightTiles*64 - (64-(320/8)), 2);
            break;
        default: __builtin_unreachable();
    }

    // Need to wait for VInt so the DMA is fired
    SYS_doVBlankProcess();

    MEM_free(planeAllWithTile0);
}

static TileSet* allocateTilesetInternal (VOID_OR_CHAR* adr)
{
    TileSet *result = (TileSet*) adr;
    result->tiles = (u32*) (adr + sizeof(TileSet));
    return result;
}

static TileSet* unpackTileset_custom (const TileSet* src)
{
    TileSet* result = allocateTilesetInternal(MEM_alloc(src->numTile * 32 + sizeof(TileSet)));
    result->numTile = src->numTile;
    result->compression = src->compression;
    unpackSelector(src->compression, (u8*) FAR_SAFE(src->tiles, src->numTile * 32), (u8*) result->tiles);
    return result;
}

static TileSet* unpackTitle256cTileset ()
{
    TileSet* tileset = img_title_bg_full.tileset;
    if (tileset->compression != COMPRESSION_NONE)
        return unpackTileset_custom(tileset);
    else
        return tileset;
}

static void loadTitle256cTilesetImmediate (u16 currTileIndex, u16 logoTileIndex, TileSet* tileset)
{
    u16 lenImmediate = logoTileIndex - currTileIndex;
    u32* dataImmediate = tileset->tiles;
    if (tileset->compression == COMPRESSION_NONE)
        dataImmediate = FAR_SAFE(tileset->tiles, tileset->numTile * 32);
    VDP_loadTileData(dataImmediate, currTileIndex, lenImmediate, DMA);
    DMA_waitCompletion();
}

static void enqueueTitle256cTilesetQueue (u16 currTileIndex, u16 logoTileIndex, TileSet* tileset)
{
    u16 lenImmediate = logoTileIndex - currTileIndex;
    // remaining tiles will be dma queued
    if (tileset->numTile > lenImmediate) {
        u16 lenToQueue = tileset->numTile - lenImmediate;
        u32* dataToQueue = tileset->tiles;
        if (tileset->compression == COMPRESSION_NONE)
            dataToQueue = FAR_SAFE(tileset->tiles, tileset->numTile * 32);
        VDP_loadTileData(dataToQueue + lenImmediate*8, currTileIndex + lenImmediate, lenToQueue, DMA_QUEUE);
    }
}

static TileMap* allocateTilemapInternal (VOID_OR_CHAR* adr)
{
    TileMap *result = (TileMap*) adr;
    result->tilemap = (u16*) (adr + sizeof(TileMap));
    return result;
}

static TileMap* unpackTilemap_custom(TileMap *src)
{
    TileMap *result = allocateTilemapInternal(MEM_alloc((src->w * src->h * 2) + sizeof(TileMap)));
    result->w = src->w;
    result->h = src->h;
    result->compression = src->compression;
    unpackSelector(src->compression, (u8*) FAR_SAFE(src->tilemap, (src->w * src->h) * 2), (u8*) result->tilemap);
    return result;
}

static TileMap* unpackTitle256cTilemap ()
{
    TileMap* tilemap = img_title_bg_full.tilemap;
    if (tilemap->compression != COMPRESSION_NONE)
        return unpackTilemap_custom(tilemap);
    else
        return tilemap;
}

static void enqueueTitle256cTilemap (VDPPlane plane, u16 currTileIndex, TileMap* tilemap)
{
    u16 baseTileAttribs = TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, currTileIndex);
    const u32 offset = 0;//mulu(y, tilemap->w) + x;
    u16* data = tilemap->tilemap + offset;
    if (tilemap->compression == COMPRESSION_NONE)
        data = (u16*) FAR_SAFE(tilemap->tilemap + offset, (TITLE_256C_WIDTH/8) * (TITLE_256C_HEIGHT/8) * 2);
    VDP_setTileMapDataRectEx(plane, data, baseTileAttribs, 0, 0, TITLE_256C_WIDTH/8, TITLE_256C_HEIGHT/8, TITLE_256C_WIDTH/8, DMA_QUEUE);
}

static u16* palettesData;
static u16* palettesDataSwap;

static void swapPalettesTitle ()
{
    u16* temp = palettesDataSwap;
    palettesDataSwap = palettesData;
    palettesData = temp;
}

static void unpackPalettesTitle ()
{
    palettesData = (u16*) MEM_alloc(TITLE_256C_STRIPS_COUNT * TITLE_256C_COLORS_PER_STRIP * sizeof(u16));
    palettesDataSwap = (u16*) MEM_alloc(TITLE_256C_STRIPS_COUNT * TITLE_256C_COLORS_PER_STRIP * sizeof(u16));

    if (pal_title_bg_full.compression != COMPRESSION_NONE) {
        // No FAR_SAFE() macro needed here. Palette data is always stored at near region.
        unpackSelector(pal_title_bg_full.compression, (u8*) pal_title_bg_full.data, (u8*) palettesData);
    }
    else {
        const u16 size = (TITLE_256C_STRIPS_COUNT * TITLE_256C_COLORS_PER_STRIP) * 2;
        // No FAR_SAFE() macro needed here. Palette data is always stored at near region.
        memcpy((u8*) palettesData, pal_title_bg_full.data, size);
    }

    // Unpack the palettes containing the correct color distribution for the displaced columns

    if (pal_title_bg_full_shifted.compression != COMPRESSION_NONE) {
        // No FAR_SAFE() macro needed here. Palette data is always stored at near region.
        unpackSelector(pal_title_bg_full_shifted.compression, (u8*) pal_title_bg_full_shifted.data, (u8*) palettesDataSwap);
    }
    else {
        const u16 size = (TITLE_256C_STRIPS_COUNT * TITLE_256C_COLORS_PER_STRIP) * 2;
        // No FAR_SAFE() macro needed here. Palette data is always stored at near region.
        memcpy((u8*) palettesDataSwap, pal_title_bg_full_shifted.data, size);
    }
}

static void freePalettesTitle ()
{
    MEM_free((void*) palettesData);
    MEM_free((void*) palettesDataSwap);
}

static u16 screenRowPos = 0;

static void enqueueTwoFirstPalsTitle (u16 startingScreenRowPos)
{
    screenRowPos = startingScreenRowPos;
    // Calculates starting offset into the palettes array
    u16 stripN = min(TITLE_256C_HEIGHT/TITLE_256C_STRIP_HEIGHT - 1, screenRowPos/TITLE_256C_STRIP_HEIGHT);
    // load 2 first palettes from the stripN position
    PAL_setColors(0, palettesData + (stripN * TITLE_256C_COLORS_PER_STRIP), TITLE_256C_COLORS_PER_STRIP * 2, DMA_QUEUE);
}

static u16 vcounterManual;
static u16* title256cPalsPtr; // 1st and 2nd strip's palette are loaded at the beginning of the display loop, so it points to 3rd strip
static u8 palIdx; // which pallete the title256cPalsPtr uses

void vertIntOnTitle256cCallback ()
{
    // On even strips we know we use [PAL0,PAL1] so starts with palIdx=0. On odd strips is [PAL1,PAL2] so starts with palIdx=32.
    palIdx = ((screenRowPos/TITLE_256C_STRIP_HEIGHT) % 2) == 0 ? 0 : TITLE_256C_COLORS_PER_STRIP;
    vcounterManual = TITLE_256C_STRIP_HEIGHT - 1;
    // Calculates starting offset into the palettes array
    u16 stripN = min(TITLE_256C_HEIGHT/TITLE_256C_STRIP_HEIGHT - 1, screenRowPos/TITLE_256C_STRIP_HEIGHT + 2);
    title256cPalsPtr = palettesData + (stripN * TITLE_256C_COLORS_PER_STRIP); // 2 first palettes where loaded in display loop
}

HINTERRUPT_CALLBACK horizIntOnTitle256cCallback_DMA ()
{
    if (vcounterManual < screenRowPos)
        return;

    /*
        With 3 DMA commands:
        Every command is CRAM address to start DMA MOVIE_DATA_COLORS_PER_STRIP/3 colors
        u32 palCmdForDMA_A = VDP_DMA_CRAM_ADDR(((u32)palIdx + 0) * 2);
        u32 palCmdForDMA_B = VDP_DMA_CRAM_ADDR(((u32)palIdx + TITLE_256C_COLORS_PER_STRIP/3) * 2);
        u32 palCmdForDMA_C = VDP_DMA_CRAM_ADDR(((u32)palIdx + 2*(TITLE_256C_COLORS_PER_STRIP/3)) * 2);
        cmd     palIdx = 0      palIdx = 32
        A       0xC0000080      0xC0400080
        B       0xC0140080      0xC0540080
        C       0xC0280080      0xC0680080
    */

    u32 palCmdForDMA;
    u32 fromAddrForDMA;
    u16 fromAddrForDMA_hi;
    const u8 hcLimit = 154;

    // Value under current conditions is always 0x74
    //u8 reg01 = VDP_getReg(0x01); // Holds current VDP register 1 value (NOTE: it holds other bits than VDP ON/OFF status)
    // NOTE: here is OK to call VDP_getReg(0x01) only if we didn't previously change the VDP's reg 1 using direct access without VDP_setReg()

    fromAddrForDMA = (u32) title256cPalsPtr >> 1; // here we manipulate the memory address not its content
    fromAddrForDMA_hi = 0x9700 | ((fromAddrForDMA >> 16) & 0x7f);

MEMORY_BARRIER();
    waitHCounter_opt2(hcLimit);
    //setupDMAForPals(TITLE_256C_COLORS_PER_STRIP/3, fromAddrForDMA);
    // Setup DMA length (in long word here): low at higher word, high at lower word
    *((vu32*) VDP_CTRL_PORT) = ((0x9300 | (u8)(TITLE_256C_COLORS_PER_STRIP/3)) << 16) |
            (0x9400 | (u8)((TITLE_256C_COLORS_PER_STRIP/3) >> 8));
    // Setup DMA address
    *((vu16*) VDP_CTRL_PORT) = 0x9500 | (u8)(fromAddrForDMA);
    *((vu16*) VDP_CTRL_PORT) = 0x9600 | (u8)(fromAddrForDMA >> 8);
    *((vu16*) VDP_CTRL_PORT) = fromAddrForDMA_hi;

    title256cPalsPtr += TITLE_256C_COLORS_PER_STRIP/3;
    palCmdForDMA = palIdx == 0 ? 0xC0000080 : 0xC0400080;
MEMORY_BARRIER();
    waitHCounter_opt2(hcLimit);
    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // Trigger DMA transfer
    turnOnVDP(0x74);

    fromAddrForDMA = (u32) title256cPalsPtr >> 1; // here we manipulate the memory address not its content
    fromAddrForDMA_hi = 0x9700 | ((fromAddrForDMA >> 16) & 0x7f);

MEMORY_BARRIER();
    waitHCounter_opt2(hcLimit);
    //setupDMAForPals(TITLE_256C_COLORS_PER_STRIP/3, fromAddrForDMA);
    // Setup DMA length (in long word here): low at higher word, high at lower word
    *((vu32*) VDP_CTRL_PORT) = ((0x9300 | (u8)(TITLE_256C_COLORS_PER_STRIP/3)) << 16) |
            (0x9400 | (u8)((TITLE_256C_COLORS_PER_STRIP/3) >> 8));
    // Setup DMA address
    *((vu16*) VDP_CTRL_PORT) = 0x9500 | (u8)(fromAddrForDMA);
    *((vu16*) VDP_CTRL_PORT) = 0x9600 | (u8)(fromAddrForDMA >> 8);
    *((vu16*) VDP_CTRL_PORT) = fromAddrForDMA_hi;

    title256cPalsPtr += TITLE_256C_COLORS_PER_STRIP/3;
    palCmdForDMA = palIdx == 0 ? 0xC0140080 : 0xC0540080;
MEMORY_BARRIER();
    waitHCounter_opt2(hcLimit);
    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // Trigger DMA transfer
    turnOnVDP(0x74);

    fromAddrForDMA = (u32) title256cPalsPtr >> 1; // here we manipulate the memory address not its content
    fromAddrForDMA_hi = 0x9700 | ((fromAddrForDMA >> 16) & 0x7f);

MEMORY_BARRIER();
    waitHCounter_opt2(hcLimit);
    //setupDMAForPals(TITLE_256C_COLORS_PER_STRIP/3 + TITLE_256C_COLORS_PER_STRIP_REMAINDER(3), fromAddrForDMA);
    // Setup DMA length (in long word here): low at higher word, high at lower word
    *((vu32*) VDP_CTRL_PORT) = ((0x9300 | (u8)(TITLE_256C_COLORS_PER_STRIP/3 + TITLE_256C_COLORS_PER_STRIP_REMAINDER(3))) << 16) |
            (0x9400 | (u8)((TITLE_256C_COLORS_PER_STRIP/3 + TITLE_256C_COLORS_PER_STRIP_REMAINDER(3)) >> 8));
    // Setup DMA address
    *((vu16*) VDP_CTRL_PORT) = 0x9500 | (u8)(fromAddrForDMA);
    *((vu16*) VDP_CTRL_PORT) = 0x9600 | (u8)(fromAddrForDMA >> 8);
    *((vu16*) VDP_CTRL_PORT) = fromAddrForDMA_hi;

    title256cPalsPtr += TITLE_256C_COLORS_PER_STRIP/3 + TITLE_256C_COLORS_PER_STRIP_REMAINDER(3);
    palCmdForDMA = palIdx == 0 ? 0xC0280080 : 0xC0680080;
    
    // Prepare vars for next HInt here so we can aliviate the waitHCounter loop and exit the HInt sooner
    vcounterManual += TITLE_256C_STRIP_HEIGHT;
    //title256cPalsPtr += TITLE_256C_COLORS_PER_STRIP; // advance to next strip's palettes (if pointer wasn't incremented previously)
    palIdx ^= TITLE_256C_COLORS_PER_STRIP; // cycles between 0 and 32
MEMORY_BARRIER();
    waitHCounter_opt2(hcLimit);
    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // Trigger DMA transfer
    turnOnVDP(0x74);
}

// Current offsets for each column (in pixels)
static s16* columnOffsets;

static void allocateMeltingEffectArrays ()
{
    columnOffsets = MEM_alloc((320/16) * sizeof(s16));
}

static void initializeMeltingEffectArrays ()
{
    columnOffsets[0] = -4;
    columnOffsets[1] = -20;
    columnOffsets[2] = -12;
    columnOffsets[3] = -0;
    columnOffsets[4] = -20;
    columnOffsets[5] = -4;
    columnOffsets[6] = -12;
    columnOffsets[7] = -16;
    columnOffsets[8] = -20;
    columnOffsets[9] = -14;
    columnOffsets[10] = -8;
    columnOffsets[11] = -24;
    columnOffsets[12] = -20;
    columnOffsets[13] = -4;
    columnOffsets[14] = -16;
    columnOffsets[15] = -12;
    columnOffsets[16] = -0;
    columnOffsets[17] = -16;
    columnOffsets[18] = -4;
    columnOffsets[19] = -12;
}

static void freeMeltingEffectArrays ()
{
    MEM_free(columnOffsets);
}

#define MELTING_OFFSET_STEPPING 4

static void updateColumnOffsets (u16 offsetAmnt)
{
    for (u16 i = 0; i < 320/16; i++) {
        columnOffsets[i] -= offsetAmnt; // Increase the offset in pixels
    }
}

static void drawBlackAboveScroll (u16 scanlineEffectPos)
{
    // Extend the Windo downwards so it covers with black the tiles that rollover by the VScroll effect
    VDP_setWindowVPos(FALSE, scanlineEffectPos/8);

    for (u16 i = 0; i < 320/16; i++) {
        // Calculate the number of tiles to overwrite with black
        u16 tilesToBlack = (-1 * columnOffsets[i]) / 8; // tile height
        // Draw 1 black tile above the scroll
        u16 yTilePos = max(0, scanlineEffectPos/8 - tilesToBlack);
        VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, 0), i*2, yTilePos, 2, 1);
    }
}

void title_show ()
{
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_COLUMN);
    VDP_setWindowHPos(FALSE, 0);
    VDP_setWindowVPos(FALSE, 0);

    // Do the unpack of the Title palettes here to avoid display glitches (unknown reason)
    unpackPalettesTitle();

    PAL_setColors(0, palette_black, 64, CPU);

    // ------------------------------------------------------
    // DOOM LOGO
    // ------------------------------------------------------

    // Overrides font tiles location
    u16 logoTileIndex = TILE_MAX_NUM - img_title_logo.tileset->numTile;
    // Draw the DOOM logo
    VDP_drawImageEx(BG_B, (const Image*) &img_title_logo, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, logoTileIndex), 4, 0, FALSE, DMA_QUEUE_COPY);
    //currTileIndex += img_title_logo.tileset->numTile;
    // Fade in the DOOM logo
    PAL_fadeInAll(img_title_logo.palette->data, 30, FALSE);

    // ------------------------------------------------------
    // FULL TITLE SCREEN
    // ------------------------------------------------------

    // Unpack resources for the Title screen
    TileSet* tileset = unpackTitle256cTileset();
    TileMap* tilemap = unpackTitle256cTilemap();
    // DMA immediately the Title's screen tileset up to the start of the Logo's tileset
    u16 currTileIndex = 1;
    loadTitle256cTilesetImmediate(currTileIndex, logoTileIndex, tileset);
    
    SYS_disableInts();

    SYS_setVBlankCallback(vertIntOnTitle256cCallback);
    VDP_setHIntCounter(TITLE_256C_STRIP_HEIGHT - 1);
    SYS_setHIntCallback(horizIntOnTitle256cCallback_DMA);
    VDP_setHInterrupt(TRUE);

    enqueueTwoFirstPalsTitle(0); // Load 1st and 2nd strip's palette
    enqueueTitle256cTilemap(BG_A, currTileIndex, tilemap);

    // Enqueue the remaining tiles of Title's screen tileset. This done here because it will ovewrite part of the DOOM logo
    enqueueTitle256cTilesetQueue(currTileIndex, logoTileIndex, tileset);
    //currTileIndex += img_title_bg_full.tileset->numTile;

    SYS_enableInts();

    // Force to empty DMA queue. 
    // NOTE: DMA will leak into next frame's active display, but resources are correctly sorted to dma in an order that no visible leak appears
    SYS_doVBlankProcess();

    // Now that resources are in VRAM we can dispose their RAM
    if (tileset->compression != COMPRESSION_NONE)
        MEM_free(tileset);
    if (tilemap->compression != COMPRESSION_NONE)
        MEM_free(tilemap);

    // title screen display loop
    for (;;)
    {
        enqueueTwoFirstPalsTitle(0); // Load 1st and 2nd strip's palette

        SYS_doVBlankProcess();

        const u16 joyState = JOY_readJoypad(JOY_1);
        if (joyState & BUTTON_START) {
            break;
        }
    }

    // ------------------------------------------------------
    // MELTING EFFECT TITLE SCREEN
    // ------------------------------------------------------

    // Prepare VSCROLL array
    allocateMeltingEffectArrays();
    initializeMeltingEffectArrays();

    // Clear Plane B just the region where the DOOM logo appears
    clearPlaneFromLogo(BG_B); // it calls SYS_doVBlankProcess()

    // Makes the palettes from displaced columns the actual one used by the HInt
    //VDP_waitVBlank(FALSE);
    //waitVCounterReg(224-4);
    swapPalettesTitle();

    u8 keyPressDelayPeriod = 8; // Helps to avoid a long pressed button in previous update loop to leek into next update loop
    u16 scanlineEffectPos = 0;

    // melting screen update loop
    for (;;)
    {
        enqueueTwoFirstPalsTitle(scanlineEffectPos); // Load 1st and 2nd strip's palette

        // Enqueue the current column offsets to the VSRAM
        VDP_setVerticalScrollTile(BG_A, 0, columnOffsets, 320/16, DMA_QUEUE);
        // Draw Black tiles above every column to cover the rolled back tiles of Title Screen due to VScroll effect
        drawBlackAboveScroll(scanlineEffectPos);
        // Advance to the next scanline where the Title Screen will be scrolled to
        scanlineEffectPos += MELTING_OFFSET_STEPPING;

        SYS_doVBlankProcess();

        updateColumnOffsets(MELTING_OFFSET_STEPPING);
        if (scanlineEffectPos >= 224)
            break;

        const u16 joyState = JOY_readJoypad(JOY_1);
        if (keyPressDelayPeriod == 0 && (joyState & BUTTON_START)) {
            break;
        }
        if (keyPressDelayPeriod > 0)
            --keyPressDelayPeriod;
    }

    SYS_disableInts();

    SYS_setVBlankCallback(NULL);
    VDP_setHInterrupt(FALSE);
    SYS_setHIntCallback(NULL);

    SYS_enableInts();

    // restore planes and settings
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    memsetU32((u32*) columnOffsets, 0, ((320/16) * sizeof(s16)) / 4); // Fill with tile 0
    VDP_setVerticalScrollTile(BG_A, 0, columnOffsets, 320/16, DMA);
    VDP_setVerticalScrollTile(BG_B, 0, columnOffsets, 320/16, DMA);

    // restore SGDK's default palettes
    PAL_setPalette(PAL0, palette_grey, DMA);
    PAL_setPalette(PAL1, palette_red, DMA);
    PAL_setPalette(PAL2, palette_green, DMA);
    PAL_setPalette(PAL3, palette_blue, DMA);

    // restore SGDK's font
    VDP_loadFont(&font_default, DMA);

    freeMeltingEffectArrays();
    freePalettesTitle();
}