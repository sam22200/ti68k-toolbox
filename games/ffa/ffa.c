// Final Fantasy Alternative (David Coz, TI-Basic, 2002) remade on the Portable Game Runtime.
// Part I: the castle and its dungeon. Field mode: continuous movement over 16x16 tile rooms,
// doors with fades, text triggers (2nd), story triggers (stepped on), random encounters.
// Injection door: scenario 0 = new game, 100 + r = room r (original number), see README.
#include "ffa.h"
#include "gfx.h"
#include "rooms.h"
#include "texts.h"

Game st;

static void new_game(void)
{
    u16 k;
    for (k = 0; k < NFLAG; k++) st.flag[k] = 0;
    st.flag[6] = st.flag[8] = 1;             // ffa: 1->clef[6,1], 1->clef[8,1]
    st.hero.lv = 1;
    st.hero.hp = st.hero.hpm = 80;
    st.hero.mp = st.hero.mpm = 15;
    st.hero.str = 10; st.hero.def = 5; st.hero.mag = 12; st.hero.mdef = 6;
    st.hero.spd = 15; st.hero.luck = 3;
    st.hero.exp = 0; st.hero.gils = 100;
    for (k = 0; k < NITEM; k++) st.item[k] = 0;
    for (k = 0; k < NARM; k++) st.own[k] = 0;
    for (k = 0; k < NMAT; k++) st.mat[k] = 0;
    st.item[I_POTION] = 3; st.item[I_ANTIDOTE] = 1;
    for (k = 0; k < 7; k++) st.name[k] = "Arthur"[k];
    st.devi = 5000 + rt_rand() % 100 + 1;   // ffa: 5000+rand(100)
    st.co = 15 + rt_rand() % 5 + 1;          // 15+rand(5): 16..20 steps
}

void game_init(void)
{
    rt_state = &st;
    rt_state_size = sizeof(st);
}

static void place_somewhere(u8 room)         // first free cell from the room centre
{
    const Room *r = &rooms[room];
    s16 d, x, y;
    st.room = room;
    for (d = 0; d < 20; d++)
        for (y = r->h / 2 - d; y <= r->h / 2 + d; y++)
            for (x = r->w / 2 - d; x <= r->w / 2 + d; x++)
                if (x >= 0 && y >= 0 && x < r->w && y < r->h && r->cell[y * r->w + x] == CELL_FLOOR) {
                    world_enter(room, x, y);
                    return;
                }
    world_enter(room, 1, 1);
}

// Story checkpoints (walkthrough steps of docs/part1.md): flags and items of that moment, the
// hero where the next step starts. 1..9, see README.
static void checkpoint(u8 n)
{
    static const u8 room_at[10] = { 8, 6, 7, 12, 14, 11, 16, 5, 18, 4 };
    static const s8 cell[10][2] = { { 4, 4 }, { 13, 3 }, { 10, 3 }, { 4, 3 }, { 15, 5 }, { 4, 7 },
                                    { 9, 8 }, { 9, 7 }, { 5, 7 }, { 9, 7 } };
    if (n >= 1) { st.flag[9] = st.flag[10] = 1; }
    if (n >= 2) { st.flag[1] = st.flag[11] = 1; }
    if (n >= 3) { st.flag[2] = 1; }
    if (n >= 4) { st.flag[3] = 1; }
    if (n >= 5) { st.flag[17] = 1; st.mat[MAT_FIRE] = 1; st.own[A_WRIST] = 1; }
    if (n >= 6) { st.flag[5] = 1; st.item[I_HIPOTION]++; }
    if (n >= 7) { st.own[A_SWORD] = 1; st.flag[8] = 0; st.own[A_BANGLE] = 1; }
    if (n >= 8) { st.flag[7] = st.flag[8] = 1; }
    if (n >= 9) { st.mat[MAT_CURE] = 1; }
    world_enter(room_index[room_at[n]], cell[n][0], cell[n][1]);
}

void game_scenario(u16 n)
{
    Game z = { 0 };
    st = z;
    new_game();
    st.mode = M_WALK;
    st.dir = DIR_DOWN;
    if (n >= 100 && n < 100 + sizeof(room_index) && room_index[n - 100] != 255)
        place_somewhere(room_index[n - 100]);
    else if (n >= 1 && n <= 9)
        checkpoint(n);
    else
        world_enter(room_index[8], 4, 4);    // ffa: dec8, a = 27, b = 27
    story_room();
}

static void fade_step(void)
{
    if (++st.fade_t < FADE_FRAMES) return;
    st.fade_t = 0;
    if (st.mode == M_FADE_OUT) {
        if (++st.fade < FADE_STEPS - 1) return;
        {
            const Door *d = &rooms[st.room].door[st.next_door];
            if (d->dest == ROOM_OUT) { st.mode = M_END; return; }
            world_enter(d->dest, d->ax, d->ay);
            story_room();
            st.cell_in = 0xFF;                // arriving on a story cell runs it (original scenar)
        }
        st.mode = M_FADE_IN;
    } else if (!--st.fade) {
        st.mode = M_WALK;
    }
}

