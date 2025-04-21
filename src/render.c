#include "render.h"
#include <vdp.h>
#include <z80_ctrl.h>
#include <timer.h>
#include <dma.h>
#include <sys.h>
#include <memory.h>
#include "consts.h"
#include "consts_ext.h"
//#include <joy.h>
#include "joy_6btn.h"
#include "hud.h"
#include "utils.h"
#include "frame_buffer.h"
#include "vint_callback.h"

extern VoidCallback *vblankCB;

#if RENDER_ENABLE_FRAME_LOAD_CALCULATION
extern bool addFrameLoad(u16 frameLoad, u32 vtime);
extern u16 getAdjustedVCounterInternal(u16 blank, u16 vcnt);
#endif

u16 render_loadTiles ()
{
    // ORDERING: Tiles are created in groups of 8 tiles except first one which is the empty tile.
    // Tiles inside each group go from Highest to Smallest (in pixel height). Groups go from Darkest to Brightest.
    // 4 most right columns of pixels for each tile is left empty because this way the Plane B can be displaced 4 bits to the right.

	// Create a buffer of the size of a tile
	u8* tile = MEM_alloc(32); // 32 bytes per tile, layout: tile[4*8]
	memset(tile, 0, 32); // clear the tile with color index 0 (which is the BG color index)

	// 9 possible tile heights: from 0 to 8 pixels

	// Tile with height 0 goes at index 0, and its color is 0
	VDP_loadTileData((u32*)tile, 0, 1, CPU);

	// Remaining 8 possible tile heights, distributed in 8 sets

    // fabri1983: we'll add 8 more tiles so we can use the other half of the palette.
    // First pass creates and loads 8 tiles with colors 0..7.
    // Second pass creates and loads 8 tiles with colors 0 and 8..14.
    for (u8 pass=0; pass < 2; ++pass) {

        // 8 tiles per set
        for (u16 t = 1; t <= 8; t++) {
            memset(tile, 0, 32); // clear the tile with color index 0
            // 8 colors: they match with those from SGDK's ramp palettes (palette_grey, red, green, blue) first 8 colors going from darker to lighter
            for (u16 c = 0; c < 8; c++) {
                // Visit the height of each tile in current set. Height here is 0 based.
                for (u16 h = t-1; h < 8; h++) {
                    // Visit the columns of current row. 1 byte holds 2 colors as per Tile definition (4 bits per color),
                    // so lower byte is color c and higher byte is color c+1, letting the RCA video signal do the blending.
                    // We only fill most left 4 columns of pixels of the tile, leaving the other 4 columns with color 0. 
                    for (u16 b = 0; b < 2; b++) {
                        // lower 4 bits
                        u8 colorLow = c;
                        if (pass == 1)
                            colorLow = c == 0 ? 0 : c+7;

                        // higher 4 bits
                        u8 colorHigh = (c+1) == 8 ? c : c+1;
                        // We clamp to color 7 since starting at SGDK's palette 8th color they repeat
                        if (pass == 1)
                            colorHigh += 7;

                        // combine lower and higher bits (low and high colors)
                        u8 color = colorLow | (colorHigh << 4);

                        // This sets 2 pixels (remember color holds 2 color pixels) at left columns [4*h] and [4*h + 1],
                        // leaving right columns at [4*h + 2] and [4*h + 2] as it is (currently 0)
                        tile[4*h + b] = color;
                        // Duplicate color in right colums too
                        //tile[4*h + 2 + b] = color;
                    }
                }
                VDP_loadTileData((u32*)tile, t + c*8 + (pass*(8*8)), 1, CPU);
            }
        }
    }

    // Add 5 tiles more to be used as floor using PAL0 (which holds grey palette) 
    // ranging from 0x0222 (darkest) to 0x0444 (brightest) using dithering with black color at position 0xF
    /*u16 floorTileStart = 1 + 8*8 + 8*8;
    for (u16 t = 0; t < 5; t++) {
        memset(tile, 0, 32); // Clear the tile
        // Iterate over all heights
        for (u16 h = 0; h < 8; h++) {
            // Iterate for 2 columns of colors
            for (u16 b = 0; b < 2; b++) {
                u8 color;
                // Different dithering ratios
                switch (t) {
                    case 0: // Darkest: 25% 0x2, 75% 0xF
                        color = (((h + b) % 4) < 1) ? 0x22 : 0xF2;
                        break;
                    case 1: // 50% 0x2, 50% 0xF
                        color = ((h + b) % 2) ? 0x22 : 0xF2;
                        break;
                    case 2: // 50% 0x2, 50% 0x4
                        color = ((h + b) % 2) ? 0x22 : 0x42;
                        break;
                    case 3: // 25% 0x2, 75% 0x4
                        color = (((h + b) % 4) < 1) ? 0x22 : 0x42;
                        break;
                    case 4: // Brightest: all 0x4
                        color = 0x44;
                        break;
                }
                // Set color for left columns
                tile[4*h + b] = color;
                // Duplicate color in right colums too
                //tile[4*h + 2 + b] = color;
            }
        }
        VDP_loadTileData((u32*)tile, floorTileStart + t, 1, CPU);
    }*/

    MEM_free(tile);

    // Returns next available tile index in VRAM
    return VRAM_INDEX_AFTER_TILES;
}

