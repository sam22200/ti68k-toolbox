/* Native Beach mechanics. OBSERVED constants and TARGET adaptations: RE_NOTES.md.
 * No game/BIOS code runs here; only the local collision-angle data is generated.
 */
#include <string.h>
#include "windjam.h"
#ifdef STATE_HASH
#include "../../tools/m68kbench/bench.h"
#endif

typedef struct { s32 vx, vy, sx, sy; u16 steps, speed; } WjProfile;
typedef struct { s32 vx, vy; } WjRecoil;
#include "generated/tables.h"
#include "generated/advanced_tables.h"
typedef struct {
    s32 x1, y1; u16 hold1, power1, bonus1;
    s32 x2, y2; u16 hold2, power2, bonus2;
    s32 dx, dy; u16 profile;
} WjTimingDoor;
#include "generated/timing_doors.h"
Windjam st;

static WjPlayer *player(u16 port)
{
    /* Only two actors: constant addresses avoid a stride multiply on the 68000. */
    return port ? &st.player[1] : &st.player[0];
}

static s32 clamp_word(s32 value, u16 low, u16 high)
{
    s16 integer = value >> 16;
    if (integer < (s16)low) return ((s32)low << 16) | ((u32)value & 65535UL);
    if (integer > (s16)high) return ((s32)high << 16) | ((u32)value & 65535UL);
    return value;
}

void wj_reset(u8 owner)
{
    memset(&st, 0, sizeof st);
    st.seconds = 30;
    /* Reference-door history: Mita has already returned the opening disc. */
    st.player[0].hold = 27; st.player[0].power = 34; st.player[0].bonus = 2;
    st.player[1].hold = 14; st.player[1].power = 4; st.player[1].bonus = 8;
    st.player[0].x = 40L << 16;
    st.player[1].x = 280L << 16;
    st.player[0].y = st.player[1].y = 138L << 16;
    st.player[0].facing = 64; st.player[1].facing = 192;
    st.disc.mode = WJ_HELD;
    st.disc.owner = owner;
    st.disc.x = player(owner)->x;
    st.disc.y = player(owner)->y;
}

void wj_walk(u16 port, u16 keys)
{
    WjPlayer *p = player(port);
    s16 dx = ((keys & K_RIGHT) != 0) - ((keys & K_LEFT) != 0);
    s16 dy = ((keys & K_DOWN) != 0) - ((keys & K_UP) != 0);
    s32 speed = dx && dy ? (port ? 0x19740L : 0x1c480L) : (port ? 0x24000L : 0x28000L);
    p->vx = dx ? (dx < 0 ? -speed : speed) : 0;
    p->vy = dy ? (dy < 0 ? -speed : speed) : 0;
    p->x = clamp_word(p->x + p->vx, port ? 180 : 27, port ? 292 : 141);
    p->y = clamp_word(p->y + p->vy, 76, 188);
}

static void dash_step(u16 port, u16 pad)
{
    WjPlayer *p = player(port);
    /* ROM 0800: signed fixed-point decay rounds negative components toward
     * zero. Keep fractions on court clamps, exactly as ordinary movement. */
    p->vx -= p->vx >> 3;
    p->vy -= p->vy >> 3;
    /* Original A hold keeps the dash animation's first event active. Motion
     * still decays. Native wall recovery always expires even while held. */
    if (p->dash_wall || !(pad & K_A)) p->dash_age++;
    if ((s16)((p->x + p->vx) >> 16) < (port ? 180 : 27) ||
        (s16)((p->x + p->vx) >> 16) > (port ? 292 : 141)) p->dash_wall = 1;
    p->x = clamp_word(p->x + p->vx, port ? 180 : 27, port ? 292 : 141);
    p->y = clamp_word(p->y + p->vy, 76, 188);
}

static void begin_dash(u16 port, u16 pad)
{
    WjPlayer *p = player(port);
    s16 dx = ((pad & K_RIGHT) != 0) - ((pad & K_LEFT) != 0);
    s16 dy = ((pad & K_DOWN) != 0) - ((pad & K_UP) != 0);
    s32 speed = dx && dy ? (port ? 476392L : 456120L) : (port ? 673792L : 645120L);
    p->vx = dx < 0 ? -speed : dx ? speed : 0;
    p->vy = dy < 0 ? -speed : dy ? speed : 0;
    p->dash_age = 1; p->dash_wall = 0;
    p->x = clamp_word(p->x + p->vx, port ? 180 : 27, port ? 292 : 141);
    p->y = clamp_word(p->y + p->vy, 76, 188);
}

static u8 direction_angle(s16 dx, s16 dy, u16 doubled)
{
    u16 x = dx < 0 ? -dx : dx, y = dy < 0 ? -dy : dy;
    u16 base, swap, tmp;
    /* Source caller doubles integer center differences before quantization. */
    base = dx <= 0 ? 0xc0 : 0;
    swap = dx <= 0;
    if (dy > 0) { base = dx <= 0 ? 0x80 : 0x40; swap = dx > 0; }
    if (doubled) { x <<= 1; y <<= 1; }
    if (swap) { tmp = x; x = y; y = tmp; }
    if (x + y < 64) { x <<= 3; y <<= 3; }
    return base + collision_angles[((y & 0x1f8) << 3) + ((x & 0x1f8) >> 3)];
}

static u8 contact_angle(s16 dx, s16 dy) { return direction_angle(dx, dy, 1); }

/* Keep the operands 16 bit. GCC4TI emits the same word multiply as the ROM;
 * isolating it prevents several products from promoting the shared operand. */
static s32 __attribute__((noinline)) signed_product(s16 a, s16 b) { return (s32)a * b; }
static u32 __attribute__((noinline)) unsigned_product(u16 a, u16 b) { return (u32)a * b; }

static s16 trig(u16 angle)
{
    angle &= 255;
    if (angle > 128) angle = (256 - angle) & 127;
    return advanced_trig[angle];
}

static void vector(u16 angle, u16 speed)
{
    st.disc.free_vx = signed_product(trig(angle + 64), speed) >> 1;
    st.disc.free_vy = signed_product(trig(angle), speed) >> 1;
}

static u16 distance_bucket(s16 dx, s16 dy)
{
    u16 x = dx < 0 ? -dx : dx, y = dy < 0 ? -dy : dy;
    u32 square = unsigned_product(x, x) + unsigned_product(y, y);
    u16 lo = 0, hi = 96;
    while (lo < hi) {
        u16 mid = (lo + hi + 1) >> 1;
        if (square >= distance_limits[mid]) lo = mid;
        else hi = mid - 1;
    }
    return lo;
}

static u8 advanced_ground(u8 mode)
{
    return mode == WJ_MITA || mode == WJ_YOO || mode == WJ_BOUNCE ||
        mode == WJ_CURVE_PLUS || mode == WJ_CURVE_MINUS;
}

