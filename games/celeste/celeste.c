// Celeste Classic, slice 1: room 0 (spawn, run, jump, gravity, wall jump, dash, spikes, respawn,
// the fake wall of room 0 and its fruit, the room title, an "end of demo" screen at the exit).
// code.lua translated in the cart's order; the assignments the cart makes in _draw run at the
// end of game_update (draw_phase), only on the frames where the cart's _draw runs (freeze ==
// 0). Gameplay state is exact and traced (test_celeste.c); cosmetic state (smoke, shake, hair,
// dead particles, the view camera) is approximated and draws its own random numbers (rt_rand):
// room 0 has no gameplay rnd (decision Q13).
#include "celeste.h"
#include "room.h"
#include "gfx.h"
#ifdef STATE_HASH
#include "../../tools/m68kbench/bench.h"
#endif

Celeste S;

// ---------------------------------------------------------------- constants (z8lua bits)
#define F_0_05 FIXB(0x0ccc)
#define F_0_1 FIXB(0x1999)
#define F_0_15 FIXB(0x2666)
#define F_0_2 FIXB(0x3333)
#define F_0_21 FIXB(0x35c2)
#define F_0_25 FIXB(0x4000)
#define F_0_3 FIXB(0x4ccc)
#define F_0_4 FIXB(0x6666)
#define F_0_5 FIXB(0x8000)
#define F_0_6 FIXB(0x9999)
#define F_0_75 FIXB(0xc000)
#define F_SQRT_HALF FIXB(0xb504)       // 0.70710678118
#define F_1_5 FIXB(0x18000)
#define F_2_5 FIXB(0x28000)
#define F_D_HALF FIXB(0x38914)         // 5 * 0.70710678118, folded as z8lua does

// sin(i / 40) for i = 0..39, printed by z8lua (fruit bobbing, gameplay: it moves the hit box)
static const fix sin40[40] = {
    FIXB(0x00000), FIXB(-0x02816), FIXB(-0x04f17), FIXB(-0x0743d), FIXB(-0x09671),
    FIXB(-0x0b505), FIXB(-0x0cf22), FIXB(-0x0e417), FIXB(-0x0f37a), FIXB(-0x0fcd8),
    FIXB(-0x10000), FIXB(-0x0fcd8), FIXB(-0x0f37a), FIXB(-0x0e417), FIXB(-0x0cf22),
    FIXB(-0x0b505), FIXB(-0x09671), FIXB(-0x0743d), FIXB(-0x04f17), FIXB(-0x02816),
    FIXB(0x00000), FIXB(0x02816), FIXB(0x04f17), FIXB(0x0743d), FIXB(0x09671),
    FIXB(0x0b505), FIXB(0x0cf22), FIXB(0x0e417), FIXB(0x0f37a), FIXB(0x0fcd8),
    FIXB(0x10000), FIXB(0x0fcd8), FIXB(0x0f37a), FIXB(0x0e417), FIXB(0x0cf22),
    FIXB(0x0b505), FIXB(0x09671), FIXB(0x0743d), FIXB(0x04f17), FIXB(0x02816),
};

static u8 btn(u32 k) { return input_held(k) != 0; }
static void sfx(s8 n) { if (S.nsfx < sizeof(S.sfx_ev)) S.sfx_ev[S.nsfx++] = n; }
static void psfx(s8 n) { if (S.sfx_timer <= 0) sfx(n); }

static u8 level_index(void) { return (S.room_x & 7) + S.room_y * 8; }

// ---------------------------------------------------------------- helpers (cart order)
static fix appr(fix val, fix target, fix amount)
{
    return val > target ? p8_max(val - amount, target) : p8_min(val + amount, target);
}
static fix sign(fix v) { return v > 0 ? FIX(1) : v < 0 ? FIX(-1) : 0; }
static s16 isign(s16 v) { return v > 0 ? 1 : v < 0 ? -1 : 0; }

static u8 tile_at(s16 x, s16 y) { return room_tiles[(y << 4) + x]; }   // room 0 only

// for i = max(0, flr(x/8)), min(15, (x+w-1)/8): on whole pixels the bounds are shifts (a
// fraction of y never changes them: the fruit passes its integer part, see celeste.h)
#define TILE_LOOP(x, y, w, h)                                                          \
    s16 i, j, i0 = (x) >> 3, j0 = (y) >> 3, il = ((x) + (w) - 1) >> 3, jl = ((y) + (h) - 1) >> 3; \
    if (i0 < 0) i0 = 0;                                                                \
    if (j0 < 0) j0 = 0;                                                                \
    if (il > 15) il = 15;                                                              \
    if (jl > 15) jl = 15;                                                              \
    for (i = i0; i <= il; i++)                                                         \
        for (j = j0; j <= jl; j++)

