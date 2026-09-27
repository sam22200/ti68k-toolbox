	.file	"traps.c"
#NO_APP
	.text
tigcc_compiled.:
	.text
#APP
	.xdef _ti89
	.text
	.xdef _ti89ti
	.text
	.xdef __ref_all___startup_code
	.text
	.xdef __ref_all___detect_calc
	.text
	.xdef __ref_all___test_for_specific_calc
	.text
	.xdef __ref_all___test_for_89
	.text
	.set _A_LINE,0xA000
	.text
	.xdef __ref_all___kernel_format_data_var
	.text
	.xdef _tigcc_native
	.text
	.xdef __ref_all___nostub
	.text
	.xdef __ref_all___kernel_format_bss
	.text
	.xdef __ref_all___kernel_format_rom_calls
	.text
	.set MT_TEXT,0x8000
	.text
	.set MT_XREF,0x9000
	.text
	.set MT_ICON,0xA000
	.text
	.set MT_CASCADE,0x4000
#NO_APP
	.text
	.even
	.globl	sum_bytes
sum_bytes:
	move.w %d0,%d1
	sub.l %a1,%a1
.L2:
	clr.w %d0
	move.b (%a0)+,%d0
	add.w %d0,%a1
	subq.w #1,%d1
	jbne .L2
	move.w %a1,%d0
	rts
	.even
	.globl	row_off
row_off:
	and.l #255,%d0
	move.l %d0,%d1
	lsl.l #8,%d1
	lsl.l #6,%d0
	add.l %d1,%d0
	rts
	.even
	.globl	row_off16
row_off16:
	and.w #255,%d0
	muls.w #320,%d0
	rts
	.even
	.globl	qr
qr:
	and.l #0xFFFF,%d0
	divu.w %d1,%d0
	move.l %d0,%d1
	swap %d1
	move.w %d1,(%a0)
	rts
	.even
	.globl	qr32
qr32:
	movm.l #0x1820,-(%sp)
	move.l %d0,%d3
	move.l %a0,%a2
	moveq #0,%d4
	move.w %d1,%d4
	move.l %d4,-(%sp)
	move.l %d0,-(%sp)
	jbsr __umodsi3
	addq.l #8,%sp
	move.w %d0,(%a2)
	move.l %d4,-(%sp)
	move.l %d3,-(%sp)
	jbsr __udivsi3
	addq.l #8,%sp
	movm.l (%sp)+,#0x418
	rts
	.even
	.globl	clr_up
clr_up:
	moveq #0,%d0
.L16:
	clr.b (%a0,%d0.l)
	addq.l #1,%d0
	moveq #100,%d1
	cmp.l %d0,%d1
	jbne .L16
	rts
	.even
	.globl	clr_down
clr_down:
	moveq #100,%d0
.L22:
	clr.b (%a0)+
	subq.w #1,%d0
	jbne .L22
	rts
	.even
	.globl	bf_test
bf_test:
	tst.b (%a0)
	jblt .L28
	clr.w %d0
	rts
.L28:
	move.b (%a0),%d0
	and.w #15,%d0
	cmp.w #3,%d0
	sle %d0
	neg.b %d0
	eor.b #1,%d0
	and.w #255,%d0
	rts
	.even
	.globl	mask_test
mask_test:
	tst.b (%a0)
	jblt .L33
	clr.w %d0
	rts
.L33:
	cmp.b #3,(%a1)
	sls %d0
	neg.b %d0
	eor.b #1,%d0
	and.w #255,%d0
	rts
	.even
	.globl	v2add
v2add:
	move.l %d3,-(%sp)
	move.l %d0,%d2
	swap %d2
	ext.l %d2
	move.l %d1,%d3
	swap %d3
	ext.l %d3
	add.w %d3,%d2
	add.w %d1,%d0
	move.w %d2,%d1
	swap %d1
	mov.w %d0,%d1
	move.l %d1,%d0
	move.l (%sp)+,%d3
	rts
	.even
	.globl	fastrange
fastrange:
	mulu.w %d1,%d0
	clr.w %d0
	swap %d0
	rts
	.even
	.globl	mwc
mwc:
	move.l z,%d1
	move.w %d1,%d0
	mulu.w #36969,%d0
	clr.w %d1
	swap %d1
	add.l %d1,%d0
	move.l %d0,z
	rts
	.even
	.globl	mwc2
mwc2:
	move.l z,%d1
	move.w %d1,%d0
	mulu.w #36969,%d0
	clr.w %d1
	swap %d1
	add.l %d1,%d0
	move.l %d0,z
	rts
	.even
	.globl	xorshift
xorshift:
	move.w xs,%d1
	move.w %d1,%d0
	lsl.w #7,%d0
	eor.w %d1,%d0
	move.w %d0,%d1
	moveq #9,%d2
	lsr.w %d2,%d1
	eor.w %d0,%d1
	move.w %d1,%d0
	lsl.w #8,%d0
	eor.w %d1,%d0
	move.w %d0,xs
	rts
	.even
	.globl	isnull
isnull:
	cmp.w #0,%a0
	seq %d0
	ext.w %d0
	neg.w %d0
	rts
	.globl	z
	.text
	.even
z:
	.long	362436069
	.globl	xs
	.even
xs:
	.word	1
