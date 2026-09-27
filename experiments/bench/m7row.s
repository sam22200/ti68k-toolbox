| Mode 7 row sampler in 68000 asm (GNU as syntax), the hand-written twin of bench3.c case 7:
|   for 40 pixels: *out++ = m7map[(vrow & 63) << 6 | (((cx + *horz++) >> 8) & 63)]
| C prototype (register parameters, explicit so that no compiler flag can change them):
|   void m7row_asm(unsigned char *out asm("%a0"), const short *horz asm("%a1"),
|                  short cx asm("%d0"), short vrow asm("%d1"));
| Uses only d0-d2/a0-a1 (scratch for GCC) plus a2, which is saved and restored.
	.text
	.even
	.globl m7row_asm
m7row_asm:
	move.l	%a2,-(%sp)
	lea	m7map,%a2
	and.w	#63,%d1
	lsl.w	#6,%d1
	add.w	%d1,%a2			| a2 = start of the texture row
	moveq	#39,%d2
0:	move.w	(%a1)+,%d1
	add.w	%d0,%d1
	lsr.w	#8,%d1
	and.w	#63,%d1
	move.b	0(%a2,%d1.w),(%a0)+
	dbra	%d2,0b
	move.l	(%sp)+,%a2
	rts
