#include "map_hit_compressed.h"
#include "consts.h"
#include "utils.h"
#include <maths.h>

#define MAP_HIT_COMPRESSED_BLOCK_SIZE (PIXEL_COLUMNS*4)

const u16 map_hit_compressed[] = {
0
};

const u32 map_hit_lookup[] = {
0
};

#if USE_MAP_HIT_COMPRESSED
static u32 row;
static u32 index;
// static u32 blockIndex;
// static u16 elemIndex;
#endif

void map_hit_reset_vars ()
{
#if USE_MAP_HIT_COMPRESSED
    // Due to optimizations made in the code to avoid repetitive calculations we need to initialize some vars
    // elemIndex = MAP_HIT_COMPRESSED_BLOCK_SIZE;
#endif
}

FORCE_INLINE void map_hit_setRow(u16 posX, u16 posY, u16 a)
{
#if USE_MAP_HIT_COMPRESSED
    u16 mapX = posX / FP;
    u16 mapY = posY / FP;
    row = (((mapX * MAP_SIZE + mapY) * (1024/(1024/AP)) + a) * PIXEL_COLUMNS) - MAP_HIT_MIN_CALCULATED_INDEX;
    // The compressed array has a row length of MAP_HIT_COMPRESSED_BLOCK_SIZE = PIXEL_COLUMNS * k.
    // So we just divide by that length to get the row into the lookup index.
    // u32 row = (((mapX * MAP_SIZE + mapY) * (1024/(1024/AP)) + a) * PIXEL_COLUMNS) - MAP_HIT_MIN_CALCULATED_INDEX;
    // blockIndex = divu(row, MAP_HIT_COMPRESSED_BLOCK_SIZE);
#endif
}

FORCE_INLINE void map_hit_setIndexForStartingColumn (u16 column)
{
#if USE_MAP_HIT_COMPRESSED
    index = row + column;
    // The compressed array has a row length of MAP_HIT_COMPRESSED_BLOCK_SIZE = PIXEL_COLUMNS * k.
    // So once the column exceed that length we need to wrap up. The correct thing should be % MAP_HIT_COMPRESSED_BLOCK_SIZE
    // but we know before hand that parameter "column" is smaller than MAP_HIT_COMPRESSED_BLOCK_SIZE.
    // if (elemIndex >= MAP_HIT_COMPRESSED_BLOCK_SIZE)
    //     elemIndex = column;
#endif
}

FORCE_INLINE void map_hit_incrementColumn ()
{
#if USE_MAP_HIT_COMPRESSED
    ++index;
    // The compressed array has a row length of MAP_HIT_COMPRESSED_BLOCK_SIZE = PIXEL_COLUMNS * k.
    // And since we are incrementing columns by 1 then we can just leave the wrap up of elemIndex to map_hit_setIndexForStartingColumn().
    // ++elemIndex;
#endif
}

u16 map_hit_decompressAt ()
{
#if USE_MAP_HIT_COMPRESSED
    u32 blockIndex = divu(index, MAP_HIT_COMPRESSED_BLOCK_SIZE);
    u16 elemIndex = modu(index, MAP_HIT_COMPRESSED_BLOCK_SIZE);

    u32 blockStart = map_hit_lookup[blockIndex];
    u16 base = map_hit_compressed[blockStart];
    u16 bits = map_hit_compressed[blockStart + 1];

    u16 totalBits = elemIndex * bits;
    u32 startWord = (totalBits / 16) + blockStart + 2;
    u16 bitOffset = totalBits % 16;

    u32 value = map_hit_compressed[startWord] >> bitOffset;
    if ((bitOffset + bits) > 16) {
        value |= (u32)map_hit_compressed[startWord + 1] << (16 - bitOffset);
    }
    value &= (1U << bits) - 1;

    return base + (u16)value;
#else
    return 0;
#endif
}