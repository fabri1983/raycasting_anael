#include <types.h>
#include <vdp_tile.h>
#include <sprite_eng.h>
#include <maths.h>
#include <timer.h>
#include "consts_ext.h"
#include "weapon_consts.h"
#include "weapon.h"
#include "weapons_res.h"
#include "hud.h"
#include "spr_eng_override.h"

u16 resetToIdle_timer;
u16 fire_coolDown_timer;
u16 select_coolDown_timer;
u16 changeWeaponEffect_timer;
s16 changeWeaponEffect_direction;

Sprite* spr_currWeapon;
u16 currWeaponId;
u16 currWeaponAnimFireCooldownTimer;
u16 currWeaponAnimReadyToHitAgainFrame;
u16 ammoInventory[WEAPON_MAX_COUNT] = {0};

u16 currWeaponSpriteX;
u16 currWeaponSpriteY;

s16 weaponSwayX = 0;
s16 weaponSwayY = 0;
s16 weaponSwayDirX = 1;
s16 weaponSwayDirY = 0; // Neutral
bool isMoving = FALSE;

#define MAX_WEAPON_SWAY_X 8 // Maximum weapon sway in pixels
#define MAX_WEAPON_SWAY_Y 6 // Maximum weapon sway in pixels. Must be >= MAX_WEAPON_SWAY_X

u16 weapon_biggestAnimTileNum ()
{
    u16 maxTileNum = 0;
    maxTileNum = max(maxTileNum, sprDef_weapon_fist_anim.maxNumTile);
    maxTileNum = max(maxTileNum, sprDef_weapon_pistol_anim.maxNumTile);
    maxTileNum = max(maxTileNum, sprDef_weapon_shotgun_anim.maxNumTile);
    return maxTileNum;
}

u16 weapon_getVRAMLocation ()
{
    // TILE_FONT_INDEX is the VRAM index location where the SGDK's Sprite Engine calculates the dedicated VRAM going backwards
    return TILE_FONT_INDEX - weapon_biggestAnimTileNum();
}

void weapon_resetState ()
{
    // Sprites use tile attributes, but no tile index when using SPR_FLAG_AUTO_VRAM_ALLOC.
    // But here we need to set a fixed VRAM index location so using TILE_ATTR_FULL.
    u16 baseTileAttribs = (u16)TILE_ATTR_FULL(WEAPON_BASE_PAL, 0, FALSE, FALSE, weapon_getVRAMLocation());

    // Loads fist sprite so we can avoid the check for NULL in other methods
    spr_currWeapon = spr_eng_addSpriteEx(&sprDef_weapon_fist_anim, 0, 0, baseTileAttribs, 
        (u16)(SPR_FLAG_AUTO_TILE_UPLOAD | SPR_FLAG_DISABLE_ANIMATION_LOOP | SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE | SPR_FLAG_INSERT_HEAD));
    SPR_setVisibility(spr_currWeapon, HIDDEN);
    SPR_setAutoAnimation(spr_currWeapon, FALSE);
    // Load the palettes at fixed RAM location so we can use it as a constant for faster DMA setup
    memcpy((void*)RAM_FIXED_WEAPON_PALETTES_ADDRESS, (void*)pal_weapon_fist_anim.data, (16*WEAPON_USED_PALS)*2); // *2 for byte addressing
    PAL_setColors(WEAPON_BASE_PAL*16 + 1, (u16*)(RAM_FIXED_WEAPON_PALETTES_ADDRESS + 1*2), 16*WEAPON_USED_PALS - 1, DMA);

    currWeaponId = (u16)WEAPON_FIST;
    currWeaponAnimFireCooldownTimer = 0;
    currWeaponAnimReadyToHitAgainFrame = 0;
    resetToIdle_timer = 0;
    fire_coolDown_timer = 0;
    select_coolDown_timer = 0;
    changeWeaponEffect_timer = 0;
}

void weapon_free_pals_buffer ()
{
    memsetU32((u32*)RAM_FIXED_WEAPON_PALETTES_ADDRESS, 0, (16*WEAPON_USED_PALS)/2);
}

