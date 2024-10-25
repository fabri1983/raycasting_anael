#include "weapons.h"
#include <vdp_tile.h>
#include <sprite_eng.h>
#include <maths.h>
#include "weapons_res.h"
#include "hint_callback.h"

static u8 resetToIdle_timer;
static u8 fire_coolDown_timer;
static u8 select_coolDown_timer;
static u8 changeWeaponEffect_timer;
static s8 changeWeaponEffect_direction;

static Sprite* spr_currWeapon;
static u8 currWeaponId;
static u8 currWeaponAnimFireCooldownTimer;
static u8 currWeaponAnimReadyToHitAgainFrame;
static u16 ammoInventory[WEAPON_MAX_COUNT] = {0};

u16 weapon_biggerSummationAnimTiles ()
{
    return 107;
}

u16 weapon_getVRAMLocation ()
{
    // TILE_FONT_INDEX is the VRAM index location where the SGDK's Sprite Engine calculates the dedicated VRAM going backwards
    return TILE_FONT_INDEX - weapon_biggerSummationAnimTiles();
}

void weapon_resetState ()
{
    // Sprites use tile attributes, but no tile index when using SPR_FLAG_AUTO_VRAM_ALLOC.
    // But here we need to set a fixed VRAM index location so using TILE_ATTR_FULL.
    u16 baseTileAttribs = TILE_ATTR_FULL(WEAPON_BASE_PAL, 0, FALSE, FALSE, weapon_getVRAMLocation());

    // Loads fist sprite so we can avoid the check for NULL in other methods
    spr_currWeapon = SPR_addSpriteEx(&sprDef_weapon_fist_anim, 0, 0, baseTileAttribs, SPR_FLAG_AUTO_TILE_UPLOAD | SPR_FLAG_DISABLE_ANIMATION_LOOP);
    SPR_setVisibility(spr_currWeapon, HIDDEN);
    SPR_setAutoAnimation(spr_currWeapon, FALSE);
    PAL_setColors(WEAPON_BASE_PAL*16 + 8, pal_weapon_fist_anim.data + 8, 8, DMA);
    //PAL_setColors((WEAPON_BASE_PAL+1)*16 + 8, sprDef_weapon_fist_anim.palette->data + 16 + 8, 8, DMA);

    currWeaponId = 0;
    currWeaponAnimFireCooldownTimer = 0;
    currWeaponAnimReadyToHitAgainFrame = 0;
    resetToIdle_timer = 0;
    fire_coolDown_timer = 0;
    select_coolDown_timer = 0;
    changeWeaponEffect_timer = 0;
}

static FORCE_INLINE void weapon_load (const SpriteDefinition* sprDef, u16* pal, s16 x, s16 y)
{
    resetToIdle_timer = 0;
    fire_coolDown_timer = 0;
    SPR_releaseSprite(spr_currWeapon);

    // Sprites use tile attributes, but no tile index when using SPR_FLAG_AUTO_VRAM_ALLOC.
    // But here we need to set a fixed VRAM index location so using TILE_ATTR_FULL.
    u16 baseTileAttribs = TILE_ATTR_FULL(WEAPON_BASE_PAL, 0, FALSE, FALSE, weapon_getVRAMLocation());

    spr_currWeapon = SPR_addSpriteEx(sprDef, x, y, baseTileAttribs, SPR_FLAG_AUTO_TILE_UPLOAD | SPR_FLAG_DISABLE_ANIMATION_LOOP);
    SPR_setAutoAnimation(spr_currWeapon, FALSE);

    hint_enqueueWeaponPal(pal);
}

