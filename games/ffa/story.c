// Story events: each event is a C coroutine (protothread style). st.pc holds the line where it
// waits, so an event survives frames (and PC save/load) with no stack. Macros, one per line:
// SAY(t) / ASK(t) (Yes/No -> st.ans), WAIT(n), WALK(i, cx, cy) (actor i to a cell, -1 = hero),
// FADE(level). Actors move 1 px a frame; the hero and the NPCs share the hitbox convention.
#include "ffa.h"
#include "texts.h"

enum {
    EV_NONE, EV_SAY, EV_STORY1, EV_BED, EV_CHEST, EV_STORY2, EV_STORY5, EV_SHOP, EV_OLEN, EV_FIND, EV_NOTICE, EV_SWITCH, EV_TRAP, EV_RIDDLE, EV_BOSS,
    NEV
};

#define BEGIN switch (st.pc) { case 0:
#define END } st.pc = 0; return 1
#define EXIT do { st.pc = 0; return 1; } while (0)
#define YIELD_UNTIL(c) do { st.pc = __LINE__; case __LINE__: if (!(c)) return 0; } while (0)
#define YIELD do { st.pc = __LINE__; return 0; case __LINE__:; } while (0)
#define SAY(t) do { dialog_open(t, 0); YIELD_UNTIL(!st.dlg_on); } while (0)
#define ASK(t) do { dialog_open(t, 1); YIELD_UNTIL(!st.dlg_on); } while (0)
#define WAIT(n) do { st.timer = (n); YIELD_UNTIL(!--st.timer); } while (0)
#define WALK(i, cx, cy) do { actor_to(i, cx, cy); YIELD_UNTIL(actor_idle(i)); } while (0)
#define WALK2(i, x1, y1, j, x2, y2) do { actor_to(i, x1, y1); actor_to(j, x2, y2); YIELD_UNTIL(actor_idle(i) && actor_idle(j)); } while (0)
#define BATTLE(n) do { battle_start(n); YIELD_UNTIL(st.mode == M_SCRIPT); } while (0)
#define FADE(l) do { st.arg = (l); st.timer = 0; YIELD_UNTIL(fade_to()); } while (0)

// ---------------------------------------------------------------- actors
static void actor_to(s8 i, s16 cx, s16 cy)
{
    if (i < 0) { st.npc[0].tx = cx * TILE + HB_X0; st.npc[0].ty = cy * TILE + HB_Y0; st.hwalk = 1; return; }
    st.npc[i].tx = cx * TILE + HB_X0;
    st.npc[i].ty = cy * TILE + HB_Y0;
}

static u8 actor_idle(s8 i)
{
    if (i < 0) return !st.hwalk;
    return st.npc[i].x == st.npc[i].tx && st.npc[i].y == st.npc[i].ty;
}

static void step_one(s16 *x, s16 *y, s16 tx, s16 ty, u8 *dir, u8 *anim)
{
    if (*x == tx && *y == ty) { *anim = 0; return; }   // vertical first, then horizontal
    if (*y < ty) { (*y)++; *dir = DIR_DOWN; }
    else if (*y > ty) { (*y)--; *dir = DIR_UP; }
    else if (*x < tx) { (*x)++; *dir = DIR_RIGHT; }
    else { (*x)--; *dir = DIR_LEFT; }
    (*anim)++;
}

void npc_step(void)
{
    u8 i;
    for (i = 1; i < NNPC; i++)
        if (st.npc[i].on) step_one(&st.npc[i].x, &st.npc[i].y, st.npc[i].tx, st.npc[i].ty, &st.npc[i].dir, &st.npc[i].anim);
    if (st.hwalk) {                          // scripted hero walk (slot 0 holds the target)
        step_one(&st.x, &st.y, st.npc[0].tx, st.npc[0].ty, &st.dir, &st.anim);
        if (st.x == st.npc[0].tx && st.y == st.npc[0].ty) st.hwalk = 0;
    }
}

static void npc_put(u8 i, u8 spr, s16 cx, s16 cy, u8 dir)
{
    Npc *n = &st.npc[i];
    n->on = 1; n->spr = spr; n->dir = dir; n->anim = 0;
    n->x = n->tx = cx * TILE + HB_X0;
    n->y = n->ty = cy * TILE + HB_Y0;
}

static u8 fade_to(void)                      // one fade step every FADE_FRAMES frames
{
    if (st.fade == (u8)st.arg) return 1;
    if (++st.timer < FADE_FRAMES) return 0;
    st.timer = 0;
    st.fade += st.fade < (u8)st.arg ? 1 : -1;
    return st.fade == (u8)st.arg;
}

