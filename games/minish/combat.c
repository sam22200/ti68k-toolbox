/* Two original opening Octoroks. Local deterministic AI, measured ordinary
 * movement/combat parameters; bounded four-rock pool and native retry. */
#include "minish.h"
#include "generated.h"
#include "combat_generated.h"
#include "hud_generated.h"
static const u32 *pixels;
static const s16 knock_x[8]={0,452,640,452,0,-452,-640,-452};
static const s16 knock_y[8]={-640,-452,0,452,640,452,0,-452};
static const s16 death_x[8]={0,271,384,271,0,-271,-384,-271};
static const s16 death_y[8]={-384,-271,0,271,384,271,0,-271};
static const s16 move_x[4]={0,96,0,-96},move_y[4]={-96,0,96,0};
static const s8 nut_x[4]={0,4,0,-4},nut_y[4]={-3,0,2,0};
static const u8 walks[4]={30,60,60,90};

u8 minish_combat_init(void)
{
    u16 size;
#ifdef MINISH_ZOOM
    pixels=(const u32 *)rt_file("mizfight",&size);
#else
    pixels=(const u32 *)rt_file("mifight",&size);
#endif
    return pixels && size>=FIGHT_SIZE && size<=FIGHT_SIZE+6;
}

void minish_combat_reset(void)
{
    u16 i;
    st.encounters=0;st.health=24;st.iframes=st.recoil=st.recoil_dir=st.kills=st.ticks=0;
    st.rng=0x5a37;
    for (i=0;i<2;i++) {
        MinishEnemy *e=&st.enemies[i];
        e->x=e->y=0;e->action=e->timer=e->face=e->age=e->hp=0;
        e->pose=e->display_pose=e->recoil=e->recoil_dir=e->fade=0;
    }
    for (i=0;i<4;i++) {st.rocks[i].x=st.rocks[i].y=0;st.rocks[i].life=st.rocks[i].face=st.rocks[i].age=0;}
}

void minish_combat_start(void)
{
    u16 i;
    st.encounters=1;
    for (i=0;i<2;i++) {
        MinishEnemy *e=&st.enemies[i];
        e->x=(s32)enemy_spawn[i][0]<<8;e->y=(s32)enemy_spawn[i][1]<<8;
        e->action=1;e->timer=24+(i<<4);e->hp=2;e->face=i ? 0 : 2;
        e->pose=e->display_pose=enemy_walk[e->face][0];
    }
}

static u16 random_word(void)
{
    u16 n=st.rng;
    n^=n<<7;n^=n>>9;n^=n<<8;
    return st.rng=n;
}

static u8 direction(s16 dx,s16 dy)
{
    u16 ax=dx<0 ? -dx : dx,ay=dy<0 ? -dy : dy;
    /* Eight-way native recoil. Cardinal/diagonal original values are retained. */
    if (ax>(ay<<1)) return dx<0 ? 6 : 2;
    if (ay>(ax<<1)) return dy<0 ? 0 : 4;
    return dy<0 ? (dx<0 ? 7 : 1) : (dx<0 ? 5 : 3);
}

static u8 free_body(s16 x,s16 y)
{
    return !minish_solid(x-5,y-6) && !minish_solid(x+5,y-6) &&
           !minish_solid(x-5,y) && !minish_solid(x+5,y) &&
           !minish_solid(x,y-8) && !minish_solid(x,y+2);
}

u8 minish_combat_before(u16 keys)
{
    s32 x,y;
    if (!st.encounters) return 0;
    st.ticks++;
    if (st.iframes) st.iframes--;
    if (!st.health) {st.moving=0;return 1;}
    if (!st.recoil) return 0;
    st.attack=st.roll=st.roll_guard=0;st.last_a=(keys&K_A)!=0;st.last_b=(keys&K_B)!=0;st.moving=0;
    st.display_pose=st.pose;
    if (!--st.recoil) return 1;
    /* Source knockback animation 24+facing, then idle on the last update. */
    st.pose=st.recoil>1 ? MINISH_HURT+hurt_seq[st.anim_face][8-st.recoil] : idle_pose[st.anim_face];
    x=st.x+knock_x[st.recoil_dir];y=st.y+knock_y[st.recoil_dir];
    if (free_body(x>>8,st.y>>8)) st.x=x;
    if (free_body(st.x>>8,y>>8)) st.y=y;
    return 1;
}