static u8 tile_flag_at(s16 x, s16 y, s16 w, s16 h, u8 mask)
{
    TILE_LOOP(x, y, w, h) if (tile_flags[tile_at(i, j)] & mask) return 1;
    return 0;
}
#define solid_at(x, y, w, h) tile_flag_at(x, y, w, h, 1)        // flag 0
#define ice_at(x, y, w, h) tile_flag_at(x, y, w, h, 16)         // flag 4

static u8 spikes_at(s16 x, s16 y, s16 w, s16 h, fix xspd, fix yspd)
{
    TILE_LOOP(x, y, w, h) {
        u8 t = tile_at(i, j);
        if (t == 17 && (((y + h - 1) & 7) >= 6 || y + h == j * 8 + 8) && yspd >= 0)
            return 1;
        else if (t == 27 && (y & 7) <= 2 && yspd <= 0)
            return 1;
        else if (t == 43 && (x & 7) <= 2 && xspd <= 0)
            return 1;
        else if (t == 59 && (((x + w - 1) & 7) >= 6 || x + w == i * 8 + 8) && xspd >= 0)
            return 1;
    }
    return 0;
}

// ---------------------------------------------------------------- the objects list
// PICO-8's all()/foreach(): the index does not advance when the list got shorter since the
// last step (the current element was deleted); elements added meanwhile are visited at the end
void for_all(ObjFn f)
{
    u8 i = 0, n = S.nobj;
    for (;;) {
        if (S.nobj >= n) i++;
        n = S.nobj;
        if (i > S.nobj) break;
        f(&S.pool[S.order[i - 1]]);
    }
}

void destroy_object(Obj *o)            // del(objects, o): shift the rest down
{
    u8 slot = o->slot, k;
    for (k = 0; k < S.nobj; k++)
        if (S.order[k] == slot) {
            for (; k + 1 < S.nobj; k++) S.order[k] = S.order[k + 1];
            S.nobj--;
            S.ntype[o->type]--;
            S.used[slot] = 2;          // freed next frame: its update may still be running
            return;
        }
}

// obj.collide(type, ox, oy): the first object of that type, in list order, overlapping.
// x is whole on both sides; y too unless one of them is the bobbing fruit (yf)
static Obj *collide(Obj *o, u8 type, s16 ox, s16 oy)
{
    u8 k;
    s16 l = o->x + o->hbx + ox, t = o->y + o->hby + oy;
    s16 r = l + o->hbw, b = t + o->hbh;
    if (!S.ntype[type]) return RT_NULL;            // most checks: no such object in the room
    for (k = 0; k < S.nobj; k++) {
        Obj *p = &S.pool[S.order[k]];
        s16 pl, pt;
        if (p->type != type || p == o || !p->collideable) continue;
        pl = p->x + p->hbx;
        if (pl + p->hbw <= l || pl >= r) continue;
        pt = p->y + p->hby;
        if (o->yf | p->yf) {
            fix ft = FIX(t) + o->yf, fpt = FIX(pt) + p->yf;
            if (fpt + FIX(p->hbh) > ft && fpt < ft + FIX(o->hbh)) return p;
        } else if (pt + p->hbh > t && pt < b)
            return p;
    }
    return RT_NULL;
}
#define check(o, type, ox, oy) (collide(o, type, ox, oy) != RT_NULL)

static u8 is_solid(Obj *o, s16 ox, s16 oy)
{
    if (oy > 0 && !check(o, T_PLATFORM, ox, 0) && check(o, T_PLATFORM, ox, oy)) return 1;
    return solid_at(o->x + o->hbx + ox, o->y + o->hby + oy, o->hbw, o->hbh)
        || check(o, T_FALLFLOOR, ox, oy) || check(o, T_FAKEWALL, ox, oy);
}
static u8 is_ice(Obj *o, s16 ox, s16 oy)
{
    return ice_at(o->x + o->hbx + ox, o->y + o->hby + oy, o->hbw, o->hbh);
}

// amount is whole (move() rounds it); abs(amount) + 1 steps from 0: the cart's own quirk, kept
static void move_x(Obj *o, s16 amount, s16 start)
{
    if (o->solids) {
        s16 step = isign(amount), i, n = amount < 0 ? -amount : amount;
        for (i = start; i <= n; i++) {
            if (!is_solid(o, step, 0))
                o->x += step;
            else { o->spdx = 0; o->remx = 0; break; }
        }
    } else
        o->x += amount;
}
static void move_y(Obj *o, s16 amount)
{
    if (o->solids) {
        s16 step = isign(amount), i, n = amount < 0 ? -amount : amount;
        for (i = 0; i <= n; i++) {
            if (!is_solid(o, 0, step))
                o->y += step;
            else { o->spdy = 0; o->remy = 0; break; }
        }
    } else
        o->y += amount;
}
static void move(Obj *o, fix ox, fix oy)
{
    s16 amount;
    o->remx += ox;
    amount = FIX_INT(o->remx + F_0_5);           // flr(rem + 0.5)
    o->remx -= FIX(amount);
    move_x(o, amount, 0);
    o->remy += oy;
    amount = FIX_INT(o->remy + F_0_5);
    o->remy -= FIX(amount);
    move_y(o, amount);
}