void story_room(void)                        // NPCs standing in the room
{
    u8 i;
    for (i = 0; i < NNPC; i++) st.npc[i].on = 0;
    st.hwalk = 0;
    switch (rooms[st.room].id) {
    case 6: npc_put(1, SPR_KING, 6, 4, DIR_DOWN); break;          // Edouard, cell -9
    case 5: npc_put(1, SPR_SOLDIER, 4, 4, DIR_DOWN);              // cell -7.1
            npc_put(2, SPR_SELLER, 12, 5, DIR_DOWN); break;       // cell -12
    case 18: npc_put(1, SPR_OLEN, 4, 4, DIR_DOWN);                // cell -10
             npc_put(2, SPR_JESS, 10, 3, DIR_DOWN); break;        // cell -11
    }
}

// ---------------------------------------------------------------- events
static u8 ev_say(void)
{
    BEGIN;
    SAY((u8)st.arg);
    END;
}

static u8 ev_story1(void)                    // room 8: Edouard comes down the stairs (story1)
{
    BEGIN;
    WALK(-1, 6, 2);                          // the hero steps aside (original: a = 45)
    st.dir = DIR_LEFT;
    npc_put(1, SPR_KING, 5, 1, DIR_DOWN);
    WALK(1, 5, 2);
    st.npc[1].dir = DIR_RIGHT;
    WAIT(10);
    SAY(T_STORY1);
    WAIT(10);
    WALK(1, 5, 1);
    st.npc[1].on = 0;
    st.flag[9] = 1;
    END;
}

static u8 ev_bed(void)                       // room 8 bed: Sleep? (full heal, free)
{
    BEGIN;
    ASK(T_SLEEP);
    if (!st.ans) EXIT;
    FADE(3);
    st.hero.hp = st.hero.hpm;
    st.hero.mp = st.hero.mpm;
    WAIT(20);
    FADE(0);
    END;
}

static u8 ev_chest(void)                     // one-shot find: st.arg = flag | item << 8
{
    BEGIN;
    SAY(T_FOUND_POTION);
    st.flag[st.arg & 0xFF] = 1;
    st.item[st.arg >> 8]++;
    END;
}

static u8 ev_story2(void)                    // room 6: "Father, I don't see where my sword is"
{
    BEGIN;
    WALK(-1, 13, 3);                         // original: b = 18, a = 108
    st.dir = DIR_LEFT;
    SAY(T_SWORD_Q);
    SAY(T_SWORD_A);
    st.flag[10] = 1;
    END;
}

static u8 ev_story5(void)                    // room 6: the knighting ceremony
{
    BEGIN;
    WALK(-1, 6, 5);                          // before the Lord (original: b = 36, a = 45)
    st.dir = DIR_UP;
    SAY(T_C1);
    SAY(T_C2);
    npc_put(2, SPR_OLEN, 8, 9, DIR_UP);
    npc_put(3, SPR_JESS, 10, 9, DIR_UP);
    WALK2(2, 8, 7, 3, 10, 7);
    SAY(T_C3);
    npc_put(4, SPR_LARC, 9, 9, DIR_UP);
    WALK(4, 9, 6);
    SAY(T_C4);
    SAY(T_C5);
    SAY(T_C6);
    SAY(T_C7);
    WAIT(16);
    SAY(T_C8);
    SAY(T_C9);
    SAY(T_C10);
    SAY(T_C11);
    SAY(T_C12);
    SAY(T_C13);
    WALK2(2, 8, 9, 3, 10, 9);                // Olen and Jess leave
    st.npc[2].on = st.npc[3].on = 0;
    SAY(T_C14);
    SAY(T_C15);
    WALK(4, 9, 9);                           // Larc leaves
    st.npc[4].on = 0;
    SAY(T_C16);
    npc_put(5, SPR_VILLAGER, 9, 9, DIR_UP);
    WALK(5, 9, 7);
    SAY(T_C17);
    SAY(T_C18);
    SAY(T_C19);
    WALK(5, 9, 9);
    st.npc[5].on = 0;
    SAY(T_C20);
    SAY(T_C21);
    SAY(T_C22);
    SAY(T_C23);
    st.flag[7] = st.flag[8] = 1;
    st.dir = DIR_DOWN;
    END;
}

