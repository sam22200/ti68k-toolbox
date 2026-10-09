/* Offline-scaled bitmap view. Full-screen word copy follows the measured
 * Alundra C blitter; canopy restoration touches only the actor rectangle. */
#include "minish.h"
#include "zoom_generated.h"
static const u8 *scene_light, *scene_dark, *scene_mask;
static const u32 *actors;
static const u16 *scale_xy;

u16 minish_scaled(u16 x) { return scale_xy ? scale_xy[x] : 0; }

u8 minish_zoom_init(void)
{
    const u8 *bank;
    u16 size;
    scene_light=scene_dark=scene_mask=RT_NULL;actors=RT_NULL;scale_xy=RT_NULL;
    bank=rt_file("mizscene",&size);
    if (!bank || size<ZOOM_SCENE_SIZE || size>ZOOM_SCENE_SIZE+6) return 0;
    scene_light=bank;scene_dark=bank+ZOOM_PLANE;scene_mask=bank+2*ZOOM_PLANE;
    bank=rt_file("mizactor",&size);
    if (!bank || size<ZOOM_ACTOR_SIZE || size>ZOOM_ACTOR_SIZE+6) return 0;
    actors=(const u32 *)bank;scale_xy=(const u16 *)(bank+ZOOM_SCALE_OFFSET);
    return 1;
}

#ifdef __m68k__
/* One long read per destination word: its high word is the source word
 * under it, its low word the next one; one shift leaves the result. */
#define W10(op) op(0) op(2) op(4) op(6) op(8) op(10) op(12) op(14) op(16) op(18)
#define BLIT_COPY(k) ((u16 *)dst)[(k)/2]=((const u16 *)row)[(k)/2];
#define BLIT_LEFT(k) ((u16 *)dst)[(k)/2]=(u16)((*(const u32 *)(row+(k))<<sh)>>16);
#define BLIT_RIGHT(k) ((u16 *)dst)[(k)/2]=(u16)(*(const u32 *)(row+(k))>>rs);
static void blit_plane(u8 *dst,const u8 *src,u16 cx,u16 cy)
{
    const u8 *row=src+(cy<<6)+((cx>>4)<<1);
    u16 sh=cx&15,rs=16-sh,r;
    if (!sh) for (r=0;r<RT_H;r++,row+=64,dst+=RT_PBYTES) { W10(BLIT_COPY) }
    else if (sh<=8) for (r=0;r<RT_H;r++,row+=64,dst+=RT_PBYTES) { W10(BLIT_LEFT) }
    else for (r=0;r<RT_H;r++,row+=64,dst+=RT_PBYTES) { W10(BLIT_RIGHT) }
}
#else
static void blit_plane(u8 *dst,const u8 *src,u16 cx,u16 cy)
{
    const u8 *row=src+(cy<<6)+((cx>>4)<<1);
    u16 sh=cx&15,r,k;
    for (r=0;r<RT_H;r++,row+=64,dst+=RT_PBYTES) {
        const u8 *s=row;
        u8 *d=dst;
        u32 acc=(u16)(s[0]<<8|s[1]);
        for (k=0;k<10;k++,d+=2) {
            s+=2;acc=(acc<<16)|(u16)(s[0]<<8|s[1]);
            d[0]=(u8)(acc>>(24-sh));d[1]=(u8)(acc>>(16-sh));
        }
    }
}
#endif

