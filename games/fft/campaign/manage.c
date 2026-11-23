// The management menus (fftmng, campaign/ENGINE.md core milestone): the campaign's party from
// its save (save.h), edited the way FFT does and written back when the menus close. The layout
// is the management prototype's classic grid (../MANAGEMENT.md, chosen by the user), still
// sprites. Everything shown or ruled on comes from the archived banks ffmg/ffcj
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
#include "../common.h"
#include "../identity_pool.h"
#include <stdlib.h>
#include <string.h>
#include "../monster_formation.h"
#ifndef __m68k__
#include "story_choice_file.h"
#endif
#ifdef RT_CYCLES
#include "../../../tools/m68kbench/bench.h"
#endif

enum { S_FORM, S_MENU, S_STATUS, S_EQUIP, S_SET, S_LEARN, S_JOBS, S_PICK,
       S_ITEM, S_INV, S_ORDER, S_JOBTABLE, S_NONE };
enum { CAREERS = 20, RAMZA_JOB = 20, JOB_SIZE = 32, ITEM_SIZE = 16, SLOTS = 5, ROWS = 6 };
enum { EQ_HEAD, EQ_BODY, EQ_ACC, EQ_WEAPON, EQ_SHIELD };   // the save's order
enum { JOB_CHANGE_FRAMES=96 }; // source-observed light column, adapted to30Hz/four greys
typedef struct {
    u8 screen, back, unit, row, top, slot, confirm, note;   // note: a message (NOTE_*)
    u8 detail;                          // [MODE]: the shared item detail panel
    u8 pick_n;                          // S_PICK: entries (pick[]: an item id or an ability)
    u16 pick[64];
    u8 changed;                         // the party edited (written at the end)
    u8 jobs[CAREERS], angle[CAREERS], job_n, job_pos;
    s8 spin;
    u8 slide,job_anim,old_picture;
    u8 remove;                          // Remove Unit: 1 the check, 2 the unit's reply
    u8 remove_row;                      // its Yes/No, then Remove/Quit
    u8 ljob;                            // S_LEARN: the career whose abilities are listed
    u8 inv_all;                         // Inventory: 0 what this unit can equip, 1 everything
    u8 inv_n;                           // its entries
    u8 inv[128];                        // their item ids
    s16 floor_from_x,floor_from_y;
} Mg;
enum { NOTE_NONE, NOTE_JP, NOTE_LOCKED, NOTE_SAVE, NOTE_RAMZA };
#define MENU_ROWS 2
Mg mg;
Save save;
static const u8 *bank, *common,*identity,*egg_bank,*monster_bank,*item_text;
static u8 *cache;
static u8 dirty;
static u8 settled;                       // hidden planes already carrying this page
static u8 props_on;
static u16 props_result=1;
static void props_start(u16 n);
static u8 props_update(u32 key);
static void props_page(void);
static void props_live(void);
u16 manage_result(void){return props_result;}


static u16 b16(const u8 *p) { return (u16)((u16)p[0] << 8 | p[1]); }
static const u8 *section(u8 k) { return bank + b16(bank + 4 + 2 * k); }
static const char *str(u16 at) { return at ? (const char *)bank + at : ""; }
static const u8 *job_rec(u8 k) { return section(0) + (u16)k * JOB_SIZE; }
static const u8 *item_rec(u8 id) { return section(1) + (u16)id * ITEM_SIZE; }
static const char *ability_name(u16 a) { return cj_ability_name(common, a); }
static const char *set_name(u8 s) { return cj_set_name(common, s); }
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
// item record fields
#define I_FLAGS 2
#define I_TYPE 3
#define I_HP 5
#define I_ICON 12
#define I_WP 13
#define I_PEV 14        // a weapon's W-EV, a shield's or an accessory's P-EV
#define I_MEV 15
#define J_CEV 31        // the job's own class evasion
static const u8 slot_flag[SLOTS] = { 0x20, 0x10, 0x08, 0x80, 0x40 };

static u16 job_ability(const u8 *j, u8 k) { return cj_set_ability(common, j[J_SET], k); }
static u16 ability_cost(u16 id)
{
    const u8 *p = cj_primary(common, id);
    return p ? cj_word(p) : 0;
}

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
    if(u->type==3){if(what)*what=255;return 0;}
    if (only && only != (u->type == 1 ? 2 : 1)) { if (what) *what = 0xFF; return 0; }
    for (r = 0; r < CAREERS; r++) {
        u8 need = (u8)(j[J_REQ + (r >> 1)] >> (r & 1 ? 0 : 4) & 15);
        if (need && job_level(u, r) < need) { if (what) *what = r; return 0; }
    }
    return 1;
}

// Source world_job_build_unit_list(mode2): reveal jobs unlocked by any party
// member, tint those unavailable to this unit, hide undiscovered jobs entirely.
static u8 job_visible(const SaveUnit *u, u8 c)
{
    u8 i,only=job_rec(c)[J_ONLY];
    if(u->type==3)return 0;
    if(only && only!=(u->type==1?2:1))return 0;
    if(c==career(u)||unlocked(u,c,RT_NULL))return 1;
    for(i=0;i<save.n;i++)if(unlocked(unit(i),c,RT_NULL))return 1;
    return 0;
}

static void open_jobs(void)
{
    u8 c;
    mg.screen=S_JOBS;mg.top=0;mg.row=career(unit(mg.unit));
    mg.job_n=mg.job_pos=0;mg.spin=0;
    for(c=0;c<CAREERS;c++)if(job_visible(unit(mg.unit),c)) {
        if(c==mg.row)mg.job_pos=mg.job_n;
        mg.jobs[mg.job_n++]=c;
    }
    for(c=0;c<mg.job_n;c++)mg.angle[c]=(u8)((u16)c*32/mg.job_n);
}

#include "../equipment_rules.h"
u8 can_equip(const SaveUnit *u, u8 slot, u8 id)
{
    u8 eq[5],k;for(k=0;k<5;k++)eq[k]=u->eq[k]?u->eq[k]:255;
    return core_equipment_allowed(common,u->job,u->type==1,b16(u->support),eq,slot,id?id:255);
}

#include "../movement_rules.h"
// A monster's growth and multipliers live in ffcj, not in the twenty career records.
static u8 mg_monster_job[JOB_SIZE];
static const u8 *mg_job_of(const SaveUnit *u)
{
    if (u->type != 3) return job_rec(record(u, career(u)));
    {
        const u8 *p = cj_section(common, CJ_JOBS) + 14U * u->job;
        memset(mg_monster_job, 0, sizeof mg_monster_job);
        memcpy(mg_monster_job + J_GROWTH, p, 10);
        mg_monster_job[J_MOVE] = p[10]; mg_monster_job[J_JUMP] = p[11];
    }
    return mg_monster_job;
}
static u8 mg_raw_current(const SaveUnit *u)
{
    u8 k;
    for (k = 0; k < save.n; k++) if (u == &save.unit[k]) return save_raw_current(&save, k);
    return 0;
}
static void mg_move_bonus(const SaveUnit *u, u8 *move, u8 *jump)
{
    u8 f[3];
    u16 id = b16(u->movement);
    cj_movement(common, u->job, (u16)(id - 486) < 24U ? (u8)(id - 485) : 0, f);
    core_movement_bonus(f[0], move, jump);
}
#define UST_JOB(u) mg_job_of(u)
#define UST_ITEM(id) item_rec(id)
#define UST_CURRENT(u) mg_raw_current(u)
#define UST_MOVE(u,m,j) mg_move_bonus(u,m,j)
#include "../unit_stats.inc"
#include "roster_edit.inc"
#define stats unit_stats

void equip(SaveUnit *u, u8 slot, u8 id)    // id 0: taken off; the inventory keeps the count
{
    if(u->type==3||slot>=SLOTS||id==u->eq[slot]||id&&(!save.inv[id]||!can_equip(u,slot,id)))return;
    if (u->eq[slot] && save.inv[u->eq[slot]] < 99) save.inv[u->eq[slot]]++;
    if (id) save.inv[id]--;
    u->eq[slot] = id;
    if(slot==EQ_WEAPON&&u->eq[EQ_SHIELD]&&!can_equip(u,EQ_SHIELD,u->eq[EQ_SHIELD]))equip(u,EQ_SHIELD,0);
    mg.changed = 1;
}

// Preserve the primary hand when removing Two Swords; then check all permissions.
static void validate_equipment(SaveUnit *u)
{
    u8 s;
    if(u->eq[EQ_SHIELD]&&!can_equip(u,EQ_SHIELD,u->eq[EQ_SHIELD]))equip(u,EQ_SHIELD,0);
    for(s=0;s<SLOTS;s++)if(u->eq[s]&&!can_equip(u,s,u->eq[s]))equip(u,s,0);
}

