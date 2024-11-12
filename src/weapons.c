#include "weapons.h"
#include <vdp_tile.h>
#include <sprite_eng.h>
#include <maths.h>
#include <timer.h>
#include "weapons_res.h"
#include "hint_callback.h"
#include "vint_callback.h"

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

s16 weaponSwayX = 0;
s16 weaponSwayY = 0;
s16 weaponSwayDirX = 1;
s16 weaponSwayDirY = 0; // Neutral
bool isMoving = FALSE;
#define MAX_WEAPON_SWAY_X 8 // Maximum weapon sway in pixels
#define MAX_WEAPON_SWAY_Y 6 // Maximum weapon sway in pixels. Must be >= MAX_WEAPON_SWAY_X
u16 currWeaponSpriteX;
u16 currWeaponSpriteY;

u16 weapon_biggerAnimTileNum ()
{
    // u16 maxTileNum = sprDef_weapon_fist_anim.maxNumTile;
    // maxTileNum = max(maxTileNum, sprDef_weapon_pistol_anim.maxNumTile);
    // maxTileNum = max(maxTileNum, sprDef_weapon_shotgun_anim.maxNumTile);
    // return maxTileNum;
    return 144;
}

u16 weapon_getVRAMLocation ()
{
    // TILE_FONT_INDEX is the VRAM index location where the SGDK's Sprite Engine calculates the dedicated VRAM going backwards
    return TILE_FONT_INDEX - weapon_biggerAnimTileNum();
}

void weapon_resetState ()
{
    // Sprites use tile attributes, but no tile index when using SPR_FLAG_AUTO_VRAM_ALLOC.
    // But here we need to set a fixed VRAM index location so using TILE_ATTR_FULL.
    u16 baseTileAttribs = TILE_ATTR_FULL(WEAPON_BASE_PAL, 0, FALSE, FALSE, weapon_getVRAMLocation());

    // Loads fist sprite so we can avoid the check for NULL in other methods
    spr_currWeapon = SPR_addSpriteEx(&sprDef_weapon_fist_anim, 0, 0, baseTileAttribs, 
        SPR_FLAG_AUTO_TILE_UPLOAD | SPR_FLAG_DISABLE_ANIMATION_LOOP | SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE | SPR_FLAG_INSERT_HEAD);
    SPR_setVisibility(spr_currWeapon, HIDDEN);
    SPR_setAutoAnimation(spr_currWeapon, FALSE);
    PAL_setColors(WEAPON_BASE_PAL*16 + 1, pal_weapon_fist_anim.data + 1, 15, DMA);
    //PAL_setColors((WEAPON_BASE_PAL+1)*16 + 1, sprDef_weapon_fist_anim.palette->data + 16 + 1, 15, DMA);

    currWeaponId = 0; // Fist
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

    spr_currWeapon = SPR_addSpriteEx(sprDef, x, y, baseTileAttribs, 
        SPR_FLAG_AUTO_TILE_UPLOAD | SPR_FLAG_DISABLE_ANIMATION_LOOP | SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE | SPR_FLAG_INSERT_HEAD);
    SPR_setAutoAnimation(spr_currWeapon, FALSE); // Animation is triggered manually

    // No need to enqueue the palette since it will be restored after the hud is displayed
    //hint_enqueueWeaponPal(pal);
    #if HUD_RELOAD_OVERRIDEN_PALETTES_AT_HINT
    hint_setPalToRestore(pal);
    #else
    vint_setPalToRestore(pal);
    #endif
}

