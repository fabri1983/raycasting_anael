#include <types.h>
#include <vdp.h>
#include <vdp_bg.h>
#include <vdp_tile.h>
#include <dma.h>
#include <memory.h>
#include <mapper.h>
#include "hud.h"
#include "hud_320.h"
#include "hud_res.h"
#include "consts.h"
#include "utils.h"
#include "hint_callback.h"

static u16* hud_tilemap_src;

#if HUD_TILEMAP_COMPRESSED
static u16 tilemapUnpacked[HUD_SOURCE_IMAGE_W * HUD_SOURCE_IMAGE_H];
u16 hud_tilemap_dst[PLANE_COLUMNS * HUD_BG_H];

static void decompressTilemap ()
{
    TileMap* src = img_hud_spritesheet.tilemap;
    unpackSelector(src->compression, (u8*) FAR_SAFE(src->tilemap, ((HUD_SOURCE_IMAGE_W) * (HUD_SOURCE_IMAGE_H)) * 2), (u8*) tilemapUnpacked);
}
#endif

static u8 weaponInventoryBits;
static u8 keyInventoryBits;
static u8 faceExpressionTimer;
static u8 faceExpressionCol;

static Digits ammo_digits = { .hundrs = 0, .tens = 0, .ones = 0 };
static Digits health_digits = { .hundrs = 0, .tens = 0, .ones = 0 };
static Digits armor_digits = { .hundrs = 0, .tens = 0, .ones = 0 };

static u16 updateFlags;

static FORCE_INLINE void addHUDDigits (Digits* digits, u16 amnt)
{
    digits->ones += amnt;

    // Check if we need to carry to tens
    if (digits->ones >= 10) {
        digits->tens += divu(digits->ones, 10);
        digits->ones = modu(digits->ones, 10);

        // Check if we need to carry to hundreds
        if (digits->tens >= 10) {
            digits->hundrs += divu(digits->tens, 10);
            digits->tens = modu(digits->tens, 10);

            // Check for overflow in hundreds
            // if (digits->hundrs >= 10) {
            //     digits->hundrs = 9;
            //     digits->tens = 9;
            //     digits->ones = 9;
            // }
        }
    }
}

static FORCE_INLINE void subHUDDigits (Digits* digits, u16 amnt)
{
    if (digits->ones >= amnt) {
        digits->ones -= amnt;
        return;
    }
    
    // If we're here, we need to borrow from tens
    if (digits->tens > 0) {
        digits->tens -= 1;
        digits->ones += 10;
        digits->ones -= amnt;
        return;
    }
    
    // If we're here, we need to borrow from hundreds
    if (digits->hundrs > 0) {
        digits->hundrs -= 1;
        digits->tens += 9;
        digits->ones += 10;
        digits->ones -= amnt;
        return;
    }
}

FORCE_INLINE void hud_resetAmmo ()
{
    updateFlags |= 1 << UPDATE_FLAG_AMMO;
    hud_setAmmo(0, 0, 0); // no ammo
}

void hud_setAmmo (u8 hundreds, u8 tens, u8 ones)
{
    updateFlags |= 1 << UPDATE_FLAG_AMMO;
    ammo_digits.hundrs = hundreds;
    ammo_digits.tens = tens;
    ammo_digits.ones = ones;
}

void hud_addAmmoUnits (u16 amnt)
{
    updateFlags |= 1 << UPDATE_FLAG_AMMO;
    addHUDDigits(&ammo_digits, amnt);
}

void hud_subAmmoUnits (u16 amnt)
{
    updateFlags |= 1 << UPDATE_FLAG_AMMO;
    subHUDDigits(&ammo_digits, amnt);
}

FORCE_INLINE void hud_resetHealth ()
{
    updateFlags |= 1 << UPDATE_FLAG_HEALTH;
    hud_setHealth(0, 0, 0); // no health
}

void hud_setHealth (u8 hundreds, u8 tens, u8 ones)
{
    updateFlags |= 1 << UPDATE_FLAG_HEALTH;
    health_digits.hundrs = hundreds;
    health_digits.tens = tens;
    health_digits.ones = ones;
}

void hud_addHealthUnits (u16 amnt)
{
    updateFlags |= 1 << UPDATE_FLAG_HEALTH;
    addHUDDigits(&health_digits, amnt);
}