// k-th entry of the unit's current job: learned with its JP (1), else 0
u8 learn(SaveUnit *u, u8 c, u8 k)
{
    const u8 *j = job_rec(record(u, c));
    u16 id = job_ability(j, k), cost = ability_cost(id);
    if (u->type==3 || !id || learned(u, c, k) || jp(u, c) < cost) return 0;
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
    if(u->type==3||c>=CAREERS)return;
    u->job = (u8)(c == 0 && u->type == 2 ? 0x01 : 0x4A + c);
    // the battle's sprite (units.h) when it has one for the job
    u->gfx = u->type == 2 ? UG_RAMZA : c == 0 ? (u->type ? UG_SQUIRE_F : UG_SQUIRE_M)
           : c == 1 ? (u->type ? UG_CHEMIST_F : UG_CHEMIST_M) : 0xFF;
    validate_equipment(u);
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
            for (k = 0; k < 16; k++) if (learned(u, c, k) && job_ability(j, k)) break;
            if (k < 16) mg.pick[n++] = j[J_SET];
            continue;
        }
        for (k = 16; k < 22; k++) {
            u16 a = job_ability(j, k);
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

static u8 card_width(const char *s)
{
    const u8 *advance=menu_font()+103;u8 width=0;
    while(*s)width+=advance[(u8)*s++-32];
    return width;
}
#define CHOICE_TEXT(x,y,s,c,l) text(x,y,s,c,l)
#define CHOICE_WIDTH(s) card_width(s)
#include "../choice_menu.inc"

static void heading(const char *s) { draw_rect(0, 0, 160, 7, C_BLACK); text(5, 1, s, C_WHITE, 153); }
static void foot(const char *s) { draw_rect(0, 93, 160, 1, C_BLACK); black(2, 95, s); }

// still pictures of the bank: per career and sex (2 c + 1 for a woman), Ramza's at 40 (FFT draws
// him alike in every job)
static u8 picture(const SaveUnit *u, u8 c) { return u->type==3?255:u->type == 2 ? 40 : (u8)(2 * c + (u->type == 1)); }
static void actor(s16 x, s16 y, u8 k)
{
    const u8 *p; if(k==255)return; p = section(5) + (u16)k * 156;
#ifdef __m68k__
    RtSprite sp = { 16, 26, p, p + 52, p + 104 };
#else
    u16 r[3][26]; u8 i, j;
    RtSprite sp = { 16, 26, r[0], r[1], r[2] };
    for (i = 0; i < 3; i++) for (j = 0; j < 26; j++) r[i][j] = b16(p + i * 52 + 2 * j);
#endif
    draw_sprite(x, y, &sp);
}
static void actor_white(s16 x,s16 y,u8 k)
{
    const u8 *p=section(5)+(u16)k*156+104;
    u16 rows[2][26];u8 r;RtSprite sp={16,26,rows[0],rows[0],rows[1]};
    for(r=0;r<26;r++) {
        u16 ink=(u16)~b16(p+2*r),above=r?(u16)~b16(p+2*r-2):0;
        u16 below=r<25?(u16)~b16(p+2*r+2):0;
        rows[0][r]=(u16)(ink&~((ink<<1)&(ink>>1)&above&below));
        rows[1][r]=(u16)~ink;
    }
    draw_sprite(x,y,&sp);
}
static void monster_actor(s16 x,s16 y,const SaveUnit *u)
{
    const u8 *p=monster_formation_frame(monster_bank,u->job);
    if(!p)return;
#ifdef __m68k__
    RtSprite sp={32,26,p,p+104,p+208};
#else
    u32 rows[3][26];u8 a,b;RtSprite sp={32,26,rows[0],rows[1],rows[2]};
    for(a=0;a<3;a++)for(b=0;b<26;b++){
        const u8 *q=p+(u16)a*104+(u16)b*4;
        rows[a][b]=(u32)q[0]<<24|(u32)q[1]<<16|(u16)q[2]<<8|q[3];
    }
#endif
    draw_sprite(x-8,y,&sp);
}
#include "../portrait32.inc"
static void icon(s16 x, s16 y, u8 id)
{
    const u8 *p = section(4) + (u16)item_rec(id)[I_ICON] * 24;
    RtSprite sp = { 8, 8, p, p + 8, p + 16 };
    draw_sprite(x, y, &sp);
}
// ffid (tools/item_help.py): FFT's own description, read in place from the archive.
static const char *item_help(u8 id)
{
    u16 at;
    if (!item_text) return "";
    at = b16(item_text + 8 + 2 * (u16)id);
    return at ? (const char *)item_text + at : "";
}
// The same bank's second table: the source's own text for all 512 abilities (HELP.LZW).
static const char *ability_help(u16 a)
{
    u16 at;
    if (!item_text || a >= 512) return "";
    at = b16(item_text + 8 + 2 * (256 + a));
    return at ? (const char *)item_text + at : "";
}
#define ID_TEXT(x,y,s,c,l) text(x,y,s,c,l)
#define ID_NUM(x,y,n) number(x,y,n)
#define ID_ICON(x,y,id) icon(x,y,id)
#define ID_ADV(c) (menu_font()[103 + (u8)(c) - 32])
#include "../item_detail.inc"
static void hand(s16 x, s16 y)
{
    choice_hand_at(x,y);
}

static const char *unit_name(const SaveUnit *u)
{
    if(u->type==3){const char *name=monster_formation_name(monster_bank,u->name);return name?name:(const char *)(cj_section(common,CJ_JOB_NAMES)+14U*u->job);}
    if (u->type == 2) return "Ramza";
    return identity_name(identity,(u16)u->name+(u->type==1?255:0));
}
static const char *job_name(const SaveUnit *u, u8 c) { return u->type==3?(const char *)(cj_section(common,CJ_JOB_NAMES)+14U*u->job):str(b16(job_rec(record(u, c)))); }

static void floor_diamond(s16 x,s16 y)
{
    s16 r;
    for(r=-5;r<=5;r++) {
        s16 a=15-3*(r<0?-r:r),inner=a-6;
        draw_rect(x-a-1,y+r,2*a+3,1,C_WHITE);
        if(inner>0) {
            draw_rect(x-a,y+r,a-inner,1,C_BLACK);
            draw_rect(x+inner+1,y+r,a-inner,1,C_BLACK);
        } else draw_rect(x-a,y+r,2*a+1,1,C_BLACK);
    }
}

#include "egg_draw.inc"

#include "data/slot_icons.h"
// The unit sheet, decided with the user on 2026-11-22: the card and the tab bar are black
// plates with white text, like the world map's own; the body below them is white and fills
// every row, so no screen reads as an empty sheet. The card's rows are one pixel apart,
// which is what frees the three columns of statistics on its right.
enum { TAB_Y = 31, TAB_H = 8, BODY_Y = 39, LIST_Y = 41, LIST_H = 10 };   // the plate is US_PLATE_H tall, its last row under the bar

// The campaign's own evasion, which the source keeps out of its attribute record: the class
// evade is the job's own byte, the shield's and the accessory's their items', and the
// weapon's its own (ffmg, I_PEV).
// The class evade is the job's own byte; the shield's, the accessory's and the weapon's are
// their items'. Magic evasion is the M-EV the same records carry, which only shields and
// mantles ever hold (16 items on the disc, Aegis Shield 50, Feather Mantle 30).
static void evades(const SaveUnit *u, u8 *ce, u8 *se, u8 *ae, u8 *we, u8 *me)
{
    u8 shield = u->eq[EQ_SHIELD], acc = u->eq[EQ_ACC], weapon = u->eq[EQ_WEAPON], k, m = 0;
    *ce = u->type == 3 ? 0 : job_rec(record(u, career(u)))[J_CEV];
    *se = shield && (item_rec(shield)[I_FLAGS] & 0x40) ? item_rec(shield)[I_PEV] : 0;
    *ae = acc ? item_rec(acc)[I_PEV] : 0;
    *we = weapon ? item_rec(weapon)[I_PEV] : 0;
    for (k = 0; k < SLOTS; k++) if (u->eq[k]) m = (u8)(m + item_rec(u->eq[k])[I_MEV]);
    *me = m;
}
#define US_TEXT(x,y,t,c,l) text(x,y,t,c,l)
#define US_PORTRAIT(p,x,y) portrait32(p,x,y)
#define US_WIDTH(t) card_width(t)
#define US_PAGES
#include "../unit_sheet.inc"

// One snapshot of a saved unit, which is all the shared plate ever sees. The portrait is
// filled separately, because finding it is by far the dearest part (175k cycles on the
// 68000) and a preview of the same unit already has it.
static void sheet_values(UnitSheet *d, const SaveUnit *u)
{
    Stats s = stats(u);
    u8 c = career(u), ce, se, ae, we, me;
    evades(u, &ce, &se, &ae, &we, &me);
    d->portrait = RT_NULL;
    d->shown = US_ALL & ~(1UL << US_CT);     // a saved roster knows no charge time
    d->max_hp = d->max_mp = 0;               // and its units are never hurt between battles
    d->name = unit_name(u);
    d->job = job_name(u, c);
    d->v[US_LV] = u->level;    d->v[US_HP] = s.hp;      d->v[US_MP] = s.mp;
    d->v[US_PA] = s.pa;        d->v[US_MA] = s.ma;
    d->v[US_AT] = u->eq[EQ_WEAPON] ? item_rec(u->eq[EQ_WEAPON])[I_WP] : 0;
    d->v[US_BRAVE] = u->brave; d->v[US_FAITH] = u->faith;
    d->v[US_MOVE] = s.move;    d->v[US_JUMP] = s.jump;  d->v[US_SPEED] = s.sp;
    d->v[US_JLV] = job_level(u, c);
    d->v[US_CEV] = ce;         d->v[US_SEV] = se;       d->v[US_AEV] = ae;
    d->v[US_WEV] = we;         d->v[US_MEV] = me;
}
static void sheet_fill(UnitSheet *d, const SaveUnit *u)
{
    sheet_values(d, u);
    if (u->type != 3)
        d->portrait = identity_portrait(identity, section(6)[picture(u, career(u))]);
}
// FFT previews a fitting before it is confirmed: while the cursor walks the candidates it
// shows what the unit WOULD have. The snapshot of that unit, or 0 when nothing is pending.
static const UnitSheet *sheet_preview(UnitSheet *t)
{
    SaveUnit c;
    if (mg.screen != S_PICK || mg.back != S_EQUIP || !mg.pick_n) return RT_NULL;
    c = *unit(mg.unit);
    c.eq[mg.slot] = (u8)mg.pick[mg.top + mg.row];
    if (mg.slot == EQ_WEAPON && c.eq[EQ_SHIELD] && !can_equip(&c, EQ_SHIELD, c.eq[EQ_SHIELD]))
        c.eq[EQ_SHIELD] = 0;
    sheet_values(t, &c);         // the preview is the same unit: it needs no second portrait
    return t;
}
// The plate, wherever it goes: the top of the unit sheet, the foot of the roster, the head
// of the deployment screen (deploy.c). Any saved unit, not only the one the menus are on.
void plate_unit(const SaveUnit *u, const UnitSheet *soon, s16 top)
{
    UnitSheet d;
    sheet_fill(&d, u);
    unit_sheet_plate(&d, soon, top);
    if (u->type == 3) monster_actor(16, top, u);
}
static void plate_at(s16 top)
{
    UnitSheet t;
    plate_unit(unit(mg.unit), sheet_preview(&t), top);
}
static void plate(void) { plate_at(0); }
// Status, Eqp., Abil., Learn and Job: the tab replaces a whole menu level. Left and right
// change tab, F1/F5 change unit.
enum { TAB_STATUS, TAB_EQUIP, TAB_ABIL, TAB_LEARN, TAB_JOB, TAB_N };
static const char *const tab_name[TAB_N] = { "Status", "Eqp.", "Abil.", "Learn", "Job" };
static u8 screen_tab(u8 screen)
{
    switch (screen) {
    case S_EQUIP: case S_INV: case S_ITEM: return TAB_EQUIP;
    case S_SET: return TAB_ABIL;
    case S_PICK: return mg.back == S_EQUIP ? TAB_EQUIP : TAB_ABIL;
    case S_LEARN: case S_JOBTABLE: return TAB_LEARN;
    case S_JOBS: return TAB_JOB;
    default: return TAB_STATUS;
    }
}
static void tab_bar(u8 active)
{ unit_sheet_tabs(TAB_Y, tab_name, TAB_N, active, save.n > 1 ? "F1 F5" : RT_NULL); }
static void body(void) { draw_rect(0, BODY_Y, 160, (s16)(100 - BODY_Y), C_WHITE); }
// Learn, the job table and the inventory carry a floating plate on their first row, so
// their list starts one row lower and shows one row less.
static u8 has_plate(void)
{ return mg.screen == S_LEARN || mg.screen == S_JOBTABLE || mg.screen == S_INV; }
static s16 list_top(void) { return (s16)(LIST_Y + (has_plate() ? 11 : 0)); }
static u8 list_shown(void) { return (u8)(has_plate() ? ROWS - 1 : ROWS); }

// The tabs carry the unit's own pages, so the formation menu keeps only what acts on the
// roster itself.
static const char *const menu_rows[] = { "Remove Unit", "Order Unit" };
static const char *const item_rows[] = { "Equip", "Best", "Remove all", "Inventory" };
static const char *const order_rows[] = { "Job", "Number", "Level", "MAX MP", "MAX HP", "Faith", "Brave" };
static const char *const slot_rows[] = { "Head", "Body", "Accessory", "R.Hand", "L.Hand" };
static const char *const set_rows[] = { "Secondary", "Reaction", "Support", "Movement" };
static const u8 slot_order[SLOTS] = { EQ_WEAPON, EQ_SHIELD, EQ_HEAD, EQ_BODY, EQ_ACC };   // shown

static void formation(void)
{
    u8 i, first = (u8)(mg.top * 5);
    const SaveUnit *u = unit(mg.unit);
    Stats s;if(!save_egg(&save,mg.unit))s=stats(u);
    for (i = 0; i < 10 && first + i < save.n; i++) {
        s16 x = 8 + (i % 5) * 32, y = 7 + (i / 5) * 27;
        formation_actor(x,y,(u8)(first+i));
    }
    if(save.n>10){black(154,8,mg.top?"^":"");black(154,54,first+10<save.n?"v":"");}
    if(save_egg(&save,mg.unit)){draw_rect(0,62,160,1,C_BLACK);egg_card(mg.unit);return;}
    (void)s;
    plate_at(62);                        // the sheet's own plate, under the roster
    if(mg.screen==S_FORM)black(37,95,"ENTER:unit  ALPHA:menu");
}

static void formation_cursor(void)
{
    u8 i,first=(u8)(mg.top*5);s16 x=16+(mg.unit%5)*32,y=29+(mg.unit/5-mg.top)*27;
    if(mg.slide) {
        x+=((mg.floor_from_x-x)*mg.slide)>>2;y+=((mg.floor_from_y-y)*mg.slide)>>2;
    }
    floor_diamond(x,y);
    // The diamond goes underneath feet; reusing the cheap actor sprites avoids
    // rebuilding stats/text/the entire cached page on each interpolation step.
    for(i=0;i<10&&first+i<save.n;i++) {
        formation_actor(8+(i%5)*32,7+(i/5)*27,(u8)(first+i));
    }
}

static void jobs_page(void)
{
    static const s16 sine[32]={0,50,98,142,181,213,237,251,256,251,237,213,181,142,98,50,
        0,-50,-98,-142,-181,-213,-237,-251,-256,-251,-237,-213,-181,-142,-98,-50};
    s16 x[CAREERS],y[CAREERS];u8 ord[CAREERS],i,j;
    const SaveUnit *u=unit(mg.unit);
    black(3,10,unit_name(u));
    for(i=0;i<mg.job_n;i++) {
        u8 a=(u8)((8+mg.angle[i]-mg.angle[mg.job_pos]+mg.spin)&31);
        x[i]=80+(sine[(a+8)&31]*62>>8);y[i]=49+(sine[a]*15>>8);
        for(j=i;j&&y[ord[j-1]]>y[i];j--)ord[j]=ord[j-1];
        ord[j]=i;
    }
    for(j=0;j<mg.job_n;j++) {
        u8 c,available;
        i=ord[j];c=mg.jobs[i];available=unlocked(u,c,RT_NULL);
        if(available)actor(x[i]-8,y[i]-22,(u8)(2*c+(u->type==1)));
        else actor_white(x[i]-8,y[i]-22,(u8)(2*c+(u->type==1)));
    }
    if(mg.job_anim) {
        u8 age=JOB_CHANGE_FRAMES-mg.job_anim,frame=age<JOB_CHANGE_FRAMES/2?mg.old_picture:picture(u,career(u));
        s16 top=8,bottom=43;
        if(age<8)bottom=8+(((s16)age+1)*35>>3);
        if(age>=JOB_CHANGE_FRAMES-8)top+=((s16)(age-(JOB_CHANGE_FRAMES-8))*35)>>3;
        // Source effect is a vertical translucent column, not an actor jump.
        // Bounded strips avoid an effect sprite bank or per-pixel blending.
        draw_rect(70,top,21,bottom-top,C_LGRAY);
        draw_rect(70,top,1,bottom-top,C_DGRAY);draw_rect(90,top,1,bottom-top,C_DGRAY);
        draw_rect(74+(age&3),top,1,bottom-top,C_WHITE);
        draw_rect(82+((age+2)&3),top,2,bottom-top,C_WHITE);
        draw_rect(78,top,3,bottom-top,C_WHITE);
        if(age>=44&&age<52)actor_white(72,16,frame);else actor(72,16,frame);
        if(age>=8&&age<JOB_CHANGE_FRAMES-8)draw_rect(72,42,17,1,C_DGRAY);
    }else actor(72,16,picture(u,career(u)));
    draw_rect(0,72,160,1,C_BLACK);black(65,76,job_name(u,mg.row));
    black(15,85,"Move");number(36,85,job_rec(mg.row)[J_MOVE]);
    black(52,85,"Jump");number(73,85,job_rec(mg.row)[J_JUMP]);
    black(110,85,"JP");number(125,85,jp(u,mg.row));
    black(15,94,unlocked(u,mg.row,RT_NULL)?"< > jobs ENTER:change":"Known by another unit");
}

static const char *ability_or_set(u8 which, u16 a) { return !a ? "-" : which ? ability_name(a) : set_name((u8)a); }
static u16 slot_value(const SaveUnit *u, u8 which)
{
    return which ? b16(slot_of((SaveUnit *)u, which)) : u->secondary;
}

// The unit's primary skill set: its job's own, which no menu can change, so the source
// prints it above the four it can (orig/mgt_ability.png, greyed out).
static const char *primary_set(const SaveUnit *u)
{
    if (u->type == 3) return "-";
    return set_name(job_rec(record(u, career(u)))[J_SET]);
}
// Status: what the plate cannot carry -- the unit's birth sign, sex, the JP it has earned
// in this job and the four ability sets it goes to battle with.
static void status_page(void)
{
    static const char *const signs[13] = { "Aries", "Taurus", "Gemini", "Cancer", "Leo", "Virgo",
        "Libra", "Scorpio", "Sagittarius", "Capricorn", "Aquarius", "Pisces", "Serpentarius" };
    const SaveUnit *u = unit(mg.unit);
    u8 c = career(u), i;
    text(4, 42, "Zodiac", C_DGRAY, 40);
    black(44, 42, u->zodiac < 13 ? signs[u->zodiac] : "-");
    text(96, 42, "Total JP", C_DGRAY, 134);
    right(156, 42, u->type == 3 ? 0 : tjp(u, c));
    draw_rect(2, 50, 156, 1, C_LGRAY);
    text(4, 53, "Primary", C_DGRAY, 56);
    black(60, 53, primary_set(u));
    for (i = 0; i < 4; i++) {
        s16 y = (s16)(62 + i * 9);
        text(4, y, set_rows[i], C_DGRAY, 56);
        black(60, y, ability_or_set(i, slot_value(u, i)));
    }
    text(112, 90, "MODE:help", C_DGRAY, 158);
}

// Equipment and abilities: both pages are the shared sheet's own (../unit_sheet.inc), so
// they read exactly as they do on the battle's Status screen.
static void equip_page(void)
{
    const SaveUnit *u = unit(mg.unit);
    UnitEquip e;
    u8 i;
    for (i = 0; i < SLOTS; i++) {
        u8 id = u->eq[slot_order[i]];
        e.icon[i] = id ? section(4) + (u16)item_rec(id)[I_ICON] * 24 : RT_NULL;
        e.name[i] = id ? str(b16(item_rec(id))) : RT_NULL;
    }
    unit_sheet_equip(&e);
}

static void set_page(void)
{
    const SaveUnit *u = unit(mg.unit);
    UnitSets d;
    u8 i;
    d.primary = primary_set(u);
    for (i = 0; i < 4; i++) d.set[i] = ability_or_set(i, slot_value(u, i));
    unit_sheet_sets(&d);
}

// Learn: the current job's abilities (its list's entries), cost or learned
static u8 learn_entry(const SaveUnit *u, u8 row)   // the row-th entry of the list (22: none)
{
    const u8 *j = job_rec(record(u, mg.ljob));
    u8 monster_job[JOB_SIZE];
    if(u->type==3){const u8 *p=cj_section(common,CJ_JOBS)+14U*u->job;memset(monster_job,0,sizeof monster_job);memcpy(monster_job+J_GROWTH,p,10);monster_job[J_MOVE]=p[10];monster_job[J_JUMP]=p[11];j=monster_job;}
    u8 k;
    for (k = 0; k < 22; k++) if (job_ability(j, k) && !row--) return k;
    return 22;
}
static u8 learn_count(const SaveUnit *u) { u8 n = 0; while (learn_entry(u, n) < 22) n++; return n; }

static void list_rows(u8 n, void (*one)(u8 k, s16 y))
{
    s16 top = list_top();
    u8 shown = list_shown(), i;
    for (i = mg.top; i < n && i < mg.top + shown; i++)
        one(i, (s16)(top + (i - mg.top) * LIST_H));
    // CHOICE_MENU.md rule 6: a solid three-pixel triangle where the list goes on.
    if (mg.top) for (i = 0; i < 3; i++)
        draw_rect((s16)(153 + i), (s16)(top + 3 - i), (s16)(1 + 2 * i), 1, C_BLACK);
    if (mg.top + shown < n) for (i = 0; i < 3; i++)
        draw_rect((s16)(153 + i), (s16)(top + (shown - 1) * LIST_H + 2 + i), (s16)(5 - 2 * i), 1, C_BLACK);
}

static void learn_row(u8 i, s16 y)
{
    const SaveUnit *u = unit(mg.unit);
    const u8 *j = job_rec(record(u, mg.ljob));
    y = (s16)(y + 2);
    u8 monster_job[JOB_SIZE];
    if(u->type==3){const u8 *p=cj_section(common,CJ_JOBS)+14U*u->job;memset(monster_job,0,sizeof monster_job);memcpy(monster_job+J_GROWTH,p,10);monster_job[J_MOVE]=p[10];monster_job[J_JUMP]=p[11];j=monster_job;}
    u8 k = learn_entry(u, i);
    black(26, y, ability_name(job_ability(j, k)));
    if (learned(u, mg.ljob, k)) text(118, y, "Learned", C_DGRAY, 151);
    else right(150, y, ability_cost(job_ability(j, k)));
}

static void pick_row(u8 i, s16 y)
{
    u16 a = mg.pick[i];
    y = (s16)(y + 2);
    if (mg.back == S_EQUIP) {
        if (!a) { black(26, y, "Remove"); return; }
        icon(24, (s16)(y - 2), (u8)a); black(36, y, str(b16(item_rec((u8)a))));
        black(132, y, "x"); number(136, y, save.inv[a]);
    } else black(26, y, a ? ability_or_set(mg.slot, a) : "Remove");
}

// ---------------------------------------------- FFT's Remove Unit, Order Unit and Inventory
// Their text is the source's own, read in place from ffmg's world section
// (WORLD/WORLD.BIN through tools/management_data.py).
enum { W_REMOVE, W_MENU, W_BEST, W_ASSIGNED, W_RAMZA, W_BYE, W_BYE_N = 8 };
static const char *world_str(u8 k)
{
    const u8 *p = section(8);
    return !p || k >= p[0] ? "" : str(b16(p + 2 + 2 * k));
}
static const char *world_line2(const char *s)   // the entry's second line
{
    while (*s && *s != '\n') s++;
    return *s ? s + 1 : "";
}
static s16 world_line1(s16 x, s16 y, const char *s)
{
    char b[40];
    u8 k = 0;
    while (s[k] && s[k] != '\n' && k < 39) { b[k] = s[k]; k++; }
    b[k] = 0;
    return black(x, y, b);
}
// Which farewell the source picks is not measured; this spreads its eight over the party
// from the unit's own numbers, so one unit always answers the same way.
static const char *farewell(const SaveUnit *u)
{
    if (u->type == 2) return world_str(W_RAMZA);
    return world_str((u8)(W_BYE + (u8)(u->brave + u->faith + u->level) % W_BYE_N));
}

// The equipment score: the campaign's own stat derivation plus the weapon's power,
// the same measure the shop's Best fit ranks with.
static u16 mg_fit(const SaveUnit *u)
{
    Stats s = stats(u);
    u16 v = (u16)(s.hp + s.mp + 10U * (s.pa + s.ma + s.sp));
    u8 k;
    for (k = 0; k < SLOTS; k++) if (u->eq[k]) v = (u16)(v + 8U * item_rec(u->eq[k])[I_WP]);
    return v;
}
// "Equip to the maximum": the best owned item per slot, weapon first so the hand it frees
// (or the two hands it takes) is settled before the shield is chosen. Two passes.
static void best_fit_unit(SaveUnit *u)
{
    u8 pass, k, id, moved = 1;
    if (u->type == 3) return;
    for (pass = 0; pass < 2 && moved; pass++) for (moved = 0, k = 0; k < SLOTS; k++) {
        u8 slot = slot_order[k], best = u->eq[slot], old = u->eq[slot];
        u16 score = mg_fit(u), s2;
        for (id = 1; id; id++) {
            if (!save.inv[id] || !can_equip(u, slot, id)) continue;
            u->eq[slot] = id; s2 = mg_fit(u); u->eq[slot] = old;
            if (s2 > score) { score = s2; best = id; }
        }
        if (best != old) { equip(u, slot, best); moved = 1; }
    }
}
static void remove_all_equipment(SaveUnit *u)
{
    u8 k;
    for (k = 0; k < SLOTS; k++) equip(u, k, 0);
}

static u8 fits_any_slot(const SaveUnit *u, u8 id)
{
    u8 s;
    for (s = 0; s < SLOTS; s++) if (can_equip(u, s, id)) return 1;
    return 0;
}
// Eqp.: what this unit could wear; All: the whole bag, as the source's own indicator switches.
static void build_inventory(const SaveUnit *u)
{
    u16 id;
    u8 n = 0;
    for (id = 1; id < 256; id++)
        if (save.inv[id] && (mg.inv_all || fits_any_slot(u, (u8)id)) && n < 128) mg.inv[n++] = (u8)id;
    mg.inv_n = n;
    if (mg.top + mg.row >= n) { mg.top = 0; mg.row = 0; }
}
static void inventory_row(u8 i, s16 y)
{
    u8 id = mg.inv[i];
    y = (s16)(y + 2);
    icon(24, (s16)(y - 2), id); black(36, y, str(b16(item_rec(id))));
    black(132, y, "x"); number(136, y, save.inv[id]);
}
// The source's own Eqp./All indicator, on a floating plate over the list.
static void float_plate(const char *label, u16 value, u8 number)
{
    draw_rect(104, (s16)(BODY_Y + 1), 54, 10, C_BLACK);
    text(108, (s16)(BODY_Y + 3), label, C_WHITE, 150);
    if (number) us_number(154, (s16)(BODY_Y + 3), value, 0, C_WHITE);
}
// The source heads its bag with two options, "Eqp." and "ALL", the active one lit
// (orig/mgt_item_list.png): Eqp. keeps what this unit could wear, All shows the whole bag.
static void inventory_filter(void)
{
    static const char *const choice[2] = { "Eqp.", "All" };
    s16 w0 = (s16)(card_width(choice[0]) + 6), w1 = (s16)(card_width(choice[1]) + 6);
    s16 x = (s16)(158 - w0 - w1), y = (s16)(BODY_Y + 1);
    u8 i;
    for (i = 0; i < 2; i++) {
        s16 w = i ? w1 : w0;
        if (i == mg.inv_all) draw_rect(x, y, w, 10, C_BLACK);
        text((s16)(x + 3), (s16)(y + 2), choice[i], i == mg.inv_all ? C_WHITE : C_DGRAY, 157);
        x = (s16)(x + w);
    }
}
static void inventory_page(void)
{
    if (!mg.inv_n) text(26, 56, "Nothing in the bag", C_DGRAY, 158);
    else list_rows(mg.inv_n, inventory_row);
    inventory_filter();
}

// Learn: the source opens on the unit's jobs, one row each, before their abilities.
static u8 jobtable_entry(const SaveUnit *u, u8 row)
{
    u8 c;
    for (c = 0; c < CAREERS; c++) if (job_visible(u, c) && !row--) return c;
    return CAREERS;
}
static u8 jobtable_count(const SaveUnit *u)
{
    u8 n = 0;
    while (jobtable_entry(u, n) < CAREERS) n++;
    return n;
}
static u8 job_mastered(const SaveUnit *u, u8 c)
{
    const u8 *j = job_rec(record(u, c));
    u8 k;
    for (k = 0; k < 22; k++) if (job_ability(j, k) && !learned(u, c, k)) return 0;
    return 1;
}
static void jobtable_row(u8 i, s16 y)
{
    const SaveUnit *u = unit(mg.unit);
    u8 c = jobtable_entry(u, i);
    static const u16 need[7] = { 200, 350, 550, 800, 1150, 1550, 2100 };
    u8 lv = job_level(u, c);
    y = (s16)(y + 2);
    black(26, y, job_name(u, c));
    right(100, y, lv);
    right(124, y, tjp(u, c));
    if (job_mastered(u, c)) text(126, y, "Master", C_DGRAY, 151);
    else right(150, y, lv < 8 ? (u16)(need[lv - 1] - tjp(u, c)) : jp(u, c));
}
static void jobtable_page(void)
{
    const SaveUnit *u = unit(mg.unit);
    u8 c = jobtable_entry(u, (u8)(mg.top + mg.row));
    list_rows(jobtable_count(u), jobtable_row);
    float_plate("JP", c < CAREERS ? jp(u, c) : 0, 1);
}

// A command overlay: fitted to its labels, no air under the last row, its glove outside
// the frame on the left (CHOICE_MENU.md).
static void overlay_at(const char *const *rows, u8 n, const char *title, u8 at,
                       s16 x, s16 y, u8 grey)       // grey: a bitmask of unselectable rows
{
    ChoiceLayout menu = { 0, 0, 0, 0, 7, CHOICE_TAB, 3, 1, 0, 8, 12, 9, 0, CHOICE_HAND_X(8), CHOICE_HAND_Y(12) };
    u8 i, w = card_width(title);
    for (i = 0; i < n; i++) { u8 v = card_width(rows[i]); if (v > w) w = v; }
    menu.width = (u8)(menu.text_x + w + 6);
    menu.height = (u8)(13 + 9 * n);
    menu.x = x < 0 ? (s16)(158 - menu.width) : x;
    menu.y = y < 0 ? (s16)(98 - menu.height) : y;
    menu.limit = (u8)(menu.x + menu.width - 3);
    choice_frame(&menu, title);
    for (i = 0; i < n; i++)
        choice_row(&menu, i, rows[i], (grey >> i) & 1 ? C_DGRAY : C_BLACK);
    choice_cursor_animated(&menu, at, rt_frame);
}
static void overlay(const char *const *rows, u8 n, const char *title, u8 at)
{ overlay_at(rows, n, title, at, -1, -1, 0); }
static void item_menu(void) { overlay(item_rows, 4, "Item", mg.row); }
static void order_menu(void) { overlay(order_rows, ORD_N, "Order", mg.row); }

// The abbreviations the plate's three columns use, as a page of its own: the source has no
// such screen, but its plate spells every word out and ours cannot.
static void legend_panel(void)
{
    // Laid out as the plate is, column for column, so the legend is read the same way.
    static const char *const word[3][5] = {
        { "PA Phys.atk", "MA Magic atk", "AT Atk power", "Br Brave", "Fa Faith" },
        { "Mv Move", "Jp Jump", "Sp Speed", "JLv Job lv", "" },
        { "CE Class ev", "SE Shield ev", "AE Access ev", "WE Weapon ev", "ME Magic ev" } };
    static const u8 col[3] = { 4, 56, 108 };
    u8 i, k;
    draw_rect(0, BODY_Y, 160, (s16)(100 - BODY_Y), C_WHITE);
    draw_rect(0, BODY_Y, 160, 1, C_BLACK);
    for (k = 0; k < 3; k++) for (i = 0; i < 5; i++)
        text((s16)col[k], (s16)(BODY_Y + 5 + i * 10), word[k][i], C_BLACK, 158);
}

// Remove Unit's two beats: the source's Check, then the unit's own answer with Remove/Quit.
static void remove_panel(void)
{
    const SaveUnit *u = unit(mg.unit);
    static const ChoiceLayout ask = { 40, 38, 60, 29, 3, CHOICE_TAB, 3, 1, 0, 24, 10, 9, 155, CHOICE_HAND_X(24), CHOICE_HAND_Y(10) };
    static const ChoiceLayout reply = { 0, 0, 160, 100, 0, CHOICE_BARE, 0, 0, 0, 52, 72, 9, 150, CHOICE_HAND_X(52), CHOICE_HAND_Y(72) };
    if (!mg.remove) return;
    if (mg.remove == 1) {
        char title[26];
        const char *word = world_str(W_REMOVE), *name = unit_name(u);
        u8 k = 0;
        while (*word && k < 20) title[k++] = *word++;
        title[k++] = ' ';
        while (*name && k < 24) title[k++] = *name++;
        title[k++] = '?'; title[k] = 0;
        choice_frame(&ask, title);
        choice_row(&ask, 0, "Yes", C_BLACK);
        choice_row(&ask, 1, "No", C_BLACK);
        choice_cursor_animated(&ask, mg.remove_row, rt_frame);
        return;
    }
    panel(4, 40, 152, 52);
    { const u8 *p = identity_portrait(identity, section(6)[picture(u, career(u))]);
      if (u->type != 3 && p) portrait32(p, 8, 44); }
    black(44, 44, unit_name(u));
    world_line1(44, 54, farewell(u));
    black(44, 62, world_line2(farewell(u)));
    world_line1(52, 72, world_str(W_MENU));
    black(52, 81, world_line2(world_str(W_MENU)));
    choice_cursor_animated(&reply, mg.remove_row, rt_frame);
}

static void note_panel(void)
{
    static const char *const notes[] = { "", "Not enough JP", "", "Save failed", "" };
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
    } else if (mg.note == NOTE_RAMZA) {
        world_line1(12, 64, world_str(W_RAMZA));
        black(12, 72, world_line2(world_str(W_RAMZA)));
        return;
    } else black(12, 64, notes[mg.note]);
    black(12, 74, "ENTER or SHIFT");
}