void weapon_select (u8 weaponId)
{
    currWeaponId = weaponId;

    // Start weapon change effect for the current weapon
    changeWeaponEffect_direction = -1;
    changeWeaponEffect_timer = WEAPON_CHANGE_COOLDOWN_TIMER;

    hud_setAmmo(0, 0, 0);
    hud_addAmmoUnits(ammoInventory[currWeaponId]);

    switch (currWeaponId) {
        case WEAPON_FIST:
            currWeaponAnimFireCooldownTimer = WEAPON_FIST_FIRE_COOLDOWN_TIMER;
            currWeaponAnimReadyToHitAgainFrame = WEAPON_FIST_ANIM_READY_TO_HIT_AGAIN_FRAME;
            currWeaponSpriteX = WEAPON_SPRITE_FIST_X;
            currWeaponSpriteY = WEAPON_SPRITE_FIST_Y;
            weapon_load(&sprDef_weapon_fist_anim, pal_weapon_fist_anim.data, WEAPON_SPRITE_FIST_X, WEAPON_SPRITE_FIST_Y);
            break;
        case WEAPON_PISTOL:
            currWeaponAnimFireCooldownTimer = WEAPON_PISTOL_FIRE_COOLDOWN_TIMER;
            currWeaponAnimReadyToHitAgainFrame = WEAPON_PISTOL_ANIM_READY_TO_HIT_AGAIN_FRAME;
            currWeaponSpriteX = WEAPON_SPRITE_PISTOL_X;
            currWeaponSpriteY = WEAPON_SPRITE_PISTOL_Y;
            weapon_load(&sprDef_weapon_pistol_anim, pal_weapon_pistol_anim.data, WEAPON_SPRITE_PISTOL_X, WEAPON_SPRITE_PISTOL_Y);
            break;
        case WEAPON_SHOTGUN: 
            currWeaponAnimFireCooldownTimer = WEAPON_SHOTGUN_FIRE_COOLDOWN_TIMER;
            currWeaponAnimReadyToHitAgainFrame = WEAPON_SHOTGUN_ANIM_READY_TO_HIT_AGAIN_FRAME;
            currWeaponSpriteX = WEAPON_SPRITE_SHOTGUN_X;
            currWeaponSpriteY = WEAPON_SPRITE_SHOTGUN_Y;
            weapon_load(&sprDef_weapon_shotgun_anim, pal_weapon_shotgun_anim.data, WEAPON_SPRITE_SHOTGUN_X, WEAPON_SPRITE_SHOTGUN_Y);
            break;
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
            select_coolDown_timer = 2 * WEAPON_CHANGE_COOLDOWN_TIMER; // *2 due to scroll down and up effect
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

    ammoInventory[weaponId] = currAmmo + amnt;
    hud_setAmmo(0, 0, 0);
    hud_addAmmoUnits(currAmmo + amnt);
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
                ammoInventory[WEAPON_BFG] -= 50;
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

    weaponSwayX = 0;
    weaponSwayY = 0;

    fire_coolDown_timer = currWeaponAnimFireCooldownTimer;
    resetToIdle_timer = WEAPON_RESET_TO_IDLE_TIMER;

    // reset the animation to the frame that starts the hit animation (no idle frame)
    SPR_setFrame(spr_currWeapon, WEAPON_START_HIT_FRAME);
    SPR_setAutoAnimation(spr_currWeapon, TRUE);
}

FORCE_INLINE void weapon_updateSway (bool _isMoving)
{
    isMoving = _isMoving;
    if (isMoving == FALSE) {
        // Set Sway Y as neutral until Sway X reaches left or right bound
        weaponSwayDirY = 0;
        return;
    }

    weaponSwayX += weaponSwayDirX;
    if (weaponSwayX > MAX_WEAPON_SWAY_X) {
        weaponSwayX = MAX_WEAPON_SWAY_X;
        weaponSwayDirX = -1;
        // This only applies at the start of moving action
        if (weaponSwayDirY == 0)
            weaponSwayDirY = 1;
    }
    else if (weaponSwayX < -MAX_WEAPON_SWAY_X) {
        weaponSwayX = -MAX_WEAPON_SWAY_X;
        weaponSwayDirX = 1;
        // This only applies at the start of moving action
        if (weaponSwayDirY == 0)
            weaponSwayDirY = 1;
    }

    // When Sway X is at the center we tell Sway Y to start moving up
    if (weaponSwayX == 0 && weaponSwayDirY == 1)
        weaponSwayDirY = -1;

    weaponSwayY += weaponSwayDirY;
    if (weaponSwayY > MAX_WEAPON_SWAY_Y) {
        weaponSwayY = MAX_WEAPON_SWAY_Y;
    }
    else if (weaponSwayY < 0) {
        weaponSwayY = 0;
        weaponSwayDirY = 0; // Wait until start of walking action and Sway X reaches any bound
    }
}

FORCE_INLINE void weapon_update ()
{
    if (SPR_getAutoAnimation(spr_currWeapon) && SPR_isAnimationDone(spr_currWeapon)) {
        stopFireAnimation();
    }

    if (fire_coolDown_timer != 0) {
        --fire_coolDown_timer;
    }
    // If fire_coolDown_timer == 0 it means is not in fire animation
    else {
        if ((weaponSwayX | weaponSwayY) != 0) {
            // When no moving we damper the sway values to 0
            if (isMoving == FALSE) {
                // Sway X damping to 0
                if (weaponSwayX > 0) weaponSwayX -= 1;
                else if (weaponSwayX < 0) weaponSwayX += 1;
                // Sway Y damping to 0
                if (weaponSwayY > 0) weaponSwayY -= 1;
            }
            isMoving = FALSE;
            SPR_setPosition(spr_currWeapon, currWeaponSpriteX + weaponSwayX, currWeaponSpriteY + weaponSwayY);
        }
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