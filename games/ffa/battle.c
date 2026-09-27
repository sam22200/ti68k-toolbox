// Battles (FF7-like, one hero): ATB gauges in wait mode, Attack / Limit, Magic (Fire, Cure),
// Item, Run. Formulas of the original (docs/part1.md §5) in integer arithmetic: stats are x STAT,
// rand(n) of TI-Basic (1..n) is R(n). The few divisions run once per action, never per pixel.
#include "ffa.h"
#include "gfx.h"

#define R(n) (rt_rand() % (n) + 1)
#define FULL (30 * 256)                       // ATB gauge full (original 127 -> 157)
enum { BP_INTRO, BP_RUN, BP_MENU, BP_MAGIC, BP_ITEM, BP_HACT, BP_MACT, BP_MSG, BP_WIN, BP_LOSE, BP_END };
enum { ACT_ATTACK, ACT_LIMIT, ACT_FIRE, ACT_CURE, ACT_ITEM, ACT_RUN };
enum { SP_NO, SP_ICE, SP_FIRE };

typedef struct {
    u16 hp; u8 atk, def, spd; u16 exp, gils; u8 ap;
    u8 drop, dnum, dden, special, spell, snum, sden, power10, mdef, evade;
} Mon;
static const Mon mons[4] = {                  // mons[n, ...] of the original, part I monsters
    { 0 },
    { 90, 7, 10, 14, 85, 50, 7, I_POTION, 2, 4, 3, SP_NO, 0, 0, 0, 0, 0 },
    { 120, 10, 30, 16, 125, 70, 10, I_POTION, 2, 5, 0, SP_ICE, 1, 3, 20, 15, 4 },
    { 1000, 30, 45, 12, 330, 1000, 0, I_HIPOTION, 1, 1, 0, SP_FIRE, 2, 5, 45, 20, 6 },
};

static const char *const msgs[] = {
    "", "Miss", "Can't run away!", "Escaped!", "Victory!", "Level UP!", "Game Over", "Max LIMIT!!",
    "Braver", "Fire", "Cure", "Potion", "Hi-Potion", "Ether", "Ice", "Not enough MP", "Found: Potion",
    "Found: Hi-Potion", "Critical!",
};
enum { MS_NONE, MS_MISS, MS_NORUN, MS_ESCAPED, MS_VICTORY, MS_LEVEL, MS_OVER, MS_MAXLIM, MS_BRAVER,
       MS_FIRE, MS_CURE, MS_POTION, MS_HIPOTION, MS_ETHER, MS_ICE, MS_NOMP, MS_FOUND_POTION,
       MS_FOUND_HIPOTION, MS_CRIT };

static u8 act;                                // action being animated (not saved: set per turn)

// ---------------------------------------------------------------- hero figures
static u16 h_str(void)
{
    u16 s = st.hero.st[S_STR];
    if (st.hero.acc[0] == A_WRIST || st.hero.acc[1] == A_WRIST) s += 5 * STAT;   // Strength+5
    return s;
}
static u8 h_atq(void) { return st.hero.weapon == A_SWORD ? 10 : 7; }
static u8 h_rate(void) { return st.hero.weapon == A_SWORD ? 92 : 100; }
static u8 h_adef(void) { return st.hero.armor == A_BANGLE ? 5 : 0; }
static u8 h_adefm(void) { return st.hero.armor == A_BANGLE ? 2 : 0; }
static u8 h_esq(void) { return st.hero.armor == A_BANGLE ? 2 : 0; }

u8 has_materia(u8 m)                          // owned and set in a slot of the weapon
{
    return st.mat[m] && st.hero.weapon && (st.hero.slot[0] == m || st.hero.slot[1] == m);
}

static u16 vary(u16 c)                        // 0.95 c - 1 + rand(int(0.1 c) + 1)
{
    s16 v = (s16)((u32)c * 95 / 100) - 1 + R(c / 10 + 1);
    return v < 0 ? 0 : v;
}

