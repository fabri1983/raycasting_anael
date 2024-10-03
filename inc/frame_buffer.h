#ifndef _FRAME_BUFFER_H_
#define _FRAME_BUFFER_H_

#include <types.h>
#include "consts.h"

// 224 px display height, but only VERTICAL_ROWS height for the frame buffer (tilemap).
// PLANE_COLUMNS is the width of the tilemap on screen.
// Multiplied by 2 because is shared between the 2 planes BG_A and BG_B.
// Removed last (PLANE_COLUMNS-TILEMAP_COLUMNS) non displayed columns (if case applies).
extern u16 frame_buffer[VERTICAL_ROWS*PLANE_COLUMNS*2 - (PLANE_COLUMNS-TILEMAP_COLUMNS)];

// Holds the offset to access the column for a given plane in the framebuffer given 
// the pixel column currently being rendered.
extern u16 frame_buffer_pxcolumn[PIXEL_COLUMNS];

// Calculate the offset to access the column for both planes defined in the framebuffer.
void loadPlaneDisplacements ();

#endif // _FRAME_BUFFER_H_