## Raycasting in SGDK (by Anael Seghezzi)

Raycasting original demo by **Anael Seghezzi**: [SGDK_raycasting](https://github.com/Stephane-D/SGDK)

This version implements the same ray casting but with a screen size of **320x192** pixels at 60Hz.

<video autoplay loop muted playsinline>
  <source src="media/raycasting_v0.1.webm" type="video/webm">
</video>
[![Demo Video](media/raycasting_v0.1.webm)](media/raycasting_v0.1.webm)

### fabri1983's notes (since Aug/18/2024)
-------------------------------------
- Ray Casting: https://lodev.org/cgtutor/raycasting.html
- DDA = Digital Differential Analysis.
- Modified to display full screen in **320x192**, +32 pixels height for the HUD.
- Only 6 button pad supported at the moment.
- Rendered columns are 4 pixels wide, thus effectively 256p/4 = 64 or 320p/4 = 80 "pixels" columns.
- Using `RAM_FIXED_xxx` regions to store arrays. This way the setup for DMA operations can be prepared in fewer instructions.
  You must only use `MEM_pack()` before any resource is setup/reset that uses `RAM_FIXED_xxx` regions, and also ensure SGDK's inner 
  `MEM_xxx()` functions do too.
- Moved `clear_buffer()` into inline ASM to take advantage of compiler optimizations => `%1 saved in cpu usage`.
- `clear_buffer()` using SP (Stack Pointer) to point end of buffer => `60 cycles saved` if *PLANE_COLUMNS=32*, and 
  `150 cycles saved` if *PLANE_COLUMNS=64*.
- If `clear_buffer()` is moved into VBlank callback => `%1 saved in cpu usage`, but runs into next display period.
- Replaced   `if ((joy & BUTTON_LEFT) || (joy & BUTTON_RIGHT))`
  by         `if (joy & (BUTTON_LEFT | BUTTON_RIGHT))`
  Same with `BUTTON_UP` and `BUTTON_DOWN`.
- Moved `write_vline_full()` into `write_vline()` and translated into inline ASM => `~1% saved in cpu usage`.
- `write_vline()`: translated into inline ASM => `2% saved in cpu usage`.
- `write_vline()`: optimized accesses for top and bottom tiles of current column made in ASM => `4% saved in cpu usage`.
- `write_vline()`: exposing as global the column pointer instead of sending it as an argument. This made the 
  `frame_buffer_pxcolumn[]` useless, so no need for `render_loadPlaneDisplacements()` neither offset before 
  `write_vline()` => `~1% saved in cpu usage` depending on the inline ASM constraints.
- Replaced `dirX` and `dirY` calculation in relation to the angle by two tables defined in `tab_dir_xy.h` => `some cycles saved`.
- Replaced `DMA_doDmaFast()` by `DMA_queueDmaFast()` => `1% saved in cpu usage` but consumes all VBlank period and runs 
  into next display period.
- Changes in `tab_wall_div.h` so the start of the vertical line to be written is calculated ahead of time => `1% saved in cpu usage`.
- Replaced variable `d` used for color calculation by two tables defined in `tab_color_d8_1.h` => `2% saved in cpu usage`.
  There is also a replacement for `tab_color_d8_1[]` by using tables in `tab_color_d8_pals_shft.h` in asm which is slightly faster.
- Replaced `mulu(sideDistX_l0, deltaDistX) >> FS` by a table => `~4% saved in cpu usage`, though rom size increased 
  460 KB (+ padding to 128KB boundary).
- DMA framebuffer row by row using ASM -> `3% saved in cpu usage` (thanks **Shannon Birt**).
- Render bottom half of framebuffer and then mirror it into top region using multi scanline HInts.
  This reduce DMA presure during VBlank up to a 50%, but adds more CPU pressure during top half active display period.
- SGDK's `SPR_update()` function was copied and modified to handle DMA for specific cases, and also removed unused features.
  See comments with tag fabri1983 at `spr_eng_override.c` => `~1% saved in cpu usage`.
- `sega.s`: use `_VINT_lean` instead of SGDK's `_VINT` to only increment vtimer and call user's vint callback.
- SGDK's `SYS_doVBlankProcessEx()` function was modified to remove unwanted logic. See render.c => `~1% saved in cpu usage`.
- SGDK's `JOY_update()` and `JOY_readJoypad(JOY_1)` functions were copied and modified to handle only 6 button joypad and used 
  at `render_SYS_doVBlankProcessEx_ON_VBLANK()` => => `2% saved in cpu usage`.
- Commented out the `#pragma` directives for loop unrolling => `~1% saved in cpu usage`. It may vary according the 
  use/abuse of *FORCE_INLINE*.
- Manual unrolling of 2 (or 4) iterations for column processing => `2% saved in cpu usage`. It may vary according
  the use/abuse of *FORCE_INLINE*.

### fabri1983's resources notes:
----------------------------
- `RAM_FIXED_VDP_SPRITE_CACHE_ADDRESS` set in `consts_ext.h`: check this constant everytime you get a black screen when running the game.
- HUD: the hud is treated as an image. The resource definition in `hud_res.res` file expects a map base attribute value which is 
  calculated before hand. See `HUD_BASE_TILE_ATTRIB` in `hud.h`.
- HUD: if resource is compressed then also set const `HUD_TILEMAP_COMPRESSED` in `hud.h`.
- WEAPONS: the location of sprite tiles is given by methods like `weapon_getVRAMLocation()`. Required for `SPR_addSpriteEx()`.
- Additional free VRAM: Planes A and B bottom region covered by the HUD, which is in Window Plane, leaves unused VRAM.
  Window Plane leaves unused VRAM from where it begins down to the address where the HUD is displayed.
  See `PB_FREE_VRAM_AT`, `PW_FREE_VRAM_AT`, `PA_FREE_VRAM_AT`, and `LAST_FREE_VRAM_AT` at `consts_ext.h`.

![additional_free_vram.png](media/additional_free_vram.png?raw=true "additional_free_vram.png")
