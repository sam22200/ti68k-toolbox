#include "art.h"
#include "yoshi.h"
#include "terrain.h"
#include "generated/art_ids.h"
#include "generated/screen_rows.h"
#include <string.h>
#include <stdlib.h>
#ifdef YJ_ZONES
#include "../../tools/m68kbench/bench.h"
#endif

/* Exactly 32 bytes on the TI ABI: frame lookup is one constant shift. */
typedef struct { RtSprite part[2]; s8 origin[2][2]; } ArtFrame;
#ifdef __m68k__
static ArtFrame *frames;
static void __attribute__((__stkparm__)) release_frames(void)
{
    if(frames) free(frames);
    frames=RT_NULL;
}
#else
static ArtFrame frames[YART_COUNT];
#endif
static const u32 *small[5];
static const u32 *cursors[2];
static RtSprite cursor_sprite[2];
static u8 ready;
#ifndef __m68k__
u8 art_reference;
#endif
static u16 be16(const u8 *p) { return ((u16)p[0] << 8) | p[1]; }

u8 art_ready(void) { return ready; }
u8 art_init(void)
{
    const u8 *p, *r;
    u16 size, n, end, part;
    ready = 0;
#ifndef __m68k__
    art_reference = 0;
#endif
#ifdef __m68k__
    frames=RT_NULL; /* The TI program's globals survive between invocations. */
#endif
    p = rt_file("yjart", &size);
    if (!p || size < 16 || memcmp(p,"YAR4",4) || be16(p+4) != YART_COUNT) return 0;
    end = be16(p+6);
    if (end > size || end < 16 + YART_COUNT * 16) return 0;
    { u16 pos=be16(p+8);
      if((pos&3) || pos<16+YART_COUNT*16 || (u32)pos+15360!=be16(p+10) || (u32)pos+21504!=end) return 0;
      for(n=0;n<5;++n,pos+=3072) small[n]=(const u32 *)(p+pos);
      for(n=0;n<2;++n,pos+=3072) {
          RtSprite *s=cursor_sprite+n;
          cursors[n]=(const u32 *)(p+pos);
          s->w=32; s->h=15; s->light=cursors[n];s->dark=cursors[n]+16;s->mask=cursors[n]+32;
      }
    }
#ifdef __m68k__
    frames=malloc(sizeof(ArtFrame)*YART_COUNT);
    if(!frames) return 0;
    if(atexit(release_frames)) { release_frames(); return 0; }
#endif
    memset(frames,0,sizeof(ArtFrame)*YART_COUNT);
    for (n = 0, r = p+16; n < YART_COUNT; ++n) {
      for (part = 0; part < 2; ++part, r += 8) {
        u16 pos = be16(r+4), length = be16(r+6);
        RtSprite *s = &frames[n].part[part];
        if (!r[0] && part) continue;
        if ((r[0] != 8 && r[0] != 16 && r[0] != 32) || !r[1] || r[1] > 32 ||
            length != (u16)((r[0] >> 3) * r[1]) || (pos & 3) ||
            pos < 16 + YART_COUNT * 16 || (u32)pos + (u16)(length * 3) > (u32)end) return 0;
        s->w = r[0]; s->h = r[1];
        s->light = p+pos; s->dark = p+pos+length; s->mask = p+pos+(length<<1);
        frames[n].origin[part][0] = (s8)r[2]; frames[n].origin[part][1] = (s8)r[3];
      }
    }
    return ready = 1;
}

