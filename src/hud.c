#include <types.h>
#include <vdp.h>
#include <vdp_bg.h>
#include <vdp_tile.h>
#include <dma.h>
#include <pal.h>
#include <memory.h>
#include <mapper.h>
#include "hud_320.h"
#include "hud_res.h"
#include "consts.h"
#include "utils.h"

#if HUD_TILEMAP_COMPRESSED

static u16 unpacked[HUD_IMAGE_W * HUD_IMAGE_H];
static u16* hud_tilemap_src;
static u16 hud_tilemap_dst[PLANE_COLUMNS * HUD_BG_H];

static void decompressTilemap ()
{
    TileMap* src = img_hud_spritesheet.tilemap;
    unpackSelector(src->compression, (u8*) FAR_SAFE(src->tilemap, ((HUD_IMAGE_W) * (HUD_IMAGE_H)) * 2), (u8*) unpacked);
}

#endif

static u16 updateFlags;

static Digits ammo_digits = { .hundrs = 0, .tens = 0, .ones = 0 };
static Digits health_digits = { .hundrs = 0, .tens = 0, .ones = 0 };
static Digits armor_digits = { .hundrs = 0, .tens = 0, .ones = 0 };

static void setDigits (Digits* digits, u8 hundreds, u8 tens, u8 ones)
{
    digits->hundrs = hundreds;
    digits->tens = tens;
    digits->ones = ones;

    // Now accomodate for the empty tile which is always at position 0 for the respective digit
    if (digits->hundrs > 0) {
        digits->hundrs += 1; // offset the empty tile
        digits->tens += 1; // offset the empty tile
    }
    else if (digits->tens > 0)
        digits->tens += 1; // offset the empty tile

    // Ones have no empty tiles
    digits->ones += 1;
}

FORCE_INLINE void resetAmmo ()
{
    updateFlags |= 1 << UPDATE_FLAG_AMMO;
    setAmmo(0, 5, 0);
}

FORCE_INLINE void setAmmo (u8 hundreds, u8 tens, u8 ones)
{
    updateFlags |= 1 << UPDATE_FLAG_AMMO;
    setDigits(&ammo_digits, hundreds, tens, ones);
}

FORCE_INLINE void resetHealth ()
{
    updateFlags |= 1 << UPDATE_FLAG_HEALTH;
    setHealth(1, 0, 0);
}

FORCE_INLINE void setHealth (u8 hundreds, u8 tens, u8 ones)
{
    updateFlags |= 1 << UPDATE_FLAG_HEALTH;
    setDigits(&health_digits, hundreds, tens, ones);
}

FORCE_INLINE void resetArmor ()
{
    updateFlags |= 1 << UPDATE_FLAG_ARMOR;
    setArmor(0, 0, 0);
}

FORCE_INLINE void setArmor (u8 hundreds, u8 tens, u8 ones)
{
    updateFlags |= 1 << UPDATE_FLAG_ARMOR;
    setDigits(&armor_digits, hundreds, tens, ones);
}

static u8 weaponInventoryBits;

FORCE_INLINE void resetWeapons ()
{
    updateFlags |= 1 << UPDATE_FLAG_WEAPON;
    weaponInventoryBits = WEAPON_PISTOL;
}

FORCE_INLINE void addWeapon (u8 weapon)
{
    updateFlags |= 1 << UPDATE_FLAG_WEAPON;
    weaponInventoryBits |= 1 << weapon;
}

static u8 keyInventoryBits;

FORCE_INLINE void resetKeys ()
{
    updateFlags |= 1 << UPDATE_FLAG_KEY;
    keyInventoryBits = KEY_NONE;
}

FORCE_INLINE void addKey (u8 key)
{
    updateFlags |= 1 << UPDATE_FLAG_KEY;
    keyInventoryBits |= 1 << key;
}

static u8 faceExpressionTimer;

static void resetFaceExpressionTimer ()
{
    faceExpressionTimer = 0;
}

