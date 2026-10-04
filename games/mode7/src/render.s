| Mode 7 - Demo 2 (David Coz, 2005): the hand-written 68000 routines, reconstructed.
| In the original they are C functions whose bodies are asm() blocks (hence the link/unlk frame of
| the first four); the code below is theirs instruction for instruction, with our comments.
| All take their parameters on the stack (TIGCC 0.96 had no register parameters).
| Syntax: GNU as, Motorola operands, registers without % (-Wa,--register-prefix-optional).
|
| Virtual screen: 30 bytes per row (the LCD layout), plane 0 (light) then plane 1 (dark) 0xF00
| bytes further. Textures: 128x128 bytes, one byte per texel, value 0..3 (bit 0 = light plane,
| bit 1 = dark plane; 0 = white).

	.text
	.globl	Mode7Far, Mode7Near, BuildTexture8, BuildTexture16
	.globl	ClearPlanes, CopyPlane, Sprite32Gray, HLine

| ---------------------------------------------------------------------------------------------
| void Mode7Far(void *dest, unsigned char *tex, short u, short v, unsigned char angle)
|
| The far part of the floor, full resolution: 15 rows of 128 pixels, from dest (row 59, distance
| index d5 = 20) up to the horizon (row 45, d5 = 6).
| For the row at distance index d5 the eye-to-floor distance is 0x3200 / d5; the row starts at
| the camera point (u, v) moved forward by that distance and 64 steps to the left, and walks
| 128 texels to the right with a step of 0x1000 / d5 (16.16 fixed point, texture coordinates =
| u, v / 4). A texel goes to the two planes as one bit each (value 1, 2 or 3).
| Registers: d4/d5 = texture position (16.16), d6/d7 = step, d3 = column byte counter,
| d2 = pixel bit, a1/a3 = destination in the light/dark planes, a0 = texture.
Mode7Far:
	link.w	a6, #0
	movea.l	8(a6), a1                        | dest (light plane)
	movea.l	12(a6), a0                       | texture 128x128
	move.w	16(a6), d2                       | u
	move.w	18(a6), d1                       | v
	clr.w	d0
	move.b	21(a6), d0                       | angle (a char passed as a word)
	movem.l	d3-d7/a2-a6, -(sp)
	andi.l	#0xffff, d0
	andi.l	#0xffff, d1
	andi.l	#0xffff, d2
	moveq	#0, d3
	moveq	#0, d4
	moveq	#0, d5
	moveq	#0, d6
	moveq	#0, d7
	moveq	#20, d5                          | distance index of the first (nearest) row
	movea.l	a1, a3
	adda.w	#0xf00, a3                       | same place in the dark plane
	lea.l	cos32k(pc), a2
	adda.w	d0, a2
	adda.w	d0, a2
	move.w	(a2), d3                         | d3 = cos(angle) * 32767
	lea.l	sin32k(pc), a2
	adda.w	d0, a2
	adda.w	d0, a2
	move.w	(a2), d4                         | d4 = sin(angle) * 32767
	lsl.l	#8, d2                           | u, v -> 16.16 texel coordinates (u / 4 << 16)
	lsl.l	#6, d2
	lsl.l	#8, d1
	lsl.l	#6, d1
.Lfar_row:
	move.l	#0x3200, d6                      | distance to this row = 0x3200 / d5
	divu.w	d5, d6
	move.w	d6, d7
	muls.w	d3, d6                           | forward vector * distance
	asr.l	#4, d6
	muls.w	d4, d7
	asr.l	#4, d7
	movem.l	d0-d5, -(sp)
	sub.l	d7, d2                           | d2/d1 = centre of the row
	add.l	d6, d1
	move.l	#0x1000, d6                      | step along the row = 0x1000 / d5
	divu.w	d5, d6
	move.l	d6, d7
	muls.w	d3, d6                           | step vector, perpendicular to the view
	muls.w	d4, d7
	asr.l	#8, d6
	asr.l	#8, d7
	move.l	d6, d4
	move.l	d7, d5
	asl.l	#6, d4                           | 64 steps back: the left end of the row
	asl.l	#6, d5
	neg.l	d4
	neg.l	d5
	add.l	d2, d4
	add.l	d1, d5
	move.w	#15, d3                          | 16 bytes = 128 pixels
	moveq	#0, d2
.Lfar_byte:
| one pixel: texel (d4 >> 16, d5 >> 16) of the 128-wide texture, bit d2 of the byte
	.irp	bit, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
	move.w	#\bit, d2
	move.l	d4, d0
	move.l	d5, d1
	swap	d0
	swap	d1
	lsl.w	#7, d1                           | row * 128 + column (wraps inside 64 KB)
	add.w	d0, d1
	move.b	(a0, d1.w), d0
	beq.b	9f                               | 0: white
	cmpi.w	#1, d0
	beq.b	1f
	cmpi.w	#2, d0
	beq.b	3f
	bra.b	2f
1:	or.b	d2, (a1)                         | 1: light
	bra.b	9f
2:	or.b	d2, (a1)                         | 3: black (both planes)
3:	or.b	d2, (a3)                         | 2: dark
9:	add.l	d6, d4                           | next texel
	add.l	d7, d5
	.endr
	addq.w	#1, a1
	addq.w	#1, a3
	dbra	d3, .Lfar_byte
	suba.w	#46, a1                          | back 16 bytes and up one row (30)
	suba.w	#46, a3
	movem.l	(sp)+, d0-d5
	subq.w	#1, d5                           | next row, farther
	cmpi.w	#5, d5
	bgt.w	.Lfar_row
	movem.l	(sp)+, d3-d7/a2-a6
	unlk	a6
	rts