void hud_subHealthUnits (u16 amnt)
{
    updateFlags |= 1 << UPDATE_FLAG_HEALTH;
    subHUDDigits(&health_digits, amnt);
}

FORCE_INLINE void hud_resetArmor ()
{
    updateFlags |= 1 << UPDATE_FLAG_ARMOR;
    hud_setArmor(0, 0, 0); // no armor
}

void hud_setArmor (u8 hundreds, u8 tens, u8 ones)
{
    updateFlags |= 1 << UPDATE_FLAG_ARMOR;
    armor_digits.hundrs = hundreds;
    armor_digits.tens = tens;
    armor_digits.ones = ones;

}

void hud_addArmorUnits (u16 amnt)
{
    updateFlags |= 1 << UPDATE_FLAG_ARMOR;
    addHUDDigits(&armor_digits, amnt);
}

void hud_subArmorUnits (u16 amnt)
{
    updateFlags |= 1 << UPDATE_FLAG_ARMOR;
    subHUDDigits(&armor_digits, amnt);
}

FORCE_INLINE void hud_resetWeapons ()
{
    updateFlags |= 1 << UPDATE_FLAG_WEAPON;
    weaponInventoryBits = 0; // no weapons
}

FORCE_INLINE void hud_addWeapon (u8 weapon)
{
    updateFlags |= 1 << UPDATE_FLAG_WEAPON;
    weaponInventoryBits |= 1 << weapon;
}

FORCE_INLINE void hud_resetKeys ()
{
    updateFlags |= 1 << UPDATE_FLAG_KEY;
    keyInventoryBits = KEY_NONE; // no keys
}

FORCE_INLINE void hud_addKey (u8 key)
{
    updateFlags |= 1 << UPDATE_FLAG_KEY;
    keyInventoryBits |= 1 << key;
}

static void resetFaceExpressionTimer ()
{
    faceExpressionTimer = 0;
}

FORCE_INLINE void hud_resetFaceExpression ()
{
    resetFaceExpressionTimer();
    updateFlags |= 1 << UPDATE_FLAG_FACE;
    faceExpressionCol = FACE_EXPRESSION_CENTERED; // default
}

FORCE_INLINE void hud_setFaceExpression (u8 expr, u8 timer)
{
    updateFlags |= 1 << UPDATE_FLAG_FACE;
    faceExpressionCol = expr;
    faceExpressionTimer = timer;
}

FORCE_INLINE bool hud_hasWeaponInInventory (u8 weapon)
{
    return weaponInventoryBits & (1 << weapon);
}

FORCE_INLINE bool hud_hasKeyInInventory (u8 key)
{
    return keyInventoryBits & (1 << key);
}

FORCE_INLINE bool hud_isDead ()
{
    // This is indeed faster
    return (health_digits.hundrs | health_digits.tens | health_digits.ones) == 0;

    // This is indeed slower
    // Hack: treat the Digits struct as a 24-bit integer
    // union {
    //     Digits d;
    //     u32 i;
    // } conv = { .d = *(&health_digits) };
    // return (conv.i & 0x00FFFFFF) == 0;

}

#define COPY_TILEMAP_DATA(from, to, TARGET_W, TARGET_H) \
    __asm volatile ( \
        ".rept (%c[HEIGHT] - 1)\n" \
        "  .rept (%c[WIDTH]/2)\n" /* divided by 2 because we move long words */ \
        "    move.l   (%0)+,(%1)+\n" \
        "  .endr\n" \
        "  .if (%c[WIDTH] & 1)\n" /* handle odd WIDTH */ \
        "    move.w   (%0)+,(%1)+\n" \
        "  .endif\n" \
        "    lea     2*%c[FROM_NEXT_ROW](%0),%0\n" \
        "    lea     2*%c[TO_NEXT_ROW](%1),%1\n" \
        ".endr\n" \
        "  .rept (%c[WIDTH]/2)\n" /* divided by 2 because we move long words */ \
        "    move.l   (%0)+,(%1)+\n" \
        "  .endr\n" \
        "  .if (%c[WIDTH] & 1)\n" /* handle odd WIDTH */ \
        "    move.w   (%0)+,(%1)+\n" \
        "  .endif\n" \
        : "+a" (from), "+a" (to) \
        : [HEIGHT] "i" (TARGET_H), [WIDTH] "i" (TARGET_W), \
          [FROM_NEXT_ROW] "i" (HUD_SOURCE_IMAGE_W - TARGET_W), [TO_NEXT_ROW] "i" (PLANE_COLUMNS - TARGET_W) \
    )

