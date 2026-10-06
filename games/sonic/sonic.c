#include <string.h>
#include "sonic.h"
#include "art.h"
#include "generated/trig.h"
#ifdef STATE_HASH
#include "../../tools/m68kbench/bench.h"
#endif

Sonic st;
static const u8 *cells, *profiles;
static RtTilemap terrain;

/* Verified common primitive from ti68k-performance.md, not new game ASM. */
static inline s32 mul16(s16 a, s16 b)
{
#ifdef __m68k__
    s32 r;
    asm("muls.w %2,%0" : "=d"(r) : "0"(a), "dmi"(b));
    return r;
#else
    return (s32)a * b;
#endif
}

static u16 be16(const u8 *p) { return (u16)((u16)p[0] << 8) | p[1]; }
static s16 radius(void) { return st.flags & S_ROLL ? 14 : 19; }
static s16 width(void) { return st.flags & S_ROLL ? 7 : 9; }
static s16 magnitude(s16 v) { return v < 0 ? -v : v; }

static const u8 *profile(s16 x, s16 y)
{
    if ((u16)x >= SON_WORLD_W || (u16)y >= 2048) return profiles;
    return profiles + ((u16)cells[(((u16)y >> 4) << 7) + ((u16)x >> 4)] << 6);
}

u8 sonic_solid(s16 x, s16 y, u8 sides)
{
    const u8 *p = profile(x, y);
    if (!(p[49] & (sides ? 2 : 1))) return 0;
    return (((const u16 *)p)[(u16)y & 15] & (0x8000U >> ((u16)x & 15))) != 0;
}

s16 sonic_floor(s16 x, s16 low, s16 high, u8 *angle)
{
    s16 row, best = 32767;
    if ((u16)x >= SON_WORLD_W || high < 0) return best;
    if (low < 0) low = 0;
    if (high >= 2048) high = 2047;
    for (row = low & ~15; row <= high; row += 16) {
        const u8 *p = profile(x, row);
        s16 h = ((const s8 *)p)[32 + ((u16)x & 15)], top;
        if (!(p[49] & 1) || !h) continue;
        top = h > 0 ? row + 16 - h : row;
        if (top < low || top > high || sonic_solid(x, top - 1, 0)) continue;
        if (top < best) { best = top; *angle = p[48]; }
    }
    if (x >= SON_BRIDGE_L && x < SON_BRIDGE_R &&
        SON_BRIDGE_Y >= low && SON_BRIDGE_Y <= high && SON_BRIDGE_Y < best) {
        best = SON_BRIDGE_Y; *angle = 0;
    }
    return best;
}

static s16 feet_floor(s16 low, s16 high, u8 *angle)
{
    u8 a = 0, b = 0;
    s16 l = sonic_floor(st.x - width(), low, high, &a);
    s16 r = sonic_floor(st.x + width(), low, high, &b);
    *angle = l <= r ? a : b;
    return l <= r ? l : r;
}

static void camera(void)
{
    s16 x = (st.x >> 1) - 80, y = (st.y >> 1) - 60;
    if (x < 0) x = 0;
    if (x > (SON_WORLD_W >> 1) - RT_W) x = (SON_WORLD_W >> 1) - RT_W;
    if (y < 0) y = 0;
    if (y > 1024 - RT_H) y = 1024 - RT_H;
    st.camx = x; st.camy = y;
}

void sonic_start(s16 x, s16 y)
{
    u8 ready = st.ready, debug = st.debug;
    u16 resets = st.resets;
    memset(&st, 0, sizeof st);
    st.ready = ready; st.debug = debug; st.resets = resets;
    st.x = x; st.y = y;
    sonic_objects_reset();
    camera();
}

