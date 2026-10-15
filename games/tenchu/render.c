#include "engine.h"
#include <string.h>
#include <stdlib.h>
typedef struct {
    u8 fb[2000],background[2000];TeSegment cover[160][TE_SEGMENTS];u8 ncover[160];
    TeState key;u16 revision;u8 valid,pad;
} RenderWork;
static RenderWork *buffers;
u8 *te_fb;
TeMetrics te_metrics;
u16 te_width,te_height,te_focal;
TeSegment (*te_cover)[TE_SEGMENTS];
u8 *te_ncover;
const s16 *te_clip_left,*te_clip_right;
static const u8 bits[8]={128,64,32,16,8,4,2,1};
u8 te_render_init(void) {
    if(!buffers){buffers=malloc(sizeof(*buffers));if(buffers)buffers->valid=0;}
    if(!buffers)return 0;
    te_fb=buffers->fb;te_cover=buffers->cover;te_ncover=buffers->ncover;
    return 1;
}
void te_render_finish(void) {
    if(buffers){free(buffers);buffers=RT_NULL;}
    te_fb=RT_NULL;te_cover=RT_NULL;te_ncover=RT_NULL;
}
u16 te_render_bytes(void) {return sizeof(*buffers);}
static void setup(void) {
    te_width=te_state.resolution==0?160:80;
    te_height=te_state.resolution==1?50:100;
    te_focal=te_width>>1;
    if(te_state.scope)te_focal<<=1;
    te_clip_left=te_clip_right=RT_NULL;
}
u8 te_background_get(void) {
    TeState *k=&buffers->key;
    setup();memset(&te_metrics,0,sizeof(te_metrics));
    if(te_state.scene==5)return 0; /* dedicated sprite timings always redraw */
    if(buffers->valid&&buffers->revision==te_revision&&
       !memcmp(&k->camera,&te_state.camera,sizeof(TeCamera))&&
       k->renderer==te_state.renderer&&k->resolution==te_state.resolution&&
       k->scene==te_state.scene&&k->training==te_state.training&&k->nsprites==te_state.nsprites&&
       k->sprite_mode==te_state.sprite_mode&&k->door_open==te_state.door_open&&
       k->scope==te_state.scope&&k->style==te_state.style) {
        memcpy(te_fb,buffers->background,2000);te_state.target=k->target;return 1;
    }
    return 0;
}
void te_background_put(void) {
    memcpy(buffers->background,te_fb,2000);buffers->key=te_state;
    buffers->revision=te_revision;buffers->valid=1;
}
void te_begin(void) {
    setup();
    memset(te_fb,0,2000);
    memset(te_ncover,0,160);
    memset(&te_metrics,0,sizeof(te_metrics));
    te_clip_left=te_clip_right=RT_NULL;
}
void te_pixel(s16 x,s16 y,u8 black) {
    u8 *p,m;
    if((u16)x>=te_width||(u16)y>=te_height)return;
    if(te_clip_left&&(x<te_clip_left[y]||x>te_clip_right[y]))return;
    p=te_fb+te_row[y]+((u16)x>>3);m=bits[(u16)x&7];
    if(black)*p|=m;else *p&=(u8)~m;
}
void te_span(s16 x0,s16 x1,s16 y,u8 pattern) {
    u8 *p,m;
    u16 a,b;
    if((u16)y>=te_height||x1<0||x0>=(s16)te_width)return;
    if(x0<0)x0=0;
    if(x1>=(s16)te_width)x1=te_width-1;
    if(te_clip_left) {
        if(x0<te_clip_left[y])x0=te_clip_left[y];
        if(x1>te_clip_right[y])x1=te_clip_right[y];
    }
    if(x0>x1)return;
    a=(u16)x0>>3;b=(u16)x1>>3;p=te_fb+te_row[y]+a;
    m=(u8)(255>>((u16)x0&7));
    if(a==b) {
        m&=(u8)(255<<(7-((u16)x1&7)));*p=(*p&~m)|(pattern&m);return;
    }
    *p=(*p&~m)|(pattern&m);p++;
    while(++a<b)*p++=pattern;
    m=(u8)(255<<(7-((u16)x1&7)));*p=(*p&~m)|(pattern&m);
}
void te_line(s16 x0,s16 y0,s16 x1,s16 y1,u8 black) {
    s16 dx=x1-x0,dy=y1-y0,sx=1,sy=1,err,e;
    /* Coordinates are bounded by projection to +/-1024. */
    if(dx<0){dx=-dx;sx=-1;}if(dy<0){dy=-dy;sy=-1;}
    err=dx-dy;
    for(;;) {
        te_pixel(x0,y0,black);
    if(x0==x1&&y0==y1)break;
        e=err+err;
        if(e>-dy){err-=dy;x0+=sx;}
        if(e<dx){err+=dx;y0+=sy;}
    }
}
s16 te_project(s16 value,u16 depth) {
    s16 f;
    s32 p;
    if(depth<8)depth=8;
    if(depth>2047)depth=2047;
    f=(s16)(te_mul(te_recip[depth],te_focal)>>6); /* Q8 */
    p=te_mul(value,f)>>8;
    if(p>1024)p=1024;
    if(p<-1024)p=-1024;
    return (s16)p;
}
s16 te_project_y(s16 value,u16 depth) {
    s16 y=te_project(value,depth);
    return te_width==80&&te_height==100?y+y:y;
}
void te_transform(s16 x,s16 y,s16 z,s16 *side,s16 *up,s16 *depth,u8 pitch) {
    const TeCamera *c=&te_state.camera;
    s16 cs=te_sin[(u8)(c->yaw+64)],sn=te_sin[c->yaw];
    s16 dx=x-c->x,dy=y-c->y,u=z-c->z;
    s16 d=(s16)((te_mul(dx,cs)+te_mul(dy,sn))>>8);
    *side=(s16)((te_mul(dx,sn)-te_mul(dy,cs))>>8);
    if(pitch) {
        cs=te_sin[(u8)(c->pitch+64)];sn=te_sin[(u8)c->pitch];
        *up=(s16)((te_mul(u,cs)-te_mul(d,sn))>>8);
        *depth=(s16)((te_mul(d,cs)+te_mul(u,sn))>>8);
    } else {*up=u;*depth=d;}
}
u8 te_occluded(u16 x,u16 y,u16 depth) {
    const TeSegment *s=te_cover[x];u16 i;
    for(i=0;i<te_ncover[x];i++,s++)
        if(y>=s->top&&y<=s->bottom&&depth>s->depth+2)return 1;
    return 0;
}
void te_present(void) {
    u16 x,y;
    u8 *p=(u8 *)rt_light;
    for(y=0;y<100;y++,p+=30) {
        const u8 *s=te_fb+te_row[te_height==50?y>>1:y];
        if(te_width==160)memcpy(p,s,20);
        else for(x=0;x<10;x++) {
            static const u8 expand[16]={0,3,12,15,48,51,60,63,192,195,204,207,240,243,252,255};
            u8 v=s[x];
            p[x+x]=expand[v>>4];p[x+x+1]=expand[v&15];
        }
    }
#ifndef RT_MONO
    memcpy(rt_dark,rt_light,RT_PSIZE);
#endif
}
u16 te_tables_bytes(void) {return sizeof(te_sin)+sizeof(te_recip)+sizeof(te_row);}
