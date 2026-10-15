#include "engine.h"
/* Placeholder human silhouette, 8x16, separate opacity and black pixels.
 * White outline is opaque, never confused with transparency. */
static const u8 ink[16]={0,24,60,60,24,60,126,126,90,24,60,60,36,36,102,0};
static const u8 opaque[16]={24,60,126,126,126,255,255,255,255,126,126,126,126,255,255,102};
#include "sprites_data.h"
#include <stdlib.h>
#include <string.h>
typedef struct {u8 data[512];u16 used;u8 key,valid;} ScaleCache;
static ScaleCache *cache;
static const u8 *premade;
static u16 cache_clock;
u8 te_sprites_init(void) {
    u16 size;
    premade=rt_file("tnspr",&size);
    if(premade&&size!=2464+TE_DATA_EXTRA)premade=RT_NULL;
    if(!cache)cache=malloc(4*sizeof(*cache));
    if(!cache)return 0;
    memset(cache,0,4*sizeof(*cache));cache_clock=0;return 1;
}
void te_sprites_finish(void) {if(cache){free(cache);cache=RT_NULL;}}
u16 te_sprite_cache_bytes(void) {return 4*sizeof(*cache);}
u16 te_sprite_flash_bytes(void) {return 2464+sizeof(scale_offset)+sizeof(levels)+sizeof(height_level)+sizeof(ink)+sizeof(opaque);}
static void scale(u8 key,u8 *out) {
    u16 h=levels[key&7],w=h>>1,stride=(w+7)>>3,y,x,v=0;
    u16 du=te_recip[w]>>3,dv=te_recip[h]>>2;
    TE_BEGIN(14);
    for(y=0;y<h;y++,v+=dv) {
        u8 *mask=out,*data=out+stride;u16 u=0;u8 in=ink[v>>8];
        if(key>=8) {
            if((v>>8)==12)in=36;
            if((v>>8)==13)in=66;
            if((v>>8)==14)in=195;
        }
        memset(out,0,stride+stride);
        for(x=0;x<w;x++,u+=du) {
            u8 bit=(u8)(128>>(u>>8)),dest=(u8)(128>>(x&7));
            if(opaque[v>>8]&bit)mask[x>>3]|=dest;
            if(in&bit)data[x>>3]|=dest;
        }
        out+=stride+stride;
    }
    TE_END(14);
}
static const u8 *cached(u8 key) {
    u16 i,oldest=0,age=0;
    cache_clock++;
    for(i=0;i<4;i++) {
        if(cache[i].valid&&cache[i].key==key) {
            cache[i].used=cache_clock;te_metrics.cache_hits++;return cache[i].data;
        }
        if(!cache[i].valid){oldest=i;age=65535;}
        else if((u16)(cache_clock-cache[i].used)>age) {
            age=(u16)(cache_clock-cache[i].used);oldest=i;
        }
    }
    scale(key,cache[oldest].data);
    cache[oldest].key=key;cache[oldest].valid=1;cache[oldest].used=cache_clock;
    te_metrics.cache_misses++;return cache[oldest].data;
}
static void masked_byte(s16 byte,s16 y,u8 mask,u8 data,u16 depth) {
    u16 bit;s16 left=0,right=te_width-1;u8 *p;
    if(!mask||byte<0||byte>=(s16)(te_width>>3))return;
    if(te_clip_left){left=te_clip_left[y];right=te_clip_right[y];}
    if(left>byte*8) {
        if(left>=byte*8+8)return;
        mask&=(u8)(255>>(left&7));
    }
    if(right<byte*8+7) {
        if(right<byte*8)return;
        mask&=(u8)(255<<(7-(right&7)));
    }
    if(te_renderer==0) {
        TE_BEGIN(13);
        for(bit=0;bit<8;bit++)if((mask&(128>>bit))&&te_occluded(byte*8+bit,y,depth))mask&=(u8)~(128>>bit);
        TE_END(13);
    }
    p=te_fb+te_row[y]+byte;
    *p=(*p&~mask)|(data&mask);
    /* Pixel count retained without a ROM popcount or per-pixel drawing calls. */
    if(premade)te_metrics.sprite_pixels+=premade[2208+mask];
    else for(bit=0;bit<8;bit++)if(mask&(128>>bit))te_metrics.sprite_pixels++;
}
void te_actor(u16 n) {
    const TeActor *a=&te_world.actor[n];
    s16 side,up,depth,x0,y0,h,w,yy;
    u16 stride,shift,k,yscale;
    u8 key,temp[512];const u8 *pixels;
    te_transform(a->x,a->y,a->z+64,&side,&up,&depth,te_renderer!=0);
    if(depth<8)return;
    h=te_project(64,depth);
    if(h<4)h=4;
    if(h>64)h=64;
    key=height_level[h]+((a->pose&1)<<3);h=levels[key&7];w=h>>1;
    x0=(te_width>>1)+te_project(side,depth)-(w>>1);
    y0=(te_height>>1)+(te_renderer?0:te_state.camera.pitch*(te_height==50?1:2))-te_project_y(up,depth);
    yscale=te_width==80&&te_height==100?2:1;
    if(x0+w<=0||x0>=(s16)te_width||y0+h*yscale<=0||y0>=(s16)te_height)return;
    stride=((u16)w+7)>>3;shift=(u16)x0&7;
    if(te_state.sprite_mode==0&&premade)pixels=premade+scale_offset[key];
    else if(te_state.sprite_mode==2)pixels=cached(key);
    else {scale(key,temp);pixels=temp;}
    for(yy=0;yy<h;yy++) {
        u16 dy;
        for(dy=0;dy<yscale;dy++) {
            s16 y=y0+yy*yscale+dy;
            if((u16)y>=te_height)continue;
            for(k=0;k<stride;k++) {
                u8 mask=pixels[k],data=pixels[stride+k];s16 dest=(x0>>3)+k;
                masked_byte(dest,y,mask>>shift,data>>shift,depth);
                if(shift)masked_byte(dest+1,y,(u8)(mask<<(8-shift)),(u8)(data<<(8-shift)),depth);
            }
        }
        pixels+=stride+stride;
    }
}
void te_sprites(void) {
    u16 n,d[10];u8 order[10];
    TE_BEGIN(4);
    for(n=0;n<te_state.nsprites;n++) {
        s16 s,u,z,j=n;
        te_transform(te_world.actor[n].x,te_world.actor[n].y,te_world.actor[n].z,&s,&u,&z,0);
        d[n]=z<0?0:z;
        while(j>0&&d[order[j-1]]<d[n]){order[j]=order[j-1];j--;}
        order[j]=n;
    }
    for(n=0;n<te_state.nsprites;n++)te_actor(order[n]);
    TE_END(4);
}
s8 te_target(void) {
    u16 i; s16 best=12; s8 result=-1;
    for(i=0;i<te_world.nanchor;i++) {
        const TeAnchor *a=&te_world.anchor[i];s16 s,u,d,x,y,score;
        te_transform(a->x,a->y,a->z,&s,&u,&d,te_renderer!=0);
    if(d<8||d>640)continue;
        x=te_project(s,d);y=(te_renderer?0:te_state.camera.pitch*(te_height==50?1:2))-te_project_y(u,d);
        score=(x<0?-x:x)+(y<0?-y:y);
        if(score<best&&te_los(te_state.camera.x,te_state.camera.y,te_state.camera.z,a->x,a->y,a->z)) {
            best=score;result=(s8)i;
        }
    }
    return result;
}
void te_markers(void) {
    u16 i; s16 cx=te_width>>1,cy=te_height>>1;
    for(i=0;i<te_world.nanchor;i++) {
        const TeAnchor *a=&te_world.anchor[i];s16 s,u,d,x,y;
        te_transform(a->x,a->y,a->z,&s,&u,&d,te_renderer!=0);
    if(d<8)continue;
        x=cx+te_project(s,d);y=cy+(te_renderer?0:te_state.camera.pitch*(te_height==50?1:2))-te_project_y(u,d);
        if((u16)x>=te_width||(u16)y>=te_height||te_occluded(x,y,d))continue;
        if(!te_los(te_state.camera.x,te_state.camera.y,te_state.camera.z,a->x,a->y,a->z))continue;
        {static const u8 outline[8]={24,60,126,255,255,126,60,24};
         static const u8 diamond[8]={0,24,60,102,102,60,24,0};u16 row,col;
         for(row=0;row<8;row++)for(col=0;col<8;col++) {
             u8 bit=(u8)(128>>col),ink=diamond[row];
             if((s8)i==te_state.target&&(row==3||row==4))ink|=24;
             if(outline[row]&bit)te_pixel(x-4+col,y-4+row,(ink&bit)!=0);
         }}
    }
    te_line(cx-3,cy,cx+3,cy,0);te_line(cx,cy-3,cx,cy+3,0);
    te_line(cx-2,cy,cx+2,cy,1);te_line(cx,cy-2,cx,cy+2,1);
}