void render_loadWallPalettes ()
{
    // palette grey and palette red were already loaded by SGDK

    // load colors used from green palette into PAL0
    PAL_setColors(PAL0*16 + 8, palette_green + 1, 7, DMA);
    // load colors used from blue palette into PAL1
    PAL_setColors(PAL1*16 + 8, palette_blue + 1, 7, DMA);
}

void render_Z80_setBusProtection (bool value)
{
    Z80_requestBus(FALSE);
	u16 busProtectSignalAddress = (Z80_DRV_PARAMS + 0x0D) & 0xFFFF; // point to Z80 PROTECT parameter
    vu8* pb = (u8*) (Z80_RAM + busProtectSignalAddress); // See Z80_useBusProtection() reference in z80_ctrl.c
    *pb = value?1:0;
	Z80_releaseBus();
}

extern DMAOpInfo *dmaQueues;

void render_DMA_flushQueue ()
{
    u16 queueIndex = DMA_getQueueSize();
    if (queueIndex == 0)
        return;

    // u8 autoInc = VDP_getAutoInc(); // save autoInc
	// Z80_requestBus(FALSE);

    vu32* vdpCtrl_ptr_l = (u32*) VDP_CTRL_PORT;
    __asm volatile (
        "subq.w  #1,%2\n\t"       // prepare for dbra
        ".fq_loop_%=:\n\t"
        "move.l  (%0)+,(%1)\n\t"
        "move.l  (%0)+,(%1)\n\t"
        "move.l  (%0)+,(%1)\n\t"
        "move.w  (%0)+,(%1)\n\t"
        "move.w  (%0)+,(%1)\n\t"  // Stef: important to use word write for command triggering DMA (see SEGA notes)
        "dbra    %2,.fq_loop_%="
        :
        : "a" (dmaQueues), "a" (vdpCtrl_ptr_l), "d" (queueIndex)
        :
    );

    // Z80_releaseBus();
    DMA_clearQueue();
    // VDP_setAutoInc(autoInc); // restore autoInc
}

static FORCE_INLINE void render_waitVInt_vtimer ()
{
    // Casting to u8* allows to use cmp.b instead of cmp.l, by using vtimerPtr+3 which is the first byte of vtimer
    const u8* vtimerPtr = (u8*)&vtimer + 3;
    // Loops while vtimer keeps unchanged. Exits loop when it changes, meaning we are in VBlank.
    u8 currVal;
	__asm volatile (
        "move.b  (%1), %0\n\t"
        "1:\n\t"
        "cmp.b   (%1), %0\n\t" // cmp: %0 - (%1) => dN - (aN)
        "beq.s   1b"           // loop back if equal
        : "=d" (currVal)
        : "a" (vtimerPtr)
        :
    );
}

#if RENDER_ENABLE_FRAME_LOAD_CALCULATION
static u32 vtimerStart;
static u16 vcnt;
static u16 blank;

static void render_setupFrameLoadCalculation ()
{
    // For frame CPU load calculation
    vtimerStart = vtimer;
    // store V-Counter and initial blank state
    vcnt = GET_VCOUNTER;
    vu16* pw = (u16*) VDP_CTRL_PORT;
    blank = *pw & VDP_VBLANK_FLAG;
}

