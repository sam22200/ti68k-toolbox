// Story events: each event is a C coroutine (protothread style). st.pc holds the line where it
// waits, so an event survives frames (and PC save/load) with no stack. Macros, one per line:
// SAY(t) / ASK(t) (Yes/No -> st.ans), WAIT(n), WALK(i, cx, cy) (actor i to a cell, -1 = hero),
// FADE(level). Actors move 1 px a frame; the hero and the NPCs share the hitbox convention.
#include "ffa.h"
#include "texts.h"

enum {
    EV_NONE, EV_SAY, EV_STORY1, EV_BED, EV_CHEST, EV_STORY2, EV_STORY5,
    NEV
};

#define BEGIN switch (st.pc) { case 0:
#define END } st.pc = 0; return 1
#define EXIT do { st.pc = 0; return 1; } while (0)
#define YIELD_UNTIL(c) do { st.pc = __LINE__; case __LINE__: if (!(c)) return 0; } while (0)
#define SAY(t) do { dialog_open(t, 0); YIELD_UNTIL(!st.dlg_on); } while (0)
#define ASK(t) do { dialog_open(t, 1); YIELD_UNTIL(!st.dlg_on); } while (0)
#define WAIT(n) do { st.timer = (n); YIELD_UNTIL(!--st.timer); } while (0)
#define WALK(i, cx, cy) do { actor_to(i, cx, cy); YIELD_UNTIL(actor_idle(i)); } while (0)
#define WALK2(i, x1, y1, j, x2, y2) do { actor_to(i, x1, y1); actor_to(j, x2, y2); YIELD_UNTIL(actor_idle(i) && actor_idle(j)); } while (0)
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

static u8 (*const events[NEV])(void) = { 0, ev_say, ev_story1, ev_bed, ev_chest, ev_story2, ev_story5 };

static u8 start(u8 ev, s16 arg)
{
    st.ev = ev;
    st.arg = arg;
    st.pc = 0;
    st.mode = M_SCRIPT;
    return 1;
}

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