// Potion seller (shop1): Potion 50 g, one per press of 2nd; ESC or shift leaves.
#define PRICE_POTION 50
static u8 shop_input(void)
{
    if (input_pressed(K_ESC | K_B)) return 1;
    if (input_pressed(K_A | K_ENTER)) {
        if (st.hero.gils < PRICE_POTION) { st.arg = 1; return 1; }
        st.hero.gils -= PRICE_POTION;
        if (st.item[I_POTION] < 99) st.item[I_POTION]++;
    }
    return 0;
}

static u8 ev_shop(void)
{
    BEGIN;
    SAY(T_SELLER);
    st.shop = 1;
    st.arg = 0;
    YIELD;                                   // the key that closed the greeting is not a buy
    YIELD_UNTIL(shop_input());
    st.shop = 0;
    if (st.arg) SAY(T_SHOP_POOR);
    SAY(T_SHOP_BYE);
    END;
}

static void num(s16 x, s16 y, u16 v)         // right-aligned number ending at x
{
    char t[6];
    u8 k = 5;
    t[5] = 0;
    do t[--k] = '0' + v % 10; while ((v /= 10) && k);
    draw_text(x - (5 - k) * 6, y, t + k, F_MEDIUM, C_BLACK);
}

static void numentry_render(void)
{
    u8 i;
    char c[2] = { 0, 0 };
    draw_rect(52, 20, 56, 30, C_BLACK);
    draw_rect(53, 21, 54, 28, C_WHITE);
    for (i = 0; i < 4; i++) {
        c[0] = '0' + st.digit[i];
        if (i == st.dpos) { draw_rect(60 + i * 11, 26, 9, 12, C_LGRAY); draw_rect(60 + i * 11, 39, 9, 2, C_BLACK); }
        draw_text(62 + i * 11, 28, c, F_MEDIUM, C_BLACK);
    }
}

void shop_render(void)
{
    if (st.shop == 2) { numentry_render(); return; }
    draw_rect(20, 14, 120, 44, C_BLACK);
    draw_rect(21, 15, 118, 42, C_DGRAY);
    draw_rect(22, 16, 116, 40, C_WHITE);
    draw_text(26, 19, "Potion", F_MEDIUM, C_BLACK);
    num(122, 19, PRICE_POTION);
    draw_text(128, 19, "g", F_MEDIUM, C_BLACK);
    draw_text(26, 31, "Owned", F_MEDIUM, C_BLACK);
    num(122, 31, st.item[I_POTION]);
    draw_rect(22, 42, 116, 1, C_LGRAY);
    draw_text(26, 45, "Gils", F_MEDIUM, C_BLACK);
    num(134, 45, st.hero.gils);
}

static u8 ev_olen(void)                      // room 18: Olen (D4): the Dungeon Key, then Cure
{
    BEGIN;
    if (!st.flag[1]) {
        SAY(T_OLEN_KEY);
        SAY(T_FOUND_DKEY);
        st.flag[1] = 1;
    } else if (st.own[A_SWORD] && !st.mat[MAT_CURE]) {
        SAY(T_OLEN_CURE);
        SAY(T_FOUND_CURE);
        st.mat[MAT_CURE] = 1;
        auto_equip();
    } else if (st.mat[MAT_CURE]) {
        SAY(T_OLEN_LUCK);
    } else {
        SAY(T_OLEN_COURAGE);
    }
    END;
}

// One-shot finds (chests, corpses): optional first line, the "Found" line, what it gives.
enum { G_FLAG, G_OWN, G_MAT };
typedef struct { u8 pre, text, kind, idx, item; } Find;
enum { F_LKEY, F_POTION11, F_ETHER12, F_ANTIDOTE13, F_FIRE, F_WRIST, F_SWORD, F_BANGLE, NFIND };
static const Find finds[NFIND] = {
    { T_HOLDS_KEY, T_FOUND_LKEY, G_FLAG, 2, 0xFF },
    { 0xFF, T_FOUND_POTION, G_FLAG, 43, I_POTION },
    { 0xFF, T_FOUND_ETHER, G_FLAG, 44, I_ETHER },
    { 0xFF, T_FOUND_ANTIDOTE, G_FLAG, 45, I_ANTIDOTE },
    { 0xFF, T_FOUND_FIRE, G_MAT, MAT_FIRE, 0xFF },
    { T_HOLDS_SOMETHING, T_FOUND_WRIST, G_OWN, A_WRIST, 0xFF },
    { 0xFF, T_FOUND_SWORD, G_OWN, A_SWORD, 0xFF },
    { 0xFF, T_FOUND_BANGLE, G_OWN, A_BANGLE, 0xFF },
};

