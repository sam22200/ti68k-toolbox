// The management menus (fftmng, campaign/ENGINE.md core milestone): the campaign's party from
// its save (save.h), edited the way FFT does and written back when the menus close. The layout
// is the management prototype's classic grid (../MANAGEMENT.md, chosen by the user), still
// sprites. Everything shown or ruled on comes from the archived bank ffmg
// (tools/management_data.py): FFT's 20 generic jobs and Ramza's own Squire, items, names,
// icons, sprites, portraits, font.
// Screens: the formation (ENTER: a unit's Status; ALPHA / DIAMOND: its menu), then Equip,
// Abilities (secondary skill set, reaction, support, movement), Learn and Change Job. FFT's
// rules: an item fits its slot and the job's equipment types and comes from the inventory (the
// one taken off goes back); an ability costs its job's JP; a job needs the job levels its row
// lists (Bard: men, Dancer: women); a job change takes off what the new job cannot equip.
// Statistics are derived as the battle derives them (fft.c unit_stats): raw stats grown by
// level with the job's growth, then its multipliers, then the equipment.
#include "../../../runtime/core/rt.h"
#include "../art.h"
#include "../units.h"
#include "save.h"
#include <stdlib.h>
#include <string.h>
#ifdef RT_CYCLES
#include "../../../tools/m68kbench/bench.h"
#endif

enum { S_FORM, S_MENU, S_STATUS, S_EQUIP, S_SET, S_LEARN, S_JOBS, S_PICK, S_NONE };
enum { CAREERS = 20, RAMZA_JOB = 20, JOB_SIZE = 120, ITEM_SIZE = 14, SLOTS = 5, ROWS = 6 };
enum { EQ_HEAD, EQ_BODY, EQ_ACC, EQ_WEAPON, EQ_SHIELD };   // the save's order
typedef struct {
    u8 screen, back, unit, row, top, slot, confirm, note;   // note: a message (NOTE_*)
    u8 pick_n;                          // S_PICK: entries (pick[]: an item id or an ability)
    u16 pick[64];
    u8 changed;                         // the party edited (written at the end)
} Mg;
enum { NOTE_NONE, NOTE_JP, NOTE_LOCKED };
Mg mg;
Save save;
static const u8 *bank;
static u8 *cache;
static u8 dirty;

static u16 b16(const u8 *p) { return (u16)((u16)p[0] << 8 | p[1]); }
static const u8 *section(u8 k) { return bank + b16(bank + 4 + 2 * k); }
static const char *str(u16 at) { return at ? (const char *)bank + at : ""; }
static const u8 *job_rec(u8 k) { return section(0) + (u16)k * JOB_SIZE; }
static const u8 *item_rec(u8 id) { return section(1) + (u16)id * ITEM_SIZE; }
static const char *ability_name(u16 a) { return str(b16(section(2) + 2 * (a & 511))); }
static const char *set_name(u8 s) { return str(b16(section(3) + 2 * (s & 63))); }
// job record fields
#define J_ID 2
#define J_SET 3
#define J_GROWTH 4
#define J_MULT 9
#define J_MOVE 14
#define J_JUMP 15
#define J_CATS 16
#define J_REQ 20
#define J_ONLY 30
#define J_ABIL 32
#define J_COST 76
// item record fields
#define I_FLAGS 2
#define I_TYPE 3
#define I_HP 5
#define I_ICON 12
static const u8 slot_flag[SLOTS] = { 0x20, 0x10, 0x08, 0x80, 0x40 };

// ---------------------------------------------------------------- the party's rules
static SaveUnit *unit(u8 i) { return &save.unit[i]; }
static u8 career(const SaveUnit *u) { u8 c = (u8)(u->job - 0x4A); return c < CAREERS ? c : 0; }
// a career's job record: Ramza's own Squire for his first one
static u8 record(const SaveUnit *u, u8 c) { return c == 0 && u->type == 2 ? RAMZA_JOB : c; }
static u16 jp(const SaveUnit *u, u8 c) { return b16(u->jp[c]); }
static u16 tjp(const SaveUnit *u, u8 c) { return b16(u->tjp[c]); }
static void set16(u8 *p, u16 v) { p[0] = (u8)(v >> 8); p[1] = (u8)v; }

u8 job_level(const SaveUnit *u, u8 c)    // FFT's: level 2 at 200 JP earned .. 8 at 2100
{
    static const u16 need[7] = { 200, 350, 550, 800, 1150, 1550, 2100 };
    u8 k = 0;
    while (k < 7 && tjp(u, c) >= need[k]) k++;
    return (u8)(k + 1);
}

u8 learned(const SaveUnit *u, u8 c, u8 k) { return (u->learned[c][k >> 3] >> (7 - (k & 7))) & 1; }

// the job levels career c needs, met; 0: locked (and *what: the first career short of it)
u8 unlocked(const SaveUnit *u, u8 c, u8 *what)
{
    const u8 *j = job_rec(c);
    u8 r, only = j[J_ONLY];
    if (only && only != (u->type == 1 ? 2 : 1)) { if (what) *what = 0xFF; return 0; }
    for (r = 0; r < CAREERS; r++) {
        u8 need = (u8)(j[J_REQ + (r >> 1)] >> (r & 1 ? 0 : 4) & 15);
        if (need && job_level(u, r) < need) { if (what) *what = r; return 0; }
    }
    return 1;
}

u8 can_equip(const SaveUnit *u, u8 slot, u8 id)
{
    const u8 *it = item_rec(id), *j = job_rec(record(u, career(u)));
    u8 t = it[I_TYPE];
    return (it[I_FLAGS] & slot_flag[slot]) && (j[J_CATS + (t >> 3)] & (0x80 >> (t & 7)));
}

typedef struct { u16 hp, mp; u8 sp, pa, ma, move, jump; } Stats;
Stats stats(const SaveUnit *u)             // fft.c unit_stats, from the save's records
{
    const u8 *j = job_rec(record(u, career(u)));
    Stats s;
    u16 v16[5];
    u8 i, k;
    for (i = 0; i < 5; i++) {
        u32 v = (u32)u->raw[i][0] << 16 | (u16)u->raw[i][1] << 8 | u->raw[i][2];
        u8 g = j[J_GROWTH + i] ? j[J_GROWTH + i] : 1;
        for (k = 2; k <= u->level; k++) v += v / (u16)(g + k - 1);
        if (v > 0xFFFFFFUL) v = 0xFFFFFFUL;
        v = v * j[J_MULT + i] / 100 >> 14;
        v16[i] = v ? (u16)v : 1;
    }
    s.hp = v16[0]; s.mp = v16[1]; s.move = j[J_MOVE]; s.jump = j[J_JUMP];
    for (i = 0; i < SLOTS; i++) if (u->eq[i]) {
        const u8 *it = item_rec(u->eq[i]);
        s.hp += it[I_HP]; s.mp += it[I_HP + 1];
        v16[2] += it[I_HP + 4]; v16[3] += it[I_HP + 2]; v16[4] += it[I_HP + 3];
        s.move += it[I_HP + 5]; s.jump += it[I_HP + 6];
    }
    if (s.hp > 999) s.hp = 999;
    if (s.mp > 999) s.mp = 999;
    if (s.jump > 7) s.jump = 7;
    s.sp = (u8)(v16[2] > 50 ? 50 : v16[2]);
    s.pa = (u8)(v16[3] > 99 ? 99 : v16[3]);
    s.ma = (u8)(v16[4] > 99 ? 99 : v16[4]);
    return s;
}

void equip(SaveUnit *u, u8 slot, u8 id)    // id 0: taken off; the inventory keeps the count
{
    if (u->eq[slot] && save.inv[u->eq[slot]] < 99) save.inv[u->eq[slot]]++;
    if (id) save.inv[id]--;
    u->eq[slot] = id;
    mg.changed = 1;
}

// k-th entry of the unit's current job: learned with its JP (1), else 0
u8 learn(SaveUnit *u, u8 k)
{
    u8 c = career(u);
    const u8 *j = job_rec(record(u, c));
    u16 cost = b16(j + J_COST + 2 * k);
    if (!b16(j + J_ABIL + 2 * k) || learned(u, c, k) || jp(u, c) < cost) return 0;
    set16(u->jp[c], (u16)(jp(u, c) - cost));
    u->learned[c][k >> 3] |= (u8)(0x80 >> (k & 7));
    mg.changed = 1;
    return 1;
}

// the ability kind of FFT id a: 1 reaction, 2 support, 3 movement (0 an action)
static u8 kind(u16 a) { return a >= 0x1E6 ? 3 : a >= 0x1C6 ? 2 : a >= 0x1A6 ? 1 : 0; }
static u8 *slot_of(SaveUnit *u, u8 k) { return k == 1 ? u->reaction : k == 2 ? u->support : u->movement; }

void change_job(SaveUnit *u, u8 c)
{
    u8 s;
    u->job = (u8)(c == 0 && u->type == 2 ? 0x01 : 0x4A + c);
    // the battle's sprite (units.h) when it has one for the job
    u->gfx = u->type == 2 ? UG_RAMZA : c == 0 ? (u->type ? UG_SQUIRE_F : UG_SQUIRE_M)
           : c == 1 ? (u->type ? UG_CHEMIST_F : UG_CHEMIST_M) : 0xFF;
    for (s = 0; s < SLOTS; s++) if (u->eq[s] && !can_equip(u, s, u->eq[s])) equip(u, s, 0);
    if (u->secondary == job_rec(record(u, c))[J_SET]) u->secondary = 0;
    mg.changed = 1;
}

// the choices of an ability slot (1..3) or the secondary skill set (0): FFT ids into mg.pick
static void choices(SaveUnit *u, u8 which)
{
    u8 c, k, n = 1, own = career(u);
    mg.pick[0] = 0;                                // "Remove"
    for (c = 0; c < CAREERS; c++) {
        const u8 *j = job_rec(record(u, c));
        if (!which) {
            if (c == own) continue;
            for (k = 0; k < 16; k++) if (learned(u, c, k) && b16(j + J_ABIL + 2 * k)) break;
            if (k < 16) mg.pick[n++] = j[J_SET];
            continue;
        }
        for (k = 16; k < 22; k++) {
            u16 a = b16(j + J_ABIL + 2 * k);
            if (a && learned(u, c, k) && kind(a) == which && n < 64) mg.pick[n++] = a;
        }
    }
    mg.pick_n = n;
}

static void item_choices(SaveUnit *u, u8 slot)
{
    u16 id;
    u8 n = 1;
    mg.pick[0] = 0;
    for (id = 1; id < 256; id++)
        if (save.inv[id] && can_equip(u, slot, (u8)id) && n < 64) mg.pick[n++] = id;
    mg.pick_n = n;
}

// ---------------------------------------------------------------- drawing
static const u8 *menu_font(void) { return section(7); }

#include "mgtext.inc"

static void heading(const char *s) { draw_rect(0, 0, 160, 13, C_BLACK); text(5, 4, s, C_WHITE, 153); }
static void foot(const char *s) { draw_rect(0, 93, 160, 1, C_BLACK); black(2, 95, s); }

#ifndef __m68k__
static u32 b32(const u8 *p) { return (u32)b16(p) << 16 | b16(p + 2); }
#endif
// still pictures of the bank: per career and sex (2 c + 1 for a woman), Ramza's at 40 (FFT draws
// him alike in every job)
static u8 picture(const SaveUnit *u, u8 c) { return u->type == 2 ? 40 : (u8)(2 * c + (u->type == 1)); }
static void actor(s16 x, s16 y, u8 k)
{
    const u8 *p = section(5) + (u16)k * 156;
#ifdef __m68k__
    RtSprite sp = { 16, 26, p, p + 52, p + 104 };
#else
    u16 r[3][26]; u8 i, j;
    RtSprite sp = { 16, 26, r[0], r[1], r[2] };
    for (i = 0; i < 3; i++) for (j = 0; j < 26; j++) r[i][j] = b16(p + i * 52 + 2 * j);
#endif
    draw_sprite(x, y, &sp);
}
static void portrait(s16 x, s16 y, u8 k)
{
    const u8 *p = section(6) + (u16)k * 348;
#ifdef __m68k__
    RtSprite sp = { 32, 29, p, p + 116, p + 232 };
#else
    u32 r[3][29]; u8 i, j;
    RtSprite sp = { 32, 29, r[0], r[1], r[2] };
    for (i = 0; i < 3; i++) for (j = 0; j < 29; j++) r[i][j] = b32(p + i * 116 + 4 * j);
#endif
    draw_sprite(x, y, &sp);
}
static void icon(s16 x, s16 y, u8 id)
{
    const u8 *p = section(4) + (u16)item_rec(id)[I_ICON] * 24;
    RtSprite sp = { 8, 8, p, p + 8, p + 16 };
    draw_sprite(x, y, &sp);
}
static void hand(s16 x, s16 y)
{
    RtSprite sp = { 32, HAND_H, hand_gfx[0], hand_gfx[1], hand_gfx[2] };
    draw_sprite(x, y, &sp);
}

static const char *unit_name(const SaveUnit *u)
{
    if (u->type == 2) return "Ramza";
    return str(b16(section(8) + 2 * (u->name + (u->type == 1 ? 255 : 0))));
}
static const char *job_name(const SaveUnit *u, u8 c) { return str(b16(job_rec(record(u, c)))); }

static void unit_header(void)
{
    const SaveUnit *u = unit(mg.unit);
    Stats s = stats(u);
    u8 c = career(u);
    portrait(5, 15, picture(u, c));
    black(40, 16, unit_name(u)); black(40, 25, job_name(u, c));
    black(92, 25, "Lv"); right(110, 25, u->level);
    black(40, 36, "HP"); right(70, 36, s.hp);
    black(78, 36, "MP"); right(106, 36, s.mp);
    black(112, 36, "JP"); right(138, 36, jp(u, c));
    actor(140, 15, picture(u, c));
    draw_rect(2, 46, 156, 1, C_LGRAY);
}

static const char *const menu_rows[] = { "Equip", "Abilities", "Learn", "Change Job" };
static const char *const slot_rows[] = { "Head", "Body", "Accessory", "R.Hand", "L.Hand" };
static const char *const set_rows[] = { "Secondary", "Reaction", "Support", "Movement" };
static const u8 slot_order[SLOTS] = { EQ_WEAPON, EQ_SHIELD, EQ_HEAD, EQ_BODY, EQ_ACC };   // shown

static void formation(void)
{
    u8 i, first = (u8)(mg.top * 4);
    const SaveUnit *u = unit(mg.unit);
    for (i = 0; i < 8 && first + i < save.n; i++) {
        s16 x = 3 + (i & 3) * 39, y = 16 + (i >> 2) * 32;
        const SaveUnit *v = unit((u8)(first + i));
        if (first + i == mg.unit) { draw_rect(x, y, 37, 31, C_LGRAY); draw_rect(x, y + 30, 37, 1, C_BLACK); }
        else draw_rect(x + 4, y + 30, 29, 1, C_LGRAY);
        black(x + 1, y + 1, "Lv"); number(x + 10, y + 1, v->level);
        actor(x + 14, y + 3, picture(v, career(v)));
    }
    draw_rect(0, 81, 160, 1, C_BLACK);
    black(4, 84, unit_name(u)); black(84, 84, job_name(u, career(u)));
    if (save.n > 8) { black(150, 16, mg.top ? "^" : " "); black(150, 72, (u8)(mg.top * 4 + 8) < save.n ? "v" : " "); }
    if (mg.screen == S_FORM) { hand(1 + (mg.unit & 3) * 39, 32 + ((mg.unit >> 2) - mg.top) * 32); foot("ENTER:status ALPHA:menu ESC:save"); }
}

static void status_page(void)
{
    const SaveUnit *u = unit(mg.unit);
    Stats s = stats(u);
    u8 c = career(u);
    static const char *const names[] = { "Move", "Jump", "Speed", "PA", "MA", "Brave", "Faith", "Job Lv" };
    u8 v[8], i;
    v[0] = s.move; v[1] = s.jump; v[2] = s.sp; v[3] = s.pa; v[4] = s.ma; v[5] = u->brave; v[6] = u->faith;
    v[7] = job_level(u, c);
    for (i = 0; i < 8; i++) {
        s16 x = i < 4 ? 6 : 85, y = 50 + (i & 3) * 7;
        black(x, y, names[i]); right(x + 66, y, v[i]);
    }
    black(6, 80, "EXP"); number(26, 80, u->exp);
    black(85, 80, "Total JP"); right(151, 80, tjp(u, c));
    foot("F1/F5:unit SHIFT:back");
}

static void equip_page(void)
{
    const SaveUnit *u = unit(mg.unit);
    u8 i;
    for (i = 0; i < SLOTS; i++) {
        u8 s = slot_order[i], id = u->eq[s];
        s16 y = 49 + i * 8;
        black(31, y, slot_rows[s]);
        if (id) { icon(72, y - 1, id); black(82, y, str(b16(item_rec(id)))); }
        else black(82, y, "-");
    }
    foot("ENTER:change SHIFT:back F1/F5:unit");
}

static const char *ability_or_set(u8 which, u16 a) { return !a ? "-" : which ? ability_name(a) : set_name((u8)a); }
static u16 slot_value(const SaveUnit *u, u8 which)
{
    return which ? b16(slot_of((SaveUnit *)u, which)) : u->secondary;
}

static void set_page(void)
{
    const SaveUnit *u = unit(mg.unit);
    u8 i;
    for (i = 0; i < 4; i++) {
        black(31, 49 + i * 8, set_rows[i]);
        black(82, 49 + i * 8, ability_or_set(i, slot_value(u, i)));
    }
    foot("ENTER:change SHIFT:back F1/F5:unit");
}

// Learn: the current job's abilities (its list's entries), cost or learned
static u8 learn_entry(const SaveUnit *u, u8 row)   // the row-th entry of the list (22: none)
{
    const u8 *j = job_rec(record(u, career(u)));
    u8 k;
    for (k = 0; k < 22; k++) if (b16(j + J_ABIL + 2 * k) && !row--) return k;
    return 22;
}
static u8 learn_count(const SaveUnit *u) { u8 n = 0; while (learn_entry(u, n) < 22) n++; return n; }

static void list_rows(u8 n, void (*one)(u8 k, s16 y))
{
    u8 i;
    for (i = mg.top; i < n && i < mg.top + ROWS; i++) one(i, (s16)(49 + (i - mg.top) * 7));
    if (mg.top) black(154, 49, "^");
    if (mg.top + ROWS < n) black(154, 84, "v");
}

static void learn_row(u8 i, s16 y)
{
    const SaveUnit *u = unit(mg.unit);
    const u8 *j = job_rec(record(u, career(u)));
    u8 k = learn_entry(u, i);
    black(31, y, ability_name(b16(j + J_ABIL + 2 * k)));
    if (learned(u, career(u), k)) black(118, y, "Learned");
    else right(146, y, b16(j + J_COST + 2 * k));
}

static void job_row(u8 c, s16 y)
{
    const SaveUnit *u = unit(mg.unit);
    black(31, y, job_name(u, c));
    if (c == career(u)) black(90, y, "Now");
    else if (unlocked(u, c, RT_NULL)) { black(90, y, "Lv"); number(100, y, job_level(u, c)); }
    else black(90, y, "Locked");
}

static void pick_row(u8 i, s16 y)
{
    u16 a = mg.pick[i];
    if (mg.back == S_EQUIP) {
        if (!a) { black(31, y, "Remove"); return; }
        icon(29, y - 1, (u8)a); black(39, y, str(b16(item_rec((u8)a))));
        black(134, y, "x"); number(138, y, save.inv[a]);
    } else black(31, y, a ? ability_or_set(mg.slot, a) : "Remove");
}

static void note_panel(void)
{
    static const char *const notes[] = { "", "Not enough JP", "" };
    if (!mg.note) return;
    panel(5, 60, 150, 24);
    if (mg.note == NOTE_LOCKED) {
        const SaveUnit *u = unit(mg.unit);
        u8 what, c = mg.top + mg.row, need;
        unlocked(u, c, &what);
        if (what == 0xFF) black(12, 64, "Not for this unit");
        else {
            need = (u8)(job_rec(c)[J_REQ + (what >> 1)] >> (what & 1 ? 0 : 4) & 15);
            black(12, 64, "Needs"); { s16 x = black(36, 64, job_name(u, what)); black(x + 4, 64, "Lv"); number(x + 14, 64, need); }
        }
    } else black(12, 64, notes[mg.note]);
    black(12, 74, "ENTER or SHIFT");
}

static void confirm_panel(void)
{
    const SaveUnit *u = unit(mg.unit);
    if (!mg.confirm) return;
    panel(5, 60, 150, 24);
    if (mg.screen == S_JOBS) { s16 x = black(12, 64, "Change to"); black(x + 4, 64, job_name(u, (u8)(mg.top + mg.row))); }
    else {
        const u8 *j = job_rec(record(u, career(u)));
        u8 k = learn_entry(u, (u8)(mg.top + mg.row));
        s16 x = black(12, 64, "Learn"); x = black(x + 4, 64, ability_name(b16(j + J_ABIL + 2 * k)));
        black(x + 4, 64, "?");
    }
    black(12, 74, "ENTER:yes SHIFT:no");
}

static void page(void)
{
    static const char *const titles[] = { "Formation", "Formation", "Status", "Equip", "Abilities", "Learn", "Change Job", "" };
    draw_clear();
    if (!bank) { draw_text(4, 40, "Missing ffmg", 0, C_BLACK); return; }
    if (!save.n) { heading("Formation"); black(8, 40, "No campaign save:"); black(8, 48, "win the battle at Gariland."); foot("ESC:quit"); return; }
    heading(mg.screen == S_PICK ? (mg.back == S_EQUIP ? slot_rows[mg.slot] : set_rows[mg.slot]) : titles[mg.screen]);
    if (mg.screen == S_FORM || mg.screen == S_MENU) {
        formation();
        if (mg.screen == S_MENU) {
            panel(56, 15, 101, 18 + 4 * 8); black(65, 19, "Menu");
            { u8 i; for (i = 0; i < 4; i++) black(81, 29 + i * 8, menu_rows[i]); }
            hand(57, 25 + mg.row * 8);
            foot("ENTER:open SHIFT:back");
        }
        note_panel();
        return;
    }
    unit_header();
    switch (mg.screen) {
    case S_STATUS: status_page(); break;
    case S_EQUIP: equip_page(); hand(5, 45 + mg.row * 8); break;
    case S_SET: set_page(); hand(5, 45 + mg.row * 8); break;
    case S_LEARN:
        list_rows(learn_count(unit(mg.unit)), learn_row);
        hand(5, 45 + mg.row * 7); foot("ENTER:learn SHIFT:back F1/F5:unit"); break;
    case S_JOBS:
        list_rows(CAREERS, job_row);
        if (!mg.confirm && !mg.note) actor(122, 56, picture(unit(mg.unit), (u8)(mg.top + mg.row)));
        hand(5, 45 + mg.row * 7); foot("ENTER:change SHIFT:back F1/F5:unit"); break;
    case S_PICK:
        list_rows(mg.pick_n, pick_row);
        hand(5, 45 + mg.row * 7); foot("ENTER:choose SHIFT:back"); break;
    }
    confirm_panel();
    note_panel();
}

// ---------------------------------------------------------------- the frame loop
static void open_list(u8 screen, u8 at)          // a scrolling list, its entry at shown
{
    mg.screen = screen;
    mg.top = at >= ROWS ? (u8)(at - ROWS + 1) : 0;
    mg.row = (u8)(at - mg.top);
}

void game_init(void)
{
    u16 size;
    bank = RT_NULL; cache = RT_NULL;     // (statics survive between runs)
    memset(&mg, 0, sizeof mg);
    bank = rt_file("ffmg", &size);
    if (bank && memcmp(bank, "FMG1", 4)) bank = RT_NULL;
    if (!rt_load(SAVE_NAME, &save, sizeof save) || memcmp(save.magic, SAVE_MAGIC, 4)) memset(&save, 0, sizeof save);
    if (save.n > PARTY_MAX) save.n = 0;
    cache = (u8 *)malloc(2 * RT_PSIZE);
    dirty = 1;
    rt_state = &mg; rt_state_size = sizeof mg;
}

void game_scenario(u16 n)                // test doors: a screen of the first unit
{
    static const u8 doors[] = { S_FORM, S_STATUS, S_EQUIP, S_SET, S_LEARN, S_JOBS, S_MENU };
    mg.screen = n < sizeof doors ? doors[n] : S_FORM;
    mg.unit = mg.row = mg.top = 0;
    if (mg.screen == S_JOBS) open_list(S_JOBS, career(unit(0)));
    dirty = 1;
}

static void leave(void)
{
    if (mg.changed) rt_save(SAVE_NAME, &save, sizeof save);
    if (cache) { free(cache); cache = RT_NULL; }
}

static u8 list_move(u32 key, u8 n)              // up / down in a scrolling list
{
    u8 at = (u8)(mg.top + mg.row);
    if (!n) return 0;
    if (key & K_UP) at = at ? (u8)(at - 1) : (u8)(n - 1);
    if (key & K_DOWN) at = (u8)(at + 1 == n ? 0 : at + 1);
    if (at < mg.top) mg.top = at;
    if (at >= mg.top + ROWS) mg.top = (u8)(at - ROWS + 1);
    mg.row = (u8)(at - mg.top);
    return at;
}

u8 game_update(void)
{
    u32 key = rt_keys & ~rt_prev;
    SaveUnit *u;
    u8 at;
    if (!key) return 1;
    dirty = 1;
    if (!bank || !save.n) { if (key & K_ESC) { leave(); return 0; } return 1; }
    u = unit(mg.unit);
    if (mg.note) { if (key & (K_ENTER | K_A | K_B | K_ESC)) mg.note = 0; return 1; }
    if (mg.confirm) {
        if (key & (K_B | K_ESC)) mg.confirm = 0;
        else if (key & (K_ENTER | K_A)) {
            mg.confirm = 0;
            if (mg.screen == S_JOBS) change_job(u, (u8)(mg.top + mg.row));
            else if (!learn(u, learn_entry(u, (u8)(mg.top + mg.row)))) mg.note = NOTE_JP;
        }
        return 1;
    }
    if ((key & (K_F1 | K_F5)) && mg.screen != S_FORM && mg.screen != S_MENU && mg.screen != S_PICK) {
        mg.unit = (u8)((mg.unit + (key & K_F1 ? save.n - 1 : 1)) % save.n);
        if (mg.screen == S_JOBS) open_list(S_JOBS, career(unit(mg.unit)));
        else if (mg.screen == S_LEARN) mg.row = mg.top = 0;
        return 1;
    }
    switch (mg.screen) {
    case S_FORM:
        if (key & K_ESC) { leave(); return 0; }
        at = mg.unit;
        if (key & K_LEFT) at = at ? (u8)(at - 1) : (u8)(save.n - 1);
        if (key & K_RIGHT) at = (u8)(at + 1 == save.n ? 0 : at + 1);
        if (key & K_UP) at = at >= 4 ? (u8)(at - 4) : at;
        if (key & K_DOWN) at = at + 4 < save.n ? (u8)(at + 4) : at;
        mg.unit = at;
        if ((at >> 2) < mg.top) mg.top = (u8)(at >> 2);
        if ((at >> 2) > mg.top + 1) mg.top = (u8)((at >> 2) - 1);
        if (key & (K_ENTER | K_A)) mg.screen = S_STATUS;
        if (key & (K_C | K_D)) { mg.screen = S_MENU; mg.row = 0; }
        return 1;
    case S_MENU:
        if (key & (K_B | K_ESC)) { mg.screen = S_FORM; return 1; }
        if (key & K_UP) mg.row = (u8)((mg.row + 3) & 3);
        if (key & K_DOWN) mg.row = (u8)((mg.row + 1) & 3);
        if (key & (K_ENTER | K_A)) {
            static const u8 dest[] = { S_EQUIP, S_SET, S_LEARN, S_JOBS };
            mg.screen = dest[mg.row]; mg.row = mg.top = 0;
            if (mg.screen == S_JOBS) open_list(S_JOBS, career(u));
        }
        return 1;
    case S_STATUS:
        if (key & (K_B | K_ESC | K_ENTER | K_A)) mg.screen = S_FORM;
        return 1;
    case S_EQUIP: case S_SET:
        if (key & (K_B | K_ESC)) { mg.row = mg.screen == S_EQUIP ? 0 : 1; mg.screen = S_MENU; return 1; }
        {
            u8 n = mg.screen == S_EQUIP ? SLOTS : 4;
            if (key & K_UP) mg.row = mg.row ? (u8)(mg.row - 1) : (u8)(n - 1);
            if (key & K_DOWN) mg.row = (u8)(mg.row + 1 == n ? 0 : mg.row + 1);
        }
        if (key & (K_ENTER | K_A)) {
            mg.back = mg.screen;
            if (mg.screen == S_EQUIP) { mg.slot = slot_order[mg.row]; item_choices(u, mg.slot); }
            else { mg.slot = mg.row; choices(u, mg.row); }
            open_list(S_PICK, 0);
        }
        return 1;
    case S_PICK:
        if (key & (K_B | K_ESC)) {
            mg.screen = mg.back; mg.top = 0;
            if (mg.back == S_EQUIP) { for (at = 0; slot_order[at] != mg.slot; at++) ; mg.row = at; }
            else mg.row = mg.slot;
            return 1;
        }
        at = list_move(key, mg.pick_n);
        if (key & (K_ENTER | K_A)) {
            u16 v = mg.pick[at];
            if (mg.back == S_EQUIP) equip(u, mg.slot, (u8)v);
            else if (!mg.slot) { u->secondary = (u8)v; mg.changed = 1; }
            else { set16(slot_of(u, mg.slot), v); mg.changed = 1; }
            mg.screen = mg.back; mg.top = 0;
            if (mg.back == S_EQUIP) { for (at = 0; slot_order[at] != mg.slot; at++) ; mg.row = at; }
            else mg.row = mg.slot;
        }
        return 1;
    case S_LEARN:
        if (key & (K_B | K_ESC)) { mg.screen = S_MENU; mg.row = 2; return 1; }
        at = list_move(key, learn_count(u));
        if ((key & (K_ENTER | K_A)) && !learned(u, career(u), learn_entry(u, at))) {
            const u8 *j = job_rec(record(u, career(u)));
            if (jp(u, career(u)) < b16(j + J_COST + 2 * learn_entry(u, at))) mg.note = NOTE_JP;
            else mg.confirm = 1;
        }
        return 1;
    case S_JOBS:
        if (key & (K_B | K_ESC)) { mg.screen = S_MENU; mg.row = 3; mg.top = 0; return 1; }
        at = list_move(key, CAREERS);
        if ((key & (K_ENTER | K_A)) && at != career(u)) {
            if (unlocked(u, at, RT_NULL)) mg.confirm = 1; else mg.note = NOTE_LOCKED;
        }
        return 1;
    }
    return 1;
}

void game_render(void)
{
    if (!cache) { draw_clear(); draw_text(4, 40, "Not enough memory", 0, C_BLACK); return; }
    if (dirty) {                         // a still page: drawn once into the cache
        void *l = rt_light, *d = rt_dark;
        rt_light = cache; rt_dark = cache + RT_PSIZE;
        page();
        rt_light = l; rt_dark = d;
        dirty = 0;
    }
    memcpy(rt_light, cache, RT_PSIZE); memcpy(rt_dark, cache + RT_PSIZE, RT_PSIZE);
}