#ifndef __m68k__
/* Independent pixel oracle for ExtGraph masked blits and transparent padding. */
static u32 row(const void *p, u8 w, u16 y)
{
    if (w == 8) return ((const u8 *)p)[y];
    if (w == 16) return ((const u16 *)p)[y];
    return ((const u32 *)p)[y];
}
static void pixel_sprite(s16 x, s16 y, const RtSprite *s)
{
    s16 r,c;
    for (r=0;r<s->h;++r) {
        s16 dy=y+r;
        u32 mask=row(s->mask,s->w,r), l=row(s->light,s->w,r), d=row(s->dark,s->w,r);
        if (dy<0 || dy>=RT_H) continue;
        for(c=0;c<s->w;++c) {
            s16 dx=x+c;
            u32 bit=1UL<<(s->w-1-c);
            u16 pos;
            u8 out;
            if(dx<0 || dx>=RT_W || (mask&bit)) continue;
            pos=dy*RT_PBYTES+(dx>>3); out=128>>(dx&7);
            ((u8 *)rt_light)[pos] = (((u8 *)rt_light)[pos]&~out) | ((l&bit)?out:0);
            ((u8 *)rt_dark)[pos] = (((u8 *)rt_dark)[pos]&~out) | ((d&bit)?out:0);
        }
    }
}
#endif

static void blit(u16 id, s16 x, s16 y)
{
    const ArtFrame *f;
    if (!ready) return;
    f=frames+id;
    /* Callers cull actors and ExtGraph clips each block. Unroll the two
       optional parts; repeating bounding checks costs more than small blits. */
#ifndef __m68k__
    if(art_reference) {
        pixel_sprite(x+f->origin[0][0],y+f->origin[0][1],f->part);
        if(f->part[1].w) pixel_sprite(x+f->origin[1][0],y+f->origin[1][1],f->part+1);
        return;
    }
#endif
    draw_sprite(x+f->origin[0][0],y+f->origin[0][1],f->part);
    if(f->part[1].w) draw_sprite(x+f->origin[1][0],y+f->origin[1][1],f->part+1);
}

void art_hero(void)
{
    const YoshiInteraction *i=&yoshi.action;
    u16 id=YART_IDLE, count=YART_IDLE_N, frame=0;
    s16 x=(yoshi.x>>1)-yoshi.camx;
    s16 y=(((s16)yoshi.y-YT_TOP)>>1)-yoshi.camy;
    if (yoshi.baby.enabled && yoshi.baby.invincible && (rt_frame & 4)) return;
    if(i->mouth>=47) { id=YART_SWALLOW;count=YART_SWALLOW_N;frame=(i->mouth-47)>>2; }
    else if(i->mouth || i->length) { id=i->up?YART_UP:YART_TONGUE;count=1; }
    else if(i->holding) { id=YART_HOLDING;count=YART_HOLDING_N; }
    else if(yoshi.items.aim || yoshi.items.throwing) { id=YART_THROW;count=YART_THROW_N;frame=yoshi.items.aim?1:yoshi.items.throwing>5?2:3; }
    else if(!yoshi.grounded && yoshi.flutter>=2) { id=YART_FLUTTER;count=4;frame=(rt_frame>>2)&3; }
    else if(!yoshi.grounded) { id=YART_JUMP;count=4;frame=yoshi.vy < -384?0:yoshi.vy<0?1:yoshi.vy<640?2:3; }
    else if(yoshi.vx) { id=YART_WALK;count=8;frame=(rt_frame>>2)&7; }
    else { u16 t=yoshi.items.idle; frame=t<98?2:t<138?0:t<141?1:t<311?0:3; }
    if (yoshi.baby.mode >= YB_LOST) {
        switch (id) {
            case YART_IDLE: id=YART_SOLO_IDLE; break;
            case YART_WALK: id=YART_SOLO_WALK; break;
            case YART_JUMP: id=YART_SOLO_JUMP; break;
            case YART_FLUTTER: id=YART_SOLO_FLUTTER; break;
            case YART_TONGUE: id=YART_SOLO_TONGUE; break;
            case YART_UP: id=YART_SOLO_UP; break;
            case YART_HOLDING: id=YART_SOLO_HOLDING; break;
            case YART_SWALLOW: id=YART_SOLO_SWALLOW; break;
            case YART_THROW: id=YART_SOLO_THROW; break;
        }
    }
    if(i->facing) id+=count;
    blit(id+frame,x,y);
}

