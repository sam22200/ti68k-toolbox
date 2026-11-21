#ifndef FFT_WAR_FUNDS_INC
#define FFT_WAR_FUNDS_INC
// The campaign's one War Funds plate, in the title screen's own style (campaign/title.c: a
// black box per line, white text): the label on one line, the amount on the next, one pixel
// between them, each box only as wide as its text needs, the pair stuck one pixel from the
// bottom and the right of the screen. Used by the world map and the town screens; the
// battle's result screen keeps its own reviewed layout.
// The includer defines WF_TEXT(x, y, s, color, limit) and WF_WIDTH(s) for its font.
static s16 war_funds(u32 amount)   // returns the leftmost column it used
{
    char digits[9];
    u8 k = 8;
    s16 w, left;
    digits[k] = 0;
    do { digits[--k] = (char)('0' + (u8)(amount % 10)); amount /= 10; } while (amount);
    w = (s16)(WF_WIDTH("War Funds") + 8);
    draw_rect((s16)(159 - w), 80, w, 9, C_BLACK);
    WF_TEXT((s16)(163 - w), 82, "War Funds", C_WHITE, 158);
    left = (s16)(159 - w);
    w = (s16)(WF_WIDTH(digits + k) + 8);
    draw_rect((s16)(159 - w), 90, w, 9, C_BLACK);
    WF_TEXT((s16)(163 - w), 92, digits + k, C_WHITE, 158);
    return (s16)(159 - w) < left ? (s16)(159 - w) : left;
}

// The source's Eqp. / All counter, in the same plates, beside the funds: how many of the
// party can wear the item the cursor is on, out of how many are in it.
static void eqp_all(s16 right, u8 fit, u8 all)
{
    char n[8];
    u8 k = 0;
    s16 w;
    n[k++] = (char)('0' + fit / 10); n[k++] = (char)('0' + fit % 10);
    n[k++] = '/';
    n[k++] = (char)('0' + all / 10); n[k++] = (char)('0' + all % 10);
    n[k] = 0;
    w = (s16)(WF_WIDTH("Eqp./All") + 8);
    draw_rect((s16)(right - w), 80, w, 9, C_BLACK);
    WF_TEXT((s16)(right - w + 4), 82, "Eqp./All", C_WHITE, 158);
    w = (s16)(WF_WIDTH(n) + 8);
    draw_rect((s16)(right - w), 90, w, 9, C_BLACK);
    WF_TEXT((s16)(right - w + 4), 92, n, C_WHITE, 158);
}
#endif
