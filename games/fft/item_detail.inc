#ifndef FFT_ITEM_DETAIL_INC
#define FFT_ITEM_DETAIL_INC
// The campaign's one item detail panel, opened by [MODE] wherever an item can be pointed at:
// the shop's Buy and Sell lists, management's Equip, the battle's and deployment's equipment
// views. The caller fills an ItemDetail from its own bank, so the panel itself needs no bank,
// no allocation and no state. It draws over whatever is behind it.
// The includer defines ID_TEXT(x,y,s,color,limit), ID_NUM(x,y,n) and ID_ICON(x,y,id).
typedef struct {
    u8 id, kind, flags;                 // FFT's item id, its category byte, its slot flags
    const char *name;
    s8 stat[7];                         // HP MP PA MA SP Move Jump, as the record gives them
    u8 power;                           // a weapon's WP, 0 otherwise
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

static void item_detail_draw(const ItemDetail *d)
{
    static const char *const label[7] = { "HP", "MP", "PA", "MA", "Speed", "Move", "Jump" };
    s16 y = 28;
    u8 i, shown = 0;
    draw_rect(8, 18, 144, 64, C_BLACK);
    draw_rect(9, 19, 142, 62, C_WHITE);
    ID_ICON(13, 21, d->id);
    ID_TEXT(25, 22, d->name, C_BLACK, 148);
    ID_TEXT(13, 31, item_family(d->kind), C_DGRAY, 148);
    if (d->power) { ID_TEXT(90, 31, "Power", C_BLACK, 148); ID_NUM(120, 31, d->power); }
    y = 42;
    for (i = 0; i < 7; i++) {
        if (!d->stat[i]) continue;
        ID_TEXT((s16)(13 + 70 * (shown & 1)), y, label[i], C_BLACK, 148);
        ID_TEXT((s16)(48 + 70 * (shown & 1)), y, d->stat[i] > 0 ? "+" : "-", C_BLACK, 148);
        ID_NUM((s16)(54 + 70 * (shown & 1)), y, (u16)(d->stat[i] > 0 ? d->stat[i] : -d->stat[i]));
        if (shown & 1) y += 9;
        shown++;
    }
    if (!shown) ID_TEXT(13, 42, "No bonus", C_DGRAY, 148);
    ID_TEXT(13, 72, "MODE or SHIFT closes", C_DGRAY, 148);
}
#endif