| ---------------------------------------------------------------------------------------------
| void Mode7Near(void *dest, unsigned char *tex, short u, short v, unsigned char angle)
|
| The near part of the floor at half resolution: 20 rows of 2x1 pixels (distance index 60 down
| to 22, step 2) drawn every other line from dest (row 98) up to row 60; the C caller copies
| each row to the line below. Same method as Mode7Far with 0x6400 / d5, a step twice as long
| and two bits (0xC0, 0x30, 0x0C, 0x03) per texel.
Mode7Near:
	link.w	a6, #0
	movea.l	8(a6), a1
	movea.l	12(a6), a0
	move.w	16(a6), d2
	move.w	18(a6), d1
	clr.w	d0
	move.b	21(a6), d0
	movem.l	d3-d7/a2-a6, -(sp)
	andi.l	#0xffff, d0
	andi.l	#0xffff, d1
	andi.l	#0xffff, d2
	moveq	#0, d3
	moveq	#0, d4
	moveq	#0, d5
	moveq	#0, d6
	moveq	#0, d7
	moveq	#60, d5
	movea.l	a1, a3
	adda.w	#0xf00, a3
	lea.l	cos32k(pc), a2
	adda.w	d0, a2
	adda.w	d0, a2
	move.w	(a2), d3
	lea.l	sin32k(pc), a2
	adda.w	d0, a2
	adda.w	d0, a2
	move.w	(a2), d4
	lsl.l	#8, d2
	lsl.l	#6, d2
	lsl.l	#8, d1
	lsl.l	#6, d1
.Lnear_row:
	move.l	#0x6400, d6
	divu.w	d5, d6
	move.w	d6, d7
	muls.w	d3, d6
	asr.l	#4, d6
	muls.w	d4, d7
	asr.l	#4, d7
	movem.l	d0-d5, -(sp)
	sub.l	d7, d2
	add.l	d6, d1
	move.l	#0x1000, d6
	divu.w	d5, d6
	move.l	d6, d7
	muls.w	d3, d6
	muls.w	d4, d7
	asr.l	#7, d6                           | step of a half-resolution pixel...
	asr.l	#7, d7
	move.l	d6, d4
	move.l	d7, d5
	asl.l	#6, d4                           | ...64 of them to the left end
	asl.l	#6, d5
	neg.l	d4
	neg.l	d5
	add.l	d2, d4
	add.l	d1, d5
	add.l	d6, d6                           | then one texel per 2 pixels
	add.l	d7, d7
	move.w	#15, d3
	moveq	#0, d2
.Lnear_byte:
	.irp	bits, 0xc0, 0x30, 0x0c, 0x03
	move.w	#\bits, d2
	move.l	d4, d0
	move.l	d5, d1
	swap	d0
	swap	d1
	lsl.w	#7, d1
	add.w	d0, d1
	move.b	(a0, d1.w), d0
	beq.b	9f
	cmpi.w	#1, d0
	beq.b	1f
	cmpi.w	#2, d0
	beq.b	3f
	bra.b	2f
1:	or.b	d2, (a1)
	bra.b	9f
2:	or.b	d2, (a1)
3:	or.b	d2, (a3)
9:	add.l	d6, d4
	add.l	d7, d5
	.endr
	addq.w	#1, a1
	addq.w	#1, a3
	dbra	d3, .Lnear_byte
	suba.w	#76, a1                          | back 16 bytes and up two rows
	suba.w	#76, a3
	movem.l	(sp)+, d0-d5
	subq.w	#2, d5
	cmpi.w	#20, d5
	bgt.w	.Lnear_row
	movem.l	(sp)+, d3-d7/a2-a6
	unlk	a6
	rts

| ---------------------------------------------------------------------------------------------
| void BuildTexture8(unsigned char *map, unsigned char *tiles, unsigned char *tex)
| Far texture: 16x16 map cells (map row stride 64) of 8x8 tiles (64 bytes each) -> 128x128.
BuildTexture8:
	link.w	a6, #0
	move.l	a2, -(sp)
	movea.l	8(a6), a2
	movea.l	12(a6), a1
	movea.l	16(a6), a0
	movem.l	d0-d7/a0-a6, -(sp)
	movea.l	a0, a4
	movea.l	a1, a5
	movea.l	a2, a6                           | a6 = map
	movea.l	a5, a1                           | a1 = tiles
	movea.l	a4, a0                           | a0 = texture
	move.w	#15, d1                          | 16 rows of tiles
	clr.w	d2
	clr.w	d3                               | d3 = map index
.Lt8_row:
	move.w	#15, d0                          | 16 tiles per row
	movea.l	a0, a3
.Lt8_tile:
	clr.w	d2
	move.b	(a6, d3.w), d2
	lsl.w	#6, d2
	lea.l	(a1, d2.w), a2                   | the tile
	.irp	off, 0, 0x80, 0x100, 0x180, 0x200, 0x280, 0x300, 0x380
	move.l	(a2)+, \off(a3)                  | 8 bytes per texture row (128 bytes)
	move.l	(a2)+, \off+4(a3)
	.endr
	addq.w	#8, a3
	addq.w	#1, d3
	dbra	d0, .Lt8_tile
	adda.w	#0x400, a0                       | 8 texture rows further
	addi.w	#48, d3                          | next map row (64 - 16)
	dbra	d1, .Lt8_row
	movem.l	(sp)+, d0-d7/a0-a6
	movea.l	(sp)+, a2
	unlk	a6
	rts