void game_init(void)
{
    const u8 *data;
    u16 size, po, vo, to, np, nt;
    rt_state = &st; rt_state_size = sizeof st;
    memset(&st, 0, sizeof st);
    cells = profiles = RT_NULL;
    son_objects = RT_NULL; son_object_count = 0;
    memset(&terrain, 0, sizeof terrain);
    data = rt_file("sonterr", &size);
    if (!data || size < 32 || memcmp(data, "SNC2", 4)) return;
    po = be16(data + 16); vo = be16(data + 18); to = be16(data + 20);
    np = be16(data + 8); nt = be16(data + 10);
    if (be16(data + 4) != 128 || be16(data + 6) != 128 || !np || np > 256 ||
        !nt || nt > 256 || be16(data + 12) != 52 || be16(data + 14) != 64 ||
        po != 16416 || vo != po + (np << 6) || to != vo + 52 * 64 ||
        (u32)to + ((u32)nt << 6) != be16(data + 24) ||
        be16(data + 26) > SON_RINGS + SON_ENEMIES ||
        /* TI OTH size also includes the type/extension trailer. */
        (u32)be16(data + 24) + be16(data + 26) * 6U > size) return;
    cells = data + 32; profiles = data + po;
    terrain.map = data + vo; terrain.w = 52; terrain.h = 64;
    terrain.tiles = (const u16 *)(data + to); terrain.ntiles = nt;
    son_objects = data + be16(data + 24); son_object_count = be16(data + 26);
    { u8 i, rings = 0, enemies = 0; const u8 *p = son_objects;
      for (i = 0; i < son_object_count; i++, p += 6) {
          if (be16(p) >= SON_END || be16(p + 2) >= 2048) return;
          if (p[4] == 0x25) rings++;
          else if (p[4] == E_MOTO || p[4] == E_BUZZ || p[4] == E_CHOP) enemies++;
          else return;
      }
      if (rings > SON_RINGS || enemies != SON_ENEMIES) return;
    }
    st.ready = sonic_art_init();
}

void game_scenario(u16 n)
{
    st.resets = 0; st.debug = 0;
    sonic_start(80, 944);
    if (!st.ready) return;
    switch (n) {
    case 1: sonic_start(192, 940); break;       /* measured flat ground */
    case 2: sonic_start(1024, 876); break;      /* before bridge */
    case 3: sonic_start(1184, 876); break;      /* rigid bridge */
    case 4: sonic_start(1456, 876); break;      /* endpoint approach */
    case 5: sonic_start(600, 720); st.flags = S_AIR; break; /* upper ledge */
    case 6: sonic_start(1176, 1080); st.flags = S_AIR; break; /* below bridge */
    case 7: sonic_start(192, 940); st.debug = 1; break;
    case 8: sonic_start(324, 864); st.flags = S_AIR | S_ROLL; break;
    case 9: sonic_start(790, 900); st.flags = S_AIR | S_ROLL; st.vx = 768; st.vy = 768; break;
    case 10: sonic_start(832, 920); st.rings = 10; st.flags = S_AIR; break;
    case 11: sonic_start(1024, 820); st.rings = 10; st.flags = S_AIR; break;
    case 12: sonic_start(192, 940); st.rings = 32; sonic_hurt(210); break;
    default: break;
    }
    if (!(st.flags & S_AIR)) {
        u8 angle = 0;
        s16 top = feet_floor(st.y + 19 - 16, st.y + 19 + 16, &angle);
        if (top != 32767) { st.y = top - 20; st.angle = angle; }
        else st.flags |= S_AIR;
    }
    camera();
}

static void integrate(s16 *position, u8 *fraction, s16 velocity)
{
    s16 sum = (s16)*fraction + velocity;
    *position += sum >> 8;
    *fraction = (u8)sum;
}

static void move_x(void)
{
    s16 target = st.x;
    u8 frac = st.fx;
    s16 step = st.vx < 0 ? -1 : 1;
    integrate(&target, &frac, st.vx);
    while (st.x != target) {
        s16 next = st.x + step, front = next + (step < 0 ? -width() : width());
        if (next < 16 || sonic_solid(front, st.y, 1) ||
            sonic_solid(front, st.y - (radius() >> 1), 1)) {
            st.vx = st.speed = 0; st.fx = 0; return;
        }
        st.x = next;
    }
    st.fx = frac;
}