static u8 curve_mode(u8 mode) { return mode == WJ_CURVE_PLUS || mode == WJ_CURVE_MINUS; }

/* Only nine of the ROM's 64 history bytes are read by 01D0CA. Keep the
 * exact recent samples, including neutral=UP=0, rather than debouncing. */
static u16 direction_bits(u16 pad)
{
    return ((pad & K_UP) ? 1 : 0) | ((pad & K_DOWN) ? 2 : 0) |
        ((pad & K_LEFT) ? 4 : 0) | ((pad & K_RIGHT) ? 8 : 0);
}

static void gesture_step(WjPlayer *p, u16 pad)
{
    u16 count = 8;
    while (count--) p->history[count + 1] = p->history[count];
    p->history[0] = gesture_directions[direction_bits(pad)];
}

static u8 gesture(const WjPlayer *p)
{
    const u8 *h = p->history;
    u16 i = 1, end;
    u8 first = *h, second, kind;
    while (i <= 4 && h[i] == first) i++;
    if (i > 4) return 0;
    second = h[i++];
    if (second == (u8)(first + 32)) kind = WJ_CURVE_MINUS;
    else if (second == (u8)(first - 32)) kind = WJ_CURVE_PLUS;
    else return 0;
    end = i + 4;
    while (i < end && h[i] == second) i++;
    if (i == end) return 0;
    if (p->power < 16 && h[i] != (u8)(second + (kind == WJ_CURVE_MINUS ? 32 : -32))) return 0;
    return kind;
}

static u8 lob_mode(u8 mode) { return mode == WJ_LOB || mode == WJ_SUPERLOB; }

u16 wj_contact(u16 port, s16 dx, s16 dy, u8 flipped)
{
    u16 hx = port ? 22 : 21;
    s16 cy = dy + (port ? 0 : 4);
    u16 ay = cy < 0 ? -cy : cy, ax = dx < 0 ? -dx : dx;
    u8 angle, start = port ? 0xe8 : 0xe4, end = port ? 0x98 : 0x9c, tmp;
    if (ax > hx || ay > 23) return 0;
    angle = contact_angle(dx, cy);
    if (flipped) { tmp = start; start = -end; end = -tmp; }
    return ((u8)(angle - start) <= (u8)(end - start) ? 0x400 : 0x800) | angle;
}

u8 wj_goal_zone(s32 y)
{
    u16 integer = y >> 16;
    return (u16)(integer - 120) < 48 ? 5 : 3;
}

s32 wj_vx(void)
{
    if ((st.disc.mode == WJ_FLIGHT && st.disc.dynamic) || st.disc.mode == WJ_BLOCK || advanced_ground(st.disc.mode) ||
        lob_mode(st.disc.mode) || st.disc.mode == WJ_DRAG) return st.disc.free_vx;
    if (st.disc.mode != WJ_FLIGHT) return 0;
    return st.disc.reverse ? -profiles[st.disc.profile].vx : profiles[st.disc.profile].vx;
}

s32 wj_vy(void)
{
    if ((st.disc.mode == WJ_FLIGHT && st.disc.dynamic) || st.disc.mode == WJ_BLOCK || advanced_ground(st.disc.mode) ||
        lob_mode(st.disc.mode) || st.disc.mode == WJ_DRAG) return st.disc.free_vy;
    return st.disc.mode == WJ_FLIGHT ? profiles[st.disc.profile].vy : 0;
}

u8 wj_disc_effect(void)
{
    WjDisc *d = &st.disc;
    if (d->mode == WJ_MITA || d->mode == WJ_YOO ||
        d->mode == WJ_SUPERLOB || d->mode == WJ_BOUNCE) return 2;
    if (d->mode != WJ_FLIGHT && !curve_mode(d->mode)) return 0;
    return (d->dynamic ? d->speed : profiles[d->profile].speed) >= 256 ? 1 : 0;
}

u16 wj_action(u16 port)
{
    WjPlayer *p = player(port);
    if (p->dash_age) return p->dash_age < (port ? 13 : 11) ? 0x800 : 0;
    if (p->ready) return 0xc00;
    if (p->charging) return 4;
    if (p->throwing && curve_mode(p->throw_kind))
        return ((p->throw_kind == WJ_CURVE_PLUS) ^ (port != 0)) ? 0x1404 : 0x1408;
    if (p->throwing) return p->throw_kind == WJ_LOB ? 0x140c :
        p->throw_kind == WJ_SUPERLOB ? 0x141c : p->throw_kind ? 0x1410 : 0x1400;
    if (p->dragged) return 0x1808;
    if (p->strong) return p->strong == WJ_SUPERLOB ? (p->recoil_reverse ? 0x1010 : 0x100c) : 0x1014;
    if (p->catching) return 0x1000;
    if (st.disc.mode == WJ_HELD && st.disc.owner == port) return 0x1004;
    return p->vx || p->vy ? 0x400 : 0;
}

u8 wj_prepare_hint(u16 port)
{
    WjPlayer *p = player(port);
    WjDisc *d = &st.disc;
    s16 dx, dy;
    s32 vx, vy;
    if (p->dash_age || p->vx || p->vy || p->ready || p->charging || p->throwing ||
        p->catching || p->strong || (d->mode != WJ_FLIGHT && !advanced_ground(d->mode))) return 0;
    vx = wj_vx(); vy = wj_vy();
    if (port ? vx <= 0 : vx >= 0) return 0;
    dx = ((d->x + vx * 4L) >> 16) - (p->x >> 16);
    dy = ((d->y + vy * 4L) >> 16) - (p->y >> 16);
    return (wj_contact(port, dx, dy, port != 0) & 0x400) != 0;
}