static u8 faceExpressionCol;

FORCE_INLINE void resetFaceExpression ()
{
    resetFaceExpressionTimer();
    updateFlags |= 1 << UPDATE_FLAG_FACE;
    faceExpressionCol = FACE_EXPRESSION_CENTERED;
}

FORCE_INLINE void setFaceExpression (u8 expr, u8 timer)
{
    updateFlags |= 1 << UPDATE_FLAG_FACE;
    faceExpressionCol = expr;
    faceExpressionTimer = timer;
}

FORCE_INLINE bool isDead ()
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
        "  .rept (%c[WIDTH])\n" \
        "    move.w   (%0)+,(%1)+\n" \
        "  .endr\n" \
        "    lea     2*%c[FROM_NEXT_ROW](%0),%0\n" \
        "    lea     2*%c[TO_NEXT_ROW](%1),%1\n" \
        ".endr\n" \
        "  .rept (%c[WIDTH])\n" \
        "    move.w   (%0)+,(%1)+\n" \
        "  .endr\n" \
        : "+a" (from), "+a" (to) \
        : [HEIGHT] "i" (TARGET_H), [WIDTH] "i" (TARGET_W), \
          [FROM_NEXT_ROW] "i" (HUD_IMAGE_W - TARGET_W), [TO_NEXT_ROW] "i" (PLANE_COLUMNS - TARGET_W) \
    )

static void setHUDBg ()
{
    // This sets the entire HUD BG tilemap
    u16* from = hud_tilemap_src + (HUD_BG_Y*HUD_IMAGE_W + HUD_BG_X);
    u16* to = hud_tilemap_dst + (HUD_BG_YP*PLANE_COLUMNS + HUD_BG_XP);
    COPY_TILEMAP_DATA(from, to, HUD_BG_W, HUD_BG_H);
}

static void setHUDDigitsCommon (u16 target_XP, u16 target_YP, Digits* digits)
{
    u16* from;
    u16* to;

    // Hundreds
    from = hud_tilemap_src + ((HUD_NUMS_Y + 0*HUD_NUMS_H)*HUD_IMAGE_W + (HUD_NUMS_X + digits->hundrs*HUD_NUMS_W));
    to = hud_tilemap_dst + (target_YP*PLANE_COLUMNS + (target_XP + 0*HUD_NUMS_W));
    COPY_TILEMAP_DATA(from, to, HUD_NUMS_W, HUD_NUMS_H);

    // Tens
    from = hud_tilemap_src + ((HUD_NUMS_Y + 1*HUD_NUMS_H)*HUD_IMAGE_W + (HUD_NUMS_X + digits->tens*HUD_NUMS_W));
    to = hud_tilemap_dst + (target_YP*PLANE_COLUMNS + (target_XP + 1*HUD_NUMS_W));
    COPY_TILEMAP_DATA(from, to, HUD_NUMS_W, HUD_NUMS_H);

    // Ones
    from = hud_tilemap_src + ((HUD_NUMS_Y + 2*HUD_NUMS_H)*HUD_IMAGE_W + (HUD_NUMS_X + digits->ones*HUD_NUMS_W));
    to = hud_tilemap_dst + (target_YP*PLANE_COLUMNS + (target_XP + 2*HUD_NUMS_W));
    COPY_TILEMAP_DATA(from, to, HUD_NUMS_W, HUD_NUMS_H);
}

static void setHUDAmmo ()
{
    updateFlags &= ~(1 << UPDATE_FLAG_AMMO);
    setHUDDigitsCommon(HUD_AMMO_XP, HUD_AMMO_YP, &ammo_digits);
}

static void setHUDHealth ()
{
    updateFlags &= ~(1 << UPDATE_FLAG_HEALTH);
    setHUDDigitsCommon(HUD_HEALTH_XP, HUD_HEALTH_YP, &health_digits);
}