static void confirm_panel(void)
{
    const SaveUnit *u = unit(mg.unit);
    static const ChoiceLayout menu={65,53,94,27,3,CHOICE_TAB,3,1,0,6,10,8,157,CHOICE_HAND_X(6),CHOICE_HAND_Y(10)};
    if (!mg.confirm) return;
    if(save_egg(&save,mg.unit)){egg_menu(mg.confirm);return;}
    choice_frame(&menu,mg.screen==S_JOBS?job_name(u,mg.row):"Learn ability?");
    choice_row(&menu,0,mg.screen==S_JOBS?"Change Job":"Learn",C_BLACK);
    choice_row(&menu,1,"Return",C_BLACK);
    choice_cursor_animated(&menu,mg.confirm-1,rt_frame);
}

static void command_menu(void)
{
    if(save_egg(&save,mg.unit)){if(!mg.confirm)egg_menu(0);return;}
    overlay_at(menu_rows, MENU_ROWS, "Menu", mg.row, 56, 15,
               unit(mg.unit)->type == 2 ? 1 : 0);
}

static void list_cursor(void)
{
    ChoiceLayout cursor={0,0,160,100,0,CHOICE_BARE,0,0,0,0,0,LIST_H,159,0,0};
    cursor.hand_y=(s8)(list_top()-1);
    if(mg.note||mg.confirm||mg.remove)return;
    switch(mg.screen) {
    case S_EQUIP: cursor.pitch=US_EQ_PITCH; cursor.hand_x=US_EQ_HAND_X;   // the shared page's own
                  cursor.hand_y=US_EQ_HAND_Y(0); break;
    case S_SET: cursor.pitch=US_SET_PITCH; cursor.hand_x=US_SET_HAND_X;
                cursor.hand_y=US_SET_HAND_Y(0); break;
    case S_STATUS: return;
    case S_INV: if(!mg.inv_n)return; break;
    case S_LEARN: case S_PICK: case S_JOBTABLE: break;
    default: return;
    }
    choice_cursor_animated(&cursor,mg.row,rt_frame);
}