static void launch_advanced(u16 port)
{
    WjPlayer *p = player(port);
    WjDisc *d = &st.disc;
    u16 angle, bucket, hold = p->hold;
    s16 tx, ty, dx, dy;
    s32 speed;
    d->mode = p->throw_kind; d->pending = d->reverse = d->grace = 0;
    d->wave = 0; d->z = d->vz = 0;
    angle = port ? (p->aim == 1 ? 224 : p->aim == 2 ? 160 : 192) :
                  (p->aim == 1 ? 32 : p->aim == 2 ? 96 : 64);
    if (curve_mode(d->mode)) angle = p->curve_aim;
    else if (!lob_mode(d->mode)) angle = p->aim == 2 ? 128 : 0;
    /* Only integer coordinates are copied; the held disc supplies fractions. */
    d->x = (p->x & ~65535L) | ((u32)d->x & 65535UL);
    d->y = (p->y & ~65535L) | ((u32)d->y & 65535UL);
    d->x += port ? -0x100000L : 0x100000L;
    d->x += signed_product(trig(angle + 64), 64);
    d->y += signed_product(trig(angle), 64);
    d->angle = (u32)angle << 16;
    if (!lob_mode(d->mode)) {
        d->speed = profiles[hold_profiles[port][hold]].speed;
        if (curve_mode(d->mode)) {
            d->dynamic = 1;
            d->turn = signed_product(d->speed, (d->mode == WJ_CURVE_MINUS ? -1 : 1) * (s16)curve_turn[port]);
        } else if (d->mode == WJ_MITA) {
            d->speed += 16; d->anchor = angle;
            d->wave = ((angle != 0) ^ (port != 0)) ? 6 : 0;
            d->turn = signed_product(d->speed, d->wave ? -1536 : 1536);
        }
        vector(angle, d->speed);
        return;
    }
    if (d->mode == WJ_LOB) {
        u16 row = 0, y = (d->y >> 16) - 64, direction = lob_direction[angle >> 4] >> 1;
        if (hold > 62) hold = 62;
        tx = port ? 23 + (hold << 1) : 296 - (hold << 1);
        while (y >= 20 && row < 7) { y -= 20; row++; }
        /* TARGET deterministic jitter. The measured target bands are original;
         * the machine-wide Neo Geo RNG is intentionally not simulated. */
        ty = lob_targets[lob_rows[(row << 3) + direction] >> 1] + st.lob_seed;
        st.lob_seed = (st.lob_seed + 13) & 31;
    } else {
        u16 shift = (hold << 1) - (hold >> 1);
        tx = port ? 47 + shift : 272 - shift;
        if (profiles[hold_profiles[port][hold]].speed == 288) tx += port ? -8 : 8;
        ty = d->y >> 16;
        if (p->aim) {
            u16 distance = tx - (d->x >> 16);
            if ((s16)distance < 0) distance = -distance;
            distance >>= 3;
            if (distance > 45) distance = 45;
            ty += p->aim == 1 ? -superlob_dy[distance] : superlob_dy[distance];
            if (ty < 84) ty = 84;
            else if (ty > 188) ty = 188;
        }
    }
    dx = tx - (d->x >> 16); dy = ty - (d->y >> 16);
    d->target_x = tx; d->target_y = ty;
    bucket = distance_bucket(dx, dy);
    if (d->mode == WJ_LOB) { speed = lob_speed[bucket >> 1]; d->vz = lob_height[bucket >> 1]; }
    else { speed = superlob_speed[bucket]; d->vz = 0xa8000L; }
    angle = direction_angle(dx, dy, 0); d->angle = (u32)angle << 16;
    d->free_vx = signed_product((s16)(speed >> 6), trig(angle + 64)) >> 6;
    d->free_vy = signed_product((s16)(speed >> 6), trig(angle)) >> 6;
    d->speed = (d->free_vx < 0 ? -d->free_vx : d->free_vx) >> 11;
}

static void launch(u16 port)
{
    WjPlayer *p = player(port);
    if (p->throw_kind) { launch_advanced(port); return; }
    s32 offset = p->aim ? 0x12d400L : 0x140000L;
    st.disc.mode = WJ_FLIGHT;
    st.disc.dynamic = 0;
    st.disc.profile = hold_profiles[port][p->hold] + p->aim;
    st.disc.reverse = st.disc.pending = 0;
    st.disc.grace = 0;
    st.disc.x = p->x + (port ? -offset : offset);
    st.disc.y = p->y + (p->aim == 1 ? -0x2d400L : p->aim == 2 ? 0x2d400L : 0);
}

static void goal_points(u16 loser, u16 award)
{
    u16 winner = loser ^ 1;
    st.last_award = award;
    st.points[winner] += st.last_award;
    if (st.points[winner] > 99) st.points[winner] = 99;
    st.serve_to = loser;
    st.disc.mode = WJ_GOAL;
    st.disc.pending = 0;
    st.goal_age = 0;
}

static void goal(u16 loser) { goal_points(loser, wj_goal_zone(st.disc.y)); }

/* Quantized signed division used by the ROM's wall substeps. A reciprocal
 * and one correction replace its two DIVS operations; both products are words. */
static s32 substep(s32 v, u16 count)
{
    s16 word = (s16)(v >> 5);
    u16 magnitude = word < 0 ? -word : word, quotient;
    if (count == 1) quotient = magnitude;
    else {
        quotient = unsigned_product(magnitude, small_reciprocal[count]) >> 16;
        if (unsigned_product(quotient, count) > magnitude) quotient--;
    }
    return (word < 0 ? -(s32)quotient : (s32)quotient) * 32L;
}

static void sample_contact(void)
{
    WjDisc *d = &st.disc;
    u16 port = d->x >> 16 < 160 ? 0 : 1, contact;
    WjPlayer *p = player(port);
    s16 dx = (d->x >> 16) - (p->x >> 16), dy = (d->y >> 16) - (p->y >> 16);
    if (p->throwing || p->dragged) return;
    if (p->ready && port) {
        u16 ax, ay;
        u8 a;
        dx += 2; ax = dx < 0 ? -dx : dx; ay = dy < 0 ? -dy : dy;
        a = contact_angle(dx, dy);
        contact = ax <= 26 && ay <= 23 ?
            (((u8)(a - 0x68) <= (u8)(0x18 - 0x68) ? 0x400 : 0x800) | a) : 0;
    } else if (lob_mode(d->mode)) {
        u16 ax = dx < 0 ? -dx : dx;
        s16 cy = dy + (port ? 0 : 4);
        u16 ay = cy < 0 ? -cy : cy;
        contact = ax <= (port ? 23 : 22) && ay <= 23 ? 0x400 | contact_angle(dx, cy) : 0;
    } else contact = wj_contact(port, dx, dy, port != 0);
    if (contact) { d->pending = contact >> 8; d->defender = port; d->catch_angle = contact; }
}

/* Returns 1 for a top/bottom wall. Airborne wall geometry is inactive above
 * ground; landing still has its own native air bounds, as in the source. */
static u8 advanced_wall(void)
{
    WjDisc *d = &st.disc;
    s32 vx = d->free_vx, vy = d->free_vy, selected = vx < 0 ? -vx : vx;
    s32 x = d->x - vx, y = d->y - vy, sx, sy;
    u16 count;
    if (d->z > 0) return 0;
    if (selected < 65536L && (vy < 0 ? -vy : vy) < 65536L) {
        x = d->x; y = d->y; count = 1; sx = sy = 0;
    } else {
        if (selected < 65536L) selected = vy < 0 ? -vy : vy;
        else if ((u32)vy > (u32)selected) selected = vy;
        { s16 integer = selected >> 16; count = (integer < 0 ? -integer : integer) + 1; }
        if (count > 32) count = 32;
        sx = substep(vx, count); sy = substep(vy, count);
    }
    while (count--) {
        x += sx; y += sy;
        if ((y >> 16) < 72 || (y >> 16) >= 200) {
            d->wall_side = (y >> 16) < 72 ? 0 : 128;
            d->x = x; d->y = clamp_word(y, 72, 199);
            return 1;
        }
        if ((x >> 16) < 16 || (x >> 16) >= 304) {
            d->x = x; d->y = y; goal(x >> 16 < 160 ? 0 : 1); return 0;
        }
    }
    return 0;
}

