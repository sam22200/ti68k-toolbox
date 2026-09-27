// FFA unit and integration tests: sw_step + asserts, no window.
#include <stdio.h>
#include "../../runtime/platform-sw/rt_sw.h"
#include "ffa.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static void hold(u32 k, u16 n) { while (n--) sw_step(k); }
static u8 idx(u8 id) { return room_index[id]; }
static void put(u8 id, s16 x, s16 y) { st.room = idx(id); st.x = x; st.y = y; st.mode = M_WALK; }
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
                    sw_init(0);
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

    // new game: room 8, original a = 27, b = 27 (cell 4, 3), first-level stats
    sw_init(0);
    CHECK(rooms[st.room].id == 8 && st.x == 4 * TILE + HB_X0 && st.y == 3 * TILE + HB_Y0);
    CHECK(st.hero.hp == 80 && st.hero.mpm == 15 && st.hero.gils == 100 && st.flag[6] && st.flag[8]);

    // walking: 1.5 px per frame, 3 when running (shift / K_B)
    x0 = st.x;
    hold(K_RIGHT, 10);
    CHECK(st.x - x0 == 15 && st.dir == DIR_RIGHT);
    x0 = st.x;
    hold(K_RIGHT | K_B, 4);
    CHECK(st.x - x0 == 12);

    // walls: pushing left for long stops flush against the wall, inside the room
    sw_init(0);
    hold(K_LEFT, 60);
    CHECK(world_cell(st.x - 1, st.y) == CELL_WALL && !world_solid(world_cell(st.x, st.y)));

    // corner sliding: room 6, cell (2,2) is a wall and (3,2) floor; the hitbox overlaps
    // column 2 by 4 px and goes up: it slides right into column 3 instead of stopping
    sw_init(0);
    put(6, 3 * TILE - 4, 3 * TILE + HB_Y0);
    CHECK(world_solid(world_cell(2 * TILE + 8, 2 * TILE + 8)) && !world_solid(world_cell(3 * TILE + 8, 2 * TILE + 8)));
    hold(K_UP, 16);
    CHECK(st.y < 3 * TILE && st.x >= 3 * TILE);
    // too much overlap (8 px): no slide, blocked
    put(6, 3 * TILE - 8, 3 * TILE + HB_Y0);
    hold(K_UP, 16);
    CHECK(st.y >= 3 * TILE && st.x == 3 * TILE - 8);

    // door with a fade: room 8 -> room 6 (stairs); arrival a = 117, b = 18 -> cell (14, 2)
    sw_init(0);
    hold(K_RIGHT, 11);                       // x = 4 * 16 + 3 + 16 (column 5, under the stairs)
    CHECK(st.x == 5 * TILE + HB_X0 + 1);
    for (k = 0; k < 40 && st.mode != M_FADE_OUT; k++) {
        sw_step(K_UP);
        if (st.mode == M_TEXT) { sw_step(0); sw_step(K_A); }   // story trigger 500 on the way
    }
    CHECK(st.mode == M_FADE_OUT && rooms[st.room].id == 8);
    hold(0, FADE_FRAMES * 2);
    CHECK(sw_level(80, 50) < 3);             // lighter while fading
    hold(0, FADE_FRAMES * 8);
    CHECK(rooms[st.room].id == 6 && st.mode == M_WALK);
    CHECK((st.x >> 4) == 14 && (st.y >> 4) == 2);

    // locked door: room 13 -> 14 needs clef[3] (the injured-number riddle)
    sw_init(113);
    for (k = 0; k < NFLAG; k++) st.flag[k] = 0;
    doors_all();

    // random encounters: the dungeon (room 10, frc 1.2) attacks within ~20 steps, the castle never
    sw_init(110);
    for (k = 0; k < 2000 && st.mode != M_BATTLE; k++) sw_step(k & 64 ? K_LEFT : K_RIGHT);
    CHECK(st.mode == M_BATTLE && st.steps <= 20 * 10 / 12 + 2);
    sw_init(0);
    for (k = 0; k < 1000; k++) sw_step(k & 32 ? K_LEFT : K_RIGHT);
    CHECK(st.mode == M_WALK && st.steps > 30);

    CHECK(sw_step(K_ESC) == 0);
    printf(fails ? "%d FAILED\n" : "all tests passed\n", fails);
    return fails != 0;
}
