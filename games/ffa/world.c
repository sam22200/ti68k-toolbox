// Field engine: cell lookup, continuous movement with axis-separated collision and corner
// sliding (Chrono Trigger feel), doors, camera, fades.
#include "ffa.h"

static const Room *cur(void) { return &rooms[st.room]; }

u8 world_cell(s16 px, s16 py)
{
    const Room *r = cur();
    if (px < 0 || py < 0) return CELL_WALL;
    px >>= 4; py >>= 4;
    if (px >= r->w || py >= r->h) return CELL_WALL;
    return r->cell[(u16)py * r->w + px];
}

u8 world_solid(u8 code)
{
    if (code == CELL_FLOOR) return 0;
    if ((code & 0xC0) == CELL_TRIG) return cur()->trig[code & 0x3F] < 0;   // texts are objects
    return 1;                                                               // walls, doors
}

static RtTilemap world_map;
static const u16 *tileset[4];

const RtTilemap *world_tilemap(void)          // the current room's map with its area's tiles
{
    world_map = rooms[st.room].map;
    world_map.tiles = tileset[rooms[st.room].set];
    return &world_map;
}

u8 world_load(void)
{
    const u16 *d = rt_file("ffadat", RT_NULL);
    u8 k;
    if (!d) return 0;
    for (k = 0; k < d[0] && k < 4; k++) tileset[k] = d + d[1 + k];
    return 1;
}

void world_enter(u8 room, s8 cx, s8 cy)
{
    st.room = room;
    if (cx >= 0) st.x = cx * TILE + HB_X0;
    if (cy >= 0) st.y = cy * TILE + HB_Y0;
    st.cell_in = world_cell(st.x + HB_W / 2, st.y + HB_H / 2);
    tilemap_dirty();
}

static u8 door_try(u8 code)                  // bumped into a door cell: open it if allowed
{
    const Door *d;
    if (!(code & CELL_DOOR)) return 0;
    d = &cur()->door[code & 0x3F];
    if (d->dest == 0xFF) {                   // out of part I (room 73): locked for good
        dialog_open(1, 0);                   // T_WEAPON_ROOM
        st.mode = M_TEXT;
        return 1;
    }
    if (d->key == KEY_NEVER || (d->key && d->key != 0xFF && !st.flag[d->key])) {
        dialog_open(0, 0);                   // T_LOCKED: "The door is locked."
        st.mode = M_TEXT;
        return 1;
    }
    st.next_room = d->dest;
    st.next_door = code & 0x3F;
    st.mode = M_FADE_OUT;
    st.fade = 0; st.fade_t = 0;
    return 1;
}

// Blocked cells along the leading edge of a move. Returns the door code met, 0xFF for a plain
// block, 0 when free; *lo / *hi = the edge's two ends are blocked.
static u8 edge(s16 x0, s16 y0, s16 x1, s16 y1, u8 *lo, u8 *hi)
{
    u8 a = world_cell(x0, y0), b = world_cell(x1, y1), r = 0;
    *lo = world_solid(a); *hi = world_solid(b);
    if (*lo) r = a & CELL_DOOR ? a : 0xFF;
    if (*hi && !(r & CELL_DOOR)) r = b & CELL_DOOR ? b : 0xFF;
    return r;
}

static void axis(s16 d, u8 horiz)
{
    s16 nx = st.x + (horiz ? d : 0), ny = st.y + (horiz ? 0 : d);
    u8 lo, hi, hit;
    if (horiz) {
        s16 ex = d > 0 ? nx + HB_W - 1 : nx;
        hit = edge(ex, ny, ex, ny + HB_H - 1, &lo, &hi);
    } else {
        s16 ey = d > 0 ? ny + HB_H - 1 : ny;
        hit = edge(nx, ey, nx + HB_W - 1, ey, &lo, &hi);
    }
    if (!hit) { st.x = nx; st.y = ny; return; }
    if (hit != 0xFF && door_try(hit)) return;
    if (lo != hi) {                          // one corner blocked: slide around it
        s16 over;
        if (horiz) {
            over = lo ? 16 - (ny & 15) : ((ny + HB_H) & 15);   // overlap into the blocked row
            if (over <= SLIDE) st.y += lo ? 1 : -1;
        } else {
            over = lo ? 16 - (nx & 15) : ((nx + HB_W) & 15);
            if (over <= SLIDE) st.x += lo ? 1 : -1;
        }
    }
    // flush against the wall: step pixel by pixel (d is at most 3)
    while (d > 0 ? --d > 0 : ++d < 0) {
        nx = st.x + (horiz ? (d > 0 ? 1 : -1) : 0);
        ny = st.y + (horiz ? 0 : (d > 0 ? 1 : -1));
        if (horiz) {
            s16 ex = d > 0 ? nx + HB_W - 1 : nx;
            if (edge(ex, ny, ex, ny + HB_H - 1, &lo, &hi)) break;
        } else {
            s16 ey = d > 0 ? ny + HB_H - 1 : ny;
            if (edge(nx, ey, nx + HB_W - 1, ey, &lo, &hi)) break;
        }
        st.x = nx; st.y = ny;
    }
}

void world_move(s16 dx, s16 dy)
{
    s16 ox = st.x, oy = st.y;
    if (dx) axis(dx, 1);
    if (dy && st.mode == M_WALK) axis(dy, 0);
    ox = st.x - ox; oy = st.y - oy;
    if (ox < 0) ox = -ox;
    if (oy < 0) oy = -oy;
    st.walked += ox > oy ? ox : oy;
}

s16 world_cam(s16 p, s16 view, s16 size)
{
    s16 c = p - view / 2;
    if (c > size - view) c = size - view;
    if (c < 0) c = 0;
    return c;
}

// Fade towards white, Link's Awakening style: every grey level drops by `level` (0..3).
// One level: 3->2, 2->1, 1->0 on each pixel = new light = dark & ~light, new dark = dark & light.
void fade_planes(u8 level)
{
    u16 *l = (u16 *)rt_light, *d = (u16 *)rt_dark;
    u16 y, x;
    if (!level) return;
    if (!d) {                                // mono: black survives one level only
        if (level > 1) for (y = 0; y < RT_H; y++, l += RT_PBYTES / 2) for (x = 0; x < RT_W / 16; x++) l[x] = 0;
        return;
    }
    for (y = 0; y < RT_H; y++, l += RT_PBYTES / 2, d += RT_PBYTES / 2) {
        for (x = 0; x < RT_W / 16; x++) {
            u16 a = l[x], b = d[x];
            if (level == 1) { l[x] = b & ~a; d[x] = b & a; }
            else if (level == 2) { l[x] = b & a; d[x] = 0; }
            else { l[x] = 0; d[x] = 0; }
        }
    }
}
