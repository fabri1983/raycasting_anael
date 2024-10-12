#include <genesis.h>
#include "utils.h"
#include "consts.h"
#include "clear_buffer.h"
#include "map_matrix.h"
#include "frame_buffer.h"
#include "write_vline.h"

#include "hud.h"
#include "weapons.h"

#include "tab_dir_xy.h"
#include "tab_wall_div.h"

#if USE_TAB_COLOR_D8_PALS_SHIFTED && !SHOW_TEXCOORD
    #include "tab_color_d8_pals_shft.h"
#elif !USE_TAB_COLOR_D8_PALS_SHIFTED && !SHOW_TEXCOORD
    #include "tab_color_d8.h"
#endif

#if USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS
    #include "perf_hash_mulu_256_shft_FS.h"
    #include "tab_deltas_perf_hash.h"
#else
    #include "tab_deltas.h"
#endif

static void dda (u16 posX, u16 posY, u16 angle);
#if USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS
static FORCE_INLINE void process_column (u16* delta_a_ptr, u16 posX, u16 posY, u32 sideDistX_l0, u32 sideDistX_l1, u32 sideDistY_l0, u32 sideDistY_l1);
#else
static FORCE_INLINE void process_column (u16* delta_a_ptr, u16 posX, u16 posY, u16 sideDistX_l0, u16 sideDistX_l1, u16 sideDistY_l0, u16 sideDistY_l1);
#endif
static void do_stepping (u16 posX, u16 posY, u16 deltaDistX, u16 deltaDistY, u16 sideDistX, u16 sideDistY, s16 stepX, s16 stepY, s16 stepYMS, s16 rayDirAngleX, s16 rayDirAngleY);

extern void gameLoop () {

    // Frame load is calculated after user's VBlank callback
	//SYS_showFrameLoad(FALSE);

	// It seems positions in the map are multiple of FP +/- fraction. From (1*FP + MAP_FRACTION) to ((MAP_SIZE-1)*FP - MAP_FRACTION).
	// Smaller positions locate at top-left corner of the map[][] layout (as seen in the .h), bigger positions locate at bottom-right.
	// But dx and dy, applied to posX and posY respectively, goes from 0 to (+/-)FP/ANGLE_DIR_NORMALIZATION (used in tab_dir_xy.h).
	u16 posX = 2*FP, posY = 2*FP;

	// angle max value is 1023 and is updated in (1024/AP) units.
	// 0 points down in the map[], 256 points right, 512 points up, 768 points left.
	u16 angle = 0; 

	for (;;)
	{
		// clear the frame buffer
        #if USE_CLEAR_FRAMEBUFFER_WITH_SP
		clear_buffer_sp(frame_buffer);
        #else
        clear_buffer(frame_buffer);
        #endif

		u16 joy = JOY_readJoypad(JOY_1);

        if (joy & BUTTON_X) {
            weapon_next(-1);
        }
        else if (joy & BUTTON_Y) {
            weapon_next(1);
        }
        else if (joy & BUTTON_A) {
            weapon_fire();
        }

        weapon_update();

        hud_update();

        // movement and collisions
        if (joy & (BUTTON_UP | BUTTON_DOWN | BUTTON_B | BUTTON_LEFT | BUTTON_RIGHT)) {

            // Direction amount and sign depending on angle
            s16 dx=0, dy=0;

            // Strafe movement is perpendicular to facing direction
            if (joy & BUTTON_B) {
                // Strafe left
                if (joy & BUTTON_LEFT) {
                    dx = tab_dir_y_div24[angle];
                    dy = -tab_dir_x_div24[angle];
                }
                // Strafe Right
                else if (joy & BUTTON_RIGHT) {
                    dx = -tab_dir_y_div24[angle];
                    dy = tab_dir_x_div24[angle];
                }
            }

            // Simple forward/backward movement
            if (joy & BUTTON_UP) {
                dx += tab_dir_x_div24[angle];
                dy += tab_dir_y_div24[angle];
            }
            else if (joy & BUTTON_DOWN) {
                dx -= tab_dir_x_div24[angle];
                dy -= tab_dir_y_div24[angle];
            }

            // Current location (normalized) before displacement
            u16 x = posX / FP; // x > 0 always because min pos x is bigger than FP
            u16 y = posY / FP; // y > 0 always because min pos x is bigger than FP

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

            // Rotation (only when not strafing)
            if (!(joy & BUTTON_B)) {
                if (joy & BUTTON_LEFT)
                    angle = (angle + (1024/AP)) & 1023;
                else if (joy & BUTTON_RIGHT)
                    angle = (angle - (1024/AP)) & 1023;
            }
        }

		// DDA (Digital Differential Analyzer)
		dda(posX, posY, angle);

        SPR_update(); // must be called before the DMA enqueue of the framebuffer

        // DMA frame_buffer Plane A portion
        // Remaining 1/2 of the frame_buffer Plane A if first 1/2 was sent in hint_callback()
        //DMA_queueDmaFast(DMA_VRAM, frame_buffer + (VERTICAL_ROWS*PLANE_COLUMNS)/2, PA_ADDR + HALF_PLANE_ADDR_OFFSET, (VERTICAL_ROWS*PLANE_COLUMNS)/2 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
        // Remaining 3/4 of the frame_buffer Plane A if first 1/4 was sent in hint_callback()
        //DMA_queueDmaFast(DMA_VRAM, frame_buffer + (VERTICAL_ROWS*PLANE_COLUMNS)/4, PA_ADDR + QUARTER_PLANE_ADDR_OFFSET, ((VERTICAL_ROWS*PLANE_COLUMNS)/4)*3 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
        // All the frame_buffer Plane A
        DMA_queueDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, VERTICAL_ROWS*PLANE_COLUMNS - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
        
        // DMA frame_buffer Plane B portion
        DMA_queueDmaFast(DMA_VRAM, frame_buffer + VERTICAL_ROWS*PLANE_COLUMNS, PB_ADDR, VERTICAL_ROWS*PLANE_COLUMNS - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);

        SYS_doVBlankProcessEx(ON_VBLANK_START);

        // showFPS(0, 1);
        showCPULoad(0, 1);
	}

    SYS_hideFrameLoad(); // is independant from whether the load is displayed or not
}

