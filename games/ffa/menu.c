// Main menu (ESC while walking; the original's APPS menu and ESC popup): Items, Equip,
// Materia, Status, Quit. Up/down move, 2nd selects or changes, shift/ESC goes back.
#include "ffa.h"

enum { MN_ROOT, MN_ITEMS, MN_EQUIP, MN_MATERIA, MN_STATUS };
enum { R_ITEMS, R_EQUIP, R_MATERIA, R_STATUS, R_SAVE, R_QUIT, NROOT };

static const char *const root_name[NROOT] = { "Items", "Equip", "Materia", "Status", "Save", "Quit" };
static const char *const item_name[NITEM] = { "Potion", "Hi-Potion", "Ether", "Turbo Ether", "X-Potion", "Elixir", "Antidote" };
static const char *const mat_name[NMAT] = { 0, 0, "Fire", "Cure" };

static const char *arm_name(u8 a)
{
    return a == A_SWORD ? "Buster Sword" : a == A_BANGLE ? "Bronze Bangle" : a == A_WRIST ? "Power Wrist" : "-";
}

void menu_open(void)
{
    st.mode = M_MENU;
    st.menu = MN_ROOT;
    st.mcur = 0;
    st.mmsg = 0;
}

static u8 count(void)
{
    switch (st.menu) {
    case MN_ROOT: return NROOT;
    case MN_ITEMS: return NITEM;
    case MN_EQUIP: return 4;
    case MN_MATERIA: return 2;
    }
    return 1;
}

static u8 next_owned(u8 cur, const u8 *opts, u8 n)   // the next option owned after cur (0 = none)
{
    u8 i, k = 0;
    for (i = 0; i < n; i++) if (opts[i] == cur) k = i;
    for (i = 1; i <= n; i++) {
        u8 o = opts[(k + i) % n];
        if (!o || st.own[o]) return o;
    }
    return 0;
}

static void use_item(u8 i)                    // objet, obj = 1: outside battles
{
    Hero *h = &st.hero;
    if (!st.item[i]) return;
    if (i == I_ANTIDOTE) { st.mmsg = 1; return; }           // "Can't use outside the battle."
    if ((i == I_ETHER || i == I_TETHER) ? h->mp == h->mpm : i == I_ELIXIR ? h->hp == h->hpm && h->mp == h->mpm : h->hp == h->hpm) return;
    st.item[i]--;
    if (i == I_POTION) h->hp = h->hp + 100 > h->hpm ? h->hpm : h->hp + 100;
    else if (i == I_HIPOTION) h->hp = h->hp + 500 > h->hpm ? h->hpm : h->hp + 500;
    else if (i == I_ETHER) h->mp = h->mp + 50 > h->mpm ? h->mpm : h->mp + 50;
    else if (i == I_TETHER) h->mp = h->mp + 200 > h->mpm ? h->mpm : h->mp + 200;
    else if (i == I_XPOTION) h->hp = h->hpm;
    else { h->hp = h->hpm; h->mp = h->mpm; }
}

u8 menu_update(void)                          // 0 = quit the game
{
    u8 go = input_pressed(K_A | K_ENTER) != 0, back = input_pressed(K_B | K_ESC) != 0, n = count();
    if (go || input_pressed(K_UP | K_DOWN)) st.mmsg = 0;
    if (input_pressed(K_UP)) st.mcur = st.mcur ? st.mcur - 1 : n - 1;
    if (input_pressed(K_DOWN)) st.mcur = st.mcur + 1 < n ? st.mcur + 1 : 0;
    if (back) {
        if (st.menu == MN_ROOT) st.mode = M_WALK;
        else { st.mcur = st.menu - 1; st.menu = MN_ROOT; }
        return 1;
    }
    if (!go) return 1;
    switch (st.menu) {
    case MN_ROOT:
        if (st.mcur == R_QUIT) return 0;
        if (st.mcur == R_SAVE) { st.mmsg = game_save() ? 3 : 4; break; }
        st.menu = st.mcur + 1;
        st.mcur = 0;
        break;
    case MN_ITEMS: use_item(st.mcur); break;
    case MN_EQUIP: {                          // 2nd cycles through what is owned for the slot
        static const u8 weapons[2] = { 0, A_SWORD }, armors[2] = { 0, A_BANGLE }, accs[2] = { 0, A_WRIST };
        Hero *h = &st.hero;
        if (st.mcur == 0) {
            h->weapon = next_owned(h->weapon, weapons, 2);
            if (!h->weapon) h->slot[0] = h->slot[1] = 0;     // no slots without a weapon
        } else if (st.mcur == 1) h->armor = next_owned(h->armor, armors, 2);
        else {
            u8 *a = &h->acc[st.mcur - 2], other = h->acc[3 - st.mcur];
            *a = next_owned(*a, accs, 2);
            if (*a && *a == other) *a = 0;                     // not twice the same accessory
        }
        break;
    }
    case MN_MATERIA: {                        // Fire, Cure or empty in each slot of the weapon
        static const u8 mats[3] = { 0, MAT_FIRE, MAT_CURE };
        u8 *s = &st.hero.slot[st.mcur], other = st.hero.slot[st.mcur ^ 1], k, i;
        if (!st.hero.weapon) { st.mmsg = 2; break; }           // "No Slot"
        for (k = 0; k < 3 && mats[k] != *s; k++) ;
        for (i = 1; i <= 3; i++) {
            u8 m = mats[(k + i) % 3];
            if (!m || (st.mat[m] && m != other)) { *s = m; break; }
        }
        break;
    }
    }
    return 1;
}

