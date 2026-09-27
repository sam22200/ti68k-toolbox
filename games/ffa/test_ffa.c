// FFA unit and integration tests: sw_step + asserts, no window.
#include <stdio.h>
#include "../../runtime/platform-sw/rt_sw.h"
#include "ffa.h"
#include "texts.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static void hold(u32 k, u16 n) { while (n--) sw_step(k); }
static u8 idx(u8 id) { return room_index[id]; }
static void put(u8 id, s16 x, s16 y) { st.room = idx(id); st.x = x; st.y = y; st.mode = M_WALK; }
// Run the current event to its end, pressing 2nd to read; returns the number of texts shown.
static u16 run_script(void)
{
    u16 k, n = 0;
    u8 last = 0xFF;
    for (k = 0; k < 5000 && st.mode == M_SCRIPT; k++) {
        if (st.dlg_on && st.dlg_text != last) { last = st.dlg_text; n++; }
        if (!st.dlg_on) last = 0xFF;
        sw_step(k % 6 == 0 ? K_A : 0);
    }
    return n;
}

// Examine what is in front of the hero; returns the first text shown (0xFF = none).
static u8 talk(void)
{
    u8 t = 0xFF;
    u16 k;
    sw_step(0);
    sw_step(K_A);
    for (k = 0; k < 400 && st.mode != M_WALK; k++) {
        if (st.dlg_on && t == 0xFF) t = st.dlg_text;
        sw_step(k % 6 == 0 ? K_A : 0);
    }
    return t;
}

// Fight: Attack whenever the menu opens (Braver when the limit is full); returns frames used.
static u16 fight(void)
{
    u16 k;
    for (k = 0; k < 6000 && st.mode == M_BATTLE; k++) sw_step(k & 1 ? 0 : K_A);
    return k;
}

static void strong(void)                     // a level-20-like hero for the scripted fights
{
    st.hero.lv = 20; st.hero.hp = st.hero.hpm = 400; st.hero.mp = st.hero.mpm = 60;
    st.hero.st[S_STR] = 30 * STAT; st.hero.st[S_DEF] = 20 * STAT; st.hero.st[S_SPD] = 25 * STAT;
}

static u8 cell_at(u8 room, s16 cx, s16 cy) { return rooms[room].cell[cy * rooms[room].w + cx]; }

// Every door of every room: stand on a free neighbour, push into it, end up in the right room
// on a cell that is not a wall (every arrival lands on floor, a trigger or the way back).
static void doors_all(void)
{
    u8 i, k, ok, tested = 0;
    s16 cx, cy;
    static const s8 nx[4] = { 0, 0, -1, 1 }, ny[4] = { 1, -1, 0, 0 };
    static const u32 push[4] = { K_UP, K_DOWN, K_RIGHT, K_LEFT };
    for (i = 0; i < nroom; i++) {
        const Room *r = &rooms[i];
        for (cy = 0; cy < r->h; cy++)
            for (cx = 0; cx < r->w; cx++) {
                u8 c = cell_at(i, cx, cy), n;
                const Door *d;
                if (!(c & CELL_DOOR)) continue;
                d = &r->door[c & 0x3F];
                for (n = 0; n < 4; n++) {        // a floor neighbour below/above/left/right
                    s16 fx = cx + nx[n], fy = cy + ny[n];
                    if (fx < 0 || fy < 0 || fx >= r->w || fy >= r->h || cell_at(i, fx, fy) != CELL_FLOOR) continue;
                    sw_init(10);
                    for (k = 0; k < NFLAG; k++) st.flag[k] = 1;   // every key
                    put(r->id, fx * TILE + HB_X0, fy * TILE + HB_Y0);
                    st.cell_in = cell_at(i, fx, fy);
                    for (k = 0; k < 60 && st.room == i && st.mode != M_END; k++) sw_step(push[n]);
                    hold(0, 20);
                    tested++;
                    if (d->dest == 0xFF || d->key == KEY_NEVER) { CHECK(st.room == i); break; }
                    if (d->dest == ROOM_OUT) { CHECK(st.mode == M_END); break; }
                    ok = st.room == d->dest || (d->dest == i && st.mode == M_WALK);
                    if (!ok) printf("door room %u cell %d,%d -> %u: now room %u mode %u\n", r->id, cx, cy,
                                    rooms[d->dest].id, rooms[st.room].id, st.mode);
                    CHECK(ok);
                    if (world_cell(st.x + HB_W / 2, st.y + HB_H / 2) == CELL_WALL)
                        printf("wall arrival: room %u cell %d,%d -> room %u at %d,%d\n", r->id, cx, cy, rooms[st.room].id, st.x >> 4, st.y >> 4);
                    CHECK(world_cell(st.x + HB_W / 2, st.y + HB_H / 2) != CELL_WALL);
                    break;
                }
            }
    }
    printf("doors: %u door cells pushed\n", tested);
    CHECK(tested >= 20);
}