| ---------------------------------------------------------------------------------------------
| void BuildTexture16(unsigned char *map, unsigned char *tiles, unsigned char *tex)
| Near texture: 8x8 map cells of 16x16 tiles (256 bytes each) -> 128x128.
BuildTexture16:
	link.w	a6, #0
	move.l	a2, -(sp)
	movea.l	8(a6), a2
	movea.l	12(a6), a1
	movea.l	16(a6), a0
	movem.l	d0-d7/a0-a6, -(sp)
	movea.l	a0, a4
	movea.l	a1, a5
	movea.l	a2, a6
	movea.l	a5, a1
	movea.l	a4, a0
	move.w	#7, d1
	clr.w	d2
	clr.w	d3
.Lt16_row:
	move.w	#7, d0
	movea.l	a0, a4
.Lt16_tile:
	movea.l	a4, a3
	move.b	(a6, d3.w), d2
	lsl.w	#8, d2
	lea.l	(a1, d2.w), a2
	.rept	3
	.irp	off, 0, 0x80, 0x100, 0x180
	move.l	(a2)+, \off(a3)                  | 16 bytes per texture row
	move.l	(a2)+, \off+4(a3)
	move.l	(a2)+, \off+8(a3)
	move.l	(a2)+, \off+12(a3)
	.endr
	adda.w	#0x200, a3                       | 4 rows done
	.endr
	.irp	off, 0, 0x80, 0x100, 0x180
	move.l	(a2)+, \off(a3)
	move.l	(a2)+, \off+4(a3)
	move.l	(a2)+, \off+8(a3)
	move.l	(a2)+, \off+12(a3)
	.endr
	adda.w	#16, a4
	addq.l	#1, d3
	dbra	d0, .Lt16_tile
	adda.w	#0x800, a0
	addi.w	#56, d3                          | next map row (64 - 8)
	dbra	d1, .Lt16_row
	movem.l	(sp)+, d0-d7/a0-a6
	movea.l	(sp)+, a2
	unlk	a6
	rts

| ---------------------------------------------------------------------------------------------
| void ClearPlanes(void *light, void *dark): clears 0xF00 bytes of each, backwards, 40 bytes
| per movem.
ClearPlanes:
	movea.l	4(sp), a0
	movea.l	8(sp), a1
	movem.l	d3-d7/a2-a4, -(sp)
	lea.l	0xf00(a0), a0
	lea.l	0xf00(a1), a1
	moveq	#0, d0
	moveq	#0, d1
	moveq	#0, d2
	moveq	#0, d3
	moveq	#0, d4
	moveq	#0, d5
	moveq	#0, d6
	moveq	#23, d7                          | 24 * 4 * 40 = 3840
	movea.l	d0, a2
	movea.l	d0, a3
	movea.l	d0, a4
1:	.rept	4
	movem.l	d0-d6/a2-a4, -(a0)
	movem.l	d0-d6/a2-a4, -(a1)
	.endr
	dbra	d7, 1b
	movem.l	(sp)+, d3-d7/a2-a4
	rts

| ---------------------------------------------------------------------------------------------
| void CopyPlane(void *src, void *dest): copies 0xF00 bytes, 48 per movem (the virtual screen
| to a grayscale plane).
CopyPlane:
	movea.l	4(sp), a0
	movea.l	8(sp), a1
	moveq	#9, d0                           | 10 * 8 * 48 = 3840
	movem.l	d3-d7/a2-a6, -(sp)
1:	.irp	off, 0, 0x30, 0x60, 0x90, 0xc0, 0xf0, 0x120, 0x150
	movem.l	(a0)+, d1-d7/a2-a6
	movem.l	d1-d7/a2-a6, \off(a1)
	.endr
	lea.l	0x180(a1), a1
	dbra	d0, 1b
	movem.l	(sp)+, d3-d7/a2-a6
	rts

| ---------------------------------------------------------------------------------------------
| void Sprite32Gray(short x, short y, short h, const void *light, const void *dark,
|                   const void *mask_light, const void *mask_dark, void *plane0, void *plane1)
| A 32-pixel wide masked grayscale sprite, h rows (4 bytes per row for each of the 4 arrays),
| no clipping. Each row: plane &= mask (rotated to x), plane |= sprite.
Sprite32Gray:
	movem.l	d3-d5/a2-a5, -(sp)
	lea.l	32(sp), a1
	move.w	(a1)+, d0                        | x
	move.w	(a1)+, d1                        | y
	move.w	(a1)+, d2                        | h
	beq.b	9f
	movea.l	(a1)+, a2                        | light
	movea.l	(a1)+, a3                        | dark
	movea.l	(a1)+, a4                        | mask light
	movea.l	(a1)+, a5                        | mask dark
	movea.l	(a1)+, a0                        | plane 0
	movea.l	(a1), a1                         | plane 1
	move.w	d1, d3                           | offset = y * 30 + x / 16 * 2
	lsl.w	#4, d1
	sub.w	d3, d1
	move.w	d0, d3
	lsr.w	#4, d3
	add.w	d1, d3
	add.w	d3, d3
	adda.w	d3, a0
	adda.w	d3, a1
	subq.w	#1, d2
	andi.w	#15, d0
	moveq	#16, d1
	sub.w	d0, d1                           | d1 = rotation (16 - x % 16)
	moveq	#-1, d3
	lsl.l	d1, d3                           | d3 = keep mask for the 48-bit span
	move.l	d3, d4
	not.l	d4