static void setHUDBg ()
{
    // This sets the entire HUD BG tilemap
    u16* from = hud_tilemap_src + (HUD_BG_Y*HUD_SOURCE_IMAGE_W + HUD_BG_X);
    u16* to = hud_tilemap_dst + (HUD_BG_YP*PLANE_COLUMNS + HUD_BG_XP);
    COPY_TILEMAP_DATA(from, to, HUD_BG_W, HUD_BG_H);
}

static void setHUDDigitsCommon (u16 target_XP, u16 target_YP, Digits* digits)
{
    u8 hundrs = digits->hundrs;
    u8 tens = digits->tens;
    u8 ones = digits->ones;

    // Now accomodate for the empty tile which is always at position 0 for the respective digit
    if (hundrs > 0) {
        hundrs += 1; // offset the empty tile
        tens += 1; // offset the empty tile
    }
    else if (tens > 0)
        tens += 1; // offset the empty tile

    // Ones have no empty tiles
    ones += 1;

    u16* from;
    u16* to;

    // Hundreds
    from = hud_tilemap_src + ((HUD_NUMS_Y + 0*HUD_NUMS_H)*HUD_SOURCE_IMAGE_W + (HUD_NUMS_X + hundrs*HUD_NUMS_W));
    to = hud_tilemap_dst + (target_YP*PLANE_COLUMNS + (target_XP + 0*HUD_NUMS_W));
    COPY_TILEMAP_DATA(from, to, HUD_NUMS_W, HUD_NUMS_H);

    // Tens
    from = hud_tilemap_src + ((HUD_NUMS_Y + 1*HUD_NUMS_H)*HUD_SOURCE_IMAGE_W + (HUD_NUMS_X + tens*HUD_NUMS_W));
    to = hud_tilemap_dst + (target_YP*PLANE_COLUMNS + (target_XP + 1*HUD_NUMS_W));
    COPY_TILEMAP_DATA(from, to, HUD_NUMS_W, HUD_NUMS_H);

    // Ones
    from = hud_tilemap_src + ((HUD_NUMS_Y + 2*HUD_NUMS_H)*HUD_SOURCE_IMAGE_W + (HUD_NUMS_X + ones*HUD_NUMS_W));
    to = hud_tilemap_dst + (target_YP*PLANE_COLUMNS + (target_XP + 2*HUD_NUMS_W));
    COPY_TILEMAP_DATA(from, to, HUD_NUMS_W, HUD_NUMS_H);
}

static FORCE_INLINE void setHUDAmmo ()
{
    setHUDDigitsCommon(HUD_AMMO_XP, HUD_AMMO_YP, &ammo_digits);
}

static FORCE_INLINE void setHUDHealth ()
{
    setHUDDigitsCommon(HUD_HEALTH_XP, HUD_HEALTH_YP, &health_digits);
}

static FORCE_INLINE void setHUDArmor ()
{
    setHUDDigitsCommon(HUD_ARMOR_XP, HUD_ARMOR_YP, &armor_digits);
}