int main(void)
{
    s16 x0;
    u16 k;
    u8 seen = 0, npc_seen = 0;

    // new game: room 8, original a = 27, b = 27 (cell 4, 4), first-level stats
    sw_init(10);
    CHECK(rooms[st.room].id == 8 && st.x == 4 * TILE + HB_X0 && st.y == 4 * TILE + HB_Y0);
    CHECK(st.hero.hp == 80 && st.hero.mpm == 15 && st.hero.gils == 100 && st.flag[6] && st.flag[8]);

    // walking: 1.5 px per frame, 3 when running (shift / K_B)
    x0 = st.x;
    hold(K_RIGHT, 10);
    CHECK(st.x - x0 == 15 && st.dir == DIR_RIGHT);
    x0 = st.x;
    hold(K_RIGHT | K_B, 4);
    CHECK(st.x - x0 == 12);

    // walls: pushing left for long stops flush against the wall, inside the room
    sw_init(10);
    hold(K_LEFT, 60);
    CHECK(world_cell(st.x - 1, st.y) == CELL_WALL && !world_solid(world_cell(st.x, st.y)));

    // corner sliding: room 6, cell (2,3) is a wall and (3,3) floor; the hitbox overlaps
    // column 2 by 4 px and goes up: it slides right into column 3 instead of stopping
    sw_init(10);
    put(6, 3 * TILE - 4, 4 * TILE + HB_Y0);
    CHECK(world_solid(world_cell(2 * TILE + 8, 3 * TILE + 8)) && !world_solid(world_cell(3 * TILE + 8, 3 * TILE + 8)));
    hold(K_UP, 16);
    CHECK(st.y < 4 * TILE && st.x >= 3 * TILE);
    // too much overlap (8 px): no slide, blocked
    put(6, 3 * TILE - 8, 4 * TILE + HB_Y0);
    hold(K_UP, 16);
    CHECK(st.y >= 4 * TILE && st.x == 3 * TILE - 8);

    // story1: stepping on 500 under the stairs brings Edouard down; the hero steps aside
    sw_init(10);
    hold(K_RIGHT, 11);                       // x = 4 * 16 + 3 + 16 (column 5, under the stairs)
    CHECK(st.x == 5 * TILE + HB_X0 + 1);
    for (k = 0; k < 40 && st.mode == M_WALK; k++) sw_step(K_UP);
    CHECK(st.mode == M_SCRIPT && !st.flag[9]);
    for (k = 0; k < 600 && st.mode == M_SCRIPT; k++) {
        if (st.dlg_on && st.dlg_text == T_STORY1) seen = 1;
        if (st.npc[1].on) npc_seen = 1;
        sw_step(k % 8 == 0 ? K_A : 0);       // read the dialogue
    }
    CHECK(st.mode == M_WALK && st.flag[9] && seen && npc_seen && !st.npc[1].on);
    CHECK(st.x == 6 * TILE + HB_X0 && st.y == 2 * TILE + HB_Y0 && st.dir == DIR_LEFT);
    for (k = 0; k < 100; k++) sw_step(K_UP); // the 500 cell does not trigger twice
    hold(0, 1);

    // door with a fade: room 8 -> room 6 (stairs); arrival a = 117, b = 18 -> cell (14, 3)
    hold(K_LEFT, 11);
    for (k = 0; k < 40 && st.mode != M_FADE_OUT; k++) sw_step(K_UP);
    CHECK(st.mode == M_FADE_OUT && rooms[st.room].id == 8);
    hold(0, FADE_FRAMES * 2);
    CHECK(sw_level(80, 50) < 3);             // lighter while fading
    hold(0, FADE_FRAMES * 8);
    CHECK(rooms[st.room].id == 6 && (st.x >> 4) >= 13 && (st.y >> 4) == 3);

    // story2 runs on arrival (the 501 cell), Edouard stands at his -9 cell
    CHECK(st.mode == M_SCRIPT && st.npc[1].on && st.npc[1].spr == SPR_KING);
    run_script();
    CHECK(st.mode == M_WALK && st.flag[10] && st.x == 13 * TILE + HB_X0 && st.y == 3 * TILE + HB_Y0);
    for (k = 0; k < 30; k++) sw_step(k & 1 ? K_RIGHT : K_LEFT);   // no second story2
    CHECK(st.mode == M_WALK);

    // Edouard's lines follow the flags; silent once the dungeon key is taken
    sw_init(1);
    put(6, 7 * TILE + HB_X0, 4 * TILE + HB_Y0);
    st.dir = DIR_LEFT;
    CHECK(talk() == T_ED_SWORD);
    st.flag[11] = 1;
    CHECK(talk() == T_ED_CLOSED);
    st.flag[1] = 1;
    CHECK(talk() == 0xFF && st.mode == M_WALK);

    // the ceremony (story5) needs the sword; injected at checkpoint 7 (in the courtyard)
    sw_init(2);
    put(6, 9 * TILE + HB_X0, 7 * TILE + HB_Y0);
    hold(K_DOWN, 12);
    CHECK(st.mode == M_WALK);                // no sword: nothing
    sw_init(7);
    CHECK(rooms[st.room].id == 5 && st.own[A_SWORD] && !st.flag[8]);
    for (k = 0; k < 40 && st.mode != M_SCRIPT; k++) sw_step(k < 12 ? K_UP : 0);
    CHECK(rooms[st.room].id == 6 && st.mode == M_SCRIPT);
    k = run_script();
    CHECK(st.mode == M_WALK && st.flag[7] && st.flag[8] && k >= 23);
    CHECK(st.x == 6 * TILE + HB_X0 && st.y == 5 * TILE + HB_Y0);
    CHECK(st.npc[1].on && !st.npc[2].on && !st.npc[3].on && !st.npc[4].on && !st.npc[5].on);

    // bed: Yes heals, No does not; the potion on the desk is found once
    sw_init(10);
    st.hero.hp = 3; st.hero.mp = 1;
    put(8, 4 * TILE + HB_X0, 2 * TILE + HB_Y0);
    st.dir = DIR_LEFT;
    sw_step(K_A);
    CHECK(st.mode == M_SCRIPT);
    for (k = 0; k < 60 && !(st.dlg_on && st.dlg_shown >= 6); k++) sw_step(0);
    sw_step(K_DOWN); sw_step(K_A);           // No
    hold(0, 5);
    CHECK(st.mode == M_WALK && st.hero.hp == 3);
    sw_step(K_A);
    for (k = 0; k < 60 && !(st.dlg_on && st.dlg_shown >= 6); k++) sw_step(0);
    sw_step(K_A);                            // Yes
    for (k = 0; k < 200 && st.mode == M_SCRIPT; k++) sw_step(0);
    CHECK(st.mode == M_WALK && st.hero.hp == st.hero.hpm && st.hero.mp == st.hero.mpm && !st.fade);
    put(8, 7 * TILE + HB_X0, 6 * TILE + HB_Y0);
    st.dir = DIR_DOWN;
    sw_step(K_A);
    for (k = 0; k < 200 && st.mode == M_SCRIPT; k++) sw_step(k % 8 == 0 ? K_A : 0);
    CHECK(st.item[I_POTION] == 4 && st.flag[40]);
    sw_step(K_A);
    hold(0, 3);
    CHECK(st.mode != M_SCRIPT && st.item[I_POTION] == 4);

    // room 5: the soldier talks, the seller sells Potions at 50 g until the gils run out
    sw_init(1);
    put(5, 4 * TILE + HB_X0, 5 * TILE + HB_Y0);
    story_room();
    st.dir = DIR_UP;
    CHECK(st.npc[1].spr == SPR_SOLDIER && st.npc[2].spr == SPR_SELLER);
    CHECK(talk() == T_SOLDIER5);
    put(5, 13 * TILE + HB_X0, 5 * TILE + HB_Y0);
    st.dir = DIR_LEFT;
    sw_step(0); sw_step(K_A);
    for (k = 0; k < 200 && !st.shop; k++) sw_step(k % 6 == 0 ? K_A : 0);
    CHECK(st.shop && st.hero.gils == 100 && st.item[I_POTION] == 3);
    sw_step(0); sw_step(K_A); sw_step(0); sw_step(K_A);
    CHECK(st.hero.gils == 0 && st.item[I_POTION] == 5);
    sw_step(0); sw_step(K_A);               // no gils left: "Sorry, not enough gils."
    CHECK(!st.shop && st.hero.gils == 0 && st.item[I_POTION] == 5);
    k = run_script();
    CHECK(st.mode == M_WALK && k == 2);
    // the exit to room 4 needs the ceremony (clef[7]); Olen's door is shut after the sword
    put(5, 9 * TILE + HB_X0, 8 * TILE + HB_Y0);
    hold(K_DOWN, 20);
    CHECK(rooms[st.room].id == 5 && st.mode == M_TEXT && st.dlg_text == T_LOCKED);
    sw_init(7);
    put(5, 15 * TILE + HB_X0, 7 * TILE + HB_Y0);
    hold(K_UP, 20);
    CHECK(rooms[st.room].id == 5 && st.mode == M_TEXT && st.dlg_text == T_LOCKED);

    // room 7: the books; the 504 cell under the dungeon door sets clef[11] silently
    sw_init(1);
    put(7, 3 * TILE + HB_X0, 4 * TILE + HB_Y0);
    story_room();
    st.dir = DIR_UP;
    CHECK(talk() == T_BOOK_EXCALIBUR);
    put(7, 6 * TILE + HB_X0, 4 * TILE + HB_Y0);
    CHECK(talk() == T_BOOK_LANGUAGE);
    put(7, 3 * TILE + HB_X0, 7 * TILE + HB_Y0);
    CHECK(talk() == T_BOOK_CLOUD);
    put(7, 7 * TILE + HB_X0, 7 * TILE + HB_Y0);
    CHECK(talk() == T_BOOK_WAR);
    put(7, 9 * TILE + HB_X0, 3 * TILE + HB_Y0);
    st.cell_in = 0;
    hold(K_UP, 10);
    CHECK(st.flag[11] && st.mode == M_WALK);
    hold(K_UP, 20);                          // the dungeon door needs the Dungeon Key
    CHECK(st.mode == M_TEXT && st.dlg_text == T_LOCKED);

    // room 18: Olen gives the Dungeon Key, then encourages; Cure once the sword is found
    sw_init(1);
    put(18, 5 * TILE + HB_X0, 4 * TILE + HB_Y0);
    story_room();
    st.dir = DIR_LEFT;
    CHECK(st.npc[1].spr == SPR_OLEN && st.npc[2].spr == SPR_JESS);
    CHECK(talk() == T_OLEN_KEY && st.flag[1]);
    CHECK(talk() == T_OLEN_COURAGE);
    st.own[A_SWORD] = 1;
    CHECK(talk() == T_OLEN_CURE && st.mat[MAT_CURE]);
    CHECK(talk() == T_OLEN_LUCK);
    put(18, 10 * TILE + HB_X0, 4 * TILE + HB_Y0);
    st.dir = DIR_UP;
    CHECK(talk() == 0xFF);                   // Jess is silent after the sword
    st.own[A_SWORD] = 0;
    CHECK(talk() == T_JESS);
    put(18, 10 * TILE + HB_X0, 7 * TILE + HB_Y0);
    CHECK(talk() == T_CARROTS);

    // locked door: room 13 -> 14 needs clef[3] (the injured-number riddle)
    sw_init(113);
    for (k = 0; k < NFLAG; k++) st.flag[k] = 0;
    doors_all();

    // random encounters: the dungeon (room 10, frc 1.2) attacks within ~20 steps, the castle never
    sw_init(110);
    for (k = 0; k < 2000 && st.mode != M_BATTLE; k++) sw_step(k & 64 ? K_LEFT : K_RIGHT);
    CHECK(st.mode == M_BATTLE && st.steps <= 20 * 10 / 12 + 2);
    sw_init(10);
    for (k = 0; k < 1000; k++) sw_step(k & 32 ? K_LEFT : K_RIGHT);
    CHECK(st.mode == M_WALK && st.steps > 30);

    // battles: a random fight in room 10 is won by attacking; rewards and a new threshold
    sw_init(110);
    for (k = 0; k < 2000 && st.mode != M_BATTLE; k++) sw_step(k & 64 ? K_LEFT : K_RIGHT);
    CHECK(st.mode == M_BATTLE && (st.battle == 1 || st.battle == 2) && st.qu);
    {
        u16 exp0 = st.hero.exp, g0 = st.hero.gils, n = st.battle;
        k = fight();
        CHECK(st.mode == M_WALK && st.won && st.hero.hp > 0 && k < 3000);
        CHECK(st.hero.exp - exp0 == (n == 1 ? 85 : 125) && st.hero.gils - g0 == (n == 1 ? 50 : 70));
        CHECK(st.co >= 21 && st.co <= 36);
        printf("battle vs monster %u won in %u frames, hp %u/%u\n", n, k, st.hero.hp, st.hero.hpm);
    }
    // level ups follow the original table (Lv3 at 777 exp: hpm 96, mpm 17, expt 1224)
    sw_init(10);
    st.hero.exp = 777;
    hero_level_up();
    CHECK(st.hero.lv == 3 && st.hero.expt == 1224 && st.hero.hpm == 96 && st.hero.mpm == 17);
    CHECK(st.hero.st[S_STR] == 10 * STAT + STAT / 4 + 2 * 10 + 2 * STAT / 4);   // speci Strength
    // a weak hero against the boss: Game Over, then 2nd starts a new game
    sw_init(6);
    st.flag[5] = 0;
    st.hero.hp = 10;
    battle_start(3);
    fight();
    CHECK(st.mode == M_GAMEOVER);
    sw_step(0); sw_step(K_A);
    CHECK(st.mode == M_TITLE);

    // title: New Game, growth stat Magic, name "Bo", the intro, then the bedroom fading in
    sw_init(0);
    CHECK(st.mode == M_TITLE && st.tstep == 0);
    for (k = 0; k < 20; k++) sw_step(0);
    sw_step(K_A);
    for (k = 0; k < 20; k++) sw_step(0);
    CHECK(st.tstep == 1);
    sw_step(K_DOWN); sw_step(0); sw_step(K_A);
    for (k = 0; k < 20; k++) sw_step(0);
    CHECK(st.tstep == 2);
    {
        u8 i;
        for (i = 0; i < 25; i++) { sw_step(K_DOWN); sw_step(0); }   // 'A' (1) - 25 -> 'c'...
        st.name[0] = 'B'; st.name[1] = 'o'; st.name[2] = ' '; st.name[3] = 0;
        sw_step(K_A);
        for (i = 0; i < 6 && st.mode == M_TITLE; i++) { hold(0, 20); sw_step(K_A); }
    }
    for (k = 0; k < 40 && st.mode != M_WALK; k++) sw_step(0);
    CHECK(st.mode == M_WALK && rooms[st.room].id == 8 && st.hero.hp == 80);
    CHECK(st.name[0] == 'B' && st.name[1] == 'o' && st.name[2] == 0);
    CHECK(st.hero.speci == S_MAG && st.hero.st[S_MAG] == 12 * STAT + STAT / 4 && st.hero.st[S_STR] == 10 * STAT);

    // dungeon: the corpse holds the little key, once
    sw_init(110);
    put(10, 5 * TILE + HB_X0, 2 * TILE + HB_Y0);
    st.dir = DIR_UP;
    CHECK(talk() == T_HOLDS_KEY && st.flag[2]);
    CHECK(talk() == 0xFF);
    // the notice re-rolls the number; the riddle wants devi - 3000
    sw_init(3);
    put(12, 3 * TILE + HB_X0, 2 * TILE + HB_Y0);
    st.dir = DIR_UP;
    CHECK(talk() == T_NOTICE && st.devi > 5000 && st.devi <= 5100);
    {
        u16 ans = st.devi - 3000;
        put(13, 15 * TILE + HB_X0, 6 * TILE + HB_Y0);
        st.cell_in = 0;
        for (k = 0; k < 20 && st.mode == M_WALK; k++) sw_step(K_RIGHT);
        CHECK(st.mode == M_SCRIPT);
        for (k = 0; k < 200 && st.shop != 2; k++) sw_step(k % 6 == 0 ? K_A : 0);
        st.digit[0] = 9; st.digit[1] = 9; st.digit[2] = 9; st.digit[3] = 9;   // wrong first
        sw_step(0); sw_step(K_A);
        run_script();
        CHECK(!st.flag[3] && st.mode == M_WALK);
        hold(K_LEFT, 12);
        st.cell_in = 0;
        for (k = 0; k < 20 && st.mode == M_WALK; k++) sw_step(K_RIGHT);
        for (k = 0; k < 200 && st.shop != 2; k++) sw_step(k % 6 == 0 ? K_A : 0);
        st.digit[0] = ans / 1000; st.digit[1] = ans / 100 % 10; st.digit[2] = ans / 10 % 10; st.digit[3] = ans % 10;
        sw_step(0); sw_step(K_A);
        run_script();
        CHECK(st.flag[3] && st.mode == M_WALK);
    }
    // the switch toggles clef[17]; the trap chest starts a fight and comes back
    put(13, 5 * TILE + HB_X0, 2 * TILE + HB_Y0);
    st.dir = DIR_UP;
    sw_step(0); sw_step(K_A);
    for (k = 0; k < 100 && !(st.dlg_on && st.dlg_ask && st.dlg_shown >= 16); k++) sw_step(0);
    sw_step(K_A);
    run_script();
    CHECK(st.flag[17] == 1);
    put(13, 9 * TILE + HB_X0, 3 * TILE + HB_Y0);
    strong();
    sw_step(0); sw_step(K_A);
    for (k = 0; k < 300 && st.mode != M_BATTLE; k++) sw_step(k % 6 == 0 ? K_A : 0);
    CHECK(st.mode == M_BATTLE && st.qu);
    fight();
    for (k = 0; k < 100 && st.mode == M_SCRIPT; k++) sw_step(0);
    CHECK(st.mode == M_WALK);

    // prison finds: Fire (14), Power Wrist (15), Bronze Bangle (17), worn by auto_equip
    sw_init(4);
    put(14, 15 * TILE + HB_X0, 4 * TILE + HB_Y0);
    st.dir = DIR_UP;
    CHECK(talk() == T_FOUND_FIRE && st.mat[MAT_FIRE]);
    put(15, 7 * TILE + HB_X0, 4 * TILE + HB_Y0);
    CHECK(talk() == T_HOLDS_SOMETHING && st.own[A_WRIST] && !st.hero.acc[0]);   // equip it in the menu
    put(17, 9 * TILE + HB_X0, 4 * TILE + HB_Y0);
    CHECK(talk() == T_FOUND_BANGLE && st.own[A_BANGLE]);

    // the boss: the 503 ring, the prisoner walks up, the fight, the Cell 2 Key; then the sword
    sw_init(5);
    CHECK(rooms[st.room].id == 11 && st.flag[17]);
    for (k = 0; k < 80 && rooms[st.room].id == 11; k++) sw_step(K_UP);
    hold(0, 30);
    CHECK(rooms[st.room].id == 16);
    strong();
    for (k = 0; k < 60 && st.mode != M_SCRIPT; k++) sw_step(K_UP);
    CHECK(st.mode == M_SCRIPT);
    for (k = 0; k < 2000 && st.mode != M_BATTLE; k++) sw_step(k % 6 == 0 ? K_A : 0);
    CHECK(st.mode == M_BATTLE && st.battle == 3 && !st.qu);
    fight();
    run_script();
    CHECK(st.mode == M_WALK && st.flag[5] && st.item[I_HIPOTION] >= 1 && st.hero.exp >= 330);
    put(16, 9 * TILE + HB_X0, 4 * TILE + HB_Y0);
    st.dir = DIR_UP;
    CHECK(talk() == T_FOUND_SWORD && st.own[A_SWORD] && !st.flag[8] && !st.hero.weapon);

    // the menu: equip the sword, put Fire in slot 1, drink a Potion, then quit
    st.hero.weapon = st.hero.slot[0] = st.hero.slot[1] = 0;
    st.hero.hp = 20;
    sw_step(0); sw_step(K_ESC);
    CHECK(st.mode == M_MENU);
    sw_step(K_DOWN); sw_step(0); sw_step(K_A); sw_step(0);   // Equip
    sw_step(K_A); sw_step(0);                                // weapon: Buster Sword
    CHECK(st.hero.weapon == A_SWORD);
    sw_step(K_B); sw_step(0); sw_step(K_DOWN); sw_step(0); sw_step(K_A); sw_step(0);   // Materia
    sw_step(K_A); sw_step(0);                                // slot 1: Fire
    CHECK(has_materia(MAT_FIRE) && st.hero.slot[0] == MAT_FIRE);
    sw_step(K_B); sw_step(0);
    for (k = 0; k < 2; k++) { sw_step(K_UP); sw_step(0); }   // Items
    sw_step(K_A); sw_step(0);
    k = st.item[I_POTION];
    sw_step(K_A); sw_step(0);                                // a Potion: +100 HP
    CHECK(st.item[I_POTION] == k - 1 && st.hero.hp == (st.hero.hpm < 120 ? st.hero.hpm : 120));
    sw_step(K_B); sw_step(0); sw_step(K_B); sw_step(0);
    CHECK(st.mode == M_WALK);

    // room 4: the house message, and the way south ends part I
    sw_init(9);
    CHECK(rooms[st.room].id == 4 && st.flag[7] && st.mat[MAT_CURE]);
    put(4, 2 * TILE + HB_X0, 6 * TILE + HB_Y0);
    st.dir = DIR_UP;
    CHECK(talk() == T_MESSAGE4);
    sw_init(9);
    for (k = 0; k < 200 && st.mode != M_END; k++) sw_step(K_DOWN);
    CHECK(st.mode == M_END);

    // save from the menu, then Continue on the title restores the room, flags and stats
    remove(SAVE_NAME ".sav");
    sw_init(0);
    CHECK(st.mode == M_TITLE);
    for (k = 0; k < 20; k++) sw_step(0);
    sw_step(K_DOWN); sw_step(0);
    CHECK(st.tcur == 0);                     // no save: Continue is not selectable
    sw_init(7);
    st.hero.gils = 1234; st.hero.lv = 5;
    sw_step(0); sw_step(K_ESC);
    for (k = 0; k < 4; k++) { sw_step(K_DOWN); sw_step(0); }
    sw_step(K_A); sw_step(0);
    CHECK(st.mode == M_MENU && st.mmsg == 3);
    sw_init(0);
    for (k = 0; k < 20; k++) sw_step(0);     // the title fades in first
    sw_step(K_DOWN); sw_step(0);
    CHECK(st.tcur == 1);
    sw_step(K_A);
    for (k = 0; k < 30 && st.mode != M_WALK; k++) sw_step(0);
    CHECK(st.mode == M_WALK && rooms[st.room].id == 5 && st.hero.gils == 1234 && st.hero.lv == 5 && st.own[A_SWORD]);
    remove(SAVE_NAME ".sav");

    sw_init(10);
    sw_step(0); sw_step(K_ESC);              // ESC opens the menu; Quit leaves the game
    CHECK(st.mode == M_MENU);
    sw_step(K_UP); sw_step(0);
    CHECK(sw_step(K_A) == 0);
    printf(fails ? "%d FAILED\n" : "all tests passed\n", fails);
    return fails != 0;
}
