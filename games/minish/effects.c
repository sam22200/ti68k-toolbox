/* Original roll timing/art and bounded native destruction presentations. */
#include "minish.h"
#include "generated.h"
#include "effects_generated.h"
static const u32 *art;

u8 minish_effects_init(void)
{
    u16 size;
#ifdef MINISH_ZOOM
    art=(const u32 *)rt_file("mizfx",&size);
#else
    art=(const u32 *)rt_file("mifx",&size);
#endif
    return art && size>=FX_SIZE && size<=FX_SIZE+6;
}

void minish_effects_reset(void)
{
    u16 i;
    st.roll=st.last_b=st.roll_guard=0;
    for (i=0;i<4;i++) {
        MinishFx *f=&st.effects[i];
        f->x=f->y=f->age=f->kind=0;f->display_pose=255;
    }
}

void minish_effects_step(void)
{
    u16 i;
    for (i=0;i<4;i++) {
        MinishFx *f=&st.effects[i];
        const u8 *timeline=f->kind ? grass_fx : bush_fx;
        u16 length=f->kind ? GRASS_FX_LEN : BUSH_FX_LEN;
        if (!f->age) continue;
        if (f->age<=length) f->display_pose=timeline[f->age++-1];
        else {f->age=0;f->display_pose=255;}
    }
}

void minish_fx_spawn(u16 x,u16 y,u8 kind)
{
    u16 i,slot=0;
    for (i=0;i<4;i++) {
        if (!st.effects[i].age) {slot=i;break;}
        if (st.effects[i].age>st.effects[slot].age) slot=i;
    }
    st.effects[slot].x=x;st.effects[slot].y=y;
    st.effects[slot].kind=kind;st.effects[slot].age=1;st.effects[slot].display_pose=255;
}

/* 0=ordinary movement, 1=start lock, 2=roll, 3=final recovery movement. */
u8 minish_roll_step(u16 keys)
{
    u16 face=st.anim_face;
    u8 b=(keys&K_B)!=0,pressed=b&&!st.last_b;
    st.last_b=b;
    if (!st.roll) {
        s16 dx,dy;u16 d;
        if (!pressed || st.attack || (keys&K_A) || !(keys&15)) return 0;
        dx=((keys&K_RIGHT)!=0)-((keys&K_LEFT)!=0);
        dy=((keys&K_DOWN)!=0)-((keys&K_UP)!=0);
        d=dy<0 ? (dx<0 ? 7 : dx>0 ? 1 : 0) : dy>0 ? (dx<0 ? 5 : dx>0 ? 3 : 4) : dx<0 ? 6 : dx>0 ? 2 : 255;
        if (d==255) return 0;
        if (!(d&1) || ((d+1-(face<<1))&4)) face=(d&6)>>1;
        st.roll=1;st.anim_face=face;st.facing=face<<3;
        st.anim_walk=st.anim_phase=0;st.anim_timer=3;
    } else ++st.roll;
    st.last_a=(keys&K_A)!=0;
    st.display_pose=st.pose;st.display_cover=st.cover;
    if (st.roll>ROLL_LEN) {
        st.roll=st.roll_guard=0;st.pose=idle_pose[face];st.moving=0;
        return 3;
    }
    st.roll_guard=roll_guard[st.roll-1];
    st.pose=84+roll_pose[face][st.roll-1];
    st.direction=face<<3;
    return st.roll==1 ? 1 : 2;
}

u16 minish_roll_speed(void) {return st.roll ? roll_speed[st.roll-1] : roll_speed[ROLL_LEN-1]+32;}
u8 minish_death_length(void) {return DEATH_FX_LEN;}
u8 minish_fx_pose_count(void) {return FX_POSES;}
u8 minish_fx_preview_pose(u8 group)
{
    return group==0 ? bush_fx[0] : group==1 ? death_fx[0] : group==2 ? death_fx[34] : roll_pose[1][8];
}

void minish_draw_fx(u8 pose,u16 wx,u16 wy,u8 cover)
{
    const s16 *m=fx_art[pose];
    const u32 *p=art+m[0];
    s16 x,y;u16 part,h=m[3];RtSprite sprite;
    if (!h) return;
#ifdef MINISH_ZOOM
    x=minish_scaled(wx)-st.camx+m[1];y=minish_scaled(wy)-st.camy+m[2];
#else
    x=wx-st.camx+m[1];y=wy-st.camy+m[2];
#endif
    if (x>=RT_W || x+m[5]<=0 || y>=RT_H || y+(s16)h<=0) return;
#ifdef __m68k__
    {s16 aligned=x&~15;
     if (!minish_flash && fx_shift[pose]!=65535 && x>=0 && x+m[5]<=RT_W && y>=0 && y+(s16)h<=RT_H) {
         const u32 *q=(const u32 *)((const u8 *)art+fx_shift[pose]);
         u16 r,offset=((u16)y<<5)-((u16)y<<1)+((u16)aligned>>3);
         u8 *l=(u8 *)rt_light+offset,*d=(u8 *)rt_dark+offset;
         u16 stride=fx_span[pose]==1 ? 3 : 6;
         q+=((u16)x&15)*h*stride;
         if (stride==3) for (r=0;r<h;r++,q+=3,l+=RT_PBYTES,d+=RT_PBYTES) {
             *(u32 *)l=(*(u32 *)l&q[2])|q[0];*(u32 *)d=(*(u32 *)d&q[2])|q[1];
         } else for (r=0;r<h;r++,q+=6,l+=RT_PBYTES,d+=RT_PBYTES) {
             *(u32 *)l=(*(u32 *)l&q[2])|q[0];*(u32 *)d=(*(u32 *)d&q[2])|q[1];
             *(u32 *)(l+4)=(*(u32 *)(l+4)&q[5])|q[3];*(u32 *)(d+4)=(*(u32 *)(d+4)&q[5])|q[4];
         }
     } else {
#endif
        sprite.w=32;sprite.h=h;
        for (part=0;part<m[4];part++,p+=h*3) {
            sprite.light=p;sprite.dark=p+h;sprite.mask=p+(h<<1);
            minish_link_sprite(x+(part<<5),y,&sprite);
        }
#ifdef __m68k__
     }}
#endif
    if (cover) minish_canopy(x,y,m[5],h);
}

void minish_draw_effects(u8 front)
{
    u16 i;
    for (i=0;i<4;i++) {
        const MinishFx *f=&st.effects[i];
        if (f->display_pose!=255 && ((f->y>=(st.y>>8))==front))
            minish_draw_fx(f->display_pose,f->x,f->y,1);
    }
}

void minish_draw_death(const MinishEnemy *e)
{
    minish_draw_fx(death_fx[DEATH_FX_LEN-e->fade],e->x>>8,e->y>>8,1);
}
