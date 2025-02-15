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
	// Create a buffer tile
	u8* tile = MEM_alloc(32); // 32 bytes per tile, layout: tile[4*8]
	memset(tile, 0, 32); // clear the tile with color index 0 (which is the BG color index)

	// 9 possible tile heights

	// Tile with only color 0 (height 0) goes at index 0
	VDP_loadTileData((u32*)tile, 0, 1, CPU);

	// Remaining 8 possible tile heights, distributed in 8 sets

    // fabri1983: we'll add 8 more tiles so we can use the other half of the palette.
    // First pass loads 8 tiles with colors 0..7.
    // Second pass loads 8 tiles with colors 0 and 8..14
    for (u8 pass=0; pass < 2; ++pass) {

        // 8 tiles per set
        for (u16 t = 1; t <= 8; t++) {
            memset(tile, 0, 32); // clear the tile with color index 0
            // 8 colors: they match with those from SGDK's ramp palettes (palette_grey, red, green, blue) first 8 colors
            for (u16 c = 0; c < 8; c++) {
                // Visit the heigh of each tile in current set
                for (u16 h = t-1; h < 8; h++) {
                    // Visit the columns of current row. 1 byte holds 2 colors as per Tile definition (4 bits per color),
                    // so lower byte is color c and higher byte is color c+1, letting the RCA video signal do the blending.
                    // We only fill most left 4 pixels of the tile, leaving the other 4 pixels with color 0. 
                    for (u16 b = 0; b < 2; b++) {
                        u8 colorLow = c;
                        if (pass == 1)
                            colorLow = c == 0 ? 0 : c+7;

                        u8 colorHigh = (c+1) == 8 ? c : c+1;
                        // We clamp to color 7 since starting at SGDK's palette 8th color they repeat
                        if (pass == 1)
                            colorHigh += 7;

                        u8 color = colorLow | (colorHigh << 4);
                        // This sets 2 pixels (remember color holds 2 color pixels) at [4*h] and [4*h + 1] 
                        // meaning we are leaving next 4 pixels at [4*h + 2] and [4*h + 2] as it is (currently 0)
                        tile[4*h + b] = color;
                    }
                }
                VDP_loadTileData((u32*)tile, c*8 + t + (pass*(8*8)), 1, CPU);
            }
        }
    }

	MEM_free(tile);

	// Returns next available tile index in VRAM
	return HUD_VRAM_START_INDEX; // 1 + 8*8 + 8*8
}

void render_loadWallPalettes ()
{
    // palette grey and palette red were already loaded

    // load colors used from green palette into PAL0
    PAL_setColors(PAL0*16 + 8, palette_green + 1, 7, DMA);
    // load colors used from blue palette into PAL1
    PAL_setColors(PAL1*16 + 8, palette_blue + 1, 7, DMA);
}

FORCE_INLINE void render_Z80_setBusProtection (bool value)
{
    Z80_requestBus(FALSE);
	u16 busProtectSignalAddress = (Z80_DRV_PARAMS + 0x0D) & 0xFFFF; // point to Z80 PROTECT parameter
    vu8* pb = (u8*) (Z80_RAM + busProtectSignalAddress); // See Z80_useBusProtection() reference in z80_ctrl.c
    *pb = value?1:0;
	Z80_releaseBus();
}

extern DMAOpInfo *dmaQueues;

FORCE_INLINE void render_DMA_flushQueue ()
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

FORCE_INLINE void render_SYS_doVBlankProcessEx_ON_VBLANK ()
{
    #if RENDER_ENABLE_FRAME_LOAD_CALCULATION
    render_setupFrameLoadCalculation();
    #endif

    // Wait until vint is triggered and returned from vint_callback()
    render_waitVInt_vtimer();

    // joy state refresh
    //JOY_update();
    joy_update_6btn();

    #if RENDER_ENABLE_FRAME_LOAD_CALCULATION
    render_calculateFrameLoad();
    showCPULoad(0, 24); // is shown on WINDOW plane
    //showFPS(0, 24);
    #endif
}