static void setHUDArmor ()
{
    updateFlags &= ~(1 << UPDATE_FLAG_ARMOR);
    setHUDDigitsCommon(HUD_ARMOR_XP, HUD_ARMOR_YP, &armor_digits);
}

static void setHUDWeapons ()
{
    updateFlags &= ~(1 << UPDATE_FLAG_WEAPON);

    // UPPER WEAPONS

    // has shotgun?
    if (weaponInventoryBits & (1 << WEAPON_SHOTGUN)) {
        u16* from = hud_tilemap_src + (HUD_WEAPON_Y*HUD_IMAGE_W + HUD_WEAPON_X);
        u16* to = hud_tilemap_dst + (HUD_WEAPON_HIGH_YP*PLANE_COLUMNS + HUD_WEAPON_HIGH_XP);
        COPY_TILEMAP_DATA(from, to, 1, 2); // dimensions for weapon 3: 1x2 tiles
    }
    // has machine gun?
    if (weaponInventoryBits & (1 << WEAPON_MACHINE_GUN)) {
        u16* from = hud_tilemap_src + (HUD_WEAPON_Y*HUD_IMAGE_W + HUD_WEAPON_X + 1); // jump previous weapons
        u16* to = hud_tilemap_dst + (HUD_WEAPON_HIGH_YP*PLANE_COLUMNS + HUD_WEAPON_HIGH_XP + 1); // jump previous weapons
        COPY_TILEMAP_DATA(from, to, 2, 2); // dimensions for weapon 4: 2x2 tiles
    }

    // LOWER WEAPONS

    if (weaponInventoryBits & (1 << WEAPON_ROCKET)) {
        u16* from = hud_tilemap_src + (HUD_WEAPON_Y*HUD_IMAGE_W + HUD_WEAPON_X + 3); // jump previous weapons
        u16* to = hud_tilemap_dst + (HUD_WEAPON_LOW_YP*PLANE_COLUMNS + HUD_WEAPON_LOW_XP);
        COPY_TILEMAP_DATA(from, to, 2, 2); // dimensions for weapon 5: 2x2 tiles
    }
    // has plasma but not shotgun?
    if ((weaponInventoryBits & ((1 << WEAPON_PLASMA) | (1 << WEAPON_SHOTGUN))) == ((1 << WEAPON_PLASMA) | (0 << WEAPON_SHOTGUN))) {
        u16* from = hud_tilemap_src + (HUD_WEAPON_Y*HUD_IMAGE_W + HUD_WEAPON_X + 5); // jump previous weapons
        u16* to = hud_tilemap_dst + (HUD_WEAPON_LOW_YP*PLANE_COLUMNS + HUD_WEAPON_LOW_XP + 2); // jump previous weapons
        COPY_TILEMAP_DATA(from, to, 1, 2); // dimensions for weapon 6: 1x2 tiles
    }
    // has plasma and shotgun?
    else if ((weaponInventoryBits & ((1 << WEAPON_PLASMA) | (1 << WEAPON_SHOTGUN))) == ((1 << WEAPON_PLASMA) | (1 << WEAPON_SHOTGUN))) {
        u16* from = hud_tilemap_src + (HUD_WEAPON_Y*HUD_IMAGE_W + HUD_WEAPON_X + 6); // jump previous weapons
        u16* to = hud_tilemap_dst + (HUD_WEAPON_LOW_YP*PLANE_COLUMNS + HUD_WEAPON_LOW_XP + 2); // jump previous weapons
        COPY_TILEMAP_DATA(from, to, 1, 2); // dimensions for weapon 6: 1x2 tiles
    }
    // has bfg but not machine gun?
    if ((weaponInventoryBits & ((1 << WEAPON_BFG) | (1 << WEAPON_MACHINE_GUN))) == ((1 << WEAPON_BFG) | (0 << WEAPON_MACHINE_GUN))) {
        u16* from = hud_tilemap_src + (HUD_WEAPON_Y*HUD_IMAGE_W + HUD_WEAPON_X + 7); // jump previous weapons
        u16* to = hud_tilemap_dst + (HUD_WEAPON_LOW_YP*PLANE_COLUMNS + HUD_WEAPON_LOW_XP + 3); // jump previous weapons
        COPY_TILEMAP_DATA(from, to, 2, 2); // dimensions for weapon 7: 2x2 tiles
    }
    // has bfg and machine gun?
    else if ((weaponInventoryBits & ((1 << WEAPON_BFG) | (1 << WEAPON_MACHINE_GUN))) == ((1 << WEAPON_BFG) | (1 << WEAPON_MACHINE_GUN))) {
        u16* from = hud_tilemap_src + (HUD_WEAPON_Y*HUD_IMAGE_W + HUD_WEAPON_X + 9); // jump previous weapons
        u16* to = hud_tilemap_dst + (HUD_WEAPON_LOW_YP*PLANE_COLUMNS + HUD_WEAPON_LOW_XP + 3); // jump previous weapons
        COPY_TILEMAP_DATA(from, to, 2, 2); // dimensions for weapon 7: 2x2 tiles
    }
}

