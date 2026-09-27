// Decompressors in plain C for the 68000 (also build on the host for testing: gcc -DHOST).
// Formats: PackBits RLE, LZ4 block, ZX0 v2 (einar-saukas/ZX0). Encoders: tools/pack.py.
// Every decoder writes forward into dst and returns the end of the output.
// Speed notes (68000): 16-bit unsigned counters (dbra), pointers only, no multiply/divide, and
// long copies when source and destination have the same parity (word/long accesses at odd
// addresses are an address error on the 68000).

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

// Forward copy of n bytes, n >= 1. Long moves when s and d have the same parity and the run is
// long enough; safe for overlapping LZ matches when d - s >= 4 (every long read was written).
static inline u8 *copy_fwd(u8 *d, const u8 *s, u16 n)
{
#ifndef BYTECOPY
    if (n >= 8 && !(((u16)(u32)d ^ (u16)(u32)s) & 1)) {
        u16 k;
        if ((u16)(u32)d & 1) { *d++ = *s++; n--; }
        k = n >> 2;                // 'while (k--)' compiles to dbra, 'do ... while (k--)' does not
        while (k--) { *(u32 *)d = *(const u32 *)s; d += 4; s += 4; }
        n &= 3;
        if (!n) return d;
    }
#endif
    while (n--) *d++ = *s++;
    return d;
}

// ------------------------------------------------------------------ RLE (PackBits)
// c < 128: c+1 literals follow; c >= 128: the next byte repeated 257-c times.
u8 *rle_unpack(const u8 *s, u16 slen, u8 *d)
{
    const u8 *end = s + slen;
    while (s < end) {
        u16 c = *s++;
        if (c < 128) {
            d = copy_fwd(d, s, c + 1);
            s += c + 1;
        } else {
            u8 v = *s++;
            c = 257 - c;
            while (c--) *d++ = v;
        }
    }
    return d;
}

// ------------------------------------------------------------------ LZ4 block
// token = literal length (4 bits) | match length - 4 (4 bits), 15 = more length bytes follow
// (255 = keep going); literals; 16-bit little-endian offset; the last sequence has no match.
u8 *lz4_unpack(const u8 *s, u16 slen, u8 *d)
{
    const u8 *end = s + slen;
    for (;;) {
        u16 tok = *s++, n = tok >> 4;
        if (n) {
            if (n == 15) { u16 b; do { b = *s++; n += b; } while (b == 255); }
            d = copy_fwd(d, s, n);
            s += n;
        }
        if (s >= end) return d;
        {
            u16 dist = s[0] | (s[1] << 8);
            const u8 *m = d - dist;
            s += 2;
            n = tok & 15;
            if (n == 15) { u16 b; do { b = *s++; n += b; } while (b == 255); }
            n += 4;
            if (dist >= 4) d = copy_fwd(d, m, n);
            else { while (n--) *d++ = *m++; }        // short-period repeat: bytes
        }
    }
}

// ------------------------------------------------------------------ ZX0 v2
// Bit stream MSB first, interlaced Elias gamma codes; literals / repeat last offset / new offset.
// Bit buffer: bit 15 = next bit, followed by the unread bits and a sentinel 1; 0x8000 = empty.
// BIT is an expression for 'if': a sign test, no 15-bit shift (lsr #15 = 38 cycles on hardware).
#define BIT (bb == 0x8000 ? bb = (*s++ << 8) | 0x80 : 0, t = bb, bb <<= 1, (short)t < 0)
#define GAMMA(x) do { x = 1; while (!BIT) { x += x; if (BIT) x++; } } while (0)
#define MATCH() do { if (dist >= 4) d = copy_fwd(d, d - dist, n); \
                     else { const u8 *m = d - dist; while (n--) *d++ = *m++; } } while (0)
u8 *zx0_unpack(const u8 *s, u8 *d)
{
    u16 dist = 1, n, bb = 0x8000, t;
literals:
    GAMMA(n);
    d = copy_fwd(d, s, n);
    s += n;
    if (BIT) goto new_offset;
    GAMMA(n);                    // repeat the last offset
    MATCH();
    if (!BIT) goto literals;
new_offset:
    n = 1;                       // offset MSB part: gamma with inverted data bits, 256 = end
    while (!BIT) { n += n; if (!BIT) n++; }
    if (n == 256) return d;
    {
        u16 lo = *s++;
        dist = (n << 7) - (lo >> 1);
        n = 1;                   // length gamma + 1; its first control bit is the low bit of lo
        if (!(lo & 1)) do { n += n; if (BIT) n++; } while (!BIT);
        n++;
        MATCH();
    }
    if (BIT) goto new_offset;
    goto literals;
}