#ifdef __m68k__
void minish_canopy(s16 x,s16 y,u16 w,u16 h)
{
    s16 left=x<0 ? 0 : x,right=x+(s16)w;
    s16 top=y<0 ? 0 : y,bottom=y+(s16)h;
    u16 aligned,count,col,row,shift=(u16)st.camx&15,rs=16-shift;
    u16 source_x,offset,dst,end;
    u32 clips[5];
    const u8 *sl,*sd,*sm;u8 *dl,*dd;
    if (right>RT_W) right=RT_W;
    if (bottom>RT_H) bottom=RT_H;
    if (left>=right || top>=bottom) return;
    aligned=(u16)left&~15;count=((u16)right-aligned+31)>>5;
    for (col=0;col<count;col++) clips[col]=0xffffffffUL;
    clips[0]>>=(u16)left&15;
    end=((u16)right-aligned)&31;
    if (end) clips[count-1]&=(u32)(0xffffffffUL<<(32-end));
    source_x=((u16)st.camx&~15)+aligned;
    offset=(((u16)top+(u16)st.camy)<<6)+(source_x>>3);
    dst=((u16)top<<5)-((u16)top<<1)+(aligned>>3);
    sl=scene_light+offset;sd=scene_dark+offset;sm=scene_mask+offset;
    dl=(u8 *)rt_light+dst;dd=(u8 *)rt_dark+dst;
    for (row=top;row<(u16)bottom;row++,sl+=64,sd+=64,sm+=64,dl+=RT_PBYTES,dd+=RT_PBYTES) {
        const u8 *l=sl,*d=sd,*m=sm;u8 *out_l=dl,*out_d=dd;
        u16 sx=source_x;
        for (col=0;col<count;col++,sx+=32,l+=4,d+=4,m+=4,out_l+=4,out_d+=4) {
            u32 mask=*(const u32 *)m,light,dark;
            /* The final source word has no following word inside its row.
               Its clipped tail is transparent; do not read beyond a plane. */
            if (shift) {
                mask<<=shift;
                if (sx<480) mask|=(u32)((const u16 *)m)[2]>>rs;
            }
            mask&=clips[col];if (!mask) continue;
            light=*(const u32 *)l;dark=*(const u32 *)d;
            if (shift) {
                light<<=shift;dark<<=shift;
                if (sx<480) {light|=(u32)((const u16 *)l)[2]>>rs;dark|=(u32)((const u16 *)d)[2]>>rs;}
            }
            *(u32 *)out_l=(*(u32 *)out_l&~mask)|(light&mask);
            *(u32 *)out_d=(*(u32 *)out_d&~mask)|(dark&mask);
        }
    }
}
#else
void minish_canopy(s16 x,s16 y,u16 w,u16 h)
{
    static const u8 right_masks[8]={255,128,192,224,240,248,252,254};
    s16 left=x<0 ? 0 : x,right=x+(s16)w;
    s16 top=y<0 ? 0 : y,bottom=y+(s16)h;
    u16 row,b,shift=(u16)st.camx&7;
    u16 first,last,count,offset,dst;
    u8 clips[RT_W/8];
    u8 *dl,*dd;
    const u8 *sl,*sd,*sm;
    if (right>RT_W) right=RT_W;
    if (bottom>RT_H) bottom=RT_H;
    if (left>=right || top>=bottom) return;
    first=(u16)left>>3;last=((u16)right-1)>>3;
    count=last-first+1;
    for (b=0;b<count;b++) clips[b]=255;
    clips[0]&=(u8)(255>>((u16)left&7));clips[count-1]&=right_masks[(u16)right&7];
    offset=(((u16)top+(u16)st.camy)<<6)+((u16)st.camx>>3)+first;
    dst=((u16)top<<5)-((u16)top<<1)+first;
    dl=(u8 *)rt_light+dst;dd=(u8 *)rt_dark+dst;
    sl=scene_light+offset;sd=scene_dark+offset;sm=scene_mask+offset;
    if (!shift) {
        for (row=top;row<(u16)bottom;row++,sl+=64,sd+=64,sm+=64,dl+=RT_PBYTES,dd+=RT_PBYTES)
            for (b=0;b<count;b++) {
                u8 bits=sm[b]&clips[b];
                if (!bits) continue;
                dl[b]=(dl[b]&~bits)|(sl[b]&bits);dd[b]=(dd[b]&~bits)|(sd[b]&bits);
            }
    } else {
        u16 right_shift=8-shift;
        for (row=top;row<(u16)bottom;row++,sl+=64,sd+=64,sm+=64,dl+=RT_PBYTES,dd+=RT_PBYTES)
        for (b=0;b<count;b++) {
            u16 m=((u16)sm[b]<<8)|sm[b+1];
            u8 bits=(u8)(m>>right_shift)&clips[b];
            if (bits) {
                u16 l=((u16)sl[b]<<8)|sl[b+1],d=((u16)sd[b]<<8)|sd[b+1];
                dl[b]=(dl[b]&~bits)|((u8)(l>>right_shift)&bits);
                dd[b]=(dd[b]&~bits)|((u8)(d>>right_shift)&bits);
            }
        }
    }
}
#endif

void minish_zoom_render(void)
{
    s16 x=minish_scaled(st.x>>8)-st.camx-16;
    s16 y=minish_scaled(st.y>>8)-st.camy-24;
    const u32 *p;
    u16 w=32,h=32;
    RtSprite actor;
    blit_plane(rt_light,scene_light,st.camx,st.camy);
    blit_plane(rt_dark,scene_dark,st.camx,st.camy);
    minish_draw_cuts();
    minish_draw_effects(0);
    minish_draw_enemies(0);
    if (st.encounters && st.iframes && (st.ticks&2)) {w=h=0;}
    else if (st.display_pose>=84) {minish_draw_fx(st.display_pose-84,st.x>>8,st.y>>8,st.display_cover);w=0;}
    else if (st.display_pose>=44) minish_draw_sword(&x,&y,&w,&h);
    else {
    p=actors+((u16)(st.display_pose<<6)+(u16)(st.display_pose<<5));
    actor.w=actor.h=32;actor.light=p;actor.dark=p+32;actor.mask=p+64;
    draw_sprite(x,y,&actor);
    { const u8 *b=zoom_actor_bounds[st.display_pose];
      x+=b[0];y+=b[1];w=b[2];h=b[3]; }
    }
    if (st.display_cover && w) minish_canopy(x,y,w,h);
    minish_draw_enemies(1);
    minish_draw_effects(1);
}