// ---------------------------------------------------------------- render
static void box(s16 x, s16 y, s16 w, s16 h)
{
    draw_rect(x, y, w, h, C_BLACK);
    draw_rect(x + 1, y + 1, w - 2, h - 2, C_DGRAY);
    draw_rect(x + 2, y + 2, w - 4, h - 4, C_WHITE);
}

static void num(s16 x, s16 y, u16 v, u8 font)   // right-aligned at x
{
    char t[6];
    u8 k = 5, cw = font == F_SMALL ? 4 : 6;
    t[5] = 0;
    do t[--k] = '0' + v % 10; while ((v /= 10) && k);
    draw_text(x - (5 - k) * cw, y, t + k, font, C_BLACK);
}

static void cursor(s16 x, s16 y)
{
    draw_rect(x, y, 2, 5, C_BLACK); draw_rect(x + 2, y + 1, 1, 3, C_BLACK); draw_rect(x + 3, y + 2, 1, 1, C_BLACK);
}

void menu_render(void)
{
    u8 i;
    const Hero *h = &st.hero;
    box(0, 0, 60, 100);                       // commands
    for (i = 0; i < NROOT; i++) draw_text(12, 5 + i * 10, root_name[i], F_MEDIUM, C_BLACK);
    if (st.menu == MN_ROOT) cursor(5, 6 + st.mcur * 10);
    else cursor(5, 6 + (st.menu - 1) * 10);
    if (st.mmsg == 3) draw_text(6, 88, "Saved.", F_SMALL, C_BLACK);
    if (st.mmsg == 4) draw_text(6, 88, "Save failed.", F_SMALL, C_BLACK);
    draw_text(6, 70, "Gils", F_SMALL, C_BLACK);
    num(56, 70, h->gils, F_SMALL);
    draw_text(6, 78, "Steps", F_SMALL, C_BLACK);
    num(56, 78, st.steps, F_SMALL);
    box(59, 0, 101, 100);                     // the page
    if (st.menu == MN_ROOT || st.menu == MN_STATUS) {
        draw_text(64, 5, st.name, F_MEDIUM, C_BLACK);
        draw_text(118, 6, "Lv", F_SMALL, C_BLACK);
        num(154, 6, h->lv, F_SMALL);
        draw_text(62, 16, "HP", F_SMALL, C_BLACK); num(122, 16, h->hp, F_SMALL); draw_text(124, 16, "/", F_SMALL, C_BLACK); num(154, 16, h->hpm, F_SMALL);
        draw_text(62, 23, "MP", F_SMALL, C_BLACK); num(122, 23, h->mp, F_SMALL); draw_text(124, 23, "/", F_SMALL, C_BLACK); num(154, 23, h->mpm, F_SMALL);
        draw_text(62, 30, "Next Lv", F_SMALL, C_BLACK); num(154, 30, h->expt - h->exp, F_SMALL);
        if (st.menu == MN_STATUS) {
            static const char *const sn[6] = { "Strength", "Vitality", "Magic", "Spirit", "Speed", "Luck" };
            for (i = 0; i < 6; i++) { draw_text(62, 40 + i * 7, sn[i], F_SMALL, C_BLACK); num(154, 40 + i * 7, h->st[i] / STAT, F_SMALL); }
            draw_text(62, 84, "Limit  Braver", F_SMALL, C_BLACK);
            draw_rect(62, 92, 92, 4, C_BLACK);
            draw_rect(63, 93, 90, 2, C_WHITE);
            if (h->jl) draw_rect(63, 93, (u32)h->jl * 90 / LIMIT_FULL, 2, C_DGRAY);
        } else {
            draw_text(62, 42, "Weapon", F_SMALL, C_BLACK); draw_text(96, 42, arm_name(h->weapon), F_SMALL, C_BLACK);
            draw_text(62, 50, "Armor", F_SMALL, C_BLACK); draw_text(96, 50, arm_name(h->armor), F_SMALL, C_BLACK);
            draw_text(62, 58, "Acc.", F_SMALL, C_BLACK); draw_text(96, 58, arm_name(h->acc[0]), F_SMALL, C_BLACK);
            draw_text(96, 66, arm_name(h->acc[1]), F_SMALL, C_BLACK);
        }
    } else if (st.menu == MN_ITEMS) {
        for (i = 0; i < NITEM; i++) {
            draw_text(66, 6 + i * 9, item_name[i], F_MEDIUM, st.item[i] ? C_BLACK : C_LGRAY);
            num(154, 6 + i * 9, st.item[i], F_MEDIUM);
        }
        cursor(59, 7 + st.mcur * 9);
        draw_text(62, 72, "HP", F_SMALL, C_BLACK); num(110, 72, h->hp, F_SMALL); draw_text(110, 72, "/", F_SMALL, C_BLACK); num(140, 72, h->hpm, F_SMALL);
        draw_text(62, 79, "MP", F_SMALL, C_BLACK); num(110, 79, h->mp, F_SMALL); draw_text(110, 79, "/", F_SMALL, C_BLACK); num(140, 79, h->mpm, F_SMALL);
        if (st.mmsg == 1) draw_text(62, 89, "Only in battle.", F_SMALL, C_BLACK);
    } else if (st.menu == MN_EQUIP) {
        static const char *const sl[4] = { "Weapon", "Armor", "Acc. 1", "Acc. 2" };
        const u8 cur[4] = { h->weapon, h->armor, h->acc[0], h->acc[1] };
        for (i = 0; i < 4; i++) {
            draw_text(66, 6 + i * 16, sl[i], F_SMALL, C_BLACK);
            draw_text(70, 13 + i * 16, arm_name(cur[i]), F_MEDIUM, C_BLACK);
        }
        cursor(59, 7 + st.mcur * 16);
        draw_text(62, 72, "Attack", F_SMALL, C_BLACK); num(154, 72, (u32)(h->st[S_STR] + (h->acc[0] == A_WRIST || h->acc[1] == A_WRIST ? 5 * STAT : 0)) * (h->weapon ? 10 : 7) / (2 * STAT), F_SMALL);
        draw_text(62, 79, "Defense", F_SMALL, C_BLACK); num(154, 79, h->st[S_DEF] / STAT + (h->armor ? 5 : 0), F_SMALL);
        draw_text(62, 86, "Slots", F_SMALL, C_BLACK); num(154, 86, h->weapon ? 2 : 0, F_SMALL);
    } else if (st.menu == MN_MATERIA) {
        draw_text(62, 6, arm_name(h->weapon), F_MEDIUM, C_BLACK);
        for (i = 0; i < 2; i++) {
            draw_text(66, 22 + i * 12, "Slot", F_SMALL, C_BLACK);
            num(90, 22 + i * 12, i + 1, F_SMALL);
            draw_text(98, 21 + i * 12, h->slot[i] ? mat_name[h->slot[i]] : "-", F_MEDIUM, C_BLACK);
        }
        cursor(59, 23 + st.mcur * 12);
        draw_text(62, 52, "Owned", F_SMALL, C_BLACK);
        draw_text(62, 60, st.mat[MAT_FIRE] ? "Fire" : "", F_SMALL, C_BLACK);
        draw_text(90, 60, st.mat[MAT_CURE] ? "Cure" : "", F_SMALL, C_BLACK);
        if (st.mmsg == 2 || !h->weapon) draw_text(62, 80, "No Slot", F_MEDIUM, C_BLACK);
    }
}
