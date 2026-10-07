#include "mmx.h"
#include "terrain.h"
#include "art.h"
#include "generated/combat_boxes.h"
#include <string.h>
#ifdef MX_TRACE
#include "../../tools/m68kbench/bench.h"
#endif

MmxState mmx;
/* Within-step scratch: rewritten before motion, never carried across steps.
   The source fires before its horizontal wall clamp. */
static u16 muzzle_x;
void mmx_reset(void)
{
    memset(&mmx,0,sizeof(mmx));mmx.x=128;mmx.y=367;mmx.hp=16;mmx.grounded=1;
    mmx.facing=1;mmx.enabled=1;
}
static void camera(void)
{
    mmx.camx=mmx.x>144?(mmx.x-144)>>1:0;
    if(mmx.camx>352)mmx.camx=352;
    mmx.camy=mmx.y>400?(mmx.y-400)>>1:0;
    if(mmx.camy>28)mmx.camy=28;
}
static void move_x(void)
{
    s16 sum=mmx.xs+mmx.vx,next=(s16)mmx.x+(sum>>8),edge;
    mmx.xs=(u8)sum;
    if(next<8) { mmx.x=8;mmx.xs=0;return; }
    if(next>1016) { mmx.x=1016;mmx.xs=0;return; }
    muzzle_x=(u16)next;
    edge=next+(mmx.vx>0?8:-8);
    mmx.wall=0;
    if(mmx.vx && (terrain_solid(edge,mmx.y-8)||terrain_solid(edge,mmx.y+8))) {
        /* Side-contact uses the inner 7-pixel sensor at intended X; the
           8-pixel outer extent clamps first, before the slide flag appears. */
        s16 probe=next+(mmx.vx>0?7:-7);
        mmx.x=mmx.vx>0?(edge&~15)-8:(edge&~15)+24;
        mmx.wall=terrain_solid(probe,mmx.y-8)?(mmx.vx>0?1:2):0;
    } else mmx.x=next;
}
static void move_y(void)
{
    s16 sum=mmx.ys-mmx.vy,next=(s16)mmx.y+(sum>>8),floor=-1,s;
    mmx.ys=(u8)sum;
    if(mmx.vy>0) {
        if(terrain_solid(mmx.x,next-16)) {
            mmx.y=((next-16)&~15)+33;mmx.vy=0;
            mmx.state=MX_FALL;mmx.phase=0;return;
        }
    } else {
        s=terrain_surface(mmx.x-6,next+16);if(s>=0 && next+16>=s)floor=s;
        s=terrain_surface(mmx.x+6,next+16);if(s>=0 && next+16>=s && (floor<0 || s<floor))floor=s;
        if(floor>=0) { mmx.y=floor-17;mmx.grounded=1;return; }
    }
    mmx.y=next;mmx.grounded=0;
}
static void air(s16 direction,u16 buttons)
{
    mmx.vx=direction>0?376:direction<0?-376:0;
    if(direction)mmx.facing=direction>0;
    if(mmx.state==MX_RISE && (!(buttons&K_A) || mmx.vy<0)) {
        mmx.state=MX_FALL;mmx.vy=0;
    }
    if(mmx.state==MX_FALL && !mmx.phase)mmx.vy=0;
    mmx.phase=2;
    if(mmx.vy>-1536)mmx.vy-=64;
    move_x();move_y();
}
static void motion(u16 buttons,u16 pressed)
{
    s16 direction=(buttons&K_RIGHT)?1:(buttons&K_LEFT)?-1:0;
    u8 jump=(pressed&K_A)!=0;
    if(mmx.state==MX_DEAD || mmx.won)return;
    if(mmx.state==MX_HURT) {
        u8 supported=mmx.grounded;
        if(!mmx.phase) { mmx.phase=2;mmx.vy=512;mmx.vx=mmx.knockleft?-138:138;mmx.grounded=0;return; }
        if(mmx.timer && !--mmx.timer) { mmx.state=MX_IDLE;return; }
        if(mmx.grounded && mmx.vy<0)mmx.vy=0;
        mmx.vy-=64;move_x();move_y();
        if(supported && terrain_solid(mmx.x,mmx.y+17))mmx.grounded=1;
        return;
    }
    if(mmx.state==MX_KICK) {
        if(!mmx.phase) { mmx.phase=2;mmx.timer=4;return; }
        if(mmx.timer) { if(!--mmx.timer) { mmx.vx=mmx.wall==1?-376:376;mmx.vy=1363;mmx.lock=7;mmx.phase=4; } return; }
        if(mmx.lock) {
            --mmx.lock;mmx.vy-=64;move_x();move_y();
            return;
        }
        mmx.state=(buttons&K_A)?MX_RISE:MX_FALL;
        mmx.phase=(buttons&K_A)?2:0;return;
    }
    if(mmx.state==MX_SLIDE) {
        if(jump) { mmx.state=MX_KICK;mmx.phase=0;return; }
        if(mmx.wallage<20)++mmx.wallage;
        if(!mmx.phase) {
            mmx.phase=2;mmx.vx=mmx.wall==1?256:-256;mmx.vy=0;mmx.timer=8;
            muzzle_x=mmx.x+(mmx.vx>>8);return;
        }
        if(!mmx.wall) { mmx.state=MX_FALL;mmx.phase=0;return; }
        if(!direction || (direction>0?1:2)!=mmx.wall) { mmx.state=MX_FALL;mmx.phase=0; }
        else {
            if(mmx.timer && --mmx.timer) { muzzle_x=mmx.x+(mmx.vx>>8);return; }
            mmx.vy=-512;move_x();move_y();
            return;
        }
    }
    if(mmx.state==MX_RISE || mmx.state==MX_FALL) {
        if(mmx.grounded) {
            if(direction) { mmx.state=MX_RUN;mmx.phase=2;mmx.vy=0; }
            else { mmx.state=MX_LAND;mmx.timer=3;return; }
        } else {
            u8 wall=mmx.wall;
            if(wall && jump) { mmx.state=MX_KICK;mmx.phase=0;return; }
            air(direction,buttons);
            if(wall && mmx.wall && mmx.vy<=0 && direction) { mmx.state=MX_SLIDE;mmx.phase=0;mmx.wallage=0; }
            return;
        }
    }
    if(mmx.state==MX_LAND) {
        if(direction)mmx.state=MX_RUN;
        else { if(!--mmx.timer)mmx.state=MX_IDLE;return; }
    }
    if(mmx.state==MX_IDLE) {
        mmx.vx=mmx.vy=0;
        if(jump) { mmx.state=MX_RISE;mmx.vy=1363;mmx.phase=0;mmx.grounded=0;return; }
        if(direction) { mmx.facing=direction>0;mmx.state=MX_START;mmx.timer=5;return; }
        return;
    }
    if(jump) { mmx.state=MX_RISE;mmx.vy=1363;mmx.phase=2;mmx.grounded=0;air(direction,buttons);return; }
    /* A new jump still wins on the last unsupported RUN update. Otherwise
       enter FALL before horizontal integration, using last update's support. */
    if(mmx.state==MX_RUN && !mmx.grounded) {
        mmx.state=MX_FALL;mmx.phase=0;return;
    }
    if(!direction) { mmx.state=MX_IDLE;return; }
    mmx.facing=direction>0;
    if(mmx.state==MX_START) {
        if(!--mmx.timer) { mmx.state=MX_RUN;return; }
        mmx.vx=direction>0?256:-256;
    } else mmx.vx=direction>0?376:-376;
    move_x();
    mmx.grounded=terrain_solid(mmx.x-7,mmx.y+17) || terrain_solid(mmx.x+7,mmx.y+17);
}
static void spawn_shot(u8 kind,u8 previous)
{
    MmxShot *p=mmx.shots;
    u8 n=3,right=mmx.facing,dx=16,dy=3;
    while(n-- && p->active)++p;
    if(p==mmx.shots+3)return;
    /* Semantic pose families; one-pixel run bob is flattened on the LCD. */
    if(mmx.state==MX_RUN || (mmx.state==MX_IDLE && previous==MX_RUN)) { dx=27;dy=4; }
    else if(previous==MX_RUN && (mmx.state==MX_RISE || (mmx.state==MX_FALL && !mmx.phase))) {
        dx=27;dy=kind==3?4:5;
    }
    else if(mmx.state==MX_RISE && mmx.phase && previous!=MX_KICK) { dx=25;dy=8; }
    else if(mmx.state==MX_SLIDE && !mmx.phase) { dx=25;dy=mmx.airage<4?8:mmx.airage<8?7:6; }
    else if(mmx.state==MX_SLIDE || previous==MX_SLIDE) {
        if(mmx.wallage<6) { dx=18;dy=9; }
        else {
            right=!mmx.facing;
            if(mmx.wallage<7) { dx=18;dy=9; }
            else if(mmx.wallage<13) { dx=15;dy=5; }
            else { dx=19;dy=2; }
        }
    } else if(mmx.state==MX_KICK || previous==MX_KICK) {
        if(mmx.phase==2 && mmx.state==MX_KICK) { dx=15;dy=5; }
        else { dx=18;dy=9; }
    } else if(mmx.state==MX_FALL) { dx=25;dy=mmx.airage<4?8:mmx.airage<8?7:6; }
    /* Charged flight does not use XS. Canonicalize reused slots because the
       wider camera changes the old pellet's removal fraction; fresh is zero. */
    p->active=1;p->kind=kind;p->age=0;if(kind)p->xs=p->vx?64:0;
    p->follow=right?dx:-(s8)dx;p->x=muzzle_x+p->follow;p->y=mmx.y-dy;
    /* Large formation retains the free slot's velocity until its launch. */
    if(kind!=3) { p->vx=kind?1536:1024;if(!right)p->vx=-p->vx; }
    mmx.shooting=20;
}
static void weapons(u16 buttons,u16 pressed,u8 previous)
{
    MmxShot *p=mmx.shots;
    u8 n=3,launch=mmx.state==MX_KICK && mmx.phase==4 && mmx.lock==7;
    while(n--) {
        if(p->active) {
            if(p->active==2) { p->active=0;++p;continue; }
            s16 x=(s16)p->x;
            /* Charged removal lags integration; normal removal follows it.
               Both use 32-pixel grace. Signed positions can wrap below zero. */
            if(p->kind && (x+32<(s16)(mmx.camx<<1) || x>=(s16)((mmx.camx<<1)+352))) {
                p->active=0;++p;continue;
            }
            ++p->age;
            if(!p->kind) {
                s16 step=p->vx+(p->vx>0?64:-64),sum=p->xs+step;
                p->xs=(u8)sum;p->x+=sum>>8;
                p->vx=step>1536?1536:step<-1536?-1536:step;
            } else if(p->kind==1) {
                if(p->age>9)p->x+=p->vx>>8;
                else p->x=muzzle_x+p->follow;
            }
            else if(p->age==6)p->vx=p->follow>0?2048:-2048;
            else if(p->age>6)p->x+=p->vx>>8;
            else p->x=muzzle_x+p->follow;
            x=(s16)p->x;
            if(!p->kind && (x+32<(s16)(mmx.camx<<1) || x>=(s16)((mmx.camx<<1)+352)))p->active=0;
        }
        ++p;
    }
    if(mmx.shooting)--mmx.shooting;
    if(mmx.state==MX_DEAD) { mmx.charge=0;return; }
    if(mmx.pending) { spawn_shot(mmx.pending,previous);mmx.pending=0; }
    /* Charging continues during recoil. Firing remains blocked through the
       update which leaves HURT; input edges during that update are consumed. */
    if((pressed&K_B) && previous!=MX_HURT && !launch)spawn_shot(0,previous);
    if(buttons&K_B) { if(mmx.charge<180)++mmx.charge; }
    else {
        if(mmx.charge>=31 && previous!=MX_HURT) {
            u8 kind=mmx.charge>=101?3:1;
            /* The launch transition consumes release now, initializes the
               charged object next update; normal presses only start charge. */
            if(launch)mmx.pending=kind;
            else spawn_shot(kind,previous);
        }
        mmx.charge=0;
    }
}
static void actors(void)
{
    MmxEnemy *e=&mmx.enemy;
    MmxShot *p=mmx.shots;
    u8 n=3;
    const MxBox *box;
    if(e->explosion)--e->explosion;
    if(!e->spawned && mmx.x>=480) {
        e->spawned=1;e->active=1;e->x=624;e->y=368;return;
    }
    if(!e->active)return;
    if(!e->phase) { e->phase=2;e->y=364;e->hp=2;e->vx=-384;return; }
    if(e->fuse) { if(!--e->fuse)e->active=0;return; }
    if(e->phase==4) {
        if(!e->brake) { e->fuse=3;e->explosion=24;++mmx.kills;return; }
        --e->brake;e->vx+=5;
    } else e->vx=-384;
    { s16 sum=e->xs+e->vx;e->x+=sum>>8;e->xs=(u8)sum; }
    ++e->age;if(e->hurt)--e->hurt;
    if(e->x>=MX_WIDTH || e->x+40<(mmx.camx<<1)) { e->active=0;return; }
    box=mx_boxes+(e->phase==4?MXBOX_BODY:MXBOX_ARMOR);
    while(n--) {
        if(p->active==1) {
            const MxBox *shot=mx_boxes+(p->kind==3?mx_shotbox_3[p->age]:p->kind==1?mx_shotbox_1[p->age]:MXBOX_PELLET);
            u8 right=p->kind==3 && p->age<6?p->follow>0:p->vx>0;
            s16 dx=(s16)p->x+(right?-shot->x:shot->x)-e->x-box->x;
            s16 dy=(s16)p->y+shot->y-e->y-box->y;
            u8 rx=shot->rx+box->rx,ry=shot->ry+box->ry;
            if(dx>=-(s16)rx && dx<=rx && dy>=-(s16)ry && dy<=ry) {
                if(!p->kind)p->active=2; /* Cleanup on the following update. */
                ++mmx.hits;e->hurt=4;
                if(e->phase==4) { e->hp=0;e->fuse=1;e->explosion=24;++mmx.kills;return; }
                if(!p->kind && e->hp>1)--e->hp;
                else { e->hp=1;e->phase=4;e->brake=43;box=mx_boxes+MXBOX_BODY; }
            }
        }
        ++p;
    }
    if(!mmx.invincible) {
        const MxBox *hero=mx_boxes+MXBOX_HERO;
        s16 dx=(s16)mmx.x+hero->x-e->x-box->x,dy=(s16)mmx.y+hero->y-e->y-box->y;
        u8 rx=hero->rx+box->rx,ry=hero->ry+box->ry;
        if(dx>=-(s16)rx && dx<=rx && dy>=-(s16)ry && dy<=ry) {
            mmx.hp=mmx.hp>2?mmx.hp-2:0;mmx.invincible=92;
            mmx.state=mmx.hp?MX_HURT:MX_DEAD;mmx.phase=0;mmx.timer=30;
            mmx.knockleft=dx<=0;
        }
    }
}
void mmx_step(u16 buttons)
{
    u16 pressed=buttons&~mmx.buttons;
    u8 previous=mmx.state,oldphase=mmx.phase,shooting=mmx.shooting!=0;
    muzzle_x=mmx.x;
    mmx.buttons=buttons;++mmx.tick;
    if(mmx.won || mmx.state==MX_DEAD)return;
    if(mmx.invincible)--mmx.invincible;
    motion(buttons,pressed);
    if(mmx.state==MX_FALL && mmx.phase) {
        if(previous!=MX_FALL || !oldphase)mmx.airage=0;
        else if(mmx.airage<8)++mmx.airage;
    }
    weapons(buttons,pressed,previous);
    if(mmx.enabled)actors();
    if(mmx.y>=512) { mmx.hp=0;mmx.state=MX_DEAD; }
    if(mmx.x>=1008 && mmx.grounded)mmx.won=1;
    if(mmx.state!=previous || (mmx.shooting!=0)!=shooting)mmx.anim=mmx.animtimer=0;
    else if(++mmx.animtimer==4) { mmx.animtimer=0;mmx.anim=(mmx.anim+1)&31; }
    camera();
}
void game_init(void)
{
    terrain_init();art_init();mmx_reset();rt_state=&mmx;rt_state_size=sizeof(mmx);
}
void game_scenario(u16 n)
{
    u8 isolated=n>=100;
    if(isolated)n-=100;
    mmx_reset();
    if(n==1)mmx.enabled=0;
    if(n==2 || n==19 || n==20 || n==23 || n==24) {
        mmx.x=824;mmx.y=386;mmx.xs=64;mmx.ys=128;mmx.vx=376;mmx.vy=-768;
        mmx.grounded=0;mmx.state=MX_FALL;mmx.phase=2;mmx.wall=1;mmx.airage=8;
        if(n!=2) {
            mmx.charge=n==19 || n==24?31:101;mmx.shots[0].vx=1536;mmx.shots[0].xs=128;
            mmx.buttons=rt_keys=rt_prev=K_RIGHT|K_B;
        }
        if(n==23 || n==24) {
            mmx.y=391;mmx.vx=256;mmx.vy=-512;mmx.state=MX_KICK;mmx.phase=2;
            mmx.timer=1;mmx.wallage=9;mmx.buttons=rt_keys=rt_prev=K_RIGHT|K_A|K_B;
        }
    }
    if(n==3) { mmx.x=780;mmx.y=367; }
    if(n==4) { mmx.x=896;mmx.y=335; }
    if(n>=5 && n<=8) {
        mmx.x=505;mmx.enemy.x=600;mmx.enemy.y=364;mmx.enemy.hp=2;
        mmx.enemy.active=mmx.enemy.spawned=1;mmx.enemy.phase=2;mmx.enemy.vx=-384;
    }
    if(n==6)mmx.charge=101;
    if(n==7) { mmx.x=543;mmx.xs=64;mmx.vx=376;mmx.enemy.x=561;mmx.hp=14;mmx.state=MX_HURT;mmx.phase=0;mmx.timer=30;mmx.knockleft=1;mmx.invincible=92; }
    if(n==8) {
        u8 j;mmx.charge=101;mmx.enemy.explosion=24;
        for(j=0;j<3;++j) { MmxShot *p=mmx.shots+j;p->x=mmx.x+30+(j<<5);p->y=mmx.y-3;p->vx=1536;p->active=1;p->kind=j==2?3:j;p->age=14; }
    }
    if(n==9) { mmx.hp=0;mmx.state=MX_DEAD; }
    if(n>=10 && n<=15) {
        MmxShot *p=mmx.shots;u8 kind=(n-10)%3;
        p->x=n<13?(u16)-25:345;p->y=364;p->active=1;p->age=24;
        p->kind=kind==2?3:kind;p->xs=0;
        p->vx=p->kind==3?2048:1536;if(n<13)p->vx=-p->vx;
        if(p->kind) { p->x=n<13?(u16)-32:351;p->xs=64; }
    }
    if(n>=16 && n<=18) {
        mmx.x=n==18?500:505;mmx.xs=n==18?64:16;mmx.vx=376;mmx.state=MX_RUN;mmx.phase=2;
        mmx.enemy.x=n==18?606:600;mmx.enemy.y=364;mmx.enemy.hp=2;mmx.enemy.vx=-384;
        mmx.enemy.active=mmx.enemy.spawned=1;mmx.enemy.phase=2;
        if(n==17 || n==18) { mmx.charge=n==17?101:76;mmx.shots[0].vx=1536;mmx.shots[0].xs=64; }
    }
    if(n==21 || n==22) { mmx.enabled=0;mmx.shots[0].xs=n==21?64:192; }
    if(n==25 || n==43 || n==44) {
        mmx.x=807;mmx.xs=160;mmx.vx=376;mmx.state=MX_RUN;mmx.phase=2;
        mmx.grounded=0;mmx.buttons=rt_keys=rt_prev=K_RIGHT;
        mmx.shots[0].xs=128;mmx.shots[0].vx=2048;
        if(n!=25) {
            mmx.charge=n==43?31:101;mmx.shots[0].vx=1536;
            mmx.buttons=rt_keys=rt_prev=K_RIGHT|K_B;
        }
    }
    if(n>=26 && n<=41) {
        static const u8 fractions[4]={0,64,128,200};u8 p=n-26;
        mmx.x=821+(p>>2);mmx.xs=fractions[p&3];mmx.y=383;mmx.ys=128;
        mmx.vx=376;mmx.vy=-704;mmx.state=MX_FALL;mmx.phase=2;
        mmx.grounded=0;mmx.airage=8;mmx.buttons=rt_keys=rt_prev=K_RIGHT;
    }
    if(isolated)mmx.enabled=0;
    camera();
}
void mmx_export(u16 *w)
{
    u8 n;
#define PUT(v) *w++=(u16)(v)
#define FIELD(v) PUT(mmx.v)
    FIELD(x);FIELD(y);FIELD(camx);FIELD(camy);FIELD(tick);FIELD(vx);FIELD(vy);
    FIELD(xs);FIELD(ys);FIELD(state);FIELD(phase);FIELD(timer);FIELD(grounded);FIELD(facing);FIELD(wall);FIELD(lock);
    FIELD(buttons);FIELD(hp);FIELD(invincible);FIELD(charge);FIELD(shooting);FIELD(won);FIELD(enabled);
    FIELD(knockleft);FIELD(kills);FIELD(hits);FIELD(anim);FIELD(animtimer);
    for(n=0;n<3;++n) {
        const MmxShot *p=mmx.shots+n;
        PUT(p->x);PUT(p->y);PUT(p->vx);PUT(p->xs);PUT(p->kind);PUT(p->active);PUT(p->age);PUT(p->follow);
    }
    FIELD(enemy.x);FIELD(enemy.y);FIELD(enemy.xs);FIELD(enemy.hp);FIELD(enemy.active);FIELD(enemy.phase);
    FIELD(enemy.hurt);FIELD(enemy.explosion);FIELD(enemy.age);FIELD(enemy.spawned);
    FIELD(enemy.vx);FIELD(enemy.brake);FIELD(enemy.fuse);
    FIELD(wallage);FIELD(airage);FIELD(pending);
    /* Include the runtime input history: edge-sensitive jump/shot/reset. */
    PUT(rt_keys);PUT(rt_prev);PUT(rt_seed);
#undef FIELD
#undef PUT
}
u8 game_update(void)
{
    u16 keys=rt_keys;
    if(input_pressed(K_ESC))return 0;
    if(input_pressed(K_ENTER))game_scenario(0);
    else {
        /* Respect the runtime's launch/reset edge history before substep one. */
        mmx.buttons=(u8)rt_prev;
        mmx_step(keys);
#ifndef MX_REFERENCE
        mmx_step(keys);
#endif
    }
#ifdef MX_TRACE
    { u16 words[MMX_WORDS],n;mmx_export(words);for(n=0;n<MMX_WORDS;++n)BENCH_VALUE(words[n]); }
#endif
    return 1;
}
void game_render(void)
{
    draw_clear();
    if(!terrain_ready()) { draw_text(8,30,"MMXMAP MISSING",F_SMALL,C_BLACK);return; }
    terrain_render(mmx.camx,mmx.camy);
    if(art_ready()) { art_actors();art_hero(); }
    else if(mmx.state!=MX_DEAD && !(mmx.invincible&2)) {
        s16 x=(mmx.x>>1)-mmx.camx,y=((mmx.y-MX_TOP)>>1)-mmx.camy;
        draw_rect(x-6,y-9,12,18,C_WHITE);draw_rect(x-5,y-8,10,16,C_BLACK);
    }
    draw_rect(3,3,5,35,C_WHITE);draw_rect(4,4,3,33,C_BLACK);
    draw_rect(4,4,3,32-(mmx.hp<<1),C_WHITE);
    if(mmx.won || mmx.state==MX_DEAD) {
        draw_rect(32,35,101,18,C_BLACK);draw_rect(33,36,99,16,C_WHITE);
        draw_text(38,40,mmx.won?"CLEAR! ENTER":"RETRY: ENTER",F_SMALL,C_BLACK);
    }
}