static void create_hair(Obj *o)
{
    u8 i;
    for (i = 0; i < 5; i++) { S.hair[i][0] = o->x << 4; S.hair[i][1] = o->y << 4; }
}

// ---------------------------------------------------------------- object types
static void restart_room(void) { S.will_restart = 1; S.delay_restart = 15; }

static void kill_player(Obj *o)
{
    static const s8 dirs[8][2] = { {0, 48}, {-34, 34}, {-48, 0}, {-34, -34},   // sin, cos * 3
                                   {0, -48}, {34, -34}, {48, 0}, {34, 34} };   // in 12.4
    u8 d;
    S.sfx_timer = 12;
    sfx(0);
    S.deaths++;
    S.shake = 10;
    destroy_object(o);
    S.ndead = 8;
    for (d = 0; d < 8; d++) {
        DeadP *p = &S.dead[d];
        p->x = (o->x + 4) << 4; p->y = (o->y + 4) << 4;
        p->t = 10; p->sx = dirs[d][0]; p->sy = dirs[d][1];
    }
    restart_room();
}

static void smoke_init(Obj *o)         // cosmetic: rt_rand instead of the cart's 5 rnd calls
{
    u16 r = rt_rand();
    o->spr = FIX(29);
    o->spdy = -F_0_1;
    o->spdx = F_0_3 + (fix)(r & 0x3333);           // 0.3 + rnd(0.2)
    o->x += (s16)((r >> 14) & 1) - 1;              // -1 + rnd(2), floored
    o->y += (s16)((r >> 15) & 1) - 1;
    o->flipx = (r >> 12) & 1;
    o->flipy = (r >> 13) & 1;
    o->solids = 0;
}

static void player_init(Obj *o)
{
    o->djump = S.max_djump;
    o->hbx = 1; o->hby = 3; o->hbw = 6; o->hbh = 5;
    create_hair(o);
}