static void weapon_load (const SpriteDefinition* sprDef, u16* pal, s16 x, s16 y)
{
    resetToIdle_timer = 0;
    fire_coolDown_timer = 0;
    SPR_releaseSprite(spr_currWeapon);

    // Sprites use tile attributes, but no tile index when using SPR_FLAG_AUTO_VRAM_ALLOC.
    // But here we need to set a fixed VRAM index location so using TILE_ATTR_FULL.
    u16 baseTileAttribs = (u16)TILE_ATTR_FULL(WEAPON_BASE_PAL, 0, FALSE, FALSE, weapon_getVRAMLocation());

    spr_currWeapon = spr_eng_addSpriteEx(sprDef, x, y, baseTileAttribs, 
        (u16)(SPR_FLAG_AUTO_TILE_UPLOAD | SPR_FLAG_DISABLE_ANIMATION_LOOP | SPR_FLAG_DISABLE_DELAYED_FRAME_UPDATE | SPR_FLAG_INSERT_HEAD));
    SPR_setAutoAnimation(spr_currWeapon, FALSE); // Animation is triggered manually

    // Load the palettes at fixed RAM location so we can use it as a constant for faster DMA setup
    memcpy((void*)RAM_FIXED_WEAPON_PALETTES_ADDRESS, (void*)pal, (u16)(16*WEAPON_USED_PALS)*2); // *2 for byte addressing
}

void weapon_select (u16 weaponId)
{
    currWeaponId = weaponId;

    // Start weapon change effect for the current weapon
    changeWeaponEffect_direction = -1;
    changeWeaponEffect_timer = (u16)WEAPON_CHANGE_COOLDOWN_TIMER;
    // Set now the selection cooldown timer
    select_coolDown_timer = (u16)WEAPON_CHANGE_COOLDOWN_TIMER * 2; // *2 due to scroll down and up effect

    hud_resetAmmo(); //hud_setAmmo((u16)0, (u16)0, (u16)0);
    u16 currAmmo = ammoInventory[currWeaponId];
    hud_addAmmoUnits(currAmmo);

    switch (currWeaponId) {
        case WEAPON_FIST:
            currWeaponAnimFireCooldownTimer = (u16)WEAPON_FIST_FIRE_COOLDOWN_TIMER;
            currWeaponAnimReadyToHitAgainFrame = (u16)WEAPON_FIST_ANIM_READY_TO_HIT_AGAIN_FRAME;
            currWeaponSpriteX = (u16)WEAPON_SPRITE_FIST_X;
            currWeaponSpriteY = (u16)WEAPON_SPRITE_FIST_Y;
            weapon_load(&sprDef_weapon_fist_anim, pal_weapon_fist_anim.data, (s16)WEAPON_SPRITE_FIST_X, (s16)WEAPON_SPRITE_FIST_Y);
            break;
        case WEAPON_PISTOL:
            currWeaponAnimFireCooldownTimer = (u16)WEAPON_PISTOL_FIRE_COOLDOWN_TIMER;
            currWeaponAnimReadyToHitAgainFrame = (u16)WEAPON_PISTOL_ANIM_READY_TO_HIT_AGAIN_FRAME;
            currWeaponSpriteX = (u16)WEAPON_SPRITE_PISTOL_X;
            currWeaponSpriteY = (u16)WEAPON_SPRITE_PISTOL_Y;
            weapon_load(&sprDef_weapon_pistol_anim, pal_weapon_pistol_anim.data, (s16)WEAPON_SPRITE_PISTOL_X, (s16)WEAPON_SPRITE_PISTOL_Y);
            break;
        case WEAPON_SHOTGUN: 
            currWeaponAnimFireCooldownTimer = (u16)WEAPON_SHOTGUN_FIRE_COOLDOWN_TIMER;
            currWeaponAnimReadyToHitAgainFrame = (u16)WEAPON_SHOTGUN_ANIM_READY_TO_HIT_AGAIN_FRAME;
            currWeaponSpriteX = (u16)WEAPON_SPRITE_SHOTGUN_X;
            currWeaponSpriteY = (u16)WEAPON_SPRITE_SHOTGUN_Y;
            weapon_load(&sprDef_weapon_shotgun_anim, pal_weapon_shotgun_anim.data, (s16)WEAPON_SPRITE_SHOTGUN_X, (s16)WEAPON_SPRITE_SHOTGUN_Y);
            break;
        case WEAPON_MACHINE_GUN: break;
        case WEAPON_ROCKET: break;
        case WEAPON_PLASMA: break;
        case WEAPON_BFG: break;
        default: __builtin_unreachable();
    }
}

