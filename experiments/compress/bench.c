// Decompression bench on the camp-fire data (tiles 13,888 B, sprites 3,816 B, fire 3,072 B):
// PackBits RLE, LZ4 block, ZX0 v2 (C decoders in unpack.c, LZ4 and ZX0 also in asm, lib/unpack68k.s) and
// ttpack/ExtGraph UnpackBuffer, all in one program (one run). Data: pdata.h from tools/pack.py.
// Build: ti-cc -o cbench bench.c ../../lib/unpack68k.s
// (~55 KB: Titanium only, AMS 2 refuses > 24 KB). Add -DBYTECOPY for byte-only copies.
// Each blob is unpacked R times into a malloc'd buffer (tick counter at 256 Hz on auto-int 1,
// int 5 silenced), then its Fletcher-16 and length are checked against the host's values.
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>
#include "extgraph.h"
#include "pdata.h"
#include "unpack.c"

static volatile unsigned long ticks;
DEFINE_INT_HANDLER(tick_handler) { ticks++; }
#define R 8
#define NF 6

u8 *lz4_asm(const u8 *s asm("%a0"), u16 slen asm("%d0"), u8 *d asm("%a1")) __attribute__((__regparm__(3)));
u8 *zx0_asm(const u8 *s asm("%a0"), u8 *d asm("%a1")) __attribute__((__regparm__(2)));

static u8 *unpack(u16 f, const u8 *s, u16 n, u8 *d)
{
    switch (f) {
    case 0:  return rle_unpack(s, n, d);
    case 1:  return lz4_unpack(s, n, d);
    case 2:  return lz4_asm(s, n, d);
    case 3:  return zx0_unpack(s, d);
    case 4:  return zx0_asm(s, d);
    default: return UnpackBuffer(s, d) ? d : d + ttunpack_size(s);
    }
}

static const u8 *const pk[NF][3] = { { rle0, rle1, rle2 }, { lz40, lz41, lz42 }, { lz40, lz41, lz42 }, { zx00, zx01, zx02 }, { zx00, zx01, zx02 }, { ttp0, ttp1, ttp2 } };
static const u16 pklen[NF][3] = { { sizeof(rle0), sizeof(rle1), sizeof(rle2) }, { sizeof(lz40), sizeof(lz41), sizeof(lz42) },
                                  { sizeof(lz40), sizeof(lz41), sizeof(lz42) }, { sizeof(zx00), sizeof(zx01), sizeof(zx02) },
                                  { sizeof(zx00), sizeof(zx01), sizeof(zx02) }, { sizeof(ttp0), sizeof(ttp1), sizeof(ttp2) } };
static const char *const fnm[NF] = { "RLE", "LZ4", "LZ4asm", "ZX0", "ZX0asm", "ttpack" };
static const u16 rawlen[3] = { RAW0_LEN, RAW1_LEN, RAW2_LEN };
static const u16 rawsum[3] = { RAW0_SUM, RAW1_SUM, RAW2_SUM };

static u16 fletcher16(const u8 *p, u16 n)
{
    u16 a = 0, b = 0;
    while (n--) { a += *p++; if (a >= 255) a -= 255; b += a; if (b >= 255) b -= 255; }
    return b << 8 | a;
}

void _main(void)
{
    INT_HANDLER o1 = GetIntVec(AUTO_INT_1), o5 = GetIntVec(AUTO_INT_5);
    unsigned long t[NF][3], tc, tm, s;
    u16 ok[NF][3], f, k, r;
    u8 *buf = malloc(RAW0_LEN + 16), *buf2 = malloc(RAW0_LEN + 16);
    if (!buf || !buf2) goto out;
    ClrScr(); printf_xy(0, 0, "Benchmarking...");
    SetIntVec(AUTO_INT_5, DUMMY_HANDLER);
    SetIntVec(AUTO_INT_1, tick_handler);
    for (f = 0; f < NF; f++)
        for (k = 0; k < 3; k++) {
            u8 *e = 0;
            s = ticks; while (ticks == s); s = ticks;
            for (r = 0; r < R; r++) e = unpack(f, pk[f][k], pklen[f][k], buf);
            t[f][k] = ticks - s;
            ok[f][k] = e - buf == rawlen[k] && fletcher16(buf, rawlen[k]) == rawsum[k];
        }
    // references: our long copy and AMS memcpy of the tile buffer
    s = ticks; while (ticks == s); s = ticks;
    for (r = 0; r < R; r++) copy_fwd(buf2, buf, RAW0_LEN);
    tc = ticks - s;
    s = ticks; while (ticks == s); s = ticks;
    for (r = 0; r < R; r++) memcpy(buf2, buf, RAW0_LEN);
    tm = ticks - s;
    SetIntVec(AUTO_INT_1, o1); SetIntVec(AUTO_INT_5, o5);

    ClrScr(); FontSetSys(F_4x6);
    printf_xy(0, 0, "cycles per output byte, ms for tiles"
#ifdef BYTECOPY
              " bytecopy"
#endif
             );
    printf_xy(0, 7, "fmt      tiles    sprites  fire     ms");
    for (f = 0; f < NF; f++) {
        char line[64], *p = line;
        p += sprintf(p, "%-7s", fnm[f]);
        for (k = 0; k < 3; k++) {
            unsigned long c10 = t[f][k] * 468750UL / ((unsigned long)R * rawlen[k]);   // cycles/byte x10
            p += sprintf(p, " %3lu.%lu %s", c10 / 10, c10 % 10, ok[f][k] ? "ok" : "BAD");
        }
        sprintf(p, " %4lu", t[f][0] * 1000 / (256UL * R));
        printf_xy(0, 15 + 7 * f, "%s", line);
    }
    printf_xy(0, 60, "copy_fwd %lu, memcpy %lu cyc/100B (t=tiles s=sprites f=fire)",
              tc * 4687500UL / ((unsigned long)R * RAW0_LEN), tm * 4687500UL / ((unsigned long)R * RAW0_LEN));
    GKeyFlush();
    ngetchx();
    FontSetSys(F_6x8);
out:
    free(buf2); free(buf);
}
