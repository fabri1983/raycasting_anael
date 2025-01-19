#include "game_loop.h"
#include <dma.h>
#include <sys.h>
#include <maths.h>
#include <joy.h>
#include "joy_6btn.h"
#include "utils.h"
#include "consts.h"
#include "map_matrix.h"
#include "frame_buffer.h"
#include "render.h"

#include "hud.h"
#include "weapons.h"
//#include <sprite_eng.h>
#include "spr_eng_override.h"

#include "tab_dir_xy.h"
#include "tab_wall_div.h"

#if RENDER_USE_TAB_COLOR_D8_1_PALS_SHIFTED && !RENDER_SHOW_TEXCOORD
    #include "tab_color_d8_1_pals_shft.h"
#elif !RENDER_USE_TAB_COLOR_D8_1_PALS_SHIFTED && !RENDER_SHOW_TEXCOORD
    #include "tab_color_d8_1.h"
#endif

#if RENDER_USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS
    #include "perf_hash_mulu_256_shft_FS.h"
    #include "tab_deltas_perf_hash.h"
#else
    #include "tab_deltas.h"
#endif

#include "map_hit_compressed.h"

static void dda (u16 posX, u16 posY, u16* delta_a_ptr);
#if RENDER_USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS
static void process_column (u16* delta_a_ptr, u16 posX, u16 posY, u32 sideDistX_l0, u32 sideDistX_l1, u32 sideDistY_l0, u32 sideDistY_l1);
#else
static void process_column (u16* delta_a_ptr, u16 posX, u16 posY, u16 sideDistX_l0, u16 sideDistX_l1, u16 sideDistY_l0, u16 sideDistY_l1);
#endif
static void do_stepping (u16 posX, u16 posY, u16 deltaDistX, u16 deltaDistY, u16 sideDistX, u16 sideDistY, s16 stepX, s16 stepY, s16 stepYMS, s16 rayDirAngleX, s16 rayDirAngleY);
#if USE_MAP_HIT_COMPRESSED
static void hit_map_do_stepping (u16 posX, u16 posY, u16 sideDistX, u16 sideDistY, s16 rayDirAngleX, s16 rayDirAngleY);
#endif
static void hitOnSideX (u16 sideDistX, u16 mapY, u16 posY, s16 rayDirAngleY);
static void hitOnSideY (u16 sideDistY, u16 mapX, u16 posX, s16 rayDirAngleX);

