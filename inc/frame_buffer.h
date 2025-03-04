#ifndef _FRAME_BUFFER_H_
#define _FRAME_BUFFER_H_

#include <types.h>

// 224 px display height, but only VERTICAL_ROWS height for the frame buffer (tilemap).
// TILEMAP_COLUMNS is the width of the tilemap on screen.
// Multiplied by 2 because is shared between the 2 planes BG_A and BG_B.
extern u16* frame_buffer;

void fb_allocate_frame_buffer ();
void fb_free_frame_buffer ();

void clear_buffer_no_usp ();
void clear_buffer ();
void clear_buffer_sp ();

void clear_buffer_halved_no_usp ();
void clear_buffer_halved ();
void clear_buffer_halved_sp ();

// Points to the first row of the column in each cycle of the for-loop of columns.
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
/// @brief Set the top entries to VRAM locations one by one. This must be called once the other half planes were effectively DMAed into VRAM.
void fb_copy_top_entries_in_VRAM ();

#endif // _FRAME_BUFFER_H_