// The item the cursor is on, 0 when it is not on one.
static u8 detail_item(void)
{
    if (mg.screen == S_EQUIP) return unit(mg.unit)->eq[slot_order[mg.row]];
    if (mg.screen == S_PICK && mg.back == S_EQUIP) return (u8)mg.pick[mg.top + mg.row];
    if (mg.screen == S_INV && mg.inv_n) return mg.inv[mg.top + mg.row];
    return 0;
}
// The ability under the cursor, 0 when there is none: Learn's own list and the four
// ability slots' choices both point at one, and both open its description with [MODE].
static u16 detail_ability(void)
{
    const SaveUnit *u = unit(mg.unit);
    if (mg.screen == S_LEARN) {
        u8 k = learn_entry(u, (u8)(mg.top + mg.row));
        return k < 22 ? job_ability(job_rec(record(u, mg.ljob)), k) : 0;
    }
    if (mg.screen == S_PICK && mg.back == S_SET && mg.slot) return mg.pick[mg.top + mg.row];
    return 0;
}
// The ability's own panel: the same frame the item detail uses, so the two read as one
// screen, with FFT's text for all 489 described abilities (ffid's second table).
static void ability_panel(void)
{
    static const char *const kind_name[4] = { "Action", "Reaction", "Support", "Movement" };
    u16 a = detail_ability();
    if (!a) { mg.detail = 0; return; }
    draw_rect(5, 13, 150, 74, C_BLACK);
    draw_rect(6, 14, 148, 72, C_WHITE);
    text(10, 17, ability_name(a), C_BLACK, 152);
    text(10, 26, kind_name[kind(a)], C_DGRAY, 100);
    text(104, 26, "JP", C_DGRAY, 152);
    right(146, 26, ability_cost(a));
    item_detail_wrap(ability_help(a), 10, 38, 140, 4);
    text(10, 78, "MODE or SHIFT closes", C_DGRAY, 152);
}
static void detail_panel(void)
{
    u8 id = detail_item();
    const u8 *r;
    ItemDetail d;
    u8 i;
    if (!id) { mg.detail = 0; return; }
    r = item_rec(id);
    d.id = id; d.kind = r[I_TYPE]; d.flags = r[I_FLAGS]; d.name = str(b16(r));
    d.help = item_help(id); d.power = r[I_WP]; d.evade = r[I_PEV];
    for (i = 0; i < 7; i++) d.stat[i] = (s8)r[I_HP + i];
    item_detail_draw(&d);
}

