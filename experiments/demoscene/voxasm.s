| Voxel terrain: one depth slice in 68000 asm, the twin of voxel.c's SAMPLE loop (same pixels).
| C prototype (voxel.c, -DVOX_ASM):
|   void vox_slice(const VoxSlice *p asm("%a0")) __attribute__((__regparm__(1)));
| VoxSlice: U, V, dU, dV (longs: U's high word = map column * 2 + a fraction bit, V's = row << 8),
|   then the pointers map, ytab row of the slice, top[], buffer (light plane, dark at +0xF00).
| Globals of voxel.c: vox_pat[PH][7][2][4] (56 bytes per column phase), vox_rowo[] (y * 30).
| PH (columns per byte: 2 or 4) comes from the command line: -Wa,--defsym,PH=2.
| Registers: a0 map, a1 ytab row, a2 top (post-incremented), a3 column byte, a5 U, a6 V,
|   d6 dU, d7 dV; d0-d5 and a4 scratch. Per sample ~104 cycles when hidden, plus ~180 per fill and
|   ~34 per row of the fill (the C loop: ~220 per sample).
	.text
	.even
	.globl	vox_slice
vox_slice:
	movem.l	%d3-%d7/%a2-%a6, -(%sp)
	movea.l	(%a0), %a5			| U
	movea.l	4(%a0), %a6			| V
	move.l	8(%a0), %d6			| dU
	move.l	12(%a0), %d7			| dV
	movea.l	20(%a0), %a1			| ytab row
	movea.l	24(%a0), %a2			| top
	lea	PH*20(%a2), %a4
	move.l	%a4, top_end			| end of top[]
	movea.l	28(%a0), %a3			| buffer: column byte 0
	movea.l	16(%a0), %a0			| map
	bra	.Lcol

| one column of phase k: sample, compare with its top, fill the rows it uncovers
	.macro	SAMPLE k
	move.l	%a6, %d0			|  4  map offset = (V >> 16 & 0x7F00) | (U >> 16 & 0xFE)
	swap	%d0				|  4
	and.w	#0x7F00, %d0			|  8
	move.l	%a5, %d1			|  4
	swap	%d1				|  4
	and.w	#0xFE, %d1			|  8
	or.w	%d1, %d0			|  4
	moveq	#0, %d1				|  4
	move.b	1(%a0, %d0.w), %d1		| 14  height
	move.b	(%a1, %d1.w), %d1		| 14  screen row y (d1.w = y)
	move.b	(%a2)+, %d2			|  8  top of the column
	cmp.b	%d2, %d1			|  4
	bcs	.Lfill\k			|  8  y < top: rows to fill
.Lnext\k:
	adda.l	%d6, %a5			|  8
	adda.l	%d7, %a6			|  8
	.endm

| the fill of phase k (out of the sample loop: the not-taken branch is the common case)
	.macro	FILL k
.Lfill\k:
	move.b	%d1, -1(%a2)			| top = y
	sub.b	%d1, %d2			| n = top - y (1..100)
	ext.w	%d2
	moveq	#0, %d3
	move.b	(%a0, %d0.w), %d3		| level * 4
	add.w	%d3, %d3			| * 8: 8 pattern bytes per level
	btst	#0, %d1
	beq.s	1f
	addq.w	#4, %d3				| odd first row
1:	lea	vox_pat+\k*56, %a4
	adda.w	%d3, %a4
	move.b	(%a4)+, %d3			| light, dark of the first row, then of the second
	move.b	(%a4)+, %d4
	move.b	(%a4)+, %d5
	move.b	(%a4), %d0
	lea	vox_rowo, %a4
	add.w	%d1, %d1
	move.w	(%a4, %d1.w), %d1
	lea	(%a3, %d1.w), %a4		| first row of the column
	move.w	%d2, %d1
	lsr.w	#1, %d1				| row pairs
	bra.s	3f
2:	or.b	%d3, (%a4)			| 12
	or.b	%d4, 0xF00(%a4)			| 16
	or.b	%d5, 30(%a4)			| 16
	or.b	%d0, 0xF1E(%a4)			| 16
	lea	60(%a4), %a4			|  8
3:	dbra	%d1, 2b				| 10
	btst	#0, %d2
	beq	.Lnext\k
	or.b	%d3, (%a4)
	or.b	%d4, 0xF00(%a4)
	bra	.Lnext\k
	.endm

.Lcol:
	SAMPLE	0
	SAMPLE	1
	.if	PH == 4
	SAMPLE	2
	SAMPLE	3
	.endif
	addq.l	#1, %a3				| next column byte
	cmpa.l	top_end(%pc), %a2		| 18
	bne	.Lcol
	movem.l	(%sp)+, %d3-%d7/%a2-%a6
	rts

	FILL	0
	FILL	1
	.if	PH == 4
	FILL	2
	FILL	3
	.endif

	.even
top_end:
	.long	0

| void vox_clear(void *buf asm("%a0")): zeroes both planes' 3000 visible bytes (100 rows of 30)
| with movem: 11 registers, 44 bytes per store, ~2.2 cycles per byte (the C loop: ~7.8).
	.globl	vox_clear
vox_clear:
	movem.l	%d3-%d7/%a2-%a4, -(%sp)
	moveq	#0, %d1
	moveq	#0, %d2
	moveq	#0, %d3
	moveq	#0, %d4
	moveq	#0, %d5
	moveq	#0, %d6
	moveq	#0, %d7
	movea.l	%d1, %a1
	movea.l	%d1, %a2
	movea.l	%d1, %a3
	movea.l	%d1, %a4
	lea	0xF00+3000(%a0), %a0		| end of the dark plane's visible part
	bsr.s	1f
	lea	-0xF00+3000(%a0), %a0		| end of the light plane's (a0 is at the dark plane)
	bsr.s	1f
	movem.l	(%sp)+, %d3-%d7/%a2-%a4
	rts
1:	moveq	#67, %d0			| 68 x 44 = 2992 bytes, then 8
2:	movem.l	%d1-%d7/%a1-%a4, -(%a0)
	dbra	%d0, 2b
	movem.l	%d1-%d2, -(%a0)
	rts
