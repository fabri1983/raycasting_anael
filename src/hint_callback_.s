#include "consts.h"
#include "hud_consts.h"

#include "asm_mac.i"

#define VDP_WRITE_VSRAM_ADDR(adr) (((0x4000 + ((adr) & 0x7F)) << 16) + 0x10)
#define _VSRAM_CMD VDP_WRITE_VSRAM_ADDR(0)
#define _HINT_COUNTER 0x8A00 | (HUD_HINT_SCANLINE_MID_SCREEN-2)

.macro macro_hint_callback_LABEL n, m
hint_mirror_planes_callback_asm_LABEL_\n:
    ;// Apply VSCROLL on both planes (writing in one go on both planes)
    move.l  #_VSRAM_CMD,(0xC00004)    ;// VDP_CTRL_PORT: 0: Plane A, 2: Plane B
    .set _ROW_OFFSET, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - \n*((2 << 16) | 2)
    move.l  #_ROW_OFFSET,(0xC00000)   ;// VDP_DATA_PORT: writes on both planes
    move.w  hint_mirror_planes_callback_asm_LABEL_\m,hintCaller+4 ;// SYS_setHIntCallback(hint_mirror_planes_callback_asm_n);
    *addi.w  #30,hintCaller+4          ;// This function block weighs 30 bytes
    rte
.endm

;// Entry point defined as a function. It matches with the first LABEL definition
func hint_mirror_planes_callback_asm_0

;// Addressed as labels: from 0 to HUD_HINT_SCANLINE_MID_SCREEN-1
macro_hint_callback_LABEL 0,1
macro_hint_callback_LABEL 1,2
macro_hint_callback_LABEL 2,3
macro_hint_callback_LABEL 3,4
macro_hint_callback_LABEL 4,5
macro_hint_callback_LABEL 5,6
macro_hint_callback_LABEL 6,7
macro_hint_callback_LABEL 7,8
macro_hint_callback_LABEL 8,9
macro_hint_callback_LABEL 9,10
macro_hint_callback_LABEL 10,11
macro_hint_callback_LABEL 11,12
macro_hint_callback_LABEL 12,13
macro_hint_callback_LABEL 13,14
macro_hint_callback_LABEL 14,15
macro_hint_callback_LABEL 15,16
macro_hint_callback_LABEL 16,17
macro_hint_callback_LABEL 17,18
macro_hint_callback_LABEL 18,19
macro_hint_callback_LABEL 19,20
macro_hint_callback_LABEL 20,21
macro_hint_callback_LABEL 21,22
macro_hint_callback_LABEL 22,23
macro_hint_callback_LABEL 23,24
macro_hint_callback_LABEL 24,25
macro_hint_callback_LABEL 25,26
macro_hint_callback_LABEL 26,27
macro_hint_callback_LABEL 27,28
macro_hint_callback_LABEL 28,29
macro_hint_callback_LABEL 29,30
macro_hint_callback_LABEL 30,31
macro_hint_callback_LABEL 31,32
macro_hint_callback_LABEL 32,33
macro_hint_callback_LABEL 33,34
macro_hint_callback_LABEL 34,35
macro_hint_callback_LABEL 35,36
macro_hint_callback_LABEL 36,37
macro_hint_callback_LABEL 37,38
macro_hint_callback_LABEL 38,39
macro_hint_callback_LABEL 39,40
macro_hint_callback_LABEL 40,41
macro_hint_callback_LABEL 41,42
macro_hint_callback_LABEL 42,43
macro_hint_callback_LABEL 43,44
macro_hint_callback_LABEL 44,45
macro_hint_callback_LABEL 45,46
macro_hint_callback_LABEL 46,47
macro_hint_callback_LABEL 47,48
macro_hint_callback_LABEL 48,49
macro_hint_callback_LABEL 49,50
macro_hint_callback_LABEL 50,51
macro_hint_callback_LABEL 51,52
macro_hint_callback_LABEL 52,53
macro_hint_callback_LABEL 53,54
macro_hint_callback_LABEL 54,55
macro_hint_callback_LABEL 55,56
macro_hint_callback_LABEL 56,57
macro_hint_callback_LABEL 57,58
macro_hint_callback_LABEL 58,59
macro_hint_callback_LABEL 59,60
macro_hint_callback_LABEL 60,61
macro_hint_callback_LABEL 61,62
macro_hint_callback_LABEL 62,63
macro_hint_callback_LABEL 63,64
macro_hint_callback_LABEL 64,65
macro_hint_callback_LABEL 65,66
macro_hint_callback_LABEL 66,67
macro_hint_callback_LABEL 67,68
macro_hint_callback_LABEL 68,69
macro_hint_callback_LABEL 69,70
macro_hint_callback_LABEL 70,71
macro_hint_callback_LABEL 71,72
macro_hint_callback_LABEL 72,73
macro_hint_callback_LABEL 73,74
macro_hint_callback_LABEL 74,75
macro_hint_callback_LABEL 75,76
macro_hint_callback_LABEL 76,77
macro_hint_callback_LABEL 77,78
macro_hint_callback_LABEL 78,79
macro_hint_callback_LABEL 79,80
macro_hint_callback_LABEL 80,81
macro_hint_callback_LABEL 81,82
macro_hint_callback_LABEL 82,83
macro_hint_callback_LABEL 83,84
macro_hint_callback_LABEL 84,85
macro_hint_callback_LABEL 85,86
macro_hint_callback_LABEL 86,87
macro_hint_callback_LABEL 87,88
macro_hint_callback_LABEL 88,89
macro_hint_callback_LABEL 89,90
macro_hint_callback_LABEL 90,91
macro_hint_callback_LABEL 91,92
macro_hint_callback_LABEL 92,93
macro_hint_callback_LABEL 93,94
macro_hint_callback_LABEL 94,95

;// Addressed as labels
.irep n, 95
hint_mirror_planes_callback_asm_LABEL_\n:
    ;// Apply VSCROLL on both planes (writing in one go on both planes)
    move.l  #_VSRAM_CMD,(0xC00004)     ;// VDP_CTRL_PORT: 0: Plane A, 2: Plane B
    .set _ROW_OFFSET, (((VERTICAL_ROWS*8) << 16) | (VERTICAL_ROWS*8)) - \n*((2 << 16) | 2)
    move.l  #_ROW_OFFSET,(0xC00000)    ;// VDP_DATA_PORT: writes on both planes
    ;// Change the HInt counter to the amount of scanlines we want to jump from here. This takes effect next callback, not immediatelly.
    move.w  #_HINT_COUNTER,(0xC00004)  ;// VDP_setHIntCounter(HUD_HINT_SCANLINE_MID_SCREEN-2);
    move.w  hint_mirror_planes_last_scanline_callback,hintCaller+4 ;// SYS_setHIntCallback(hint_mirror_planes_last_scanline_callback);
    rte
.endr