static void walk(void)
{
    s16 v, dx = 0, dy = 0;
    u8 c;
    st.sub ^= 1;
    v = input_held(K_B) ? 3 : 1 + st.sub;    // walk 1.5 px/frame (48 px/s), run 3 px/frame
    if (input_held(K_LEFT)) { dx = -v; st.dir = DIR_LEFT; }
    if (input_held(K_RIGHT)) { dx = v; st.dir = DIR_RIGHT; }
    if (input_held(K_UP)) { dy = -v; st.dir = DIR_UP; }
    if (input_held(K_DOWN)) { dy = v; st.dir = DIR_DOWN; }
    if (dx || dy) { world_move(dx, dy); st.anim++; } else st.anim = 0;
    if (st.mode != M_WALK) return;

    while (st.walked >= STEP_PX) {           // one original step: encounter counter
        st.walked -= STEP_PX;
        st.steps++;
        st.mc += rooms[st.room].frc;
        if (rooms[st.room].frc && st.mc > st.co * 10) {
            st.mc = 0;
            st.co = 15 + rt_rand() % 5 + 1;
            st.mode = M_BATTLE;
            return;
        }
    }
    c = world_cell(st.x + HB_W / 2, st.y + HB_H / 2);   // story trigger: stepped on
    if (c != st.cell_in) {
        st.cell_in = c;
        if ((c & 0xC0) == CELL_TRIG && rooms[st.room].trig[c & 0x3F] >= 5000) {
            st.trig = rooms[st.room].trig[c & 0x3F];
            if (story_trigger(st.trig, 0)) { st.anim = 0; return; }
            st.trig = 0;
        }
    }
    if (input_pressed(K_A | K_ENTER)) {      // examine the cell in front of the hero
        static const s8 fx[4] = { 0, 0, -1, 1 }, fy[4] = { 1, -1, 0, 0 };
        c = world_cell(st.x + HB_W / 2 + fx[st.dir] * (HB_W / 2 + 4), st.y + HB_H / 2 + fy[st.dir] * (HB_H / 2 + 8));
        if ((c & 0xC0) == CELL_TRIG && rooms[st.room].trig[c & 0x3F] < 0) {
            st.trig = rooms[st.room].trig[c & 0x3F];
            if (story_trigger(st.trig, 1)) { st.anim = 0; return; }
            st.mode = M_TEXT;               // not remade yet: placeholder box showing p
        }
    }
}

u8 game_update(void)
{
    if (input_pressed(K_ESC) && st.mode == M_WALK) return 0;   // (the ESC menu comes later)
    switch (st.mode) {
    case M_WALK: walk(); break;
    case M_FADE_OUT: case M_FADE_IN: fade_step(); break;
    case M_TEXT:
        if (st.dlg_on) { dialog_update(); if (!st.dlg_on) st.mode = M_WALK; break; }
        /* fall through: placeholder box */
    case M_BATTLE:
        if (input_pressed(K_A | K_ENTER)) { st.mode = M_WALK; st.trig = 0; }
        break;
    case M_SCRIPT: story_run(); break;
    case M_END: break;
    }
    return 1;
}

static void put_num(char *s, s16 v)
{
    char t[8];
    u8 k = 0;
    if (v < 0) { *s++ = '-'; v = -v; }
    do t[k++] = '0' + v % 10; while ((v /= 10));
    while (k) *s++ = t[--k];
    *s = 0;
}

static void draw_actors(s16 cx, s16 cy)       // hero and NPCs, back to front (by feet y)
{
    static const u8 cycle[4] = { 0, 1, 0, 2 };   // stand, step, stand, other step
    s16 ys[NNPC + 1];
    u8 order[NNPC + 1], n = 0, i, j, t;
    for (i = 1; i < NNPC; i++) if (st.npc[i].on) { order[n] = i; ys[n++] = st.npc[i].y; }
    order[n] = 0; ys[n++] = st.y;
    for (i = 1; i < n; i++)                  // insertion sort, n <= 4
        for (j = i; j && ys[j - 1] > ys[j]; j--) {
            s16 y = ys[j]; ys[j] = ys[j - 1]; ys[j - 1] = y;
            t = order[j]; order[j] = order[j - 1]; order[j - 1] = t;
        }
    for (i = 0; i < n; i++) {
        RtSprite s;
        s.w = 16; s.h = 24;
        if (!order[i]) {
            u8 f = st.dir * 3 + (st.anim ? cycle[(st.anim >> 2) & 3] : 0);
            s.light = hero_light[f]; s.dark = hero_dark[f]; s.mask = hero_mask[f];
            draw_sprite(st.x + SPR_DX - cx, st.y + SPR_DY - cy, &s);
        } else {
            const Npc *p = &st.npc[order[i]];
            u8 f = p->spr * 6 + (p->dir == DIR_UP ? 3 : 0) + (p->anim ? cycle[(p->anim >> 2) & 3] : 0);
            s.light = npc_light[f]; s.dark = npc_dark[f]; s.mask = npc_mask[f];
            draw_sprite(p->x + SPR_DX - cx, p->y + SPR_DY - cy, &s);
        }
    }
}

void game_render(void)
{
    const Room *r = &rooms[st.room];
    s16 cx = world_cam(st.x + HB_W / 2, RT_W, r->w * TILE), cy = world_cam(st.y + HB_H / 2, RT_H, r->h * TILE);
    char s[24];
    if (st.mode == M_END) {
        draw_clear();
        draw_text(40, 40, "End of Part I", F_MEDIUM, C_BLACK);
        return;
    }
    draw_tilemap(&r->map, cx, cy);
    draw_actors(cx, cy);
    if (st.shop) shop_render();
    if (st.dlg_on) dialog_render(st.y - cy > 60);
    if ((st.mode == M_TEXT && !st.dlg_on) || st.mode == M_BATTLE) {
        draw_rect(4, 70, 152, 26, C_BLACK);
        draw_rect(5, 71, 150, 24, C_WHITE);
        if (st.mode == M_BATTLE) draw_text(8, 74, "A monster attacks!", F_SMALL, C_BLACK);
        else { s[0] = 'p'; s[1] = '='; put_num(s + 2, st.trig); draw_text(8, 74, s, F_SMALL, C_BLACK); }
    }
    if (st.fade) fade_planes(st.fade);
}
