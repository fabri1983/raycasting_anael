;// These instructions needs to be beat: 
;//   op1(d6) * op2(d7) => mulu  d7,d6
;//   op1(d6) >> FS     => lsr.l #8,d6
;// Result stored in op1 (d6).
;// Timings:
;//   mulu   d7,d6  => 38~70 cycles
;//   lsr.l  #8,d6  => 24 cycles
;// NOTES: 
;//  - high word part of the result matters, and since after multiplication 
;//  - we have to: d6 >> FS (FS=256), then is safe to not clear d6 with moveq #0,d6 in those case_N blocks having the optimized instructions.
;//  - d6 = 0..256 and d7 = 0..65535


;// This works on GCC GAS with C pre-processor
.set PAD_NUM_BYTES, 16  // this needs to be 16 otherwise you'll have to modify macro shft_adjusted
#define mulu256_pad(label) .fill PAD_NUM_BYTES-(.-label),1,0x00

.macro shft_adjusted _d
  ;//lsr.l  #8+4,\_d
  ;// 4 cycles faster:
  andi.w  #~((1<<(8+4))-1),\_d
  swap    \_d
  rol.l   #8-4,\_d
.endm

  ;// jump to target block (multiple of PAD_NUM_BYTES) only if d6 comes multiplied by PAD_NUM_BYTES
  jmp     table_mulu_256(pc,d6.w)
* vs:
  ;// jump to target block (multiple of PAD_NUM_BYTES) only if d6 comes multiplied by PAD_NUM_BYTES
  ;// and only if the effective address of label 'table_mulu_256' was added into d6.
  movea   d6,a5
  jmp     (a5)

table_mulu_256:
case_0:
  bra     shft_FS
  mulu256_pad(case_0)
case_1:
  move.w  d7,d6
  bra     shft_FS
  mulu256_pad(case_1)
case_2:
  move.w  d7,d6
  add.l	  d6,d6
  mulu256_pad(case_2)
case_3:
  move.w  d7,d6
  move.l  d6,d7
  add.l	  d6,d6
  add.l	  d7,d6
  bra     shft_FS
  mulu256_pad(case_3)
case_4:
  move.w  d7,d6
  lsl.l   #2,d6
  bra     shft_FS
  mulu256_pad(case_4)
case_5:
  move.w  d7,d6
  move.l  d6,d7
  lsl.l   #2,d6
  add.l   d7,d6
  bra     shft_FS
  mulu256_pad(case_5)
case_6:
  move.w  d7,d6
  add.l   d6,d6
  move.l  d6,d7
  add.l   d6,d6
  add.l   d7,d6
  bra     shft_FS
  mulu256_pad(case_6)
case_7:
  move.w  d7,d6
  move.l  d6,d7
  lsl.l   #3,d6
  sub.l   d7,d6
  bra     shft_FS
  mulu256_pad(case_7)
case_8:
  move.w  d7,d6
  lsl.l   #3,d6
  bra     shft_FS
  mulu256_pad(case_8)
case_9:
  move.w  d7,d6
  move.l  d6,d7
  lsl.l   #3,d6
  add.l   d7,d6
  bra     shft_FS
  mulu256_pad(case_9)
case_10:
  move.w  d7,d6
  move.l  d6,d7
  lsl.l   #2,d6
  add.l   d7,d6
  add.l   d6,d6
  bra     shft_FS
  mulu256_pad(case_10)
case_11:
  move.w  d7,d6
  move.l  d6,d7
  add.l   d7,d6
  add.l	  d7,d6
  lsl.l	  #2,d6
  sub.l	  d7,d6
  bra     shft_FS
  mulu256_pad(case_11)
case_12:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_12)
case_13:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_13)
case_14:
  move.w  d7,d6
  move.l  d6,d7
  lsl.l   #3,d6
  sub.l   d7,d6
  add.l   d6,d6
  bra     shft_FS
  mulu256_pad(case_14)