void auto_equip(void)                        // stand-in for the APPS menu: wear what is owned
{
    u8 k = 0;
    st.hero.weapon = st.own[A_SWORD] ? A_SWORD : 0;
    st.hero.armor = st.own[A_BANGLE] ? A_BANGLE : 0;
    st.hero.acc[0] = st.own[A_WRIST] ? A_WRIST : 0;
    st.hero.slot[0] = st.hero.slot[1] = 0;
    if (st.hero.weapon) {                    // the Buster Sword has 2 slots
        if (st.mat[MAT_FIRE]) st.hero.slot[k++] = MAT_FIRE;
        if (st.mat[MAT_CURE]) st.hero.slot[k++] = MAT_CURE;
    }
}

static u8 found(u8 f)
{
    const Find *d = &finds[f];
    return d->kind == G_FLAG ? st.flag[d->idx] : d->kind == G_OWN ? st.own[d->idx] : st.mat[d->idx];
}

static u8 ev_find(void)
{
    BEGIN;
    if (finds[st.arg].pre != 0xFF) SAY(finds[st.arg].pre);
    SAY(finds[st.arg].text);
    {
        const Find *d = &finds[st.arg];
        if (d->kind == G_FLAG) st.flag[d->idx] = 1;
        else if (d->kind == G_OWN) st.own[d->idx] = 1;
        else st.mat[d->idx] = 1;
        if (d->item != 0xFF) st.item[d->item]++;
        if (st.arg == F_SWORD) st.flag[8] = 0;   // Olen's room shuts until the ceremony
        auto_equip();
    }
    END;
}

static u8 ev_notice(void)                    // room 12: the number is re-rolled at each reading
{
    BEGIN;
    st.devi = 5000 + rt_rand() % 100 + 1;
    st.num = st.devi;
    SAY(T_NOTICE);
    END;
}

static u8 ev_switch(void)                    // room 13: toggles clef[17] (door 11 -> 16)
{
    BEGIN;
    ASK(T_SWITCH_Q);
    if (!st.ans) EXIT;
    st.flag[17] ^= 1;
    SAY(T_LOCK_NOISE);
    END;
}

static u8 ev_trap(void)                      // room 13: the trap chest, a random fight each time
{
    BEGIN;
    SAY(T_TRAP);
    BATTLE(0);
    END;
}

// Number entry: 4 digits, up/down change, left/right move, 2nd confirms, ESC cancels (st.ans 0)
static u8 num_input(void)
{
    if (input_pressed(K_ESC)) { st.ans = 0; return 1; }
    if (input_pressed(K_A | K_ENTER)) { st.ans = 1; return 1; }
    if (input_pressed(K_LEFT) && st.dpos) st.dpos--;
    if (input_pressed(K_RIGHT) && st.dpos < 3) st.dpos++;
    if (input_pressed(K_UP)) st.digit[st.dpos] = st.digit[st.dpos] == 9 ? 0 : st.digit[st.dpos] + 1;
    if (input_pressed(K_DOWN)) st.digit[st.dpos] = st.digit[st.dpos] ? st.digit[st.dpos] - 1 : 9;
    return 0;
}

static u8 ev_riddle(void)                    // room 13: story3, the answer is devi - 3000
{
    BEGIN;
    SAY(T_RIDDLE);
    st.digit[0] = st.digit[1] = st.digit[2] = st.digit[3] = 0;
    st.dpos = 0;
    st.shop = 2;
    YIELD;
    YIELD_UNTIL(num_input());
    st.shop = 0;
    if (!st.ans) EXIT;
    if (st.devi && st.digit[0] * 1000 + st.digit[1] * 100 + st.digit[2] * 10 + st.digit[3] == st.devi - 3000) {
        SAY(T_RIGHT);
        st.flag[3] = 1;
    } else {
        SAY(T_FALSE);
    }
    END;
}

static u8 ev_boss(void)                      // room 16: story4, the prisoner guarding the sword
{
    BEGIN;
    npc_put(1, SPR_PRISONER, 9, 8, DIR_UP);
    WAIT(20);
    SAY(T_BOSS1);
    st.dir = DIR_DOWN;
    SAY(T_BOSS2);
    WALK(1, (st.x - HB_X0 + 8) >> 4, 8);     // to the hero's column, then up to him
    WALK(1, (st.x - HB_X0 + 8) >> 4, ((st.y - HB_Y0 + 8) >> 4) + 1);
    st.npc[1].dir = DIR_UP;
    SAY(T_BOSS3);
    BATTLE(3);
    st.npc[1].on = 0;
    SAY(T_FOUND_CELL2);
    SAY(T_CELL2);
    st.flag[5] = 1;
    END;
}