static void player_update(Obj *o)
{
    s16 input, v_input;
    u8 on_ground, on_ice, jump, dash;
    if (S.pause_player) return;
    input = btn(K_RIGHT) ? 1 : (btn(K_LEFT) ? -1 : 0);

    if (spikes_at(o->x + o->hbx, o->y + o->hby, o->hbw, o->hbh, o->spdx, o->spdy))
        kill_player(o);                // the update goes on, as in the cart
    if (o->y > 128) kill_player(o);

    on_ground = is_solid(o, 0, 1);
    on_ice = is_ice(o, 0, 1);
    if (on_ground && !o->was_on_ground) init_object(T_SMOKE, o->x, o->y + 4);

    jump = btn(K_JUMP) && !o->p_jump;
    o->p_jump = btn(K_JUMP);
    if (jump) o->jbuffer = 4;
    else if (o->jbuffer > 0) o->jbuffer--;

    dash = btn(K_DASH) && !o->p_dash;
    o->p_dash = btn(K_DASH);

    if (on_ground) {
        o->grace = 6;
        if (o->djump < S.max_djump) { psfx(54); o->djump = S.max_djump; }
    } else if (o->grace > 0)
        o->grace--;

    o->dash_effect_time -= FIX(1);
    if (o->dash_time > 0) {
        init_object(T_SMOKE, o->x, o->y);
        o->dash_time--;
        o->spdx = appr(o->spdx, o->dtx, o->dax);
        o->spdy = appr(o->spdy, o->dty, o->day);
    } else {
        // move
        fix maxrun = FIX(1), accel = F_0_6, deccel = F_0_15, maxfall, gravity;
        if (!on_ground)
            accel = F_0_4;
        else if (on_ice) {
            accel = F_0_05;
            if (input == (o->flipx ? -1 : 1)) accel = F_0_05;
        }
        if (p8_abs(o->spdx) > maxrun)
            o->spdx = appr(o->spdx, sign(o->spdx), deccel);           // sign * maxrun (1)
        else
            o->spdx = appr(o->spdx, FIX(input), accel);               // input * maxrun

        // facing
        if (o->spdx != 0) o->flipx = o->spdx < 0;

        // gravity
        maxfall = FIX(2);
        gravity = F_0_21;
        if (p8_abs(o->spdy) <= F_0_15) gravity = FIXB(0x1ae1);       // 0.21 * 0.5 (floored)

        // wall slide
        if (input != 0 && is_solid(o, input, 0) && !is_ice(o, input, 0)) {
            maxfall = F_0_4;
            if (rt_rand() < 13107) init_object(T_SMOKE, o->x + input * 6, o->y);   // rnd(10) < 2
        }
        if (!on_ground) o->spdy = appr(o->spdy, maxfall, gravity);

        // jump
        if (o->jbuffer > 0) {
            if (o->grace > 0) {        // normal jump
                psfx(1);
                o->jbuffer = 0;
                o->grace = 0;
                o->spdy = FIX(-2);
                init_object(T_SMOKE, o->x, o->y + 4);
            } else {                   // wall jump
                s16 wall_dir = is_solid(o, -3, 0) ? -1 : is_solid(o, 3, 0) ? 1 : 0;
                if (wall_dir != 0) {
                    psfx(2);
                    o->jbuffer = 0;
                    o->spdy = FIX(-2);
                    o->spdx = FIX(-wall_dir * 2);                     // -wall_dir * (maxrun + 1)
                    if (!is_ice(o, wall_dir * 3, 0)) init_object(T_SMOKE, o->x + wall_dir * 6, o->y);
                }
            }
        }

        // dash (d_full = 5, d_half = 5 * 0.70710678118)
        if (o->djump > 0 && dash) {
            init_object(T_SMOKE, o->x, o->y);
            o->djump--;
            o->dash_time = 4;
            S.has_dashed = 1;
            o->dash_effect_time = FIX(10);
            v_input = btn(K_UP) ? -1 : (btn(K_DOWN) ? 1 : 0);
            if (input != 0) {
                if (v_input != 0) {
                    o->spdx = input < 0 ? -F_D_HALF : F_D_HALF;
                    o->spdy = v_input < 0 ? -F_D_HALF : F_D_HALF;
                }
                else { o->spdx = FIX(input * 5); o->spdy = 0; }
            } else if (v_input != 0) {
                o->spdx = 0; o->spdy = FIX(v_input * 5);
            } else {
                o->spdx = o->flipx ? FIX(-1) : FIX(1); o->spdy = 0;
            }
            psfx(3);
            S.freeze = 2;
            S.shake = 6;
            o->dtx = sign(o->spdx) << 1;
            o->dty = sign(o->spdy) << 1;
            o->dax = F_1_5;
            o->day = F_1_5;
            if (o->spdy < 0) o->dty = FIXB(-0x18000);                 // -2 * 0.75
            if (o->spdy != 0) o->dax = FIXB(0x10f86);                 // 1.5 * 0.70710678118
            if (o->spdx != 0) o->day = FIXB(0x10f86);
        } else if (dash && o->djump <= 0) {
            psfx(9);
            init_object(T_SMOKE, o->x, o->y);
        }
    }

    // animation
    o->spr_off += F_0_25;
    if (!on_ground)
        o->spr = is_solid(o, input, 0) ? FIX(5) : FIX(3);
    else if (btn(K_DOWN))
        o->spr = FIX(6);
    else if (btn(K_UP))
        o->spr = FIX(7);
    else if (o->spdx == 0 || (!btn(K_LEFT) && !btn(K_RIGHT)))
        o->spr = FIX(1);
    else
        o->spr = FIX(1) + p8_mod2k(o->spr_off, 2);

    // next level: the end of this slice (decision Q4: the "end of demo" screen)
    if (o->y < -4 && level_index() < 30) S.mode = M_END;

    o->was_on_ground = on_ground;
}

static void spawn_init(Obj *o)
{
    sfx(4);
    o->spr = FIX(3);
    o->ty = o->y;
    o->y = 128;
    o->spdy = FIX(-4);
    o->solids = 0;
    create_hair(o);
}

static void spawn_update(Obj *o)
{
    if (o->state == 0) {               // jumping up
        if (o->y < o->ty + 16) { o->state = 1; o->delay = 3; }
    } else if (o->state == 1) {        // falling
        o->spdy += F_0_5;
        if (o->spdy > 0 && o->delay > 0) { o->spdy = 0; o->delay--; }
        if (o->spdy > 0 && o->y > o->ty) {
            o->y = o->ty;
            o->spdx = o->spdy = 0;
            o->state = 2;
            o->delay = 5;
            S.shake = 5;
            init_object(T_SMOKE, o->x, o->y + 4);
            sfx(5);
        }
    } else if (o->state == 2) {        // landing
        o->delay--;
        o->spr = FIX(6);
        if (o->delay < 0) {
            destroy_object(o);
            init_object(T_PLAYER, o->x, o->y);
        }
    }
}