FORCE_INLINE void render_DMA_enqueue_framebuffer ()
{
    // DMA frame_buffer Plane A
    #if DMA_FRAMEBUFFER_A_EIGHT_CHUNKS_ON_DISPLAY_PERIOD_AND_HINT && DMA_FRAMEBUFFER_ROW_BY_ROW == FALSE
    // Send first 1/8 of frame_buffer Plane A
    DMA_doDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, ((VERTICAL_ROWS*PLANE_COLUMNS)/8)*1 - (PLANE_COLUMNS-TILEMAP_COLUMNS), -1);
    // The other 1/8 is sent in HInt
    // Remaining 6/8 of the frame_buffer Plane A
    DMA_queueDmaFast(DMA_VRAM, frame_buffer + ((VERTICAL_ROWS*PLANE_COLUMNS)/8)*2, PA_ADDR + EIGHTH_PLANE_ADDR_OFFSET_BYTES*2, 
        ((VERTICAL_ROWS*PLANE_COLUMNS)/8)*6 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
    #elif DMA_FRAMEBUFFER_A_FIRST_QUARTER_ON_HINT && DMA_FRAMEBUFFER_ROW_BY_ROW == FALSE
    // Remaining 3/4 of the frame_buffer Plane A if first 1/4 was sent in hint_load_hud_pals_callback()
    DMA_queueDmaFast(DMA_VRAM, frame_buffer + (VERTICAL_ROWS*PLANE_COLUMNS)/4, PA_ADDR + QUARTER_PLANE_ADDR_OFFSET_BYTES, 
        ((VERTICAL_ROWS*PLANE_COLUMNS)/4)*3 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
    #elif DMA_FRAMEBUFFER_A_FIRST_HALF_ON_HINT && DMA_FRAMEBUFFER_ROW_BY_ROW == FALSE
    // Remaining 1/2 of the frame_buffer Plane A if first 1/2 was sent in hint_load_hud_pals_callback()
    DMA_queueDmaFast(DMA_VRAM, frame_buffer + (VERTICAL_ROWS*PLANE_COLUMNS)/2, PA_ADDR + HALF_PLANE_ADDR_OFFSET_BYTES, 
        (VERTICAL_ROWS*PLANE_COLUMNS)/2 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
    #else
    #if RENDER_MIRROR_PLANES_USING_VDP_VRAM && DMA_FRAMEBUFFER_ROW_BY_ROW == FALSE
    // Half bottom frame_buffer Plane A
    DMA_queueDmaFast(DMA_VRAM, frame_buffer + (VERTICAL_ROWS*PLANE_COLUMNS)/2, PA_ADDR + HALF_PLANE_ADDR_OFFSET_BYTES, 
        (VERTICAL_ROWS*PLANE_COLUMNS)/2 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
    #elif DMA_FRAMEBUFFER_ROW_BY_ROW == FALSE
    // All the frame_buffer Plane A
    DMA_queueDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, (VERTICAL_ROWS*PLANE_COLUMNS) - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
    #endif
    #endif

    #if RENDER_MIRROR_PLANES_USING_VDP_VRAM && DMA_FRAMEBUFFER_ROW_BY_ROW == FALSE
    // Half bottom frame_buffer Plane B
    DMA_queueDmaFast(DMA_VRAM, frame_buffer + (VERTICAL_ROWS*PLANE_COLUMNS) + (VERTICAL_ROWS*PLANE_COLUMNS)/2, PB_ADDR + HALF_PLANE_ADDR_OFFSET_BYTES, 
        (VERTICAL_ROWS*PLANE_COLUMNS)/2 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
    #elif DMA_FRAMEBUFFER_ROW_BY_ROW == FALSE
    // All the frame_buffer Plane B
    DMA_queueDmaFast(DMA_VRAM, frame_buffer + (VERTICAL_ROWS*PLANE_COLUMNS), PB_ADDR, (VERTICAL_ROWS*PLANE_COLUMNS) - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
    #endif
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
        doDMAfast_fixed_args(vdpCtrl_ptr_l, FRAME_BUFFER_ADDRESS + (VERTICAL_ROWS*PLANE_COLUMNS/2 + i*PLANE_COLUMNS)*2, 
            VDP_DMA_VRAM_ADDR(PA_ADDR + HALF_PLANE_ADDR_OFFSET_BYTES + i*PLANE_COLUMNS*2), TILEMAP_COLUMNS);
        // Plane B row
        doDMAfast_fixed_args(vdpCtrl_ptr_l, FRAME_BUFFER_ADDRESS + (VERTICAL_ROWS*PLANE_COLUMNS + VERTICAL_ROWS*PLANE_COLUMNS/2 + i*PLANE_COLUMNS)*2, 
            VDP_DMA_VRAM_ADDR(PB_ADDR + HALF_PLANE_ADDR_OFFSET_BYTES + i*PLANE_COLUMNS*2), TILEMAP_COLUMNS);
    }
    #else
    #pragma GCC unroll 24 // Always set the max number since it does not accept defines
    for (u8 i=0; i < VERTICAL_ROWS; ++i) {
        // Plane A row
        doDMAfast_fixed_args(vdpCtrl_ptr_l, FRAME_BUFFER_ADDRESS + i*PLANE_COLUMNS*2, VDP_DMA_VRAM_ADDR(PA_ADDR + i*PLANE_COLUMNS*2), TILEMAP_COLUMNS);
        // Plane B row
        doDMAfast_fixed_args(vdpCtrl_ptr_l, FRAME_BUFFER_ADDRESS + (VERTICAL_ROWS*PLANE_COLUMNS + i*PLANE_COLUMNS)*2, VDP_DMA_VRAM_ADDR(PB_ADDR + i*PLANE_COLUMNS*2), TILEMAP_COLUMNS);
    }
    #endif
}

FORCE_INLINE void render_DMA_halved_mirror_planes ()
{
    fb_mirror_planes_in_VRAM();
}

FORCE_INLINE void render_copy_top_entries_in_VRAM ()
{
    fb_copy_top_entries_in_VRAM();
}