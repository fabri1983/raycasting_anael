;// ** Makro MULUQW \multiplicator \src \scratch **
;// ---------------------------
;// \multiplicator as a constant
;// \src      rx1.w source operand and result of the multiplication
;//           (can be an address register if there is no shift command used, but
;//           should be a data register for a faster execution on the MC68000)
;// \scratch  rx2.w scratch register
;//           (not needed, if the multiplicator is only a multiple of 2)
;//           (can be an address register if there is no shift command used, but
;//           should be a data register for a faster execution on the MC68000)
.macro MULUQW multiplicator, src, scratch
  .if \multiplicator==2                ;//*2
    add.w   \src,\src
  .endif
  .if \multiplicator==3                ;//*3
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \scratch,\src
  .endif
  .if \multiplicator==4                ;//*4
    add.w   \src,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==5                ;//*5
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \src,\src
    add.w   \scratch,\src
  .endif
  .if \multiplicator==6                ;//*6
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \scratch,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==7                ;//*7
    move.w  \src,\scratch
    lsl.w   #3,\src
    sub.w   \scratch,\src
  .endif
  .if \multiplicator==8                ;//*8
    lsl.w   #3,\src
  .endif
  .if \multiplicator==9                ;//*9
    move.w  \src,\scratch
    lsl.w   #3,\src
    add.w   \scratch,\src
  .endif
  .if \multiplicator==10               ;//*10
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \src,\src
    add.w   \scratch,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==11               ;//*11
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \src,\src
    add.w   \scratch,\src
    add.w   \src,\src
    add.w   \scratch,\src
  .endif
  .if \multiplicator==12               ;//*12
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \scratch,\src
    add.w   \src,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==13               ;//*13
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \scratch,\src
    add.w   \src,\src
    add.w   \src,\src
    add.w   \scratch,\src
  .endif
  .if \multiplicator==14               ;//*14
    move.w  \src,\scratch
    lsl.w   #3,\src
    sub.w   \scratch,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==15               ;//*15
    move.w  \src,\scratch
    lsl.w   #4,\src
    sub.w   \scratch,\src
  .endif
  .if \multiplicator==16               ;//*16
    lsl.w   #4,\src
  .endif
  .if \multiplicator==17               ;//*17
    move.w  \src,\scratch
    lsl.w   #4,\src
    add.w   \scratch,\src
  .endif
  .if \multiplicator==18               ;//*18
    move.w  \src,\scratch
    lsl.w   #3,\src
    add.w   \scratch,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==20               ;//*20
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \src,\src
    add.w   \scratch,\src
    add.w   \src,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==23               ;//*23
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #3,\src
    sub.w   \scratch,\src
  .endif
  .if \multiplicator==24               ;//*24
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #3,\src
  .endif
  .if \multiplicator==28               ;//*28
    move.w  \src,\scratch
    lsl.w   #3,\src
    sub.w   \scratch,\src
    add.w   \src,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==29               ;//*29
    move.w \src,\scratch
    add.w  \src,\src
    add.w  \src,\scratch
    lsl.w  #4,\src
    sub.w  \scratch,\src
  .endif
  .if \multiplicator==30               ;//*30
    move.w  \src,\scratch
    lsl.w   #4,\src
    sub.w   \scratch,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==31               ;//*31
    move.w  \src,\scratch
    lsl.w   #5,\src
    sub.w   \scratch,\src
  .endif
  .if \multiplicator==32               ;//*32
    lsl.w   #5,\src
  .endif
  .if \multiplicator==33               ;//*33
    move.w  \src,\scratch
    lsl.w   #5,\src
    add.w   \scratch,\src
  .endif
  .if \multiplicator==34               ;//*34
    move.w  \src,\scratch
    lsl.w   #4,\src
    add.w   \scratch,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==35               ;//*35
    move.w  \src,\scratch
    lsl.w   #4,\src
    add.w   \scratch,\src
    add.w   \src,\src
    add.w   \scratch,\src
  .endif
  .if \multiplicator==36               ;//*36
    move.w  \src,\scratch
    lsl.w   #3,\src
    add.w   \scratch,\src
    add.w   \src,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==40               ;//*40
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #3,\src
  .endif
  .if \multiplicator==41               ;//*41
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #3,\src
    add.w   \scratch,\src
  .endif
  .if \multiplicator==44               ;//*44
    move.w  \src,\scratch
    add.w   \scratch,\scratch
    add.w   \scratch,\src
    lsl.w   #4,\src
    add.w   \scratch,\scratch
    sub.w   \scratch,\src
  .endif
  .if \multiplicator==45               ;//*45
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \src,\scratch
    move.w  \scratch,\src
    lsl.w   #4,\src
    sub.w   \scratch,\src
  .endif
  .if \multiplicator==46               ;//*46
    move.w  \src,\scratch
    add.w   \scratch,\scratch
    add.w   \scratch,\src
    lsl.w   #4,\src
    sub.w   \scratch,\src
  .endif
  .if \multiplicator==48               ;//*48
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #4,\src
  .endif
  .if \multiplicator==49               ;//*49
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #4,\src
    add.w   \scratch,\src
  .endif
  .if \multiplicator==56               ;//*56
    move.w  \src,\scratch
    lsl.w   #3,\src
    sub.w   \scratch,\src
    lsl.w   #3,\src
  .endif
  .if \multiplicator==60               ;//*60
    move.w  \src,\scratch
    lsl.w   #4,\src
    sub.w   \scratch,\src
    add.w   \src,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==62               ;//*62
    move.w  \src,\scratch
    lsl.w   #5,\src
    sub.w   \scratch,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==63               ;//*63
    move.w  \src,\scratch
    lsl.w   #6,\src
    sub.w   \scratch,\src
  .endif
  .if \multiplicator==64               ;//*64
    lsl.w   #6,\src
  .endif
  .if \multiplicator==65               ;//*65
    move.w  \src,\scratch
    lsl.w   #6,\src
    add.w   \scratch,\src
  .endif
  .if \multiplicator==66               ;//*66
    move.w  \src,\scratch
    lsl.w   #5,\src
    add.w   \scratch,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==68               ;//*68
    move.w  \src,\scratch
    lsl.w   #4,\src
    add.w   \scratch,\src
    add.w   \src,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==72               ;//*72
    move.w  \src,\scratch
    lsl.w   #3,\src
    add.w   \scratch,\src
    lsl.w   #3,\src
  .endif
  .if \multiplicator==80               ;//*80
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #4,\src
  .endif
  .if \multiplicator==84               ;//*84
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \src,\src
    add.w   \scratch,\src
    add.w   \src,\src
    add.w   \src,\src
    add.w   \scratch,\src
    add.w   \src,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==92               ;//*92
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #3,\src
    sub.w   \scratch,\src
    add.w   \src,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==96               ;//*96
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #5,\src
  .endif
  .if \multiplicator==112              ;//*112
    move.w  \src,\scratch
    lsl.w   #3,\src
    sub.w   \scratch,\src
    lsl.w   #4,\src
  .endif
  .if \multiplicator==120              ;//*120
    move.w  \src,\scratch
    lsl.w   #4,\src
    sub.w   \scratch,\src
    lsl.w   #3,\src
  .endif
  .if \multiplicator==124              ;//*124
    move.w  \src,\scratch
    lsl.w   #5,\src
    sub.w   \scratch,\src
    add.w   \src,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==126              ;//*126
    move.w  \src,\scratch
    lsl.w   #6,\src
    sub.w   \scratch,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==127              ;//*127
    move.w  \src,\scratch
    lsl.w   #7,\src
    sub.w   \scratch,\src
  .endif
  .if \multiplicator==128              ;//*128
    lsl.w   #7,\src
  .endif
  .if \multiplicator==129              ;//*129
    move.w  \src,\scratch
    lsl.w   #7,\src
    add.w   \scratch,\src
  .endif
  .if \multiplicator==130              ;//*130
    move.w  \src,\scratch
    lsl.w   #6,\src
    add.w   \scratch,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==132              ;//*132
    move.w  \src,\scratch
    lsl.w   #5,\src
    add.w   \scratch,\src
    add.w   \src,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==136              ;//*136
    move.w  \src,\scratch
    lsl.w   #4,\src
    add.w   \scratch,\src
    lsl.w   #3,\src
  .endif
  .if \multiplicator==144              ;//*144
    move.w  \src,\scratch
    lsl.w   #3,\src
    add.w   \scratch,\src
    lsl.w   #4,\src
  .endif
  .if \multiplicator==156              ;//*156
    move.w \src,\scratch
    add.w  \scratch,\scratch
    add.w  \scratch,\scratch
    add.w  \scratch,\src
    lsl.w  #5,\src
    sub.w  \scratch,\src
  .endif
  .if \multiplicator==160              ;//*160
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #5,\src
  .endif
  .if \multiplicator==184              ;//*184
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #3,\src
    sub.w   \scratch,\src
    lsl.w   #3,\src
  .endif
  .if \multiplicator==192              ;//*192
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #6,\src
  .endif
  .if \multiplicator==196              ;//*196
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #4,\src
    add.w   \scratch,\src
    add.w   \src,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==200              ;//*200
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #3,\src
    add.w   \scratch,\src
    lsl.w   #3,\src
  .endif
  .if \multiplicator==208              ;//*208
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \scratch,\src
    add.w   \src,\src
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #4,\src
  .endif
  .if \multiplicator==224              ;//*224
    move.w  \src,\scratch
    lsl.w   #3,\src
    sub.w   \scratch,\src
    lsl.w   #5,\src
  .endif
  .if \multiplicator==240              ;//*240
    move.w  \src,\scratch
    lsl.w   #4,\src
    sub.w   \scratch,\src
    lsl.w   #4,\src
  .endif
  .if \multiplicator==248              ;//*248
    move.w  \src,\scratch
    lsl.w   #5,\src
    sub.w   \scratch,\src
    lsl.w   #3,\src
  .endif
  .if \multiplicator==252              ;//*252
    move.w  \src,\scratch
    lsl.w   #6,\src
    sub.w   \scratch,\src
    add.w   \src,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==254              ;//*254
    move.w  \src,\scratch
    lsl.w   #7,\src
    sub.w   \scratch,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==255              ;//*255
    move.w  \src,\scratch
    lsl.w   #8,\src
    sub.w   \scratch,\src
  .endif
  .if \multiplicator==256              ;//*256
    lsl.w   #8,\src
  .endif
  .if \multiplicator==257              ;//*257
    move.w  \src,\scratch
    lsl.w   #8,\src
    add.w   \scratch,\src
  .endif
  .if \multiplicator==258              ;//*258
    move.w  \src,\scratch
    lsl.w   #7,\src
    add.w   \scratch,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==260              ;//*260
    move.w  \src,\scratch
    lsl.w   #6,\src
    add.w   \scratch,\src
    add.w   \src,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==264              ;//*264
    move.w  \src,\scratch
    lsl.w   #5,\src
    add.w   \scratch,\src
    lsl.w   #3,\src
  .endif
  .if \multiplicator==272              ;//*272
    move.w  \src,\scratch
    lsl.w   #4,\src
    add.w   \scratch,\src
    lsl.w   #4,\src
  .endif
  .if \multiplicator==288              ;//*288
    move.w  \src,\scratch
    lsl.w   #3,\src
    add.w   \scratch,\src
    lsl.w   #5,\src
  .endif
  .if \multiplicator==304              ;//*304
    move.w  \src,\scratch
    lsl.w   #3,\src
    add.w   \scratch,\src
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #4,\src
  .endif
  .if \multiplicator==320              ;//*320
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #6,\src
  .endif
  .if \multiplicator==384              ;//*384
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #7,\src
  .endif
  .if \multiplicator==400              ;//*400
    move.w  \src,\scratch
    lsl.w   #3,\src
    add.w   \src,\scratch
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #4,\src
  .endif
  .if \multiplicator==416              ;//*416
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \src,\src
    add.w   \src,\scratch
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #5,\src
  .endif
  .if \multiplicator==480              ;//*480
    move.w  \src,\scratch
    lsl.w   #4,\src
    sub.w   \scratch,\src
    lsl.w   #5,\src
  .endif
  .if \multiplicator==512              ;//*512
    lsl.w   #8,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==576              ;//*576
    move.w  \src,\scratch
    lsl.w   #3,\src
    add.w   \scratch,\src
    lsl.w   #6,\src
  .endif
  .if \multiplicator==608              ;//*608
    move.w  \src,\scratch
    lsl.w   #3,\src
    add.w   \scratch,\src
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #5,\src
  .endif
  .if \multiplicator==624              ;//*624
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #3,\src
    sub.w   \scratch,\src
    lsl.w   #4,\src
  .endif
  .if \multiplicator==625              ;//*625
    move.w  \src,\scratch
    lsl.w   #4,\src
    sub.w   \src,\scratch
    lsl.w   #3,\src
    add.w   \src,\scratch
    add.w   \src,\src
    add.w   \src,\src
    add.w   \scratch,\src
  .endif
  .if \multiplicator==640              ;//*640
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #7,\src
  .endif
  .if \multiplicator==768              ;//*768
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #8,\src
  .endif
  .if \multiplicator==896              ;//*896
    move.w  \src,\scratch
    lsl.w   #3,\src
    sub.w   \scratch,\src
    lsl.w   #7,\src
  .endif
  .if \multiplicator==960              ;//*960
    move.w  \src,\scratch
    lsl.w   #4,\src
    sub.w   \scratch,\src
    lsl.w   #6,\src
  .endif
  .if \multiplicator==1024             ;//*1024
    lsl.w   #8,\src
    add.w   \src,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==1280             ;//*1280
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #8,\src
  .endif
  .if \multiplicator==1920             ;//*1920
    move.w  \src,\scratch
    lsl.w   #4,\src
    sub.w   \scratch,\src
    lsl.w   #7,\src
  .endif
  .if \multiplicator==2048             ;//*2048
    lsl.w   #8,\src
    lsl.w   #3,\src
  .endif
  .if \multiplicator==2560             ;//*2560
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #8,\src
    add.w   \src,\src
  .endif
  .if \multiplicator==3072             ;//*3072
    move.w  \src,\scratch
    add.w   \src,\src
    add.w   \scratch,\src
    lsl.w   #8,\src
    add.w   \src,\src
    add.w   \src,\src
  .endif
