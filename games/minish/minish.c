/* Minish Woods: measured walking, original scenery and outlined ROM actors.
 * Two source steps per draw; all decoding and palette reduction are offline. */
#include "minish.h"
#include "generated.h"
#include "zoom_generated.h"
#ifdef STATE_HASH
#include "../../tools/m68kbench/bench.h"
static u32 frame_begin;
#endif

MinishState st;
static const u8 *collision, *shape_map, *acts;
static const u16 *masks;
static const u16 direction_masks[8] = {
    0x0006, 0x6006, 0x6000, 0x6060, 0x0060, 0x0660, 0x0600, 0x0606
};
static const s16 vx[8] = {0,226,320,226,0,-226,-320,-226};
static const s16 vy[8] = {-320,-226,0,226,320,226,0,-226};
static const u8 slopes[8] = {19,18,18,16,16,17,17,19};

static u16 cell(u16 x, u16 y) { return ((y & 0x3f0) << 2) | (x >> 4); }

u8 minish_solid(u16 x, u16 y)
{
    u16 index;
    if (x >= WOODS_W || y >= WOODS_H || !st.ready) return 1;
    index = shape_map[cell(x,y)];
    if (!index || minish_cut_cell(cell(x,y))) return 0;
    return (masks[(index << 4) | (y & 15)] >> (15 - (x & 15))) & 1;
}

static u16 contacts(u16 x, u16 y)
{
    return (minish_solid(x+5,y) ? 0x4000 : 0) |
           (minish_solid(x+5,y-6) ? 0x2000 : 0) |
           (minish_solid(x-5,y) ? 0x0400 : 0) |
           (minish_solid(x-5,y-6) ? 0x0200 : 0) |
           (minish_solid(x+3,y+2) ? 0x0040 : 0) |
           (minish_solid(x-3,y+2) ? 0x0020 : 0) |
           (minish_solid(x+3,y-8) ? 0x0004 : 0) |
           (minish_solid(x-3,y-8) ? 0x0002 : 0);
}

static u8 input_direction(u16 k)
{
    s16 x = ((k & K_RIGHT) != 0) - ((k & K_LEFT) != 0);
    s16 y = ((k & K_DOWN) != 0) - ((k & K_UP) != 0);
    if (y < 0) return x < 0 ? 28 : x > 0 ? 4 : 0;
    if (y > 0) return x < 0 ? 20 : x > 0 ? 12 : 16;
    return x < 0 ? 24 : x > 0 ? 8 : 255;
}

static u8 slope_direction(u8 d, u16 x, u16 y)
{
    s16 dx, dy;
    u16 i, t;
    if (d & 4) return d;
    if (d & 8) { dx = d & 16 ? -5 : 5; dy = 3; }
    else { dx = 3; dy = d ? 5 : -5; }
    for (i = 0; i < 2; ++i) {
        if ((u16)(x+dx) < WOODS_W && (u16)(y+dy) < WOODS_H) {
            t = collision[cell(x+dx,y+dy)];
            if (t == slopes[d >> 2]) return (d+4) & 31;
            if (t == slopes[(d >> 2)+1]) return (d-4) & 31;
        }
        if (d & 16) dy = -dy; else dx = -dx;
    }
    return d;
}

/* A cardinal corner contact can slide by one pixel along the free side. */
static u8 slide(u8 d, u16 c)
{
    switch (d) {
    case 0:
        if (!(c & 0xe)) break;
        if (!(c & 0xe004)) return 8;
        if (!(c & 0x0e02)) return 24;
        break;
    case 8:
        if (!(c & 0xe000)) break;
        if (!(c & 0x200e)) return 0;
        if (!(c & 0x40e0)) return 16;
        break;
    case 16:
        if (!(c & 0xe0)) break;
        if (!(c & 0xe040)) return 8;
        if (!(c & 0x0e20)) return 24;
        break;
    case 24:
        if (!(c & 0x0e00)) break;
        if (!(c & 0x020e)) return 0;
        if (!(c & 0x04e0)) return 16;
        break;
    }
    return d;
}

/* Preserve the source's fractional coordinate while backing a penetrating
 * central probe out of a partial metatile. The scan stays within one tile. */