static u16 wall_limit(u16 a, u16 normal)
{
    WjDisc *d = &st.disc;
    if (!d->owner) {
        if (!normal && (a < 16 || a >= 192)) a = 16;
        if (normal && a >= 113 && a < 193) a = 112;
    } else {
        if (!normal && (a < 65 || a >= 241)) a = 240;
        if (normal && a >= 64 && a < 144) a = 144;
    }
    return a;
}

static u16 reflected_angle(u16 a)
{
    WjDisc *d = &st.disc;
    u8 start = d->wall_side - 68, delta = (u8)(a - start);
    if (delta < 136) a = d->wall_side + (delta < 68 ? -72 : 72);
    return wall_limit((d->wall_side * 2 + 128 - a) & 255, d->wall_side);
}

static u16 curve_reflection(void)
{
    WjDisc *d = &st.disc;
    u16 normal = d->wall_side ^ 128, a = (d->angle >> 16) & 255;
    u8 start = normal - 68, delta = (u8)(a - start);
    if (delta < 136) a = normal + (delta < 68 ? -72 : 72);
    a = (normal * 2 + 128 - a +
        (d->mode == WJ_CURVE_MINUS ? -16 : d->mode == WJ_CURVE_PLUS ? 16 : 0)) & 255;
    return wall_limit(a, normal);
}

static void advanced_step(void)
{
    WjDisc *d = &st.disc;
    u8 air = lob_mode(d->mode), catchable = !air || (d->z >> 16 < 8 && d->vz < 0);
    u8 wall;
    if (curve_mode(d->mode)) {
        d->angle += (u32)d->turn;
        vector(d->angle >> 16, d->speed);
    } else if (d->mode == WJ_MITA) {
        s16 difference;
        d->angle += (u32)d->turn;
        difference = (s16)((d->angle >> 16) - d->anchor);
        if (difference < 0) difference = -difference;
        if (difference >= 128) {
            d->anchor += 128; d->angle = (u32)d->anchor << 16;
            d->wave = d->wave ? 0 : 6;
            d->turn = signed_product(d->speed, d->wave ? -1536 : 1536);
        }
        vector(d->angle >> 16, d->speed);
    }
    d->x += d->free_vx; d->y += d->free_vy;
    if (air) {
        d->z += d->vz; d->vz -= d->mode == WJ_LOB ? 0x2a00L : 0xa800L;
        if (d->z <= -0x100000L) d->z = -0x100000L;
    }
    wall = advanced_wall();
    if (d->mode == WJ_GOAL) return;
    if (wall) {
        if (d->mode == WJ_YOO && !d->wave) {
            d->wave = 4; d->speed -= (d->speed >> 1) + (d->speed >> 3);
            d->angle = (u32)(d->owner ? 192 : 64) << 16;
            vector(d->angle >> 16, d->speed);
        } else {
            u16 angle;
            if (curve_mode(d->mode) || (d->mode == WJ_FLIGHT && d->dynamic))
                angle = curve_reflection();
            else angle = reflected_angle(((d->angle >> 16) + (d->mode == WJ_MITA ? 16 : 0)) & 255);
            d->angle = (d->angle & 65535UL) | ((u32)angle << 16);
            if (!air) vector(angle, d->speed);
            if (curve_mode(d->mode)) { d->turn = 0; d->mode = WJ_FLIGHT; }
        }
    }
    if (air) {
        d->y = clamp_word(d->y, 74, 198);
        if (d->mode == WJ_LOB) {
            d->x = clamp_word(d->x, 20, 300);
            d->free_vx -= d->free_vx >> 5; d->free_vy -= d->free_vy >> 5;
            d->vz -= d->vz >> 5;
        }
        if (d->z >> 16 <= -16) {
            d->free_vy = d->vz = 0;
            if (d->mode == WJ_LOB) {
                d->free_vx = 0; d->mode = WJ_LAND; d->grace = 30; return;
            }
            d->mode = WJ_BOUNCE;
            d->angle = (u32)(d->owner ? 192 : 64) << 16;
            if ((d->free_vx < 0 ? -d->free_vx : d->free_vx) < 0x40000L)
                d->free_vx = d->owner ? -0x40000L : 0x40000L;
        }
    } else if (d->mode == WJ_BOUNCE || (d->mode == WJ_YOO && d->wave && !wall)) {
        d->free_vx += d->free_vx >> (d->mode == WJ_BOUNCE ? 4 : 5);
        if (d->mode == WJ_YOO) {
            if (d->free_vx < -0x90000L) d->free_vx = -0x90000L;
            else if (d->free_vx > 0x90000L) d->free_vx = 0x90000L;
        }
        d->speed = (d->free_vx < 0 ? -d->free_vx : d->free_vx) >> 11;
    }
    if (air) catchable = (d->z >> 16) < 8 && d->vz < 0;
    if (catchable) sample_contact();
    else if (air) {
        /* The original landing marker invites a charge before the disc falls.
         * Its projected box is distinct from the small catchable disc. */
        u16 port;
        for (port = 0; port < 2; port++) {
            WjPlayer *p = player(port);
            s16 dx = (s16)d->target_x - (p->x >> 16), dy = (s16)d->target_y - (p->y >> 16);
            if (!p->throwing && !p->ready && !p->charging &&
                wj_contact(port, dx, dy, port != 0)) { d->pending = 6; d->defender = port; }
        }
    }
}

