#ifndef FFT_ITEM_DETAIL_INC
#define FFT_ITEM_DETAIL_INC
// The campaign's one item detail panel, opened by [MODE] wherever an item can be pointed at:
// the shop's Buy and Sell lists, management's Equip, the battle's and deployment's equipment
// views. The caller fills an ItemDetail from its own banks, so the panel itself needs no bank,
// no allocation and no state. It draws over whatever is behind it.
// The includer defines ID_TEXT(x,y,s,color,limit), ID_NUM(x,y,n), ID_ICON(x,y,id) and
// ID_ADV(c) (its font's advance for one character).
typedef struct {
    u8 id, kind, flags;                 // FFT's item id, its category byte, its slot flags
    const char *name;
    const char *help;                   // FFT's own description (ffid), "" when it has none
    s8 stat[7];                         // HP MP PA MA SP Move Jump, as the record gives them
    u8 power;                           // a weapon's WP, 0 otherwise
    u8 evade;                           // its evade %, which the source prints beside the power
} ItemDetail;

// The family names the Shop Guide uses, by the item's category byte.
static const char *item_family(u8 kind)
{
    static const char *const weapon[18] = { "", "Dagger", "Ninja Sword", "Sword", "Knight Sword",
        "Katana", "Axe", "Rod", "Staff", "Flail", "Gun", "Crossbow", "Bow", "Harp", "Book",
        "Spear", "Stick", "Bag" };
    static const char *const wear[15] = { "Cloth", "Shield", "Helmet", "Hat", "Hair Adornment",
        "Armor", "Clothing", "Robe", "Shoes", "Armguard", "Ring", "Armlet", "Cloak", "Perfume",
        "Shuriken" };
    if (kind < 18) return weapon[kind];
    if (kind < 33) return wear[kind - 18];
    return kind == 33 ? "Bomb" : "Item";
}

// FFT's description, wrapped on the caller's own font. Three lines are enough for every one of
// the 253 the disc carries. `item_detail_line` gives one line at a time, so a caller that draws
// a line a frame (the battle's status page) and one that draws a whole panel share the wrap.
static u8 item_detail_line(const char *s, u8 limit, u8 want, char *out)
{
    u8 row;
    for (row = 0; *s; row++) {
        u8 n = 0, last = 0, width = 0;
        while (s[n] && n < 63) {
            u8 adv = ID_ADV(s[n]);
            if (width + adv > limit) break;
            if (s[n] == ' ') last = n;
            width = (u8)(width + adv); n++;
        }
        if (s[n] && last) n = last;
        if (row == want) { memcpy(out, s, n); out[n] = 0; return n; }
        s += n;
        while (*s == ' ') s++;
    }
    out[0] = 0;
    return 0;
}
#ifdef ID_ICON
static void item_detail_wrap(const char *s, s16 x, s16 y, u8 limit, u8 lines)
{
    char line[64];
    u8 row;
    for (row = 0; row < lines; row++) {
        if (!item_detail_line(s, limit, row, line)) break;
        ID_TEXT(x, (s16)(y + 7 * row), line, C_BLACK, 158);
    }
}
#endif

#ifdef ID_ICON
static void item_detail_draw(const ItemDetail *d)
{
    static const char *const label[7] = { "HP", "MP", "PA", "MA", "Spd", "Mov", "Jmp" };
    s16 y;
    u8 i, shown = 0;
    draw_rect(5, 13, 150, 74, C_BLACK);
    draw_rect(6, 14, 148, 72, C_WHITE);
    ID_ICON(10, 16, d->id);
    ID_TEXT(22, 17, d->name, C_BLACK, 152);
    ID_TEXT(22, 26, item_family(d->kind), C_DGRAY, 152);
    // The source's own Help box prints "Attack power:13  Evade:5%" under the name.
    if (d->power) { ID_TEXT(80, 26, "Power", C_DGRAY, 152); ID_NUM(106, 26, d->power); }
    if (d->evade) { ID_TEXT(118, 26, "Ev", C_DGRAY, 152); ID_NUM(130, 26, d->evade);
                    ID_TEXT((s16)(130 + (d->evade > 9 ? 9 : 5)), 26, "%", C_DGRAY, 152); }
    if (d->help && *d->help) item_detail_wrap(d->help, 10, 36, 140, 3);
    y = 60;
    for (i = 0; i < 7; i++) {
        if (!d->stat[i]) continue;
        ID_TEXT((s16)(10 + 36 * (shown & 3)), y, label[i], C_BLACK, 152);
        ID_TEXT((s16)(10 + 36 * (shown & 3) + 16), y, d->stat[i] > 0 ? "+" : "-", C_BLACK, 152);
        ID_NUM((s16)(10 + 36 * (shown & 3) + 22), y, (u16)(d->stat[i] > 0 ? d->stat[i] : -d->stat[i]));
        if ((shown & 3) == 3) y += 9;
        shown++;
    }
    ID_TEXT(10, 78, "MODE or SHIFT closes", C_DGRAY, 152);
}
#endif
#endif