static u8 overlap(s16 dx,s16 dy,u16 w,u16 h)
{
    /* Original boxes include their boundary pixels. */
    return (u16)(dx+w)<=w+w && (u16)(dy+h)<=h+h;
}

static u8 sword_hit(s16 x,s16 y,u16 radius)
{
    const s8 *b;
    if (!st.attack) return 0;
    b=sword_boxes[st.anim_face][st.attack-1];
    if (!b[2] || !b[3]) return 0;
    return overlap(x-(st.x>>8)-b[0],y-(st.y>>8)-b[1],radius+b[2],radius+b[3]);
}

static void hurt(s16 x,s16 y)
{
    if (st.iframes || st.roll_guard || !st.health) return;
    st.health=st.health<2 ? 0 : st.health-2;
    st.iframes=30;st.recoil=8;
    st.recoil_dir=direction((st.x>>8)-x,(st.y>>8)-y);
    st.attack=st.roll=st.roll_guard=0;st.pose=MINISH_HURT+hurt_seq[st.anim_face][0];
}

static void pause_enemy(MinishEnemy *e)
{
    e->action=1;e->timer=24+(random_word()&0x38);
}

static void spit(MinishEnemy *e)
{
    u16 i;
    for (i=0;i<4;i++) if (!st.rocks[i].life) {
        MinishRock *r=&st.rocks[i];
        r->x=e->x+(s32)nut_x[e->face]*256;
        r->y=e->y+(s32)nut_y[e->face]*256;
        r->face=e->face;r->life=48;r->age=0;
        return;
    }
}

void minish_combat_step(void)
{
    u16 i;
    s16 px=st.x>>8,py=st.y>>8;
    if (!st.encounters || !st.health) return;
    /* Existing projectiles move first; a newly spat nut starts next update. */
    for (i=0;i<4;i++) if (st.rocks[i].life) {
        MinishRock *r=&st.rocks[i];
        s16 x,y;
        r->x+=knock_x[r->face<<1];r->y+=knock_y[r->face<<1];r->age++;
        x=r->x>>8;y=r->y>>8;
        if (minish_solid(x,y) || !--r->life || sword_hit(x,y,2)) {r->life=0;continue;}
        if (overlap(x-px,y-(py-3),8,8)) {hurt(x,y);r->life=0;}
    }
    for (i=0;i<2;i++) {
        MinishEnemy *e=&st.enemies[i];
        s16 x=e->x>>8,y=e->y>>8;
        e->display_pose=e->pose;
        if (!e->hp) {
            if (e->recoil) {
                s32 nx=e->x+death_x[e->recoil_dir];
                s32 ny=e->y+death_y[e->recoil_dir];
                --e->recoil;
                if (free_body(nx>>8,ny>>8)) {e->x=nx;e->y=ny;}
            } else if (e->fade) --e->fade;
            continue;
        }
        if (e->recoil) {--e->recoil;continue;}
        if (e->action==1) {
            if (!--e->timer) {
                u16 r=random_word();
                e->action=2;e->timer=walks[r&3];e->face=(r>>3)&3;e->age=0;
                if (x<240) e->face=1;
                if (x>368) e->face=3;
                if (y<48) e->face=2;
                if (y>176) e->face=0;
            }
        } else if (e->action==2) {
            s32 nx=e->x+move_x[e->face],ny=e->y+move_y[e->face];
            if (free_body(nx>>8,ny>>8)) {e->x=nx;e->y=ny;}
            if (!--e->timer) {
                /* Native AI: turn toward nearby Link before the measured
                   spit animation. Wandering orientation must not hide shots. */
                s16 dx=px-(e->x>>8),dy=py-(e->y>>8);
                u8 target=(direction(px-(e->x>>8),py-(e->y>>8))+1)>>1;
                if (dx>=-128 && dx<=128 && dy>=-96 && dy<=96 && (random_word()&3)) {
                    e->face=target&3;e->action=3;e->age=0;
                }
                else pause_enemy(e);
            }
        } else {
            if (e->age==19) spit(e);
            if (e->age==27) pause_enemy(e);
        }
        e->pose=e->action==3 ? enemy_shoot[e->face][e->age<28 ? e->age : 27] : enemy_walk[e->face][e->age&31];
        e->age++;
        x=e->x>>8;y=e->y>>8;
        if (sword_hit(x,y-3,6)) {
            e->hp=0;e->recoil=12;e->fade=minish_death_length();
            e->recoil_dir=direction(x-px,y-py);st.kills++;
        } else if (!st.iframes && overlap(x-px,y-py,12,12)) {
            hurt(x,y);e->recoil=1;
        }
    }
}