static void resolve(u16 x, u16 y, u8 side)
{
    s16 n, start, step;
    u16 bits, index;
    start = side & 1 ? x & 15 : y & 15;
    if (!start || start == 15) return;
    index = shape_map[cell(x,y)];
    step = side == 0 || side == 3 ? 1 : -1;
    n = start;
    do {
        bits = masks[(index << 4) | (side & 1 ? y & 15 : (u16)n)];
        if (!(bits & (0x8000 >> (side & 1 ? (u16)n : x & 15)))) break;
        n += step;
    } while (n > 0 && n < 15);
    n -= step;
    if (side & 1) st.x += (s32)(n-start) * 256;
    else st.y += (s32)(n-start) * 256;
}

static void camera(void)
{
    s16 x = minish_scaled(st.x>>8)-RT_W/2;
    s16 y = minish_scaled(st.y>>8)-RT_H/2-8;
    st.camx = x < 0 ? 0 : x > ZOOM_W-RT_W ? ZOOM_W-RT_W : x;
    st.camy = y < 0 ? 0 : y > ZOOM_H-RT_H ? ZOOM_H-RT_H : y;
}

void minish_place(u16 x, u16 y)
{
    st.x = (s32)x << 8; st.y = (s32)y << 8;
    st.direction = 255; st.facing = 16; st.moving = 0; st.steps = 0;
    st.anim_face = 2; st.anim_walk = st.anim_phase = 0; st.anim_timer = 3;
    st.pose = st.display_pose = idle_pose[2]; st.cover = st.display_cover = 1;
    st.preview = 0;
    st.attack=st.last_a=st.cut_count=st.display_cut_count=0;
    { u16 i;for (i=0;i<7;i++) st.cut_flags[i]=0; }
    st.collisions = st.ready ? contacts(x,y) : 0;
    minish_combat_reset();
    minish_effects_reset();
    camera();
}

static void animation(u8 requested, u8 foreground)
{
    u16 direction, face = st.anim_face, walking = st.moving;
    st.display_pose = st.pose; st.display_cover = st.cover;
    st.cover = foreground ? 0 : 1;
    if (requested != 255) {
        direction = requested >> 2;
        /* Original diagonal facing retains a compatible previous cardinal. */
        if (!(direction & 1) || ((direction+1-(face<<1)) & 4))
            face = (direction & 6) >> 1;
    }
    if (walking != st.anim_walk || face != st.anim_face) {
        st.anim_phase = 0; st.anim_timer = 3;
    } else if (walking && !--st.anim_timer) {
        if (++st.anim_phase == WALK_POSES) st.anim_phase = 0;
        st.anim_timer = 3;
    }
    st.anim_walk = walking; st.anim_face = face;
    st.pose = walking ? walk_pose[face][st.anim_phase] : idle_pose[face];
}

static void walking_step(u16 keys)
{
    u16 x, y, c, filtered;
    s16 dx, dy;
    u8 d, move;
    u8 act, slow,rolling;
    s32 oldx = st.x, oldy = st.y;
    if (!st.ready) return;
    rolling=minish_roll_step(keys);
    if (rolling==1) { camera();return; }
    if (!rolling && minish_action_step(keys)) { camera();return; }
    x = st.x >> 8; y = st.y >> 8;
    st.collisions = c = contacts(x,y);
    st.direction = d = rolling ? st.anim_face<<3 : input_direction(keys);
    act = acts[cell(x,y)];
    slow = act == 38 || act == 39 || act == 52 || act == 53;
    if (d != 255 && act != 40 && act != 41) {
        if (!rolling) st.direction = d = slope_direction(d,x,y);
        move = d & 7 ? d : slide(d,c);
        if (move != d) {
            dx = move == 8 ? 256 : move == 24 ? -256 : 0;
            dy = move == 16 ? 256 : move == 0 ? -256 : 0;
        } else {
            if (rolling) {
                s16 speed=minish_roll_speed();
                dx=move==8 ? speed : move==24 ? -speed : 0;
                dy=move==16 ? speed : move==0 ? -speed : 0;
            } else {dx = vx[move >> 2]; dy = vy[move >> 2];}
            if (slow && !rolling) {
                dx = dx == 320 ? 240 : dx == -320 ? -240 : dx > 0 ? 169 : dx < 0 ? -169 : 0;
                dy = dy == 320 ? 240 : dy == -320 ? -240 : dy > 0 ? 169 : dy < 0 ? -169 : 0;
            }
        }
        filtered = c & direction_masks[move >> 2];
        if (!(filtered & 0xee00)) st.x += dx;
        if (!(filtered & 0x00ee)) st.y += dy;
        if (d != 0 && d != 16) {
            x = st.x >> 8; y = (st.y >> 8)-3;
            if (minish_solid(x+5,y)) resolve(x+5,y,1);
            x = st.x >> 8;
            if (minish_solid(x-5,y)) resolve(x-5,y,3);
        }
        if (d != 8 && d != 24) {
            x = st.x >> 8; y = (st.y >> 8)+2;
            if (minish_solid(x,y)) resolve(x,y,2);
            y = (st.y >> 8)-8;
            if (minish_solid(x,y)) resolve(x,y,0);
        }
        st.facing = d;
    }
    /* Native slice boundary, separate from original room/exit behavior. */
    if (st.x < 8*256L) st.x = 8*256L;
    if (st.x > (WOODS_W-8)*256L) st.x = (WOODS_W-8)*256L;
    if (st.y < 12*256L) st.y = 12*256L;
    if (st.y > (WOODS_H-4)*256L) st.y = (WOODS_H-4)*256L;
    st.moving = st.x != oldx || st.y != oldy;
    if (st.moving) st.steps++;
    if (rolling) st.cover=!(act==38 || act==39);
    else animation(input_direction(keys),act == 38 || act == 39);
    camera();
}