static void setHUDKeys ()
{
    updateFlags &= ~(1 << UPDATE_FLAG_KEY);

    // CARDS

    // has blue card?
    if (keyInventoryBits & (1 << KEY_CARD_BLUE)) {
        u16* from = hud_tilemap_src + (HUD_KEY_Y*HUD_IMAGE_W + HUD_KEY_X);
        u16* to = hud_tilemap_dst + (HUD_KEY_YP*PLANE_COLUMNS + HUD_KEY_XP);
        COPY_TILEMAP_DATA(from, to, 2, 2);
    }
    // has blue card and yellow card?
    if ((keyInventoryBits & ((1 << KEY_CARD_BLUE) | (1 << KEY_CARD_YELLOW))) == ((1 << KEY_CARD_BLUE) | (1 << KEY_CARD_YELLOW))) {
        u16* from = hud_tilemap_src + (HUD_KEY_Y*HUD_IMAGE_W + HUD_KEY_X + 2);
        u16* to = hud_tilemap_dst + ((HUD_KEY_YP+1)*PLANE_COLUMNS + HUD_KEY_XP);
        COPY_TILEMAP_DATA(from, to, 2, 2);
    }
    // has yellow card but not blue card?
    else if ((keyInventoryBits & ((1 << KEY_CARD_YELLOW) | (1 << KEY_CARD_BLUE))) == ((1 << KEY_CARD_YELLOW) | (0 << KEY_CARD_BLUE))) {
        u16* from = hud_tilemap_src + (HUD_KEY_Y*HUD_IMAGE_W + HUD_KEY_X + 4);
        u16* to = hud_tilemap_dst + ((HUD_KEY_YP+1)*PLANE_COLUMNS + HUD_KEY_XP);
        COPY_TILEMAP_DATA(from, to, 2, 2);
    }
    // has red card?
    if (keyInventoryBits & (1 << KEY_CARD_RED)) {
        u16* from = hud_tilemap_src + (HUD_KEY_Y*HUD_IMAGE_W + HUD_KEY_X + 6);
        u16* to = hud_tilemap_dst + ((HUD_KEY_YP+3)*PLANE_COLUMNS + HUD_KEY_XP);
        COPY_TILEMAP_DATA(from, to, 2, 1);
    }

    // SKULLS

    // has blue skull?
    if (keyInventoryBits & (1 << KEY_SKULL_BLUE)) {
        u16* from = hud_tilemap_src + (HUD_KEY_Y*HUD_IMAGE_W + HUD_KEY_X + 8);
        u16* to = hud_tilemap_dst + (HUD_KEY_YP*PLANE_COLUMNS + HUD_KEY_XP);
        COPY_TILEMAP_DATA(from, to, 2, 2);
    }
    // has blue skull and yellow skull?
    if ((keyInventoryBits & ((1 << KEY_SKULL_BLUE) | (1 << KEY_SKULL_YELLOW))) == ((1 << KEY_SKULL_BLUE) | (1 << KEY_SKULL_YELLOW))) {
        u16* from = hud_tilemap_src + (HUD_KEY_Y*HUD_IMAGE_W + HUD_KEY_X + 10);
        u16* to = hud_tilemap_dst + ((HUD_KEY_YP+1)*PLANE_COLUMNS + HUD_KEY_XP);
        COPY_TILEMAP_DATA(from, to, 2, 2);
    }
    // has yellow skull but not blue skull?
    else if ((keyInventoryBits & ((1 << KEY_SKULL_YELLOW) | (1 << KEY_SKULL_BLUE))) == ((1 << KEY_SKULL_YELLOW) | (0 << KEY_SKULL_BLUE))) {
        u16* from = hud_tilemap_src + (HUD_KEY_Y*HUD_IMAGE_W + HUD_KEY_X + 12);
        u16* to = hud_tilemap_dst + ((HUD_KEY_YP+1)*PLANE_COLUMNS + HUD_KEY_XP);
        COPY_TILEMAP_DATA(from, to, 2, 2);
    }
    // has red skull?
    if (keyInventoryBits & (1 << KEY_SKULL_RED)) {
        u16* from = hud_tilemap_src + (HUD_KEY_Y*HUD_IMAGE_W + HUD_KEY_X + 14);
        u16* to = hud_tilemap_dst + ((HUD_KEY_YP+2)*PLANE_COLUMNS + HUD_KEY_XP);
        COPY_TILEMAP_DATA(from, to, 2, 2);
    }
    // has yellow skull and red skull?
    if ((keyInventoryBits & ((1 << KEY_SKULL_YELLOW) | (1 << KEY_SKULL_RED))) == ((1 << KEY_SKULL_YELLOW) | (1 << KEY_SKULL_RED))) {
        u16* from = hud_tilemap_src + (HUD_KEY_Y*HUD_IMAGE_W + HUD_KEY_X + 16);
        u16* to = hud_tilemap_dst + ((HUD_KEY_YP+2)*PLANE_COLUMNS + HUD_KEY_XP);
        COPY_TILEMAP_DATA(from, to, 2, 1);
    }
}

