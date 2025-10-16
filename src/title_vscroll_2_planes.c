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

const Palette32AllStrips* pal_title_bg_full_shifted_ptrs[224/MELTING_OFFSET_STEPPING_PIXELS + 1] = {
    &pal_title_bg_full_shifted_0,
    &pal_title_bg_full_shifted_1,
    &pal_title_bg_full_shifted_2,
    &pal_title_bg_full_shifted_3,
    &pal_title_bg_full_shifted_4,
    &pal_title_bg_full_shifted_5,
    &pal_title_bg_full_shifted_6,
    &pal_title_bg_full_shifted_7,
    &pal_title_bg_full_shifted_8,
    &pal_title_bg_full_shifted_9,
    &pal_title_bg_full_shifted_10,
    &pal_title_bg_full_shifted_11,
    &pal_title_bg_full_shifted_12,
    &pal_title_bg_full_shifted_13,
    &pal_title_bg_full_shifted_14,
    &pal_title_bg_full_shifted_15,
    &pal_title_bg_full_shifted_16,
    &pal_title_bg_full_shifted_17,
    &pal_title_bg_full_shifted_18,
    &pal_title_bg_full_shifted_19,
    &pal_title_bg_full_shifted_20,
    &pal_title_bg_full_shifted_21,
    &pal_title_bg_full_shifted_22,
    &pal_title_bg_full_shifted_23,
    &pal_title_bg_full_shifted_24,
    &pal_title_bg_full_shifted_25,
    &pal_title_bg_full_shifted_26,
    &pal_title_bg_full_shifted_27,
    &pal_title_bg_full_shifted_28,
    &pal_title_bg_full_shifted_29,
    &pal_title_bg_full_shifted_30,
    &pal_title_bg_full_shifted_31,
    &pal_title_bg_full_shifted_32,
    &pal_title_bg_full_shifted_33,
    &pal_title_bg_full_shifted_34,
    &pal_title_bg_full_shifted_35,
    &pal_title_bg_full_shifted_36,
    &pal_title_bg_full_shifted_37,
    &pal_title_bg_full_shifted_38,
    &pal_title_bg_full_shifted_39,
    &pal_title_bg_full_shifted_40,
    &pal_title_bg_full_shifted_41,
    &pal_title_bg_full_shifted_42,
    &pal_title_bg_full_shifted_43,
    &pal_title_bg_full_shifted_44,
    &pal_title_bg_full_shifted_45,
    &pal_title_bg_full_shifted_46,
    &pal_title_bg_full_shifted_47,
    &pal_title_bg_full_shifted_48,
    &pal_title_bg_full_shifted_49,
    &pal_title_bg_full_shifted_50,
    &pal_title_bg_full_shifted_51,
    &pal_title_bg_full_shifted_52,
    &pal_title_bg_full_shifted_53,
    &pal_title_bg_full_shifted_54,
    &pal_title_bg_full_shifted_55,
    &pal_title_bg_full_shifted_56
};

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