static void fake_wall_update(Obj *o)
{
    Obj *hit;
    o->hbx = -1; o->hby = -1; o->hbw = 18; o->hbh = 18;
    hit = collide(o, T_PLAYER, 0, 0);
    if (hit != RT_NULL && hit->dash_effect_time > 0) {
        hit->spdx = hit->spdx > 0 ? -F_1_5 : hit->spdx < 0 ? F_1_5 : 0;   // -sign(spd.x) * 1.5
        hit->spdy = -F_1_5;
        hit->dash_time = -1;
        S.sfx_timer = 20;
        sfx(16);
        destroy_object(o);
        init_object(T_SMOKE, o->x, o->y);
        init_object(T_SMOKE, o->x + 8, o->y);
        init_object(T_SMOKE, o->x, o->y + 8);
        init_object(T_SMOKE, o->x + 8, o->y + 8);
        init_object(T_FRUIT, o->x + 4, o->y + 4);
    }
    o->hbx = 0; o->hby = 0; o->hbw = 16; o->hbh = 16;
}

static void fruit_update(Obj *o)
{
    Obj *hit = collide(o, T_PLAYER, 0, 0);
    fix y;
    if (hit != RT_NULL) {
        hit->djump = S.max_djump;
        S.sfx_timer = 20;
        sfx(13);
        S.got_fruit |= 1UL << level_index();
        hit = init_object(T_LIFEUP, o->x, o->y);
        if (hit) hit->yf = o->yf;      // the "1000" starts at the fruit's fractional y
        destroy_object(o);
    }
    if (++o->off40 == 40) o->off40 = 0;
    y = o->start + p8_mul(sin40[o->off40], F_2_5);
    o->y = FIX_INT(y);
    o->yf = (u16)y;
}

static void obj_init(Obj *o)
{
    switch (o->type) {
    case T_SPAWN: spawn_init(o); break;
    case T_PLAYER: player_init(o); break;
    case T_SMOKE: smoke_init(o); break;
    case T_TITLE: o->delay = 5; break;
    case T_FRUIT: o->start = FIX(o->y); break;
    case T_LIFEUP:
        o->spdy = -F_0_25; o->delay = 30; o->x -= 2; o->y -= 4; o->solids = 0;
        break;
    }
}

static void obj_update(Obj *o)         // the _update loop body
{
    move(o, o->spdx, o->spdy);
    switch (o->type) {
    case T_SPAWN: spawn_update(o); break;
    case T_PLAYER: player_update(o); break;
    case T_SMOKE:
        o->spr += F_0_2;
        if (o->spr >= FIX(32)) destroy_object(o);
        break;
    case T_FAKEWALL: fake_wall_update(o); break;
    case T_FRUIT: fruit_update(o); break;
    case T_LIFEUP: if (--o->delay <= 0) destroy_object(o); break;
    }
}

Obj *init_object(u8 type, s16 x, s16 y)
{
    static const u8 tile[] = { 0, 1, 0, 0, 0, 64, 26, 0, 0, 0 };
    Obj *o;
    u8 k;
    if ((type == T_FAKEWALL || type == T_FRUIT) && (S.got_fruit >> level_index() & 1)) return RT_NULL;
    for (k = 0; k < MAXOBJ && S.used[k]; k++) ;
    if (k == MAXOBJ) { S.overflow = 1; return RT_NULL; }
    o = &S.pool[k];
    {
        u8 *b = (u8 *)o, n;
        for (n = 0; n < sizeof(Obj); n++) b[n] = 0;
    }
    S.used[k] = 1;
    o->type = type;
    o->slot = k;
    o->collideable = 1;
    o->solids = 1;
    o->spr = FIX(tile[type]);
    o->x = x; o->y = y;
    o->hbw = 8; o->hbh = 8;
    S.order[S.nobj++] = k;
    S.ntype[type]++;
    obj_init(o);
    return o;
}

// ---------------------------------------------------------------- rooms
static u8 room_l[128 * 16], room_d[128 * 16];   // room pre-render: 128x128, 16 bytes a row

static void render_room(void)          // once per room load (decision Q10)
{
    u8 tx, ty, r;
    for (ty = 0; ty < 16; ty++)
        for (tx = 0; tx < 16; tx++) {
            const u8 *l = cell_l + room_cells[ty * 16 + tx] * 8, *d = cell_d + room_cells[ty * 16 + tx] * 8;
            u8 *pl = room_l + ty * 128 + tx, *pd = room_d + ty * 128 + tx;
            for (r = 0; r < 8; r++) { pl[r * 16] = l[r]; pd[r * 16] = d[r]; }
        }
}