static void setHUDFace ()
{
    updateFlags &= ~(1 << UPDATE_FLAG_FACE);

    u16 face_Y;
    u16 face_X;

    if (isDead()) {
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

    u16* from = hud_tilemap_src + (face_Y*HUD_IMAGE_W + face_X);
    u16* to = hud_tilemap_dst + (HUD_FACE_YP*PLANE_COLUMNS + HUD_FACE_XP);
    COPY_TILEMAP_DATA(from, to, HUD_FACE_W, HUD_FACE_H);
}

int loadInitialHUD (u16 currentTileIndex)
{
    // We have already set in resource file the base tile index from which the tileset will be located in VRAM: HUD_VRAM_START_INDEX
    currentTileIndex += img_hud_spritesheet.tileset->numTile;

    // Loads all the tileset at specified VRAM location
	VDP_loadTileSet(img_hud_spritesheet.tileset, HUD_VRAM_START_INDEX, DMA);

    #if HUD_TILEMAP_COMPRESSED
    decompressTilemap();
    hud_tilemap_src = unpacked;
    #else
    hud_tilemap_src = img_hud_spritesheet.tilemap->tilemap;
    #endif

    // Loads the HUD background, only once
    setHUDBg();

    resetAmmo();
    resetHealth();
    resetWeapons();
    resetArmor();
    resetKeys();
    resetFaceExpression();

    updateHUD();

    return currentTileIndex;
}

static FORCE_INLINE void updateFaceExpressionTimer ()
{
    if (faceExpressionTimer > 0) {
        --faceExpressionTimer;
        if (faceExpressionTimer == 0) {
            resetFaceExpression();
        }
    }
}

void updateHUD ()
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
    DMA_queueDmaFast(DMA_VRAM, hud_tilemap_dst, PW_ADDR, (PLANE_COLUMNS*HUD_BG_H) - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
}

static u32 pal0_addrForDMA;
static u32 pal1_addrForDMA;

void prepareHUDPalAddresses ()
{
    pal0_addrForDMA = (u32) (img_hud_spritesheet.palette->data + 1) >> 1;
    pal1_addrForDMA = (u32) (img_hud_spritesheet.palette->data + 16 + 1) >> 1;
}

HINTERRUPT_CALLBACK hIntCallbackHud ()
{
	if (GET_VCOUNTER <= HINT_SCANLINE_CHANGE_BG_COLOR) {
		waitHCounter_DMA(156);
		// set background color used for the floor
		PAL_setColor(0, palette_grey[2]);
		return;
	}

	// We are one scanline earlier so wait until entering th HBlank region
	waitHCounter_DMA(152);

	// Prepare DMA cmd and source address for first palette
    u32 palCmd = VDP_DMA_CRAM_ADDR((PAL0 * 16 + 1) * 2); // target starting color index multiplied by 2
    u32 fromAddrForDMA = pal0_addrForDMA;

    // This seems to take an entire scanline, if you turn off VDP then it will draw black scanline (or whatever the BG color is) as a way off measure.
    setupDMAForPals(15, fromAddrForDMA);

    // At this moment we are at the middle/end of the scanline due to the previous DMA setup.
    // So we need to wait for next HBlank (indeed some pixels before to absorb some overhead)
    waitHCounter_DMA(152);

    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
    turnOnVDP(0x74);

	// Prepare DMA cmd and source address for second palette
    palCmd = VDP_DMA_CRAM_ADDR((PAL1 * 16 + 1) * 2); // target starting color index multiplied by 2
    fromAddrForDMA = pal1_addrForDMA;

	// This seems to take an entire scanline, if you turn off VDP then it will draw black scanline (or whatever the BG color is) as a way off measure.
    setupDMAForPals(15, fromAddrForDMA);

	// At this moment we are at the middle/end of the scanline due to the previous DMA setup.
    // So we need to wait for next HBlank (indeed some pixels before to absorb some overhead)
    waitHCounter_DMA(152);

    turnOffVDP(0x74);
    *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
    turnOnVDP(0x74);

	// set background color used for the roof
	// THIS INTRODUCES A COLOR DOT GLITCH BARELY NOTICABLE. SOLUTION IS TO DO THIS IN VBLANK CALLBACK
	//PAL_setColor(0, palette_grey[1]);

	// Send 1/2 of the PA
	//DMA_doDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, (VERTICAL_ROWS*PLANE_COLUMNS)/2 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);
	// Send 1/4 of the PA
	//DMA_doDmaFast(DMA_VRAM, frame_buffer, PA_ADDR, (VERTICAL_ROWS*PLANE_COLUMNS)/4 - (PLANE_COLUMNS-TILEMAP_COLUMNS), 2);

	// Reload the first 2 palettes that were overriden by the HUD palettes
	// waitVCounterReg(224);
	// // grey palette
	// palCmd = VDP_DMA_CRAM_ADDR((PAL0 * 16 + 1) * 2); // target starting color index multiplied by 2
    // fromAddrForDMA = (u32) (palette_grey + 1) >> 1; // TODO: this can be set outside the HInt (or maybe the compiler does it already)
	// turnOffVDP(0x74);
    // setupDMAForPals(8, fromAddrForDMA);
    // *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
	// // red palette
	// palCmd = VDP_DMA_CRAM_ADDR((PAL1 * 16 + 1) * 2); // target starting color index multiplied by 2
    // fromAddrForDMA = (u32) (palette_red + 1) >> 1; // TODO: this can be set outside the HInt (or maybe the compiler does it already)
    // setupDMAForPals(8, fromAddrForDMA);
    // *((vu32*) VDP_CTRL_PORT) = palCmd; // trigger DMA transfer
	// turnOnVDP(0x74);
}