// ---------------------------------------------------------------- start
void battle_start(u8 n)
{
    const u8 *tab;
    u8 id = rooms[st.room].id;
    st.ret_mode = st.mode;
    st.qu = n == 0;                           // random fights can be escaped, bosses never
    if (!n) {                                 // choimon: the room's table [N, n1, t1, n2, t2]
        static const u8 t10[5] = { 5, 1, 1, 2, 5 }, t11[5] = { 5, 1, 3, 2, 5 };
        u8 bb;
        tab = id == 10 ? t10 : t11;
        bb = R(tab[0]);
        n = bb <= tab[2] ? tab[1] : tab[3];
    }
    st.battle = n;
    st.mhp = mons[n].hp;
    st.ja = (1 + R(10)) * 256;
    st.jae = (1 + R(10)) * 256;
    st.bp = BP_INTRO;
    st.bt = 0;
    st.bcur = 0;
    st.bmsg = MS_NONE;
    st.bwho = 0;
    st.won = 0;
    st.bdrop = 0;
    st.mode = M_BATTLE;
}

static void say(u8 m, u8 next)                // show a message, then go to phase `next`
{
    st.bmsg = m;
    st.bsub = next;
    st.bp = BP_MSG;
    st.bt = 0;
}

static void hit_hero(u16 dmg)                 // damage taken: HP and the limit gauge
{
    u32 g;
    st.hero.hp = dmg >= st.hero.hp ? 0 : st.hero.hp - dmg;
    g = (u32)31 * 256 * dmg / st.hero.hpm;   // (35 - 4 lim) * coup / hpm, lim 1
    g += st.hero.jl;
    st.hero.jl = g > LIMIT_FULL ? LIMIT_FULL : g;
}

// ---------------------------------------------------------------- actions
static void hero_apply(void)                  // the hero's action lands (mid-animation)
{
    const Mon *m = &mons[st.battle];
    u16 c;
    st.bwho = 1;
    if (act == ACT_ATTACK || act == ACT_LIMIT) {
        u16 s = h_str(), rate = h_rate();
        if (act == ACT_LIMIT) { s = s * 5 / 2; rate = 200; }   // Braver: 2.5 x strength, sure hit
        c = (u32)s * h_atq() * (255 - m->def) / (510UL * STAT);
        c = vary(c);
        if (R(100) < st.hero.st[S_LUCK] / STAT) c *= 2;       // critical
        if ((s16)R(100) - st.hero.lv / 4 - st.hero.st[S_LUCK] / (2 * STAT) > (s16)rate - m->evade) c = 0;
    } else if (act == ACT_FIRE) {
        c = vary(st.hero.st[S_MAG] * 9 / STAT);
        c = (u32)c * (255 - m->mdef) / 255;
    } else if (act == ACT_CURE) {
        u16 r = st.hero.st[S_MAG] * 9 / STAT;
        r = r * 9 / 10 - 1 + R(r / 5 + 1);
        st.bwho = 2;
        st.bnum = -(s16)r;
        st.hero.hp = st.hero.hp + r > st.hero.hpm ? st.hero.hpm : st.hero.hp + r;
        return;
    } else {                                  // item: already applied
        st.bwho = 2;
        return;
    }
    st.bnum = c;
    st.mhp = c >= st.mhp ? 0 : st.mhp - c;
}

static void mon_apply(void)                   // the monster's action lands
{
    const Mon *m = &mons[st.battle];
    u16 c;
    st.bwho = 2;
    if (st.bsub == 1) {                       // spell: power x degm 9, magic defence in %
        c = vary(m->power10 * 9 / 10);
        c = (u32)c * (100 - h_adefm() - st.hero.st[S_MDEF] / STAT) / 100;
    } else {
        c = vary(m->atk);
        if (R(30) <= 1) c *= 2;
        c = (u32)c * (100 - h_adef() - st.hero.st[S_DEF] / STAT) / 100;
        if (m->special && R(10) <= m->special) c = c * 3 / 2;
        if (R(100) <= h_esq() + st.hero.st[S_LUCK] / (2 * STAT)) c = 0;   // evaded
    }
    st.bnum = c;
    st.bflash = st.hero.jl < LIMIT_FULL;      // remember: was the gauge not yet full?
    hit_hero(c);
    st.bflash = st.bflash && st.hero.jl >= LIMIT_FULL;
}