case_15:
  move.w  d7,d6
  move.l  d6,d7
  lsl.l   #4,d6
  sub.l   d7,d6
  bra     shft_FS
  mulu256_pad(case_15)
case_16:
  move.w  d7,d6
  lsl.l   #4,d6
  bra     shft_FS
  mulu256_pad(case_16)
case_17:
  move.w  d7,d6
  move.l  d6,d7
  lsl.l   #4,d6
  add.l   d7,d6
  bra     shft_FS
  mulu256_pad(case_17)
case_18:
  move.w  d7,d6
  add.l   d6,d6
  move.l  d6,d7
  lsl.l   #3,d6
  add.l   d7,d6
  bra     shft_FS
  mulu256_pad(case_18)
case_19:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_19)
case_20:
  move.w  d7,d6
  move.l  d6,d7
  lsl.l   #2,d6
  add.l   d7,d6
  lsl.l   #2,d6
  bra     shft_FS
  mulu256_pad(case_20)
case_21:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_21)
case_22:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_22)
case_23:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_23)
case_24:
  move.w  d7,d6
  move.l  d6,d7
  add.l   d6,d6
  add.l   d7,d6
  lsl.l   #3,d6
  bra     shft_FS
  mulu256_pad(case_24)
case_25:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_25)
case_26:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_26)
case_27:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_27)
case_28:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_28)
case_29:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_29)
case_30:
  move.w  d7,d6
  move.l  d6,d7
  lsl.l   #5,d6
  sub.l   d7,d6
  sub.l   d7,d6
  bra     shft_FS
  mulu256_pad(case_30)
case_31:
  move.w  d7,d6
  move.l  d6,d7
  add.l   d6,d6
  add.l   d7,d6
  lsl.l   #3,d6
  bra     shft_FS
  mulu256_pad(case_31)
case_32:
  move.w  d7,d6
  lsl.l   #5,d6
  bra     shft_FS
  mulu256_pad(case_32)
case_33:
  move.w  d7,d6
  move.l  d6,d7
  lsl.l   #5,d6
  add.l   d7,d6
  bra     shft_FS
  mulu256_pad(case_33)
case_34:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_34)
case_35:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_35)
case_36:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_36)
case_37:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_37)
case_38:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_38)
case_39:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_39)
case_40:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_40)
case_41:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_41)
case_42:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_42)
case_43:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_43)
case_44:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_44)
case_45:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_45)
case_46:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_46)
case_47:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_47)
case_48:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_48)
case_49:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_49)
case_50:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_50)
case_51:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_51)
case_52:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_52)
case_53:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_53)
case_54:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_54)
case_55:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_55)
case_56:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_56)
case_57:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_57)
case_58:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_58)
case_59:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_59)
case_60:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_60)
case_61:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_61)
case_62:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_62)
case_63:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_63)
case_64:
  move.w  d7,d6
  lsl.l   #6,d6
  bra     shft_FS
  mulu256_pad(case_64)
case_65:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_65)
case_66:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_66)
case_67:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_67)
case_68:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_68)
case_69:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_69)
case_70:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_70)
case_71:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_71)
case_72:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_72)
case_73:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_73)
case_74:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_74)
case_75:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_75)
case_76:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_76)
case_77:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_77)
case_78:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_78)
case_79:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_79)
case_80:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_80)
case_81:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_81)
case_82:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_82)
case_83:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_83)
case_84:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_84)
case_85:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_85)
case_86:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_86)
case_87:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_87)
case_88:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_88)
case_89:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_89)
case_90:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_90)
case_91:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_91)
case_92:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_92)
case_93:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_93)
case_94:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_94)
case_95:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_95)
case_96:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_96)
case_97:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_97)
case_98:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_98)
case_99:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_99)
case_100:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_100)
case_101:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_101)
case_102:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_102)
case_103:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_103)
case_104:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_104)
case_105:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_105)
case_106:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_106)
case_107:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_107)
case_108:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_108)
case_109:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_109)
case_110:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_110)
case_111:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_111)
case_112:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_112)
case_113:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_113)
case_114:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_114)
case_115:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_115)
case_116:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_116)
case_117:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_117)
case_118:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_118)
case_119:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_119)
case_120:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_120)
case_121:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_121)
case_122:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_122)
case_123:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_123)
case_124:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_124)
case_125:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_125)
case_126:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_126)
case_127:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_127)
case_128:
  move.w  d7,d6
  lsl.l   #7,d6
  bra     shft_FS
  mulu256_pad(case_128)
