// The management menus' text and panels (fftmng, fftmap): the small pixel font of the bank ffmg
// (tools/management_data.py). The includer defines menu_font(): that font's bytes.
static s16 text(s16 x, s16 y, const char *s, u8 color, s16 limit)   // the prototype's small font
{
    static const u8 zero[8] = { 0 };
    const u8 *f = menu_font(), *base = f + 198 + 1520;
    while (*s) {
        u8 c = (u8)*s++, adv;
        const u8 *g;
        if (c < 32 || c > 126) c = '?';
        c -= 32; adv = f[8 + 95 + c];
        if (x + adv > limit) break;
        g = base + (u16)c * 10;
        { RtSprite sp = { 8, 5, color & 1 ? g : zero, color & 2 ? g : zero, g + 5 }; draw_sprite(x, y, &sp); }
        x += adv;
    }
    return x;
}
static s16 black(s16 x, s16 y, const char *s) { return text(x, y, s, C_BLACK, 159); }

#ifndef MGTEXT_TEXT_ONLY                // (fftmap shows no numbers)
static s16 number(s16 x, s16 y, u16 n)
{
    char b[6];
    u8 k = 5;
    b[k] = 0;
    do { b[--k] = (char)('0' + n % 10); n /= 10; } while (n);
    return black(x, y, b + k);
}
static s16 right(s16 x, s16 y, u16 n)           // right-aligned at x
{
    u16 m = n;
    s16 w = 0;
    do { w += 4; m /= 10; } while (m);
    return number(x - w, y, n);
}

#endif

static void panel(s16 x, s16 y, s16 w, s16 h)
{
    draw_rect(x, y, w, h, C_BLACK); draw_rect(x + 1, y + 1, w - 2, h - 2, C_WHITE);
    draw_rect(x + 2, y + 2, w - 4, 1, C_LGRAY);
}