static void load_room(u8 x, u8 y)
{
    s16 tx, ty;
    S.has_dashed = 0;
    S.has_key = 0;
    for_all(destroy_object);
    S.room_x = x; S.room_y = y;
    if (x == 0 && y == 0) {
        for (tx = 0; tx < 16; tx++)
            for (ty = 0; ty < 16; ty++) {
                u8 t = tile_at(tx, ty);
                if (t == 1) init_object(T_SPAWN, tx * 8, ty * 8);
                else if (t == 64) init_object(T_FAKEWALL, tx * 8, ty * 8);
                else if (t == 11 || t == 12 || t == 18 || t == 22 || t == 23 || t == 26 || t == 28 ||
                         t == 8 || t == 20 || t == 86 || t == 96 || t == 118)
                    S.unsupported = 1;
            }
        render_room();
    } else
        S.unsupported = 1;
    init_object(T_TITLE, 0, 0);        // not the title room (31): no title screen in this slice
}

static void begin_game(void)
{
    S.frames = 0; S.seconds = 0; S.minutes = 0;
    load_room(0, 0);
}

// ---------------------------------------------------------------- the cart's _draw state changes
#define VIEW_MAX (128 - RT_H)          // 28: the lowest view
#define VIEW_DEAD 8                    // view camera dead zone (decision Q5)
#define VIEW_LEAD 8                    // look ahead: the vertical speed times this many frames

static Obj *player_or_spawn(void)
{
    u8 k;
    for (k = 0; k < S.nobj; k++) {
        Obj *o = &S.pool[S.order[k]];
        if (o->type == T_PLAYER || o->type == T_SPAWN) return o;
    }
    return RT_NULL;
}

static void draw_obj(Obj *o)
{
    if (o->type == T_PLAYER) {
        if (o->x < -1 || o->x > 121) {             // clamp in screen
            o->x = o->x < -1 ? -1 : 121;
            o->spdx = 0;
        }
    } else if (o->type == T_TITLE) {
        if (--o->delay < -30) destroy_object(o);
    }
    if (o->type == T_PLAYER || o->type == T_SPAWN) {   // hair (cosmetic: h += (last - h) / 1.5)
        s16 lx = (o->x + 4 - ((o->type == T_PLAYER && o->flipx) ? -2 : 2)) << 4;
        s16 ly = (o->y + (btn(K_DOWN) ? 4 : 3)) << 4;
        u8 i;
        for (i = 0; i < 5; i++) {
            s16 dx = lx - S.hair[i][0], dy = ly + 8 - S.hair[i][1];
            S.hair[i][0] += (dx >> 1) + (dx >> 3) + (dx >> 5);
            S.hair[i][1] += (dy >> 1) + (dy >> 3) + (dy >> 5);
            lx = S.hair[i][0]; ly = S.hair[i][1];
        }
    }
}

static void view_camera(void)          // cosmetic: the 100 rows of the room on screen
{
    Obj *p = player_or_spawn();
    s16 target, d;
    if (!p) return;                    // dead: the view stays
    if (p->type == T_SPAWN) { S.view_y = VIEW_MAX; return; }   // the entry from below
    target = p->y + 4 - RT_H / 2 + FIX_INT(p->spdy * VIEW_LEAD);
    d = target - S.view_y;
    if (d > VIEW_DEAD) S.view_y += (d - VIEW_DEAD + 1) >> 1;
    else if (d < -VIEW_DEAD) S.view_y += (d + VIEW_DEAD) >> 1;
    if (S.view_y < 0) S.view_y = 0;
    if (S.view_y > VIEW_MAX) S.view_y = VIEW_MAX;
}

static void draw_phase(void)
{
    u8 k;
    for_all(draw_obj);
    for (k = 0; k < S.ndead;) {        // dead particles: foreach + del
        DeadP *d = &S.dead[k];
        d->x += d->sx; d->y += d->sy;
        if (--d->t <= 0) { S.dead[k] = S.dead[--S.ndead]; continue; }
        k++;
    }
    view_camera();
}

// ---------------------------------------------------------------- runtime hooks
void game_init(void)
{
    u8 *b = (u8 *)&S;
    u16 n;
    for (n = 0; n < sizeof(S); n++) b[n] = 0;
    rt_state = &S;
    rt_state_size = sizeof(S);
    S.max_djump = 1;                   // _init: title_screen() (its room 31 has no objects)
    S.view_y = VIEW_MAX;
}

void game_scenario(u16 n)
{
    // 0: the game starts in room 0 (p8trace --pre "begin_game()")
    // 1: the player already standing on the spawn tile (README: the matching --pre)
    begin_game();
    if (n == 1) {
        u8 k;
        for (k = 0; k < S.nobj; k++) {
            Obj *o = &S.pool[S.order[k]];
            if (o->type == T_SPAWN) { destroy_object(o); init_object(T_PLAYER, o->x, o->ty); break; }
        }
    }
}