void minish_draw_enemies(u8 front)
{
    u16 i;
    if (!st.encounters) return;
    for (i=0;i<2;i++) {
        const MinishEnemy *e=&st.enemies[i];
        const s16 *m;
#ifdef MINISH_ZOOM
        const u16 *p;
#else
        const u32 *p;
#endif
        s16 x,y;
        RtSprite s;
        if ((!e->hp && !e->recoil && !e->fade) || (((e->y>>8)>=(st.y>>8))!=front)) continue;
        if (!e->hp && !e->recoil) {minish_draw_death(e);continue;}
        if (!e->hp && (st.ticks&2)) continue;
        m=enemy_art[e->display_pose];
#ifdef MINISH_ZOOM
        p=(const u16 *)pixels+m[0];
#else
        p=pixels+m[0];
#endif
#ifdef MINISH_ZOOM
        x=minish_scaled(e->x>>8)-st.camx+m[1];y=minish_scaled(e->y>>8)-st.camy+m[2];
#else
        x=(e->x>>8)-st.camx+m[1];y=(e->y>>8)-st.camy+m[2];
#endif
        if (x>=RT_W || x+m[3]<=0 || y>=RT_H || y+m[4]<=0) continue;
        s.w=m[3];s.h=m[4];s.light=p;s.dark=p+s.h;s.mask=p+(s.h<<1);
#ifndef MINISH_ZOOM
        s.w=32;
#endif
#if defined(__m68k__) && defined(MINISH_ZOOM)
        {s16 aligned=x&~15;
         if (aligned>=0 && aligned<=RT_W-32 && y>=0 && y+s.h<=RT_H) {
             const u32 *q=(const u32 *)((const u8 *)pixels+enemy_shift[e->display_pose]);
             u16 row,offset=((u16)y<<5)-((u16)y<<1)+((u16)aligned>>3);
             u8 *l=(u8 *)rt_light+offset,*d=(u8 *)rt_dark+offset;
             q+=((u16)x&15)*s.h*3;
             for (row=0;row<s.h;row++,l+=RT_PBYTES,d+=RT_PBYTES,q+=3) {
                 *(u32 *)l=(*(u32 *)l&q[2])|q[0];*(u32 *)d=(*(u32 *)d&q[2])|q[1];
             }
         } else draw_sprite(x,y,&s);
        }
#else
        draw_sprite(x,y,&s);
#endif
        minish_canopy(x,y,m[3],m[4]);
    }
    if (!front) return;
    for (i=0;i<4;i++) if (st.rocks[i].life) {
        s16 x,y;
        RtSprite s;
#ifdef MINISH_ZOOM
        x=minish_scaled(st.rocks[i].x>>8)-st.camx-4;
        y=minish_scaled(st.rocks[i].y>>8)-st.camy-6;
#else
        x=(st.rocks[i].x>>8)-st.camx-4;y=(st.rocks[i].y>>8)-st.camy-7;
#endif
        s.w=8;s.h=8;s.light=rock_light;s.dark=rock_dark;s.mask=rock_mask;
#ifdef __m68k__
        {s16 aligned=x&~15;
         if (aligned>=0 && aligned<=RT_W-32 && y>=0 && y<=RT_H-8) {
             const u32 *p=(const u32 *)((const u8 *)pixels+ROCK_BASE)+((u16)x&15)*24;u16 r;
             u16 offset=((u16)y<<5)-((u16)y<<1)+((u16)aligned>>3);
             u8 *l=(u8 *)rt_light+offset,*d=(u8 *)rt_dark+offset;
             for (r=0;r<8;r++,l+=RT_PBYTES,d+=RT_PBYTES,p+=3) {
                 *(u32 *)l=(*(u32 *)l&p[2])|p[0];*(u32 *)d=(*(u32 *)d&p[2])|p[1];
             }
         } else draw_sprite(x,y,&s);
        }
#else
        draw_sprite(x,y,&s);
#endif
        minish_canopy(x,y,8,8);
    }
}

