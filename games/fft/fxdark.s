| fxdark.s -- Dark 1 over the 160 visible pixels (5 long words) of a four-grey row.
|   rt.h: grey = light + 2*dark;  v -> min(v+1,3)  is  d' = d | l,  l' = d | ~l
|
| The operation wants ten long words at once -- five of each plane -- and the 68000 has
| eight data registers, so movem cannot carry both planes in one go. It is done three
| words at a time and then two, with one register (%d6) as the shared temporary.
|
| No temporary would be needed if d' could be had from l', and it can: d' = d | ~l'.
| That costs two extra not.l per word, 12 cycles, against 4 for the move. The move wins.
|
| void fx_dark_asm(u8 *light %a0, u8 *dark %a1, short rows %d0, long stride %d1)
	.text
	.even
	.globl fx_dark_asm
fx_dark_asm:
	movem.l	%d3-%d7/%a2,-(%sp)
	movea.l	%d1,%a2             | row stride (RT_PBYTES)
	move.w	%d0,%d7
	subq.w	#1,%d7
0:
	movem.l	(%a0),%d0-%d2       | light words 0..2
	movem.l	(%a1),%d3-%d5       | dark  words 0..2
	not.l	%d0
	move.l	%d0,%d6
	or.l	%d3,%d0             | l' = d | ~l
	not.l	%d6                 | %d6 back to l
	or.l	%d6,%d3             | d' = d | l
	not.l	%d1
	move.l	%d1,%d6
	or.l	%d4,%d1
	not.l	%d6
	or.l	%d6,%d4
	not.l	%d2
	move.l	%d2,%d6
	or.l	%d5,%d2
	not.l	%d6
	or.l	%d6,%d5
	movem.l	%d3-%d5,(%a1)
	movem.l	%d0-%d2,(%a0)
	movem.l	12(%a0),%d0-%d1     | light words 3..4
	movem.l	12(%a1),%d3-%d4     | dark  words 3..4
	not.l	%d0
	move.l	%d0,%d6
	or.l	%d3,%d0
	not.l	%d6
	or.l	%d6,%d3
	not.l	%d1
	move.l	%d1,%d6
	or.l	%d4,%d1
	not.l	%d6
	or.l	%d6,%d4
	movem.l	%d3-%d4,12(%a1)
	movem.l	%d0-%d1,12(%a0)
	adda.l	%a2,%a0
	adda.l	%a2,%a1
	dbra	%d7,0b
	movem.l	(%sp)+,%d3-%d7/%a2
	rts
