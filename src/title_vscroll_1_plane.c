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

static void dmaTitle256cTileset (u16 currTileIndex, u16 logoTileIndex, TileSet* tileset, bool isFirstChunk, TransferMethod tm)
{
    u16 lenImmediate = logoTileIndex - currTileIndex;
    u32* data = tileset->tiles;

    if (tileset->compression == COMPRESSION_NONE)
        data = FAR_SAFE(tileset->tiles, tileset->numTile * 32);

    if (isFirstChunk) {
        VDP_loadTileData(data, currTileIndex, lenImmediate, tm);
    }
    else {
        if (tileset->numTile > lenImmediate) {
            u16 len = tileset->numTile - lenImmediate;
            VDP_loadTileData(data + (lenImmediate * 8), currTileIndex + lenImmediate, len, tm);
        }
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

static TileMap* copyTilemap (TileMap *src)
{
    TileMap *result = allocateTilemapInternal(MEM_alloc((src->w * src->h * 2) + sizeof(TileMap)));
    result->w = src->w;
    result->h = src->h;
    result->compression = src->compression;
    memcpy(result->tilemap, src->tilemap, src->w * src->h * 2);
    return result;
}

static TileMap* unpackTitle256cTilemap ()
{
    TileMap* tilemap = img_title_bg_full.tilemap;
    if (tilemap->compression != COMPRESSION_NONE)
        return unpackTilemap_custom(tilemap);
    else
        return copyTilemap(tilemap);
}

static void copyTilemapIntoAddress (TileMap* tilemap, u16 currTileIndex, u32 fixedAddress)
{
    u16 baseTileAttribs = TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, currTileIndex);
    // we can increment both index and palette
    const u16 baseinc = baseTileAttribs & (TILE_INDEX_MASK | TILE_ATTR_PALETTE_MASK);
    // we can only do logical OR on priority and HV flip
    const u16 baseor = baseTileAttribs & (TILE_ATTR_PRIORITY_MASK | TILE_ATTR_VFLIP_MASK | TILE_ATTR_HFLIP_MASK);

    u16* src = tilemap->tilemap;
    u16* dst = (u16*) fixedAddress;
    for (u16 i = 0; i < ((TITLE_256C_HEIGHT/8)*(TITLE_256C_WIDTH/8)); ++i) {
        *dst++ = baseor | (*src + baseinc);
        ++src;
    }
}

// static void enqueueTitle256cTilemap (u16 currTileIndex, TileMap* tilemap, VDPPlane plane)
// {
//     u16 baseTileAttribs = TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, currTileIndex);
//     const u32 offset = 0;//mulu(y, tilemap->w) + x;
//     u16* data = tilemap->tilemap + offset;
//     VDP_setTileMapDataRectEx(plane, data, baseTileAttribs, 0, 0, TITLE_256C_WIDTH/8, TITLE_256C_HEIGHT/8, TITLE_256C_WIDTH/8, DMA_QUEUE);
// }

static void dmaRowByRowTitle256cTilemap (u32 tilemapAddr, VDPPlane plane)
{
    vu32* vdpCtrl_ptr_l = (vu32*) VDP_CTRL_PORT;

    #pragma GCC unroll 28 // Always set the max number since it does not accept defines
    for (u16 i=0; i < TITLE_256C_HEIGHT/8; ++i) {
        if (plane == BG_A) {
            doDMAfast_fixed_args(vdpCtrl_ptr_l, 
                tilemapAddr + i*(TITLE_256C_WIDTH/8)*2, 
                VDP_DMA_VRAM_ADDR(PLANE_A_ADDR + i*64*2), 
                (TITLE_256C_WIDTH/8));
        }
        else if (plane == BG_B) {
            doDMAfast_fixed_args(vdpCtrl_ptr_l, 
                tilemapAddr + i*(TITLE_256C_WIDTH/8)*2, 
                VDP_DMA_VRAM_ADDR(PLANE_B_ADDR + i*64*2), 
                (TITLE_256C_WIDTH/8));
        }
    }
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
    palettesData = (u16*) MEM_alloc(TITLE_256C_STRIPS_COUNT * TITLE_256C_COLORS_PER_STRIP * 2);
    palettesDataSwap = (u16*) MEM_alloc(TITLE_256C_STRIPS_COUNT * TITLE_256C_COLORS_PER_STRIP * 2);

    // Unpack the palettes of the normal title image

    if (pal_title_bg_full.compression != COMPRESSION_NONE) {
        // No FAR_SAFE() macro needed here. Palette data is always stored at near region.
        unpackSelector(pal_title_bg_full.compression, (u8*) pal_title_bg_full.data, (u8*) palettesData);
    }
    else {
        const u16 size = (TITLE_256C_STRIPS_COUNT * TITLE_256C_COLORS_PER_STRIP) * 2;
        // No FAR_SAFE() macro needed here. Palette data is always stored at near region.
        memcpy((u8*) palettesData, pal_title_bg_full.data, size);
    }

    // Unpack the palettes of the melted title image

    if (pal_title_bg_full_melted.compression != COMPRESSION_NONE) {
        // No FAR_SAFE() macro needed here. Palette data is always stored at near region.
        unpackSelector(pal_title_bg_full_melted.compression, (u8*) pal_title_bg_full_melted.data, (u8*) palettesDataSwap);
    }
    else {
        const u16 size = (TITLE_256C_STRIPS_COUNT * TITLE_256C_COLORS_PER_STRIP) * 2;
        // No FAR_SAFE() macro needed here. Palette data is always stored at near region.
        memcpy((u8*) palettesDataSwap, pal_title_bg_full_melted.data, size);
    }
}

static void freePalettesTitle ()
{
    MEM_free((void*) palettesData);
    MEM_free((void*) palettesDataSwap);
}

// Clears a VDPPlane only the region where the DOOM logo appears, and width extended to 64 columns so only one DMA command is issued.
static void clearPlaneBFromLogoImmediately ()
{
    //VDP_clearTileMap(PLANE_B_ADDR, 0, TITLE_LOGO_HEIGHT_TILES*64 - (64-(320/8)), TRUE);
    //VDP_setAutoInc(2);

    vu32* vdpCtrl_ptr_l = (vu32*) VDP_CTRL_PORT;

    // DMA Fill sets 1 byte at a time
    *(vu16*)vdpCtrl_ptr_l = 0x8F00 | 1;

    #pragma GCC unroll 19 // Always set the max number since it does not accept defines
    for (u16 i=0; i < TITLE_LOGO_HEIGHT_TILES; ++i) {
        DMA_doVRamFill(PLANE_B_ADDR + (TITLE_LOGO_Y_POS_TILES*64)*2 + TITLE_LOGO_X_POS_TILES*2 + (i*64)*2, TITLE_LOGO_WIDTH_TILES*2, 0, -1);
        while (GET_VDP_STATUS(VDP_DMABUSY_FLAG)); // wait DMA completion
    }

    // Important to restore VDP AutoInc back to 2 since DMA fill performs 1 byte stepping
    *(vu16*)vdpCtrl_ptr_l = 0x8F00 | 2;
}

static u16 screenRowPos;

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

static void resetVIntOnTitle256c ()
{
    // On even strips we know we use [PAL0,PAL1] so starts with palIdx=0. On odd strips is [PAL1,PAL2] so starts with palIdx=32.
    palIdx = ((screenRowPos/TITLE_256C_STRIP_HEIGHT) % 2) == 0 ? 0 : TITLE_256C_COLORS_PER_STRIP;
    vcounterManual = TITLE_256C_STRIP_HEIGHT - 1;
    // Calculates starting offset into the palettes array
    u16 stripN = min(TITLE_256C_HEIGHT/TITLE_256C_STRIP_HEIGHT - 1, screenRowPos/TITLE_256C_STRIP_HEIGHT + 2);
    title256cPalsPtr = palettesData + (stripN * TITLE_256C_COLORS_PER_STRIP); // 2 first palettes where loaded in display loop
}

static void vintOnTitle256cCallback ()
{
    resetVIntOnTitle256c();
}

static HINTERRUPT_CALLBACK hintOnTitle256cCallback_DMA ()
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

static void drawBlackAboveScroll (u16 scanlineEffectPos, VDPPlane plane)
{
    // Draw a row of black tiles above the applied scroll. 
    // This is applied at the end of screen since the VSCroll direction is downwards and we need to compensate the vertical offset.
    VDP_fillTileMapRect(plane, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, 0), 0, 224/8 - scanlineEffectPos/8, 320/8, 1);
}