1:	move.l	(a4)+, d0                        | mask, plane 0
	rol.l	d1, d0
	move.w	d0, d5
	or.w	d3, d5
	and.w	d5, (a0)+
	or.l	d4, d0
	and.l	d0, (a0)
	move.l	(a5)+, d0                        | mask, plane 1
	rol.l	d1, d0
	move.w	d0, d5
	or.w	d3, d5
	and.w	d5, (a1)+
	or.l	d4, d0
	and.l	d0, (a1)
	move.l	(a2)+, d0                        | sprite, plane 0
	rol.l	d1, d0
	move.w	d0, d5
	and.l	d3, d0
	or.l	d0, (a0)
	and.w	d4, d5
	or.w	d5, -(a0)
	move.l	(a3)+, d0                        | sprite, plane 1
	rol.l	d1, d0
	move.w	d0, d5
	and.l	d3, d0
	or.l	d0, (a1)
	and.w	d4, d5
	or.w	d5, -(a1)
	lea.l	30(a0), a0
	lea.l	30(a1), a1
	dbra	d2, 1b
9:	movem.l	(sp)+, d3-d5/a2-a5
	rts

| the end masks of HLine, before it as in the original: (d8,pc,Xn) reaches only 127 bytes
hline_left:                                      | bits from x % 16 to the right end
	.word	0xffff, 0x7fff, 0x3fff, 0x1fff, 0x0fff, 0x07ff, 0x03ff, 0x01ff
	.word	0x00ff, 0x007f, 0x003f, 0x001f, 0x000f, 0x0007, 0x0003, 0x0001
hline_right:                                     | bits from the left end to x % 16
	.word	0x8000, 0xc000, 0xe000, 0xf000, 0xf800, 0xfc00, 0xfe00, 0xff00
	.word	0xff80, 0xffc0, 0xffe0, 0xfff0, 0xfff8, 0xfffc, 0xfffe, 0xffff

| ---------------------------------------------------------------------------------------------
| void HLine(void *plane, short x1, short x2, short y, short mode)
| Horizontal line from x1 to x2 (any order) on row y, 30 bytes per row; mode 0 clears,
| 1 sets, 2 inverts. Words at the ends, longs in the middle.
HLine:
	move.l	d3, -(sp)                        | fixed: the original saves d3 with move.w but its
	movea.l	8(sp), a0                        | moveq #32, d3 clears the high word; its caller only
	move.w	12(sp), d0                       | used the low one, GCC4TI's code keeps longs in d3
	move.w	14(sp), d1
	move.w	16(sp), d2
	cmp.w	d0, d1
	bge.b	1f
	exg.l	d0, d1                           | d0 = left, d1 = right
1:	move.w	d2, d3                           | a0 += y * 30 + left / 16 * 2
	lsl.w	#4, d2
	sub.w	d3, d2
	move.w	d0, d3
	lsr.w	#4, d3
	add.w	d3, d2
	add.w	d2, d2
	adda.w	d2, a0
	lsl.w	#4, d3
	addi.w	#16, d3                          | d3 = x of the second word
	move.w	d1, d2
	andi.w	#15, d0
	add.w	d0, d0
	move.w	hline_left(pc, d0.w), d0         | left word mask
	andi.w	#15, d1
	add.w	d1, d1
	move.w	hline_right(pc, d1.w), d1        | right word mask
	cmp.w	d3, d2
	blt.b	.Lhl_one                         | both ends in the same word
	sub.w	d3, d2
	move.w	18(sp), d3
	beq.b	.Lhl_clear
	cmpi.w	#2, d3
	beq.b	.Lhl_xor
	moveq	#32, d3                          | set
	or.w	d0, (a0)+
	moveq	#-1, d0
	sub.w	d3, d2
	blt.b	2f
1:	move.l	d0, (a0)+
	sub.w	d3, d2
	bge.b	1b
2:	cmpi.w	#-16, d2
	blt.b	3f
	move.w	d0, (a0)+
3:	or.w	d1, (a0)
	move.l	(sp)+, d3
	rts
.Lhl_clear:
	moveq	#32, d3
	not.w	d0
	and.w	d0, (a0)+
	moveq	#0, d0
	sub.w	d3, d2
	blt.b	2f
1:	move.l	d0, (a0)+
	sub.w	d3, d2
	bge.b	1b
2:	cmpi.w	#-16, d2
	blt.b	3f
	move.w	d0, (a0)+
3:	not.w	d1
	and.w	d1, (a0)
	move.l	(sp)+, d3
	rts
.Lhl_xor:
	moveq	#32, d3
	eor.w	d0, (a0)+
	sub.w	d3, d2
	blt.b	2f
1:	not.l	(a0)+
	sub.w	d3, d2
	bge.b	1b
2:	cmpi.w	#-16, d2
	blt.b	3f
	not.w	(a0)+
3:	eor.w	d1, (a0)
	move.l	(sp)+, d3
	rts
.Lhl_one:
	and.w	d0, d1
	move.w	18(sp), d3
	beq.b	1f
	cmpi.w	#2, d3
	beq.b	2f
	or.w	d1, (a0)
	move.l	(sp)+, d3
	rts
1:	not.w	d1
	and.w	d1, (a0)
	move.l	(sp)+, d3
	rts
2:	eor.w	d1, (a0)
	move.l	(sp)+, d3
	rts

