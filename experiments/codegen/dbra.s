	.file	"dbra.c"
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
	.globl	a
a:
	jbra .L2
.L3:
	clr.b (%a0)+
.L2:
	dbra %d0,.L3
	rts
	.even
	.globl	b
b:
	jbra .L11
.L8:
	clr.b (%a0)+
.L11:
	subq.w #1,%d0
	jbpl .L8
	rts
	.even
	.globl	c
c:
	jbra .L13
.L14:
	clr.b (%a0)+
.L13:
	dbra %d0,.L14
	rts
	.even
	.globl	d
d:
	clr.w %d1
	jbra .L18
.L19:
	clr.b (%a0)+
	addq.w #1,%d1
.L18:
	cmp.w %d1,%d0
	jbne .L19
	rts