static void page(void)
{
    draw_clear();
    if (!bank) { draw_text(4, 40, "Missing ffmg", 0, C_BLACK); return; }
    if (!save.n) { heading("Formation"); black(8, 40, "No campaign save:"); black(8, 48, "win the battle at Gariland."); foot("ESC:quit"); return; }
    if (mg.screen == S_FORM || mg.screen == S_MENU || mg.screen == S_ORDER) {
        heading("Formation");
        formation();
        if (mg.screen == S_ORDER) { note_panel(); return; }
        if (mg.screen == S_MENU) {
            formation_cursor();
            if(!save_egg(&save,mg.unit))foot("ENTER:open SHIFT:back");
        }
        note_panel();
        return;
    }
    if (mg.screen == S_JOBS) { heading("Change Job"); jobs_page(); note_panel(); return; }
    // Every other screen is a tab of the unit's own sheet: the plate, the bar, the body.
    plate();
    tab_bar(screen_tab(mg.screen));
    body();
    switch (mg.screen) {
    case S_STATUS: status_page(); break;
    case S_EQUIP: case S_ITEM: equip_page(); break;
    case S_SET: set_page(); break;
    case S_LEARN:
        list_rows(learn_count(unit(mg.unit)), learn_row);
        float_plate("JP", jp(unit(mg.unit), mg.ljob), 1); break;
    case S_PICK: list_rows(mg.pick_n, pick_row); break;
    case S_INV: inventory_page(); break;
    case S_JOBTABLE: jobtable_page(); break;
    }
    note_panel();
    if (mg.detail) {
        if (mg.screen == S_STATUS) legend_panel();
        else if (detail_ability()) ability_panel();
        else detail_panel();
    }
}

