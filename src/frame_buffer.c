#include "frame_buffer.h"

u16 frame_buffer[VERTICAL_ROWS*PLANE_COLUMNS*2 - (PLANE_COLUMNS-TILEMAP_COLUMNS)];

u16 frame_buffer_pxcolumn[PIXEL_COLUMNS];

void loadPlaneDisplacements () {
	for (u16 column = 0; column < PIXEL_COLUMNS; column++) {
		// column is even => base offset is 0
		// column is odd => base offset is VERTICAL_ROWS*PLANE_COLUMNS which is after the first plane
		frame_buffer_pxcolumn[column] = column&1 ? VERTICAL_ROWS*PLANE_COLUMNS + column/2 : 0 + column/2;
	}
}