void weapon_select (u8 weaponId)
{
    // start weapon change effect for the current weapon
    changeWeaponEffect_direction = -1;
    changeWeaponEffect_timer = WEAPON_CHANGE_COOLDOWN_TIMER;

    currWeaponId = weaponId;
    switch (currWeaponId) {
        case WEAPON_FIST:
            currWeaponAnimFireCooldownTimer = WEAPON_FIST_FIRE_COOLDOWN_TIMER;
            currWeaponAnimReadyToHitAgainFrame = WEAPON_FIST_ANIM_READY_TO_HIT_AGAIN_FRAME;
            weapon_load(&sprDef_weapon_fist_anim, pal_weapon_fist_anim.data, 
                WEAPON_SPRITE_X_CENTERED - ((WEAPON_FIST_ANIM_WIDTH+1)/2)*8 - 2*8, 
                WEAPON_SPRITE_Y_ABOVE_HUD - WEAPON_FIST_ANIM_HEIGHT*8);
            break;
        case WEAPON_PISTOL:
            currWeaponAnimFireCooldownTimer = WEAPON_PISTOL_FIRE_COOLDOWN_TIMER;
            currWeaponAnimReadyToHitAgainFrame = WEAPON_PISTOL_ANIM_READY_TO_HIT_AGAIN_FRAME;
            weapon_load(&sprDef_weapon_pistol_anim, pal_weapon_pistol_anim.data, 
                WEAPON_SPRITE_X_CENTERED - ((WEAPON_PISTOL_ANIM_WIDTH+1)/2)*8, 
                WEAPON_SPRITE_Y_ABOVE_HUD - WEAPON_PISTOL_ANIM_HEIGHT*8);
            break;
        case WEAPON_SHOTGUN: break;
        case WEAPON_MACHINE_GUN: break;
        case WEAPON_ROCKET: break;
        case WEAPON_PLASMA: break;
        case WEAPON_BFG: break;
        default: break;
    }
}

static FORCE_INLINE void stopFireAnimation ()
{
    SPR_setAutoAnimation(spr_currWeapon, FALSE);
    // reset the animation to the frame that stays ready to hit the hit animation (no idle frame)
    SPR_setFrame(spr_currWeapon, currWeaponAnimReadyToHitAgainFrame);
}

FORCE_INLINE void weapon_next (u8 sign)
{
    if (select_coolDown_timer != 0)
        return;

    if (fire_coolDown_timer != 0)
        stopFireAnimation();

    // Iterate until next weapon in inventory is found
    u8 nextWeaponId = currWeaponId;
    for (u8 i=WEAPON_MAX_COUNT; --i;) {
        nextWeaponId = modu((nextWeaponId + sign + WEAPON_MAX_COUNT), WEAPON_MAX_COUNT);
        if (hud_hasWeaponInInventory(nextWeaponId)) {
            weapon_select(nextWeaponId);
            select_coolDown_timer = 2*WEAPON_CHANGE_COOLDOWN_TIMER;
            return;
        }
    }
}

void weapon_addAmmo (u8 weaponId, u16 amnt)
{
    u16 currAmmo = ammoInventory[weaponId];
    switch (currWeaponId) {
        case WEAPON_FIST: return;
        case WEAPON_PISTOL:
            if ((currAmmo + amnt) > WEAPON_PISTOL_MAX_AMMO)
                amnt = WEAPON_PISTOL_MAX_AMMO - currAmmo;
            break;
        case WEAPON_SHOTGUN:
            if ((currAmmo + amnt) > WEAPON_SHOTGUN_MAX_AMMO)
                amnt = WEAPON_SHOTGUN_MAX_AMMO - currAmmo;
            break;
        case WEAPON_MACHINE_GUN:
            if ((currAmmo + amnt) > WEAPON_MACHINE_GUN_MAX_AMMO)
                amnt = WEAPON_MACHINE_GUN_MAX_AMMO - currAmmo;
            break;
        case WEAPON_ROCKET:
            if ((currAmmo + amnt) > WEAPON_ROCKET_MAX_AMMO)
                amnt = WEAPON_ROCKET_MAX_AMMO - currAmmo;
            break;
        case WEAPON_PLASMA:
            if ((currAmmo + amnt) > WEAPON_PLASMA_MAX_AMMO)
                amnt = WEAPON_PLASMA_MAX_AMMO - currAmmo;
            break;
        case WEAPON_BFG:
            if ((currAmmo + amnt) > WEAPON_BFG_MAX_AMMO)
                amnt = WEAPON_BFG_MAX_AMMO - currAmmo;
            break;
        default: return;
    }

    ammoInventory[weaponId] += amnt;
    hud_addAmmoUnits(amnt);
}

