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

static TileSet* allocateTileSetInternal (VOID_OR_CHAR* adr) {
    TileSet *result = (TileSet*) adr;
    result->tiles = (u32*) (adr + sizeof(TileSet));
    return result;
}

static TileSet* unpackTileSet_custom (const TileSet* src) {
    TileSet* result = allocateTileSetInternal(MEM_alloc(src->numTile * 32 + sizeof(TileSet)));
    result->numTile = src->numTile;
    result->compression = src->compression;
    unpackSelector(src->compression, (u8*) FAR_SAFE(src->tiles, src->numTile * 32), (u8*) result->tiles);
    return result;
}

static TileSet* unpackTitle256cTileSet () {
    TileSet* tileset = img_title_bg_full.tileset;
    if (tileset->compression != COMPRESSION_NONE)
        return unpackTileSet_custom(tileset);
    else
        return tileset;
}

static void loadTitle256cTileSetImmediate (u16 currTileIndex, u16 logoTileIndex, TileSet* tileset) {
    u16 lenImmediate = logoTileIndex - currTileIndex;
    u32* dataImmediate = tileset->tiles;
    if (tileset->compression == COMPRESSION_NONE)
        dataImmediate = FAR_SAFE(tileset->tiles, tileset->numTile * 32);
    VDP_loadTileData(dataImmediate, currTileIndex, lenImmediate, DMA);
    DMA_waitCompletion();
}

static void enqueueTitle256cTileSetQueue (u16 currTileIndex, u16 logoTileIndex, TileSet* tileset) {
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

static TileMap* allocateTileMapInternal (VOID_OR_CHAR* adr) {
    TileMap *result = (TileMap*) adr;
    result->tilemap = (u16*) (adr + sizeof(TileMap));
    return result;
}

static TileMap* unpackTileMap_custom(TileMap *src) {
    TileMap *result = allocateTileMapInternal(MEM_alloc((src->w * src->h * 2) + sizeof(TileMap)));
    result->w = src->w;
    result->h = src->h;
    result->compression = src->compression;
    unpackSelector(src->compression, (u8*) FAR_SAFE(src->tilemap, (src->w * src->h) * 2), (u8*) result->tilemap);
    return result;
}

static TileMap* unpackTitle256cTileMap () {
    TileMap* tilemap = img_title_bg_full.tilemap;
    if (tilemap->compression != COMPRESSION_NONE)
        return unpackTileMap_custom(tilemap);
    else
        return tilemap;
}

static void enqueueTitle256cTileMap (VDPPlane plane, u16 currTileIndex, TileMap* tilemap) {
    u16 baseTileAttribs = TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, currTileIndex);
    const u32 offset = 0;//mulu(y, tilemap->w) + x;
    u16* data = tilemap->tilemap + offset;
    if (tilemap->compression == COMPRESSION_NONE)
        data = (u16*) FAR_SAFE(tilemap->tilemap + offset, (TITLE_256C_WIDTH/8) * (TITLE_256C_HEIGHT/8) * 2);
    VDP_setTileMapDataRectEx(plane, data, baseTileAttribs, 0, 0, TITLE_256C_WIDTH/8, TITLE_256C_HEIGHT/8, TITLE_256C_WIDTH/8, DMA_QUEUE);
}

static u16* palettesData;

static void unpackPalettes () {
    palettesData = (u16*) MEM_alloc(TITLE_256C_STRIPS_COUNT * TITLE_256C_COLORS_PER_STRIP * sizeof(u16));
    if (pal_title_bg_full.compression != COMPRESSION_NONE) {
        // No FAR_SAFE() macro needed here. Palette data is always stored at near region.
        unpackSelector(pal_title_bg_full.compression, (u8*) pal_title_bg_full.data, (u8*) palettesData);
    }
    else {
        const u16 size = (TITLE_256C_STRIPS_COUNT * TITLE_256C_COLORS_PER_STRIP) * 2;
        // No FAR_SAFE() macro needed here. Palette data is always stored at near region.
        memcpy((u8*) palettesData, pal_title_bg_full.data, size);
    }

    // TODO: set all transparent colors as black
}

static void freePalettes () {
    MEM_free((void*) palettesData);
    palettesData = NULL;
}

void enqueueTwoFirstPals () {
    PAL_setColors(0, palettesData, TITLE_256C_COLORS_PER_STRIP * 2, DMA_QUEUE);
}

static u16 vcounterManual;
static u16* title256cPalsPtr; // 1st and 2nd strip's palette are loaded at the beginning of the display loop, so it points to 3rd strip
static u8 palIdx; // which pallete the title256cPalsPtr uses

