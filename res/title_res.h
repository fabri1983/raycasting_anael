#include <genesis.h>

#ifndef _RES_TITLE_RES_H_
#define _RES_TITLE_RES_H_

typedef struct {
    u16 numTile;
    u32* tiles;
} TileSetOriginalCustom;

typedef struct {
    u16 w;
    u16 h;
    u16* tilemap;
} TileMapOriginalCustom;

typedef struct {
    u16* tilemap;
} TileMapCustom;

typedef struct {
    u16 compression;
    u16* tilemap;
} TileMapCustomCompField;

typedef struct {
    TileSetOriginalCustom* tileset;
    TileMapOriginalCustom* tilemap;
} ImageNoPals;

typedef struct {
    TileSet* tileset;
    TileMap* tilemap;
} ImageNoPalsCompField;

typedef struct {
    TileSetOriginalCustom* tileset1;
    TileSetOriginalCustom* tileset2;
    TileMapCustom* tilemap1;
} ImageNoPalsSplit21;

typedef struct {
    TileSet* tileset1;
    TileSet* tileset2;
    TileMapCustom* tilemap1;
    TileMapCustom* tilemap2;
} ImageNoPalsSplit22;

typedef struct {
    TileSetOriginalCustom* tileset1;
    TileSetOriginalCustom* tileset2;
    TileSetOriginalCustom* tileset3;
    TileMapCustom* tilemap1;
} ImageNoPalsSplit31;

typedef struct {
    TileSetOriginalCustom* tileset1;
    TileSetOriginalCustom* tileset2;
    TileSetOriginalCustom* tileset3;
    TileMapCustom* tilemap1;
    TileMapCustom* tilemap2;
} ImageNoPalsSplit32;

typedef struct {
    TileSet* tileset1;
    TileSet* tileset2;
    TileSet* tileset3;
    TileMapCustom* tilemap1;
    TileMapCustom* tilemap2;
    TileMapCustom* tilemap3;
} ImageNoPalsSplit33;

typedef struct {
    TileSet* tileset1;
    TileSet* tileset2;
    TileMapCustom* tilemap1;
} ImageNoPalsSplit21CompField;

typedef struct {
    TileSet* tileset1;
    TileSet* tileset2;
    TileMapCustomCompField* tilemap1;
    TileMapCustomCompField* tilemap2;
} ImageNoPalsSplit22CompField;

typedef struct {
    TileSet* tileset1;
    TileSet* tileset2;
    TileSet* tileset3;
    TileMapCustomCompField* tilemap1;
} ImageNoPalsSplit31CompField;

typedef struct {
    TileSet* tileset1;
    TileSet* tileset2;
    TileSet* tileset3;
    TileMapCustomCompField* tilemap1;
    TileMapCustomCompField* tilemap2;
} ImageNoPalsSplit32CompField;

typedef struct {
    TileSet* tileset1;
    TileSet* tileset2;
    TileSet* tileset3;
    TileMapCustomCompField* tilemap1;
    TileMapCustomCompField* tilemap2;
    TileMapCustomCompField* tilemap3;
} ImageNoPalsSplit33CompField;

typedef struct {
    u16* data;
} Palette16;

typedef struct {
    u16* data;
} Palette32;

typedef struct {
    u16* data;
} Palette16AllStrips;

typedef struct {
    u16* data1;
    u16* data2;
} Palette16AllStripsSplit2;

typedef struct {
    u16* data1;
    u16* data2;
    u16* data3;
} Palette16AllStripsSplit3;

typedef struct {
    u16 compression;
    u16* data;
} Palette16AllStripsCompField;

typedef struct {
    u16 compression;
    u16* data1;
    u16* data2;
} Palette16AllStripsSplit2CompField;

typedef struct {
    u16 compression;
    u16* data1;
    u16* data2;
    u16* data3;
} Palette16AllStripsSplit3CompField;

typedef struct {
    u16* data;
} Palette32AllStrips;

typedef struct {
    u16* data1;
    u16* data2;
} Palette32AllStripsSplit2;

typedef struct {
    u16* data1;
    u16* data2;
    u16* data3;
} Palette32AllStripsSplit3;

typedef struct {
    u16 compression;
    u16* data;
} Palette32AllStripsCompField;

typedef struct {
    u16 compression;
    u16* data1;
    u16* data2;
} Palette32AllStripsSplit2CompField;

