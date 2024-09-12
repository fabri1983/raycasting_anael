#ifndef _FRAME_BUFFER_H_
#define _FRAME_BUFFER_H_

#include <types.h>
#include "consts.h"

// 224 px display height, but only VERTICAL_COLUMNS height for the frame buffer (tilemap).
// PLANE_COLUMNS is the width of the tilemap on screen.
// Multiplied by 2 because is shared between the 2 planes BG_A and BG_B
extern u16 frame_buffer[VERTICAL_COLUMNS*PLANE_COLUMNS*2];

// Holds the offset to access the right column for a given plane in the framebuffer.
extern u16 frame_buffer_column[PLANE_COLUMNS*2];

// Calculate the offset to access the column for both planes defined in the framebuffer.
void loadPlaneDisplacements ();

#endif // _FRAME_BUFFER_H_