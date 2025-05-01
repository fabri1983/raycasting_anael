#ifndef _SPRITE_ENGINE_OVERRIDE_H_
#define _SPRITE_ENGINE_OVERRIDE_H_

#include <types.h>
#include <sprite_eng.h>

#define SPR_ENG_ALLOW_MULTI_PALS FALSE // Remember to update your res files accordingly

/**
 *  \brief
 *      Adds a new sprite with specified parameters and returns it.
 *
 *  \param spriteDef
 *      the SpriteDefinition data to assign to this sprite.
 *  \param x
 *      default X position.
 *  \param y
 *      default Y position.
 *  \param attribut
 *      sprite attribut (see TILE_ATTR() macro).
 *  \param flag
 *      specific settings for this sprite:<br>
 *      #SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE = Disable delaying of frame update when we are running out of DMA capacity.<br>
 *          If you set this flag then sprite frame update always happen immediately but may lead to some graphical glitches (tiles data and sprite table data not synchronized).
 *          You can use SPR_setDelayedFrameUpdate(..) method to change this setting.<br>
 *      #SPR_FLAG_AUTO_VISIBILITY = NOT USED. Visibility defaults to 1. Use SPR_setVisibility(..) method).<br>
 *      #SPR_FLAG_FAST_AUTO_VISIBILITY = Enable fast computation for the automatic visibility calculation (disabled by default)<br>
 *          If you set this flag the automatic visibility calculation will be done globally for the (meta) sprite and not per internal
 *          hardware sprite. This result in faster visibility computation at the expense of some waste of hardware sprite.
 *          You can set the automatic visibility computation by using SPR_setVisibility(..) method.<br>
 *      #SPR_FLAG_AUTO_VRAM_ALLOC = NOT USED. It defaults to manual VRAM location set with the <i>attribut</i> parameter or by using the #SPR_setVRAMTileIndex(..) method<br>
 *      #SPR_FLAG_AUTO_TILE_UPLOAD = Enable automatic upload of sprite tiles data into VRAM (enabled by default)<br>
 *          If you don't set this flag you will have to manually upload tiles data of sprite into the VRAM (you can change this setting using #SPR_setAutoTileUpload(..) method).<br>
 *      #SPR_FLAG_INSERT_HEAD = Allow to insert the sprite at the start/head of the list.<br>
 *          When you use this flag the sprite will be inserted at the head of the list making it top most (equivalent to #SPR_setDepth(#SPR_MIN_DEPTH))<br>
 *          while default insertion position is at the end of the list (equivalent to #SPR_setDepth(#SPR_MAX_DEPTH))<br>
 *      <br>
 *  \return the new sprite or <i>NULL</i> if the operation failed (some logs can be generated in the KMod console in this case)
 *
 *      By default the sprite uses the provided flag setting for automatic resources allocation and sprite visibility computation.<br>
 *      If auto visibility is not enabled then sprite is considered as not visible by default (see SPR_setVisibility(..) method).<br>
 *      You can release all sprite resources by using SPR_releaseSprite(..) or SPR_reset(..).<br>
 *      IMPORTANT NOTE: sprite allocation can fail (return NULL) when you are using auto VRAM allocation (SPR_FLAG_AUTO_VRAM_ALLOC) even if there is enough VRAM available,<br>
 *      this can happen because of the VRAM fragmentation. You can use #SPR_addSpriteExSafe(..) method instead so it take care about VRAM fragmentation.
 *
 *  \see SPR_addSprite(..)
 *  \see SPR_addSpriteExSafe(..)
 *  \see SPR_releaseSprite(..)
 */
Sprite* spr_eng_addSpriteEx (const SpriteDefinition* spriteDef, s16 x, s16 y, u16 attribut, u16 flag);

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