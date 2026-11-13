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
#ifdef RT_CYCLES
#include "../../../tools/m68kbench/bench.h"
#endif

enum { S_FORM, S_MENU, S_STATUS, S_EQUIP, S_SET, S_LEARN, S_JOBS, S_PICK, S_NONE };
enum { CAREERS = 20, RAMZA_JOB = 20, JOB_SIZE = 32, ITEM_SIZE = 14, SLOTS = 5, ROWS = 6 };
enum { EQ_HEAD, EQ_BODY, EQ_ACC, EQ_WEAPON, EQ_SHIELD };   // the save's order
enum { JOB_CHANGE_FRAMES=96 }; // source-observed light column, adapted to30Hz/four greys
typedef struct {
    u8 screen, back, unit, row, top, slot, confirm, note;   // note: a message (NOTE_*)
    u8 pick_n;                          // S_PICK: entries (pick[]: an item id or an ability)
    u16 pick[64];
    u8 changed;                         // the party edited (written at the end)
    u8 jobs[CAREERS], angle[CAREERS], job_n, job_pos;
    s8 spin;
    u8 slide,job_anim,old_picture;
    s16 floor_from_x,floor_from_y;
} Mg;
enum { NOTE_NONE, NOTE_JP, NOTE_LOCKED };
Mg mg;
Save save;
static const u8 *bank, *common,*identity;
static u8 *cache;
static u8 dirty;

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