static void stopFireAnimation ()
{
    SPR_setAutoAnimation(spr_currWeapon, FALSE);
    // reset the animation to the frame that stays ready to hit the hit animation (no idle frame)
    SPR_setFrame(spr_currWeapon, currWeaponAnimReadyToHitAgainFrame);
}

void weapon_next (s16 dir)
{
    // allow select other weapon after a certain period of time
    if (select_coolDown_timer != 0)
        return;

    if (fire_coolDown_timer != 0)
        stopFireAnimation();

    // Iterate until next weapon in inventory is found
    u16 nextWeaponId = currWeaponId;
    for (u16 i = (u16)WEAPON_MAX_COUNT; --i;) {
        u16 newDir = nextWeaponId + dir + (u16)WEAPON_MAX_COUNT;
        nextWeaponId = modu(newDir, (u16)WEAPON_MAX_COUNT);
        bool hasWeapon = hud_hasWeaponInInventory(nextWeaponId);
        if (hasWeapon) {
            weapon_select(nextWeaponId);
            return;
        }
    }
}

void weapon_addAmmo (u16 weaponId, u16 amnt)
{
    u16 currAmmo = ammoInventory[weaponId];
    u16 newAmnt_limit = currAmmo + amnt;
    switch (currWeaponId) {
        case WEAPON_FIST: return;
        case WEAPON_PISTOL:
            if (newAmnt_limit > (u16)WEAPON_PISTOL_MAX_AMMO)
                amnt = (u16)WEAPON_PISTOL_MAX_AMMO - currAmmo;
            break;
        case WEAPON_SHOTGUN:
            if (newAmnt_limit > (u16)WEAPON_SHOTGUN_MAX_AMMO)
                amnt = (u16)WEAPON_SHOTGUN_MAX_AMMO - currAmmo;
            break;
        case WEAPON_MACHINE_GUN:
            if (newAmnt_limit > (u16)WEAPON_MACHINE_GUN_MAX_AMMO)
                amnt = (u16)WEAPON_MACHINE_GUN_MAX_AMMO - currAmmo;
            break;
        case WEAPON_ROCKET:
            if (newAmnt_limit > (u16)WEAPON_ROCKET_MAX_AMMO)
                amnt = (u16)WEAPON_ROCKET_MAX_AMMO - currAmmo;
            break;
        case WEAPON_PLASMA:
            if (newAmnt_limit > (u16)WEAPON_PLASMA_MAX_AMMO)
                amnt = (u16)WEAPON_PLASMA_MAX_AMMO - currAmmo;
            break;
        case WEAPON_BFG:
            if (newAmnt_limit > (u16)WEAPON_BFG_MAX_AMMO)
                amnt = (u16)WEAPON_BFG_MAX_AMMO - currAmmo;
            break;
        default: __builtin_unreachable();
    }

    u16 newAmnt = currAmmo + amnt;
    ammoInventory[weaponId] = newAmnt;
    hud_resetAmmo(); //hud_setAmmo((u16)0, (u16)0, (u16)0);
    hud_addAmmoUnits(newAmnt);
}

static bool useAmmo ()
{
    if (currWeaponId == (u16)WEAPON_FIST)
        return TRUE;

    u16 currAmmo = ammoInventory[currWeaponId];
    if (currAmmo == 0)
        return FALSE;

    switch (currWeaponId) {
        case WEAPON_PISTOL:
            ammoInventory[currWeaponId] = currAmmo - (u16)1;
            hud_subAmmoUnits((u16)1);
            break;
        case WEAPON_SHOTGUN:
            ammoInventory[currWeaponId] = currAmmo - (u16)1;
            hud_subAmmoUnits((u16)1);
            break;
        case WEAPON_MACHINE_GUN:
            if (currAmmo < 10)
                ammoInventory[currWeaponId] = 0;
            else
                ammoInventory[currWeaponId] = currAmmo - (u16)8;
            hud_subAmmoUnits((u16)8);
            break;
        case WEAPON_ROCKET:
            ammoInventory[currWeaponId] = currAmmo - (u16)1;
            hud_subAmmoUnits((u16)1);
            break;
        case WEAPON_PLASMA:
            ammoInventory[currWeaponId] = currAmmo - (u16)1;
            hud_subAmmoUnits((u16)1);
            break;
        case WEAPON_BFG:
            if (currAmmo < (u16)50)
                ammoInventory[currWeaponId] = 0;
            else
                ammoInventory[currWeaponId] = currAmmo - (u16)50;
            hud_subAmmoUnits((u16)50);
            break;
        default: break;
    }

    return TRUE;
}

