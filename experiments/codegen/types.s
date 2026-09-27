	.file	"types.c"
#NO_APP
	.text
tigcc_compiled.:
	.text
	.even
	.globl	div4_s
div4_s:
	move.w 4(%sp),%d0
	jbge .L2
	addq.w #3,%d0
.L2:
	asr.w #2,%d0
	rts
	.even
	.globl	div4_u
div4_u:
	move.w 4(%sp),%d0
	lsr.w #2,%d0
	rts
	.even
	.globl	sar4_s
sar4_s:
	move.w 4(%sp),%d0
	asr.w #2,%d0
	rts
	.even
	.globl	mod8_s
mod8_s:
	move.w 4(%sp),%d1
	moveq #8,%d2
	ext.l %d1
	divs.w %d2,%d1
	move.l %d1,%d0
	swap %d0
	rts
	.even
	.globl	mod8_u
mod8_u:
	move.w 4(%sp),%d0
	and.w #7,%d0
	rts
	.even
	.globl	div10_s
div10_s:
	move.w 4(%sp),%d0
	moveq #10,%d2
	ext.l %d0
	divs.w %d2,%d0
	rts
	.even
	.globl	div10_recip
div10_recip:
	move.w 4(%sp),%d0
	mulu.w #6554,%d0
	clr.w %d0
	swap %d0
	rts
	.even
	.globl	range_s
range_s:
	move.w 4(%sp),%d0
	add.w #-10,%d0
	cmp.w #39,%d0
	sls %d0
	ext.w %d0
	neg.w %d0
	rts
	.even
	.globl	range_u
range_u:
	move.w 4(%sp),%d0
	add.w #-10,%d0
	cmp.w #39,%d0
	sls %d0
	ext.w %d0
	neg.w %d0
	rts
	.even
	.globl	sum_uchar
sum_uchar:
	move.b 5(%sp),%d2
	clr.b %d1
	sub.l %a1,%a1
	jbra .L21
.L22:
	moveq #0,%d0
	move.b %d1,%d0
	lea tabc,%a0
	move.b (%a0,%d0.l),%d0
	and.w #255,%d0
	add.w %d0,%a1
	addq.b #1,%d1
.L21:
	cmp.b %d1,%d2
	jbne .L22
	move.w %a1,%d0
	rts
	.even
	.globl	sum_short
sum_short:
	move.w 4(%sp),%d2
	clr.w %d1
	sub.l %a1,%a1
	jbra .L26
.L27:
	moveq #0,%d0
	move.w %d1,%d0
	lea tabc,%a0
	move.b (%a0,%d0.l),%d0
	and.w #255,%d0
	add.w %d0,%a1
	addq.w #1,%d1
.L26:
	cmp.w %d1,%d2
	jbne .L27
	move.w %a1,%d0
	rts
	.even
	.globl	idx_uchar
idx_uchar:
	moveq #0,%d0
	move.b 5(%sp),%d0
	add.l %d0,%d0
	lea tabs,%a0
	move.w (%a0,%d0.l),%d0
	rts
	.even
	.globl	idx_short
idx_short:
	moveq #0,%d0
	move.b 5(%sp),%d0
	add.l %d0,%d0
	lea tabs,%a0
	move.w (%a0,%d0.l),%d0
	rts
	.even
	.globl	add_uchar
add_uchar:
	move.b 5(%sp),%d0
	add.b 7(%sp),%d0
	and.w #255,%d0
	rts
	.even
	.globl	mul_long_const
mul_long_const:
	move.l 4(%sp),%d0
	move.l %d0,%d1
	add.l %d0,%d1
	lsl.l #5,%d0
	sub.l %d1,%d0
	rts
	.even
	.globl	mul_short_const
mul_short_const:
	move.w 4(%sp),%d0
	muls.w #30,%d0
	rts
	.even
	.globl	mul_shifts
mul_shifts:
	move.w 4(%sp),%d0
	move.w %d0,%d1
	add.w %d0,%d1
	lsl.w #5,%d0
	sub.w %d1,%d0
	rts