typedef struct { u16 hp, mp; u8 sp, pa, ma, move, jump; } Stats;
#include "../movement_rules.h"
#include "../raw_stats.h"
Stats stats(const SaveUnit *u)             // fft.c unit_stats, from the save's records
{
    const u8 *j = job_rec(record(u, career(u)));
    u8 monster_job[JOB_SIZE];
    if(u->type==3){const u8 *p=cj_section(common,CJ_JOBS)+14U*u->job;memset(monster_job,0,sizeof monster_job);memcpy(monster_job+J_GROWTH,p,10);monster_job[J_MOVE]=p[10];monster_job[J_JUMP]=p[11];j=monster_job;}
    Stats s;
    u16 v16[5];
    u8 i,k,current=0;
    for(k=0;k<save.n;k++)if(u==&save.unit[k]){current=save_raw_current(&save,k);break;}
    for (i = 0; i < 5; i++) {
        u32 v = (u32)u->raw[i][0] << 16 | (u16)u->raw[i][1] << 8 | u->raw[i][2];
        if(!current)v=core_raw_grown(v,j[J_GROWTH+i],u->level);
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
    {u8 f[3];u16 id=b16(u->movement);cj_movement(common,u->job,(u16)(id-486)<24U?(u8)(id-485):0,f);
     core_movement_bonus(f[0],&s.move,&s.jump);}
    if (s.jump > 7) s.jump = 7;
    s.sp = (u8)(v16[2] > 50 ? 50 : v16[2]);
    s.pa = (u8)(v16[3] > 99 ? 99 : v16[3]);
    s.ma = (u8)(v16[4] > 99 ? 99 : v16[4]);
    return s;
}

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
u8 learn(SaveUnit *u, u8 k)
{
    u8 c = career(u);
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
#include "../portrait32.inc"
static void icon(s16 x, s16 y, u8 id)
{
    const u8 *p = section(4) + (u16)item_rec(id)[I_ICON] * 24;
    RtSprite sp = { 8, 8, p, p + 8, p + 16 };
    draw_sprite(x, y, &sp);
}
static void hand(s16 x, s16 y)
{
    choice_hand_at(x,y);
}

static const char *unit_name(const SaveUnit *u)
{
    if(u->type==3)return (const char *)(cj_section(common,CJ_JOB_NAMES)+14U*u->job);
    if (u->type == 2) return "Ramza";
    return identity_name(identity,(u16)u->name+(u->type==1?255:0));
}
static const char *job_name(const SaveUnit *u, u8 c) { return u->type==3?unit_name(u):str(b16(job_rec(record(u, c)))); }

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

#include "../unit_card.h"
#define CARD_TEXT(x,y,s,c,l,context) text(x,y,s,c,l)
#define CARD_WIDTH(s) card_width(s)
#define CARD_MANAGEMENT_LAYOUTS
#include "../unit_card.inc"
static void management_card(UnitCard *d,const SaveUnit *u,const Stats *s)
{
    d->portrait=u->type==3?RT_NULL:identity_portrait(identity,section(6)[picture(u,career(u))]);
    d->name=unit_name(u);d->job=job_name(u,career(u));
    d->hp=d->max_hp=s->hp;d->mp=d->max_mp=s->mp;
    d->level=u->level;d->exp=u->exp;d->brave=u->brave;d->faith=u->faith;
}

// Formation and deployment use the same portrait/stat layout and full HP/MP bars.
static void formation_details(const SaveUnit *u,const Stats *s,s16 y)
{
    UnitCard d;u8 part;management_card(&d,u,s);
    for(part=0;part<UC_PARTS;part++)unit_card_draw(&d,UC_FORMATION,y,part,0,0);
}

static void unit_header(void)
{
    const SaveUnit *u = unit(mg.unit);
    Stats s = stats(u);
    UnitCard d;u8 part;management_card(&d,u,&s);
    for(part=0;part<=UC_JOB;part++)unit_card_draw(&d,UC_MANAGEMENT,0,part,0,0);
    actor(140,15,picture(u,career(u)));
    draw_rect(2, 46, 156, 1, C_LGRAY);
}

static const char *const menu_rows[] = { "Equip", "Abilities", "Learn", "Change Job" };
static const char *const slot_rows[] = { "Head", "Body", "Accessory", "R.Hand", "L.Hand" };
static const char *const set_rows[] = { "Secondary", "Reaction", "Support", "Movement" };
static const u8 slot_order[SLOTS] = { EQ_WEAPON, EQ_SHIELD, EQ_HEAD, EQ_BODY, EQ_ACC };   // shown

static void formation(void)
{
    u8 i, first = (u8)(mg.top * 5);
    const SaveUnit *u = unit(mg.unit);
    Stats s=stats(u);
    for (i = 0; i < 10 && first + i < save.n; i++) {
        s16 x = 8 + (i % 5) * 32, y = 7 + (i / 5) * 27;
        const SaveUnit *v = unit((u8)(first + i));
        actor(x,y,picture(v,career(v)));
    }
    draw_rect(0,62,160,1,C_BLACK);formation_details(u,&s,65);
    if(save.n>10){black(154,8,mg.top?"^":"");black(154,54,first+10<save.n?"v":"");}
    if(mg.screen==S_FORM)black(37,95,"ENTER:status ALPHA:menu");
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
        const SaveUnit *v=unit((u8)(first+i));
        actor(8+(i%5)*32,7+(i/5)*27,picture(v,career(v)));
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
    u8 monster_job[JOB_SIZE];
    if(u->type==3){const u8 *p=cj_section(common,CJ_JOBS)+14U*u->job;memset(monster_job,0,sizeof monster_job);memcpy(monster_job+J_GROWTH,p,10);monster_job[J_MOVE]=p[10];monster_job[J_JUMP]=p[11];j=monster_job;}
    u8 k;
    for (k = 0; k < 22; k++) if (job_ability(j, k) && !row--) return k;
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
    u8 monster_job[JOB_SIZE];
    if(u->type==3){const u8 *p=cj_section(common,CJ_JOBS)+14U*u->job;memset(monster_job,0,sizeof monster_job);memcpy(monster_job+J_GROWTH,p,10);monster_job[J_MOVE]=p[10];monster_job[J_JUMP]=p[11];j=monster_job;}
    u8 k = learn_entry(u, i);
    black(31, y, ability_name(job_ability(j, k)));
    if (learned(u, career(u), k)) black(118, y, "Learned");
    else right(146, y, ability_cost(job_ability(j, k)));
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
    static const ChoiceLayout menu={65,53,94,27,3,CHOICE_TAB,3,1,0,6,10,8,157,-15,7};
    if (!mg.confirm) return;
    choice_frame(&menu,mg.screen==S_JOBS?job_name(u,mg.row):"Learn ability?");
    choice_row(&menu,0,mg.screen==S_JOBS?"Change Job":"Learn",C_BLACK);
    choice_row(&menu,1,"Return",C_BLACK);
    choice_cursor_animated(&menu,mg.confirm-1,rt_frame);
}

static void command_menu(void)
{
    static const ChoiceLayout menu={56,15,101,50,0,CHOICE_PANEL,9,4,0,25,14,8,159,1,10};
    u8 i;choice_frame(&menu,"Menu");
    for(i=0;i<4;i++)choice_row(&menu,i,menu_rows[i],C_BLACK);
    choice_cursor_animated(&menu,mg.row,rt_frame);
}

static void list_cursor(void)
{
    ChoiceLayout cursor={0,0,160,100,0,CHOICE_BARE,0,0,0,0,0,7,159,5,39};
    if(mg.note||mg.confirm)return;
    if(mg.screen==S_EQUIP||mg.screen==S_SET)cursor.pitch=8;
    else if(mg.screen!=S_LEARN&&mg.screen!=S_PICK)return;
    choice_cursor_animated(&cursor,mg.row,rt_frame);
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
            formation_cursor();
            foot("ENTER:open SHIFT:back");
        }
        note_panel();
        return;
    }
    if(mg.screen==S_JOBS){jobs_page();note_panel();return;}
    unit_header();
    if(mg.screen==S_LEARN){black(91,25,"JP");right(127,25,jp(unit(mg.unit),career(unit(mg.unit))));}
    switch (mg.screen) {
    case S_STATUS: status_page(); break;
    case S_EQUIP: equip_page(); break;
    case S_SET: set_page(); break;
    case S_LEARN:
        list_rows(learn_count(unit(mg.unit)), learn_row);
        foot("ENTER:learn SHIFT:back F1/F5:unit"); break;
    case S_PICK:
        list_rows(mg.pick_n, pick_row);
        foot("ENTER:choose SHIFT:back"); break;
    }
    note_panel();
    // Recover six rows beneath the compact title; all body/overlay coordinates
    // above remain shared with the existing equipment and ability list layout.
    memmove((u8 *)rt_light+7*RT_PBYTES,(u8 *)rt_light+13*RT_PBYTES,87*RT_PBYTES);
    memmove((u8 *)rt_dark+7*RT_PBYTES,(u8 *)rt_dark+13*RT_PBYTES,87*RT_PBYTES);
    draw_rect(0,94,160,6,C_WHITE);
}

