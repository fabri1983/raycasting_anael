#include "asm_mac.i"

;// 896*2=1792
#define FRAMEBUFFER_SIZE 896*2 
;// 1792 translated into bytes -> 3584
#define TOTAL_BYTES FRAMEBUFFER_SIZE*2

func clear_buffer
    move.l 4(sp),a0           ;// a0 = buffer
    lea TOTAL_BYTES(a0),a0    ;// a0 = buffer end

    movem.l d2-d7/a2-a6,-(sp)

    moveq  #0,d0
    move.l d0,d1
    move.l d0,d2
    move.l d0,d3
    move.l d0,d4
    move.l d0,d5
    move.l d0,d6
    move.l d0,d7
    move.l d0,a1
    move.l d0,a2
    move.l d0,a3
    move.l d0,a4
    move.l d0,a5
    move.l d0,a6

    ;// Clear all the bytes by using 14 registers with long word access: 3584/(14*4)=64 movem ops.
    ;// NOTE: if reminder from the division isn't 0 you need to add the missing operations.
    .rept (TOTAL_BYTES/(14*4))
    movem.l d0-d7/a1-a6,-(a0)
    .endr

    movem.l (sp)+,d2-d7/a2-a6
    rts