extern void gameLoopAuto () {

    const u16 posStepping = 1;

    // NOTE: here we are moving from the most UPPER-LEFT position of the map[][] layout, 
    // stepping DOWN into Y Axis, and RIGHT into X Axis, where in each position we do a full rotation.
    // Therefore we only interesting in collisions with x+1 and y+1.

    for (u16 posX = MIN_POS_XY; posX <= MAX_POS_XY; posX += posStepping) {

        for (u16 posY = MIN_POS_XY; posY <= MIN_POS_XY; posY += posStepping) {

            // Current location (normalized)
            u16 x = posX / FP; // x > 0 always because min pos x is bigger than FP
            u16 y = posY / FP; // y > 0 always because min pos y is bigger than FP

            // Limit y axis location (normalized)
            const u16 ytop = (posY - (MAP_FRACTION-1)) / FP;
            const u16 ybottom = (posY + (MAP_FRACTION-1)) / FP;

            // Check x axis collision
            // Moving right as per map[][] layout?
            if (map[y][x+1] || map[ytop][x+1] || map[ybottom][x+1]) {
                if (posX > ((x+1)*FP - MAP_FRACTION)) {
                    // Move one block of map: (FP + 2*MAP_FRACTION) = 384 units. The block sizes FP, but we account for a safe distant to avoid clipping.
                    posX += (FP + 2*MAP_FRACTION) - posStepping;
                    break; // Stop current Y and continue with next X until it gets outside the collision
                }
            }
            // Moving left as per map[][] layout?
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
            // Moving down as per map[][] layout?
            if (map[y+1][x] || map[y+1][xleft] || map[y+1][xright]) {
                if (posY > ((y+1)*FP - MAP_FRACTION)) {
                    // Move one block of map: (FP + 2*MAP_FRACTION) = 384 units. The block sizes FP, but we account for a safe distant to avoid clipping.
                    posY += (FP + 2*MAP_FRACTION) - posStepping;
                    continue; // Continue with next Y until it gets outside the collision
                }
            }
            // Moving up as per map[][] layout?
            // else if (map[y-1][x] || map[y-1][xleft] || map[y-1][xright]) {
            //     if (posY < (y*FP + MAP_FRACTION)) {
            //         // Move one block of map: (FP + 2*MAP_FRACTION) = 384 units. The block sizes FP, but we account for a safe distant to avoid clipping.
            //         posY -= (FP + 2*MAP_FRACTION) + posStepping;
            //         continue; // Continue with next Y until it gets outside the collision
            //     }
            // }

            for (u16 angle = 0; angle < 1024; angle += (1024/AP)) {

                // clear the frame buffer
                #if USE_CLEAR_FRAMEBUFFER_WITH_SP
                clear_buffer_sp(frame_buffer);
                #else
                clear_buffer(frame_buffer);
                #endif
                
                // DDA (Digital Differential Analyzer)
                dda(posX, posY, angle);

                // Enqueue the frame buffer for DMA during VBlank period
                // Remaining 1/2 of PA
                //DMA_queueDmaFast(DMA_VRAM, frame_buffer + (VERTICAL_ROWS*PLANE_COLUMNS)/2, PA_ADDR + HALF_PLANE_ADDR_OFFSET, (VERTICAL_ROWS*PLANE_COLUMNS)/2 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
                // Remaining 3/4 of PA
                //DMA_queueDmaFast(DMA_VRAM, frame_buffer + (VERTICAL_ROWS*PLANE_COLUMNS)/4, PA_ADDR + QUARTER_PLANE_ADDR_OFFSET, ((VERTICAL_ROWS*PLANE_COLUMNS)/4)*3 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
                DMA_queueDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, VERTICAL_ROWS*PLANE_COLUMNS - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
                DMA_queueDmaFast(DMA_VRAM, frame_buffer + VERTICAL_ROWS*PLANE_COLUMNS, PB_ADDR, VERTICAL_ROWS*PLANE_COLUMNS - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);

                SYS_doVBlankProcessEx(ON_VBLANK_START);

                // handle inputs
                // u16 joy = JOY_readJoypad(JOY_1);
                // if (joy & BUTTON_START)
                //     break;
            }
        }
    }
}