void title_vscroll_1_plane_show ()
{
    // Setup VDP
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
    VDP_setWindowOff();

    // Clean mem region where fixed buffers are located
    memsetU32((u32*)TITLE_FULL_TILEMAP_ADDRESS, 0, ((TITLE_256C_HEIGHT/8)*(TITLE_256C_WIDTH/8)*2) / 4);
    memsetU32((u32*)TITLE_MELTDOWN_TILEMAP_ADDRESS, 0, ((TITLE_256C_HEIGHT/8)*(TITLE_256C_WIDTH/8)*2) / 4);

    // Do the unpack of the Title palettes here to avoid display glitches (unknown reason)
    unpackPalettesTitle();

    PAL_setColors(0, palette_black, 64, DMA);

    // ------------------------------------------------------
    // DOOM LOGO
    // ------------------------------------------------------

    // Overrides font tiles location
    u16 logoTileIndex = TILE_MAX_NUM - img_title_logo.tileset->numTile;
    // Draw the DOOM logo
    VDP_drawImageEx(BG_B, (const Image*) &img_title_logo, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, logoTileIndex), 
        TITLE_LOGO_X_POS_TILES, TITLE_LOGO_Y_POS_TILES, FALSE, TRUE);
    // Fade in the DOOM logo
    PAL_fadeInAll(img_title_logo.palette->data, 30, FALSE);

    // ------------------------------------------------------
    // FULL TITLE SCREEN
    // ------------------------------------------------------

    resetVIntOnTitle256c();

    // We start placing tiles at VRAM location 1, since location 0 is reserved
    u16 currTileIndex = 1;

    // Unpack resources for the Title screen
    TileSet* tilesetFull = unpackTitle256cTileset();
    TileMap* tilemapFull = unpackTitle256cTilemap();
    copyTilemapIntoAddress(tilemapFull, currTileIndex, TITLE_FULL_TILEMAP_ADDRESS);
    MEM_free(tilemapFull);

    // DMA immediately the Title's screen tileset only the region up to the start of the Logo's tileset
    dmaTitle256cTileset(currTileIndex, logoTileIndex, tilesetFull, TRUE, DMA);

    // Let's wait to next VInt then wait until we reach the scanline after the Logo
    VDP_waitVBlank(FALSE);
    // Wait until we reach the scanline of Logo's end (actually some scanlines earlier)
    waitVCounterReg(TITLE_LOGO_Y_POS_TILES*8 + TITLE_LOGO_HEIGHT_TILES*8 - 25); // -25 gives enough time to DMA what's next without visible glitches

    VDP_setEnable(FALSE);

    // Clear Plane B region where the DOOM logo is located
    clearPlaneBFromLogoImmediately();
    // DMA tilemapFull
    dmaRowByRowTitle256cTilemap(TITLE_FULL_TILEMAP_ADDRESS, BG_A);
    // DMA the remaining tiles of Title's screen tileset. This done here because otherwise may ovewrite part of the DOOM logo
    dmaTitle256cTileset(currTileIndex, logoTileIndex, tilesetFull, FALSE, DMA);
    currTileIndex += img_title_bg_full.tileset->numTile; // update just in case we need to load more tiles
    
    VDP_setEnable(TRUE);

    enqueueTwoFirstPalsTitle(0); // Load 1st and 2nd strip's palette

    SYS_disableInts();
        SYS_setVBlankCallback(vintOnTitle256cCallback);
        VDP_setHIntCounter(TITLE_256C_STRIP_HEIGHT - 1);
        SYS_setHIntCallback(hintOnTitle256cCallback_DMA);
        VDP_setHInterrupt(TRUE);
    SYS_enableInts();

    // Force to empty DMA queue.
    // NOTE: DMA will leak into next frame's active display, but resources are correctly sorted to dma in such an order that no visible glitch is noticeable.
    SYS_doVBlankProcess();

    // Now that resources are in VRAM we can free their RAM
    if (tilesetFull->compression != COMPRESSION_NONE)
        MEM_free(tilesetFull);

    // Title screen display loop
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

    //VDP_waitVBlank(FALSE);
    //waitVCounterReg(224-4);
    // Set the palettes from displaced columns image to be ready for HInt the next frame
    swapPalettesTitle();

    u8 keyPressDelayPeriod = 10; // Helps to avoid a long pressed button in previous update loop to leek into next update loop
    u16 scanlineEffectPos = 0;

    // Melting screen update loop
    for (;;)
    {
        enqueueTwoFirstPalsTitle(scanlineEffectPos); // Load 1st and 2nd strip's palette

        // Enqueue the current column offsets to the VSRAM
        VDP_setVerticalScrollVSync(BG_B, -scanlineEffectPos);

        // Draw Black tiles above every column to cover the rolled back tiles of Title Screen due to VScroll effect
        drawBlackAboveScroll(scanlineEffectPos, BG_B);
        // Advance to the next scanline where the Title Screen will be scrolled to
        scanlineEffectPos += MELTING_OFFSET_STEPPING;

        SYS_doVBlankProcess();

        if (scanlineEffectPos >= (224 + 2*MELTING_OFFSET_STEPPING))
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

    freePalettesTitle();
    // Clear mem region where fixed buffers are located
    memsetU32((u32*)TITLE_FULL_TILEMAP_ADDRESS, 0, ((TITLE_256C_HEIGHT/8)*(TITLE_256C_WIDTH/8)*2) / 4);
    memsetU32((u32*)TITLE_MELTDOWN_TILEMAP_ADDRESS, 0, ((TITLE_256C_HEIGHT/8)*(TITLE_256C_WIDTH/8)*2) / 4);

    VDP_resetScreen();
}