// Hash of the gameplay state (every traced value, every object but the cosmetic smoke): the TI
// binary prints it per frame under ti-cycles (-DSTATE_HASH, `make tihash`) and test_celeste
// --hash on the PC, so the 68000 build is checked against the PC build, itself checked
// against the cart.
static u32 hv;
static void hmix(u32 v) { hv = ((hv << 5) | (hv >> 27)) ^ v; }
u32 state_hash(void)
{
    u8 k;
    hv = 0;
    hmix(S.mode); hmix(S.frames); hmix(S.seconds); hmix(S.freeze); hmix(S.shake); hmix(S.deaths);
    hmix(S.will_restart); hmix(S.delay_restart); hmix(S.sfx_timer); hmix(S.has_dashed);
    hmix(S.got_fruit); hmix(S.nsfx);
    for (k = 0; k < S.nsfx; k++) hmix(S.sfx_ev[k]);
    for (k = 0; k < S.nobj; k++) {
        Obj *o = &S.pool[S.order[k]];
        if (o->type == T_SMOKE) continue;
        hmix(o->type); hmix(o->collideable); hmix(o->solids); hmix(o->flipx); hmix(o->flipy);
        hmix(o->hbx); hmix(o->hby); hmix(o->hbw); hmix(o->hbh);
        hmix(o->x); hmix(o->y); hmix(o->yf); hmix(o->spdx); hmix(o->spdy); hmix(o->remx); hmix(o->remy);
        hmix(o->spr); hmix(o->p_jump); hmix(o->p_dash); hmix(o->was_on_ground); hmix(o->grace);
        hmix(o->jbuffer); hmix(o->djump); hmix(o->dash_time); hmix(o->dash_effect_time);
        hmix(o->dtx); hmix(o->dty); hmix(o->dax); hmix(o->day); hmix(o->spr_off); hmix(o->state);
        hmix(o->delay); hmix(o->ty); hmix(o->start); hmix(o->off40);
    }
    return hv;
}

static u8 update(void);
u8 game_update(void)
{
    u8 r = update();
#ifdef STATE_HASH
    if (r) BENCH_VALUE(state_hash());
#endif
    return r;
}

static u8 update(void)
{
    u8 k;
    if (input_held(K_ESC)) return 0;
    if (S.mode == M_END) {             // "end of demo": [ENTER] plays room 0 again
        if (input_pressed(K_ENTER)) { game_init(); game_scenario(0); }
        return 1;
    }
    for (k = 0; k < MAXOBJ; k++) if (S.used[k] == 2) S.used[k] = 0;
    S.nsfx = 0;

    S.frames = S.frames == 29 ? 0 : S.frames + 1;
    if (S.frames == 0 && level_index() < 30) {
        S.seconds = S.seconds == 59 ? 0 : S.seconds + 1;
        if (S.seconds == 0) S.minutes++;
    }
    if (S.sfx_timer > 0) S.sfx_timer--;

    if (S.freeze > 0)
        S.freeze--;                    // the cart's _update returns here
    else {
        if (S.shake > 0) {             // screen shake (vertical only, decision Q11)
            S.shake--;
            S.cam_y = 0;
            if (S.shake > 0) S.cam_y = (s8)(((rt_rand() & 0xff) * 5) >> 8) - 2;   // -2 + rnd(5)
        }
        if (S.will_restart && S.delay_restart > 0) {
            if (--S.delay_restart <= 0) { S.will_restart = 0; load_room(S.room_x, S.room_y); }
        }
        for_all(obj_update);
    }
    if (S.freeze == 0 && S.mode == M_PLAY) draw_phase();   // the cart's _draw runs only then
    return 1;
}

// ---------------------------------------------------------------- render (placeholders, pure)
// The room's 100 visible rows are copied from the pre-render into the hidden planes (byte
// aligned: the room starts at x = 16, byte 2 of each 30-byte row); the side bands of the HUD
// are cleared to white in the same pass (bytes 0-1 and 18-19).
static void copy_room(s16 top)
{
    s16 r;
    for (r = 0; r < RT_H; r++) {
        s16 sr = top + r;
        u32 *dl = (u32 *)((u8 *)rt_light + r * RT_PBYTES + ROOM_X / 8);
#ifndef RT_MONO
        u32 *dd = (u32 *)((u8 *)rt_dark + r * RT_PBYTES + ROOM_X / 8);
        ((u16 *)dd)[-1] = ((u16 *)dd)[8] = 0;
#endif
        ((u16 *)dl)[-1] = ((u16 *)dl)[8] = 0;
        if (sr < 0 || sr > 127) {      // shaken past the room: black
            dl[0] = dl[1] = dl[2] = dl[3] = 0xffffffffUL;
#ifndef RT_MONO
            dd[0] = dd[1] = dd[2] = dd[3] = 0xffffffffUL;
#endif
            continue;
        }
        {
#ifdef RT_MONO
            const u32 *s = (const u32 *)(room_d + sr * 16);      // mono: the dark plane
            dl[0] = s[0]; dl[1] = s[1]; dl[2] = s[2]; dl[3] = s[3];
#else
            const u32 *sl = (const u32 *)(room_l + sr * 16), *sd = (const u32 *)(room_d + sr * 16);
            dl[0] = sl[0]; dl[1] = sl[1]; dl[2] = sl[2]; dl[3] = sl[3];
            dd[0] = sd[0]; dd[1] = sd[1]; dd[2] = sd[2]; dd[3] = sd[3];
#endif
        }
    }
}