// ---------------------------------------------------------------- the frame loop
static void open_list(u8 screen, u8 at)          // a scrolling list, its entry at shown
{
    u8 shown;
    mg.screen = screen;
    shown = list_shown();
    mg.top = at >= shown ? (u8)(at - shown + 1) : 0;
    mg.row = (u8)(at - mg.top);
}

static void roster_position(u8 screen)
{
    u8 row=mg.unit/5;
    mg.screen=screen;mg.top=row?row-1:0;
}

void game_init(void)
{
    u16 size;
#ifndef __m68k__
    if(cache)free(cache);
#endif
    bank = RT_NULL; cache = RT_NULL; item_text = RT_NULL;   // (statics survive between runs)
    memset(&mg, 0, sizeof mg);props_on=0;props_result=1;
    bank = rt_file("ffmg", &size);
    if (bank && memcmp(bank, "FMG5", 4)) bank = RT_NULL;
    identity=rt_file("ffcp",&size);if(!identity_valid(identity,size))bank=RT_NULL;
    common = rt_file("ffcj", &size);
    if (!cj_valid(common, size)) bank = RT_NULL;
    save_load(&save);
    egg_bank=rt_file("ffeg",&size);if(!egg_art_valid(egg_bank,size))bank=RT_NULL;
    item_text=rt_file("ffid",&size);
    if(item_text&&(memcmp(item_text,"FID2",4)||b16(item_text+4)!=256||b16(item_text+6)!=512))
        item_text=RT_NULL;
    monster_bank=rt_file("ffma",&size);if(!monster_formation_valid(monster_bank,size))monster_bank=RT_NULL;
    if (save.n > PARTY_MAX) save.n = 0;
    cache = (u8 *)malloc(2 * RT_PSIZE);
    dirty = 1;
    rt_state = &mg; rt_state_size = sizeof mg;
}

