| proj68k.s: sp_rows_asm, the row loop of proj.c's sp_piece (shared-face projection).
| One call draws every row of one piece, front to back, into the colour, seen and
| two-layer coverage planes. Exactly the C path's result (checked by test_proj/bench).
|
| In: a0 = SpRows block (proj.h). The caller guarantees rows >= 1, every row inside
| the plane and bytes bx..bx+3 inside the 40-byte line (no clipping here).
| Texture bytes: bit 7 = light plane, bit 6 = dark plane (proj.py).
|
| Registers: a0 row stream, a1 texture, a2 seen, a3 light, a4 dark, a5 cov0, a6 cov1
| (all at this row's first byte), d5 = 2*plane (cov2 = a5+d5, cov3 = a6+d5),
| d6 = depth, d7 = rows-1 (dbra). Scratch on the stack: M mask, L light, D dark longs.

	.text
	.even
	.globl	sp_rows_asm

	.set	M,0
	.set	L,4
	.set	D,8
	.set	SH,12

| Emit byte k of the row: seen, colour, then the C path's coverage rule.
	.macro	EMIT k
	move.b	M+\k(%sp),%d0
	beq.s	9f
	or.b	%d0,\k(%a2)
	move.b	L+\k(%sp),%d1
	and.b	%d0,%d1
	or.b	%d1,\k(%a3)
	move.b	D+\k(%sp),%d1
	and.b	%d0,%d1
	or.b	%d1,\k(%a4)
	tst.b	\k(%a6)
	beq.s	5f
	cmp.b	\k(%a5),%d6
	beq.s	5f
	tst.b	\k(%a6,%d5.w)
	beq.s	6f
	cmp.b	\k(%a5,%d5.w),%d6
	bne.s	9f
6:	move.b	%d6,\k(%a5,%d5.w)
	or.b	%d0,\k(%a6,%d5.w)
	bra.s	9f
5:	move.b	%d6,\k(%a5)
	or.b	%d0,\k(%a6)
9:
	.endm

| Visible bits of byte k: mask & ~seen; d3 collects whether any remain.
	.macro	HIDE k
	move.b	M+\k(%sp),%d1
	beq.s	9f
	move.b	\k(%a2),%d2
	not.b	%d2
	and.b	%d2,%d1
	move.b	%d1,M+\k(%sp)
	or.b	%d1,%d3
9:
	.endm

sp_rows_asm:
	movem.l	%d3-%d7/%a2-%a6,-(%sp)
	lea	-16(%sp),%sp
	move.l	24(%a0),%a6
	move.l	20(%a0),%a5
	move.l	16(%a0),%a4
	move.l	12(%a0),%a3
	move.l	8(%a0),%a2
	move.l	4(%a0),%a1
	move.w	28(%a0),%d5
	move.b	30(%a0),%d6
	move.b	31(%a0),SH(%sp)
	move.w	32(%a0),%d7
	subq.w	#1,%d7
	move.l	(%a0),%a0

row:
	move.b	(%a0)+,%d4
	add.b	SH(%sp),%d4		| st = x0 + shift
	moveq	#0,%d0
	move.b	(%a0)+,%d0		| n
	beq	next
	and.w	#0xFF,%d4
	moveq	#-1,%d1
	lsr.l	%d4,%d1			| bits from st on
	move.w	%d4,%d2
	add.w	%d0,%d2			| st + n
	moveq	#-1,%d3
	lsr.l	%d2,%d3
	eor.l	%d3,%d1
	move.l	%d1,M(%sp)		| row mask
	moveq	#32,%d4
	sub.w	%d2,%d4			| left alignment after n pixels

	moveq	#0,%d3
	HIDE	0
	HIDE	1
	HIDE	2
	HIDE	3
	tst.b	%d3
	bne.s	visible
	add.w	%d0,%a0			| fully hidden: skip its texels
	bra	next

visible:
	move.w	%d0,%d1
	lsl.w	#4,%d1
	sub.w	%d0,%d1
	sub.w	%d0,%d1			| n * 14 bytes per pixel block
	neg.w	%d1
	moveq	#0,%d0
	moveq	#0,%d2
	moveq	#0,%d3
	bra	dispatch		| over the 336-byte unrolled block
texels:
	.rept	24
	move.b	(%a0)+,%d0
	move.b	0(%a1,%d0.w),%d1
	add.b	%d1,%d1			| X = light bit
	addx.l	%d2,%d2
	add.b	%d1,%d1			| X = dark bit
	addx.l	%d3,%d3
	.endr
texels_end:
	bra.s	aligned
dispatch:
	jmp	texels_end(%pc,%d1.w)
aligned:
	lsl.l	%d4,%d2
	lsl.l	%d4,%d3
	move.l	%d2,L(%sp)
	move.l	%d3,D(%sp)
	EMIT	0
	EMIT	1
	EMIT	2
	EMIT	3

next:
	lea	40(%a2),%a2
	lea	40(%a3),%a3
	lea	40(%a4),%a4
	lea	40(%a5),%a5
	lea	40(%a6),%a6
	dbra	%d7,row

	lea	16(%sp),%sp
	movem.l	(%sp)+,%d3-%d7/%a2-%a6
	rts
