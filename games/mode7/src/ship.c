/* Mode 7 - Demo 2 (David Coz, 2005): the ship, its physics and its collisions with the walls.
 * Reconstructed from the binary (see mode7.h). Original addresses in the comments.
 * Arithmetic is 16-bit where the original's is (TIGCC's int is 16 bits). */
#include "mode7.h"

#ifdef ORIGINAL
#define MUL(a, b) ((long)(a) * (b))
#else
/* optimised: one muls.w; the operands fit 16 bits (speed < 3072 by the throttle curve, wall
 * distances < 4096 half units, normals < 1024, t < 8192): the same products as the original's
 * __mulsi3 calls */
static inline long muls16(short a, short b)
{ long r; asm("muls.w %2,%0" : "=d"(r) : "0"(a), "dmi"(b)); return r; }
#define MUL(a, b) muls16(a, b)
#endif

/* 0x32a6 */
void InitShip(Ship *s)
{
    s->x = (short)(start_x << 5);         /* world units * 8 */
    s->y = (short)(start_y << 5);
    s->angle = start_angle << 8;
    s->speed = 0;
    s->accel = 100;
    s->brake = 150;
    s->turn_base = 1200;
    s->turn_loss = 35;
    s->dt = 6;
    s->drag = 1000;
}

/* 0x38d0: pushes the ship out of the walls of its cell. The track is cut into 8x8 cells of 4096
 * (1/8 unit) steps, each with up to 8 wall segments: a start point, an outward normal (1024 =
 * 1 after >> 5) and a length. A ship behind a wall (dot product < 0) and within its length is
 * put back on the wall line, 8 half units outside; only the first such wall counts. */
static void Collide(Ship *s)
{
    short cx = (unsigned long)s->x >> 12;
    short cy8 = (short)((unsigned long)s->y >> 12) << 3;
    short n = cell_walls_count[cy8 + cx];
    long px = s->x >> 4, py = s->y >> 4;  /* half units */
    short k;

    for (k = 0; k < n; k++) {
        short w = cell_walls[((long)(cy8 + cx) << 3) + k];
        long nx = (short)(wall_normal[w][0] >> 5);
        long ny = (short)(wall_normal[w][1] >> 5);
        long ax = (short)(wall_point[w][0] << 2);
        long ay = (short)(wall_point[w][1] << 2);
        long dx = px - ax, dy = py - ay;
        long t = MUL(dx, ny) - MUL(dy, nx);   /* position along the wall */
        long len;
        if (t < 0) continue;
        t >>= 10;
        len = wall_length[w];
        if (MUL(len, len) << 4 < MUL(t, t)) continue;
        if (MUL(dx, nx) + MUL(dy, ny) >= 0) continue;  /* in front of the wall */
        s->x = ((MUL(t, ny) >> 10) + ((nx << 3) >> 10) + ax) << 4;
        s->y = ((MUL(-t, nx) >> 10) + (ny >> 7) + ay) << 4;
        return;
    }
}

/* 0x37de: turn (-1 right, 1 left), throttle (1 accelerate, 0 coast, -1 brake) */
static void UpdateShip(Ship *s, short turn, short throttle)
{
    long speed = s->speed;
    short rate, up, brake, drag;
    unsigned char a;
    long fx, fy;

    /* the faster, the slower it turns */
    rate = (short)(s->turn_base - (short)MUL(speed >> 7, s->turn_loss)) >> 2;
    s->angle += rate * (turn * s->dt);
    up = (short)(accel_curve[(unsigned long)speed >> 8] * s->accel * s->dt) >> 7;
    brake = (short)(s->brake * s->dt) >> 3;
    drag = (short)(((s->drag >> 3) + 150) * s->dt) >> 6;
    if (throttle == 1) s->speed = speed + up;
    if (throttle == 0) s->speed -= drag;
    if (throttle == -1) s->speed -= brake;
    if (s->speed < 0) s->speed = 0;

    a = s->angle >> 8;                    /* heading 0..255: 0 = +y, 64 = -x */
    speed = s->speed;
    fy = MUL(speed, cos128[a]) >> 7;
    fx = MUL(speed, sin128[a]);
    s->x += (-fx) >> 10;
    s->y += fy >> 3;
    Collide(s);
}

/* 0x3744 */
void ReadKeys(Game *g)
{
    short turn, throttle;
    turn = -(_keytest(RR_RIGHT) != 0);
    if (_keytest(RR_LEFT)) turn = 1;
    throttle = _keytest(RR_2ND) != 0;
    if (_keytest(RR_DIAMOND)) throttle = -1;
    DriveShip(g, turn, throttle);
}

void DriveShip(Game *g, short turn, short throttle)
{
    UpdateShip(&g->ship, turn, throttle);
    g->shipx = g->ship.x >> 3;
    g->shipy = g->ship.y >> 3;
    g->angle = g->ship.angle >> 8;
}