void game_scenario(u16 n)                // test doors: a screen of the first unit
{
    static const u8 doors[] = { S_FORM, S_STATUS, S_EQUIP, S_SET, S_LEARN, S_JOBS, S_MENU,
                                S_ITEM, S_INV, S_ORDER, S_JOBTABLE, S_FORM, S_FORM };
    if(save.missions.report.ready&&n<100)n=100;
    if(n>=100&&n<=101){props_start(n);return;}
    mg.screen = n < sizeof doors ? doors[n] : S_FORM;
    mg.unit = mg.row = mg.top = 0;
    if(save_egg(&save,0))mg.screen=S_FORM;
    else if(unit(0)->type==3&&mg.screen!=S_FORM&&mg.screen!=S_MENU)mg.screen=S_STATUS;
    if (mg.screen == S_JOBS) open_jobs();
    if (mg.screen == S_LEARN || mg.screen == S_JOBTABLE) mg.ljob = career(unit(mg.unit));
    if (mg.screen == S_INV) build_inventory(unit(mg.unit));
    if (n == 11 || n == 12) {             // Remove Unit's two beats, on a unit it lets go
        while (mg.unit < save.n && unit(mg.unit)->type == 2) mg.unit++;
        if (mg.unit < save.n) {
            mg.screen = S_MENU; mg.row = 4; mg.remove = (u8)(n - 10); mg.remove_row = 0;
        } else mg.unit = 0;
    }
    dirty = 1;
}

#include "../../../lib/tilog.h"
static u8 leave(void)
{
    tilog("manage leave%s", mg.changed ? " (session saved)" : "");
    if (mg.changed) {
#ifdef __m68k__
        u8 ok=rt_save(SAVE_NAME,&save,sizeof save);
#else
        u8 ok=story_choice_store_file(&save);
#endif
        if(!ok){mg.note=NOTE_SAVE;dirty=1;return 1;}
    }
    if (cache) { free(cache); cache = RT_NULL; }
    return 0;
}

static u8 list_move(u32 key, u8 n)              // up / down in a scrolling list
{
    u8 at = (u8)(mg.top + mg.row);
    if (!n) return 0;
    if (key & K_UP) at = at ? (u8)(at - 1) : (u8)(n - 1);
    if (key & K_DOWN) at = (u8)(at + 1 == n ? 0 : at + 1);
    if (at < mg.top) mg.top = at;
    if (at >= mg.top + list_shown()) mg.top = (u8)(at - list_shown() + 1);
    mg.row = (u8)(at - mg.top);
    return at;
}