static FORCE_INLINE void setHUDWeapons ()
{
    // UPPER WEAPONS

    // has shotgun?
    if (weaponInventoryBits & (1 << WEAPON_SHOTGUN)) {
        u16* from = hud_tilemap_src + (HUD_WEAPON_Y*HUD_SOURCE_IMAGE_W + HUD_WEAPON_X);
        u16* to = hud_tilemap_dst + (HUD_WEAPON_HIGH_YP*PLANE_COLUMNS + HUD_WEAPON_HIGH_XP);
        COPY_TILEMAP_DATA(from, to, 1, 2); // dimensions for weapon 3: 1x2 tiles
    }
    // has machine gun?
    if (weaponInventoryBits & (1 << WEAPON_MACHINE_GUN)) {
        u16* from = hud_tilemap_src + (HUD_WEAPON_Y*HUD_SOURCE_IMAGE_W + HUD_WEAPON_X + 1); // jump previous weapons
        u16* to = hud_tilemap_dst + (HUD_WEAPON_HIGH_YP*PLANE_COLUMNS + HUD_WEAPON_HIGH_XP + 1); // jump previous weapons
        COPY_TILEMAP_DATA(from, to, 2, 2); // dimensions for weapon 4: 2x2 tiles
    }

    // LOWER WEAPONS

    if (weaponInventoryBits & (1 << WEAPON_ROCKET)) {
        u16* from = hud_tilemap_src + (HUD_WEAPON_Y*HUD_SOURCE_IMAGE_W + HUD_WEAPON_X + 3); // jump previous weapons
        u16* to = hud_tilemap_dst + (HUD_WEAPON_LOW_YP*PLANE_COLUMNS + HUD_WEAPON_LOW_XP);
        COPY_TILEMAP_DATA(from, to, 2, 2); // dimensions for weapon 5: 2x2 tiles
    }
    // has plasma but not shotgun?
    if ((weaponInventoryBits & ((1 << WEAPON_PLASMA) | (1 << WEAPON_SHOTGUN))) == ((1 << WEAPON_PLASMA) | (0 << WEAPON_SHOTGUN))) {
        u16* from = hud_tilemap_src + (HUD_WEAPON_Y*HUD_SOURCE_IMAGE_W + HUD_WEAPON_X + 5); // jump previous weapons
        u16* to = hud_tilemap_dst + (HUD_WEAPON_LOW_YP*PLANE_COLUMNS + HUD_WEAPON_LOW_XP + 2); // jump previous weapons
        COPY_TILEMAP_DATA(from, to, 1, 2); // dimensions for weapon 6: 1x2 tiles
    }
    // has plasma and shotgun?
    else if ((weaponInventoryBits & ((1 << WEAPON_PLASMA) | (1 << WEAPON_SHOTGUN))) == ((1 << WEAPON_PLASMA) | (1 << WEAPON_SHOTGUN))) {
        u16* from = hud_tilemap_src + (HUD_WEAPON_Y*HUD_SOURCE_IMAGE_W + HUD_WEAPON_X + 6); // jump previous weapons
        u16* to = hud_tilemap_dst + (HUD_WEAPON_LOW_YP*PLANE_COLUMNS + HUD_WEAPON_LOW_XP + 2); // jump previous weapons
        COPY_TILEMAP_DATA(from, to, 1, 2); // dimensions for weapon 6: 1x2 tiles
    }
    // has bfg but not machine gun?
    if ((weaponInventoryBits & ((1 << WEAPON_BFG) | (1 << WEAPON_MACHINE_GUN))) == ((1 << WEAPON_BFG) | (0 << WEAPON_MACHINE_GUN))) {
        u16* from = hud_tilemap_src + (HUD_WEAPON_Y*HUD_SOURCE_IMAGE_W + HUD_WEAPON_X + 7); // jump previous weapons
        u16* to = hud_tilemap_dst + (HUD_WEAPON_LOW_YP*PLANE_COLUMNS + HUD_WEAPON_LOW_XP + 3); // jump previous weapons
        COPY_TILEMAP_DATA(from, to, 2, 2); // dimensions for weapon 7: 2x2 tiles
    }
    // has bfg and machine gun?
    else if ((weaponInventoryBits & ((1 << WEAPON_BFG) | (1 << WEAPON_MACHINE_GUN))) == ((1 << WEAPON_BFG) | (1 << WEAPON_MACHINE_GUN))) {
        u16* from = hud_tilemap_src + (HUD_WEAPON_Y*HUD_SOURCE_IMAGE_W + HUD_WEAPON_X + 9); // jump previous weapons
        u16* to = hud_tilemap_dst + (HUD_WEAPON_LOW_YP*PLANE_COLUMNS + HUD_WEAPON_LOW_XP + 3); // jump previous weapons
        COPY_TILEMAP_DATA(from, to, 2, 2); // dimensions for weapon 7: 2x2 tiles
    }
}