/**
 * Digital Differential Analyzer
 */
static void dda (u16 posX, u16 posY, u16 angle) {

    #if USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS
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

    u16 a = angle / (1024/AP); // a range is [0, 128)
    u16* delta_a_ptr = (u16*) (tab_deltas + (a * PIXEL_COLUMNS * DELTA_PTR_OFFSET_AMNT));

    // reset to the start of frame_buffer
    column_ptr = frame_buffer;

    // 256p or 320p width, but 4 "pixels" wide column => effectively 256/4=64 or 320/4=80 pixels width.
    for (u8 column = 0; column < PIXEL_COLUMNS; column += COLUMNS_UNROLL) {
        #if COLUMNS_UNROLL == 1
        process_column(delta_a_ptr, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
        if ((column & 1) == 0)
            column_ptr += VERTICAL_ROWS*PLANE_COLUMNS;
        else
            column_ptr += -VERTICAL_ROWS*PLANE_COLUMNS + 1;
        #elif COLUMNS_UNROLL == 2
        process_column(delta_a_ptr + 0*DELTA_PTR_OFFSET_AMNT, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
        column_ptr += VERTICAL_ROWS*PLANE_COLUMNS;
        process_column(delta_a_ptr + 1*DELTA_PTR_OFFSET_AMNT, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
        column_ptr += -VERTICAL_ROWS*PLANE_COLUMNS + 1;
        #elif COLUMNS_UNROLL == 4
        process_column(delta_a_ptr + 0*DELTA_PTR_OFFSET_AMNT, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
        column_ptr += VERTICAL_ROWS*PLANE_COLUMNS;
        process_column(delta_a_ptr + 1*DELTA_PTR_OFFSET_AMNT, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
        column_ptr += -VERTICAL_ROWS*PLANE_COLUMNS + 1;
        process_column(delta_a_ptr + 2*DELTA_PTR_OFFSET_AMNT, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
        column_ptr += VERTICAL_ROWS*PLANE_COLUMNS;
        process_column(delta_a_ptr + 3*DELTA_PTR_OFFSET_AMNT, posX, posY, sideDistX_l0, sideDistX_l1, sideDistY_l0, sideDistY_l1);
        column_ptr += -VERTICAL_ROWS*PLANE_COLUMNS + 1;
        #endif
        delta_a_ptr += COLUMNS_UNROLL * DELTA_PTR_OFFSET_AMNT;
    }
}

#if USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS
static FORCE_INLINE void process_column (u16* delta_a_ptr, u16 posX, u16 posY, u32 sideDistX_l0, u32 sideDistX_l1, u32 sideDistY_l0, u32 sideDistY_l1)
#else
static FORCE_INLINE void process_column (u16* delta_a_ptr, u16 posX, u16 posY, u16 sideDistX_l0, u16 sideDistX_l1, u16 sideDistY_l0, u16 sideDistY_l1)
#endif
{
	const u16 deltaDistX = *(delta_a_ptr + 0); // value from 182 up to 65535, but only 915 different values
	const u16 deltaDistY = *(delta_a_ptr + 1); // value from 182 up to 65535, but only 915 different values
	const s16 rayDirAngleX = (s16) *(delta_a_ptr + 2); // value from 0 up to 65535, but only 717 signed different values in [-360, 360] (due to precision lack)
	const s16 rayDirAngleY = (s16) *(delta_a_ptr + 3); // value from 0 up to 65535, but only 717 signed different values in [-360, 360] (due to precision lack)
    #if USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS
    const u16 deltaDistX_perf_hash = *(delta_a_ptr + 4); // 0..MPH_VALUES_DELTADIST_NKEYS-1 multiplied by 2 for faster ASM access
	const u16 deltaDistY_perf_hash = *(delta_a_ptr + 5); // 0..MPH_VALUES_DELTADIST_NKEYS-1 multiplied by 2 for faster ASM access
    #endif

// u16 expectedX, expectedY;

	u16 sideDistX, sideDistY; // effective value goes up to 4096-1 once in the stepping loop due to condition control
	s16 stepX, stepY, stepYMS;

	if (rayDirAngleX < 0) {
		stepX = -1;
        #if USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS
        // expectedX = mulu_shft_FS(sideDistX_l0/(MPH_VALUES_DELTADIST_NKEYS * ASM_PERF_HASH_JUMP_BLOCK_SIZE_BYTES), deltaDistX);
        sideDistX = perf_hash_mulu_shft_FS(sideDistX_l0, deltaDistX_perf_hash);
        #else
        sideDistX = mulu_shft_FS(sideDistX_l0, deltaDistX); //(u16)(mulu(sideDistX_l0, deltaDistX) >> FS);
        #endif
	}
	else {
		stepX = 1;
        #if USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS
        // expectedX = mulu_shft_FS(sideDistX_l1/(MPH_VALUES_DELTADIST_NKEYS * ASM_PERF_HASH_JUMP_BLOCK_SIZE_BYTES), deltaDistX);
        sideDistX = perf_hash_mulu_shft_FS(sideDistX_l1, deltaDistX_perf_hash);
        #else
        sideDistX = mulu_shft_FS(sideDistX_l1, deltaDistX); //(u16)(mulu(sideDistX_l1, deltaDistX) >> FS);
        #endif
	}

	if (rayDirAngleY < 0) {
		stepY = -1;
		stepYMS = -MAP_SIZE;
		#if USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS
        // expectedY = mulu_shft_FS(sideDistY_l0/(MPH_VALUES_DELTADIST_NKEYS * ASM_PERF_HASH_JUMP_BLOCK_SIZE_BYTES), deltaDistY);
        sideDistY = perf_hash_mulu_shft_FS(sideDistY_l0, deltaDistY_perf_hash);
        #else
        sideDistY = mulu_shft_FS(sideDistY_l0, deltaDistY); //(u16)(mulu(sideDistY_l0, deltaDistY) >> FS);
        #endif
	}
	else {
		stepY = 1;
		stepYMS = MAP_SIZE;
		#if USE_PERF_HASH_TAB_MULU_DIST_256_SHFT_FS
        // expectedY = mulu_shft_FS(sideDistY_l1/(MPH_VALUES_DELTADIST_NKEYS * ASM_PERF_HASH_JUMP_BLOCK_SIZE_BYTES), deltaDistY);
        sideDistY = perf_hash_mulu_shft_FS(sideDistY_l1, deltaDistY_perf_hash);
        #else
        sideDistY = mulu_shft_FS(sideDistY_l1, deltaDistY); //(u16)(mulu(sideDistY_l1, deltaDistY) >> FS);
        #endif
	}

// if (expectedX != sideDistX || expectedY != sideDistY) {
//     KLog_U4("", expectedX, " ", sideDistX, "   ", expectedY, " ", sideDistY);
// }

    do_stepping(posX, posY, deltaDistX, deltaDistY, sideDistX, sideDistY, stepX, stepY, stepYMS, rayDirAngleX, rayDirAngleY);
}

static FORCE_INLINE void do_stepping (u16 posX, u16 posY, u16 deltaDistX, u16 deltaDistY, u16 sideDistX, u16 sideDistY, s16 stepX, s16 stepY, s16 stepYMS, s16 rayDirAngleX, s16 rayDirAngleY)
{
    u16 mapX = posX / FP;
	u16 mapY = posY / FP;
	const u8 *map_ptr = &map[mapY][mapX];

	for (u16 n = 0; n < STEP_COUNT_LOOP; ++n) {

		// side X
		if (sideDistX < sideDistY) {

			mapX += stepX;
			map_ptr += stepX;

			u16 hit = *map_ptr; // map[mapY][mapX];
			if (hit) {

				u16 h2 = tab_wall_div[sideDistX];

				#if SHOW_TEXCOORD
				u16 d = (7 - min(7, sideDistX / FP));
				u16 wallY = posY + (muls(sideDistX, rayDirAngleY) >> FS);
				//wallY = ((wallY * 8) >> FS) & 7; // faster? But is actually slower given the context
				wallY = max((wallY - mapY*FP) * 8 / FP, 0); // cleaner
				u16 color = ((0 + 2*(mapY&1)) << TILE_ATTR_PALETTE_SFT) + 1 + min(d, wallY)*8;
                #elif USE_TAB_COLOR_D8_PALS_SHIFTED
                u16 color = tab_color_d8_X_pals_shft[sideDistX + sideDistX + (mapY&1)];
				#else
				u16 d8 = tab_color_d8[sideDistX]; // the bigger the distant the darker the color is
				u16 color;
				if (mapY&1) color = d8 | (PAL2 << TILE_ATTR_PALETTE_SFT);
				else color = d8 | (PAL0 << TILE_ATTR_PALETTE_SFT);
				#endif

				write_vline(h2, color);
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

				u16 h2 = tab_wall_div[sideDistY];

				#if SHOW_TEXCOORD
				u16 d = (7 - min(7, sideDistY / FP));
				u16 wallX = posX + (muls(sideDistY, rayDirAngleX) >> FS);
				//wallX = ((wallX * 8) >> FS) & 7; // faster? But is actually slower given the context
				wallX = max((wallX - mapX*FP) * 8 / FP, 0); // cleaner
				u16 color = ((1 + 2*(mapX&1)) << TILE_ATTR_PALETTE_SFT) + 1 + min(d, wallX)*8;
				#elif USE_TAB_COLOR_D8_PALS_SHIFTED
				u16 color = tab_color_d8_Y_pals_shft[sideDistY + sideDistY + (mapX&1)];
                #else
				u16 d8 = tab_color_d8[sideDistY]; // the bigger the distant the darker the color is
				u16 color;
				if (mapX&1) color = d8 | (PAL3 << TILE_ATTR_PALETTE_SFT);
				else color = d8 |= (PAL1 << TILE_ATTR_PALETTE_SFT);
				#endif

				write_vline(h2, color);
				break;
			}

			sideDistY += deltaDistY;
		}
	}
}