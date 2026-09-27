// Shared 68000 asm decompressors (lib/unpack68k.s): link the .s with the program,
//   ti-cc -o name main.c ../../lib/unpack68k.s
// Both return the end of the output. The output buffer must hold the whole raw data (the packer
// prints its length); ZX0 streams come from lib/zx0pack.py.
// Measured on 20 KB of game graphics: ZX0 42-73 cycles per output byte, LZ4 19-26.
#ifndef UNPACK68K_H
#define UNPACK68K_H
unsigned char *zx0_asm(const unsigned char *s asm("%a0"), unsigned char *d asm("%a1")) __attribute__((__regparm__(2)));
unsigned char *lz4_asm(const unsigned char *s asm("%a0"), unsigned short slen asm("%d0"),
                       unsigned char *d asm("%a1")) __attribute__((__regparm__(3)));
#endif