void minish_step(u16 keys)
{
    if (!st.ready) return;
    minish_effects_step();
    st.display_cut_count=st.cut_count;
    if (!minish_combat_before(keys)) walking_step(keys);
    minish_combat_step();
    if (st.encounters) camera();
}

void game_init(void)
{
    u16 size;
    const u8 *bank;
    u8 zoom_ready;
    st.ready = 0;
#ifdef STATE_HASH
    frame_begin = 0;
#endif
    collision = shape_map = acts = RT_NULL; masks = RT_NULL;
    zoom_ready = minish_zoom_init();
    bank = rt_file("mindat", &size);
    if (bank && size >= 9984 && size <= 9990) {
        collision = bank; masks = (const u16 *)(bank+2048);
        shape_map = bank+3328; acts = bank+7936;
        st.ready = zoom_ready;
    }
    st.ready = st.ready && minish_actions_init() && minish_combat_init() && minish_effects_init();
    rt_state = &st; rt_state_size = sizeof(st);
    minish_place(248,88);
    minish_combat_start();
}

void game_scenario(u16 n)
{
    if (n>=700 && n<892) {
        u16 i=n-700;
        minish_place(320,184);st.preview=1;
        st.pose=st.display_pose=84+minish_fx_preview_pose(i<128 ? i>>5 : 2);
        if (i<128) st.camx-=i&31;
        else {
            st.camx=minish_scaled(320);
            st.camx-=i>=160 ? RT_W-16+(s16)(i&31) : 16-(s16)(i&31);
        }
        return;
    }
    if (n>=512 && n<512+3*minish_fx_pose_count()) {
        u16 count=minish_fx_pose_count(),group=(n-512)/count,pose=(n-512)%count;
        minish_place(group==1 ? 246 : group==2 ? 552 : 320,group==1 ? 160 : group==2 ? 128 : 184);
        st.pose=st.display_pose=84+pose;st.preview=1;return;
    }
    if (n>=440 && n<456) {
        u16 i;game_scenario(260+(n&7));
        for (i=0;i<4;i++) {
            minish_fx_spawn(410+i*10,148+(i&1)*16,i&1);
            st.effects[i].age=1+((n-440)&3)*7;
        }
        if (n>=448) for (i=0;i<2;i++) {
            st.enemies[i].hp=st.enemies[i].recoil=0;
            st.enemies[i].fade=minish_death_length()-(n-448)*6;
        }
        return;
    }
    if (n==430 || n==432) {
        minish_place(320,184);
        if (n==432) {
            minish_combat_start();st.enemies[1].hp=0;
            st.enemies[0].x=320*256L;st.enemies[0].y=164*256L;
            st.enemies[0].action=1;st.enemies[0].timer=200;
            st.anim_face=0;st.facing=0;st.pose=st.display_pose=idle_pose[0];
        }
        return;
    }
    if (n>=408 && n<420) {
        /* Knockback poses (408..417) and two damage-flash phases (418, 419). */
        minish_place(280,184);minish_combat_start();st.preview=1;
        st.enemies[0].hp=st.enemies[1].hp=0;
        if (n<418) st.pose=st.display_pose=MINISH_HURT+n-408;
        else st.iframes=n==418 ? 26 : 29;
        return;
    }
    if (n==420) {
        /* Real walk-to-spit AI transition, no forced projectile spawn. */
        minish_place(280,184);minish_combat_start();
        st.enemies[0].x=328*256L;st.enemies[0].y=184*256L;
        st.enemies[0].action=2;st.enemies[0].timer=1;st.enemies[0].face=0;
        st.enemies[1].hp=0;
        return;
    }
    if (n>=360 && n<408) {
        u16 i=n-360;
        minish_place(320,184);minish_combat_start();st.preview=1;
        st.enemies[1].hp=0;
        st.enemies[0].x=(s32)(i<32 ? 196+i : i<40 ? 440+i-32 : 320)<<8;
        st.enemies[0].y=(s32)(i<40 ? 184 : 84+(i-40)*24)<<8;
        st.rocks[0].x=st.enemies[0].x;st.rocks[0].y=st.enemies[0].y;
        st.rocks[0].life=48;
        return;
    }
    if (n>=300 && n<360) {
        u16 group=(n-300)/20,pose=(n-300)%20;
        minish_place(group==2 ? 248 : 280,group==2 ? 136 : 184);
        minish_combat_start();st.preview=1;
        st.enemies[0].x=(s32)(group==2 ? 246 : group==1 ? 292 : 280)<<8;
        st.enemies[0].y=(s32)(group==2 ? 160 : group==1 ? 184 : 152)<<8;
        st.enemies[0].pose=st.enemies[0].display_pose=pose;st.enemies[1].hp=0;
        return;
    }
    if (n>=256 && n<272) {
        u16 i;
        minish_place(280,184);minish_combat_start();
        if (n==257) minish_place(280,152),minish_combat_start();
        if (n==258) {st.health=2;st.x=280*256L;st.y=152*256L;}
        if (n>=260) {
            minish_place(424+(n-260)*2,160);minish_combat_start();
            for (i=0;i<53;i++) {st.cut_flags[i>>3]|=1<<(i&7);st.cut_list[i]=i;}
            st.cut_count=st.display_cut_count=53;
            st.enemies[0].x=400*256L;st.enemies[0].y=160*256L;
            st.enemies[1].x=444*256L;st.enemies[1].y=168*256L;
            for (i=0;i<4;i++) {st.rocks[i].x=(s32)(410+i*8)*256;st.rocks[i].y=140*256L;st.rocks[i].life=48;st.rocks[i].face=2;}
        }
        st.anim_face=0;st.facing=0;st.pose=st.display_pose=idle_pose[0];
        camera();return;
    }
    if (n>=144 && n<256) {
        u16 i,pose=n<184 ? n-144 : n<224 ? n-184 : 14;
        minish_place(n<184 ? 424 : n<224 ? 632 : 424+n-224,
                     n<184 ? 160 : n<224 ? 128 : 136);
        for (i=0;i<53;i++) {st.cut_flags[i>>3]|=1<<(i&7);st.cut_list[i]=i;}
        st.cut_count=st.display_cut_count=53;st.pose=st.display_pose=44+pose;st.preview=1;
        return;
    }
    if (n>=100 && n<140) {
        minish_place(424,160);st.preview=1;
        st.pose=st.display_pose=44+n-100;
        return;
    }
    if (n>=140 && n<=143) {
        minish_place(n==143 ? 632 : 424,n==143 ? 128 : 160);
        st.anim_face=0;st.facing=0;st.pose=st.display_pose=idle_pose[0];
        if (n==141 || n==142) {
            u16 i;for (i=0;i<53;i++) {st.cut_flags[i>>3]|=1<<(i&7);st.cut_list[i]=i;}
            st.cut_count=st.display_cut_count=53;
            if (n==142) {st.pose=st.display_pose=58;st.preview=1;}
        }
        return;
    }
    if (n>=64 && n<100) {
        static const u16 corner_x[4]={8,712,8,712};
        static const u16 corner_y[4]={12,12,316,316};
        if (n<96) minish_place(320+n-64,184);
        else minish_place(corner_x[n-96],corner_y[n-96]);
        st.preview=1;
        return;
    }
    if (n >= 16 && n < 16+CHAR_POSES) {
        minish_place(248,136);
        st.preview = 1; st.pose = st.display_pose = n-16;
        return;
    }
    switch (n) {
    case 9: minish_place(248,88);break; /* Original enemy-free study door. */
    case 1: minish_place(32,88); break;
    case 2: minish_place(244,120); break;
    case 3: minish_place(392,48); break;
    case 4: minish_place(544,48); break;
    case 5: minish_place(692,136); break;
    case 6: minish_place(120,88); break;
    case 7: case 8:
        minish_place(246,160); st.preview = 1;
        st.cover = st.display_cover = n == 7;
        break;
    default: minish_place(248,88); minish_combat_start();break;
    }
}

