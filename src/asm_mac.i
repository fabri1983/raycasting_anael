.macro func _name, _align=2
    .section .text.asm.\_name
    .globl  \_name
    .type   \_name, @function
    .align \_align
  \_name:
.endm

  adda.w	d0,d0
  move.w	d0,d1
  add.w	d0,d0
  add.w	d1,d0
