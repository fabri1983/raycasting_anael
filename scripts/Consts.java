public class Consts {
    
    public static final int FS = 8; // fixed point size in bits
    public static final int FP = (1 << FS); // fixed precision
    public static final int AP = 128; // angle precision (optimal for a rotation step of 8 : 1024/8 = 128)
    public static final int STEP_COUNT = 15; // View distance depth. (STEP_COUNT+1 should be a power of two)
    public static final int STEP_COUNT_LOOP = STEP_COUNT;

    // 224 px display height / 8 = 28. Tiles are 8 pixels in height.
    // The HUD takes the bottom 32px / 8 = 4 tiles => 28-4=24
    public static final int VERTICAL_ROWS = 24;

    // 320/8=40. 256/8=32.
    public static final int TILEMAP_COLUMNS = 40;

    // 64 for 320p. 32 for 256p.
    public static final int PLANE_COLUMNS = 64;

    // 320/4=80. 256/4=64.
    public static final int PIXEL_COLUMNS = TILEMAP_COLUMNS * 2;

    public static final int MAP_SIZE = 16;
    public static final int MAP_FRACTION = 32;
    public static final int MIN_POS_XY = (FP + MAP_FRACTION);
    public static final int MAX_POS_XY = (FP * (MAP_SIZE - 1) - MAP_FRACTION);

    public static final int ANGLE_DIR_NORMALIZATION = 24;

    public static final int MAP_HIT_MASK_MAPXY = (16 - 1);
    public static final int MAP_HIT_MASK_SIDEDISTXY = (4096 - 1);
    public static final int MAP_HIT_OFFSET_MAPXY = 0;
    public static final int MAP_HIT_OFFSET_SIDEDISTXY = 4;
    public static final int MAP_HIT_MIN_CALCULATED_INDEX = 174080;

    //---------------------
    // SGDK constants
    //---------------------
    public static final int PAL0 = 0;
    public static final int PAL1 = 1;
    public static final int PAL2 = 2;
    public static final int PAL3 = 3;
    public static final int TILE_ATTR_PALETTE_SFT = 13;
    public static final int TILE_ATTR_VFLIP_MASK = 1 << 12;
    public static final double M_PI = Math.PI;
    public static final int MAX_U8 = 255; // Assuming MAX_U8 as 255 since it's an 8-bit value
    public static final int MAX_U16 = 0xFFFF;
    public static final long MAX_U32 = 0xFFFFFFFFL;
}