static u8 (*const events[NEV])(void) = { 0, ev_say, ev_story1, ev_bed, ev_chest, ev_story2, ev_story5, ev_shop, ev_olen,
                                         ev_find, ev_notice, ev_switch, ev_trap, ev_riddle, ev_boss };


static u8 start(u8 ev, s16 arg)
{
    st.ev = ev;
    st.arg = arg;
    st.pc = 0;
    st.mode = M_SCRIPT;
    return 1;
}

static u8 find(u8 f) { return found(f) ? 0 : start(EV_FIND, f); }

u8 story_trigger(s16 p, u8 examine)          // p = original value x 10
{
    u8 id = rooms[st.room].id;
    switch (id) {
    case 8:
        if (p == 5000) return st.flag[9] ? 0 : start(EV_STORY1, 0);
        if (p == -25) return start(EV_BED, 0);
        if (p == -35) return start(EV_SAY, T_PLAQUE8);
        if (p == -30) return start(EV_SAY, T_LARC_DOOR);
        if (p == -190) return st.flag[40] ? 0 : start(EV_CHEST, 40 | I_POTION << 8);
        break;
    case 6:
        if (p == 5010) return st.flag[10] ? 0 : start(EV_STORY2, 0);
        if (p == -90) {                      // Edouard (D3); silent once the key is taken
            if (!st.flag[11]) return start(EV_SAY, T_ED_SWORD);
            if (!st.flag[1]) return start(EV_SAY, T_ED_CLOSED);
            return 1;
        }
        if (p == 5050) return st.own[A_SWORD] && !st.flag[8] && !st.flag[7] ? start(EV_STORY5, 0) : 0;
        break;
    case 5:
        if (p == -71) return start(EV_SAY, T_SOLDIER5);
        if (p == -120) return start(EV_SHOP, 0);
        if (p == 5140) return 0;             // "Finally you come back": late game (clef[56])
        break;
    case 7:
        if (p == 5040) { st.flag[11] = 1; return 0; }   // silent: changes Edouard's line
        if (p == -20) return start(EV_SAY, T_BOOK_EXCALIBUR);
        if (p == -15) return start(EV_SAY, T_BOOK_LANGUAGE);
        if (p == -40) return start(EV_SAY, T_BOOK_CLOUD);
        if (p == -30) return start(EV_SAY, T_BOOK_WAR);
        break;
    case 18:
        if (p == -100) return start(EV_OLEN, 0);
        if (p == -110) return st.own[A_SWORD] ? 1 : start(EV_SAY, T_JESS);   // silent after the sword
        if (p == -85) return start(EV_SAY, T_CARROTS);
        break;
    case 10: if (p == -180) return find(F_LKEY); break;
    case 11: if (p == -180) return find(F_POTION11); break;
    case 12:
        if (p == -60) return start(EV_NOTICE, 0);
        if (p == -180) return find(F_ETHER12);
        break;
    case 13:
        if (p == -40) return start(EV_SWITCH, 0);
        if (p == -20) return start(EV_TRAP, 0);
        if (p == -180) return find(F_ANTIDOTE13);
        if (p == 5020) return st.flag[3] ? 0 : start(EV_RIDDLE, 0);
        break;
    case 14:
        if (p == -180) return find(F_FIRE);
        if (p == -30) return start(EV_SAY, T_CELLS_SIGN);
        break;
    case 15:
        if (p == -70) return start(EV_SAY, T_PLAQUE15);
        if (p == -180) return find(F_WRIST);
        if (p == -80) return 1;              // no handler in the original: nothing
        break;
    case 16:
        if (p == 5030) return st.flag[5] ? 0 : start(EV_BOSS, 0);
        if (p == -180) return find(F_SWORD);
        if (p == -190) return start(EV_SAY, T_GOLD_SEAL);
        break;
    case 17: if (p == -180) return find(F_BANGLE); break;
    case 4: if (p == -74) return start(EV_SAY, T_MESSAGE4); break;
    }
    return 0;
}

u8 story_run(void)
{
    u8 done;
    if (st.dlg_on) dialog_update();
    npc_step();
    if (st.dlg_on) return 0;
    done = events[st.ev]();
    if (done) { st.ev = 0; st.arg = 0; st.mode = M_WALK; }
    return done;
}