void game_loop () {

	// It seems positions in the map are multiple of FP +/- fraction. From (1*FP + MAP_FRACTION) to ((MAP_SIZE-1)*FP - MAP_FRACTION).
	// Smaller positions locate at top-left corner of the map[][] layout (as seen in the .h), bigger positions locate at bottom-right.
	// But dx and dy, applied to posX and posY respectively, goes from 0 to (+/-)FP/ANGLE_DIR_NORMALIZATION (used in tab_dir_xy.h).
	u16 posX = 2*FP, posY = 2*FP;

	// angle max value is 1023 and is updated in (1024/AP) units.
	// 0 points down in the map[], 256 points right, 512 points up, 768 points left, 1024 = 0.
	u16 angle = 0; 
    u16* delta_a_ptr = (u16*)tab_deltas;

    map_hit_reset_vars();
    map_hit_setRow(posX, posY, angle / (1024/AP));

	for (;;)
	{
		// clear the frame buffer
        #if RENDER_CLEAR_FRAMEBUFFER_WITH_SP
		clear_buffer_sp();
        #elif RENDER_CLEAR_FRAMEBUFFER
        clear_buffer();
        #endif

        // ceiling_copy_tilemap(BG_B, angle);
        // floor_copy_tilemap(BG_B, angle);
        // ceiling_dma_tileset(angle);
        // floor_dma_tileset(angle);

		//u16 joyState = JOY_readJoypad(JOY_1);
        u16 joyState = joy_readJoypad_joy1();

        if (joyState & BUTTON_X) {
            weapon_next(-1);
        }
        else if (joyState & BUTTON_Y) {
            weapon_next(1);
        }
        else if (joyState & BUTTON_A) {
            weapon_fire();
        }

        // movement and collisions
        if (joyState & (BUTTON_UP | BUTTON_DOWN | BUTTON_B | BUTTON_LEFT | BUTTON_RIGHT)) {

            // Direction amount and sign depending on angle
            s16 dx=0, dy=0;

            // Simple forward/backward movement
            if (joyState & BUTTON_UP) {
                dx = tab_dir_x_div24[angle];
                dy = tab_dir_y_div24[angle];
            }
            else if (joyState & BUTTON_DOWN) {
                dx = -tab_dir_x_div24[angle];
                dy = -tab_dir_y_div24[angle];
            }

            // Strafe movement is perpendicular to facing direction
            if (joyState & BUTTON_B) {
                // Strafe left
                if (joyState & BUTTON_LEFT) {
                    dx += tab_dir_y_div24[angle];
                    dy -= tab_dir_x_div24[angle];
                }
                // Strafe Right
                else if (joyState & BUTTON_RIGHT) {
                    dx -= tab_dir_y_div24[angle];
                    dy += tab_dir_x_div24[angle];
                }
            }
            // Rotation (only when not strafing)
            else {
                if (joyState & BUTTON_LEFT)
                    angle = (angle + (1024/AP)) & 1023;
                else if (joyState & BUTTON_RIGHT)
                    angle = (angle - (1024/AP)) & 1023;
            }

            // Current location (normalized) before displacement
            u16 x = posX / FP; // x > 0 always because min pos x is bigger than FP
            u16 y = posY / FP; // y > 0 always because min pos y is bigger than FP

            // Limit y axis location (normalized)
            const u16 ytop = (posY - (MAP_FRACTION-1)) / FP;
            const u16 ybottom = (posY + (MAP_FRACTION-1)) / FP;

            // Apply displacement
            posX += dx;
            posY += dy;

            // Check x axis collision
            // Moving right as per map[][] layout
            if (dx > 0) {
                if (map[y][x+1] || map[ytop][x+1] || map[ybottom][x+1]) {
                    posX = min(posX, (x+1)*FP - MAP_FRACTION);
                    x = posX / FP;
                }
            }
            // Moving left as per map[][] layout
            else {
                if (map[y][x-1] || map[ytop][x-1] || map[ybottom][x-1]) {
                    posX = max(posX, x*FP + MAP_FRACTION);
                    x = posX / FP;
                }
            }

            // Limit x axis location (normalized)
            const u16 xleft = (posX - (MAP_FRACTION-1)) / FP;
            const u16 xright = (posX + (MAP_FRACTION-1)) / FP;

            // Check y axis collision
            // Moving down as per map[][] layout
            if (dy > 0) {
                if (map[y+1][x] || map[y+1][xleft] || map[y+1][xright])
                    posY = min(posY, (y+1)*FP - MAP_FRACTION);
            }
            // Moving up as per map[][] layout
            else {
                if (map[y-1][x] || map[y-1][xleft] || map[y-1][xright])
                    posY = max(posY, y*FP + MAP_FRACTION);
            }

            u16 a = angle / (1024/AP); // a range is [0, 128)
            delta_a_ptr = (u16*) (tab_deltas + a * PIXEL_COLUMNS * DELTA_PTR_OFFSET_AMNT);

            map_hit_setRow(posX, posY, a);

            weapon_updateSway(dx | dy);
        }

        weapon_update();
        hud_update();
        spr_eng_update();

		// DDA (Digital Differential Analyzer)
		dda(posX, posY, delta_a_ptr);

        // DMA frame_buffer Plane A portion
        #if DMA_FRAMEBUFFER_A_EIGHT_CHUNKS_ON_DISPLAY_PERIOD_AND_HINT
        // Send first 1/8 of frame_buffer Plane A
        DMA_doDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, (VERTICAL_ROWS*PLANE_COLUMNS)/8 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
        // Remaining 6/8 of the frame_buffer Plane A
        DMA_queueDmaFast(DMA_VRAM, frame_buffer + ((VERTICAL_ROWS*PLANE_COLUMNS)/8)*2, PA_ADDR + EIGHTH_PLANE_ADDR_OFFSET*2, 
            ((VERTICAL_ROWS*PLANE_COLUMNS)/8)*6 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
        #elif DMA_FRAMEBUFFER_A_FIRST_QUARTER_ON_HINT
        // Remaining 3/4 of the frame_buffer Plane A if first 1/4 was sent in hint_callback()
        DMA_queueDmaFast(DMA_VRAM, frame_buffer + (VERTICAL_ROWS*PLANE_COLUMNS)/4, PA_ADDR + QUARTER_PLANE_ADDR_OFFSET, 
            ((VERTICAL_ROWS*PLANE_COLUMNS)/4)*3 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
        #elif DMA_FRAMEBUFFER_A_FIRST_HALF_ON_HINT
        // Remaining 1/2 of the frame_buffer Plane A if first 1/2 was sent in hint_callback()
        DMA_queueDmaFast(DMA_VRAM, frame_buffer + (VERTICAL_ROWS*PLANE_COLUMNS)/2, PA_ADDR + HALF_PLANE_ADDR_OFFSET, 
            (VERTICAL_ROWS*PLANE_COLUMNS)/2 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
        #else
        // All the frame_buffer Plane A
        DMA_queueDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, VERTICAL_ROWS*PLANE_COLUMNS - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
        #endif

        // DMA frame_buffer Plane B portion
        DMA_queueDmaFast(DMA_VRAM, frame_buffer + VERTICAL_ROWS*PLANE_COLUMNS, PB_ADDR, VERTICAL_ROWS*PLANE_COLUMNS - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);

        //SYS_doVBlankProcessEx(ON_VBLANK);
        render_SYS_doVBlankProcessEx_ON_VBLANK();

        #if RENDER_ENABLE_FRAME_LOAD_CALCULATION
        // showFPS(0, 1);
        showCPULoad(0, 1);
        #endif
	}
}

void game_loop_auto ()
{
    weapon_update();
    hud_update();
    spr_eng_update();

    // NOTE: here we are moving from the most UPPER-LEFT position of the map[][] layout, 
    // stepping DOWN into Y Axis, and RIGHT into X Axis, where in each position we do a full rotation.
    // Therefore we only interesting in collisions with x+1 and y+1.

    const u16 posStepping = 1;

    for (u16 posX = MIN_POS_XY; posX <= MAX_POS_XY; posX += posStepping) {

        for (u16 posY = MIN_POS_XY; posY <= MAX_POS_XY; posY += posStepping) {

            // Current location (normalized)
            u16 x = posX / FP; // x > 0 always because min pos x is bigger than FP
            u16 y = posY / FP; // y > 0 always because min pos y is bigger than FP

            // Limit y axis location (normalized)
            const u16 ytop = (posY - (MAP_FRACTION-1)) / FP;
            const u16 ybottom = (posY + (MAP_FRACTION-1)) / FP;

            // Check x axis collision
            // Moving right as per map[][] layout
            if (map[y][x+1] || map[ytop][x+1] || map[ybottom][x+1]) {
                if (posX > ((x+1)*FP - MAP_FRACTION)) {
                    // Move one block of map: (FP + 2*MAP_FRACTION) = 384 units. The block sizes FP, but we account for a safe distant to avoid clipping.
                    posX += (FP + 2*MAP_FRACTION) - posStepping;
                    break; // Stop current Y and continue with next X until it gets outside the collision
                }
            }
            // Moving left as per map[][] layout
            // else if (map[y][x-1] || map[ytop][x-1] || map[ybottom][x-1]) {
            //     if (posX < (x*FP + MAP_FRACTION)) {
            //         // Move one block of map: (FP + 2*MAP_FRACTION) = 384 units. The block sizes FP, but we account for a safe distant to avoid clipping.
            //         posX -= (FP + 2*MAP_FRACTION) + posStepping;
            //         break; // Stop current Y and continue with next X until it gets outside the collision
            //     }
            // }

            // Limit x axis location (normalized)
            const u16 xleft = (posX - (MAP_FRACTION-1)) / FP;
            const u16 xright = (posX + (MAP_FRACTION-1)) / FP;

            // Check y axis collision
            // Moving down as per map[][] layout
            if (map[y+1][x] || map[y+1][xleft] || map[y+1][xright]) {
                if (posY > ((y+1)*FP - MAP_FRACTION)) {
                    // Move one block of map: (FP + 2*MAP_FRACTION) = 384 units. The block sizes FP, but we account for a safe distant to avoid clipping.
                    posY += (FP + 2*MAP_FRACTION) - posStepping;
                    continue; // Continue with next Y until it gets outside the collision
                }
            }
            // Moving up as per map[][] layout
            // else if (map[y-1][x] || map[y-1][xleft] || map[y-1][xright]) {
            //     if (posY < (y*FP + MAP_FRACTION)) {
            //         // Move one block of map: (FP + 2*MAP_FRACTION) = 384 units. The block sizes FP, but we account for a safe distant to avoid clipping.
            //         posY -= (FP + 2*MAP_FRACTION) + posStepping;
            //         continue; // Continue with next Y until it gets outside the collision
            //     }
            // }

            for (u16 angle = 0; angle < 1024; angle += (1024/AP)) {

                // clear the frame buffer
                #if RENDER_CLEAR_FRAMEBUFFER_WITH_SP
                clear_buffer_sp();
                #elif RENDER_CLEAR_FRAMEBUFFER
                clear_buffer();
                #endif

                u16 a = angle / (1024/AP); // a range is [0, 128)
                u16* delta_a_ptr = (u16*) (tab_deltas + a * PIXEL_COLUMNS * DELTA_PTR_OFFSET_AMNT);

                // DDA (Digital Differential Analyzer)
                dda(posX, posY, delta_a_ptr);

                // DMA frame_buffer Plane A portion
                #if DMA_FRAMEBUFFER_A_EIGHT_CHUNKS_ON_DISPLAY_PERIOD_AND_HINT
                // Send first 1/8 of frame_buffer Plane A
                DMA_doDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, (VERTICAL_ROWS*PLANE_COLUMNS)/8 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
                // Remaining 6/8 of the frame_buffer Plane A
                DMA_queueDmaFast(DMA_VRAM, frame_buffer + ((VERTICAL_ROWS*PLANE_COLUMNS)/8)*2, PA_ADDR + EIGHTH_PLANE_ADDR_OFFSET*2, 
                    ((VERTICAL_ROWS*PLANE_COLUMNS)/8)*6 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
                #elif DMA_FRAMEBUFFER_A_FIRST_QUARTER_ON_HINT
                // Remaining 3/4 of the frame_buffer Plane A if first 1/4 was sent in hint_callback()
                DMA_queueDmaFast(DMA_VRAM, frame_buffer + (VERTICAL_ROWS*PLANE_COLUMNS)/4, PA_ADDR + QUARTER_PLANE_ADDR_OFFSET, 
                    ((VERTICAL_ROWS*PLANE_COLUMNS)/4)*3 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
                #elif DMA_FRAMEBUFFER_A_FIRST_HALF_ON_HINT
                // Remaining 1/2 of the frame_buffer Plane A if first 1/2 was sent in hint_callback()
                DMA_queueDmaFast(DMA_VRAM, frame_buffer + (VERTICAL_ROWS*PLANE_COLUMNS)/2, PA_ADDR + HALF_PLANE_ADDR_OFFSET, 
                    (VERTICAL_ROWS*PLANE_COLUMNS)/2 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
                #else
                // All the frame_buffer Plane A
                DMA_queueDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, VERTICAL_ROWS*PLANE_COLUMNS - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
                #endif

                // DMA frame_buffer Plane B portion
                DMA_queueDmaFast(DMA_VRAM, frame_buffer + VERTICAL_ROWS*PLANE_COLUMNS, PB_ADDR, VERTICAL_ROWS*PLANE_COLUMNS - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);

                //SYS_doVBlankProcessEx(ON_VBLANK);
                render_SYS_doVBlankProcessEx_ON_VBLANK();

                // handle inputs
                // u16 joyState = JOY_readJoypad(JOY_1);
                // u16 joyState = joy_readJoypad_joy1();
                // if (joyState & BUTTON_START)
                //     break;
            }
        }
    }
}

/// @brief Digital Differential Analyzer algorithm
/// @param posX 
/// @param posY 
/// @param delta_a_ptr tab_deltas at the current viewing angle
static void dda (u16 posX, u16 posY, u16* delta_a_ptr) {

    #if RENDER_USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS
    // Value goes from 0...FP (including), multiplied by MPH_VALUES_DELTADIST_NKEYS and by 2 for faster array acces in ASM
    u32 sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1;
    sideDistX_l0 = MPH_VALUES_DELTADIST_NKEYS * ASM_PERF_HASH_JUMP_BLOCK_SIZE_BYTES * (posX & (FP-1));
    sideDistX_l1 = MPH_VALUES_DELTADIST_NKEYS * ASM_PERF_HASH_JUMP_BLOCK_SIZE_BYTES * FP - sideDistX_l0;
    sideDistY_l0 = MPH_VALUES_DELTADIST_NKEYS * ASM_PERF_HASH_JUMP_BLOCK_SIZE_BYTES * (posY & (FP-1));
    sideDistY_l1 = MPH_VALUES_DELTADIST_NKEYS * ASM_PERF_HASH_JUMP_BLOCK_SIZE_BYTES * FP - sideDistY_l0;
    #else
    // Value goes from 0...FP (including)
    u16 sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1;
    sideDistX_l0 = posX & (FP-1); // (posX - (posX/FP)*FP);
    sideDistX_l1 = FP - sideDistX_l0; // (((posX/FP) + 1)*FP - posX);
    sideDistY_l0 = posY & (FP-1); // (posY - (posY/FP)*FP);
    sideDistY_l1 = FP - sideDistY_l0; // (((posY/FP) + 1)*FP - posY);
    #endif

    //u16 a = angle / (1024/AP); // a range is [0, 128)
    //u16* delta_a_ptr = (u16*) (tab_deltas + (a * PIXEL_COLUMNS * DELTA_PTR_OFFSET_AMNT));

    map_hit_setIndexForStartingColumn(0);

    // reset to the start of frame_buffer
    column_ptr = frame_buffer;

    // 256p or 320p width, but 4 "pixels" wide column => effectively 256/4=64 or 320/4=80 pixels width.
    for (u8 column = 0; column < PIXEL_COLUMNS; column += RENDER_COLUMNS_UNROLL) {
        #if RENDER_COLUMNS_UNROLL == 1

        process_column(delta_a_ptr, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
        map_hit_incrementColumn();
        if ((column & 1) == 0)
            column_ptr += VERTICAL_ROWS*PLANE_COLUMNS;
        else
            column_ptr += -VERTICAL_ROWS*PLANE_COLUMNS + 1;

        #elif RENDER_COLUMNS_UNROLL == 2

        process_column(delta_a_ptr + 0*DELTA_PTR_OFFSET_AMNT, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
        map_hit_incrementColumn();
        column_ptr += VERTICAL_ROWS*PLANE_COLUMNS;
        process_column(delta_a_ptr + 1*DELTA_PTR_OFFSET_AMNT, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
        map_hit_incrementColumn();
        column_ptr += -VERTICAL_ROWS*PLANE_COLUMNS + 1;

        #elif RENDER_COLUMNS_UNROLL == 4

        process_column(delta_a_ptr + 0*DELTA_PTR_OFFSET_AMNT, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
        map_hit_incrementColumn();
        column_ptr += VERTICAL_ROWS*PLANE_COLUMNS;
        process_column(delta_a_ptr + 1*DELTA_PTR_OFFSET_AMNT, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
        map_hit_incrementColumn();
        column_ptr += -VERTICAL_ROWS*PLANE_COLUMNS + 1;
        process_column(delta_a_ptr + 2*DELTA_PTR_OFFSET_AMNT, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
        map_hit_incrementColumn();
        column_ptr += VERTICAL_ROWS*PLANE_COLUMNS;
        process_column(delta_a_ptr + 3*DELTA_PTR_OFFSET_AMNT, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
        map_hit_incrementColumn();
        column_ptr += -VERTICAL_ROWS*PLANE_COLUMNS + 1;
        #endif

        delta_a_ptr += RENDER_COLUMNS_UNROLL * DELTA_PTR_OFFSET_AMNT;
    }
}

#if RENDER_USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS
static FORCE_INLINE void process_column (u16* delta_a_ptr, u16 posX, u16 posY, u32 sideDistX_l0, u32 sideDistX_l1, u32 sideDistY_l0, u32 sideDistY_l1)
#else
static FORCE_INLINE void process_column (u16* delta_a_ptr, u16 posX, u16 posY, u16 sideDistX_l0, u16 sideDistX_l1, u16 sideDistY_l0, u16 sideDistY_l1)
#endif
{
	const u16 deltaDistX = *(delta_a_ptr + 0); // value from 182 up to 65535, but only 915 different values
	const u16 deltaDistY = *(delta_a_ptr + 1); // value from 182 up to 65535, but only 915 different values
	const s16 rayDirAngleX = (s16) *(delta_a_ptr + 2); // value from 0 up to 65535 (unsigned), but only 717 signed different values in [-360, 360]
	const s16 rayDirAngleY = (s16) *(delta_a_ptr + 3); // value from 0 up to 65535 (unsigned), but only 717 signed different values in [-360, 360]
    #if RENDER_USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS
    const u16 deltaDistX_perf_hash = *(delta_a_ptr + 4); // 0..MPH_VALUES_DELTADIST_NKEYS-1 multiplied by 2 for faster ASM access
	const u16 deltaDistY_perf_hash = *(delta_a_ptr + 5); // 0..MPH_VALUES_DELTADIST_NKEYS-1 multiplied by 2 for faster ASM access
    #endif

	u16 sideDistX, sideDistY; // effective value goes up to 4096-1 once in the stepping loop due to condition control
	s16 stepX, stepY, stepYMS;

	if (rayDirAngleX < 0) {
		stepX = -1;
        #if RENDER_USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS
        sideDistX = perf_hash_mulu_shft_FS(sideDistX_l0, deltaDistX_perf_hash);
        #else
        sideDistX = mulu_shft_FS(sideDistX_l0, deltaDistX); //(u16)(mulu(sideDistX_l0, deltaDistX) >> FS);
        #endif
	}
	else {
		stepX = 1;
        #if RENDER_USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS
        sideDistX = perf_hash_mulu_shft_FS(sideDistX_l1, deltaDistX_perf_hash);
        #else
        sideDistX = mulu_shft_FS(sideDistX_l1, deltaDistX); //(u16)(mulu(sideDistX_l1, deltaDistX) >> FS);
        #endif
	}

	if (rayDirAngleY < 0) {
		stepY = -1;
		stepYMS = -MAP_SIZE;
		#if RENDER_USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS
        sideDistY = perf_hash_mulu_shft_FS(sideDistY_l0, deltaDistY_perf_hash);
        #else
        sideDistY = mulu_shft_FS(sideDistY_l0, deltaDistY); //(u16)(mulu(sideDistY_l0, deltaDistY) >> FS);
        #endif
	}
	else {
		stepY = 1;
		stepYMS = MAP_SIZE;
		#if RENDER_USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS
        sideDistY = perf_hash_mulu_shft_FS(sideDistY_l1, deltaDistY_perf_hash);
        #else
        sideDistY = mulu_shft_FS(sideDistY_l1, deltaDistY); //(u16)(mulu(sideDistY_l1, deltaDistY) >> FS);
        #endif
	}

    #if RENDER_USE_MAP_HIT_COMPRESSED
    hit_map_do_stepping(posX, posY, sideDistX, sideDistY, rayDirAngleX, rayDirAngleY);
    #else
    do_stepping(posX, posY, deltaDistX, deltaDistY, sideDistX, sideDistY, stepX, stepY, stepYMS, rayDirAngleX, rayDirAngleY);
    #endif
}

static FORCE_INLINE void do_stepping (u16 posX, u16 posY, u16 deltaDistX, u16 deltaDistY, u16 sideDistX, u16 sideDistY, s16 stepX, s16 stepY, s16 stepYMS, s16 rayDirAngleX, s16 rayDirAngleY)
{
    u16 mapX = posX / FP;
	u16 mapY = posY / FP;
	const u8* map_ptr = &map[mapY][mapX];

	for (u16 n = 0; n < STEP_COUNT_LOOP; ++n) {

		// side X
		if (sideDistX < sideDistY) {
			mapX += stepX;
			map_ptr += stepX;
			u16 hit = *map_ptr; // map[mapY][mapX];
			if (hit) {
                hitOnSideX(sideDistX, mapY, posY, rayDirAngleY);
				break;
			}
			sideDistX += deltaDistX;
		}
		// side Y
		else {
			mapY += stepY;
			map_ptr += stepYMS;
			u16 hit = *map_ptr; // map[mapY][mapX];
			if (hit) {
                hitOnSideY(sideDistY, mapX, posX, rayDirAngleX);
				break;
			}
			sideDistY += deltaDistY;
		}
	}
}

#if USE_MAP_HIT_COMPRESSED
static FORCE_INLINE void hit_map_do_stepping (u16 posX, u16 posY, u16 sideDistX, u16 sideDistY, s16 rayDirAngleX, s16 rayDirAngleY)
{
    u16 hit_value = map_hit_decompressAt();
    // if (hit_value == 0)
    //     return;

    // The value is compacted as next layout:
    //   16 bits:  dddddddddddd    mmmm
    //              sideDistXY     mapXY
    u16 hit_mapXY = (hit_value >> MAP_HIT_OFFSET_MAPXY) & MAP_HIT_MASK_MAPXY;
    u16 hit_sideDistXY = (hit_value >> MAP_HIT_OFFSET_SIDEDISTXY);// & MAP_HIT_MASK_SIDEDISTXY;

    // side X
    if (hit_sideDistXY < sideDistY) {
        hitOnSideX(hit_sideDistXY, hit_mapXY, posY, rayDirAngleY);
    }
    // side Y
    else {
        hitOnSideY(hit_sideDistXY, hit_mapXY, posX, rayDirAngleX);
    }
}
#endif

static FORCE_INLINE void hitOnSideX (u16 sideDistX, u16 mapY, u16 posY, s16 rayDirAngleY)
{
    u16 h2 = tab_wall_div[sideDistX]; // height halved

    #if RENDER_SHOW_TEXCOORD
    u16 d = (7 - min(7, sideDistX / FP));
    u16 wallY = posY + (mulu(sideDistX, rayDirAngleY) >> FS);
    //wallY = ((wallY * 8) >> FS) & 7; // Faster? But is actually slower given the context
    wallY = max((wallY - mapY*FP) * 8 / FP, 0); // cleaner
    u16 tileAttrib = (PAL0 << TILE_ATTR_PALETTE_SFT) + 1 + min(d, wallY)*8 + (mapY&1)*(8*8);
    #elif RENDER_USE_TAB_COLOR_D8_1_PALS_SHIFTED && !RENDER_SHOW_TEXCOORD
    u16 tileAttrib = tab_color_d8_1_X_pals_shft[sideDistX + sideDistX + (mapY&1)];
    #else
    u8 d8_1 = tab_color_d8_1[sideDistX]; // the bigger the distant the darker the color is
    u16 tileAttrib;
    if (mapY&1) tileAttrib = (PAL0 << TILE_ATTR_PALETTE_SFT) | (d8_1 + (8*8)); // use the tiles that point to second half of wall's palette
    else tileAttrib = (PAL0 << TILE_ATTR_PALETTE_SFT) | d8_1;
    #endif

    write_vline(h2, tileAttrib);
}

static FORCE_INLINE void hitOnSideY (u16 sideDistY, u16 mapX, u16 posX, s16 rayDirAngleX)
{
    u16 h2 = tab_wall_div[sideDistY]; // height halved

    #if RENDER_SHOW_TEXCOORD
    u16 d = (7 - min(7, sideDistY / FP));
    u16 wallX = posX + (mulu(sideDistY, rayDirAngleX) >> FS);
    //wallX = ((wallX * 8) >> FS) & 7; // Faster? But is actually slower given the context
    wallX = max((wallX - mapX*FP) * 8 / FP, 0); // cleaner
    u16 tileAttrib = (PAL1 << TILE_ATTR_PALETTE_SFT) + 1 + min(d, wallX)*8 + (mapX&1)*(8*8);
    #elif RENDER_USE_TAB_COLOR_D8_1_PALS_SHIFTED && !RENDER_SHOW_TEXCOORD
    u16 tileAttrib = tab_color_d8_1_Y_pals_shft[sideDistY + sideDistY + (mapX&1)];
    #else
    u8 d8_1 = tab_color_d8_1[sideDistY]; // the bigger the distant the darker the color is
    u16 tileAttrib;
    if (mapX&1) tileAttrib = (PAL1 << TILE_ATTR_PALETTE_SFT) | (d8_1 + (8*8)); // use the tiles that point to second half of wall's palette
    else tileAttrib = (PAL1 << TILE_ATTR_PALETTE_SFT) | d8_1;
    #endif

    write_vline(h2, tileAttrib);
}