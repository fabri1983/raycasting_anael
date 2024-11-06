#include "frame_buffer.h"

u16 frame_buffer[VERTICAL_ROWS*PLANE_COLUMNS*2 - (PLANE_COLUMNS-TILEMAP_COLUMNS)];

u16 frame_buffer_pxcolumn[PIXEL_COLUMNS];