static FORCE_INLINE void setHUDKeys ()
{
    // CARDS

    // has blue card?
    if (keyInventoryBits & (1 << KEY_CARD_BLUE)) {
        u16* from = hud_tilemap_src + (HUD_KEY_Y*HUD_SOURCE_IMAGE_W + HUD_KEY_X);
        u16* to = hud_tilemap_dst + (HUD_KEY_YP*PLANE_COLUMNS + HUD_KEY_XP);
        COPY_TILEMAP_DATA(from, to, 2, 2);
    }
    // has blue card and yellow card?
    if ((keyInventoryBits & ((1 << KEY_CARD_BLUE) | (1 << KEY_CARD_YELLOW))) == ((1 << KEY_CARD_BLUE) | (1 << KEY_CARD_YELLOW))) {
        u16* from = hud_tilemap_src + (HUD_KEY_Y*HUD_SOURCE_IMAGE_W + HUD_KEY_X + 2);
        u16* to = hud_tilemap_dst + ((HUD_KEY_YP+1)*PLANE_COLUMNS + HUD_KEY_XP);
        COPY_TILEMAP_DATA(from, to, 2, 2);
    }
    // has yellow card but not blue card?
    else if ((keyInventoryBits & ((1 << KEY_CARD_YELLOW) | (1 << KEY_CARD_BLUE))) == ((1 << KEY_CARD_YELLOW) | (0 << KEY_CARD_BLUE))) {
        u16* from = hud_tilemap_src + (HUD_KEY_Y*HUD_SOURCE_IMAGE_W + HUD_KEY_X + 4);
        u16* to = hud_tilemap_dst + ((HUD_KEY_YP+1)*PLANE_COLUMNS + HUD_KEY_XP);
        COPY_TILEMAP_DATA(from, to, 2, 2);
    }
    // has red card?
    if (keyInventoryBits & (1 << KEY_CARD_RED)) {
        u16* from = hud_tilemap_src + (HUD_KEY_Y*HUD_SOURCE_IMAGE_W + HUD_KEY_X + 6);
        u16* to = hud_tilemap_dst + ((HUD_KEY_YP+3)*PLANE_COLUMNS + HUD_KEY_XP);
        COPY_TILEMAP_DATA(from, to, 2, 1);
    }

    // SKULLS

    // has blue skull?
    if (keyInventoryBits & (1 << KEY_SKULL_BLUE)) {
        u16* from = hud_tilemap_src + (HUD_KEY_Y*HUD_SOURCE_IMAGE_W + HUD_KEY_X + 8);
        u16* to = hud_tilemap_dst + (HUD_KEY_YP*PLANE_COLUMNS + HUD_KEY_XP);
        COPY_TILEMAP_DATA(from, to, 2, 2);
    }
    // has blue skull and yellow skull?
    if ((keyInventoryBits & ((1 << KEY_SKULL_BLUE) | (1 << KEY_SKULL_YELLOW))) == ((1 << KEY_SKULL_BLUE) | (1 << KEY_SKULL_YELLOW))) {
        u16* from = hud_tilemap_src + (HUD_KEY_Y*HUD_SOURCE_IMAGE_W + HUD_KEY_X + 10);
        u16* to = hud_tilemap_dst + ((HUD_KEY_YP+1)*PLANE_COLUMNS + HUD_KEY_XP);
        COPY_TILEMAP_DATA(from, to, 2, 2);
    }
    // has yellow skull but not blue skull?
    else if ((keyInventoryBits & ((1 << KEY_SKULL_YELLOW) | (1 << KEY_SKULL_BLUE))) == ((1 << KEY_SKULL_YELLOW) | (0 << KEY_SKULL_BLUE))) {
        u16* from = hud_tilemap_src + (HUD_KEY_Y*HUD_SOURCE_IMAGE_W + HUD_KEY_X + 12);
        u16* to = hud_tilemap_dst + ((HUD_KEY_YP+1)*PLANE_COLUMNS + HUD_KEY_XP);
        COPY_TILEMAP_DATA(from, to, 2, 2);
    }
    // has red skull?
    if (keyInventoryBits & (1 << KEY_SKULL_RED)) {
        u16* from = hud_tilemap_src + (HUD_KEY_Y*HUD_SOURCE_IMAGE_W + HUD_KEY_X + 14);
        u16* to = hud_tilemap_dst + ((HUD_KEY_YP+2)*PLANE_COLUMNS + HUD_KEY_XP);
        COPY_TILEMAP_DATA(from, to, 2, 2);
    }
    // has yellow skull and red skull?
    if ((keyInventoryBits & ((1 << KEY_SKULL_YELLOW) | (1 << KEY_SKULL_RED))) == ((1 << KEY_SKULL_YELLOW) | (1 << KEY_SKULL_RED))) {
        u16* from = hud_tilemap_src + (HUD_KEY_Y*HUD_SOURCE_IMAGE_W + HUD_KEY_X + 16);
        u16* to = hud_tilemap_dst + ((HUD_KEY_YP+2)*PLANE_COLUMNS + HUD_KEY_XP);
        COPY_TILEMAP_DATA(from, to, 2, 1);
    }
}