static void flight(void)
{
    WjDisc *d = &st.disc;
    const WjProfile *v = &profiles[d->profile];
    s32 vx = d->reverse ? -v->vx : v->vx, sx = d->reverse ? -v->sx : v->sx;
    s32 x = d->x, y = d->y, full_x = x + vx, full_y = y + v->vy;
    u16 n = v->steps, hit = 0, crossing = 0;
    while (n--) {
        x += sx; y += v->sy;
        if ((y >> 16) < 72) { y = (72L << 16) | ((u32)y & 65535UL); hit = 1; break; }
        if ((y >> 16) >= 200) { y = (199L << 16) | ((u32)y & 65535UL); hit = 1; break; }
        if ((x >> 16) < 16 || (x >> 16) >= 304) crossing = 1;
    }
    d->x = hit ? x : full_x;
    d->y = hit ? y : full_y;
    if (hit) d->profile = reflected[d->profile];
    if (crossing) { goal(d->x >> 16 < 160 ? 0 : 1); return; }
    if (d->grace) { d->grace--; return; }
    /* Neutral descriptors; full turn/action-pose selection belongs to M2. */
    {
        u16 port = d->x >> 16 < 160 ? 0 : 1;
        WjPlayer *p = player(port);
        u16 contact;
        if (p->throwing) return;
        if (p->ready && port) {
            /* Measured Yoo ready descriptor: mirrored X anchor -2, half_x18
             * plus disc8, Y half15 plus disc8; same neutral catch arc. */
            s16 dx = (d->x >> 16) - (p->x >> 16) + 2;
            s16 dy = (d->y >> 16) - (p->y >> 16);
            u16 ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
            u8 a = contact_angle(dx, dy);
            contact = ax <= 26 && ay <= 23 ?
                (((u8)(a - 0x68) <= (u8)(0x18 - 0x68) ? 0x400 : 0x800) | a) : 0;
        } else contact = wj_contact(port, (d->x >> 16) - (p->x >> 16),
                             (d->y >> 16) - (p->y >> 16), port != 0);
        if (contact) { d->pending = contact >> 8; d->defender = port; }
    }
}

static u16 bot_input(u16 port)
{
    WjPlayer *p = player(port);
    s16 y = p->y >> 16, target = st.disc.mode == WJ_FLIGHT || advanced_ground(st.disc.mode) ? st.disc.y >> 16 : 138;
    u16 move = 0;
    if (st.disc.mode == WJ_HELD && st.disc.owner == port) {
        if (p->strong && !p->boundary) return K_A;
        if (p->catching || p->hold < 12) return 0;
        return K_A | (st.shot_number == 1 ? K_UP : st.shot_number == 2 ? K_DOWN : 0);
    }
    if (lob_mode(st.disc.mode) && (st.disc.target_x < 160 ? 0 : 1) == port) {
        s16 x = p->x >> 16;
        target = st.disc.target_y;
        if (x < (s16)st.disc.target_x - 2) move = K_RIGHT;
        else if (x > (s16)st.disc.target_x + 2) move = K_LEFT;
    }
    return move | (y < target - 2 ? K_DOWN : y > target + 2 ? K_UP : 0);
}

static void capture(void)
{
    WjDisc *d = &st.disc;
    WjPlayer *p = player(d->defender);
    p->catch_age = 0;
    p->catching = 1;
    p->recoil_profile = d->profile;
    p->recoil_aim = profiles[d->profile].vy < 0 ? 1 : profiles[d->profile].vy > 0 ? 2 : 0;
    p->recoil_reverse = d->reverse;
    p->recoil_dynamic = d->dynamic;
    p->recoil_angle = d->angle >> 16; p->recoil_speed = d->speed;
    { u16 speed = d->dynamic ? d->speed : profiles[d->profile].speed;
    if (d->dynamic) p->recoil_aim = d->free_vy < 0 ? 1 : d->free_vy > 0 ? 2 : 0;
    p->lock = speed >= 160 ?
        (speed >= 256 ? (d->defender ? 22 : 18) : (d->defender ? 20 : 16)) :
        (d->defender ? 13 : 9);
    }
    p->hold = 0;
    d->owner = d->defender;
    d->mode = WJ_HELD;
    d->pending = 0;
}

static void catch_step(u16 port)
{
    WjPlayer *p = player(port);
    p->catch_age++;
    p->hold++;
    if (p->recoil_dynamic && p->recoil_speed >= 160) {
        u16 speed = p->recoil_speed >> ((p->catch_age - 1) >> 1);
        p->vx = signed_product(trig(p->recoil_angle + 64), speed) >> 1;
        p->vy = signed_product(trig(p->recoil_angle), speed) >> 1;
    } else if (!p->recoil_dynamic && profiles[p->recoil_profile].speed >= 160) {
        u16 level = (p->catch_age - 1) >> 1;
        const WjRecoil *v = &recoil[p->recoil_profile][level < 9 ? level : 9];
        p->vx = p->recoil_reverse ? -v->vx : v->vx;
        p->vy = v->vy;
    } else p->vx = p->vy = 0;
    /* Source 01DA2C tests <=low, then nudges a holder five pixels inward;
     * it tests >high on the other side and nudges four pixels inward.
     * Preserve fractions, as with neutral walking. */
    p->x += p->vx;
    if ((p->x >> 16) <= (port ? 179 : 26))
        p->x = ((port ? 184L : 31L) << 16) | ((u32)p->x & 65535UL);
    else if ((p->x >> 16) > (port ? 292 : 141))
        p->x = ((port ? 288L : 137L) << 16) | ((u32)p->x & 65535UL);
    p->y = clamp_word(p->y + p->vy, 76, 188);
    if (p->catch_age >= p->lock) { p->catching = 0; p->lock = 0; }
}

u16 wj_hold_value(u16 port)
{
    WjPlayer *p = player(port);
    return p->ready || (p->charging && !p->air_charge) ? p->ready_counter : p->hold;
}

static void advanced_capture(void)
{
    WjDisc *d = &st.disc;
    WjPlayer *p = player(d->defender);
    u8 kind = d->mode;
    vector(d->angle >> 16, d->speed);
    p->vx = d->free_vx; p->vy = d->free_vy;
    p->counter_kind = kind;
    p->recoil_aim = d->free_vy < 0 ? 1 : d->free_vy > 0 ? 2 : 0;
    p->catch_age = p->hold = p->boundary = p->air_charge = p->y_boundary = 0;
    { u8 side = d->catch_angle + 64; if (!d->defender) side = -side; p->recoil_reverse = (side & 128) != 0; }
    p->strong = kind; p->catching = p->ready = p->charging = p->lock = 0;
    p->charge = 0;
    d->mode = WJ_HELD; d->owner = d->defender; d->pending = 0;
    d->z = d->vz = 0;
}

static void strong_step(u16 port)
{
    WjPlayer *p = player(port);
    s16 x;
    p->catch_age++; p->hold++;
    if (p->y_boundary) p->vy = -p->vy;
    p->x += p->vx; p->y = clamp_word(p->y + p->vy, 76, 188);
    x = p->x >> 16;
    if (p->boundary) {
        p->dragged = 1; p->strong = 0; p->hold = 0;
        st.disc.mode = WJ_DRAG;
    } else if (x <= (port ? 179 : 26) || x > (port ? 292 : 141)) {
            p->boundary = 1;
            p->x = ((s32)(x <= (port ? 179 : 26) ? (port ? 184 : 31) : (port ? 288 : 137)) << 16) | ((u32)p->x & 65535UL);
    } else p->boundary = 0;
    p->y_boundary = p->vy && ((p->y >> 16) <= 76 || (p->y >> 16) >= 188);
    if (p->y_boundary) p->boundary = 0;
    /* TARGET safe recovery if a slow captured special never reaches the rear.
     * The tested strong-push prefixes retain their original constant velocity. */
    if (p->catch_age >= 40) { p->strong = p->counter_kind = 0; p->vx = p->vy = 0; }
}