case_129:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_129)
case_130:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_130)
case_131:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_131)
case_132:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_132)
case_133:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_133)
case_134:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_134)
case_135:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_135)
case_136:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_136)
case_137:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_137)
case_138:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_138)
case_139:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_139)
case_140:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_140)
case_141:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_141)
case_142:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_142)
case_143:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_143)
case_144:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_144)
case_145:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_145)
case_146:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_146)
case_147:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_147)
case_148:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_148)
case_149:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_149)
case_150:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_150)
case_151:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_151)
case_152:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_152)
case_153:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_153)
case_154:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_154)
case_155:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_155)
case_156:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_156)
case_157:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_157)
case_158:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_158)
case_159:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_159)
case_160:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_160)
case_161:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_161)
case_162:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_162)
case_163:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_163)
case_164:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_164)
case_165:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_165)
case_166:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_166)
case_167:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_167)
case_168:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_168)
case_169:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_169)
case_170:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_170)
case_171:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_171)
case_172:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_172)
case_173:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_173)
case_174:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_174)
case_175:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_175)
case_176:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_176)
case_177:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_177)
case_178:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_178)
case_179:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_179)
case_180:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_180)
case_181:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_181)
case_182:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_182)
case_183:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_183)
case_184:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_184)
case_185:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_185)
case_186:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_186)
case_187:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_187)
case_188:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_188)
case_189:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_189)
case_190:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_190)
case_191:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_191)
case_192:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_192)
case_193:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_193)
case_194:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_194)
case_195:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_195)
case_196:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_196)
case_197:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_197)
case_198:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_198)
case_199:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_199)
case_200:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_200)
case_201:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_201)
case_202:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_202)
case_203:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_203)
case_204:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_204)
case_205:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_205)
case_206:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_206)
case_207:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_207)
case_208:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_208)
case_209:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_209)
case_210:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_210)
case_211:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_211)
case_212:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_212)
case_213:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_213)
case_214:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_214)
case_215:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_215)
case_216:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_216)
case_217:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_217)
case_218:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_218)
case_219:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_219)
case_220:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_220)
case_221:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_221)
case_222:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_222)
case_223:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_223)
case_224:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_224)
case_225:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_225)
case_226:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_226)
case_227:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_227)
case_228:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_228)
case_229:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_229)
case_230:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_230)
case_231:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_231)
case_232:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_232)
case_233:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_233)
case_234:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_234)
case_235:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_235)
case_236:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_236)
case_237:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_237)
case_238:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_238)
case_239:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_239)
case_240:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_240)
case_241:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_241)
case_242:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_242)
case_243:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_243)
case_244:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_244)
case_245:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_245)
case_246:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_246)
case_247:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_247)
case_248:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_248)
case_249:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_249)
case_250:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_250)
case_251:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_251)
case_252:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_252)
case_253:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_253)
case_254:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_254)
case_255:
  mulu    d7,d6
  shft_adjusted d6
  bra     mulu_end
  mulu256_pad(case_255)
case_256:
  move.w  d7,d6
  lsl.l   #8,d6
  bra     shft_FS
  mulu256_pad(case_256)

shft_FS:
  lsr.l  #8,d6
mulu_end:
