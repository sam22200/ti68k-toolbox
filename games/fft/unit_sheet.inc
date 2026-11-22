#ifndef FFT_UNIT_SHEET_INC
#define FFT_UNIT_SHEET_INC
// Requires US_TEXT(x,y,s,color,limit); the plate itself also needs US_PORTRAIT(planes,x,y),
// and a tab bar needs US_WIDTH(label). A caller that only wants the shared "this value
// moved" emphasis may leave the others out.
#include "unit_sheet.h"

// Every column of the plate is one family of numbers, so a glance goes straight down to the
// group it wants. The left column is the unit itself, under its name and job.
//
//      col 1        col 2      col 3      col 4
//  y1  <name>       PA         Mv         CE
//  y7  <job>        MA         Jp         SE
//  y13 Lv           AT         Sp         AE
//  y19 HP           Br         JLv        WE
//  y25 MP           Fa         CT         ME
//      the unit     its blow   its reach  its guard
//
// Brave and Faith sit with the blow because FFT multiplies by them: Faith scales magic both
// ways, Brave drives the physical reactions. CT closes the reach column, and only a battle
// fills it. Experience was dropped for the room, with the user on 2026-11-22: it is the one
// number here that no screen acts on.
static const char *const us_label[US_N] = { "Lv", "HP", "MP",
    "PA", "MA", "AT", "Br", "Fa",
    "Mv", "Jp", "Sp", "JLv", "CT",
    "CE", "SE", "AE", "WE", "ME" };
static const u8 us_x[US_N]  = { 33, 33, 33,
    73, 73, 73, 73, 73,   106, 106, 106, 106, 106,   133, 133, 133, 133, 133 };
static const u8 us_at[US_N] = { 70, 70, 70,
    103, 103, 103, 103, 103,   130, 130, 130, 130, 130,   158, 158, 158, 158, 158 };
static const u8 us_row[US_N] = { 13, 19, 25,
    1, 7, 13, 19, 25,   1, 7, 13, 19, 25,   1, 7, 13, 19, 25 };

// A cell's number, right-aligned on the column's edge; with a maximum it reads "44/51",
// which is what a battle needs of HP and MP and a saved roster never does.
static u8 us_width(u16 n, u16 max)
{
    u8 w = 4;
    while (n > 9) { n /= 10; w = (u8)(w + 4); }
    if (max) { w = (u8)(w + 4); while (max > 9) { max /= 10; w = (u8)(w + 4); } }
    return w;
}
static void us_number(s16 at, s16 y, u16 n, u16 max, u8 color)
{
    char b[12];
    u8 k = 11;
    b[k] = 0;
    if (max) { do { b[--k] = (char)('0' + max % 10); max /= 10; } while (max); b[--k] = '/'; }
    do { b[--k] = (char)('0' + n % 10); n /= 10; } while (n);
    US_TEXT((s16)(at - (11 - k) * 4), y, b + k, color, 159);
}

// A value a pending change moved, lifted out of whatever it sits on: white ground, black
// ink, and a triangle whose narrow end points the way the number went. `at` is its right
// edge. This is the one emphasis for a preview anywhere -- the sheet's plate and the
// shop's fitting bench both call it, so a change always reads the same.
static void us_changed(s16 at, s16 y, u16 then, u8 grew)
{
    u8 w = us_width(then, 0), i;
    draw_rect((s16)(at - w - 7), (s16)(y - 1), (s16)(w + 8), 8, C_WHITE);
    us_number(at, y, then, 0, C_BLACK);
    for (i = 0; i < 3; i++)
        draw_rect((s16)(at - w - 4 - i), (s16)(grew ? y + 1 + i : y + 4 - i),
                  (s16)(1 + 2 * i), 1, C_BLACK);
}
// One cell of the plate, with its short name on the left and its value on the column's edge.
static void us_cell(const UnitSheet *d, const UnitSheet *soon, u8 k, s16 top)
{
    s16 at = (s16)us_at[k], y = (s16)(top + us_row[k]);
    u16 now, then, max;
    if (!((d->shown >> k) & 1)) return;            // this owner has no such number
    now = d->v[k]; then = soon ? soon->v[k] : now;
    max = k == US_HP ? d->max_hp : k == US_MP ? d->max_mp : 0;
    if (soon && ((US_LIVE >> k) & 1) && now != then) {
        US_TEXT((s16)us_x[k], y, us_label[k], C_WHITE, (u8)(at - us_width(then, 0) - 8));
        us_changed(at, y, then, (u8)(then > now));
        return;
    }
    US_TEXT((s16)us_x[k], y, us_label[k], C_WHITE, (u8)(at - us_width(now, max) - 1));
    us_number(at, y, now, max, C_WHITE);
}

#ifdef US_WIDTH
// The sheet's tab bar, the one that replaced a whole menu level: the active tab is lit, the
// others are separated by a thin rule, and `badge` -- the keys that change unit, where the
// source puts its L1 / R1 corner badges -- closes the bar on the right.
enum { US_TAB_H = 8 };
static void unit_sheet_tabs(s16 top, const char *const *name, u8 n, u8 active, const char *badge)
{
    u8 i;
    s16 x = 0;
    draw_rect(0, top, 160, US_TAB_H, C_BLACK);
    for (i = 0; i < n; i++) {
        s16 w = (s16)(US_WIDTH(name[i]) + 7);
        if (i == active) draw_rect(x, top, w, US_TAB_H, C_WHITE);
        US_TEXT((s16)(x + 4), (s16)(top + 1), name[i], i == active ? C_BLACK : C_WHITE, 159);
        if (i != active) draw_rect((s16)(x + w - 1), (s16)(top + 2), 1, 4, C_DGRAY);
        x = (s16)(x + w);
    }
    if (badge) {
        s16 at = (s16)(158 - US_WIDTH(badge));
        for (i = 0; i < 3; i++) {
            draw_rect((s16)(at - 6 + i), (s16)(top + 3 - i), 1, (s16)(1 + 2 * i), C_WHITE);
            draw_rect((s16)(159 - i), (s16)(top + 3 - i), 1, (s16)(1 + 2 * i), C_WHITE);
        }
        US_TEXT((s16)(at - 2), (s16)(top + 1), badge, C_LGRAY, 156);
    }
}
#endif

#ifdef US_PORTRAIT
// The plate, drawn at `top`: 0 on the unit sheet and on the battle's Status page, the foot
// of the roster on Formation, the head of the deployment screen.
static void unit_sheet_plate(const UnitSheet *d, const UnitSheet *soon, s16 top)
{
    u8 k;
    draw_rect(0, top, 160, US_PLATE_H, C_BLACK);
    if (d->portrait) US_PORTRAIT(d->portrait, 0, top);
    US_TEXT(33, (s16)(top + 1), d->name, C_WHITE, 72);
    US_TEXT(33, (s16)(top + 7), d->job, C_WHITE, 72);
    for (k = 0; k < US_N; k++) us_cell(d, soon, k, top);
}
#endif
#endif