static FORCE_INLINE void setHUDFace ()
{
    u16 face_Y;
    u16 face_X;

    if (hud_isDead()) {
        face_Y = HUD_FACE_DEAD_Y;
        face_X = HUD_FACE_DEAD_X;
    }
    else {
        face_Y = HUD_FACE_Y;
        if (health_digits.hundrs < 1) {
            u8 tens = health_digits.tens;
            if (tens >= 8)
                face_Y = HUD_FACE_Y + 1*HUD_FACE_H;
            else if (tens >= 5)
                face_Y = HUD_FACE_Y + 2*HUD_FACE_H;
            else if (tens >= 2)
                face_Y = HUD_FACE_Y + 3*HUD_FACE_H;
            else
                face_Y = HUD_FACE_Y + 4*HUD_FACE_H;
        }
        face_X = HUD_FACE_X + faceExpressionCol*HUD_FACE_W;
    }

    u16* from = hud_tilemap_src + (face_Y*HUD_SOURCE_IMAGE_W + face_X);
    u16* to = hud_tilemap_dst + (HUD_FACE_YP*PLANE_COLUMNS + HUD_FACE_XP);
    COPY_TILEMAP_DATA(from, to, HUD_FACE_W, HUD_FACE_H);
}

u16 hud_loadInitialState (u16 currentTileIndex)
{
    // We have already set in resource file the base tile index from which the tileset will be located in VRAM: HUD_VRAM_START_INDEX
    currentTileIndex += img_hud_spritesheet.tileset->numTile;

    // Loads all the tileset at specified VRAM location
	VDP_loadTileSet(img_hud_spritesheet.tileset, HUD_VRAM_START_INDEX, DMA);

    #if HUD_TILEMAP_COMPRESSED
    decompressTilemap();
    hud_tilemap_src = tilemapUnpacked;
    #else
    hud_tilemap_src = img_hud_spritesheet.tilemap->tilemap;
    #endif

    // Loads the HUD background, only once
    setHUDBg();

    hud_resetAmmo(); // default ammo
    hud_resetHealth(); // default health
    hud_resetWeapons(); // default weapons
    hud_resetArmor(); // default armor
    hud_resetKeys(); // default keys
    hud_resetFaceExpression(); // default face

    return currentTileIndex;
}

void hud_setup_hint_pals (u32* palA_addr, u32* palB_addr)
{
    // NOTE: palettes in the resource image are located at the top
    *palA_addr = (u32) (img_hud_spritesheet.palette->data + (0*16) + 1) >> 1;
    *palB_addr = (u32) (img_hud_spritesheet.palette->data + (1*16) + 1) >> 1;
}

FORCE_INLINE u16* hud_getTilemap ()
{
    return hud_tilemap_dst;
}

static FORCE_INLINE void updateFaceExpressionTimer ()
{
    if (faceExpressionTimer > 0) {
        --faceExpressionTimer;
        if (faceExpressionTimer == 0) {
            hud_resetFaceExpression();
        }
    }
}

FORCE_INLINE void hud_update ()
{
    updateFaceExpressionTimer();

    if (updateFlags & (1 << UPDATE_FLAG_AMMO))
        setHUDAmmo();
    if (updateFlags & (1 << UPDATE_FLAG_HEALTH))
        setHUDHealth();
    if (updateFlags & (1 << UPDATE_FLAG_WEAPON))
        setHUDWeapons();
    if (updateFlags & (1 << UPDATE_FLAG_ARMOR))
        setHUDArmor();
    if (updateFlags & (1 << UPDATE_FLAG_KEY))
        setHUDKeys();
    if (updateFlags & (1 << UPDATE_FLAG_FACE))
        setHUDFace();

    // PW_ADDR comes with the correct base position in screen
    if (updateFlags) {
        //DMA_queueDmaFast(DMA_VRAM, hud_tilemap_dst, PW_ADDR, (PLANE_COLUMNS*HUD_BG_H) - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
        hint_enqueueHudTilemap();
        updateFlags = 0;
    }
}