typedef struct {
    u16 compression;
    u16* data1;
    u16* data2;
    u16* data3;
} Palette32AllStripsSplit3CompField;

extern const Image img_title_logo;
extern const ImageNoPalsCompField img_title_bg_full;
extern const Palette32AllStrips pal_title_bg_full;
extern const Palette32AllStrips pal_title_bg_full_shifted_0;
extern const Palette32AllStrips pal_title_bg_full_shifted_1;
extern const Palette32AllStrips pal_title_bg_full_shifted_2;
extern const Palette32AllStrips pal_title_bg_full_shifted_3;
extern const Palette32AllStrips pal_title_bg_full_shifted_4;
extern const Palette32AllStrips pal_title_bg_full_shifted_5;
extern const Palette32AllStrips pal_title_bg_full_shifted_6;
extern const Palette32AllStrips pal_title_bg_full_shifted_7;
extern const Palette32AllStrips pal_title_bg_full_shifted_8;
extern const Palette32AllStrips pal_title_bg_full_shifted_9;
extern const Palette32AllStrips pal_title_bg_full_shifted_10;
extern const Palette32AllStrips pal_title_bg_full_shifted_11;
extern const Palette32AllStrips pal_title_bg_full_shifted_12;
extern const Palette32AllStrips pal_title_bg_full_shifted_13;
extern const Palette32AllStrips pal_title_bg_full_shifted_14;
extern const Palette32AllStrips pal_title_bg_full_shifted_15;
extern const Palette32AllStrips pal_title_bg_full_shifted_16;
extern const Palette32AllStrips pal_title_bg_full_shifted_17;
extern const Palette32AllStrips pal_title_bg_full_shifted_18;
extern const Palette32AllStrips pal_title_bg_full_shifted_19;
extern const Palette32AllStrips pal_title_bg_full_shifted_20;
extern const Palette32AllStrips pal_title_bg_full_shifted_21;
extern const Palette32AllStrips pal_title_bg_full_shifted_22;
extern const Palette32AllStrips pal_title_bg_full_shifted_23;
extern const Palette32AllStrips pal_title_bg_full_shifted_24;
extern const Palette32AllStrips pal_title_bg_full_shifted_25;
extern const Palette32AllStrips pal_title_bg_full_shifted_26;
extern const Palette32AllStrips pal_title_bg_full_shifted_27;
extern const Palette32AllStrips pal_title_bg_full_shifted_28;
extern const Palette32AllStrips pal_title_bg_full_shifted_29;
extern const Palette32AllStrips pal_title_bg_full_shifted_30;
extern const Palette32AllStrips pal_title_bg_full_shifted_31;
extern const Palette32AllStrips pal_title_bg_full_shifted_32;
extern const Palette32AllStrips pal_title_bg_full_shifted_33;
extern const Palette32AllStrips pal_title_bg_full_shifted_34;
extern const Palette32AllStrips pal_title_bg_full_shifted_35;
extern const Palette32AllStrips pal_title_bg_full_shifted_36;
extern const Palette32AllStrips pal_title_bg_full_shifted_37;
extern const Palette32AllStrips pal_title_bg_full_shifted_38;
extern const Palette32AllStrips pal_title_bg_full_shifted_39;
extern const Palette32AllStrips pal_title_bg_full_shifted_40;
extern const Palette32AllStrips pal_title_bg_full_shifted_41;
extern const Palette32AllStrips pal_title_bg_full_shifted_42;
extern const Palette32AllStrips pal_title_bg_full_shifted_43;
extern const Palette32AllStrips pal_title_bg_full_shifted_44;
extern const Palette32AllStrips pal_title_bg_full_shifted_45;
extern const Palette32AllStrips pal_title_bg_full_shifted_46;
extern const Palette32AllStrips pal_title_bg_full_shifted_47;
extern const Palette32AllStrips pal_title_bg_full_shifted_48;
extern const Palette32AllStrips pal_title_bg_full_shifted_49;
extern const Palette32AllStrips pal_title_bg_full_shifted_50;
extern const Palette32AllStrips pal_title_bg_full_shifted_51;
extern const Palette32AllStrips pal_title_bg_full_shifted_52;
extern const Palette32AllStrips pal_title_bg_full_shifted_53;
extern const Palette32AllStrips pal_title_bg_full_shifted_54;
extern const Palette32AllStrips pal_title_bg_full_shifted_55;
extern const Palette32AllStrips pal_title_bg_full_shifted_56;

#endif // _RES_TITLE_RES_H_
