// Final Fantasy Alternative (David Coz, TI-Basic, 2002) remade on the Portable Game Runtime.
// Part I: the castle and its dungeon. Field mode: continuous movement over 16x16 tile rooms,
// doors with fades, text triggers (2nd), story triggers (stepped on), random encounters.
// Injection door: scenario 0 = new game, 100 + r = room r (original number), see README.
#include "ffa.h"
#include "gfx.h"
#include "rooms.h"

Game st;
static const RtSprite hero_spr = { 16, 24, hero_light[0], hero_dark[0], hero_mask[0] };

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

void game_scenario(u16 n)
{
    Game z = { 0 };
    st = z;
    new_game();
    st.mode = M_WALK;
    st.dir = DIR_DOWN;
    if (n >= 100 && n < 100 + sizeof(room_index) && room_index[n - 100] != 255)
        place_somewhere(room_index[n - 100]);
    else
        world_enter(room_index[8], 4, 3);    // ffa: dec8, a = 27, b = 27
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
            st.mode = M_TEXT;
            return;
        }
    }
    if (input_pressed(K_A | K_ENTER)) {      // examine the cell in front of the hero
        static const s8 fx[4] = { 0, 0, -1, 1 }, fy[4] = { 1, -1, 0, 0 };
        c = world_cell(st.x + HB_W / 2 + fx[st.dir] * (HB_W / 2 + 4), st.y + HB_H / 2 + fy[st.dir] * (HB_H / 2 + 4));
        if ((c & 0xC0) == CELL_TRIG && rooms[st.room].trig[c & 0x3F] < 0) {
            st.trig = rooms[st.room].trig[c & 0x3F];
            st.mode = M_TEXT;
        }
    }
}

u8 game_update(void)
{
    if (input_pressed(K_ESC)) return 0;
    switch (st.mode) {
    case M_WALK: walk(); break;
    case M_FADE_OUT: case M_FADE_IN: fade_step(); break;
    case M_TEXT: case M_BATTLE:
        if (input_pressed(K_A | K_ENTER)) { st.mode = M_WALK; st.trig = 0; }
        break;
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
    draw_sprite(st.x + SPR_DX - cx, st.y + SPR_DY - cy, &hero_spr);
    if (st.mode == M_TEXT || st.mode == M_BATTLE) {
        draw_rect(4, 70, 152, 26, C_BLACK);
        draw_rect(5, 71, 150, 24, C_WHITE);
        if (st.mode == M_BATTLE) draw_text(8, 74, "A monster attacks!", F_SMALL, C_BLACK);
        else if (st.trig == -1) draw_text(8, 74, "The door is locked.", F_SMALL, C_BLACK);
        else { s[0] = 'p'; s[1] = '='; put_num(s + 2, st.trig); draw_text(8, 74, s, F_SMALL, C_BLACK); }
    }
    if (st.mode == M_FADE_OUT || st.mode == M_FADE_IN) fade_planes(st.fade);
}