static void copyInterleavedHalvedTilemaps (TileMap* tilemap, u16 currTileIndex)
{
    u16 baseTileAttribs = TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, currTileIndex);
    // we can increment both index and palette
    const u16 baseinc = baseTileAttribs & (TILE_INDEX_MASK | TILE_ATTR_PALETTE_MASK);
    // we can only do logical OR on priority and HV flip
    const u16 baseor = baseTileAttribs & (TILE_ATTR_PRIORITY_MASK | TILE_ATTR_VFLIP_MASK | TILE_ATTR_HFLIP_MASK);

    u16* src = tilemap->tilemap;
    u16* dstA = (u16*) TITLE_HALVED_TILEMAP_A_ADDRESS;
    u16* dstB = (u16*) TITLE_HALVED_TILEMAP_B_ADDRESS;
    #pragma GCC unroll 0 // do not unroll
    for (u16 i = 0; i < ((TITLE_256C_HEIGHT/8)*(TITLE_256C_WIDTH/8))/2; ++i) {
        // halved tilemap A contains even entries from src
        *dstA++ = baseor | (*src + baseinc);
        ++src;
        // halved tilemap B contains odd entries from src
        *dstB++ = baseor | (*src + baseinc);
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

static void dmaRowByRowTitle256cHalvedTilemaps ()
{
    vu32* vdpCtrl_ptr_l = (vu32*) VDP_CTRL_PORT;

    // We want to write into VRAM every other tilemap entry
    *(vu16*)vdpCtrl_ptr_l = 0x8F00 | 4;

    #pragma GCC unroll 256 // Always set a big number since it does not accept defines
    for (u16 i=0; i < TITLE_256C_HEIGHT/8; ++i) {
        // Plane A row
        doDMAfast_fixed_args(vdpCtrl_ptr_l, 
            TITLE_HALVED_TILEMAP_A_ADDRESS + i*((TITLE_256C_WIDTH/8)/2)*2, 
            VDP_DMA_VRAM_ADDR(PLANE_A_ADDR + i*64*2), 
            ((TITLE_256C_WIDTH/8)/2));
        // Plane B row
        doDMAfast_fixed_args(vdpCtrl_ptr_l, 
            TITLE_HALVED_TILEMAP_B_ADDRESS + i*((TITLE_256C_WIDTH/8)/2)*2, 
            VDP_DMA_VRAM_ADDR(PLANE_B_ADDR + 2 + i*64*2), 
            ((TITLE_256C_WIDTH/8)/2));
    }

    // Restore VDP Auto Inc
    *(vu16*)vdpCtrl_ptr_l = 0x8F00 | 2;
}

static u16* palettesData;

static void allocatePalettesTitle ()
{
    palettesData = (u16*) MEM_alloc(TITLE_256C_STRIPS_COUNT * TITLE_256C_COLORS_PER_STRIP * 2);
}

static void unpackPalettesTitle (u16 *data, u8 compression)
{
    // No FAR_SAFE() macro needed here. Palette data is always stored at near region.
    unpackSelector(compression, (u8*) data, (u8*) palettesData);
}

static void freePalettesTitle ()
{
    MEM_free((void*) palettesData);
}

// Clears a VDPPlane only the region where the DOOM logo appears.
static void clearPlaneBFromLogoImmediately ()
{
    /*
    // Do the DMA fill it in one command, considering the extending width up to 64 tiles
    VDP_clearTileMap(PLANE_B_ADDR + (TITLE_LOGO_Y_POS_TILES*64)*2 + TITLE_LOGO_X_POS_TILES*2, 0, 
        TITLE_LOGO_HEIGHT_TILES*64 - (64-(320/8)) - (TITLE_LOGO_Y_POS_TILES*64) - TITLE_LOGO_X_POS_TILES, TRUE);
    VDP_setAutoInc(2); // restore VDP's AutoInc back to 2 since DMA fill performs 1 byte stepping
    */

    vu32* vdpCtrl_ptr_l = (vu32*) VDP_CTRL_PORT;

    // DMA Fill sets 1 byte at a time
    *(vu16*)vdpCtrl_ptr_l = 0x8F00 | 1;

    #pragma GCC unroll 256 // Always set a big number since it does not accept defines
    for (u16 i=0; i < TITLE_LOGO_HEIGHT_TILES; ++i) {
        DMA_doVRamFill(PLANE_B_ADDR + (TITLE_LOGO_Y_POS_TILES*64)*2 + TITLE_LOGO_X_POS_TILES*2 + (i*64)*2, TITLE_LOGO_WIDTH_TILES*2, 0, -1);
        while (GET_VDP_STATUS(VDP_DMABUSY_FLAG)); // wait DMA completion
    }

    // Important to restore VDP's AutoInc back to 2 since DMA fill performs 1 byte stepping
    *(vu16*)vdpCtrl_ptr_l = 0x8F00 | 2;
}

static void enqueueTwoFirstPalsTitle ()
{
    // Enqueue 2 first palettes from the stripN position
    PAL_setColors(0, palettesData, TITLE_256C_COLORS_PER_STRIP * 2, DMA_QUEUE);
}

static u16* title256cPalsPtr; // 1st and 2nd strip's palette are loaded at the beginning of the display loop, so it points to 3rd strip
static u8 palIdx; // which pallete the title256cPalsPtr uses

static void resetVIntOnTitle256c ()
{
    palIdx = 0; // We know we always start with palettes [PAL0,PAL1]
    title256cPalsPtr = palettesData + (2 * TITLE_256C_COLORS_PER_STRIP); // 2 first palettes where enqueue in display loop
}

static void vintOnTitle256cCallback ()
{
    resetVIntOnTitle256c();
}

static HINTERRUPT_CALLBACK hintOnTitle256cCallback_DMA_asm ()
{
    /*
        With 3 DMA commands and different DMA lenghts:
        Every command is CRAM address to start DMA TITLE_256C_COLORS_PER_STRIP/3. The last one issues TITLE_256C_COLORS_PER_STRIP/3 + REMAINDER.
        u32 palCmdForDMA_A = VDP_DMA_CRAM_ADDR((u32)(palIdx + 0) * 2);
        u32 palCmdForDMA_B = VDP_DMA_CRAM_ADDR(((u32)palIdx + TITLE_256C_COLORS_PER_STRIP/3) * 2);
        u32 palCmdForDMA_C = VDP_DMA_CRAM_ADDR(((u32)palIdx + (TITLE_256C_COLORS_PER_STRIP/3)*2) * 2);
        cmd     palIdx = 0      palIdx = 32
        A       0xC0000080      0xC0400080
        B       0xC0140080      0xC0540080
        C       0xC0280080      0xC0680080
    */

    __asm volatile (
        // Prepare regs
        "   move.l      %c[title256cPalsPtr],%%a0\n" // a0: title256cPalsPtr
        "   lea         0xC00004,%%a1\n"          // a1: VDP_CTRL_PORT 0xC00004
        "   lea         5(%%a1),%%a2\n"           // a2: HCounter address 0xC00009 (VDP_HVCOUNTER_PORT + 1)
        "   move.w      %[turnOff],%%d3\n"        // d3: VDP's register with display OFF value
        "   move.w      %[turnOn],%%d4\n"         // d4: VDP's register with display ON value
        "   move.b      %[hcLimit],%%d6\n"        // d6: HCounter limit
        "   move.l      #0x140000,%%a3\n"         // a3: 0x140000 is the command offset for 10 colors (TITLE_256C_COLORS_PER_STRIP/3) sent to the VDP, used as: cmdAddress += 0x140000
        "   move.w      %[_TITLE_256C_COLORS_PER_STRIP_DIV_3]*2,%%d7\n"

        // DMA batch 1
        "   move.l      %%a0,%%d2\n"            // d2: title256cPalsPtr
        "   adda.w      %%d7,%%a0\n"            // title256cPalsPtr += TITLE_256C_COLORS_PER_STRIP/3;
        // palCmdForDMA = palIdx == 0 ? 0xC0000080 : 0xC0400080;
        // set base command address once and then we'll add the right offset in next sets
		"   move.l      #0xC0000080,%%d5\n"     // d5: palCmdForDMA = 0xC0000080
		"   tst.b       %[palIdx]\n"            // palIdx == 0?
		"   beq.s       0f\n"
		"   move.l      #0xC0400080,%%d5\n"     // d5: palCmdForDMA = 0xC0400080
        "0:\n"
        // Setup DMA command
        "   lsr.w       #1,%%d2\n"              // d2: fromAddrForDMA = (u32) title256cPalsPtr >> 1;
            // NOTE: previous lsr.l can be replaced by lsr.w in case we don't need to use d2: fromAddrForDMA >> 16
        "   move.w      #0x9500,%%d0\n"         // d0: 0x9500
        "   or.b        %%d2,%%d0\n"            // d0: 0x9500 | (u8)(fromAddrForDMA)
        "   move.w      %%d2,-(%%sp)\n"
        "   move.w      #0x9600,%%d1\n"         // d1: 0x9600
        "   or.b        (%%sp)+,%%d1\n"         // d1: 0x9600 | (u8)(fromAddrForDMA >> 8)
        //"   swap        %%d2\n"                 // d2: fromAddrForDMA >> 16
        //"   andi.w      #0x007f,%%d2\n"         // d2: (fromAddrForDMA >> 16) & 0x7f
            // NOTE: previous & 0x7f operation might be discarded if higher bits are somehow already zeroed
        //"   ori.w       #0x9700,%%d2\n"         // d2: 0x9700 | (u8)((fromAddrForDMA >> 16) & 0x7f)
        // Setup DMA length (in word here)
        "   move.w      %[_DMA_9300_LEN_DIV_3],(%%a1)\n"  // *((vu16*) VDP_CTRL_PORT) = 0x9300 | ((TITLE_256C_COLORS_PER_STRIP/3) & 0xff);
        //"   move.w      %[_DMA_9400_LEN_DIV_3],(%%a1)\n"  // *((vu16*) VDP_CTRL_PORT) = 0x9400 | (((TITLE_256C_COLORS_PER_STRIP/3) >> 8) & 0xff);
        // Setup DMA address
        "   move.w      %%d0,(%%a1)\n"          // *((vu16*) VDP_CTRL_PORT) = 0x9500 | (u8)(fromAddrForDMA);
        "   move.w      %%d1,(%%a1)\n"          // *((vu16*) VDP_CTRL_PORT) = 0x9600 | (u8)(fromAddrForDMA >> 8);
        //"   move.w      %%d2,(%%a1)\n"          // *((vu16*) VDP_CTRL_PORT) = 0x9700 | (u8)((fromAddrForDMA >> 16) & 0x7f);
        // wait HCounter
        "1:\n"
        "   cmp.b       (%%a2),%%d6\n"          // cmp: d6 - (a2). Compare byte size given that d6 won't be > 160 for our practical cases
        "   bhi.s       1b\n"                   // loop back if d6 > (a2)
		// turn off VDP
		"   move.w      %%d3,(%%a1)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
        // trigger DMA transfer
        "   move.l      %%d5,(%%a1)\n"          // *((vu32*) VDP_CTRL_PORT) = palCmdForDMA;
		// turn on VDP
		"   move.w      %%d4,(%%a1)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);

        // DMA batch 2
        "   move.l      %%a0,%%d2\n"            // d2: title256cPalsPtr
        "   adda.w      %%d7,%%a0\n"            // title256cPalsPtr += TITLE_256C_COLORS_PER_STRIP/3;
        // palCmdForDMA = palIdx == 0 ? 0xC0140080 : 0xC0540080;
		"   add.l       %%a3,%%d5\n"            // d5: palCmdForDMA += 0x140000 // previous batch advanced 10 colors (TITLE_256C_COLORS_PER_STRIP/3)
        // Setup DMA command
        "   lsr.w       #1,%%d2\n"              // d2: fromAddrForDMA = (u32) title256cPalsPtr >> 1;
            // NOTE: previous lsr.l can be replaced by lsr.w in case we don't need to use d2: fromAddrForDMA >> 16
        "   move.w      #0x9500,%%d0\n"         // d0: 0x9500
        "   or.b        %%d2,%%d0\n"            // d0: 0x9500 | (u8)(fromAddrForDMA)
        "   move.w      %%d2,-(%%sp)\n"
        "   move.w      #0x9600,%%d1\n"         // d1: 0x9600
        "   or.b        (%%sp)+,%%d1\n"         // d1: 0x9600 | (u8)(fromAddrForDMA >> 8)
        //"   swap        %%d2\n"                 // d2: fromAddrForDMA >> 16
        //"   andi.w      #0x007f,%%d2\n"         // d2: (fromAddrForDMA >> 16) & 0x7f
            // NOTE: previous & 0x7f operation might be discarded if higher bits are somehow already zeroed
        //"   ori.w       #0x9700,%%d2\n"         // d2: 0x9700 | (u8)((fromAddrForDMA >> 16) & 0x7f)
        // Setup DMA length (in word here)
        "   move.w      %[_DMA_9300_LEN_DIV_3],(%%a1)\n"  // *((vu16*) VDP_CTRL_PORT) = 0x9300 | ((TITLE_256C_COLORS_PER_STRIP/3) & 0xff);
        //"   move.w      %[_DMA_9400_LEN_DIV_3],(%%a1)\n"  // *((vu16*) VDP_CTRL_PORT) = 0x9400 | (((TITLE_256C_COLORS_PER_STRIP/3) >> 8) & 0xff);
        // Setup DMA address
        "   move.w      %%d0,(%%a1)\n"          // *((vu16*) VDP_CTRL_PORT) = 0x9500 | (u8)(fromAddrForDMA);
        "   move.w      %%d1,(%%a1)\n"          // *((vu16*) VDP_CTRL_PORT) = 0x9600 | (u8)(fromAddrForDMA >> 8);
        //"   move.w      %%d2,(%%a1)\n"          // *((vu16*) VDP_CTRL_PORT) = 0x9700 | (u8)((fromAddrForDMA >> 16) & 0x7f);
        // wait HCounter
        "1:\n"
        "   cmp.b       (%%a2),%%d6\n"          // cmp: d6 - (a2). Compare byte size given that d6 won't be > 160 for our practical cases
        "   bhi.s       1b\n"                   // loop back if d6 > (a2)
		// turn off VDP
		"   move.w      %%d3,(%%a1)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
        // trigger DMA transfer
        "   move.l      %%d5,(%%a1)\n"          // *((vu32*) VDP_CTRL_PORT) = palCmdForDMA;
		// turn on VDP
		"   move.w      %%d4,(%%a1)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);

        // DMA batch 3
        "   move.l      %%a0,%%d2\n"            // d2: title256cPalsPtr
        "   addq.w      %[_TITLE_256C_COLORS_PER_STRIP_DIV_3_REM_ONLY]*2,%%d7\n"  // TITLE_256C_COLORS_PER_STRIP/3 + REMAINDER
        "   adda.w      %%d7,%%a0\n"            // title256cPalsPtr += TITLE_256C_COLORS_PER_STRIP/3 + REMAINDER;
        // palCmdForDMA = palIdx == 0 ? 0xC0280080 : 0xC0680080;
		"   add.l       %%a3,%%d5\n"            // d5: palCmdForDMA += 0x140000 // previous batch advanced 10 colors (TITLE_256C_COLORS_PER_STRIP/3)
        // Setup DMA command
        "   lsr.w       #1,%%d2\n"              // d2: fromAddrForDMA = (u32) title256cPalsPtr >> 1;
            // NOTE: previous lsr.l can be replaced by lsr.w in case we don't need to use d2: fromAddrForDMA >> 16
        "   move.w      #0x9500,%%d0\n"         // d0: 0x9500
        "   or.b        %%d2,%%d0\n"            // d0: 0x9500 | (u8)(fromAddrForDMA)
        "   move.w      %%d2,-(%%sp)\n"
        "   move.w      #0x9600,%%d1\n"         // d1: 0x9600
        "   or.b        (%%sp)+,%%d1\n"         // d1: 0x9600 | (u8)(fromAddrForDMA >> 8)
        //"   swap        %%d2\n"                 // d2: fromAddrForDMA >> 16
        //"   andi.w      #0x007f,%%d2\n"         // d2: (fromAddrForDMA >> 16) & 0x7f
            // NOTE: previous & 0x7f operation might be discarded if higher bits are somehow already zeroed
        //"   ori.w       #0x9700,%%d2\n"         // d2: 0x9700 | (u8)((fromAddrForDMA >> 16) & 0x7f)
        // Setup DMA length (in word here)
        "   move.w      %[_DMA_9300_LEN_DIV_3_REM],(%%a1)\n"  // *((vu16*) VDP_CTRL_PORT) = 0x9300 | ((TITLE_256C_COLORS_PER_STRIP/3 + REMAINDER) & 0xff);
        //"   move.w      %[_DMA_9400_LEN_DIV_3_REM],(%%a1)\n"  // *((vu16*) VDP_CTRL_PORT) = 0x9400 | (((TITLE_256C_COLORS_PER_STRIP/3 + REMAINDER) >> 8) & 0xff);
        // Setup DMA address
        "   move.w      %%d0,(%%a1)\n"          // *((vu16*) VDP_CTRL_PORT) = 0x9500 | (u8)(fromAddrForDMA);
        "   move.w      %%d1,(%%a1)\n"          // *((vu16*) VDP_CTRL_PORT) = 0x9600 | (u8)(fromAddrForDMA >> 8);
        //"   move.w      %%d2,(%%a1)\n"          // *((vu16*) VDP_CTRL_PORT) = 0x9700 | (u8)((fromAddrForDMA >> 16) & 0x7f);
        // Prepare vars for next HInt here so we can aliviate the waitHCounter loop and exit the HInt sooner
        "   eori.b      %[_TITLE_256C_COLORS_PER_STRIP],%c[palIdx]\n"  // palIdx ^= TITLE_256C_COLORS_PER_STRIP // cycles between 0 and 32
        "   move.l      %%a0,%c[title256cPalsPtr]\n"                   // store current pointer value of a0 into variable title256cPalsPtr
        // wait HCounter
        "1:\n"
        "   cmp.b       (%%a2),%%d6\n"          // cmp: d6 - (a2). Compare byte size given that d6 won't be > 160 for our practical cases
        "   bhi.s       1b\n"                   // loop back if d6 > (a2)
		// turn off VDP
		"   move.w      %%d3,(%%a1)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 & ~0x40);
        // trigger DMA transfer
        "   move.l      %%d5,(%%a1)\n"          // *((vu32*) VDP_CTRL_PORT) = palCmdForDMA;
		// turn on VDP
		"   move.w      %%d4,(%%a1)\n"          // *(vu16*) VDP_CTRL_PORT = 0x8100 | (reg01 | 0x40);
		:
		:
        [title256cPalsPtr] "m" (title256cPalsPtr),
		[palIdx] "m" (palIdx),
		[turnOff] "i" (0x8100 | (0x74 & ~0x40)), // 0x8134
		[turnOn] "i" (0x8100 | (0x74 | 0x40)), // 0x8174
        [hcLimit] "i" (156),
        [_DMA_9300_LEN_DIV_3] "i" (0x9300 | ((TITLE_256C_COLORS_PER_STRIP/3) & 0xff)),
        [_DMA_9400_LEN_DIV_3] "i" (0x9400 | (((TITLE_256C_COLORS_PER_STRIP/3) >> 8) & 0xff)),
        [_DMA_9300_LEN_DIV_3_REM] "i" (0x9300 | ((TITLE_256C_COLORS_PER_STRIP/3 + TITLE_256C_COLORS_PER_STRIP_REMAINDER(3)) & 0xff)),
        [_DMA_9400_LEN_DIV_3_REM] "i" (0x9400 | (((TITLE_256C_COLORS_PER_STRIP/3 + TITLE_256C_COLORS_PER_STRIP_REMAINDER(3)) >> 8) & 0xff)),
		[_TITLE_256C_COLORS_PER_STRIP] "i" (TITLE_256C_COLORS_PER_STRIP),
        [_TITLE_256C_COLORS_PER_STRIP_DIV_3] "i" (TITLE_256C_COLORS_PER_STRIP/3),
        [_TITLE_256C_COLORS_PER_STRIP_DIV_3_REM_ONLY] "i" (TITLE_256C_COLORS_PER_STRIP_REMAINDER(3)),
        [_TITLE_256C_STRIP_HEIGHT] "i" (TITLE_256C_STRIP_HEIGHT)
		:
        // backup registers used in the asm implementation including the scratch pad since this code is used in an interrupt call.
		"d0","d1","d2","d3","d4","d5","d6","d7","a0","a1","a2","a3","cc"
    );
}

// Current offsets for each column (in pixels) for Plane A
static s16* columnOffsetsA;
// Current offsets for each column (in pixels) for Plane B
static s16* columnOffsetsB;

static void allocateMeltingEffectArrays ()
{
    columnOffsetsA = MEM_alloc((320/16) * 2); // columns span 2 tiles each, hence 16px
    columnOffsetsB = MEM_alloc((320/16) * 2); // columns span 2 tiles each, hence 16px
}

static void initializeMeltingEffectArrays ()
{
    // multiples of 8

    columnOffsetsA[0] = -8;
    columnOffsetsA[1] = -24;
    columnOffsetsA[2] = -16;
    columnOffsetsA[3] = -0;
    columnOffsetsA[4] = -24;
    columnOffsetsA[5] = -8;
    columnOffsetsA[6] = -24;
    columnOffsetsA[7] = -24;
    columnOffsetsA[8] = -32;
    columnOffsetsA[9] = -16;
    columnOffsetsA[10] = -32;
    columnOffsetsA[11] = -24;
    columnOffsetsA[12] = -32;
    columnOffsetsA[13] = -0;
    columnOffsetsA[14] = -16;
    columnOffsetsA[15] = -8;
    columnOffsetsA[16] = -0;
    columnOffsetsA[17] = -16;
    columnOffsetsA[18] = -32;
    columnOffsetsA[19] = -24;

    columnOffsetsB[0] = -0;
    columnOffsetsB[1] = -32;
    columnOffsetsB[2] = -8;
    columnOffsetsB[3] = -4;
    columnOffsetsB[4] = -32;
    columnOffsetsB[5] = -16;
    columnOffsetsB[6] = -8;
    columnOffsetsB[7] = -16;
    columnOffsetsB[8] = -8;
    columnOffsetsB[9] = -8;
    columnOffsetsB[10] = -0;
    columnOffsetsB[11] = -16;
    columnOffsetsB[12] = -16;
    columnOffsetsB[13] = -8;
    columnOffsetsB[14] = -24;
    columnOffsetsB[15] = -24;
    columnOffsetsB[16] = -32;
    columnOffsetsB[17] = -8;
    columnOffsetsB[18] = -8;
    columnOffsetsB[19] = -8;
}

static void freeMeltingEffectArrays ()
{
    MEM_free(columnOffsetsA);
    MEM_free(columnOffsetsB);
}

static void updateColumnOffsets ()
{
    // C version
    /*#pragma GCC unroll 0 // do not unroll
    for (u16 i = 0; i < (320/16); i++) {
        columnOffsetsA[i] -= MELTING_OFFSET_STEPPING_PIXELS; // Increase the offset in pixels
        columnOffsetsB[i] -= MELTING_OFFSET_STEPPING_PIXELS; // Increase the offset in pixels
    }*/

    // ASM version
    u16 i = (320/16)/2 - 1;
    __asm volatile (
        "\n"
        "1:\n"
        "    sub.l  %[offsetAmnt],(%[colA_ptr])+\n"
        "    sub.l  %[offsetAmnt],(%[colB_ptr])+\n"
        "    dbf    %[i],1b"
        : [i] "+d" (i)
        : [offsetAmnt] "id" ((MELTING_OFFSET_STEPPING_PIXELS << 16) | MELTING_OFFSET_STEPPING_PIXELS),
          [colA_ptr] "a" (columnOffsetsA), [colB_ptr] "a" (columnOffsetsB)
        : "cc"
    );
}

static void drawBlackAboveScroll (u16 scanlineEffectPos)
{
    // Draw a row of black tiles above the applied scroll. 
    // This is applied at the end of screen since the VSCroll direction is downwards and we need to compensate the vertical offset.
    u16 black_tile_idx = 0;
    VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, black_tile_idx), 0, 224/8 - scanlineEffectPos/8, 320/8, 1);
    VDP_fillTileMapRect(BG_B, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, black_tile_idx), 0, 224/8 - scanlineEffectPos/8, 320/8, 1);
}

