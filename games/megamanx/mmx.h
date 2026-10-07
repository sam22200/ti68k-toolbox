#ifndef MMX_GAME_H
#define MMX_GAME_H
#include "../../runtime/core/rt.h"
enum { MX_IDLE=0,MX_START=2,MX_RUN=4,MX_RISE=6,MX_FALL=8,MX_LAND=10,
       MX_DEAD=12,MX_HURT=14,MX_KICK=16,MX_SLIDE=18 };
typedef struct { u16 x,y; s16 vx; u8 xs,kind,active,age; s8 follow; } MmxShot;
typedef struct { u16 x,y; s16 vx; u8 xs,hp,active,phase,hurt,explosion,age,spawned,brake,fuse; } MmxEnemy;
typedef struct {
    u16 x,y,camx,camy,tick;
    s16 vx,vy;
    u8 xs,ys,state,phase,timer,grounded,facing,wall,lock;
    u8 buttons,hp,invincible,charge,shooting,won,enabled;
    u8 knockleft,kills,hits;
    u8 anim,animtimer,wallage,airage,pending;
    MmxShot shots[3];
    MmxEnemy enemy;
} MmxState;
extern MmxState mmx;
void mmx_reset(void);
void mmx_step(u16 buttons);
#define MMX_WORDS 71
void mmx_export(u16 *words);
#endif