u8 game_update(void)
{
    u32 key = rt_keys & ~rt_prev;
    SaveUnit *u;
    u8 at;
    if(props_on)return props_update(key);
    if(mg.slide)mg.slide--;
    if(mg.job_anim) {
        mg.job_anim--;dirty=1;
        if(key&(K_B|K_ESC))mg.job_anim=0;
        else {if(key&(K_ENTER|K_A))mg.job_anim=0;return 1;}
    }
    if(mg.spin){mg.spin+=mg.spin>0?-1:1;dirty=1;return 1;}
    if (!key) return 1;
    dirty = 1;
    if (!bank || !save.n) { if (key & K_ESC)return leave();return 1; }
    u = unit(mg.unit);
    if(prop_away(&save.missions.dispatches,mg.unit)&&mg.screen!=S_FORM&&mg.screen!=S_MENU&&mg.screen!=S_STATUS){mg.screen=S_STATUS;mg.note=NOTE_LOCKED;}
    if(prop_away(&save.missions.dispatches,mg.unit)&&mg.screen==S_STATUS)key&=K_B|K_ESC|K_F1|K_F5|K_MODE;
    if (mg.note) { if (key & (K_ENTER | K_A | K_B | K_ESC)) mg.note = 0; return 1; }
    if (mg.remove) {
        if(prop_away(&save.missions.dispatches,mg.unit)||save.missions.report.ready){mg.remove=0;mg.note=NOTE_LOCKED;return 1;}
        if (key & (K_B | K_ESC)) { mg.remove = 0; mg.remove_row = 0; return 1; }
        mg.remove_row = choice_move(mg.remove_row, 2, key);
        if (key & (K_ENTER | K_A)) {
            u8 yes = !mg.remove_row;
            if (!yes) { mg.remove = 0; mg.remove_row = 0; return 1; }
            if (mg.remove == 1) { mg.remove = 2; mg.remove_row = 0; return 1; }
            mg.remove = 0; mg.remove_row = 0;
            if (roster_remove(&save, mg.unit)) {
                mg.changed = 1;
                if (mg.unit >= save.n) mg.unit = save.n ? (u8)(save.n - 1) : 0;
                roster_position(S_FORM); mg.row = mg.slide = 0;
            }
        }
        return 1;
    }
    if (mg.detail) { if (key & (K_MODE | K_B | K_ESC | K_ENTER | K_A)) mg.detail = 0; return 1; }
    if ((key & K_MODE) && (detail_item() || detail_ability() || mg.screen == S_STATUS))
        { mg.detail = 1; return 1; }
    if (mg.confirm) {
        if(save_egg(&save,mg.unit)){
            if(key&(K_B|K_ESC))mg.confirm=0;
            else if(key&(K_ENTER|K_A)){
                u8 accept=mg.confirm==2;mg.confirm=0;
                if(accept&&save_dismiss_egg(&save,mg.unit)){
                    mg.changed=1;if(mg.unit>=save.n)mg.unit=save.n?(u8)(save.n-1):0;
                    mg.row=mg.slide=0;roster_position(S_FORM);
                }
            }else mg.confirm=choice_move(mg.confirm-1,2,key)+1;
            return 1;
        }
        if (key & (K_B | K_ESC)) mg.confirm = 0;
        else if (key & (K_ENTER | K_A)) {
            u8 accept=mg.confirm==1;
            mg.confirm = 0;
            if(accept) {
                if (mg.screen == S_JOBS) {mg.old_picture=picture(u,career(u));change_job(u,mg.row);mg.job_anim=JOB_CHANGE_FRAMES;}
                else if (!learn(u, mg.ljob, learn_entry(u, (u8)(mg.top + mg.row)))) mg.note = NOTE_JP;
            }
        }else mg.confirm=choice_move(mg.confirm-1,2,key)+1;
        return 1;
    }
    if ((key & (K_F1 | K_F5)) && mg.screen != S_FORM && mg.screen != S_MENU && mg.screen != S_PICK
        && mg.screen != S_INV) {        // the inventory's own F1/F5 switches Eqp./All
        u8 tries=save.n;
        do{mg.unit=(u8)((mg.unit+(key&K_F1?save.n-1:1))%save.n);}
        while((save_egg(&save,mg.unit)||(unit(mg.unit)->type==3&&mg.screen!=S_STATUS))&&--tries);
        if(save_egg(&save,mg.unit)||(unit(mg.unit)->type==3&&mg.screen!=S_STATUS)){roster_position(S_FORM);return 1;}
        if (mg.screen == S_JOBS) open_jobs();
        else if (mg.screen == S_LEARN) mg.row = mg.top = 0;
        return 1;
    }
    // Left and right change tab on the unit's own sheet; its screens have no horizontal
    // navigation of their own (the job wheel keeps its arrows and is left with SHIFT).
    if ((key & (K_LEFT | K_RIGHT)) && mg.screen != S_FORM && mg.screen != S_MENU &&
        mg.screen != S_ORDER && mg.screen != S_JOBS && mg.screen != S_PICK && mg.screen != S_ITEM) {
        static const u8 dest[TAB_N] = { S_STATUS, S_EQUIP, S_SET, S_JOBTABLE, S_JOBS };
        u8 tab = screen_tab(mg.screen);
        tab = (u8)(key & K_LEFT ? (tab ? tab - 1 : TAB_N - 1) : (tab + 1 == TAB_N ? 0 : tab + 1));
        if (u->type == 3 && (tab == TAB_LEARN || tab == TAB_JOB)) tab = TAB_STATUS;
        mg.screen = dest[tab]; mg.row = mg.top = 0;
        if (mg.screen == S_JOBS) open_jobs();
        else if (mg.screen == S_JOBTABLE) mg.ljob = career(u);
        return 1;
    }
    switch (mg.screen) {
    case S_FORM:
        if (key&K_ESC||(save_egg(&save,mg.unit)&&(key&K_B)))return leave();
        at = mg.unit;
        if (key & K_LEFT) at = at ? (u8)(at - 1) : (u8)(save.n - 1);
        if (key & K_RIGHT) at = (u8)(at + 1 == save.n ? 0 : at + 1);
        if (key & K_UP) at = at >= 5 ? (u8)(at - 5) : at;
        if (key & K_DOWN) at = at + 5 < save.n ? (u8)(at + 5) : at;
        if(at!=mg.unit) {
            s16 x=16+(mg.unit%5)*32,y=29+(mg.unit/5-mg.top)*27;
            if(mg.slide){x+=((mg.floor_from_x-x)*mg.slide)>>2;y+=((mg.floor_from_y-y)*mg.slide)>>2;}
            mg.floor_from_x=x;mg.floor_from_y=y;mg.slide=4;
        }
        mg.unit = at;
        if (at / 5 < mg.top) mg.top = at / 5;
        if (at / 5 > mg.top + 1) mg.top = at / 5 - 1;
        if ((key & (K_ENTER | K_A))&&!save_egg(&save,mg.unit)) { mg.screen = prop_away(&save.missions.dispatches,mg.unit)?S_STATUS:S_EQUIP; mg.row = mg.top = 0; }
        if (key & (K_C | K_D)) { mg.screen = S_MENU; mg.row = 0;mg.slide=0; }
        return 1;
    case S_MENU:
        if (key & (K_B | K_ESC)) { roster_position(S_FORM); return 1; }
        if(save_egg(&save,mg.unit)){
            if(key&(K_ENTER|K_A))mg.confirm=1;
            return 1;
        }
        mg.row=choice_move(mg.row,MENU_ROWS,key);
        if (key & (K_ENTER | K_A)) {
            if(save.missions.report.ready||(!mg.row&&prop_away(&save.missions.dispatches,mg.unit))){mg.note=NOTE_LOCKED;return 1;}
            if (!mg.row) {                           // Remove Unit
                if (u->type == 2) { mg.note = NOTE_RAMZA; return 1; }
                mg.remove = 1; mg.remove_row = 0; return 1;
            }
            mg.screen = S_ORDER; mg.row = mg.top = 0;
        }
        return 1;
    case S_ITEM:
        if (key & (K_B | K_ESC)) { mg.screen = S_EQUIP; mg.row = 0; return 1; }
        mg.row = choice_move(mg.row, 4, key);
        if (key & (K_ENTER | K_A)) {
            if (mg.row == 0) { mg.screen = S_EQUIP; mg.row = mg.top = 0; }
            else if (mg.row == 1) best_fit_unit(u);   // the source equips, with no notice
            else if (mg.row == 2) remove_all_equipment(u);
            else { mg.screen = S_INV; mg.row = mg.top = 0; build_inventory(u); }
        }
        return 1;
    case S_INV:
        if (key & (K_B | K_ESC)) { mg.screen = S_EQUIP; mg.row = 0; mg.top = 0; return 1; }
        if (key & (K_F1 | K_F5)) { mg.inv_all ^= 1; build_inventory(u); return 1; }
        list_move(key, mg.inv_n);
        return 1;
    case S_ORDER:
        if (key & (K_B | K_ESC)) { roster_position(S_MENU); mg.row = 1; return 1; }
        mg.row = choice_move(mg.row, ORD_N, key);
        if (key & (K_ENTER | K_A)) {
            u8 i, k, best;
            if (mg.row != ORD_NUMBER) {
                for (i = 0; i + 1 < save.n; i++) {
                    best = i;
                    for (k = (u8)(i + 1); k < save.n; k++) {
                        Stats a, b;
                        a = stats(&save.unit[best]); b = stats(&save.unit[k]);
                        if (roster_key(&save, k, mg.row, b.hp, b.mp) <
                            roster_key(&save, best, mg.row, a.hp, a.mp)) best = k;
                    }
                    if (best != i) roster_slot_swap(&save, i, best);
                }
                mg.changed = 1;
            }
            mg.unit = 0; roster_position(S_FORM); mg.row = mg.slide = 0;
        }
        return 1;
    case S_JOBTABLE:
        if (key & (K_B | K_ESC)) { roster_position(S_FORM); return 1; }
        at = list_move(key, jobtable_count(u));
        if (key & (K_ENTER | K_A)) {
            u8 c = jobtable_entry(u, at);
            if (c < CAREERS) { mg.ljob = c; mg.screen = S_LEARN; mg.row = mg.top = 0; }
        }
        return 1;
    case S_STATUS:
        if (key & (K_B | K_ESC)) roster_position(S_FORM);
        return 1;
    case S_EQUIP: case S_SET:
        if (key & (K_B | K_ESC)) { roster_position(S_FORM); return 1; }
        if ((key & (K_C | K_D)) && mg.screen == S_EQUIP) { mg.screen = S_ITEM; mg.row = 0; return 1; }
        {
            u8 n = mg.screen == S_EQUIP ? SLOTS : 4;
            mg.row=choice_move(mg.row,n,key);
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
            else { set16(slot_of(u, mg.slot), v); mg.changed = 1;
                validate_equipment(u);
            }
            mg.screen = mg.back; mg.top = 0;
            if (mg.back == S_EQUIP) { for (at = 0; slot_order[at] != mg.slot; at++) ; mg.row = at; }
            else mg.row = mg.slot;
        }
        return 1;
    case S_LEARN:
        if (key & (K_B | K_ESC)) {
            if (u->type == 3) { roster_position(S_MENU); mg.row = 2; }
            else { mg.screen = S_JOBTABLE; mg.row = mg.top = 0; }
            return 1;
        }
        at = list_move(key, learn_count(u));
        if ((key & (K_ENTER | K_A)) && !learned(u, mg.ljob, learn_entry(u, at))) {
            const u8 *j = job_rec(record(u, mg.ljob));
    u8 monster_job[JOB_SIZE];
    if(u->type==3){const u8 *p=cj_section(common,CJ_JOBS)+14U*u->job;memset(monster_job,0,sizeof monster_job);memcpy(monster_job+J_GROWTH,p,10);monster_job[J_MOVE]=p[10];monster_job[J_JUMP]=p[11];j=monster_job;}
            if (jp(u, mg.ljob) < ability_cost(job_ability(j, learn_entry(u, at)))) mg.note = NOTE_JP;
            else mg.confirm = 1;
        }
        return 1;
    case S_JOBS:
        if (key & (K_B | K_ESC)) { roster_position(S_FORM); return 1; }
        if(key&(K_LEFT|K_UP|K_RIGHT|K_DOWN)) {
            u8 old=mg.job_pos;
            s16 delta;
            if(key&(K_LEFT|K_UP))mg.job_pos=mg.job_pos?mg.job_pos-1:mg.job_n-1;
            else mg.job_pos=mg.job_pos+1<mg.job_n?mg.job_pos+1:0;
            delta=mg.angle[mg.job_pos]-mg.angle[old];
            if(delta>16)delta-=32;
            if(delta<-16)delta+=32;
            mg.spin=(s8)delta;mg.row=mg.jobs[mg.job_pos];
        }
        at=mg.row;
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
        if(props_on)props_page();else page();
        rt_light = l; rt_dark = d;
        dirty = 0; settled = 0;
    }
    // A page with nothing live on it has to reach the hidden planes once each and no more:
    // the TI alternates two of them (GrayDBufToggleSync), so once both carry the picture the
    // display is already right and the frame can cost nothing. The detail panel is the case
    // that matters -- it is modal and full screen, so no cursor is drawn over it.
    if (mg.detail && settled >= 2) return;
    memcpy(rt_light, cache, RT_PSIZE); memcpy(rt_dark, cache + RT_PSIZE, RT_PSIZE);
    if (settled < 2) settled++;
    if(!bank||!save.n)return;
    if(props_on){props_live();return;}
    if(mg.detail)return;                 // the detail panel is modal: no live cursor over it
    if(mg.screen==S_FORM)formation_cursor();
    if(mg.screen==S_MENU&&!mg.remove)command_menu();
    if(mg.screen==S_ITEM)item_menu();
    if(mg.screen==S_ORDER)order_menu();
    list_cursor();
    confirm_panel();
    remove_panel();
}


#include "proposition_ui.inc"