| ---------------------------------------------------------------------------------------------
| void Span2(void *plane0, short x1, short x2, short y, short color)          (added, optimised)
| One span of a flat triangle on both planes (plane 1 = plane0 + 0xF00): colour bit 0 sets or
| clears plane 0, bit 1 plane 1. Same pixels as HLine(plane0, .., color & 1) then
| HLine(plane1, .., color >> 1 & 1), with the address and the masks computed once.
span_left:                                       | before Span2: (d8,pc,Xn) reach
	.word	0xffff, 0x7fff, 0x3fff, 0x1fff, 0x0fff, 0x07ff, 0x03ff, 0x01ff
	.word	0x00ff, 0x007f, 0x003f, 0x001f, 0x000f, 0x0007, 0x0003, 0x0001
span_right:
	.word	0x8000, 0xc000, 0xe000, 0xf000, 0xf800, 0xfc00, 0xfe00, 0xff00
	.word	0xff80, 0xffc0, 0xffe0, 0xfff0, 0xfff8, 0xfffc, 0xfffe, 0xffff
	.globl	Span2
Span2:
	movem.l	d3-d5, -(sp)
	movea.l	16(sp), a0
	move.w	20(sp), d0                       | x1
	move.w	22(sp), d1                       | x2
	cmp.w	d0, d1
	bge.b	1f
	exg.l	d0, d1
1:	move.w	24(sp), d2                       | a0 += y * 30 + x1 / 16 * 2
	move.w	d2, d3
	lsl.w	#4, d2
	sub.w	d3, d2
	move.w	d0, d3
	lsr.w	#4, d3
	add.w	d3, d2
	add.w	d2, d2
	adda.w	d2, a0
	move.w	d1, d2                           | d2 = words spanned - 1
	lsr.w	#4, d2
	sub.w	d3, d2
	andi.w	#15, d0
	add.w	d0, d0
	move.w	span_left(pc, d0.w), d0
	andi.w	#15, d1
	add.w	d1, d1
	move.w	span_right(pc, d1.w), d1
	move.w	26(sp), d5
	movea.l	a0, a1
	btst	#0, d5
	sne	d4
	bsr.b	.Lsp_plane
	lea.l	0xf00(a0), a1
	btst	#1, d5
	sne	d4
	bsr.b	.Lsp_plane
	movem.l	(sp)+, d3-d5
	rts
| a1 = first word, d0/d1 = left/right masks, d2 = words - 1, d4.b = 0 (clear) or -1 (set)
.Lsp_plane:
	tst.w	d2
	bne.b	.Lsp_multi
	move.w	d0, d3
	and.w	d1, d3
	tst.b	d4
	beq.b	1f
	or.w	d3, (a1)
	rts
1:	not.w	d3
	and.w	d3, (a1)
	rts
.Lsp_multi:
	move.w	d2, d3
	subq.w	#1, d3                           | full words in the middle
	tst.b	d4
	beq.b	.Lsp_clear
	or.w	d0, (a1)+
	bra.b	2f
1:	move.w	#-1, (a1)+
2:	dbra	d3, 1b
	or.w	d1, (a1)
	rts
.Lsp_clear:
	move.w	d0, d4
	not.w	d4
	and.w	d4, (a1)+
	bra.b	2f
1:	clr.w	(a1)+
2:	dbra	d3, 1b
	move.w	d1, d4
	not.w	d4
	and.w	d4, (a1)
	rts

| ---------------------------------------------------------------------------------------------
| long TriSpans(void *vs, short y, short yend, short fa, short fb, short sa, short sb, short color)
|                                                                            (added, optimised)
| The scanline loop of FillTri for one half of a triangle: for y = y .. yend - 1, the span
| between fa >> 5 and fb >> 5 (clamped to 0..127) when y >= 0, then fa += sa, fb += sb.
| Returns fa << 16 | fb (unsigned 16-bit) so that the second half continues the edges.
| Same pixels as the C loop calling Span2 (colour bit 0: plane at vs, bit 1: plane at +0xF00).
| One copy of the loop per colour (no colour test per line), the clamp as one unsigned compare,
| the mask tables in a6, the middle words of both planes in one loop.
| ts_clamp reg: reg = clamp(reg, 0, 127), reg already >> 5, flags of cmpi #127 on the rare path
	.macro	ts_clamp reg
	cmpi.w	#127, \reg
	bls.b	6f                               | (6: the loop uses 1..5, 8, 9)
	spl	\reg                             | > 127: 127, negative: 0
	ext.w	\reg
	andi.w	#127, \reg
6:
	.endm
| ts_word dst, mask, set: one word of one plane (set: or, clear: and not; not.w mask is undone)
	.macro	ts_op set, mask, dst
	.if	\set
	or.w	\mask, \dst
	.else
	not.w	\mask
	and.w	\mask, \dst
	not.w	\mask
	.endif
	.endm
	.macro	ts_loop p0, p1
	bra.w	9f
1:	tst.w	d4
	blt.w	8f
	move.w	d0, d2                           | xa, xb
	asr.w	#5, d2
	ts_clamp d2
	move.w	d1, d3
	asr.w	#5, d3
	ts_clamp d3
	cmp.w	d2, d3
	bge.b	2f
	exg.l	d2, d3
2:	move.w	d2, d7                           | d7 = first word, d5 = words - 1
	lsr.w	#4, d7
	move.w	d3, d5
	lsr.w	#4, d5
	sub.w	d7, d5
	andi.w	#15, d2
	add.w	d2, d2
	move.w	(a6, d2.w), d2                   | left mask
	andi.w	#15, d3
	add.w	d3, d3
	move.w	32(a6, d3.w), d3                 | right mask
	add.w	d7, d7
	lea.l	(a0, d7.w), a1
	tst.w	d5
	bne.b	3f
	and.w	d2, d3                           | one word
	ts_op	\p1, d3, 0xf00(a1)
	ts_op	\p0, d3, (a1)
	bra.b	8f