void art_shy(s16 x, s16 y, u8 facing, u8 moving)
{
    u16 id=YART_SHY;
    if(facing) id+=YART_SHY_N;
    if(moving) id+=(rt_frame>>3)&3;
    blit(id,x,y);
}
static void small_blit(u16 id,u16 phase,s16 x,s16 y)
{
#ifdef __m68k__
    const ArtFrame *f=frames+id;
    s16 sx=x+f->origin[0][0],sy=y+f->origin[0][1];
    if(ready && sx>=0 && sx<RT_W && sy>=0 && sy<=RT_H-10) {
        const u32 *s=small[phase]+(((u16)sx&15)<<5)+(((u16)sx&15)<<4);
        u16 offset=screen_rows[sy]+(((u16)sx>>4)<<1);
        u8 *l=(u8 *)rt_light+offset,*d=(u8 *)rt_dark+offset;
        /* Ten constant rows; all shifts and mask dilation are offline.
           Transparent padding preserves scenery and neighboring sprites. */
#define SMALL_ROW do { u32 m=s[2]; *(u32 *)l=(*(u32 *)l&m)|s[0]; *(u32 *)d=(*(u32 *)d&m)|s[1]; s+=3; l+=30; d+=30; } while(0);
        SMALL_ROW SMALL_ROW SMALL_ROW SMALL_ROW SMALL_ROW
        SMALL_ROW SMALL_ROW SMALL_ROW SMALL_ROW SMALL_ROW
#undef SMALL_ROW
        return;
    }
#endif
    blit(id,x,y);
}
void art_egg(s16 x,s16 y) { small_blit(YART_EGG,0,x,y); }
void art_coin(s16 x,s16 y) { u16 frame=(rt_frame>>3)&3; small_blit(YART_COIN+frame,frame+1,x,y); }

void art_aim(s16 x,s16 y,u8 locked)
{
    if(!ready) return;
#ifdef __m68k__
    if(x>=0 && x<=RT_W-15 && y>=0 && y<=RT_H-15) {
        u16 phase=x&15;
        const u32 *s=cursors[locked]+(phase<<5)+(phase<<4);
        u16 offset=screen_rows[y]+(((u16)x>>4)<<1);
        u8 *l=(u8 *)rt_light+offset,*d=(u8 *)rt_dark+offset;
        /* Cursor ink is black on both planes; reuse each loaded row. */
#define AIM_ROW do { u32 mask=s[32],ink=s[0]; *(u32 *)l=(*(u32 *)l&mask)|ink; *(u32 *)d=(*(u32 *)d&mask)|ink; ++s; l+=30; d+=30; } while(0);
        AIM_ROW AIM_ROW AIM_ROW AIM_ROW AIM_ROW
        AIM_ROW AIM_ROW AIM_ROW AIM_ROW AIM_ROW
        AIM_ROW AIM_ROW AIM_ROW AIM_ROW AIM_ROW
#undef AIM_ROW
        return;
    }
#else
    if(art_reference) { pixel_sprite(x,y,cursor_sprite+locked); return; }
#endif
    draw_sprite(x,y,cursor_sprite+locked);
}

void art_hit(s16 x,s16 y)
{
    static const u8 pixels[8]={0,0x18,0x3c,0x7e,0x7e,0x3c,0x18,0};
    static const u8 mask[8]={0xc3,0x81,0,0,0,0,0x81,0xc3};
    RtSprite sprite;
    sprite.w=8; sprite.h=8; sprite.light=sprite.dark=pixels;sprite.mask=mask;
    draw_sprite(x,y,&sprite);
}