static s16 friction(s16 speed, s16 amount)
{
    if (speed > 0) return speed > amount ? speed - amount : 0;
    return speed < -amount ? speed + amount : 0;
}

static void ground_speed(u16 keys)
{
    s16 v = st.speed, slope = sin_tab[st.angle], roll = (st.flags & S_ROLL) != 0;
    if (v) {
        s16 force = (s16)(mul16(slope, roll ? 80 : 32) >> 8);
        if (roll && ((v > 0 && force < 0) || (v < 0 && force > 0))) force >>= 2;
        v += force;
    }
    if (roll) {
        if ((keys & K_LEFT) && v > 0) v -= 32;
        if ((keys & K_RIGHT) && v < 0) v += 32;
        v = friction(v, 6);
        if (!v) { st.flags &= ~S_ROLL; st.y -= 5; }
    } else {
        if (keys & K_LEFT) {
            if (v > 0) { v -= SON_BRAKE; if (v < 0) v = -SON_BRAKE; }
            else { v -= SON_ACCEL; if (v < -SON_TOP_SPEED) v = -SON_TOP_SPEED; st.flags |= S_LEFT; }
        }
        if (keys & K_RIGHT) {
            if (v < 0) { v += SON_BRAKE; if (v > 0) v = SON_BRAKE; }
            else { v += SON_ACCEL; if (v > SON_TOP_SPEED) v = SON_TOP_SPEED; st.flags &= ~S_LEFT; }
        }
        if (!(keys & (K_LEFT | K_RIGHT))) v = friction(v, SON_ACCEL);
        if ((keys & K_DOWN) && !(keys & (K_LEFT | K_RIGHT)) && magnitude(v) >= 128) {
            st.flags |= S_ROLL; st.y += 5;
        }
    }
    st.speed = v;
    st.vx = (s16)(mul16(v, sin_tab[(u8)(st.angle + 64)]) >> 8);
    st.vy = (s16)(mul16(v, slope) >> 8);
}

static void air_speed(u16 keys)
{
    s16 v = st.vx;
    if (!(st.flags & S_ROLLJUMP)) {
        if (keys & K_LEFT) { v -= 24; if (v < -SON_TOP_SPEED) v = -SON_TOP_SPEED; st.flags |= S_LEFT; }
        if (keys & K_RIGHT) { v += 24; if (v > SON_TOP_SPEED) v = SON_TOP_SPEED; st.flags &= ~S_LEFT; }
    }
    if (st.vy < 0 && st.vy >= -1024) v -= v >> 5;
    st.vx = v;
}