u32 minish_hash(void)
{
    /* Hash fields explicitly, independent of ABI padding and byte order. */
    u32 h = 2166136261UL;
#define MIX(v) do { h = ((h << 5) | (h >> 27)) ^ (u32)(v); } while (0)
    MIX(st.x); MIX(st.y); MIX(st.collisions); MIX(st.steps);
    MIX(st.camx); MIX(st.camy); MIX(st.direction); MIX(st.facing);
    MIX(st.moving); MIX(st.ready);
    MIX(st.anim_face); MIX(st.anim_walk); MIX(st.anim_phase); MIX(st.anim_timer);
    MIX(st.pose); MIX(st.display_pose); MIX(st.cover); MIX(st.display_cover); MIX(st.preview);
    MIX(st.attack);MIX(st.last_a);MIX(st.cut_count);MIX(st.display_cut_count);
    MIX(st.roll);MIX(st.last_b);MIX(st.roll_guard);
    { u16 i;for (i=0;i<7;i++) MIX(st.cut_flags[i]);
      for (i=0;i<st.cut_count;i++) MIX(st.cut_list[i]); }
    MIX(st.rng);MIX(st.encounters);MIX(st.health);MIX(st.iframes);MIX(st.recoil);
    MIX(st.recoil_dir);MIX(st.kills);MIX(st.ticks);
    {u16 i;for (i=0;i<2;i++) {
        const MinishEnemy *e=&st.enemies[i];
        MIX(e->x);MIX(e->y);MIX(e->action);MIX(e->timer);MIX(e->face);MIX(e->age);
        MIX(e->hp);MIX(e->pose);MIX(e->display_pose);MIX(e->recoil);MIX(e->recoil_dir);MIX(e->fade);
    }for(i=0;i<4;i++) {
        MIX(st.rocks[i].x);MIX(st.rocks[i].y);MIX(st.rocks[i].life);MIX(st.rocks[i].face);MIX(st.rocks[i].age);
        MIX(st.effects[i].x);MIX(st.effects[i].y);
        MIX(((u16)st.effects[i].age<<8)|st.effects[i].kind);MIX(st.effects[i].display_pose);
    }}
#undef MIX
    return h;
}

