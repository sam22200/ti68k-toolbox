#ifndef FFT_BODY_BLIT_H
#define FFT_BODY_BLIT_H
// A wide monster body drawn at an arbitrary x costs two ExtGraph clipped calls a frame,
// and the per-row shift inside them dominates: 15,189 cycles for a 32x28 masked grey body
// (ti68k-performance.md section 7). Its x only changes when the unit or the camera moves,
// so the shift is worth keeping. BodyRow holds one row already shifted into the three
// words it lands on; blitting is then one read-mask-or per word per plane, 7,299 cycles.
#include "../../runtime/core/rt.h"
#define BODY_WORDS 3                           // 32 columns at any x span three words
typedef struct { u16 l[BODY_WORDS], d[BODY_WORDS], m[BODY_WORDS]; } BodyRow;

#endif
// The code, included again from monster_body.inc after module.h: inside a module the
// planes and the unit questions are the context's, and those macros do not exist yet
// where the type is needed (fft.c defines Cover before module.h).
#ifdef BODY_BLIT_CODE
#ifndef FFT_BODY_BLIT_CODE
#define FFT_BODY_BLIT_CODE
// One row of a 32-column body, its left strip a and right strip b, shifted by s = x & 15
// into the words from x >> 4. A 16-column body passes bl = bd = 0 and bm = 0xFFFF: an
// entirely transparent right strip. Mask bit 1 keeps the destination, as RtSprite does,
// so every bit outside the body's own 32 columns is set.
static void body_row_shift(BodyRow *o, u16 al, u16 ad, u16 am,
                           u16 bl, u16 bd, u16 bm, u8 s)
{
    u32 t[3];
    u8 k, up = (u8)(16 - s);
    t[0] = (u32)al << 16 | bl; t[1] = (u32)ad << 16 | bd; t[2] = (u32)am << 16 | bm;
    for (k = 0; k < 3; k++) {                  // light, dark, mask: one shift sequence
        u16 *w = o->l + 3 * k;                 // the three arrays are adjacent
        w[0] = (u16)(t[k] >> (16 + s)); w[1] = (u16)(t[k] >> s); w[2] = (u16)(t[k] << up);
    }
    o->m[0] |= (u16)((u32)0xFFFFUL << up);     // outside the body's own columns: keep
    o->m[2] |= (u16)((u32)0xFFFFUL >> s);
}

// The plane is big-endian: byte 0 of a word is its leftmost eight pixels. On the 68000
// that is a plain word access; the PC build must say so, or the two draw different
// pixels from the same bytes.
#ifdef __m68k__
#define BB_RD(p)    (*(const u16 *)(p))
#define BB_WR(p, v) (*(u16 *)(p) = (v))
#else
#define BB_RD(p)    ((u16)((u16)((const u8 *)(p))[0] << 8 | ((const u8 *)(p))[1]))
#define BB_WR(p, v) do { u16 bb_ = (v); ((u8 *)(p))[0] = (u8)(bb_ >> 8); \
                         ((u8 *)(p))[1] = (u8)bb_; } while (0)
#endif

// h shifted rows at word x >> 4, from row y. Returns 0 when it declines to draw, which
// only the 68000 build does, for a body clipped sideways: its caller then falls back to
// draw_sprite, whose pixels are identical (check_monster_body.c compares the two over
// every sub-word x). Keeping one unrolled path there is what makes this worth doing at
// all: a loop over the three words cost 13,952 cycles for thirteen 32x28 bodies against
// 7,299 unrolled, barely better than the 15,189 of the two ExtGraph calls it replaces.
// The 68000 allows a long at any even address, so a word-aligned destination is enough.
static u8 body_rows_blit(s16 x, s16 y, const BodyRow *rows, u8 h)
{
    s16 w0 = (s16)(x >> 4), r;
#if defined(__m68k__) && !defined(RT_MONO)
    s16 r0 = y < 0 ? (s16)-y : 0, r1 = h;      // the rows on screen, decided once
    u8 *l, *d;
    if (w0 < 0 || w0 + BODY_WORDS > RT_PBYTES / 2) return 0;
    if (y + r1 > (s16)RT_DRAW_ROWS) r1 = (s16)((s16)RT_DRAW_ROWS - y);
    if (r0 >= r1) return 1;
    l = (u8 *)rt_light + (u16)(y + r0) * RT_PBYTES + 2 * w0;
    d = (u8 *)rt_dark + (u16)(y + r0) * RT_PBYTES + 2 * w0;
    for (r = r0; r < r1; r++, l += RT_PBYTES, d += RT_PBYTES) {
        const BodyRow *o = rows + r;
        u32 m01 = *(const u32 *)&o->m[0];
        u16 m2 = o->m[2];
        *(u32 *)l = (*(u32 *)l & m01) | *(const u32 *)&o->l[0];
        *(u16 *)(l + 4) = (u16)((*(u16 *)(l + 4) & m2) | o->l[2]);
        *(u32 *)d = (*(u32 *)d & m01) | *(const u32 *)&o->d[0];
        *(u16 *)(d + 4) = (u16)((*(u16 *)(d + 4) & m2) | o->d[2]);
    }
#else
    u8 c0 = 0, c1 = BODY_WORDS;                // the words on screen, decided once
    if (w0 <= -BODY_WORDS || w0 >= RT_PBYTES / 2) return 1;
    if (w0 < 0) c0 = (u8)(-w0);
    if (w0 + BODY_WORDS > RT_PBYTES / 2) c1 = (u8)(RT_PBYTES / 2 - w0);
    for (r = 0; r < h; r++) {
        s16 py = (s16)(y + r);
        const BodyRow *o = rows + r;
        u8 *l, c;
#ifndef RT_MONO
        u8 *d;
#endif
        if ((u16)py >= RT_DRAW_ROWS) continue;
        l = (u8 *)rt_light + (u16)py * RT_PBYTES + 2 * (w0 + c0);
#ifndef RT_MONO
        d = (u8 *)rt_dark + (u16)py * RT_PBYTES + 2 * (w0 + c0);
#endif
        for (c = c0; c < c1; c++, l += 2
#ifndef RT_MONO
             , d += 2
#endif
             ) {
#ifdef RT_MONO
            BB_WR(l, (u16)((BB_RD(l) & o->m[c]) | o->d[c]));   // mono draws the dark plane
#else
            BB_WR(l, (u16)((BB_RD(l) & o->m[c]) | o->l[c]));
            BB_WR(d, (u16)((BB_RD(d) & o->m[c]) | o->d[c]));
#endif
        }
    }
#endif
    return 1;
}
#endif
#endif