static FORCE_INLINE bool useAmmo ()
{
    switch (currWeaponId) {
        case WEAPON_FIST: return TRUE;
        case WEAPON_PISTOL:
            if (ammoInventory[WEAPON_PISTOL] == 0) return FALSE;
            --ammoInventory[WEAPON_PISTOL];
            hud_subAmmoUnits(1);
            return TRUE;
        case WEAPON_SHOTGUN:
            if (ammoInventory[WEAPON_SHOTGUN] == 0) return FALSE;
            --ammoInventory[WEAPON_SHOTGUN];
            hud_subAmmoUnits(1);
            return TRUE;
        case WEAPON_MACHINE_GUN:
            if (ammoInventory[WEAPON_MACHINE_GUN] == 0) return FALSE;
            if (ammoInventory[WEAPON_MACHINE_GUN] < 10)
                ammoInventory[WEAPON_MACHINE_GUN] = 0;
            else
                ammoInventory[WEAPON_MACHINE_GUN] = ammoInventory[WEAPON_MACHINE_GUN] - 8;
            hud_subAmmoUnits(8);
            return TRUE;
        case WEAPON_ROCKET:
            if (ammoInventory[WEAPON_ROCKET] == 0) return FALSE;
            --ammoInventory[WEAPON_ROCKET];
            hud_subAmmoUnits(1);
            return TRUE;
        case WEAPON_PLASMA:
            if (ammoInventory[WEAPON_PLASMA] == 0) return FALSE;
            --ammoInventory[WEAPON_PLASMA];
            hud_subAmmoUnits(1);
            return TRUE;
        case WEAPON_BFG:
            if (ammoInventory[WEAPON_BFG] == 0) return FALSE;
            if (ammoInventory[WEAPON_BFG] < 50)
                ammoInventory[WEAPON_BFG] = 0;
            else
                ammoInventory[WEAPON_BFG] = ammoInventory[WEAPON_BFG] - 0;
            hud_subAmmoUnits(50);
            return TRUE;
        default: return FALSE;
    }
}

FORCE_INLINE void weapon_fire ()
{
    if ((fire_coolDown_timer | changeWeaponEffect_timer) != 0)
        return;

    if (useAmmo() == FALSE)
        return;

    fire_coolDown_timer = currWeaponAnimFireCooldownTimer;
    resetToIdle_timer = WEAPON_RESET_TO_IDLE_TIMER;

    // reset the animation to the frame that starts the hit animation (no idle frame)
    SPR_setFrame(spr_currWeapon, WEAPON_START_HIT_FRAME);
    SPR_setAutoAnimation(spr_currWeapon, TRUE);
}

FORCE_INLINE void weapon_update ()
{
    if (SPR_isAnimationDone(spr_currWeapon))
        stopFireAnimation();

    if (fire_coolDown_timer != 0) {
        --fire_coolDown_timer;
    }

    if (select_coolDown_timer != 0) {
        --select_coolDown_timer;
    }

    if (changeWeaponEffect_timer != 0) {
        // scroll up or down depending on the direction
        --changeWeaponEffect_timer;
    }
    else if (changeWeaponEffect_direction == -1) {
        // once scroll down effect finishes continue with scroll up effect
        changeWeaponEffect_direction = 1;
        changeWeaponEffect_timer = WEAPON_CHANGE_COOLDOWN_TIMER;
    }

    if (resetToIdle_timer != 0) {
        --resetToIdle_timer;
        if (resetToIdle_timer == 0)
            SPR_setFrame(spr_currWeapon, 0);
    }
}