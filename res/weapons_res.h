#include <genesis.h>

#ifndef _RES_WEAPONS_RES_H_
#define _RES_WEAPONS_RES_H_

typedef struct {
    u16* data;
} Palette16;

extern const SpriteDefinition sprDef_weapon_fist_anim;
extern const Palette16 pal_weapon_fist_anim;
extern const SpriteDefinition sprDef_weapon_pistol_anim;
extern const Palette16 pal_weapon_pistol_anim;
extern const SpriteDefinition sprDef_weapon_shotgun_anim;
extern const Palette16 pal_weapon_shotgun_anim;

#endif // _RES_WEAPONS_RES_H_