void title_vscroll_2_planes_show ()
{
    // Setup VDP
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_COLUMN);
    VDP_setWindowOff();

    // Clean mem region where fixed buffers are located
    memsetU32((u32*)TITLE_HALVED_TILEMAP_B_ADDRESS, 0, ((TITLE_256C_HEIGHT/8)*((TITLE_256C_WIDTH/8)/2)*2) / 4);
    memsetU32((u32*)TITLE_HALVED_TILEMAP_A_ADDRESS, 0, ((TITLE_256C_HEIGHT/8)*((TITLE_256C_WIDTH/8)/2)*2) / 4);

    // Do the unpack of the Title palettes here to avoid display glitches (unknown reason)
    allocatePalettesTitle();
    unpackPalettesTitle(pal_title_bg_full.data, COMPRESSION_APLIB);

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
    TileSet* tileset = unpackTitle256cTileset();
    TileMap* tilemap = unpackTitle256cTilemap();
    copyInterleavedHalvedTilemaps(tilemap, currTileIndex);
    MEM_free(tilemap);

    // DMA immediately the Title's screen tileset only the region up to the start of the Logo's tileset
    dmaTitle256cTileset(currTileIndex, logoTileIndex, tileset, TRUE, DMA);

    // Let's wait to next VInt to consume what remains of current display loop
    VDP_waitVBlank(FALSE);
    // Wait until we reach the scanline of Logo's end (actually some scanlines earlier)
    waitVCounterReg(TITLE_LOGO_Y_POS_TILES*8 + TITLE_LOGO_HEIGHT_TILES*8 - 25); // -25 gives enough time to DMA what's next without visible glitches

    VDP_setEnable(FALSE);

    // Clear Plane B region where the DOOM logo is located
    clearPlaneBFromLogoImmediately();
    // DMA what remains of halved tilemaps
    dmaRowByRowTitle256cHalvedTilemaps();
    // DMA the remaining tiles of Title's screen tileset. This done here because otherwise may ovewrite part of the DOOM logo
    dmaTitle256cTileset(currTileIndex, logoTileIndex, tileset, FALSE, DMA);
    currTileIndex += img_title_bg_full.tileset->numTile; // update just in case we need to load more tiles
    
    VDP_setEnable(TRUE);

    SYS_disableInts();
        SYS_setVBlankCallback(vintOnTitle256cCallback);
        VDP_setHIntCounter(TITLE_256C_STRIP_HEIGHT - 1);
        SYS_setHIntCallback(hintOnTitle256cCallback_DMA_asm);
        VDP_setHInterrupt(TRUE);
    SYS_enableInts();

    enqueueTwoFirstPalsTitle(); // Load 1st and 2nd strip's palette

    // Force to empty DMA queue.
    // NOTE: DMA will leak into next frame's active display, but resources are correctly sorted to dma in such an order that no visible glitch is noticeable.
    SYS_doVBlankProcess();

    // Now that tileset is in VRAM we can free their RAM
    if (tileset->compression != COMPRESSION_NONE)
        MEM_free(tileset);

    // Title screen display loop
    for (;;)
    {
        enqueueTwoFirstPalsTitle(); // Load 1st and 2nd strip's palette

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

    // Let's wait to next VInt to consume what remains of current display loop
    VDP_waitVBlank(FALSE);

    u16 scanlineEffectPos = 0;

    // Melting screen update loop
    for (;;)
    {
        // NOTE: first loop still draws palettes from previous phase and no columns are shifted because they are DMA queued

        enqueueTwoFirstPalsTitle(); // Load 1st and 2nd strip's palette

        // Enqueue for DMA the current column offsets to the VSRAM, for next frame
        VDP_setVerticalScrollTile(BG_A, 0, columnOffsetsA, 320/16, DMA_QUEUE);
        VDP_setVerticalScrollTile(BG_B, 0, columnOffsetsB, 320/16, DMA_QUEUE);

        // update column offsets for next frame
        updateColumnOffsets(MELTING_OFFSET_STEPPING_PIXELS);

        // Draw Black tiles above every column to cover the rolled back tiles of Title Screen due to VScroll effect
        drawBlackAboveScroll(scanlineEffectPos);

        // Advance to the next scanline where the Title Screen will be scrolled to
        scanlineEffectPos += MELTING_OFFSET_STEPPING_PIXELS;

        // Unpack palettes for next frame
        u16* nextPalData = pal_title_bg_full_shifted_ptrs[scanlineEffectPos/MELTING_OFFSET_STEPPING_PIXELS]->data;
        unpackPalettesTitle(nextPalData, COMPRESSION_LZ4W);

        SYS_doVBlankProcess();

        if (scanlineEffectPos >= 224)
            break;
    }

    SYS_disableInts();

        SYS_setVBlankCallback(NULL);
        VDP_setHInterrupt(FALSE);
        SYS_setHIntCallback(NULL);

    SYS_enableInts();

    freeMeltingEffectArrays();
    freePalettesTitle();
    // Clear mem region where fixed buffers are located
    memsetU32((u32*)TITLE_HALVED_TILEMAP_B_ADDRESS, 0, ((TITLE_256C_HEIGHT/8)*((TITLE_256C_WIDTH/8)/2)*2) / 4);
    memsetU32((u32*)TITLE_HALVED_TILEMAP_A_ADDRESS, 0, ((TITLE_256C_HEIGHT/8)*((TITLE_256C_WIDTH/8)/2)*2) / 4);

    VDP_resetScreen();
}