static void hero_start(u8 a)
{
    act = a;
    st.bp = BP_HACT;
    st.bt = 0;
    st.bwho = 0;
}

static void mon_start(void)                   // atqen: spell if the dice say so, else strike
{
    const Mon *m = &mons[st.battle];
    st.bsub = m->spell && R(m->sden) <= m->snum && st.hero.hp * 10 > m->atk * 6;
    st.bp = BP_MACT;
    st.bt = 0;
    st.bwho = 0;
}

static void win(void)
{
    const Mon *m = &mons[st.battle];
    st.hero.exp += m->exp;
    st.hero.gils += m->gils;
    st.bdrop = R(m->dden) <= m->dnum;
    if (st.bdrop && st.item[m->drop] < 99) st.item[m->drop]++;
    st.stats_kills++;
    st.co = 20 + R(16);                       // fincomb: a new encounter threshold
    say(MS_VICTORY, BP_WIN);
    st.bcur = 0;                              // step of the reward screen
}

void hero_level_up(void)                      // fincomb: every threshold passed
{
    static const u8 gain[6] = { 10, 8, 10, 8, 5, 4 };   // +0.5 +0.4 +0.5 +0.4 +0.25 +0.2
    u8 k;
    while (st.hero.exp >= st.hero.expt) {
        st.hero.expn = (u32)st.hero.expn * 11 / 10;
        st.hero.expt += st.hero.expn;
        st.hero.lv++;
        for (k = 0; k < 6; k++) st.hero.st[k] += gain[k];
        st.hero.st[st.hero.speci] += STAT / 4;
        st.hero.hpm = (u32)st.hero.hpm * (1105 - st.hero.lv) / 1000;
        st.hero.mpm = (u32)st.hero.mpm * (1105 - st.hero.lv) / 1000;
    }
}

static void end(u8 won)
{
    st.won = won;
    st.bp = BP_END;
    st.mode = st.ret_mode == M_SCRIPT ? M_SCRIPT : M_WALK;
    st.mc = 0;
}

// ---------------------------------------------------------------- update
static u8 menu_n(void) { return st.bp == BP_MENU ? 4 : st.bp == BP_MAGIC ? 2 : 3; }

