#ifndef FFT_WAR_FUNDS_INC
#define FFT_WAR_FUNDS_INC
// The campaign's bottom plates, in the title screen's own style (campaign/title.c: a black box
// per line, white text): a label on one line, its value on the next, one pixel between them,
// each box only as wide as its text needs, the row ending one pixel from the bottom and the
// right of the screen. The source puts War Funds there, and beside it whatever the screen owes
// the player -- a shop's Eqp./All, the Soldier Office's Units Max and Price.
// The includer defines WF_TEXT(x, y, s, color, limit) and WF_WIDTH(s) for its font.

// One label-over-value plate ending at `right`; returns the leftmost column it used.
static s16 plate_pair(s16 right, const char *label, const char *value)
{
    s16 w = (s16)(WF_WIDTH(label) + 8), left;
    draw_rect((s16)(right - w), 80, w, 9, C_BLACK);
    WF_TEXT((s16)(right - w + 4), 82, label, C_WHITE, 158);
    left = (s16)(right - w);
    w = (s16)(WF_WIDTH(value) + 8);
    draw_rect((s16)(right - w), 90, w, 9, C_BLACK);
    WF_TEXT((s16)(right - w + 4), 92, value, C_WHITE, 158);
    return (s16)(right - w) < left ? (s16)(right - w) : left;
}
static char *wf_digits(char *out, u32 n)        // out: at least 9 bytes
{
    u8 k = 8;
    out[k] = 0;
    do { out[--k] = (char)('0' + (u8)(n % 10)); n /= 10; } while (n);
    return out + k;
}
static s16 war_funds(u32 amount)
{
    char digits[9];
    return plate_pair(159, "War Funds", wf_digits(digits, amount));
}
// The source's Eqp. / All counter: how many of the party can wear the item the cursor is on,
// out of how many are in it.
static void eqp_all(s16 right, u8 fit, u8 all)
{
    char n[8];
    u8 k = 0;
    n[k++] = (char)('0' + fit / 10); n[k++] = (char)('0' + fit % 10);
    n[k++] = '/';
    n[k++] = (char)('0' + all / 10); n[k++] = (char)('0' + all % 10);
    n[k] = 0;
    plate_pair(right, "Eqp./All", n);
}
// The Soldier Office's own row: how many units the party holds of its maximum, what a recruit
// costs, and the funds -- the three plates the source lines up along the bottom.
static void office_plates(u8 units, u8 max, u16 price, u32 funds)
{
    char value[9], counter[8];
    u8 k = 0;
    s16 left = war_funds(funds);
    left = plate_pair((s16)(left - 3), "Price", wf_digits(value, price));
    counter[k++] = (char)('0' + units / 10); counter[k++] = (char)('0' + units % 10);
    counter[k++] = '/';
    counter[k++] = (char)('0' + max / 10); counter[k++] = (char)('0' + max % 10);
    counter[k] = 0;
    plate_pair((s16)(left - 3), "Units Max", counter);
}
#endif
