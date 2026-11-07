// Proportional text in the shared font (ffcu ADV/FONT), any colour; shared by the battle
// engine and the scene player.
static void ui_text(s16 x, s16 y, const char *s, u8 color, s16 limit)
{
    while (*s) {
        u8 c = (u8)*s++, adv, r, shift = 24 - (x & 15);
        const u8 *g;
        u8 *l, *d;
        if (c < 32 || c > 126) c = '?';
        c -= 32; adv = UI_SHARED(ADV)[c];
        if (x + adv > limit) break;
        g = UI_SHARED(FONT) + (u16)c * 5;
        l = (u8 *)rt_light + y * RT_PBYTES + ((x >> 3) & ~1);
        d = (u8 *)rt_dark + y * RT_PBYTES + ((x >> 3) & ~1);
        for (r = 0; r < 5; r++, l += RT_PBYTES, d += RT_PBYTES) {
            u32 bits = (u32)*g++ << shift;
            u16 a = (u16)(bits >> 16), b = (u16)bits;
            if (color & 1) WR16(l, 0, RD16(l, 0) | a);
            else WR16(l, 0, RD16(l, 0) & ~a);
            if (color & 2) WR16(d, 0, RD16(d, 0) | a);
            else WR16(d, 0, RD16(d, 0) & ~a);
            if (b && x < 144) {
                if (color & 1) WR16(l, 1, RD16(l, 1) | b);
                else WR16(l, 1, RD16(l, 1) & ~b);
                if (color & 2) WR16(d, 1, RD16(d, 1) | b);
                else WR16(d, 1, RD16(d, 1) & ~b);
            }
        }
        x += adv;
    }
}