static void render_calculateFrameLoad ()
{
    // update CPU frame load
    addFrameLoad(getAdjustedVCounterInternal(blank, vcnt), vtimerStart);
}
#endif

void render_SYS_doVBlankProcessEx_ON_VBLANK ()
{
    #if RENDER_ENABLE_FRAME_LOAD_CALCULATION
    render_setupFrameLoadCalculation();
    #endif

    // joy state refresh
    //JOY_update();
    joy_update_6btn();

    // Wait until vint is triggered and returned from vint_callback()
    render_waitVInt_vtimer();

    #if RENDER_ENABLE_FRAME_LOAD_CALCULATION
    render_calculateFrameLoad();
    showCPULoad(0, 24); // is shown on WINDOW plane
    //showFPS(0, 24);
    #endif
}

void render_DMA_enqueue_framebuffer ()
{
    // All the frame_buffer Plane A
    DMA_queueDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, (VERTICAL_ROWS*PLANE_COLUMNS) - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
 
    // All the frame_buffer Plane B
    DMA_queueDmaFast(DMA_VRAM, frame_buffer + (VERTICAL_ROWS*PLANE_COLUMNS), PB_ADDR, (VERTICAL_ROWS*PLANE_COLUMNS) - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
}

void render_DMA_row_by_row_framebuffer ()
{
    // Arguments must be in byte addressing mode.
    // Except the lenght since DMA RAM to VRAM copies 2 bytes on every cycle.
    // Also we assume VDP's stepping is already 2.

    vu32* vdpCtrl_ptr_l = (vu32*) VDP_CTRL_PORT;

    #if RENDER_MIRROR_PLANES_USING_VDP_VRAM || RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT || RENDER_MIRROR_PLANES_USING_VSCROLL_IN_HINT_MULTI_CALLBACKS
    #pragma GCC unroll 24 // Always set the max number since it does not accept defines
    for (u8 i=0; i < VERTICAL_ROWS/2; ++i) {
        // Plane A row
        doDMAfast_fixed_args(vdpCtrl_ptr_l, RAM_FIXED_FRAME_BUFFER_ADDRESS + (VERTICAL_ROWS*TILEMAP_COLUMNS/2 + i*TILEMAP_COLUMNS)*2, 
            VDP_DMA_VRAM_ADDR(PA_ADDR + HALF_PLANE_ADDR_OFFSET_BYTES + i*PLANE_COLUMNS*2), TILEMAP_COLUMNS);
        // Plane B row
        doDMAfast_fixed_args(vdpCtrl_ptr_l, RAM_FIXED_FRAME_BUFFER_ADDRESS + (VERTICAL_ROWS*TILEMAP_COLUMNS + VERTICAL_ROWS*TILEMAP_COLUMNS/2 + i*TILEMAP_COLUMNS)*2, 
            VDP_DMA_VRAM_ADDR(PB_ADDR + HALF_PLANE_ADDR_OFFSET_BYTES + i*PLANE_COLUMNS*2), TILEMAP_COLUMNS);
    }
    #else
    #pragma GCC unroll 24 // Always set the max number since it does not accept defines
    for (u8 i=0; i < VERTICAL_ROWS; ++i) {
        // Plane A row
        doDMAfast_fixed_args(vdpCtrl_ptr_l, RAM_FIXED_FRAME_BUFFER_ADDRESS + i*TILEMAP_COLUMNS*2, 
            VDP_DMA_VRAM_ADDR(PA_ADDR + i*PLANE_COLUMNS*2), TILEMAP_COLUMNS);
        // Plane B row
        doDMAfast_fixed_args(vdpCtrl_ptr_l, RAM_FIXED_FRAME_BUFFER_ADDRESS + (VERTICAL_ROWS*TILEMAP_COLUMNS + i*TILEMAP_COLUMNS)*2, 
            VDP_DMA_VRAM_ADDR(PB_ADDR + i*PLANE_COLUMNS*2), TILEMAP_COLUMNS);
    }
    #endif
}

void render_mirror_planes_in_VRAM ()
{
    fb_mirror_planes_in_VRAM();
}

void render_copy_top_entries_in_VRAM ()
{
    fb_copy_top_entries_in_VRAM();
}