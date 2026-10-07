#include "yoshi.h"
#include "terrain.h"
#include "art.h"
#include "generated/coins.h"
#include "generated/egg_angles.h"
#include "generated/egg_cursor.h"
#include <string.h>
#ifdef YJ_ZONES
#include "../../tools/m68kbench/bench.h"
#endif

static void counter(void)
{
    static const u8 digits[10][7] = {
        {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
        {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
        {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
        {14,17,17,15,1,1,14}
    };
    YoshiItems *i=&yoshi.items;
    u16 tens=0,ones=i->count,n;
    while(ones>=10) { ones-=10; ++tens; }
    i->hud[0]=i->hud[8]=0;
    for(n=0;n<7;++n) i->hud[n+1]=((u16)digits[tens][n]<<9)|((u16)digits[ones][n]<<2);
}

void items_reset(void)
{
    YoshiItems *i=&yoshi.items;
    u16 n;
    memset(i,0,sizeof(*i)); i->enabled=1;
    /* Seed a spatial path behind the player. A stationary Yoshi does not
       write duplicate samples: eggs keep their spacing through long idles. */
    for(n=0;n<YI_TRAIL;++n) {
        s16 x=(s16)yoshi.x+4-(s16)(n<<2);
        i->trail[(0-n)&63].x=x<8?8:x;
        i->trail[(0-n)&63].y=yoshi.y+16;
    }
    counter();
}

void items_scenario(u16 n)
{
    YoshiItems *i=&yoshi.items;
    items_reset();
    if(n && n<60) i->enabled=0; /* Preserve isolated earlier reference doors. */
    if(n>=60 && n<=70) {
        yoshi.action.enabled=1;
        memset(yoshi.action.actors,0,sizeof(yoshi.action.actors));
        yoshi.baby.enabled=1;
        if(n==60) { yoshi.x=288; yoshi.y=1824; }
        else if(n==61) { yoshi.x=400; yoshi.y=1904; yoshi.action.eggs=6; }
        else if(n==62 || n==63) {
            YoshiActor *a=yoshi.action.actors;
            yoshi.x=464; yoshi.y=n==63?1868:1888;
            a->x=a->home=496; a->y=1904; a->state=16; a->awake=1; a->timer=20;
            if(n==63) { yoshi.x=496; yoshi.grounded=0; yoshi.vy=640; }
        } else if(n==64) {
            YoshiActor *a=yoshi.action.actors;
            u16 k;
            yoshi.x=256; yoshi.y=1904; yoshi.action.eggs=6;
            for(k=0;k<YA_COUNT;++k,++a) {
                a->x=a->home=144+(k<<4)+(k<<3); a->y=1904; a->state=16; a->awake=1;
            }
            yoshi.baby.mode=YB_LOST; yoshi.baby.phase=11; yoshi.baby.age=80;
            yoshi.baby.x=300; yoshi.baby.y=1836; yoshi.baby.vy=-319;
        } else if(n>=67) {
            YoshiActor *a=yoshi.action.actors;
            if(n==67) { yoshi.x=464; yoshi.y=1888; a->x=a->home=528; a->y=1904; }
            else if(n==68 || n==69) {
                yoshi.x=n==69?288:384; yoshi.y=1904;
                a->x=a->home=n==69?400:424; a->y=1920;
                if(n==69) yoshi.action.eggs=3;
            } else { yoshi.x=288; yoshi.y=1904; yoshi.action.eggs=1; }
            a->state=n==70?0:16; a->awake=1; a->timer=200;
        } else { yoshi.x=1008; yoshi.y=1740; yoshi.action.eggs=6; }
        yoshi.x_sub=0;
        yoshi.camx=yoshi.x>144?(yoshi.x-144)>>1:0;
        yoshi.camy=(yoshi.y-144-YT_TOP)>>1;
        items_reset();
        if(n==66) {
            u16 k;
            /* Six in-flight eggs with their food actors already consumed. */
            yoshi.action.eggs=0;
            yoshi.baby.mode=YB_LOST; yoshi.baby.phase=11; yoshi.baby.age=80;
            yoshi.baby.x=yoshi.x-100; yoshi.baby.y=yoshi.y-64; yoshi.baby.vy=-319;
            for(k=0;k<YI_EGGS;++k) {
                YoshiEgg *e=i->shots+k;
                e->x=yoshi.x-128+(k<<4); e->y=yoshi.y-48;
                e->vx=2032; e->vy=-1016; e->life=180;
            }
        }
    }
}

u8 items_collect(u16 x,u16 y,u16 width,u16 height)
{
    YoshiItems *i=&yoshi.items;
    const u16 *p;
    u16 n,end,bin; u8 gained=0;
    if(!i->enabled) return 0;
    if(x>=1280) return 0;
    /* Include the coin's 16px width on the left of the query rectangle. */
    n=coin_start[x>=16?(x-16)>>7:0];
    bin=((x+width)>>7)+1; if(bin>10) bin=10;
    end=coin_start[bin]; p=coin_xy[n];
    for(;n<end;++n,p+=2) {
        u8 bit;
        if(x+width<p[0] || x>=p[0]+16 || y+height<p[1] || y>=p[1]+16) continue;
        bit=1<<(n&7);
        if(i->collected[n>>3]&bit) continue;
        i->collected[n>>3]|=bit; ++i->count; ++gained;
    }
    if(gained) counter();
    return gained;
}

static u8 angle(void)
{
    return (4-yoshi.items.phase)&63;
}

static void launch(void)
{
    YoshiItems *i=&yoshi.items;
    YoshiEgg *e=i->shots;
    u16 n; u8 a=angle();
    for(n=0;n<YI_EGGS;++n,++e) if(!e->life) {
        memset(e,0,sizeof(*e));
        u8 facing=i->locked?i->aim_facing:yoshi.action.facing;
        e->x=yoshi.x+(facing?0:16); e->y=yoshi.y+8;
        e->vx=egg_sin[(a+16)&63]; e->vy=egg_sin[a];
        if(facing) e->vx=-e->vx;
        e->life=180;
        --yoshi.action.eggs;
        /* The nearest follower is launched; shift the rest towards Yoshi. */
        memmove(i->followers,i->followers+1,sizeof(i->followers[0])*(YI_EGGS-1));
        i->reserve=yoshi.action.eggs; i->throwing=10; i->aim=i->locked=0;
        return;
    }
    /* A full projectile pool preserves the reserve and active aim. */
}

static void projectile(YoshiEgg *e)
{
    s16 sum,nx,ny,floor;
    u16 k; YoshiActor *a;
    u8 bounce=0;
    if(!e->life || !--e->life) return;
    sum=e->xs+e->vx; e->xs=(u8)sum; nx=e->x+(sum>>8);
    sum=e->ys+e->vy; e->ys=(u8)sum; ny=e->y+(sum>>8);
    /* At <9 pixels/step a shot cannot jump across a 16px solid cell.
       Axis checks also catch a diagonal corner before entering it. */
    if(terrain_solid(nx+8,e->y+8)) { e->vx=-e->vx; nx=e->x; bounce=1; }
    floor=ny>(s16)e->y?terrain_actor_floor(nx+8,e->y+16,ny+16):YT_NONE;
    if(terrain_solid(nx+8,ny+8) || floor!=YT_NONE) {
        e->vy=-e->vy; ny=e->y; bounce=1;
    }
    e->x=nx; e->y=ny;
    if(bounce && ++e->bounces==3) { e->life=0; return; }
    if(nx<0 || nx>1280 || ny<(s16)YT_TOP || ny>=(s16)YT_BOTTOM) { e->life=0; return; }
    items_collect(e->x,e->y,16,16);
    for(k=0,a=yoshi.action.actors;k<YA_COUNT;++k,++a) {
        if(a->state!=16 || !a->awake) continue;
        if((u16)(e->x-a->x+16)<=32 && (u16)(e->y-a->y+16)<=32) {
            a->state=0; a->defeated=8; e->life=0; return;
        }
    }
}

void items_step(void)
{
    YoshiItems *i=&yoshi.items;
    u16 n; YoshiPoint *last=i->trail+i->head;
    if(yoshi.vx || !yoshi.grounded || yoshi.action.mouth || yoshi.action.holding || i->aim || i->throwing)
        i->idle=0;
    else if(++i->idle==420) i->idle=0;
    items_collect(yoshi.x,yoshi.y+4,16,28);
    if((u16)(yoshi.x+4-last->x+3)>6 || (u16)(yoshi.y+16-last->y+3)>6) {
        i->head=(i->head+1)&63;
        i->trail[i->head].x=yoshi.x+4; i->trail[i->head].y=yoshi.y+16;
    }
    {
        YoshiPoint *f=i->followers;
        u8 remaining=yoshi.action.eggs,position=i->head-6;
        while(remaining--) {
            const YoshiPoint *target=i->trail+(u16)(position&63);
            f->x=target->x; f->y=target->y;
            ++f; position-=6;
        }
    }
    i->reserve=yoshi.action.eggs;
    if(!i->enabled) return;
    if(i->throwing) --i->throwing;
    if(yoshi.baby.recoil || yoshi.action.mouth || yoshi.action.holding || input_pressed(K_B) || input_held(K_DOWN)) i->aim=i->locked=0;
    else if(input_pressed(K_C) && yoshi.action.eggs) {
        if(i->aim) launch();
        else { i->aim=1; i->phase=0; i->reverse=i->locked=0; }
    } else if(i->aim && input_pressed(K_D)) {
        i->locked^=1; i->aim_facing=yoshi.action.facing;
    } else if(i->aim && !i->locked) {
        if(i->reverse) { if(!i->phase) i->reverse=0; else --i->phase; }
        else if(i->phase==20) i->reverse=1; else ++i->phase;
    }
    for(n=0;n<YI_EGGS;++n) if(i->shots[n].life) projectile(i->shots+n);
}

void items_render(void)
{
#ifdef YJ_ZONES
    BENCH_BEGIN(7);
#endif
    const YoshiItems *i=&yoshi.items;
    const u16 *p;
    u16 n,end,bin; s16 x,y;
    static const s8 bob[8]={0,-1,-1,0,0,1,1,0};
    RtSprite score;
    /* Draw coins on isolated art doors too; collection remains disabled. */
    bin=((yoshi.camx<<1)+RT_W*2)>>7; if(bin>9)bin=9;
    n=coin_start[yoshi.camx>=8?((yoshi.camx<<1)-16)>>7:0];
    end=coin_start[bin+1]; p=coin_xy[n];
    for(;n<end;++n,p+=2) {
        if(i->collected[n>>3]&(1<<(n&7))) continue;
        x=(p[0]>>1)-yoshi.camx; y=((p[1]-YT_TOP)>>1)-yoshi.camy;
        if(x>=-8 && x<RT_W && y>=-8 && y<RT_H) art_coin(x,y);
    }
    {
      const YoshiPoint *f=i->followers;
      u8 phase=(rt_frame>>2)&7,remaining=yoshi.action.eggs;
      while(remaining--) {
        x=(f->x>>1)-yoshi.camx;
        y=(((s16)f->y-YT_TOP)>>1)-yoshi.camy;
        if(x>=-16 && x<RT_W && y>=-16 && y<RT_H)
            art_egg(x,y+bob[phase]);
        ++f; phase=(phase+1)&7;
      }
    }
    for(n=0;n<YI_EGGS;++n) {
        const YoshiEgg *e=i->shots+n;
        if(!e->life) continue;
        x=(e->x>>1)-yoshi.camx; y=(((s16)e->y-YT_TOP)>>1)-yoshi.camy;
        if(x>=-16 && x<RT_W && y>=-16 && y<RT_H) art_egg(x,y);
    }
    if(i->enabled) {
        art_coin(2,2);
        score.w=16; score.h=9; score.light=score.dark=i->hud; score.mask=RT_NULL;
        draw_sprite(13,2,&score);
    }
#ifdef YJ_ZONES
    BENCH_END(7);
#endif
}

void items_aim_render(void)
{
    const YoshiItems *i=&yoshi.items;
    s16 x,y,dx;
    u8 a,facing;
    if(!i->aim) return;
    a=angle(); facing=i->locked?i->aim_facing:yoshi.action.facing;
    dx=egg_cursor[(a+16)&63]; if(facing) dx=-dx;
    x=(((s16)yoshi.x+(facing?8:24)+dx)>>1)-yoshi.camx;
    y=(((s16)yoshi.y+16+egg_cursor[a]-YT_TOP)>>1)-yoshi.camy;
    /* Overlay after the world, including actors/baby: no sprite hides aim. */
    art_aim(x-7,y-7,i->locked);
}

#if defined(YJ_ITEM_TRACE) || !defined(__m68k__)
void items_trace(void (*emit)(u32))
{
    const YoshiItems *i=&yoshi.items;
    u16 n;
    emit(((u32)i->idle<<16)|((u16)i->count<<8)|i->enabled);
    emit(((u32)i->collected[0]<<24)|((u32)i->collected[1]<<16)|((u16)i->collected[2]<<8)|i->head);
    emit(((u32)i->reserve<<24)|((u32)i->aim<<16)|((u16)i->phase<<8)|i->reverse);
    emit(((u32)i->aim_facing<<16)|((u16)i->locked<<8)|i->throwing);
    for(n=0;n<8;n+=2) emit(((u32)i->hud[n]<<16)|i->hud[n+1]);
    emit(i->hud[8]);
    for(n=0;n<YI_TRAIL;++n) emit(((u32)i->trail[n].x<<16)|i->trail[n].y);
    for(n=0;n<YI_EGGS;++n) emit(((u32)i->followers[n].x<<16)|i->followers[n].y);
    for(n=0;n<YI_EGGS;++n) {
        const YoshiEgg *e=i->shots+n;
        emit(((u32)e->x<<16)|e->y); emit(((u32)(u16)e->vx<<16)|(u16)e->vy);
        emit(((u32)e->xs<<24)|((u32)e->ys<<16)|((u16)e->life<<8)|e->bounces);
    }
}
#endif
