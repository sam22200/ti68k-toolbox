/* Independent flat heightfield DDA, informed by Descend's multi-step columns.
 * No FAT SDK code, texture data, DLL, generated scaler or heap execution. */
#include "engine.h"
static s16 horizon;
static void segment(u16 x,s16 top,s16 bottom,u16 depth,u8 shade) {
    s16 y;TeSegment *s;u8 *p,mask;
    if(top<0)top=0;
    if(bottom>=(s16)te_height)bottom=te_height-1;
    if(top>bottom)return;
    if(te_ncover[x]<TE_SEGMENTS) {
        s=&te_cover[x][te_ncover[x]++];s->top=top;s->bottom=bottom;s->depth=depth;
    } else te_metrics.segment_overflow++;
    p=te_fb+te_row[top]+(x>>3);mask=(u8)(128>>(x&7));
    *p|=mask;p+=20;
    for(y=top+1;y<=bottom;y++,p+=20)
        if(shade==1||((x+(u16)y)&1)==0)*p|=mask;
    te_metrics.walls++;
}
void te_ray(void) {
    u16 x;
    const TeCamera *c=&te_state.camera;
    s16 sn=te_sin[c->yaw],cs=te_sin[(u8)(c->yaw+64)];
    horizon=(te_height>>1)+c->pitch*(te_height==50?1:2);
    TE_BEGIN(3);
    for(x=0;x<te_width;x++) {
        s16 plane=(s16)(te_mul((s16)x-(s16)(te_width>>1),te_recip[te_focal])>>6);
        s16 dx=cs+(s16)(te_mul(sn,plane)>>8),dy=sn-(s16)(te_mul(cs,plane)>>8);
        s16 mx=c->x>>6,my=c->y>>6,sx=1,sy=1;
        u16 ax,ay,tx,ty,dt,idx,step;
        s16 clip=te_height-1,prev=te_state.feet_z;

        if(dx<0){sx=-1;dx=-dx;}if(dy<0){sy=-1;dy=-dy;}
        ax=dx?te_recip[dx]:32767;ay=dy?te_recip[dy]:32767;
        tx=dx?(u16)(te_mul(sx>0?64-(c->x&63):c->x&63,ax)>>6):32767;
        ty=dy?(u16)(te_mul(sy>0?64-(c->y&63):c->y&63,ay)>>6):32767;
        for(step=0;step<32&&clip>=0;step++) {
            s16 h,top,bot;u8 side;
            if(tx<ty){dt=tx;tx+=ax;mx+=sx;side=1;}else{dt=ty;ty+=ay;my+=sy;side=2;}
            if((u16)mx>=16||(u16)my>=16||dt>=2048)break;
            idx=((u16)my<<4)+(u16)mx;h=te_world.floor[idx]*8;
            if(te_world.door[idx]&&te_state.door_open)h=0;
            if(h>prev) {
                top=horizon-te_project_y(h-c->z,dt);
                bot=horizon-te_project_y(prev-c->z,dt);
                if(bot>clip)bot=clip;
                segment(x,top,bot,dt,side);
                if(top<=clip)clip=top-1;
            }
            /* Far-to-near horizontal terrace footprint approximated with DDA
             * entry/exit projection. No overlapping vertical layers. */
            if(h&&h<c->z&&clip>=0) {
                u16 exit=tx<ty?tx:ty;
                top=horizon-te_project_y(h-c->z,exit);
                bot=horizon-te_project_y(h-c->z,dt);
                if(bot>clip)bot=clip;
                segment(x,top,bot,dt,2);
    if(top<=clip)clip=top-1;
            }
            prev=h;
        }
    }
    TE_END(3);
}