u8 game_update(void)
{
#ifdef STATE_HASH
    frame_begin = BENCH_CYCLES;
#endif
    if (input_pressed(K_ESC)) return 0;
    if (input_pressed(K_ENTER)) game_scenario(0);
    if (rt_keys & (15|K_A|K_B)) st.preview = 0;
    if (!st.preview) { minish_step((u16)rt_keys); minish_step((u16)rt_keys); }
#ifdef STATE_HASH
    BENCH_VALUE(minish_hash());
#endif
    return 1;
}


void game_render(void)
{
    s16 x, y;
    if (!st.ready) {
        draw_clear(); draw_text(10,28,"Send data banks",F_MEDIUM,C_BLACK);
        draw_text(10,42,"mindat mizscene mizactor",F_SMALL,C_BLACK);
        draw_text(10,54,"mizact mizfight mizfx",F_SMALL,C_BLACK);
        return;
    }
    minish_zoom_render();
    minish_draw_health();
    /* The endpoint is a native exploration marker, not a ROM quest object. */
    x = minish_scaled(692)-st.camx; y = minish_scaled(136)-st.camy;
    if (x >= 2 && x < RT_W-8 && y >= 12 && y < RT_H-8) {
        draw_rect(x-1,y-11,3,12,C_WHITE);
        draw_rect(x,y-10,1,10,C_BLACK);
        draw_rect(x+1,y-10,6,4,C_BLACK);
    }
#ifdef STATE_HASH
    /* Conservative complete update+render cost, including hash/marker overhead. */
    BENCH_VALUE(BENCH_CYCLES-frame_begin);
#endif
}
