#include <dma.h>
#include <sys.h>
#include <maths.h>
#include <joy.h>
#include "joy_6btn.h"
#include "utils.h"
#include "consts.h"
#include "consts_ext.h"
#include "map_matrix.h"
#include "frame_buffer.h"
#include "render.h"

#include "hud.h"
#include "weapon.h"
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

#if RENDER_USE_MAP_HIT_COMPRESSED
#include "map_hit_compressed.h"
#endif

#include "game_loop.h"

static void dda (u16 posX, u16 posY, u16* delta_a_ptr);

#if RENDER_USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS
static void process_column (u16* delta_a_ptr, u16 posX, u16 posY, u32 sideDistX_l0, u32 sideDistX_l1, u32 sideDistY_l0, u32 sideDistY_l1);
#else
static void process_column (u16* delta_a_ptr, u16 posX, u16 posY, u16 sideDistX_l0, u16 sideDistX_l1, u16 sideDistY_l0, u16 sideDistY_l1);
#endif

#if RENDER_USE_MAP_HIT_COMPRESSED
static void do_stepping (u16 posX, u16 posY, u16 sideDistX, u16 sideDistY, s16 rayDirAngleX, s16 rayDirAngleY);
#else
static void do_stepping (u16 posX, u16 posY, u16 deltaDistX, u16 deltaDistY, u16 sideDistX, u16 sideDistY, s16 stepX, s16 stepY, s16 stepYMS, s16 rayDirAngleX, s16 rayDirAngleY);
#endif

static void hitOnSideX (u16 sideDistX, u16 mapY, u16 posY, s16 rayDirAngleY);
static void hitOnSideY (u16 sideDistY, u16 mapX, u16 posX, s16 rayDirAngleX);

static void clearBuffer ()
{
    #if RENDER_MIRROR_PLANES_USING_VDP_VRAM
        // ramebuffer is cleared while VRAM to VRAM copy async ops are running. See fb_mirror_planes_in_VRAM().
    #else
        #if RENDER_CLEAR_FRAMEBUFFER_NO_USP
            #if RENDER_HALVED_PLANES
            clear_buffer_halved_no_usp();
            #else
            clear_buffer_no_usp();
            #endif
        #elif RENDER_CLEAR_FRAMEBUFFER
            #if RENDER_HALVED_PLANES
            clear_buffer_halved();
            #else
            clear_buffer();
            #endif
        #elif RENDER_CLEAR_FRAMEBUFFER_WITH_SP
            #if RENDER_HALVED_PLANES
            clear_buffer_halved_sp();
            #else
            clear_buffer_sp();
            #endif
        #endif
    #endif
}