void battle_update(void)
{
    u8 go = input_pressed(K_A | K_ENTER), back = input_pressed(K_B | K_ESC);
    st.bt++;
    switch (st.bp) {
    case BP_INTRO:
        if (st.bt >= 16) { st.bp = BP_RUN; st.bt = 0; }
        break;
    case BP_RUN:                              // wait mode: the gauges run only here
        st.ja += st.hero.st[S_SPD] * 12 / STAT;
        st.jae += mons[st.battle].spd * 12;
        if (st.jae >= FULL) { st.jae = 0; mon_start(); }
        else if (st.ja >= FULL) { st.ja = FULL; st.bp = BP_MENU; st.bcur = 0; }
        break;
    case BP_MENU: case BP_MAGIC: case BP_ITEM:
        if (input_pressed(K_UP)) st.bcur = st.bcur ? st.bcur - 1 : menu_n() - 1;
        if (input_pressed(K_DOWN)) st.bcur = st.bcur + 1 < menu_n() ? st.bcur + 1 : 0;
        if (back && st.bp != BP_MENU) { st.bp = BP_MENU; st.bcur = 0; break; }
        if (!go) break;
        if (st.bp == BP_MENU) {
            if (st.bcur == 0) hero_start(st.hero.jl >= LIMIT_FULL ? ACT_LIMIT : ACT_ATTACK);
            else if (st.bcur == 3) {
                st.ja = 0;
                if (st.qu && R(4) < 4) { st.stats_flight++; say(MS_ESCAPED, BP_END); }
                else say(MS_NORUN, BP_RUN);
            } else { st.bp = st.bcur == 1 ? BP_MAGIC : BP_ITEM; st.bcur = 0; }
        } else if (st.bp == BP_MAGIC) {       // Fire 4 MP, Cure 5 MP; the turn is lost without
            u8 m = st.bcur ? MAT_CURE : MAT_FIRE, cost = st.bcur ? 5 : 4;
            if (!has_materia(m)) break;
            if (st.hero.mp < cost) { st.ja = 0; say(MS_NOMP, BP_RUN); break; }
            st.hero.mp -= cost;
            hero_start(st.bcur ? ACT_CURE : ACT_FIRE);
        } else {                              // Potion +100 HP, Hi-Potion +500, Ether +50 MP
            static const u8 it[3] = { I_POTION, I_HIPOTION, I_ETHER };
            u8 i = it[st.bcur];
            if (!st.item[i]) break;
            st.item[i]--;
            if (i == I_ETHER) { st.hero.mp = st.hero.mp + 50 > st.hero.mpm ? st.hero.mpm : st.hero.mp + 50; st.bnum = -50; }
            else {
                u16 r = i == I_POTION ? 100 : 500;
                st.bnum = -(s16)(st.hero.hpm - st.hero.hp < r ? st.hero.hpm - st.hero.hp : r);
                st.hero.hp = st.hero.hp + r > st.hero.hpm ? st.hero.hpm : st.hero.hp + r;
            }
            hero_start(ACT_ITEM);
        }
        break;
    case BP_HACT:                             // 32 frames: step in, strike at 16, step back
        if (st.bt == 16) hero_apply();
        if (st.bt >= 32) {
            if (act == ACT_LIMIT) st.hero.jl = 0;
            st.ja = 0;
            st.bp = BP_RUN;
            if (!st.mhp) win();
        }
        break;
    case BP_MACT:
        if (st.bt == 12) mon_apply();
        if (st.bt >= 28) {
            st.bp = BP_RUN;
            if (!st.hero.hp) say(MS_OVER, BP_LOSE);
            else if (st.bflash) { st.bflash = 0; say(MS_MAXLIM, BP_RUN); }
        }
        break;
    case BP_MSG:
        if (st.bt >= 40 || (go && st.bt > 4)) {
            st.bp = st.bsub;
            st.bt = 0;
            st.bmsg = MS_NONE;
            if (st.bp == BP_END) end(0);
            if (st.bp == BP_LOSE) { st.mode = M_GAMEOVER; }
        }
        break;
    case BP_WIN:                              // gains, drop, level ups: one screen, 2nd ends
        if (st.bt == 1) {
            u8 lv = st.hero.lv;
            hero_level_up();
            st.bcur = st.hero.lv != lv;
        }
        if (go && st.bt > 8) end(1);
        break;
    }
}

// ---------------------------------------------------------------- render
static void box(s16 x, s16 y, s16 w, s16 h)
{
    draw_rect(x, y, w, h, C_BLACK);
    draw_rect(x + 1, y + 1, w - 2, h - 2, C_WHITE);
}

static char *utoa5(char *d, u16 v)
{
    char t[6];
    u8 k = 0;
    do t[k++] = '0' + v % 10; while ((v /= 10));
    while (k) *d++ = t[--k];
    *d = 0;
    return d;
}

static void gauge(s16 x, s16 y, u16 v, u16 full)
{
    u16 w = (u32)v * 40 / full;
    draw_rect(x, y, 42, 4, C_BLACK);
    draw_rect(x + 1, y + 1, 40, 2, C_WHITE);
    if (w) draw_rect(x + 1, y + 1, w, 2, v >= full ? C_BLACK : C_DGRAY);
}

