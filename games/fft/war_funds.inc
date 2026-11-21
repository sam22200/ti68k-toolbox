#ifndef FFT_WAR_FUNDS_INC
#define FFT_WAR_FUNDS_INC
// The campaign's one War Funds plate, in the title screen's own style (campaign/title.c: a
// black box per line, white text): the label on one line, the amount on the next, one pixel
// between them, each box only as wide as its text needs, the pair stuck one pixel from the
// bottom and the right of the screen. Used by the world map and the town screens; the
// battle's result screen keeps its own reviewed layout.
// The includer defines WF_TEXT(x, y, s, color, limit) and WF_WIDTH(s) for its font.
static void war_funds(u32 amount)
{
    char digits[9];
    u8 k = 8;
    s16 w;
    digits[k] = 0;
    do { digits[--k] = (char)('0' + (u8)(amount % 10)); amount /= 10; } while (amount);
    w = (s16)(WF_WIDTH("War Funds") + 8);
    draw_rect((s16)(159 - w), 80, w, 9, C_BLACK);
    WF_TEXT((s16)(163 - w), 82, "War Funds", C_WHITE, 158);
    w = (s16)(WF_WIDTH(digits + k) + 8);
    draw_rect((s16)(159 - w), 90, w, 9, C_BLACK);
    WF_TEXT((s16)(163 - w), 92, digits + k, C_WHITE, 158);
}
#endif