3:	ts_op	\p1, d2, 0xf00(a1)               | first word
	ts_op	\p0, d2, (a1)+
	subq.w	#1, d5                           | full words in the middle
	bra.b	5f
4:	.if	\p1
	move.w	#-1, 0xf00(a1)
	.else
	clr.w	0xf00(a1)
	.endif
	.if	\p0
	move.w	#-1, (a1)+
	.else
	clr.w	(a1)+
	.endif
5:	dbra	d5, 4b
	ts_op	\p1, d3, 0xf00(a1)               | last word
	ts_op	\p0, d3, (a1)
8:	add.w	a3, d0
	add.w	a4, d1
	lea.l	30(a0), a0
	addq.w	#1, d4
9:	cmp.w	a5, d4
	blt.w	1b
	bra.w	.Lts_done
	.endm

	.globl	TriSpans
TriSpans:
	movem.l	d3-d7/a2-a6, -(sp)               | 40 bytes
	movea.l	44(sp), a0                       | vs
	move.w	48(sp), d4                       | y
	movea.w	50(sp), a5                       | yend
	move.w	52(sp), d0                       | fa
	move.w	54(sp), d1                       | fb
	movea.w	56(sp), a3                       | sa
	movea.w	58(sp), a4                       | sb
	move.w	60(sp), d6                       | colour
	lea.l	span_left(pc), a6                | masks: left at 0, right at 32
	move.w	d4, d2                           | a0 = row y (y may be negative)
	muls.w	#30, d2
	adda.l	d2, a0
	andi.w	#3, d6
	add.w	d6, d6
	move.w	.Lts_jump(pc, d6.w), d6
	jmp	.Lts_jump(pc, d6.w)
.Lts_jump:
	.word	.Lts_c0 - .Lts_jump, .Lts_c1 - .Lts_jump, .Lts_c2 - .Lts_jump, .Lts_c3 - .Lts_jump
.Lts_c0: ts_loop 0, 0
.Lts_c1: ts_loop 1, 0
.Lts_c2: ts_loop 0, 1
.Lts_c3: ts_loop 1, 1
.Lts_done:
	swap	d0
	move.w	d1, d0
	movem.l	(sp)+, d3-d7/a2-a6
	rts