void vertIntOnTitle256cCallback () {
    // On even strips we know we use [PAL0,PAL1] so starts with palIdx=0. On odd strips is [PAL1,PAL2] so starts with palIdx=32.
    palIdx = 0;
    vcounterManual = TITLE_256C_STRIP_HEIGHT - 1;
    title256cPalsPtr = palettesData + (2 * TITLE_256C_COLORS_PER_STRIP);
}

HINTERRUPT_CALLBACK horizIntOnTitle256cCallback_DMA () {
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
    u16 fromAddrForDMA_low, fromAddrForDMA_mid, fromAddrForDMA_hi;
    const u8 hcLimit = 154;

    // Value under current conditions is always 0x74
    //u8 reg01 = VDP_getReg(0x01); // Holds current VDP register 1 value (NOTE: it holds other bits than VDP ON/OFF status)
    // NOTE: here is OK to call VDP_getReg(0x01) only if we didn't previously change the VDP's reg 1 using direct access without VDP_setReg()

    fromAddrForDMA = (u32) title256cPalsPtr >> 1; // here we manipulate the memory address not its content
    fromAddrForDMA_low = 0x9500 | (u8)(fromAddrForDMA);
    fromAddrForDMA_mid = 0x9600 | (u8)(fromAddrForDMA >> 8);
    fromAddrForDMA_hi = 0x9700 | (u8)((fromAddrForDMA >> 16) & 0x7f);

MEMORY_BARRIER();
    waitHCounter_DMA(hcLimit);
    //setupDMAForPals(TITLE_256C_COLORS_PER_STRIP/3, fromAddrForDMA);
    // Setup DMA length (in word here)
    *((vu16*) VDP_CTRL_PORT) = 0x9300 | (u8)(TITLE_256C_COLORS_PER_STRIP/3);
    *((vu16*) VDP_CTRL_PORT) = 0x9400 | (u8)((TITLE_256C_COLORS_PER_STRIP/3) >> 8);
    // Setup DMA address
    *((vu16*) VDP_CTRL_PORT) = fromAddrForDMA_low;
    *((vu16*) VDP_CTRL_PORT) = fromAddrForDMA_mid;
    *((vu16*) VDP_CTRL_PORT) = fromAddrForDMA_hi; // needed to avoid glitches with black palette

    title256cPalsPtr += TITLE_256C_COLORS_PER_STRIP/3;
    palCmdForDMA = palIdx == 0 ? 0xC0000080 : 0xC0400080;
MEMORY_BARRIER();
    waitHCounter_DMA(hcLimit);
    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // Trigger DMA transfer
    turnOnVDP(0x74);

    fromAddrForDMA = (u32) title256cPalsPtr >> 1; // here we manipulate the memory address not its content
    fromAddrForDMA_low = 0x9500 | (u8)(fromAddrForDMA);
    fromAddrForDMA_mid = 0x9600 | (u8)(fromAddrForDMA >> 8);
    fromAddrForDMA_hi = 0x9700 | (u8)((fromAddrForDMA >> 16) & 0x7f);

MEMORY_BARRIER();
    waitHCounter_DMA(hcLimit);
    //setupDMAForPals(TITLE_256C_COLORS_PER_STRIP/3, fromAddrForDMA);
    // Setup DMA length (in word here)
    *((vu16*) VDP_CTRL_PORT) = 0x9300 | (u8)(TITLE_256C_COLORS_PER_STRIP/3);
    *((vu16*) VDP_CTRL_PORT) = 0x9400 | (u8)((TITLE_256C_COLORS_PER_STRIP/3) >> 8);
    // Setup DMA address
    *((vu16*) VDP_CTRL_PORT) = fromAddrForDMA_low;
    *((vu16*) VDP_CTRL_PORT) = fromAddrForDMA_mid;
    *((vu16*) VDP_CTRL_PORT) = fromAddrForDMA_hi; // needed to avoid glitches with black palette

    title256cPalsPtr += TITLE_256C_COLORS_PER_STRIP/3;
    palCmdForDMA = palIdx == 0 ? 0xC0140080 : 0xC0540080;
MEMORY_BARRIER();
    waitHCounter_DMA(hcLimit);
    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // Trigger DMA transfer
    turnOnVDP(0x74);

    fromAddrForDMA = (u32) title256cPalsPtr >> 1; // here we manipulate the memory address not its content
    fromAddrForDMA_low = 0x9500 | (u8)(fromAddrForDMA);
    fromAddrForDMA_mid = 0x9600 | (u8)(fromAddrForDMA >> 8);
    fromAddrForDMA_hi = 0x9700 | (u8)((fromAddrForDMA >> 16) & 0x7f);

MEMORY_BARRIER();
    waitHCounter_DMA(hcLimit);
    //setupDMAForPals(TITLE_256C_COLORS_PER_STRIP/3 + TITLE_256C_COLORS_PER_STRIP_REMAINDER(3), fromAddrForDMA);
    // Setup DMA length (in word here)
    *((vu16*) VDP_CTRL_PORT) = 0x9300 | (u8)(TITLE_256C_COLORS_PER_STRIP/3 + TITLE_256C_COLORS_PER_STRIP_REMAINDER(3));
    *((vu16*) VDP_CTRL_PORT) = 0x9400 | (u8)((TITLE_256C_COLORS_PER_STRIP/3 + TITLE_256C_COLORS_PER_STRIP_REMAINDER(3)) >> 8);
    // Setup DMA address
    *((vu16*) VDP_CTRL_PORT) = fromAddrForDMA_low;
    *((vu16*) VDP_CTRL_PORT) = fromAddrForDMA_mid;
    *((vu16*) VDP_CTRL_PORT) = fromAddrForDMA_hi; // needed to avoid glitches with black palette

    title256cPalsPtr += TITLE_256C_COLORS_PER_STRIP/3 + TITLE_256C_COLORS_PER_STRIP_REMAINDER(3);
    palCmdForDMA = palIdx == 0 ? 0xC0280080 : 0xC0680080;
MEMORY_BARRIER();
    waitHCounter_DMA(hcLimit);
    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = palCmdForDMA; // Trigger DMA transfer
    turnOnVDP(0x74);

    vcounterManual += TITLE_256C_STRIP_HEIGHT;
    //title256cPalsPtr += TITLE_256C_COLORS_PER_STRIP; // advance to next strip's palettes (if pointer wasn't incremented previously)
    palIdx ^= TITLE_256C_COLORS_PER_STRIP; // cycles between 0 and 32
    //palIdx = palIdx == 0 ? 32 : 0;
    //palIdx = (palIdx + 32) & 63; // (palIdx + 32) % 64 => x mod y = x & (y-1) when y is power of 2
}

