#include "art.h"
#include "mmx.h"
#include "terrain.h"
#include "generated/art_ids.h"
#include <string.h>
#include <stdlib.h>
typedef struct { RtSprite sprite; s8 x,y; } ArtFrame;
#ifdef __m68k__
static ArtFrame *frames;
static void __attribute__((__stkparm__)) release_frames(void)
{
    if(frames)free(frames);frames=RT_NULL;
}
#else
static ArtFrame frames[MXART_COUNT];
#endif
static u8 ready;
static u16 be16(const u8 *p) { return ((u16)p[0]<<8)|p[1]; }
u8 art_ready(void) { return ready; }
u8 art_init(void)
{
    const u8 *p,*r;
    u16 size,end,n;
    ready=0;
#ifdef __m68k__
    frames=RT_NULL;
#endif
    p=rt_file("mmxart",&size);
    if(!p || size<16 || memcmp(p,"MXA1",4) || be16(p+4)!=MXART_COUNT)return 0;
    end=be16(p+6);if(end>size || end<16+MXART_COUNT*8)return 0;
#ifdef __m68k__
    frames=malloc(sizeof(ArtFrame)*MXART_COUNT);
    if(!frames)return 0;
    if(atexit(release_frames)) { release_frames();return 0; }
#endif
    for(n=0,r=p+16;n<MXART_COUNT;++n,r+=8) {
        ArtFrame *f=frames+n;
        u16 offset=be16(r+4),length=be16(r+6);
        if((r[0]!=8 && r[0]!=16 && r[0]!=32) || !r[1] || r[1]>32 ||
           length!=(u16)((r[0]>>3)*r[1]) || (offset&3) || offset<16+MXART_COUNT*8 ||
           (u32)offset+3UL*length>end)return 0;
        f->sprite.w=r[0];f->sprite.h=r[1];f->x=(s8)r[2];f->y=(s8)r[3];
        f->sprite.light=p+offset;f->sprite.dark=p+offset+length;f->sprite.mask=p+offset+(length<<1);
    }
    return ready=1;
}
static void blit(u16 id,s16 x,s16 y)
{
    const ArtFrame *f;
    if(!ready)return;
    f=frames+id;draw_sprite(x+f->x,y+f->y,&f->sprite);
}
static u8 phase(u8 frame,u8 count)
{
    if(count==1)return 0;
    while(frame>=count)frame-=count;
    return frame;
}
/* Native LCD charge art: masked sparks converge, and armor shades pulse.
 * Keep the silhouette/white outline and draw each particle in one blit. */
static const u8 spark_small_pixels[]={0,0x10,0};
static const u8 spark_small_mask[]={0xef,0xc7,0xef};
static const u8 spark_light[]={0,0x10,0x38,0x10,0};
static const u8 spark_dark[]={0,0x10,0x28,0x10,0};
static const u8 spark_mask[]={0xef,0xc7,0x83,0xc7,0xef};
static const RtSprite spark_small={8,3,spark_small_pixels,spark_small_pixels,spark_small_mask};
static const RtSprite spark={8,5,spark_light,spark_dark,spark_mask};

static void charge_effect(s16 x,s16 y,u8 right)
{
    static const u8 distance_x[]={18,17,16,15,13,12,11,10,9,8,7,6,5,4,2,1};
    static const u8 distance_y[]={12,11,10,9,8,7,6,6,5,4,4,3,2,2,1,0};
    u8 full=mmx.charge>=101,beat=(mmx.tick>>1)&15,i;
    s16 bx=x+(right?5:-5),by=y-4;
    for(i=0;i<(full?4:2);++i) {
        u8 p=(beat+(i<<2))&15;
        s16 dx=distance_x[p],dy=distance_y[p],sx,sy;
        if(!(i&1))dx=-dx;
        if(i==0 || i==2)dy=-dy;else dy>>=1;
        sx=bx+(right?dx:-dx);sy=by+dy;
        if(p<8)draw_sprite(sx-3,sy-1,&spark_small);
        else draw_sprite(sx-3,sy-2,&spark);
    }
    if(full && (beat&2))draw_sprite(bx-3,by-2,&spark);
    else draw_sprite(bx-3,by-1,&spark_small);
}