static void put_num(char *s, s16 v)    // small unsigned decimal, no sprintf
{
    char t[6];
    u8 n = 0;
    do { t[n++] = '0' + v % 10; v /= 10; } while (v && n < 5);
    while (n) *s++ = t[--n];
    *s = 0;
}

static void render_end(void)
{
    char b[8];
    u8 berries = 0;
    u32 g;
    for (g = S.got_fruit; g; g >>= 1) berries += g & 1;
    draw_clear();
    draw_text(40, 20, "END OF THE DEMO", F_MEDIUM, C_BLACK);
    put_num(b, S.minutes); draw_text(40, 40, "time", F_SMALL, C_BLACK); draw_text(80, 40, b, F_SMALL, C_BLACK);
    b[0] = 0;
    {
        char m[4];
        put_num(m, S.seconds);
        draw_text(92, 40, m, F_SMALL, C_BLACK);
    }
    put_num(b, S.deaths); draw_text(40, 50, "deaths", F_SMALL, C_BLACK); draw_text(80, 50, b, F_SMALL, C_BLACK);
    put_num(b, berries); draw_text(40, 60, "berries", F_SMALL, C_BLACK); draw_text(80, 60, b, F_SMALL, C_BLACK);
    draw_text(28, 82, "ENTER: again  ESC: quit", F_SMALL, C_BLACK);
}

#define SX(x) (ROOM_X + (x))
#define SY(y) ((y) - cy)

void game_render(void)
{
    s16 cy = S.view_y + S.cam_y;
    u8 k;
    char b[6];
    if (S.mode == M_END) { render_end(); return; }
    copy_room(cy);
    put_num(b, S.deaths); draw_text(2, 2, b, F_SMALL, C_BLACK);
    put_num(b, (S.got_fruit & 1)); draw_text(ROOM_X + 130, 2, b, F_SMALL, C_BLACK);
    for (k = 0; k < S.nobj; k++) {
        Obj *o = &S.pool[S.order[k]];
        s16 x = SX(o->x), y = SY(o->y);
        switch (o->type) {
        case T_PLAYER:
        case T_SPAWN: {                // hair under the outlined sprite (the cart's order)
            u8 i, zero = o->type == T_PLAYER && o->djump == 0;
            s16 n = FIX_INT(o->spr);
            if (zero)                  // light hair (no dash left) gets a dark ring to read
                for (i = 0; i < 5; i++) {
                    s16 hs = i < 2 ? 2 : 1;
                    draw_rect(SX(S.hair[i][0] >> 4) - hs - 1, SY(S.hair[i][1] >> 4) - hs - 1, 2 * hs + 3, 2 * hs + 3, C_BLACK);
                }
            for (i = 0; i < 5; i++) {
                s16 hs = i < 2 ? 2 : 1;
                draw_rect(SX(S.hair[i][0] >> 4) - hs, SY(S.hair[i][1] >> 4) - hs, 2 * hs + 1, 2 * hs + 1,
                          zero ? GFX_HAIR_0 : GFX_HAIR_1);
            }
            if (n < 1 || n > 7) n = 1;
            draw_sprite(x - 1, y - 1, &spr_player[zero][o->flipx][n - 1]);
            break;
        }
        case T_SMOKE:
            draw_sprite(x, y, &spr_smoke[FIX_INT(o->spr) - 29]);
            break;
        case T_FAKEWALL:
            draw_sprite(x, y, &spr_fakewall);
            break;
        case T_FRUIT:
            draw_sprite(x - 1, y - 1, &spr_fruit);
            break;
        case T_LIFEUP:
            if (y >= 0 && y < RT_H - 5) draw_text(x, y, "1000", F_SMALL, C_BLACK);
            break;
        case T_TITLE:
            if (o->delay < 0) {
                draw_rect(52, 42, 56, 13, C_BLACK);
                draw_rect(53, 43, 54, 11, C_WHITE);
                draw_text(68, 46, "100 m", F_SMALL, C_BLACK);
            }
            break;
        }
    }
    for (k = 0; k < S.ndead; k++) {
        DeadP *d = &S.dead[k];
        s16 r = d->t / 5;
        draw_rect(SX(d->x >> 4) - r, SY(d->y >> 4) - r, 2 * r + 1, 2 * r + 1, C_BLACK);
    }
}