static void handle_input(u16* posX, u16* posY, u16* angle, u16** delta_a_ptr)
{
    u16 joyState = joy_readJoypad_joy1();
    // if (joyState & BUTTON_START)
    //     break;

    if (joyState & (u16)BUTTON_X) {
        weapon_next(-1);
    }
    else if (joyState & (u16)BUTTON_Y) {
        weapon_next(1);
    }
    else if (joyState & (u16)BUTTON_A) {
        weapon_fire();
    }

    // movement and collisions
    if (joyState & (u16)(BUTTON_UP | BUTTON_DOWN | BUTTON_B | BUTTON_LEFT | BUTTON_RIGHT)) {

        // Direction amount and sign depending on angle
        s16 dx=0, dy=0;

        // Simple forward/backward movement
        if (joyState & (u16)BUTTON_UP) {
            dx = tab_dir_x_div24[*angle];
            dy = tab_dir_y_div24[*angle];
        }
        else if (joyState & (u16)BUTTON_DOWN) {
            dx = -tab_dir_x_div24[*angle];
            dy = -tab_dir_y_div24[*angle];
        }

        // Strafe movement is perpendicular to facing direction
        if (joyState & (u16)BUTTON_B) {
            // Strafe left
            if (joyState & (u16)BUTTON_LEFT) {
                dx += tab_dir_y_div24[*angle];
                dy -= tab_dir_x_div24[*angle];
            }
            // Strafe Right
            else if (joyState & (u16)BUTTON_RIGHT) {
                dx -= tab_dir_y_div24[*angle];
                dy += tab_dir_x_div24[*angle];
            }
        }
        // Rotation (only when not strafing)
        else {
            if (joyState & (u16)BUTTON_LEFT)
                *angle = (*angle + (u16)(1024/AP)) & (u16)1023;
            else if (joyState & (u16)BUTTON_RIGHT)
                *angle = (*angle - (u16)(1024/AP)) & (u16)1023;
        }

        // Current location (normalized) before displacement
        u16 x = *posX / (u16)FP; // x > 0 always because min pos x is bigger than FP
        u16 y = *posY / (u16)FP; // y > 0 always because min pos y is bigger than FP

        // Limit y axis location (normalized)
        const u16 ytop = (*posY - (u16)(MAP_FRACTION-1)) / (u16)FP;
        const u16 ybottom = (*posY + (u16)(MAP_FRACTION-1)) / (u16)FP;

        // Apply displacement
        *posX += dx;
        *posY += dy;

        // Check x axis collision
        // Moving right as per map[][] layout
        if (dx > 0) {
            if (map[y][x+1] || map[ytop][x+1] || map[ybottom][x+1]) {
                *posX = min(*posX, (x+1)*(u16)FP - (u16)MAP_FRACTION);
                x = *posX / FP;
            }
        }
        // Moving left as per map[][] layout
        else {
            if (map[y][x-1] || map[ytop][x-1] || map[ybottom][x-1]) {
                *posX = max(*posX, x*(u16)FP + (u16)MAP_FRACTION);
                x = *posX / FP;
            }
        }

        // Limit x axis location (normalized)
        const u16 xleft = (*posX - (u16)(MAP_FRACTION-1)) / (u16)FP;
        const u16 xright = (*posX + (u16)(MAP_FRACTION-1)) / (u16)FP;

        // Check y axis collision
        // Moving down as per map[][] layout
        if (dy > 0) {
            if (map[y+1][x] || map[y+1][xleft] || map[y+1][xright])
                *posY = min(*posY, (y+1)*(u16)FP - (u16)MAP_FRACTION);
        }
        // Moving up as per map[][] layout
        else {
            if (map[y-1][x] || map[y-1][xleft] || map[y-1][xright])
                *posY = max(*posY, y*(u16)FP + (u16)MAP_FRACTION);
        }

        u16 a = *angle / (u16)(1024/AP); // a range is [0, 128)
        *delta_a_ptr = (u16*) (tab_deltas + a * PIXEL_COLUMNS * DELTA_PTR_OFFSET_AMNT);

        #if RENDER_USE_MAP_HIT_COMPRESSED
        map_hit_setRow(*posX, *posY, a);
        #endif

        weapon_updateSway(dx | dy);
    }
}

void game_loop ()
{
	// It seems positions in the map are multiple of FP +/- fraction. From (1*FP + MAP_FRACTION) to ((MAP_SIZE-1)*FP - MAP_FRACTION).
	// Smaller positions locate at top-left corner of the map[][] layout (as seen in the .h), bigger positions locate at bottom-right.
	// But dx and dy, applied to posX and posY respectively, goes from 0 to (+/-)FP/ANGLE_DIR_NORMALIZATION (used in tab_dir_xy.h).
	u16 posX = 2*FP - 3*MAP_FRACTION, posY = 2*FP;

	// angle max value is 1023 and is updated in (1024/AP) units.
	// 0 points down in the map[], 256 points right, 512 points up, 768 points left, 1024 = 0.
	u16 angle = 0; 
    u16* delta_a_ptr = (u16*)tab_deltas;

    #if RENDER_USE_MAP_HIT_COMPRESSED
    map_hit_reset_vars();
    map_hit_setRow(posX, posY, angle / (1024/AP));
    #endif

	for (;;)
	{
		// clear the frame buffer
        clearBuffer();

        // ceiling_copy_tilemap(BG_B, angle);
        // floor_copy_tilemap(BG_B, angle);
        // ceiling_dma_tileset(angle);
        // floor_dma_tileset(angle);

        handle_input(&posX, &posY, &angle, &delta_a_ptr);
        weapon_update();
        hud_update();
        spr_eng_update();

		dda(posX, posY, delta_a_ptr);

        render_SYS_doVBlankProcessEx_ON_VBLANK();
	}
}

