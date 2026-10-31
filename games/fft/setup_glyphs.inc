// Cold arrow/counter cache builders; the accepted algorithms and archive art are unchanged.
#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define digits CONTOUR_REF_digits
#else
static const u8 digits[11][3] = { { 0x1F, 0x11, 0x1F }, { 0x09, 0x1F, 0x01 }, { 0x17, 0x15, 0x1D },
    { 0x15, 0x15, 0x1F }, { 0x1C, 0x04, 0x1F }, { 0x1D, 0x15, 0x17 }, { 0x1F, 0x15, 0x17 },
    { 0x10, 0x10, 0x1F }, { 0x1F, 0x15, 0x1F }, { 0x1D, 0x15, 0x1F }, { 0x03, 0x04, 0x18 } };
#endif
static void setup_glyphs(void)
{
    {
    u8 n, r, g;
    for (n = 0; n < 4; n++) {
        u8 b[7];
        for (r = 0; r < 7; r++)
            for (b[r] = 0, g = 0; g < 3; g++)
                if (r && r < 6 && digits[n][g] & (0x20 >> r)) b[r] |= (u8)(0x40 >> g);
        for (r = 0; r < 7; r++) {
            u8 m = (u8)(b[r] | (r ? b[r - 1] : 0) | (r < 6 ? b[r + 1] : 0));
            W->dcg[n][0][r] = b[r]; W->dcg[n][1][r] = (u8)~(m | m << 1 | m >> 1);
        }
    }
    }
    {
    #if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define tri CONTOUR_REF_tri
#else
static const u8 tri[8] = { 0, 0x7E, 0x3E, 0x1E, 0x0E, 0x06, 0x02, 0 };
#endif
    u8 g, r, b;
    for (g = 0; g < 4; g++)
        for (r = 0; r < 8; r++) {
            u8 f = tri[g == 1 || g == 2 ? 7 - r : r], m = 0;
            if (g >= 2) { for (b = 0; b < 8; b++) if (f & (1 << b)) m |= (u8)(0x80 >> b); f = m; }
            W->arrow[g][0][r] = f;
        }
    for (g = 0; g < 4; g++)
        for (r = 0; r < 8; r++) {
            u8 o = (u8)((r ? W->arrow[g][0][r - 1] : 0) | W->arrow[g][0][r] | (r < 7 ? W->arrow[g][0][r + 1] : 0));
            W->arrow[g][1][r] = (u8)(o | o << 1 | o >> 1);
        }
    }
}