static void begin_throw(u16 port, u16 pad)
{
    WjPlayer *p = player(port);
    s16 hold;
    u8 lob = (pad & K_B) != 0, special = p->charged;
    if (!lob) {
        p->power = ((u16)p->power + p->hold) >> 1;
        hold = (s16)p->hold - (p->power >> 3) + p->bonus;
        p->bonus = p->bonus > 6 ? p->bonus - 6 : 0;
        if (st.points[port] < st.points[port ^ 1]) hold -= st.points[port ^ 1] - st.points[port];
        p->hold = hold < 0 ? 0 : hold > 64 ? 64 : hold;
    }
    if (p->counter_kind && p->hold <= 2) special = 1;
    p->throw_kind = lob ? (special ? WJ_SUPERLOB : WJ_LOB) :
        special ? (p->counter_kind == WJ_MITA || p->counter_kind == WJ_YOO ? p->counter_kind : port ? WJ_YOO : WJ_MITA) : 0;
    if (!lob && !special) p->throw_kind = gesture(p);
    p->curve_aim = curve_aims[(port ? 16 : 0) + direction_bits(pad)];
    if (special) p->power = p->power > 8 ? p->power - 8 : 0;
    else if (curve_mode(p->throw_kind)) p->power = p->power > 4 ? p->power - 4 : 0;
    p->throw_catch = p->catching;
    p->aim = pad & K_UP ? 1 : pad & K_DOWN ? 2 : 0;
    /* Catch interruption retains the neutral reception animation ($32);
     * settled hold samples the pad's facing before beginning the throw. */
    p->throw_delay = special ? (lob ? 12 : port ? 16 : 12) :
        p->strong ? (p->recoil_aim == 1 ? 8 : 12) : p->catching ?
        (p->recoil_dynamic && p->recoil_speed >= 160 && p->recoil_aim == 1 ? 8 : 12) : p->aim == 2 ? 8 : 12;
    if (curve_mode(p->throw_kind)) {
        u16 action = ((p->throw_kind == WJ_CURVE_PLUS) ^ (port != 0)) ? 0 : 8;
        p->throw_delay = curve_delays[action + (p->facing >> 5)];
    }
    p->catching = p->charging = p->charged = p->lock = 0;
    p->vx = p->vy = 0;
    p->strong = p->dragged = p->counter_kind = p->boundary = p->air_charge = p->y_boundary = 0;
    p->throwing = 1; p->age = 0;
    st.shot_number = st.shot_number == 2 ? 0 : st.shot_number + 1;
}

static void ready_step(u16 port)
{
    WjPlayer *p = player(port);
    u16 phase = p->ready_age++ & 15;
    p->ready_counter = p->ready_age | ((u16)(port ? ready_window_1[phase] : ready_window_0[phase]) << 8);
    p->vx -= p->vx >> 2; p->vy -= p->vy >> 2;
    p->x = clamp_word(p->x + p->vx, port ? 180 : 27, port ? 292 : 141);
    p->y = clamp_word(p->y + p->vy, 76, 188);
    if (p->ready_age >= 13) {
        p->ready = 0; p->ready_counter = p->hold = 0;
        /* A block may restart its animation in the original. Native block
         * recovery is bounded separately from the exact input prefix. */
    }
}

static void receive_ready(u16 port)
{
    WjPlayer *p = player(port);
    WjDisc *d = &st.disc;
    d->owner = port; d->pending = d->reverse = 0;
    /* Copy only integer player coordinates: source leaves disc fractions. */
    d->x = (p->x & ~65535L) | ((u32)d->x & 65535UL);
    d->y = (p->y & ~65535L) | ((u32)d->y & 65535UL);
    if (p->ready_counter & 256) {
        d->mode = WJ_LIFT; d->z = 0; d->vz = 0x34000L;
        p->vx = p->vy = 0;
    } else {
        /* TARGET block target: source chooses a random airborne destination.
         * Classification is measured; its RNG trajectory remains separate. */
        d->mode = WJ_BLOCK; d->z = 0; d->vz = 0x28000L;
        d->free_vx = port ? -0x10000L : 0x10000L; d->free_vy = 0;
        p->vx = p->vy = 0;
    }
}

static void air_step(void)
{
    WjDisc *d = &st.disc;
    if (d->mode == WJ_BLOCK) {
        d->x += d->free_vx; d->y += d->free_vy;
        if ((d->x >> 16) < 16 || (d->x >> 16) >= 304) { goal(d->x >> 16 < 160 ? 0 : 1); return; }
    }
    d->z += d->vz; d->vz -= 0x1500L;
    if (d->vz < 0 && (d->z >> 16) < 3) {
        u16 port = d->x >> 16 < 160 ? 0 : 1;
        WjPlayer *p = player(port);
        if (wj_contact(port, (d->x >> 16) - (p->x >> 16), (d->y >> 16) - (p->y >> 16), port != 0)) {
            d->pending = 4; d->defender = port;
        } else if (d->z <= 0) {
            /* TARGET missed native block: ordinary slow return to play. Full
             * airborne landing/bounce/corner rules belong to M3a. */
            d->z = d->vz = 0; d->mode = WJ_FLIGHT;
            d->profile = hold_profiles[port][64]; d->reverse = 0;
        }
    }
}

