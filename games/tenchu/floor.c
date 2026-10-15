/* Phase 4: software plane mapping. Inverse pitched-camera projection with one
 * reciprocal lookup per row, affine world increments ONLY within that row.
 * Horizontally flat elevated faces use their own Z; sloped roofs stay flat shaded.
 * Coordinates are Q8, denominator Q2. No float, divide or long multiply in loop. */
#include "engine.h"
void te_floor_span(s16 y,s16 x0,s16 x1,s16 z) {
    const TeCamera *c=&te_state.camera;
    s16 cs=te_sin[(u8)(c->yaw+64)],sn=te_sin[c->yaw];
    s16 cp=te_sin[(u8)(c->pitch+64)],sp=te_sin[(u8)c->pitch];
    s16 row=(te_height>>1)-y,den,forward,side,rx,ry,t,dx,dy,dz=z-c->z,f=te_focal;
    s32 wx,wy,factor;
    u16 x,step=te_state.style==1?2:1;
    u8 *pixel,mask;
    if(te_width==80&&te_height==100)f+=f;
    den=(s16)((te_mul(row,cp)+te_mul(f,sp))>>6);
    forward=(s16)((te_mul(f,cp)-te_mul(row,sp))>>6);
    if(!den||(den<0)!=(dz<0))return;
    if(den<0){den=-den;dz=-dz;}
    factor=te_mul(dz,te_recip[den])>>4;
    if(factor<=0||factor>8192)return;
    t=(s16)factor; /* Q8 world units per pixel */
    if(x0<0)x0=0;
    if(x1>=(s16)te_width)x1=te_width-1;
    if(te_clip_left) {
        if(x0<te_clip_left[y])x0=te_clip_left[y];
        if(x1>te_clip_right[y])x1=te_clip_right[y];
    }
    if(x0>x1)return;
    side=(x0-(s16)(te_width>>1))*4;
    if(te_width==80&&te_height==100)side+=side;
    rx=(s16)((te_mul(forward,cs)+te_mul(side,sn))>>8);
    ry=(s16)((te_mul(forward,sn)-te_mul(side,cs))>>8);
    wx=te_mul(c->x,256)+(te_mul(rx,t)>>2);
    wy=te_mul(c->y,256)+(te_mul(ry,t)>>2);
    dx=(s16)(te_mul(sn,t)>>8);dy=(s16)(te_mul(-cs,t)>>8);
    if(te_width==80&&te_height==100){dx+=dx;dy+=dy;}
    if(step==2){dx+=dx;dy+=dy;}
    pixel=te_fb+te_row[y]+((u16)x0>>3);mask=(u8)(128>>((u16)x0&7));
    for(x=x0;x<=(u16)x1;x+=step,wx+=dx,wy+=dy) {
        /* Sparse 32-unit checker; world-aligned at all resolutions/elevations. */
        u8 black=((wx^wy)&8192L)!=0;
        if(black)*pixel|=mask;else *pixel&=(u8)~mask;
        mask>>=1;if(!mask){mask=128;pixel++;}
        if(step==2&&x<(u16)x1) {
            if(black)*pixel|=mask;else *pixel&=(u8)~mask;
            mask>>=1;if(!mask){mask=128;pixel++;}
        }
    }
}
void te_floor(void) {
    s16 y;
    TE_BEGIN(11);
    for(y=0;y<(s16)te_height;y++)te_floor_span(y,0,te_width-1,0);
    TE_END(11);
}