/* The source damage palette pulses in four 4-update phases over the
 * invulnerability: on four greys, darker / black body / black / ordinary.
 * White outline pixels stay white; Link never disappears. */
u8 minish_flash;

void minish_flash_begin(void)
{
    static const u8 phases[4]={1,2,2,0};
    minish_flash=st.encounters && st.iframes ? phases[((30-st.iframes)>>2)&3] : 0;
}

void minish_link_sprite(s16 x,s16 y,const RtSprite *s)
{
    u32 buf[128];
    const u32 *l=s->light,*d=s->dark;
    RtSprite t;
    u16 r;
    if (!minish_flash || s->h>64) {draw_sprite(x,y,s);return;}
    /* Grey g = 2*dark+light: one step darker is (dark|light, dark),
       black is (dark|light, dark|light); white (0,0) is unchanged. */
    for (r=0;r<s->h;r++) {
        u32 a=d[r]|l[r];
        buf[r]=minish_flash==2 ? a : d[r];buf[64+r]=a;
    }
    t=*s;t.light=buf;t.dark=buf+64;
    draw_sprite(x,y,&t);
}

void minish_draw_hurt(u8 pose,u8 cover)
{
    const s16 *m=hurt_art[pose];
    const u32 *p=(const u32 *)((const u8 *)pixels+HURT_BASE)+m[0];
    s16 x,y;u16 part,h=m[3];RtSprite s;
#ifdef MINISH_ZOOM
    x=minish_scaled(st.x>>8)-st.camx+m[1];y=minish_scaled(st.y>>8)-st.camy+m[2];
#else
    x=(st.x>>8)-st.camx+m[1];y=(st.y>>8)-st.camy+m[2];
#endif
    s.w=32;s.h=h;
    for (part=0;part<m[4];part++,p+=h*3) {
        s.light=p;s.dark=p+h;s.mask=p+(h<<1);
        minish_link_sprite(x+(part<<5),y,&s);
    }
    if (cover) minish_canopy(x,y,m[5],h);
}

void minish_draw_health(void)
{
    RtSprite s;
    if (!st.encounters) return;
    s.w=32;s.h=8;s.light=heart_light[st.health>>1];s.dark=heart_dark[st.health>>1];s.mask=heart_mask;
#ifdef __m68k__
    {u16 r;u8 *l=(u8 *)rt_light+16,*d=(u8 *)rt_dark+16;
     const u32 *hl=s.light,*hd=s.dark,*m=heart_mask;
     for (r=0;r<8;r++,l+=RT_PBYTES,d+=RT_PBYTES) {
         u32 mask=*m++;
         *(u32 *)l=(*(u32 *)l&mask)|*hl++;
         *(u32 *)d=(*(u32 *)d&mask)|*hd++;
     }}
#else
    draw_sprite(RT_W-32,0,&s);
#endif
    if (!st.health) {
        draw_rect(31,39,98,24,C_WHITE);
        draw_text(52,42,"GAME OVER",F_SMALL,C_BLACK);
        draw_text(42,53,"ENTER: RETRY",F_SMALL,C_BLACK);
    }
}

void minish_draw_label(void)
{
#ifdef __m68k__
    u16 r;u8 *l=(u8 *)rt_light,*d=(u8 *)rt_dark;
    for (r=0;r<8;r++,l+=RT_PBYTES,d+=RT_PBYTES) {
        *(u32 *)l=*(u32 *)d=label_rows[0][r];
        *(u32 *)(l+4)=*(u32 *)(d+4)=label_rows[1][r];
        l[8]&=0x3f;d[8]&=0x3f;
    }
#else
    draw_rect(0,0,66,8,C_WHITE);
    draw_text(2,1,"MINISH WOODS",F_SMALL,C_BLACK);
#endif
}