void battle_render(void)
{
    const u32 *ml, *md, *mm;
    u8 mh;
    s16 hx = 116, mx = 18, k;
    char s[24], *p;
    RtSprite spr;
    // background: the room where the fight happens (Chrono Trigger style), hero-centred
    {
        const Room *r = &rooms[st.room];
        draw_tilemap(world_tilemap(), world_cam(st.x + HB_W / 2, RT_W, r->w * TILE), world_cam(st.y, 70, r->h * TILE));
    }
    // monster
    if (st.battle == 1) { ml = mon1_light[0]; md = mon1_dark[0]; mm = mon1_mask[0]; mh = MON1_H; }
    else if (st.battle == 2) { ml = mon2_light[0]; md = mon2_dark[0]; mm = mon2_mask[0]; mh = MON2_H; }
    else { ml = mon3_light[0]; md = mon3_dark[0]; mm = mon3_mask[0]; mh = MON3_H; }
    if (st.bp == BP_RUN || st.bp == BP_MENU || st.bp == BP_MAGIC || st.bp == BP_ITEM)
        mh -= (rt_frame >> 4) & 1 ? 1 : 0;   // breathing: the sprite bobs by one pixel
    if (st.bp == BP_MACT && st.bt < 12 && !st.bsub) mx += st.bt;          // lunge
    else if (st.bp == BP_MACT && st.bt < 24 && !st.bsub) mx += 24 - st.bt;
    if (!(st.bp == BP_INTRO && (st.bt & 2)) && !(st.mhp == 0 && st.bp == BP_MSG && (st.bt & 4))
        && !(st.bp == BP_HACT && st.bwho == 1 && st.bt < 26 && (st.bt & 2))) {
        spr.w = 32; spr.h = mh; spr.light = ml; spr.dark = md; spr.mask = mm;
        draw_sprite(mx, 68 - mh, &spr);
    }
    // hero: steps towards the monster to strike
    if (st.bp == BP_HACT && (act == ACT_ATTACK || act == ACT_LIMIT))
        hx -= st.bt < 16 ? st.bt * 4 : (32 - st.bt) * 4;
    if (!(st.bp == BP_MACT && st.bwho == 2 && st.bt < 22 && (st.bt & 2))) {
        u8 sw = st.hero.weapon == A_SWORD;
        spr.w = 32; spr.h = BHERO_H;
        spr.light = sw ? bheros_light[0] : bhero_light[0];
        spr.dark = sw ? bheros_dark[0] : bhero_dark[0];
        spr.mask = sw ? bheros_mask[0] : bhero_mask[0];
        draw_sprite(hx, 68 - BHERO_H, &spr);
    }
    // spell effects: fire flickers on the monster, cure sparkles on the hero
    if (st.bp == BP_HACT && act == ACT_FIRE && st.bt >= 6 && st.bt < 22)
        for (k = 0; k < 5; k++) draw_rect(mx + 6 + ((k * 7 + st.bt * 3) % 20), 68 - mh / 2 - ((k * 5 + st.bt * 2) % 18), 3, 4, k & 1 ? C_WHITE : C_BLACK);
    if (st.bp == BP_HACT && act == ACT_CURE && st.bt >= 6 && st.bt < 26)
        for (k = 0; k < 4; k++) draw_rect(hx + 6 + k * 6, 60 - ((st.bt * 2 + k * 9) % 36), 2, 2, C_WHITE);
    // damage / heal number
    if (st.bwho && (st.bp == BP_HACT || st.bp == BP_MACT)) {
        s16 x = st.bwho == 1 ? mx + 8 : hx + 8, y = 68 - (st.bwho == 1 ? mh : BHERO_H) - 4 - (st.bt & 15) / 4;
        if (st.bnum == 0 && st.bwho) { box(x - 2, y - 1, 28, 10); draw_text(x + 1, y, "Miss", F_MEDIUM, C_BLACK); }
        else {
            p = utoa5(s, st.bnum < 0 ? -st.bnum : st.bnum);
            box(x - 2, y - 1, (p - s) * 6 + 4, 10);
            draw_text(x, y, s, F_MEDIUM, st.bnum < 0 ? C_DGRAY : C_BLACK);
        }
    }
    // panel: commands left, HP / MP / gauges right
    box(0, 70, 160, 30);
    if (st.bp == BP_MENU || st.bp == BP_MAGIC || st.bp == BP_ITEM) {
        static const char *const cmd[4] = { "Attack", "Magic", "Item", "Run" };
        static const char *const mag[2] = { "Fire  4", "Cure  5" };
        static const char *const itn[3] = { "Potion", "Hi-Pot", "Ether" };
        static const u8 iti[3] = { I_POTION, I_HIPOTION, I_ETHER };
        for (k = 0; k < menu_n(); k++) {
            const char *t = st.bp == BP_MENU ? (k == 0 && st.hero.jl >= LIMIT_FULL ? "Braver" : cmd[k])
                          : st.bp == BP_MAGIC ? mag[k] : itn[k];
            u8 dim = (st.bp == BP_MAGIC && !has_materia(k ? MAT_CURE : MAT_FIRE)) || (st.bp == BP_ITEM && !st.item[iti[k]]);
            draw_text(10, 72 + k * 7, t, F_SMALL, dim ? C_LGRAY : C_BLACK);
            if (st.bp == BP_ITEM) { utoa5(s, st.item[iti[k]]); draw_text(40, 72 + k * 7, s, F_SMALL, C_BLACK); }
        }
        draw_rect(4, 73 + st.bcur * 7, 3, 3, C_BLACK);
    }
    draw_text(62, 72, "HP", F_SMALL, C_BLACK);
    p = utoa5(s, st.hero.hp); *p++ = '/'; utoa5(p, st.hero.hpm);
    draw_text(74, 72, s, F_SMALL, C_BLACK);
    draw_text(62, 79, "MP", F_SMALL, C_BLACK);
    p = utoa5(s, st.hero.mp); *p++ = '/'; utoa5(p, st.hero.mpm);
    draw_text(74, 79, s, F_SMALL, C_BLACK);
    draw_text(62, 86, "T", F_SMALL, C_BLACK);
    gauge(70, 87, st.ja, FULL);
    draw_text(62, 93, "L", F_SMALL, C_BLACK);
    gauge(70, 94, st.hero.jl, LIMIT_FULL);
    // messages and the reward screen
    if (st.bp == BP_MSG && st.bmsg) {
        const char *t = msgs[st.bmsg];
        u8 n = 0;
        while (t[n]) n++;
        box(80 - n * 3 - 6, 4, n * 6 + 12, 12);
        draw_text(80 - n * 3, 6, t, F_MEDIUM, C_BLACK);
    }
    if (st.bp == BP_WIN) {
        const Mon *m = &mons[st.battle];
        box(20, 4, 120, st.bcur ? 50 : 42);
        p = utoa5(s, m->exp); draw_text(26, 7, "EXP", F_MEDIUM, C_BLACK); draw_text(80, 7, s, F_MEDIUM, C_BLACK);
        p = utoa5(s, m->gils); draw_text(26, 17, "Gils", F_MEDIUM, C_BLACK); draw_text(80, 17, s, F_MEDIUM, C_BLACK);
        if (st.bdrop) draw_text(26, 27, msgs[m->drop == I_POTION ? MS_FOUND_POTION : MS_FOUND_HIPOTION], F_MEDIUM, C_BLACK);
        if (st.bcur) draw_text(26, st.bdrop ? 37 : 27, "Level UP!", F_MEDIUM, C_BLACK);
    }
    if (st.bp == BP_INTRO && st.bt < 8) fade_planes(3 - st.bt / 3);    // flash in
}