void game_loop_auto ()
{
    // NOTE: here we are moving from the most UPPER-LEFT position of the map[][] layout, 
    // stepping DOWN into Y Axis, and RIGHT into X Axis, where in each position we do a full rotation.
    // Therefore we only interesting in collisions with x+1 and y+1.

    const u16 posStepping = 1;

    #pragma GCC unroll 0 // do not unroll
    for (u16 posX = MIN_POS_XY; posX <= MAX_POS_XY; posX += posStepping) {

        #pragma GCC unroll 0 // do not unroll
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
                clearBuffer();

                weapon_update();
                hud_update();
                spr_eng_update();

                u16 a = angle / (1024/AP); // a range is [0, 128)
                u16* delta_a_ptr = (u16*) (tab_deltas + a * PIXEL_COLUMNS * DELTA_PTR_OFFSET_AMNT);

                dda(posX, posY, delta_a_ptr);

                render_SYS_doVBlankProcessEx_ON_VBLANK();

                // handle inputs
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
static void dda (u16 posX, u16 posY, u16* delta_a_ptr)
{
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

    //u16 a = angle / (u16)(1024/AP); // a range is [0, 128)
    //u16* delta_a_ptr = (u16*) (tab_deltas + (a * PIXEL_COLUMNS * DELTA_PTR_OFFSET_AMNT));

    // ALWAYS starts rendering process at column 0, given that column_ptr incremental amount is hardcoded and assumes first column is even.
    // If the target display dimensions are lower than full screen (except the hud) then just adjust PA_ADDR and PB_ADDR.
    u16 column = 0;

    #if RENDER_USE_MAP_HIT_COMPRESSED
    map_hit_setIndexForStartingColumn(column);
    #endif
    #if RENDER_MIRROR_PLANES_USING_CPU_RAM || RENDER_MIRROR_PLANES_USING_VDP_VRAM
    fb_set_top_entries_column(column);
    #endif

    // reset to the start of frame_buffer
    column_ptr = (u16*) RAM_FIXED_FRAME_BUFFER_ADDRESS;

    #if RENDER_COLUMNS_UNROLL == 1
    s16 offset_xor = -VERTICAL_ROWS*TILEMAP_COLUMNS + 1;
    #endif

    // 256p or 320p width, but 4 "pixels" wide column => effectively 256/4=64 or 320/4=80 pixels width.
    #pragma GCC unroll 0 // do not unroll
    for (; column < (u16)PIXEL_COLUMNS; column += (u16)RENDER_COLUMNS_UNROLL) {
        #if RENDER_COLUMNS_UNROLL == 1

        process_column(delta_a_ptr, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
        #if RENDER_MIRROR_PLANES_USING_CPU_RAM || RENDER_MIRROR_PLANES_USING_VDP_VRAM
        fb_increment_entries_column();
        #endif
        #if RENDER_USE_MAP_HIT_COMPRESSED
        map_hit_incrementColumn();
        #endif
        // cycles between (-VERTICAL_ROWS*TILEMAP_COLUMNS + 1) and (VERTICAL_ROWS*TILEMAP_COLUMNS)
        offset_xor ^= (-VERTICAL_ROWS*TILEMAP_COLUMNS + 1) ^ (VERTICAL_ROWS*TILEMAP_COLUMNS);
        column_ptr += offset_xor;

        #elif RENDER_COLUMNS_UNROLL == 2

        process_column(delta_a_ptr + 0*DELTA_PTR_OFFSET_AMNT, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
        #if RENDER_MIRROR_PLANES_USING_CPU_RAM || RENDER_MIRROR_PLANES_USING_VDP_VRAM
        fb_increment_entries_column();
        #endif
        #if RENDER_USE_MAP_HIT_COMPRESSED
        map_hit_incrementColumn();
        #endif
        column_ptr += VERTICAL_ROWS*TILEMAP_COLUMNS; // jumps into Plane B region of framebuffer

        process_column(delta_a_ptr + 1*DELTA_PTR_OFFSET_AMNT, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
        #if RENDER_MIRROR_PLANES_USING_CPU_RAM || RENDER_MIRROR_PLANES_USING_VDP_VRAM
        fb_increment_entries_column();
        #endif
        #if RENDER_USE_MAP_HIT_COMPRESSED
        map_hit_incrementColumn();
        #endif
        column_ptr += -VERTICAL_ROWS*TILEMAP_COLUMNS + 1; // go back to Plane A region of framebuffer and advance one tilemap entry

        #elif RENDER_COLUMNS_UNROLL == 4

        process_column(delta_a_ptr + 0*DELTA_PTR_OFFSET_AMNT, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
        #if RENDER_MIRROR_PLANES_USING_CPU_RAM || RENDER_MIRROR_PLANES_USING_VDP_VRAM
        fb_increment_entries_column();
        #endif
        #if RENDER_USE_MAP_HIT_COMPRESSED
        map_hit_incrementColumn();
        #endif
        column_ptr += VERTICAL_ROWS*TILEMAP_COLUMNS; // jumps into Plane B region of framebuffer

        process_column(delta_a_ptr + 1*DELTA_PTR_OFFSET_AMNT, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
        #if RENDER_MIRROR_PLANES_USING_CPU_RAM || RENDER_MIRROR_PLANES_USING_VDP_VRAM
        fb_increment_entries_column();
        #endif
        #if RENDER_USE_MAP_HIT_COMPRESSED
        map_hit_incrementColumn();
        #endif
        column_ptr += -VERTICAL_ROWS*TILEMAP_COLUMNS + 1; // go back to Plane A region of framebuffer and advance one tilemap entry

        process_column(delta_a_ptr + 2*DELTA_PTR_OFFSET_AMNT, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
        #if RENDER_MIRROR_PLANES_USING_CPU_RAM || RENDER_MIRROR_PLANES_USING_VDP_VRAM
        fb_increment_entries_column();
        #endif
        #if RENDER_USE_MAP_HIT_COMPRESSED
        map_hit_incrementColumn();
        #endif
        column_ptr += VERTICAL_ROWS*TILEMAP_COLUMNS; // jumps into Plane B region of framebuffer

        process_column(delta_a_ptr + 3*DELTA_PTR_OFFSET_AMNT, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
        #if RENDER_MIRROR_PLANES_USING_CPU_RAM || RENDER_MIRROR_PLANES_USING_VDP_VRAM
        fb_increment_entries_column();
        #endif
        #if RENDER_USE_MAP_HIT_COMPRESSED
        map_hit_incrementColumn();
        #endif
        column_ptr += -VERTICAL_ROWS*TILEMAP_COLUMNS + 1; // go back to Plane A region of framebuffer and advance one tilemap entry

        #endif

        delta_a_ptr += RENDER_COLUMNS_UNROLL * DELTA_PTR_OFFSET_AMNT;
    }

    #if RENDER_MIRROR_PLANES_USING_CPU_RAM
    fb_mirror_planes_in_RAM();
    #endif
}

#if RENDER_USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS
static FORCE_INLINE void process_column (u16* delta_a_ptr, u16 posX, u16 posY, u32 sideDistX_l0, u32 sideDistX_l1, u32 sideDistY_l0, u32 sideDistY_l1)
#else
static FORCE_INLINE void process_column (u16* delta_a_ptr, u16 posX, u16 posY, u16 sideDistX_l0, u16 sideDistX_l1, u16 sideDistY_l0, u16 sideDistY_l1)
#endif
{
    // Length of ray from one X or Y side to next X or Y side. Value from 182 up to 65535, but only 915 different values
	const u16 deltaDistX = delta_a_ptr[0], deltaDistY = delta_a_ptr[1];
    // Value from 0 up to 65535 (unsigned), but only 717 signed different values in [-360, 360]
	const s16 rayDirAngleX = (s16) delta_a_ptr[2], rayDirAngleY = (s16) delta_a_ptr[3];
    #if RENDER_USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS
    // 0..MPH_VALUES_DELTADIST_NKEYS-1 multiplied by 2 for faster memory access in ASM
    const u16 deltaDistX_perf_hash = delta_a_ptr[4], deltaDistY_perf_hash = delta_a_ptr[5];
    #endif

    // Length of ray from current position to next X or Y side. Wffective value goes up to 4096-1 once in the stepping loop due to condition control
	u16 sideDistX, sideDistY;
    // What direction to step in X or Y direction (either +1 or -1)
	s16 stepX, stepY, stepYMS;

    // Calculate step and initial sideDist

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
    do_stepping(posX, posY, sideDistX, sideDistY, rayDirAngleX, rayDirAngleY);
    #else
    do_stepping(posX, posY, deltaDistX, deltaDistY, sideDistX, sideDistY, stepX, stepY, stepYMS, rayDirAngleX, rayDirAngleY);
    #endif
}

#if RENDER_USE_MAP_HIT_COMPRESSED
static void do_stepping (u16 posX, u16 posY, u16 sideDistX, u16 sideDistY, s16 rayDirAngleX, s16 rayDirAngleY)
{
    u16 hit_value = map_hit_decompressAt();
    // if (hit_value == 0)
    //     return;

    // The value is compacted as next layout:
    //   16 bits:  dddddddddddd    mmmm
    //              sideDistXY     mapXY
    //              (12 bits)     (4 bits)
    u16 hit_mapXY = (hit_value >> MAP_HIT_OFFSET_MAPXY) & MAP_HIT_MASK_MAPXY;
    u16 hit_sideDistXY = (hit_value >> MAP_HIT_OFFSET_SIDEDISTXY);// & MAP_HIT_MASK_SIDEDISTXY;

    // Jump to next map square, either in X or Y direction
    // Side X
    if (hit_sideDistXY < sideDistY) {
        hitOnSideX(hit_sideDistXY, hit_mapXY, posY, rayDirAngleY);
    }
    // Side Y
    else {
        hitOnSideY(hit_sideDistXY, hit_mapXY, posX, rayDirAngleX);
    }
}
#else
static void do_stepping (u16 posX, u16 posY, u16 deltaDistX, u16 deltaDistY, u16 sideDistX, u16 sideDistY, s16 stepX, s16 stepY, s16 stepYMS, s16 rayDirAngleX, s16 rayDirAngleY)
{
    // Which box of the map we're in
    u16 mapX = posX / (u16)FP;
	u16 mapY = posY / (u16)FP;
	u8* map_ptr = (u8*) &map[mapY][mapX];

    // Now the actual DDA starts. It's a loop that increments the ray in 1 square every time, until a wall is hit.
    //#pragma GCC unroll 0 // do not unroll
	for (u16 n = 0; n < STEP_COUNT_LOOP; ++n) {

        // Jump into next map square, either in X or Y direction

		// Side X
		if (sideDistX < sideDistY) {
			mapX += stepX;
			map_ptr += stepX;
			u16 hit = *map_ptr; // map[mapY][mapX];
			if (hit) {
                hitOnSideX(sideDistX, mapY, posY, rayDirAngleY);
				return;
			}
			sideDistX += deltaDistX;
		}
		// Side Y
		else {
			mapY += stepY;
			map_ptr += stepYMS;
			u16 hit = *map_ptr; // map[mapY][mapX];
			if (hit) {
                hitOnSideY(sideDistY, mapX, posX, rayDirAngleX);
				return;
			}
			sideDistY += deltaDistY;
		}
	}
}
#endif

static void hitOnSideX (u16 sideDistX, u16 mapY, u16 posY, s16 rayDirAngleY)
{
    #if RENDER_SHOW_TEXCOORD

    // We only need the Texture X coordinate because we stay in the same vertical stripe of the screen.
    // Calculate the X coordinate where exactly the wall was hit (this is not the final Texture X coordinate)
    u16 wallTexX = posY + (mulu(sideDistX, rayDirAngleY) >> FS);
    // Substracting the integer value of the wall off it
    //wallTexX = ((wallTexX * 8) >> FS) & 7; // Faster? But is actually slower given the context
    wallTexX = max((wallTexX - mapY*FP) * 8 / FP, 0); // cleaner
    // Now let's calculate a color to represent the wall X coordinate (not the final Texture X coordinate)
    u16 d = (7 - min(7, sideDistX / FP));
    u16 tileAttrib;
    if (mapY&1) tileAttrib = (PAL0 << TILE_ATTR_PALETTE_SFT) + 1 + min(d, wallTexX)*8 + (8*8);
    else tileAttrib = (PAL0 << TILE_ATTR_PALETTE_SFT) + 1 + min(d, wallTexX)*8
    u16 h2 = tab_wall_div[sideDistX]; // height halved

    #elif RENDER_USE_TAB_COLOR_D8_1_PALS_SHIFTED && !RENDER_SHOW_TEXCOORD

    // C version
    // u16 tileAttrib = tab_color_d8_1_X_pals_shft[2*sideDistX + (mapY&1)]; // *2 because each element has one value for (mapY&1)=0 and other for (mapY&1)=1
    // u16 h2 = tab_wall_div[sideDistX]; // height halved

    // ASM version
    u16 tileAttrib;
    u16 h2;
    __asm volatile (
        "andi.w  #1,%[mapY]\n\t"                        // mapY &= 1
        "add.w   %[sideDistX],%[sideDistX]\n\t"         // sideDistX *= 2 (discern between element for (mapY&1)=0 and element for (mapY&1)=1)
        "move.w  (%[tab_wall_div],%[sideDistX].w),%[h2]\n\t" // h2 = tab_wall_div[sideDistX]
        "add.w   %[mapY],%[sideDistX]\n\t"              // index = 2*sideDistX + (mapY & 1)
        "add.w   %[sideDistX],%[sideDistX]\n\t"         // byte offset (word stride)
        "lea     %c[tab_color_mem],%[tab_wall_div]\n\t"  // this way we save one register
        "move.w  (%[tab_wall_div],%[sideDistX].w),%[tileAttrib]" // tileAttrib = tab_color_d8_1_X_pals_shft[index]
        : [h2] "=d" (h2), [tileAttrib] "=d" (tileAttrib), [sideDistX] "+d" (sideDistX), [mapY] "+d" (mapY)
        : [tab_wall_div] "a" (tab_wall_div), [tab_color_mem] "s" (tab_color_d8_1_X_pals_shft)
        :
    );

    #else

    u8 d8_1 = tab_color_d8_1[sideDistX]; // the bigger the distant the darker the color is
    u16 tileAttrib;
    if (mapY&1) tileAttrib = (PAL0 << TILE_ATTR_PALETTE_SFT) + d8_1 + (8*8); // use the tiles that point to second half of wall's palette
    else tileAttrib = (PAL0 << TILE_ATTR_PALETTE_SFT) + d8_1;
    u16 h2 = tab_wall_div[sideDistX]; // height halved

    #endif

    #if RENDER_HALVED_PLANES
    write_vline_halved(h2, tileAttrib);
    #else
    write_vline(h2, tileAttrib);
    #endif
}

static void hitOnSideY (u16 sideDistY, u16 mapX, u16 posX, s16 rayDirAngleX)
{
    #if RENDER_SHOW_TEXCOORD

    // We only need the Texture X coordinate because we stay in the same vertical stripe of the screen.
    // Calculate the X coordinate where exactly the wall was hit (this is not the final Texture X coordinate)
    u16 wallTexX = posX + (mulu(sideDistY, rayDirAngleX) >> FS);
    // Substracting the integer value of the wall off it
    //wallTexX = ((wallTexX * 8) >> FS) & 7; // Faster? But is actually slower given the context
    wallTexX = max((wallTexX - mapX*FP) * 8 / FP, 0); // cleaner
    // Now let's calculate a color to represent the wall X coordinate (not the final Texture X coordinate)
    u16 d = (7 - min(7, sideDistY / FP));
    u16 tileAttrib;
    if (mapX&1) tileAttrib = (PAL1 << TILE_ATTR_PALETTE_SFT) + 1 + min(d, wallTexX)*8 + (8*8);
    else tileAttrib = (PAL1 << TILE_ATTR_PALETTE_SFT) + 1 + min(d, wallTexX)*8;
    u16 h2 = tab_wall_div[sideDistY]; // height halved

    #elif RENDER_USE_TAB_COLOR_D8_1_PALS_SHIFTED && !RENDER_SHOW_TEXCOORD

    // C version
    // u16 tileAttrib = tab_color_d8_1_Y_pals_shft[2*sideDistY + (mapX&1)]; // *2 because each element has one value for (mapX&1)=0 and other for (mapX&1)=1
    // u16 h2 = tab_wall_div[sideDistY]; // height halved

    // ASM version
    u16 tileAttrib;
    u16 h2;
    __asm volatile (
        "andi.w  #1,%[mapX]\n\t"                        // mapX &= 1
        "add.w   %[sideDistY],%[sideDistY]\n\t"         // sideDistY *= 2 (discern between element for (mapX&1)=0 and element for (mapX&1)=1)
        "move.w  (%[tab_wall_div],%[sideDistY].w),%[h2]\n\t" // h2 = tab_wall_div[sideDistY]
        "add.w   %[mapX],%[sideDistY]\n\t"              // index = 2*sideDistY + (mapX & 1)
        "add.w   %[sideDistY],%[sideDistY]\n\t"         // index *= 2 (word stride)
        "lea     %c[tab_color_mem],%[tab_wall_div]\n\t"  // this way we save one register
        "move.w  (%[tab_wall_div],%[sideDistY].w),%[tileAttrib]" // tileAttrib = tab_color_d8_1_Y_pals_shft[index]
        : [h2] "=d" (h2), [tileAttrib] "=d" (tileAttrib), [sideDistY] "+d" (sideDistY), [mapX] "+d" (mapX)
        : [tab_wall_div] "a" (tab_wall_div), [tab_color_mem] "s" (tab_color_d8_1_Y_pals_shft)
        :
    );

    #else

    u8 d8_1 = tab_color_d8_1[sideDistY]; // the bigger the distant the darker the color is
    u16 tileAttrib;
    if (mapX&1) tileAttrib = (PAL1 << TILE_ATTR_PALETTE_SFT) + d8_1 + (8*8); // use the tiles that point to second half of wall's palette
    else tileAttrib = (PAL1 << TILE_ATTR_PALETTE_SFT) + d8_1;
    u16 h2 = tab_wall_div[sideDistY]; // height halved

    #endif

    #if RENDER_HALVED_PLANES
    write_vline_halved(h2, tileAttrib);
    #else
    write_vline(h2, tileAttrib);
    #endif
}