void wj_logic(u16 p1, u16 p2)
{
    u16 pads[2], port;
    pads[0] = p1; pads[1] = p2;
    st.logic_frame++;
    /* Native 30-second display clock: 2959/50 logic Hz, also during goals.
     * Training continues at zero; original round endings belong to M3. */
    if (st.seconds) {
        st.clock_phase += 50;
        if (st.clock_phase >= 2959) {
            st.clock_phase -= 2959;
            if (!--st.seconds) st.clock_phase = 0;
        }
    }
    if (st.disc.mode == WJ_GOAL) {
        if (++st.goal_age >= 90) {
            /* TARGET direct service, retaining points and scheduler/input state. */
            for (port = 0; port < 2; port++) {
                WjPlayer *p = player(port);
                memset(p, 0, sizeof *p);
                p->power = 4; p->bonus = 8;
                p->facing = port ? 192 : 64;
                p->x = (port ? 280L : 40L) << 16; p->y = 138L << 16;
            }
            memset(&st.disc, 0, sizeof st.disc);
            st.disc.mode = WJ_HELD; st.disc.owner = st.serve_to;
            st.disc.x = player(st.serve_to)->x; st.disc.y = 138L << 16;
        }
        st.keys[0] = p1; st.keys[1] = p2;
        return;
    }
    /* Original contact is sampled after flight, consumed before the next
     * player integration. This same frame contains the first recoil step. */
    if (st.disc.pending == 6 && lob_mode(st.disc.mode)) {
        WjPlayer *p = player(st.disc.defender);
        p->charging = p->air_charge = 1; p->charge = 0; st.disc.pending = 0;
    } else if (st.disc.pending == 4) {
        WjPlayer *p = player(st.disc.defender);
        if (st.disc.mode == WJ_LIFT || st.disc.mode == WJ_BLOCK || st.disc.mode == WJ_LOB) {
            st.disc.owner = st.disc.defender; st.disc.mode = WJ_HELD;
            st.disc.pending = 0; st.disc.z = st.disc.vz = 0;
            p->charging = p->ready = p->catching = p->lock = p->air_charge = 0; p->hold = 0;
        } else if (st.disc.mode == WJ_FLIGHT || advanced_ground(st.disc.mode) || st.disc.mode == WJ_SUPERLOB) {
            if (p->ready) receive_ready(st.disc.defender);
            else if (st.disc.mode == WJ_FLIGHT || curve_mode(st.disc.mode)) capture();
            else advanced_capture();
        }
    }
    for (port = 0; port < 2; port++) {
        WjPlayer *p = player(port);
        u16 pressed;
        if (p->dash_age && (p->catching || p->strong || p->charging ||
            (st.disc.mode == WJ_HELD && st.disc.owner == port))) p->dash_age = 0;
        if (st.bots & (1 << port)) pads[port] = bot_input(port);
        gesture_step(p, pads[port]);
        if (!p->throwing && !p->catching && !p->strong && !p->charging &&
            direction_bits(pads[port])) p->facing = p->history[0];
        pressed = pads[port] & ~st.keys[port];
        /* Original $1b272 injects an A edge on release while possessing the
         * disc (player $20 bit 5), including the capture frame. */
        if (st.disc.mode == WJ_HELD && st.disc.owner == port)
            pressed |= (st.keys[port] & ~pads[port]) & K_A;
        if (p->ready && st.disc.mode == WJ_LIFT && st.disc.owner == port && st.disc.z) {
            p->ready = 0; p->charging = 1; p->charge = 0;
        }
        if (p->charging) {
            p->vx = p->vy = 0;
            if (!p->charged) {
                if (p->charge > (port ? 40 : 43)) { p->charged = 1; p->charge = 0; }
                else p->charge++;
            }
        } else if (p->ready) ready_step(port);
        else if (p->strong) {
            if (!p->boundary && (pressed & (K_A | K_B))) begin_throw(port, pads[port]);
            else strong_step(port);
        } else if (p->dragged) {
            p->x += p->vx; p->y += p->vy;
            if ((p->y >> 16) < 76 || (p->y >> 16) > 188) {
                p->y = clamp_word(p->y, 76, 188); p->vy = -p->vy;
            }
        } else if (p->catching) {
            /* Capture input is accepted before recoil/age integration. */
            if (pressed & (K_A | K_B)) begin_throw(port, pads[port]);
            else catch_step(port);
        }
        else if (p->throwing) {
            p->age++;
            if (p->age == p->throw_delay) launch(port);
            if (p->age >= (curve_mode(p->throw_kind) ? 24 : p->throw_kind == WJ_SUPERLOB ? 36 : p->throw_kind >= WJ_YOO ? 52 : 28)) {
                p->throwing = 0;
            }
        } else if (st.disc.mode == WJ_HELD && st.disc.owner == port) {
            p->vx = p->vy = 0;
            if (p->lock) p->lock--;
            else {
                u16 release = pressed & (K_A | K_B);
                if (!release && p->hold < 64) p->hold++;
                if (release || p->hold == 64) {
                    begin_throw(port, pads[port]);
                }
            }
        } else if (p->dash_age) {
            if (p->dash_age >= (port ? 13 : 11)) {
                p->dash_age = p->dash_wall = 0;
                wj_walk(port, pads[port]);
            } else dash_step(port, pads[port]);
        } else if (pressed & K_A) {
            if (((pads[port] & K_RIGHT) != 0) != ((pads[port] & K_LEFT) != 0) ||
                ((pads[port] & K_DOWN) != 0) != ((pads[port] & K_UP) != 0)) begin_dash(port, pads[port]);
            else {
                p->ready = 1; p->ready_age = 0; p->vx = p->vy = 0;
                ready_step(port);
            }
        } else {
            wj_walk(port, pads[port]);
        }
        st.keys[port] = pads[port];
    }
    if (st.disc.mode == WJ_FLIGHT && !st.disc.dynamic) {
        {
            if (st.disc.pending == 8) {
                /* TARGET reflected rear hit, not original deflection speed/pose. */
                st.disc.reverse ^= 1; st.disc.pending = 0; st.disc.grace = 4;
            }
            flight();
        }
    }
    if (st.disc.mode == WJ_LIFT || st.disc.mode == WJ_BLOCK) air_step();
    else if (lob_mode(st.disc.mode) || advanced_ground(st.disc.mode) ||
             (st.disc.mode == WJ_FLIGHT && st.disc.dynamic)) {
        if (st.disc.pending == 8) {
            /* TARGET rear deflection, already separate from measured front
             * catches: keep the advanced shot playable in either direction. */
            st.disc.pending = 0; st.disc.free_vx = -st.disc.free_vx;
            st.disc.owner ^= 1; st.disc.angle ^= 128UL << 16;
        }
        advanced_step();
    } else if (st.disc.mode == WJ_LAND) {
        if (!--st.disc.grace) {
            u16 loser = st.disc.x >> 16 < 160 ? 0 : 1;
            goal_points(loser, 2);
        }
    } else if (st.disc.mode == WJ_DRAG) {
        WjPlayer *p = player(st.disc.owner);
        st.disc.x = p->x; st.disc.y = p->y;
        if ((p->x >> 16) < 16 || (p->x >> 16) >= 304) goal(st.disc.owner);
    }
    if (st.disc.mode == WJ_HELD) {
        WjPlayer *p = player(st.disc.owner);
        st.disc.x = p->x; st.disc.y = p->y;
    }
    wj_art_tick();
}

void game_init(void)
{
    wj_art_init();
    wj_reset(0);
    rt_state = &st; rt_state_size = sizeof st;
}

