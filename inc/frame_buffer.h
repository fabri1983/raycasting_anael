#ifndef _FRAME_BUFFER_H_
#define _FRAME_BUFFER_H_

#include <types.h>
#include "consts.h"

// 224 px display height, but only VERTICAL_ROWS height for the frame buffer (tilemap).
// PLANE_COLUMNS is the width of the tilemap on screen.
// Multiplied by 2 because is shared between the 2 planes BG_A and BG_B.
// Removed last (PLANE_COLUMNS-TILEMAP_COLUMNS) non displayed columns (if case applies).
extern u16 frame_buffer[VERTICAL_ROWS*PLANE_COLUMNS*2 - (PLANE_COLUMNS-TILEMAP_COLUMNS)];

void clear_buffer ();
void clear_buffer_sp ();
void clear_buffer_halved ();
void clear_buffer_halved_sp ();

// Points to the start of the column in first display row for each cycle of the for-loop of columns
extern u16* column_ptr;

void write_vline (u16 h2, u16 tileAttrib);
void write_vline_halved (u16 h2, u16 tileAttrib);

void fb_set_top_entries_column (u8 pixel_column);
void fb_increment_entries_column ();

/// @brief Mirrors in inverted fashion the bottom half region of frame_buffer's Plane A and B into their respective top half region.
/// Uses CPU and RAM. Then set the correct top tilemap entries so they are not inverted.
void fb_mirror_planes_in_RAM ();
/// @brief Mirrors in inverted fashion the bottom half region of frame_buffer's Plane A and B into their respective top half region.
/// Uses VDP's VRAM to VRAM copy operation. Then set the correct top tilemap entries so they are not inverted.
void fb_mirror_planes_in_VRAM ();

#endif // _FRAME_BUFFER_H_