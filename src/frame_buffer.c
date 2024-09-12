#include "frame_buffer.h"

u16 frame_buffer[VERTICAL_COLUMNS*PLANE_COLUMNS*2];

u16 frame_buffer_column[PLANE_COLUMNS*2];

void loadPlaneDisplacements () {
	for (u16 i = 0; i < PLANE_COLUMNS*2; i++) {
		frame_buffer_column[i] = i&1 ? VERTICAL_COLUMNS*PLANE_COLUMNS + i/2 : i/2;
	}
}