void wj_timing_door(u16 receiver)
{
    const WjTimingDoor *d = receiver ? &timing_doors[1] : &timing_doors[0];
    wj_reset(receiver ^ 1);
    st.bots = 0; st.disc.mode = WJ_FLIGHT;
    st.player[0].x = d->x1; st.player[0].y = d->y1;
    st.player[0].hold = d->hold1; st.player[0].power = d->power1; st.player[0].bonus = d->bonus1;
    st.player[1].x = d->x2; st.player[1].y = d->y2;
    st.player[1].hold = d->hold2; st.player[1].power = d->power2; st.player[1].bonus = d->bonus2;
    st.disc.x = d->dx; st.disc.y = d->dy; st.disc.profile = d->profile;
}

void game_scenario(u16 n)
{
    wj_reset(n == 1 ? 1 : 0);
    st.bots = n == 1 ? 0 : 2;
    if (n == 27 || n == 28) {
        wj_reset(n == 28 ? 0 : 1); st.bots = 0; st.human_port = n == 28;
        return;
    }
    if (n >= 23 && n <= 26) {
        u16 owner = n >= 25;
        wj_reset(owner); st.bots = 0; st.human_port = owner;
        player(owner ^ 1)->x = (owner ? 137L : 184L) << 16;
        player(owner ^ 1)->y = 76L << 16;
        return;
    }
    if (n >= 15 && n <= 22) {
        u16 owner = (n - 15) & 1;
        if (n >= 17) wj_timing_door(owner);
        else wj_reset(owner);
        st.bots = 0; st.human_port = owner;
        if (n >= 21) player(owner ^ 1)->y = 76L << 16;
        return;
    }
    if (n == 9 || n == 10) {
        st.bots = 0;
        st.disc.owner = n == 9 ? 1 : 0;
        if (n == 10) {
            st.player[0].x = 1871872L; st.player[0].hold = 29;
            st.player[1].hold = 25; st.player[1].power = 11; st.player[1].bonus = 2;
        }
        else { st.player[0].hold = 64; st.player[1].hold = 16; }
        st.disc.x = player(st.disc.owner)->x;
        st.disc.y = player(st.disc.owner)->y;
    }
    if (n >= 11 && n <= 14) {
        wj_timing_door(n == 12 || n == 14); st.human_port = n == 12 || n == 14; return;
    }
    if (n == 2) {
        st.bots = 0; st.disc.mode = WJ_FLIGHT; st.disc.profile = 4;
        st.disc.x = 15856304L; st.disc.y = 7598864L;
    }
    if (n == 3) {
        st.bots = 0; st.disc.mode = WJ_FLIGHT; st.disc.profile = 3;
        st.disc.x = 26L << 16; st.disc.y = 138L << 16;
        st.player[0].y = 88L << 16;
    }
    if (n == 4) st.bots = 3;
    if (n == 5) {
        st.bots = 0; st.disc.mode = WJ_FLIGHT; st.disc.profile = 3;
        st.player[0].x = 80L << 16; st.player[0].y = 130L << 16;
        st.disc.x = 65L << 16; st.disc.y = 130L << 16;
    }
    if (n == 6) { st.points[1] = 5; st.disc.mode = WJ_GOAL; st.serve_to = 0; st.last_award = 5; }
    if (n == 7 || n == 8) {
        st.bots = 0; st.disc.mode = WJ_FLIGHT;
        st.disc.profile = n == 7 ? 3 : 0;
        st.disc.x = (n == 7 ? 62L : 260L) << 16;
        st.disc.y = 138L << 16;
    }
}

u16 wj_hash(void)
{
    u16 h = 0x574a, port;
#define H(v) do { h = (u16)((h << 5) | (h >> 11)); h ^= (u16)(v); } while (0)
#define HL(v) do { H((u32)(v) >> 16); H(v); } while (0)
    for (port = 0; port < 2; port++) {
        WjPlayer *p = player(port);
        HL(p->x); HL(p->y); HL(p->vx); HL(p->vy);
        H(p->throwing); H(p->age); H(p->aim); H(p->throw_delay); H(p->throw_catch); H(p->lock); H(p->hold); H(p->power); H(p->bonus);
        H(p->catching); H(p->catch_age); H(p->recoil_profile); H(p->recoil_reverse); H(p->recoil_aim);
        H(p->art_action); H(p->art_direction); H(p->art_tick);
        H(p->ready); H(p->ready_age); H(p->charging); H(p->charge); H(p->charged); H(p->ready_counter);
        H(p->throw_kind); H(p->counter_kind); H(p->strong); H(p->boundary); H(p->dragged); H(p->air_charge); H(p->y_boundary);
        { u16 i; for (i = 0; i < 9; i++) H(p->history[i]); }
        H(p->curve_aim); H(p->recoil_dynamic); H(p->recoil_angle); H(p->recoil_speed); H(p->facing);
        H(p->dash_age); H(p->dash_wall);
        H(st.keys[port]); H(st.points[port]);
    }
    HL(st.disc.x); HL(st.disc.y); HL(st.disc.z); HL(st.disc.vz);
    HL(st.disc.free_vx); HL(st.disc.free_vy); H(st.disc.mode); H(st.disc.owner);
    H(st.disc.profile); H(st.disc.reverse); H(st.disc.pending); H(st.disc.defender); H(st.disc.grace);
    HL(st.disc.angle); HL(st.disc.turn); H(st.disc.speed); H(st.disc.anchor); H(st.disc.wave); H(st.disc.wall_side); H(st.disc.catch_angle);
    H(st.disc.target_x); H(st.disc.target_y);
    H(st.disc.dynamic);
    H(st.logic_frame); H(st.draw_frame); H(st.phase); H(st.goal_age); H(st.bots); H(st.serve_to);
    H(st.shot_number); H(st.last_award);
    H(st.seconds); H(st.clock_phase); H(st.human_port); H(st.lob_seed);
    { u16 i; for (i = 0; i < 4; i++) { H(st.effect_x[i]); H(st.effect_y[i]); } }
    H(st.effect_count); H(st.effect_kind); H(st.effect_owner);
#undef HL
#undef H
    return h;
}

u8 game_update(void)
{
    if (input_pressed(K_ESC)) return 0;
    if (input_pressed(K_ENTER)) game_scenario(0);
    /* 59.18 = 2959/50 Hz, 256 runtime ticks/s, no multiply/divide in loop. */
    st.phase += RT_FRAME_LEN(st.draw_frame) == 8 ? 23672 : 26631;
    st.draw_frame++;
    while (st.phase >= 12800) {
        st.phase -= 12800;
        wj_logic(st.human_port ? 0 : (u16)rt_keys, st.human_port ? (u16)rt_keys : 0);
    }
    wj_effect_tick();
#ifdef STATE_HASH
    BENCH_VALUE(wj_hash());
#endif
    return 1;
}