.endm

;// ** Makro MULUQL \multiplicator \src \scratch **
;// ---------------------------
;// \multiplicator as a constant
;// \src      rx1.w source operand and result of the multiplication
;//           (can be an address register if there is no shift command used)
;// \scratch  rx2.w scratch register
;//           (not needed, if the multiplicator is only a multiple of 2)
;//           (can be an address register if there is no shift command used)
.macro MULUQL multiplicator, src, scratch
  .if \multiplicator==2                ;//*2
    add.l   \src,\src
  .endif
  .if \multiplicator==3                ;//*3
    move.l  \src,\scratch
    add.l   \src,\src
    add.l   \scratch,\src
  .endif
  .if \multiplicator==4                ;//*4
    lsl.l   #2,\src
  .endif
  .if \multiplicator==5                ;//*5
    move.l  \src,\scratch
    lsl.l   #2,\src
    add.l   \scratch,\src
  .endif
  .if \multiplicator==6                ;//*6
    move.l  \src,\scratch
    add.l   \src,\src
    add.l   \scratch,\src
    add.l   \src,\src
  .endif
  .if \multiplicator==7                ;//*7
    move.l  \src,\scratch
    lsl.l   #3,\src
    sub.l   \scratch,\src
  .endif
  .if \multiplicator==8                ;//*8
    lsl.l   #3,\src
  .endif
  .if \multiplicator==9                ;//*9
    move.l  \src,\scratch
    lsl.l   #3,\src
    add.l   \scratch,\src
  .endif
  .if \multiplicator==10               ;//*10
    move.l  \src,\scratch
    lsl.l   #2,\src
    add.l   \scratch,\src
    add.l   \src,\src
  .endif
  .if \multiplicator==11               ;//*11
    move.l  \src,\scratch
    lsl.l   #2,\src
    add.l   \scratch,\src
    add.l   \src,\src
    add.l   \scratch,\src
  .endif
  .if \multiplicator==12               ;//*12
    move.l  \src,\scratch
    add.l   \src,\src
    add.l   \scratch,\src
    lsl.l   #2,\src
  .endif
  .if \multiplicator==13               ;//*13
    move.l  \src,\scratch
    add.l   \src,\src
    add.l   \scratch,\src
    lsl.l   #2,\src
    add.l   \scratch,\src
  .endif
  .if \multiplicator==14               ;//*14
    move.l  \src,\scratch
    lsl.l   #3,\src
    sub.l   \scratch,\src
    add.l   \src,\src
  .endif
  .if \multiplicator==15               ;//*15
    move.l  \src,\scratch
    lsl.l   #4,\src
    sub.l   \scratch,\src
  .endif
  .if \multiplicator==16               ;//*16
    lsl.l   #4,\src
  .endif
  .if \multiplicator==17               ;//*17
    move.l  \src,\scratch
    lsl.l   #4,\src
    add.l   \scratch,\src
  .endif
  .if \multiplicator==18               ;//*18
    move.l  \src,\scratch
    lsl.l   #3,\src
    add.l   \scratch,\src
    add.l   \src,\src
  .endif
  .if \multiplicator==20               ;//*20
    move.l  \src,\scratch
    lsl.l   #2,\src
    add.l   \scratch,\src
    lsl.l   #2,\src
  .endif
  .if \multiplicator==23               ;//*23
    move.l  \src,\scratch
    add.l   \src,\src
    add.l   \scratch,\src
    lsl.l   #3,\src
    sub.l   \scratch,\src
  .endif
  .if \multiplicator==24               ;//*24
    move.l  \src,\scratch
    add.l   \src,\src
    add.l   \scratch,\src
    lsl.l   #3,\src
  .endif
  .if \multiplicator==28               ;//*28
    move.l  \src,\scratch
    lsl.l   #3,\src
    sub.l   \scratch,\src
    lsl.l   #2,\src
  .endif
  .if \multiplicator==29               ;//*29
    move.l \src,\scratch
    add.l  \src,\src
    add.l  \src,\scratch
    lsl.l  #4,\src
    sub.l  \scratch,\src
  .endif
  .if \multiplicator==30               ;//*30
    move.l  \src,\scratch
    lsl.l   #4,\src
    sub.l   \scratch,\src
    add.l   \src,\src
  .endif
  .if \multiplicator==31               ;//*31
    move.l  \src,\scratch
    lsl.l   #5,\src
    sub.l   \scratch,\src
  .endif
  .if \multiplicator==32               ;//*32
    lsl.l   #5,\src
  .endif
  .if \multiplicator==33               ;//*33
    move.l  \src,\scratch
    lsl.l   #5,\src
    add.l   \scratch,\src
  .endif
  .if \multiplicator==34               ;//*34
    move.l  \src,\scratch
    lsl.l   #4,\src
    add.l   \scratch,\src
    add.l   \src,\src
  .endif
  .if \multiplicator==35               ;//*35
    move.l  \src,\scratch
    lsl.l   #4,\src
    add.l   \scratch,\src
    add.l   \src,\src
    add.l   \scratch,\src
  .endif
  .if \multiplicator==36               ;//*36
    move.l  \src,\scratch
    lsl.l   #3,\src
    add.l   \scratch,\src
    lsl.l   #2,\src
  .endif
  .if \multiplicator==40               ;//*40
    move.l  \src,\scratch
    lsl.l   #2,\src
    add.l   \scratch,\src
    lsl.l   #3,\src
  .endif
  .if \multiplicator==41               ;//*41
    move.l  \src,\scratch
    lsl.l   #2,\src
    add.l   \scratch,\src
    lsl.l   #3,\src
    add.l   \scratch,\src
  .endif
  .if \multiplicator==44               ;//*44
    move.l  \src,\scratch
    add.l   \scratch,\scratch
    add.l   \scratch,\src
    lsl.l   #4,\src
    add.l   \scratch,\scratch
    sub.l   \scratch,\src
  .endif
  .if \multiplicator==45               ;//*45
    move.l  \src,\scratch
    add.l   \src,\src
    add.l   \src,\scratch
    move.l  \scratch,\src
    lsl.l   #4,\src
    sub.l   \scratch,\src
  .endif
  .if \multiplicator==46               ;//*46
    move.l  \src,\scratch
    add.l   \scratch,\scratch
    add.l   \scratch,\src
    lsl.l   #4,\src
    sub.l   \scratch,\src
  .endif
  .if \multiplicator==48               ;//*48
    move.l  \src,\scratch
    add.l   \src,\src
    add.l   \scratch,\src
    lsl.l   #4,\src
  .endif
  .if \multiplicator==49               ;//*49
    move.l  \src,\scratch
    add.l   \src,\src
    add.l   \scratch,\src
    lsl.l   #4,\src
    add.l   \scratch,\src
  .endif
  .if \multiplicator==56               ;//*56
    move.l  \src,\scratch
    lsl.l   #3,\src
    sub.l   \scratch,\src
    lsl.l   #3,\src
  .endif
  .if \multiplicator==60               ;//*60
    move.l  \src,\scratch
    lsl.l   #4,\src
    sub.l   \scratch,\src
    lsl.l   #2,\src
  .endif
  .if \multiplicator==62               ;//*62
    move.l  \src,\scratch
    lsl.l   #5,\src
    sub.l   \scratch,\src
    add.l   \src,\src
  .endif
  .if \multiplicator==63               ;//*63
    move.l  \src,\scratch
    lsl.l   #6,\src
    sub.l   \scratch,\src
  .endif
  .if \multiplicator==64               ;//*64
    lsl.l   #6,\src
  .endif
  .if \multiplicator==65               ;//*65
    move.l  \src,\scratch
    lsl.l   #6,\src
    add.l   \scratch,\src
  .endif
  .if \multiplicator==66               ;//*66
    move.l  \src,\scratch
    lsl.l   #5,\src
    add.l   \scratch,\src
    add.l   \src,\src
  .endif
  .if \multiplicator==68               ;//*68
    move.l  \src,\scratch
    lsl.l   #4,\src
    add.l   \scratch,\src
    lsl.l   #2,\src
  .endif
  .if \multiplicator==72               ;//*72
    move.l  \src,\scratch
    lsl.l   #3,\src
    add.l   \scratch,\src
    lsl.l   #3,\src
  .endif
  .if \multiplicator==80               ;//*80
    move.l  \src,\scratch
    lsl.l   #2,\src
    add.l   \scratch,\src
    lsl.l   #4,\src
  .endif
  .if \multiplicator==84               ;//*84
    move.l  \src,\scratch
    lsl.l   #2,\src
    add.l   \scratch,\src
    lsl.l   #2,\src
    add.l   \scratch,\src
    lsl.l   #2,\src
  .endif
  .if \multiplicator==92               ;//*92
    move.l  \src,\scratch
    add.l   \src,\src
    add.l   \scratch,\src
    lsl.l   #3,\src
    sub.l   \scratch,\src
    lsl.l   #2,\src
  .endif
  .if \multiplicator==96               ;//*96
    move.l  \src,\scratch
    add.l   \src,\src
    add.l   \scratch,\src
    lsl.l   #5,\src
  .endif
  .if \multiplicator==112              ;//*112
    move.l  \src,\scratch
    lsl.l   #3,\src
    sub.l   \scratch,\src
    lsl.l   #4,\src
  .endif
  .if \multiplicator==120              ;//*120
    move.l  \src,\scratch
    lsl.l   #4,\src
    sub.l   \scratch,\src
    lsl.l   #3,\src
  .endif
  .if \multiplicator==124              ;//*124
    move.l  \src,\scratch
    lsl.l   #5,\src
    sub.l   \scratch,\src
    lsl.l   #2,\src
  .endif
  .if \multiplicator==126              ;//*126
    move.l  \src,\scratch
    lsl.l   #6,\src
    sub.l   \scratch,\src
    add.l   \src,\src
  .endif
  .if \multiplicator==127              ;//*127
    move.l  \src,\scratch
    lsl.l   #7,\src
    sub.l   \scratch,\src
  .endif
  .if \multiplicator==128              ;//*128
    lsl.l   #7,\src
  .endif
  .if \multiplicator==129              ;//*129
    move.l  \src,\scratch
    lsl.l   #7,\src
    add.l   \scratch,\src
  .endif
  .if \multiplicator==130              ;//*130
    move.l  \src,\scratch
    lsl.l   #6,\src
    add.l   \scratch,\src
    add.l   \src,\src
  .endif
  .if \multiplicator==132              ;//*132
    move.l  \src,\scratch
    lsl.l   #5,\src
    add.l   \scratch,\src
    lsl.l   #2,\src
  .endif
  .if \multiplicator==136              ;//*136
    move.l  \src,\scratch
    lsl.l   #4,\src
    add.l   \scratch,\src
    lsl.l   #3,\src
  .endif
  .if \multiplicator==144              ;//*144
    move.l  \src,\scratch
    lsl.l   #3,\src
    add.l   \scratch,\src
    lsl.l   #4,\src
  .endif
  .if \multiplicator==156              ;//*156
    move.l \src,\scratch
    lsl.l  #2,\scratch
    add.l  \scratch,\src
    lsl.l  #5,\src
    sub.l  \scratch,\src
  .endif
  .if \multiplicator==160              ;//*160
    move.l  \src,\scratch
    lsl.l   #2,\src
    add.l   \scratch,\src
    lsl.l   #5,\src
  .endif
  .if \multiplicator==184              ;//*184
    move.l  \src,\scratch
    add.l   \src,\src
    add.l   \scratch,\src
    lsl.l   #3,\src
    sub.l   \scratch,\src
    lsl.l   #3,\src
  .endif
  .if \multiplicator==192              ;//*192
    move.l  \src,\scratch
    add.l   \src,\src
    add.l   \scratch,\src
    lsl.l   #6,\src
  .endif
  .if \multiplicator==196              ;//*196
    move.l  \src,\scratch
    add.l   \src,\src
    add.l   \scratch,\src
    lsl.l   #4,\src
    add.l   \scratch,\src
    lsl.l   #2,\src
  .endif
  .if \multiplicator==200              ;//*200
    move.l  \src,\scratch
    add.l   \src,\src
    add.l   \scratch,\src
    lsl.l   #3,\src
    add.l   \scratch,\src
    lsl.l   #3,\src
  .endif
  .if \multiplicator==208              ;//*208
    move.l  \src,\scratch
    add.l   \src,\src
    add.l   \scratch,\src
    lsl.l   #2,\src
    add.l   \scratch,\src
    lsl.l   #4,\src
  .endif
  .if \multiplicator==224              ;//*224
    move.l  \src,\scratch
    lsl.l   #3,\src
    sub.l   \scratch,\src
    lsl.l   #5,\src
  .endif
  .if \multiplicator==240              ;//*240
    move.l  \src,\scratch
    lsl.l   #4,\src
    sub.l   \scratch,\src
    lsl.l   #4,\src
  .endif
  .if \multiplicator==248              ;//*248
    move.l  \src,\scratch
    lsl.l   #5,\src
    sub.l   \scratch,\src
    lsl.l   #3,\src
  .endif
  .if \multiplicator==252              ;//*252
    move.l  \src,\scratch
    lsl.l   #6,\src
    sub.l   \scratch,\src
    lsl.l   #2,\src
  .endif
  .if \multiplicator==254              ;//*254
    move.l  \src,\scratch
    lsl.l   #7,\src
    sub.l   \scratch,\src
    add.l   \src,\src
  .endif
  .if \multiplicator==255              ;//*255
    move.l  \src,\scratch
    lsl.l   #8,\src
    sub.l   \scratch,\src
  .endif
  .if \multiplicator==256              ;//*256
    lsl.l   #8,\src
  .endif
  .if \multiplicator==257              ;//*257
    move.l  \src,\scratch
    lsl.l   #8,\src
    add.l   \scratch,\src
  .endif
  .if \multiplicator==258              ;//*258
    move.l  \src,\scratch
    lsl.l   #7,\src
    add.l   \scratch,\src
    add.l   \src,\src
  .endif
  .if \multiplicator==260              ;//*260
    move.l  \src,\scratch
    lsl.l   #6,\src
    add.l   \scratch,\src
    lsl.l   #2,\src
  .endif
  .if \multiplicator==264              ;//*264
    move.l  \src,\scratch
    lsl.l   #5,\src
    add.l   \scratch,\src
    lsl.l   #3,\src
  .endif
  .if \multiplicator==272              ;//*272
    move.l  \src,\scratch
    lsl.l   #4,\src
    add.l   \scratch,\src
    lsl.l   #4,\src
  .endif
  .if \multiplicator==288              ;//*288
    move.l  \src,\scratch
    lsl.l   #3,\src
    add.l   \scratch,\src
    lsl.l   #5,\src
  .endif
  .if \multiplicator==304              ;//*304
    move.l  \src,\scratch
    lsl.l   #3,\src
    add.l   \scratch,\src
    add.l   \src,\src
    add.l   \scratch,\src
    lsl.l   #4,\src
  .endif
  .if \multiplicator==320              ;//*320
    move.l  \src,\scratch
    lsl.l   #2,\src
    add.l   \scratch,\src
    lsl.l   #6,\src
  .endif
  .if \multiplicator==384              ;//*384
    move.l  \src,\scratch
    add.l   \src,\src
    add.l   \scratch,\src
    lsl.l   #7,\src
  .endif
  .if \multiplicator==400              ;//*400
    move.l  \src,\scratch
    lsl.l   #3,\src
    add.l   \src,\scratch
    add.l   \src,\src
    add.l   \scratch,\src
    lsl.l   #4,\src
  .endif
  .if \multiplicator==416              ;//*416
    move.l  \src,\scratch
    lsl.l   #2,\src
    add.l   \src,\scratch
    add.l   \src,\src
    add.l   \scratch,\src
    lsl.l   #5,\src
  .endif
  .if \multiplicator==480              ;//*480
    move.l  \src,\scratch
    lsl.l   #4,\src
    sub.l   \scratch,\src
    lsl.l   #5,\src
  .endif
  .if \multiplicator==512              ;//*512
    lsl.l   #8,\src
    add.l   \src,\src
  .endif
  .if \multiplicator==576              ;//*576
    move.l  \src,\scratch
    lsl.l   #3,\src
    add.l   \scratch,\src
    lsl.l   #6,\src
  .endif
  .if \multiplicator==608              ;//*608
    move.l  \src,\scratch
    lsl.l   #3,\src
    add.l   \scratch,\src
    add.l   \src,\src
    add.l   \scratch,\src
    lsl.l   #5,\src
  .endif
  .if \multiplicator==624              ;//*624
    move.l  \src,\scratch
    lsl.l   #2,\src
    add.l   \scratch,\src
    lsl.l   #3,\src
    sub.l   \scratch,\src
    lsl.l   #4,\src
  .endif
  .if \multiplicator==625              ;//*625
    move.l  \src,\scratch
    lsl.l   #4,\src
    sub.l   \src,\scratch
    lsl.l   #3,\src
    add.l   \src,\scratch
    lsl.l   #2,\src
    add.l   \scratch,\src
  .endif
  .if \multiplicator==640              ;//*640
    move.l  \src,\scratch
    lsl.l   #2,\src
    add.l   \scratch,\src
    lsl.l   #7,\src
  .endif
  .if \multiplicator==768              ;//*768
    move.l  \src,\scratch
    add.l   \src,\src
    add.l   \scratch,\src
    lsl.l   #8,\src
  .endif
  .if \multiplicator==896              ;//*896
    move.l  \src,\scratch
    lsl.l   #3,\src
    sub.l   \scratch,\src
    lsl.l   #7,\src
  .endif
  .if \multiplicator==960              ;//*960
    move.l  \src,\scratch
    lsl.l   #4,\src
    sub.l   \scratch,\src
    lsl.l   #6,\src
  .endif
  .if \multiplicator==1024             ;//*1024
    lsl.l   #8,\src
    lsl.l   #2,\src
  .endif
  .if \multiplicator==1280             ;//*1280
    move.l  \src,\scratch
    lsl.l   #2,\src
    add.l   \scratch,\src
    lsl.l   #8,\src
  .endif
  .if \multiplicator==1920             ;//*1920
    move.l  \src,\scratch
    lsl.l   #4,\src
    sub.l   \scratch,\src
    lsl.l   #7,\src
  .endif
  .if \multiplicator==2048             ;//*2048
    lsl.l   #8,\src
    lsl.l   #3,\src
  .endif
  .if \multiplicator==2560             ;//*2560
    move.l  \src,\scratch
    lsl.l   #2,\src
    add.l   \scratch,\src
    lsl.l   #8,\src
    add.l   \src,\src
  .endif
  .if \multiplicator==3072             ;//*3072
    move.l  \src,\scratch
    add.l   \src,\src
    add.l   \scratch,\src
    lsl.l   #8,\src
    lsl.l   #2,\src
  .endif
.endm