| =============================================================================================
| Optimised floor (added). One 32 KB texture block, 128 rows of 256 bytes: the near texture in
| columns 0..127, the far one in columns 128..255, so that a texel index is v << 8 | u and needs
| no shift: swap + move.b instead of lsl.w #7 (20 cycles). Texels are stored with their plane bits
| at the top of the byte (far: bit 7 dark, bit 6 light; near, already doubled for its 2-pixel
| wide texels: 4 of them per byte, each bit doubled by a 16-byte table) and shifted into two byte
| accumulators with
| add.b / addx.b, written once per 8 pixels with move: no compare chain, no or.b to memory per
| pixel, and the screen needs no clearing. The sampling (16.16 u and v, the steps, the rows) is
| the original's instruction for instruction, v being kept as v << 8 (8.24) for the index.
| ---------------------------------------------------------------------------------------------
| Packed stepping (from rwill's "Just some small effects", knowledge base game techniques §14):
| the texel index d1.w = v_int << 8 | u_int is stepped as a whole: the u fraction's carry goes in
| with addx.w, the v fraction's carry adds 0x100. 20-22 cycles per pixel instead of 36 for the two
| add.l and the swap / move.b index. The index is linear (v * 256 + u): it differs from the
| original's per-byte wrap only when u crosses a multiple of 256, i.e. outside the texture.
| PACK_STEPS fu, fv: from u, v (d4, d5) and their steps (d6, d7) in 16.16, set d1 = the index,
| d4 / d5 = the fractions, fu / fv = the step fractions, d6 = dv_int << 8 + du_int, d7 = 0x100.
	.macro	PACK_STEPS fu, fv
	move.l	d5, d1
	swap	d1
	lsl.w	#8, d1
	move.l	d4, d0
	swap	d0
	move.b	d0, d1
	movea.w	d6, \fu
	movea.w	d7, \fv
	swap	d6
	swap	d7
	lsl.w	#8, d7
	add.w	d7, d6
	move.w	#0x100, d7
	.endm
| STEP fu, fv: next texel. add.w An,Dn sets X like any add.
	.macro	STEP fu, fv
	add.w	\fu, d4                          |  4  u fraction, X = its carry
	addx.w	d6, d1                           |  4  index += dv_int << 8 + du_int + X
	add.w	\fv, d5                          |  4  v fraction
	bcc.s	1f                               | 10 (8 when it falls through)
	add.w	d7, d1                           |  4  its carry: next row
1:
	.endm
| void Mode7FarFast(void *dest, unsigned char *tex, short u, short v, unsigned char angle)
|   tex = the block + 128. ~53 cycles per pixel (~66 before the packed stepping) (the original: ~88 on white, ~115 otherwise).
	.globl	Mode7FarFast, Mode7NearFast, BuildTexture8W, BuildTexture16W, SkyCopy
| per-row quotients of the floor routines (the original divides twice per row), index = the
| row's distance index (far 6..20, near 22..60)
far_dist:
	.word	0, 12800, 6400, 4266, 3200, 2560, 2133, 1828, 1600, 1422
	.word	1280, 1163, 1066, 984, 914, 853, 800, 752, 711, 673
	.word	640
far_step:
	.word	0, 4096, 2048, 1365, 1024, 819, 682, 585, 512, 455
	.word	409, 372, 341, 315, 292, 273, 256, 240, 227, 215
	.word	204
near_dist:
	.word	0, 25600, 12800, 8533, 6400, 5120, 4266, 3657, 3200, 2844
	.word	2560, 2327, 2133, 1969, 1828, 1706, 1600, 1505, 1422, 1347
	.word	1280, 1219, 1163, 1113, 1066, 1024, 984, 948, 914, 882
	.word	853, 825, 800, 775, 752, 731, 711, 691, 673, 656
	.word	640, 624, 609, 595, 581, 568, 556, 544, 533, 522
	.word	512, 501, 492, 483, 474, 465, 457, 449, 441, 433
	.word	426
near_step:
	.word	0, 4096, 2048, 1365, 1024, 819, 682, 585, 512, 455
	.word	409, 372, 341, 315, 292, 273, 256, 240, 227, 215
	.word	204, 195, 186, 178, 170, 163, 157, 151, 146, 141
	.word	136, 132, 128, 124, 120, 117, 113, 110, 107, 105
	.word	102, 99, 97, 95, 93, 91, 89, 87, 85, 83
	.word	81, 80, 78, 77, 75, 74, 73, 71, 70, 69
	.word	68
Mode7FarFast:
	link.w	a6, #0
	movea.l	8(a6), a1
	movea.l	12(a6), a0
	move.w	16(a6), d2
	move.w	18(a6), d1
	clr.w	d0
	move.b	21(a6), d0
	movem.l	d3-d7/a2-a6, -(sp)
	andi.l	#0xffff, d0
	andi.l	#0xffff, d1
	andi.l	#0xffff, d2
	moveq	#20, d5
	lea.l	cos32k(pc), a2
	adda.w	d0, a2
	adda.w	d0, a2
	move.w	(a2), d3
	lea.l	sin32k(pc), a2
	adda.w	d0, a2
	adda.w	d0, a2
	move.w	(a2), d4
	lsl.l	#8, d2
	lsl.l	#6, d2
	lsl.l	#8, d1
	lsl.l	#6, d1
.Lff_row:                                        | row set-up: the original's
	move.w	d5, d6                           | distance = 0x3200 / d5, from the table
	add.w	d6, d6
	lea.l	far_dist(pc), a2
	move.w	(a2, d6.w), d6
	move.w	d6, d7
	muls.w	d3, d6
	asr.l	#4, d6
	muls.w	d4, d7
	asr.l	#4, d7
	movem.l	d0-d5, -(sp)
	sub.l	d7, d2
	add.l	d6, d1
	move.w	d5, d6                           | step = 0x1000 / d5, from the table
	add.w	d6, d6
	lea.l	far_step(pc), a2
	move.w	(a2, d6.w), d6
	move.l	d6, d7
	muls.w	d3, d6
	muls.w	d4, d7
	asr.l	#8, d6
	asr.l	#8, d7
	move.l	d6, d4
	move.l	d7, d5
	asl.l	#6, d4
	asl.l	#6, d5
	neg.l	d4
	neg.l	d5
	add.l	d2, d4
	add.l	d1, d5
	PACK_STEPS a3, a4                        | index d1 = v << 8 | u, fractions in d4 / d5
	lea.l	16(a1), a2                       | end of the row
.Lff_byte:
	.rept	8
	move.b	(a0, d1.w), d0                   | 14
	add.b	d0, d0                           |  4  bit 7: dark
	addx.b	d3, d3                           |  4
	add.b	d0, d0                           |  4  bit 6: light
	addx.b	d2, d2                           |  4
	STEP	a3, a4                           | 20-22
	.endr
	move.b	d3, 0xf00(a1)
	move.b	d2, (a1)+
	cmpa.l	a2, a1
	bne.w	.Lff_byte
	suba.w	#46, a1
	movem.l	(sp)+, d0-d5
	subq.w	#1, d5
	cmpi.w	#5, d5
	bgt.w	.Lff_row
	movem.l	(sp)+, d3-d7/a2-a6
	unlk	a6
	rts

| nibble abcd -> byte aabbccdd
nibble_double:
	.byte	0x00, 0x03, 0x0c, 0x0f, 0x30, 0x33, 0x3c, 0x3f, 0xc0, 0xc3, 0xcc, 0xcf, 0xf0, 0xf3, 0xfc, 0xff
| ---------------------------------------------------------------------------------------------
| void Mode7NearFast(void *dest, unsigned char *tex, short u, short v, unsigned char angle)
|   tex = the block. Writes each row and the line below it (the original's memcpy pass).
Mode7NearFast:
	link.w	a6, #0
	movea.l	8(a6), a1
	movea.l	12(a6), a0
	move.w	16(a6), d2
	move.w	18(a6), d1
	clr.w	d0
	move.b	21(a6), d0
	movem.l	d3-d7/a2-a6, -(sp)
	andi.l	#0xffff, d0
	andi.l	#0xffff, d1
	andi.l	#0xffff, d2
	moveq	#60, d5
	lea.l	nibble_double(pc), a3
	lea.l	cos32k(pc), a2
	adda.w	d0, a2
	adda.w	d0, a2
	move.w	(a2), d3
	lea.l	sin32k(pc), a2
	adda.w	d0, a2
	adda.w	d0, a2
	move.w	(a2), d4
	lsl.l	#8, d2
	lsl.l	#6, d2
	lsl.l	#8, d1
	lsl.l	#6, d1
.Lnf_row:
	move.w	d5, d6                           | distance = 0x6400 / d5, from the table
	add.w	d6, d6
	lea.l	near_dist(pc), a2
	move.w	(a2, d6.w), d6
	move.w	d6, d7
	muls.w	d3, d6
	asr.l	#4, d6
	muls.w	d4, d7
	asr.l	#4, d7
	movem.l	d0-d5, -(sp)
	sub.l	d7, d2
	add.l	d6, d1
	move.w	d5, d6                           | step = 0x1000 / d5, from the table
	add.w	d6, d6
	lea.l	near_step(pc), a2
	move.w	(a2, d6.w), d6
	move.l	d6, d7
	muls.w	d3, d6
	muls.w	d4, d7
	asr.l	#7, d6
	asr.l	#7, d7
	move.l	d6, d4
	move.l	d7, d5
	asl.l	#6, d4
	asl.l	#6, d5
	neg.l	d4
	neg.l	d5
	add.l	d2, d4
	add.l	d1, d5
	add.l	d6, d6
	add.l	d7, d7
	PACK_STEPS a4, a5
	lea.l	16(a1), a2
.Lnf_byte:
	.rept	4
	move.b	(a0, d1.w), d0
	add.b	d0, d0                           | bit 7: the plane at +0xF00
	addx.b	d3, d3
	add.b	d0, d0                           | bit 6: the plane at dest
	addx.b	d2, d2
	STEP	a4, a5
	.endr
	andi.w	#15, d3                          | 4 texels: each bit doubled (2-pixel texels)
	move.b	(a3, d3.w), d3
	andi.w	#15, d2
	move.b	(a3, d2.w), d2
	move.b	d3, 0xf00(a1)                    | the row and the line below it, both planes
	move.b	d3, 0xf1e(a1)
	move.b	d2, 30(a1)
	move.b	d2, (a1)+
	cmpa.l	a2, a1
	bne.w	.Lnf_byte
	suba.w	#76, a1
	movem.l	(sp)+, d0-d5
	subq.w	#2, d5
	cmpi.w	#20, d5
	bgt.w	.Lnf_row
	movem.l	(sp)+, d3-d7/a2-a6
	unlk	a6
	rts

| ---------------------------------------------------------------------------------------------
| void BuildTexture8W(unsigned char *map, unsigned char *tiles, unsigned char *tex)
| void BuildTexture16W(unsigned char *map, unsigned char *tiles, unsigned char *tex)
| BuildTexture8 / 16 for the 256-byte texture rows.
BuildTexture8W:
	movem.l	d3/a2-a4, -(sp)
	movea.l	20(sp), a1                       | map
	movea.l	24(sp), a2                       | tiles
	movea.l	28(sp), a0                       | texture
	moveq	#15, d1
	moveq	#0, d3
1:	moveq	#15, d0
	movea.l	a0, a3
2:	moveq	#0, d2
	move.b	(a1, d3.w), d2
	lsl.w	#6, d2
	lea.l	(a2, d2.w), a4
	.irp	off, 0, 0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x700
	move.l	(a4)+, \off(a3)
	move.l	(a4)+, \off+4(a3)
	.endr
	addq.w	#8, a3
	addq.w	#1, d3
	dbra	d0, 2b
	adda.w	#0x800, a0
	addi.w	#48, d3
	dbra	d1, 1b
	movem.l	(sp)+, d3/a2-a4
	rts
BuildTexture16W:
	movem.l	d3/a2-a4, -(sp)
	movea.l	20(sp), a1
	movea.l	24(sp), a2
	movea.l	28(sp), a0
	moveq	#7, d1
	moveq	#0, d3
1:	moveq	#7, d0
	movea.l	a0, a3
2:	moveq	#0, d2
	move.b	(a1, d3.w), d2
	lsl.w	#8, d2
	lea.l	(a2, d2.w), a4
	.irp	off, 0, 0x100, 0x200, 0x300, 0x400, 0x500, 0x600, 0x700, 0x800, 0x900, 0xa00, 0xb00, 0xc00, 0xd00, 0xe00, 0xf00
	move.l	(a4)+, \off(a3)
	move.l	(a4)+, \off+4(a3)
	move.l	(a4)+, \off+8(a3)
	move.l	(a4)+, \off+12(a3)
	.endr
	adda.w	#16, a3
	addq.w	#1, d3
	dbra	d0, 2b
	adda.w	#0x1000, a0
	addi.w	#56, d3
	dbra	d1, 1b
	movem.l	(sp)+, d3/a2-a4
	rts

| ---------------------------------------------------------------------------------------------
| void SkyCopy(const void *src0, const void *src1, void *dest, short rows): the left 16 bytes of
| each row, src0 (30-byte rows) to the plane at dest, src1 to the one at dest + 0xF00, movem
SkyCopy:
	movem.l	d3-d4/a2, -(sp)
	movea.l	16(sp), a0
	movea.l	20(sp), a2
	movea.l	24(sp), a1
	move.w	28(sp), d4
	subq.w	#1, d4
1:	movem.l	(a0)+, d0-d3
	movem.l	d0-d3, (a1)
	movem.l	(a2)+, d0-d3
	movem.l	d0-d3, 0xf00(a1)
	lea.l	14(a0), a0
	lea.l	14(a2), a2
	lea.l	30(a1), a1
	dbra	d4, 1b
	movem.l	(sp)+, d3-d4/a2
	rts
