#ifndef _SPRITE_ENGINE_OVERRIDE_H_
#define _SPRITE_ENGINE_OVERRIDE_H_

#include <types.h>

#define SPR_ENG_ALLOW_MULTI_PALS FALSE // Remember to update your res files accordingly

/**
 *  \brief
 *      Single VDP sprite info structure for sprite animation frame. Modified from SGDK's sprite_eng.h to add paletteId.
 *
 *  \param offsetY
 *      Y offset for this VDP sprite relative to global Sprite position
 *  \param offsetYFlip
 *      Y offset (flip version) for this VDP sprite relative to global Sprite position
 *  \param size
 *      sprite size (see SPRITE_SIZE macro)
 *  \param offsetX
 *      X offset for this VDP sprite relative to global Sprite position
 *  \param offsetXFlip
 *      X offset (flip version) for this VDP sprite relative to global Sprite position
 *  \param numTile
 *      number of tile for this VDP sprite (should be coherent with the given size field)
 *  \param paletteId
 *      which palette id this VDP Sprite point to: 0, 1, 2, or 3. THIS COMES ALREADY SHIFTED 13 PLACES.
 */
typedef struct
{
    u8 offsetY;          // respect VDP sprite field order, may help
    u8 offsetYFlip;
    u8 size;
    u8 offsetX;
    u8 offsetXFlip;
    u8 numTile;
    u16 paletteId;
} FrameVDPSpriteWithPal;

void spr_eng_update ();

#endif // _SPRITE_ENGINE_OVERRIDE_H_