void art_baby(s16 x,s16 y,u8 bubble)
{
#ifdef YJ_ZONES
    BENCH_BEGIN(5);
#endif
    blit((bubble ? YART_BUBBLE : YART_BABY) + ((rt_frame >> 2) & 3),x,y);
    /* White-edged tears stay legible after half-scale conversion. */
    if (bubble && (rt_frame & 8)) {
        static const u32 pixels[4]={0,0x40010000UL,0x40010000UL,0};
        static const u32 mask[4]={0x1FFC7FFFUL,0x1FFC7FFFUL,0x1FFC7FFFUL,0x1FFC7FFFUL};
        RtSprite tears;
        tears.w=32; tears.h=4; tears.light=tears.dark=pixels; tears.mask=mask;
        draw_sprite(x-4,y+3,&tears);
    }
#ifdef YJ_ZONES
    BENCH_END(5);
#endif
}

void art_counter(u16 seconds,u8 alert)
{
#ifdef YJ_ZONES
    BENCH_BEGIN(6);
#endif
    /* Whole white panel and digits baked together: one native 16px blit.
       Eleven values, with/without alert borders; no per-frame glyph build. */
    static const u16 panels[22][16] = {
        {0x0,0x0,0x0,0x70,0x88,0x98,0xa8,0xc8,0x88,0x70,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x20,0x60,0x20,0x20,0x20,0x20,0x70,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x70,0x88,0x8,0x10,0x20,0x40,0xf8,0x0,0x0,0x0},
        {0x0,0x0,0x0,0xf0,0x8,0x8,0x70,0x8,0x8,0xf0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x10,0x30,0x50,0x90,0xf8,0x10,0x10,0x0,0x0,0x0},
        {0x0,0x0,0x0,0xf8,0x80,0x80,0xf0,0x8,0x8,0xf0,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x70,0x80,0x80,0xf0,0x88,0x88,0x70,0x0,0x0,0x0},
        {0x0,0x0,0x0,0xf8,0x8,0x10,0x20,0x40,0x40,0x40,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x70,0x88,0x88,0x70,0x88,0x88,0x70,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x70,0x88,0x88,0x78,0x8,0x8,0x70,0x0,0x0,0x0},
        {0x0,0x0,0x0,0x870,0x1888,0x898,0x8a8,0x8c8,0x888,0x1c70,0x0,0x0,0x0},
        {0xffff,0x0,0x0,0x70,0x88,0x98,0xa8,0xc8,0x88,0x70,0x0,0x0,0xffff},
        {0xffff,0x0,0x0,0x20,0x60,0x20,0x20,0x20,0x20,0x70,0x0,0x0,0xffff},
        {0xffff,0x0,0x0,0x70,0x88,0x8,0x10,0x20,0x40,0xf8,0x0,0x0,0xffff},
        {0xffff,0x0,0x0,0xf0,0x8,0x8,0x70,0x8,0x8,0xf0,0x0,0x0,0xffff},
        {0xffff,0x0,0x0,0x10,0x30,0x50,0x90,0xf8,0x10,0x10,0x0,0x0,0xffff},
        {0xffff,0x0,0x0,0xf8,0x80,0x80,0xf0,0x8,0x8,0xf0,0x0,0x0,0xffff},
        {0xffff,0x0,0x0,0x70,0x80,0x80,0xf0,0x88,0x88,0x70,0x0,0x0,0xffff},
        {0xffff,0x0,0x0,0xf8,0x8,0x10,0x20,0x40,0x40,0x40,0x0,0x0,0xffff},
        {0xffff,0x0,0x0,0x70,0x88,0x88,0x70,0x88,0x88,0x70,0x0,0x0,0xffff},
        {0xffff,0x0,0x0,0x70,0x88,0x88,0x78,0x8,0x8,0x70,0x0,0x0,0xffff},
        {0xffff,0x0,0x0,0x870,0x1888,0x898,0x8a8,0x8c8,0x888,0x1c70,0x0,0x0,0xffff}
    };
    RtSprite sprite;
    const u16 *pixels=panels[seconds+(alert?11:0)];
    sprite.w=16; sprite.h=13; sprite.light=sprite.dark=pixels; sprite.mask=RT_NULL;
    draw_sprite(142,2,&sprite);
#ifdef YJ_ZONES
    BENCH_END(6);
#endif
}