void title_show ()
{
    PAL_setColors(0, palette_black, 64, CPU);

    // Overrides font tiles location
    u16 logoTileIndex = TILE_MAX_NUM - img_title_logo.tileset->numTile;
    VDP_drawImageEx(BG_B, (const Image*) &img_title_logo, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, logoTileIndex), 4, 0, FALSE, DMA_QUEUE_COPY);
    //currTileIndex += img_title_logo.tileset->numTile;

    PAL_fadeInAll(img_title_logo.palette->data, 30, FALSE);

    TileSet* tileset = unpackTitle256cTileSet();
    u16 currTileIndex = 1;
    loadTitle256cTileSetImmediate(currTileIndex, logoTileIndex, tileset);
    TileMap* tilemap = unpackTitle256cTileMap();
    unpackPalettes();

    SYS_disableInts();

    SYS_setVBlankCallback(vertIntOnTitle256cCallback);
    VDP_setHIntCounter(TITLE_256C_STRIP_HEIGHT - 1);
    SYS_setHIntCallback(horizIntOnTitle256cCallback_DMA);
    VDP_setHInterrupt(TRUE);

    enqueueTwoFirstPals(); // Load 1st and 2nd strip's palette
    enqueueTitle256cTileMap(BG_A, currTileIndex, tilemap);
    if (tilemap->compression != COMPRESSION_NONE)
        MEM_free(tilemap);

    SYS_enableInts();
    
    enqueueTitle256cTileSetQueue(currTileIndex, logoTileIndex, tileset);
    //currTileIndex += img_title_bg_full.tileset->numTile;
    if (tileset->compression != COMPRESSION_NONE)
        MEM_free(tileset);

    SYS_doVBlankProcess();

    // title screen display loop
    for (;;)
    {
        enqueueTwoFirstPals(); // Load 1st and 2nd strip's palette

        SYS_doVBlankProcess();

        const u16 joyState = JOY_readJoypad(JOY_1);
        if (joyState & BUTTON_START) {
            break;
        }
    }

    SYS_disableInts();

    SYS_setVBlankCallback(NULL);
    VDP_setHInterrupt(FALSE);
    SYS_setHIntCallback(NULL);

    SYS_enableInts();

    freePalettes();
}