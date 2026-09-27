| unpack68k.s — LZ4 block and ZX0 v2 decoders in 68000 asm, same formats and results as the C
| decoders of experiments/compress/unpack.c (measured there). C prototypes: unpack68k.h; packer on
| the PC: lib/zx0pack.py (ZX0), experiments/compress/tools/pack.py (LZ4).
| Both return the end of the output (in a0 and d0). They save d3-d4/a2-a3.

| Forward copy of \n bytes (n >= 1) from \s to \d; \t is a scratch data register.
| Long moves when the run is >= 8 bytes and s and d have the same parity (a long access at an
| odd address is an address error); safe for overlapping matches when d - s >= 4.
.macro COPY s, d, n, t
	cmp.w	#8,\n
	bcs.s	3f
	move.w	\d,\t
	sub.w	\s,\t
	btst	#0,\t
	bne.s	3f
	move.w	\d,\t
	btst	#0,\t
	beq.s	1f
	move.b	(\s)+,(\d)+
	subq.w	#1,\n
1:	move.w	\n,\t
	lsr.w	#2,\t
	subq.w	#1,\t
2:	move.l	(\s)+,(\d)+
	dbra	\t,2b
	and.w	#3,\n
	beq.s	4f
3:	subq.w	#1,\n
5:	move.b	(\s)+,(\d)+
	dbra	\n,5b
4:
.endm

| Next bit of the ZX0 stream into C and X. %d1.b holds the unread bits followed by a sentinel 1;
| when only the sentinel is left, add.b gives 0 (and X = 1), and addx.b shifts the sentinel into
| the new byte. 14 cycles in the common case, no multi-bit shift.
.macro GETBIT
	add.b	%d1,%d1
	bne.s	9f
	move.b	(%a0)+,%d1
	addx.b	%d1,%d1
9:
.endm

	.text
	.even
	.globl	lz4_asm
| token = literal length (4 bits) | match length - 4 (4 bits), 15 = more length bytes follow
| (255 = keep going); literals; 16-bit little-endian offset; the last sequence has no match.
lz4_asm:
	movem.l	%d3-%d4/%a2-%a3,-(%sp)
	and.l	#0xFFFF,%d0
	lea	(%a0,%d0.l),%a3		| end of the packed data
.Llz_seq:
	moveq	#0,%d1
	move.b	(%a0)+,%d1		| token
	move.w	%d1,%d2
	lsr.w	#4,%d2			| literal length
	beq.s	.Llz_match
	cmp.w	#15,%d2
	bne.s	.Llz_lit
.Llz_litx:
	moveq	#0,%d3
	move.b	(%a0)+,%d3
	add.w	%d3,%d2
	addq.b	#1,%d3			| 255 + 1 = 0: another length byte follows
	beq.s	.Llz_litx
.Llz_lit:
	COPY	%a0, %a1, %d2, %d4
.Llz_match:
	cmp.l	%a3,%a0
	bcc.s	.Llz_done
	moveq	#0,%d2
	move.b	1(%a0),%d2		| offset, little-endian
	lsl.w	#8,%d2
	move.b	(%a0),%d2
	addq.l	#2,%a0
	move.l	%a1,%a2
	sub.l	%d2,%a2			| match source
	moveq	#15,%d3
	and.w	%d1,%d3			| match length - 4
	cmp.w	#15,%d3
	bne.s	.Llz_mlen
.Llz_mx:
	moveq	#0,%d0
	move.b	(%a0)+,%d0
	add.w	%d0,%d3
	addq.b	#1,%d0
	beq.s	.Llz_mx
.Llz_mlen:
	addq.w	#4,%d3
	cmp.w	#4,%d2
	bcs.s	.Llz_short
	COPY	%a2, %a1, %d3, %d4
	bra.w	.Llz_seq
.Llz_short:				| offset 1..3: a short-period repeat, bytes only
	subq.w	#1,%d3
.Llz_sb:
	move.b	(%a2)+,(%a1)+
	dbra	%d3,.Llz_sb
	bra.w	.Llz_seq
.Llz_done:
	move.l	%a1,%a0
	move.l	%a1,%d0
	movem.l	(%sp)+,%d3-%d4/%a2-%a3
	rts

	.even
	.globl	zx0_asm
| Registers: a0 packed data, a1 output, d0 length, d1 bit buffer, d2 last offset (a positive
| distance, < 32768), a2 match source, d3/d4 scratch.
zx0_asm:
	movem.l	%d3-%d4/%a2-%a3,-(%sp)
	moveq	#-128,%d1		| empty bit buffer: only the sentinel
	moveq	#1,%d2
.Lzx_literals:
	moveq	#1,%d0			| Elias gamma, interlaced: control bit (1 = stop), data bit
.Lzx_g1:
	GETBIT
	bcs.s	.Lzx_g1e
	GETBIT
	addx.w	%d0,%d0
	bra.s	.Lzx_g1
.Lzx_g1e:
	COPY	%a0, %a1, %d0, %d4
	GETBIT
	bcs.s	.Lzx_new
	moveq	#1,%d0			| repeat the last offset: length gamma
.Lzx_g2:
	GETBIT
	bcs.s	.Lzx_g2e
	GETBIT
	addx.w	%d0,%d0
	bra.s	.Lzx_g2
.Lzx_g2e:
	bsr.w	.Lzx_match
	GETBIT
	bcc.s	.Lzx_literals
.Lzx_new:
	moveq	#1,%d0			| offset MSB part: gamma with inverted data bits, 256 = end
.Lzx_g3:
	GETBIT
	bcs.s	.Lzx_g3e
	add.w	%d0,%d0
	GETBIT
	bcs.s	.Lzx_g3
	addq.w	#1,%d0
	bra.s	.Lzx_g3
.Lzx_g3e:
	cmp.w	#256,%d0
	beq.s	.Lzx_done
	moveq	#0,%d3
	move.b	(%a0)+,%d3
	lsl.w	#7,%d0
	move.w	%d0,%d2
	moveq	#1,%d0
	lsr.w	#1,%d3			| C = low bit = first control bit of the length gamma
	bcs.s	.Lzx_len1
	sub.w	%d3,%d2			| offset = (msb << 7) - (lo >> 1)
.Lzx_g4:
	GETBIT
	addx.w	%d0,%d0
	GETBIT
	bcc.s	.Lzx_g4
	bra.s	.Lzx_len
.Lzx_len1:
	sub.w	%d3,%d2
.Lzx_len:
	addq.w	#1,%d0
	bsr.w	.Lzx_match
	GETBIT
	bcs.s	.Lzx_new
	bra.w	.Lzx_literals
.Lzx_done:
	move.l	%a1,%a0
	move.l	%a1,%d0
	movem.l	(%sp)+,%d3-%d4/%a2-%a3
	rts
| copy d0 bytes from a1 - d2 to a1
.Lzx_match:
	move.l	%a1,%a2
	suba.w	%d2,%a2
	cmp.w	#4,%d2
	bcs.s	.Lzx_short
	COPY	%a2, %a1, %d0, %d4
	rts
.Lzx_short:
	subq.w	#1,%d0
.Lzx_sb:
	move.b	(%a2)+,(%a1)+
	dbra	%d0,.Lzx_sb
	rts