// ---------------------------------------------------------------- the frame loop
static void open_list(u8 screen, u8 at)          // a scrolling list, its entry at shown
{
    mg.screen = screen;
    mg.top = at >= ROWS ? (u8)(at - ROWS + 1) : 0;
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
    bank = RT_NULL; cache = RT_NULL;     // (statics survive between runs)
    memset(&mg, 0, sizeof mg);
    bank = rt_file("ffmg", &size);
    if (bank && memcmp(bank, "FMG3", 4)) bank = RT_NULL;
    identity=rt_file("ffcp",&size);if(!identity_valid(identity,size))bank=RT_NULL;
    common = rt_file("ffcj", &size);
    if (!cj_valid(common, size)) bank = RT_NULL;
    save_load(&save);
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
    if (mg.screen == S_JOBS) open_jobs();
    dirty = 1;
}

#include "../../../lib/tilog.h"
static void leave(void)
{
    tilog("manage leave%s", mg.changed ? " (session saved)" : "");
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
    if(mg.slide)mg.slide--;
    if(mg.job_anim) {
        mg.job_anim--;dirty=1;
        if(key&(K_B|K_ESC))mg.job_anim=0;
        else {if(key&(K_ENTER|K_A))mg.job_anim=0;return 1;}
    }
    if(mg.spin){mg.spin+=mg.spin>0?-1:1;dirty=1;return 1;}
    if (!key) return 1;
    dirty = 1;
    if (!bank || !save.n) { if (key & K_ESC) { leave(); return 0; } return 1; }
    u = unit(mg.unit);
    if (mg.note) { if (key & (K_ENTER | K_A | K_B | K_ESC)) mg.note = 0; return 1; }
    if (mg.confirm) {
        if (key & (K_B | K_ESC)) mg.confirm = 0;
        else if (key & (K_ENTER | K_A)) {
            u8 accept=mg.confirm==1;
            mg.confirm = 0;
            if(accept) {
                if (mg.screen == S_JOBS) {mg.old_picture=picture(u,career(u));change_job(u,mg.row);mg.job_anim=JOB_CHANGE_FRAMES;}
                else if (!learn(u, learn_entry(u, (u8)(mg.top + mg.row)))) mg.note = NOTE_JP;
            }
        }else mg.confirm=choice_move(mg.confirm-1,2,key)+1;
        return 1;
    }
    if ((key & (K_F1 | K_F5)) && mg.screen != S_FORM && mg.screen != S_MENU && mg.screen != S_PICK) {
        mg.unit = (u8)((mg.unit + (key & K_F1 ? save.n - 1 : 1)) % save.n);
        if (mg.screen == S_JOBS) open_jobs();
        else if (mg.screen == S_LEARN) mg.row = mg.top = 0;
        return 1;
    }
    switch (mg.screen) {
    case S_FORM:
        if (key & K_ESC) { leave(); return 0; }
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
        if (key & (K_ENTER | K_A)) mg.screen = S_STATUS;
        if (key & (K_C | K_D)) { mg.screen = S_MENU; mg.row = 0;mg.slide=0; }
        return 1;
    case S_MENU:
        if (key & (K_B | K_ESC)) { roster_position(S_FORM); return 1; }
        mg.row=choice_move(mg.row,4,key);
        if (key & (K_ENTER | K_A)) {
            static const u8 dest[] = { S_EQUIP, S_SET, S_LEARN, S_JOBS };
            if(u->type==3){mg.note=NOTE_LOCKED;return 1;}
            mg.screen = dest[mg.row]; mg.row = mg.top = 0;
            if (mg.screen == S_JOBS) open_jobs();
        }
        return 1;
    case S_STATUS:
        if (key & (K_B | K_ESC | K_ENTER | K_A)) roster_position(S_FORM);
        return 1;
    case S_EQUIP: case S_SET:
        if (key & (K_B | K_ESC)) { mg.row = mg.screen == S_EQUIP ? 0 : 1; roster_position(S_MENU); return 1; }
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
        if (key & (K_B | K_ESC)) { roster_position(S_MENU); mg.row = 2; return 1; }
        at = list_move(key, learn_count(u));
        if ((key & (K_ENTER | K_A)) && !learned(u, career(u), learn_entry(u, at))) {
            const u8 *j = job_rec(record(u, career(u)));
    u8 monster_job[JOB_SIZE];
    if(u->type==3){const u8 *p=cj_section(common,CJ_JOBS)+14U*u->job;memset(monster_job,0,sizeof monster_job);memcpy(monster_job+J_GROWTH,p,10);monster_job[J_MOVE]=p[10];monster_job[J_JUMP]=p[11];j=monster_job;}
            if (jp(u, career(u)) < ability_cost(job_ability(j, learn_entry(u, at)))) mg.note = NOTE_JP;
            else mg.confirm = 1;
        }
        return 1;
    case S_JOBS:
        if (key & (K_B | K_ESC)) { roster_position(S_MENU); mg.row = 3; return 1; }
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
        page();
        rt_light = l; rt_dark = d;
        dirty = 0;
    }
    memcpy(rt_light, cache, RT_PSIZE); memcpy(rt_dark, cache + RT_PSIZE, RT_PSIZE);
    if(!bank||!save.n)return;
    if(mg.screen==S_FORM)formation_cursor();
    if(mg.screen==S_MENU)command_menu();
    list_cursor();
    confirm_panel();
}