void sonic_step(u16 keys, u8 jump_pressed)
{
    u8 angle = 0;
    s16 top, oldfeet;
    if (!st.ready || st.finished) return;
    st.logic++;
    if (!st.hurt && st.invuln) st.invuln--;
    if (st.hurt) { keys = 0; jump_pressed = 0; }
    if (!(st.flags & S_AIR) && jump_pressed &&
        !sonic_solid(st.x - width(), st.y - radius() - 6, 1) &&
        !sonic_solid(st.x + width(), st.y - radius() - 6, 1)) {
        if (st.flags & S_ROLL) st.flags |= S_ROLLJUMP;
        else { st.flags |= S_ROLL; st.y += 5; }
        st.flags = (st.flags | S_AIR) & ~S_BRIDGE;
        st.jumping = 1;
        st.vx += (s16)(mul16(SON_JUMP_SPEED, sin_tab[st.angle]) >> 8);
        st.vy -= (s16)(mul16(SON_JUMP_SPEED, sin_tab[(u8)(st.angle + 64)]) >> 8);
        /* The observed launch frame changes posture/speed, not position. */
        sonic_objects_step(); camera(); return;
    }
    if (!(st.flags & S_AIR)) {
        ground_speed(keys);
        move_x();
        integrate(&st.y, &st.fy, st.vy);
        top = feet_floor(st.y + radius() - 14, st.y + radius() + 17, &angle);
        if (top == 32767) { st.flags = (st.flags | S_AIR) & ~S_BRIDGE; st.jumping = 0; }
        else {
            st.y = top - 1 - radius(); st.angle = angle;
            st.flags &= ~S_BRIDGE;
            if (top == SON_BRIDGE_Y && st.x >= SON_BRIDGE_L && st.x < SON_BRIDGE_R) st.flags |= S_BRIDGE;
        }
    } else {
        if (st.jumping && !(keys & K_A) && st.vy < -1024) st.vy = -1024;
        if (!st.hurt) air_speed(keys);
        oldfeet = st.y + radius();
        move_x();
        integrate(&st.y, &st.fy, st.vy);
        st.vy += st.hurt ? 48 : SON_GRAVITY;
        if (st.vy < 0) {
            u16 count = 0;
            while (count++ < 32 && (sonic_solid(st.x - width(), st.y - radius(), 1) ||
                                    sonic_solid(st.x + width(), st.y - radius(), 1))) {
                st.y++; st.vy = 0;
            }
        } else {
            top = feet_floor(oldfeet, st.y + radius() + 1, &angle);
            if (top != 32767) {
                st.y = top - 1 - radius();
                if (st.flags & S_ROLL) st.y -= 5;
                st.flags &= ~(S_AIR | S_ROLL | S_ROLLJUMP);
                st.angle = angle; st.jumping = 0; st.vy = 0; st.speed = st.vx;
                if (st.hurt) { st.hurt = 0; st.invuln = 120; st.vx = st.speed = 0; }
            }
        }
    }
    if (st.y > 1200) { st.resets++; sonic_start(80, 944); }
    sonic_objects_step();
    if (st.x >= SON_END) { st.x = SON_END; st.finished = 1; st.vx = st.vy = st.speed = 0; }
    camera();
}

u32 sonic_hash(void)
{
    const u16 values[] = { (u16)st.x, (u16)st.y, (u16)st.vx, (u16)st.vy, (u16)st.speed,
        st.camx, st.camy, st.logic, st.resets, st.fx, st.fy, st.flags, st.angle,
        st.jumping, st.finished, st.debug, st.ready };
    const u16 *p = values;
    u16 n = sizeof values / sizeof values[0];
    u32 hash = 0;
    while (n--) hash = ((hash << 5) | (hash >> 27)) ^ *p++;
    return sonic_objects_hash(hash);
}

u8 game_update(void)
{
    if (input_held(K_ESC)) return 0;
    if (input_pressed(K_D)) st.debug ^= 1;
    if (input_pressed(K_ENTER)) { u16 resets = st.resets + 1; game_scenario(0); st.resets = resets; }
    else {
        sonic_step((u16)rt_keys, input_pressed(K_A) != 0);
        sonic_step((u16)rt_keys, 0);
    }
#ifdef STATE_HASH
    BENCH_VALUE(sonic_hash());
#endif
    return 1;
}

void game_render(void)
{
    s16 x;
    u8 i;
    if (!st.ready) {
        draw_clear(); draw_text(4, 20, "send sonterr.89y", F_SMALL, C_BLACK);
        draw_text(4, 30, "and sonart.89y", F_SMALL, C_BLACK); return;
    }
    sonic_art_world();
    for (i = 0; i < 12; i++) sonic_art_draw(ART_BRIDGE_0_RIGHT, 1096 + ((u16)i << 4), 904);
    sonic_objects_render();
    sonic_art_player();
    x = (SON_END >> 1) - st.camx;
    draw_rect(x, (876 >> 1) - st.camy - 18, 2, 28, C_BLACK);
    draw_rect(x + 2, (876 >> 1) - st.camy - 18, 8, 5, C_DGRAY);
    if (st.finished) {
        draw_rect(28, 12, 104, 16, C_WHITE);
        draw_text(34, 17, "END - ENTER restart", F_SMALL, C_BLACK);
    }
    if (st.debug) {
        draw_rect(0, 0, 160, 8, C_WHITE);
        draw_text(2, 1, st.flags & S_AIR ? "AIR" : st.flags & S_ROLL ? "ROLL" : "GROUND", F_SMALL, C_BLACK);
    }
}