static void hero_blit(u16 id,s16 x,s16 y)
{
    const ArtFrame *f;
    RtSprite pose;
    u8 glow;
    if(!ready)return;
    f=frames+id;pose=f->sprite;
    if(mmx.charge>=31) {
        glow=(mmx.tick>>(mmx.charge>=101?2:3))&3;
        if(glow&1) { pose.light=f->sprite.dark;pose.dark=f->sprite.light; }
        else if(glow==2 && mmx.charge>=101)pose.dark=pose.light;
    }
    draw_sprite(x+f->x,y+f->y,&pose);
}
void art_hero(void)
{
    u16 id=MXART_IDLE;u8 count=MXART_IDLE_N,first=0;
    s16 x=(mmx.x>>1)-mmx.camx,y=((mmx.y-MX_TOP)>>1)-mmx.camy;
#define POSE(name) do { id=mmx.facing?MXART_##name:MXART_##name##_LEFT;count=MXART_##name##_N; } while(0)
    if(mmx.state==MX_DEAD || (mmx.invincible&2))return;
    POSE(IDLE);
    if(mmx.state==MX_RUN || mmx.state==MX_START)POSE(RUN);
    if(mmx.state==MX_RISE)POSE(JUMP);
    if(mmx.state==MX_FALL)POSE(FALL);
    if(mmx.state==MX_SLIDE)POSE(SLIDE);
    if(mmx.state==MX_KICK)POSE(KICK);
    if(mmx.state==MX_KICK && mmx.phase==4 && mmx.lock==7)POSE(KICK_PUSH);
    if(mmx.shooting) {
        POSE(SHOOT);
        if(mmx.state==MX_RUN || mmx.state==MX_START)POSE(RUN_SHOOT);
        if(mmx.state==MX_RISE || mmx.state==MX_FALL)POSE(JUMP_SHOOT);
        if(mmx.state==MX_SLIDE)POSE(WALL_SHOOT);
        if((mmx.state==MX_RISE && mmx.phase) || mmx.state==MX_FALL)first=1;
        if(mmx.state==MX_SLIDE && mmx.phase && mmx.wallage>=6)first=mmx.wallage<13?2:count-1;
        if(mmx.state==MX_KICK) {
            if(!mmx.phase) { POSE(WALL_SHOOT);first=mmx.wallage>=6?2:0; }
            else if(mmx.phase==2)POSE(KICK_WINDUP_SHOOT);
            else if(mmx.lock<7)POSE(KICK_LAUNCH_SHOOT);
            else POSE(KICK_PUSH);
        }
    }
    if(mmx.state==MX_HURT)POSE(HURT);
    /* Idle waits are longer than the running cycle; avoid frantic blinking. */
    hero_blit(id+first+phase(mmx.state==MX_IDLE && !mmx.shooting?(mmx.anim>>3):mmx.anim,count-first),x,y);
    if(mmx.charge>=31) {
        u8 outward=((mmx.state==MX_SLIDE && mmx.phase) || (mmx.state==MX_KICK && !mmx.phase)) && mmx.wallage>=6;
        u8 right=outward?!mmx.facing:mmx.facing;
        charge_effect(x,y,right);
    }
#undef POSE
}
void art_actors(void)
{
    const MmxEnemy *e=&mmx.enemy;
    const MmxShot *p=mmx.shots;
    u8 n=3;
    if(e->active && !e->fuse && !(e->hurt&2)) {
        u16 id=e->phase==4?MXART_ROLLER_BROKEN:MXART_ROLLER;
        u8 count=e->phase==4?MXART_ROLLER_BROKEN_N:MXART_ROLLER_N;
        u8 frame=e->phase==4?(43-e->brake)>>1:(e->age>>2)&31;
        blit(id+phase(frame,count),(e->x>>1)-mmx.camx,((e->y-MX_TOP)>>1)-mmx.camy);
    }
    if(e->explosion) {
        s16 x=(e->x>>1)-mmx.camx,y=((e->y-MX_TOP)>>1)-mmx.camy;
#ifdef MXART_EXPLOSION
        blit(MXART_EXPLOSION+phase((24-e->explosion)>>1,MXART_EXPLOSION_N),x,y);
#else
        draw_rect(x-6,y-6,13,13,C_WHITE);draw_rect(x-4,y-4,9,9,C_BLACK);
#endif
    }
    while(n--) {
        if(p->active) {
            u8 right=p->kind==3 && p->age<6?p->follow>0:p->vx>0;
            u16 id=right?MXART_PELLET:MXART_PELLET_LEFT;u8 count=MXART_PELLET_N;
            if(p->kind==1) { id=right?MXART_MEDIUM:MXART_MEDIUM_LEFT;count=MXART_MEDIUM_N; }
            if(p->kind==3) { id=right?MXART_LARGE:MXART_LARGE_LEFT;count=MXART_LARGE_N; }
            { u8 frame=0;
              if(!p->kind) { frame=(p->age>>1);if(frame>=count)frame=count-1; }
              else if(p->kind==1 && p->age<=9) { frame=(p->age?p->age-1:0)>>1;if(frame>=MXART_MEDIUM_START_N)frame=MXART_MEDIUM_START_N-1; }
              else if(p->kind==3 && p->age<=6) { frame=p->age>=3;if(frame>=MXART_LARGE_START_N)frame=MXART_LARGE_START_N-1; }
              else {
                  if(p->kind==1) { id=p->vx>0?MXART_MEDIUM_FLIGHT:MXART_MEDIUM_LEFT_FLIGHT;count=MXART_MEDIUM_FLIGHT_N;frame=(p->age-10)>>1; }
                  else { id=p->vx>0?MXART_LARGE_FLIGHT:MXART_LARGE_LEFT_FLIGHT;count=MXART_LARGE_FLIGHT_N;frame=(p->age-7)>>1; }
                  frame=phase(frame,count);
              }
              blit(id+frame,((s16)p->x>>1)-mmx.camx,((p->y-MX_TOP)>>1)-mmx.camy);
            }
        }
        ++p;
    }
}