void weapon_fire ()
{
    if ((fire_coolDown_timer | changeWeaponEffect_timer) != 0)
        return;
    
    bool used = useAmmo();
    if (!used)
        return;

    weaponSwayX = 0;
    weaponSwayY = 0;

    fire_coolDown_timer = currWeaponAnimFireCooldownTimer;
    resetToIdle_timer = (u16)WEAPON_RESET_TO_IDLE_TIMER;

    // reset the animation to the frame that starts the hit animation (no idle frame)
    SPR_setFrame(spr_currWeapon, (s16)WEAPON_START_HIT_FRAME);
    SPR_setAutoAnimation(spr_currWeapon, TRUE);
}

void weapon_updateSway (bool _isMoving)
{
    isMoving = _isMoving;
    if (isMoving == FALSE) {
        // Set Sway Y as neutral until Sway X reaches left or right bound
        weaponSwayDirY = 0;
        return;
    }

    weaponSwayX += weaponSwayDirX;
    if (weaponSwayX > (s16)MAX_WEAPON_SWAY_X) {
        weaponSwayX = (s16)MAX_WEAPON_SWAY_X;
        weaponSwayDirX = -1;
        // This only applies at the start of moving action
        if (weaponSwayDirY == 0)
            weaponSwayDirY = 1;
    }
    else if (weaponSwayX < (s16)-MAX_WEAPON_SWAY_X) {
        weaponSwayX = (s16)-MAX_WEAPON_SWAY_X;
        weaponSwayDirX = 1;
        // This only applies at the start of moving action
        if (weaponSwayDirY == 0)
            weaponSwayDirY = 1;
    }

    // When Sway X is at the center we tell Sway Y to start moving up
    if (weaponSwayX == 0 && weaponSwayDirY == 1)
        weaponSwayDirY = -1;

    weaponSwayY += weaponSwayDirY;
    if (weaponSwayY > (s16)MAX_WEAPON_SWAY_Y) {
        weaponSwayY = (s16)MAX_WEAPON_SWAY_Y;
    }
    else if (weaponSwayY < 0) {
        weaponSwayY = 0;
        weaponSwayDirY = 0; // Wait until start of walking action and Sway X reaches any bound
    }
}

void weapon_update ()
{
    if (SPR_isAnimationDone(spr_currWeapon)) {
        stopFireAnimation();
    }

    if (fire_coolDown_timer != 0) {
        --fire_coolDown_timer;
        if ((weaponSwayX | weaponSwayY) != 0) {
            //SPR_setPosition(spr_currWeapon, currWeaponSpriteX, currWeaponSpriteY);
            spr_currWeapon->x = currWeaponSpriteX + 0x80;
            spr_currWeapon->y = currWeaponSpriteY + 0x80;
            weaponSwayX = 0;
            weaponSwayY = 0;
        }
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
            //SPR_setPosition(spr_currWeapon, currWeaponSpriteX + weaponSwayX, currWeaponSpriteY + weaponSwayY);
            spr_currWeapon->x = currWeaponSpriteX + weaponSwayX + 0x80;
            spr_currWeapon->y = currWeaponSpriteY + weaponSwayY + 0x80;
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
        changeWeaponEffect_timer = (u16)WEAPON_CHANGE_COOLDOWN_TIMER;
    }

    if (resetToIdle_timer != 0) {
        --resetToIdle_timer;
        if (resetToIdle_timer == 0)
            SPR_setFrame(spr_currWeapon, (s16)0);
    }
}