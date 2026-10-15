#include "engine.h"
#include <string.h>
#include <stdlib.h>
TeWorld *te_world_ptr;
u16 te_revision;
TeState te_state;
u8 te_world_init(void) {
    if(!te_world_ptr)te_world_ptr=malloc(sizeof(*te_world_ptr));
    return te_world_ptr!=RT_NULL;
}
void te_world_finish(void) {if(te_world_ptr){free(te_world_ptr);te_world_ptr=RT_NULL;}}

static void box(s16 x0,s16 y0,s16 z0,s16 x1,s16 y1,s16 z1,u8 mat,u8 roof,u8 flags) {
    TeBox *b=&te_world.box[te_world.nbox++];
    u16 x,y;
    b->x0=x0;b->y0=y0;b->z0=z0;b->x1=x1;b->y1=y1;b->z1=z1;
    b->material=mat;b->cell=0;b->roof=roof;b->flags=flags;
    /* Phase 1 adapter: one highest surface per tile. Bridge loses its underside,
     * sloped roof becomes a flat top. These losses are explicit in RESEARCH.md. */
    for(y=(u16)y0>>6;y<((u16)y1>>6);y++)
        for(x=(u16)x0>>6;x<((u16)x1>>6);x++)
            if((u16)z1>>3 > te_world.floor[(y<<4)+x])
                te_world.floor[(y<<4)+x]=(u8)((u16)z1>>3);
}
static void anchor(s16 x,s16 y,s16 z) {
    TeAnchor *a=&te_world.anchor[te_world.nanchor++];a->x=x;a->y=y;a->z=z;
}
static s16 surface(const TeBox *b,s16 y) {
    s16 z=b->z1;
    if(b->roof) {
        s16 d=y-b->y0,e=b->y1-y;
        if(e<d)d=e;
        z+=(s16)(te_mul(b->roof,(s16)(te_mul(d,te_recip[(b->y1-b->y0)>>1])>>6))>>8);
    }
    return z;
}
void te_scene(u8 scene) {
    u16 x,y,i;
    te_revision++;memset(&te_world,0,sizeof(te_world));te_world.scene=scene;
    for(y=0;y<16;y++)for(x=0;x<16;x++)
        if(x==0||y==0||x==15||y==15)te_world.floor[(y<<4)+x]=24;
    box(0,0,0,1024,64,192,1,0,0);
    box(0,960,0,1024,1024,192,1,0,0);
    box(0,64,0,64,960,192,1,0,0);
    box(960,64,0,1024,960,192,1,0,0);
    if(scene==0) { /* B: Tenchu courtyard, primary fixture. */
        box(256,448,0,448,640,128,2,48,0);
        box(640,576,0,832,768,192,3,0,0);
        box(448,640,112,640,704,128,2,0,1); /* bridge */
        box(640,512,0,704,576,32,2,0,0);
        box(704,512,0,768,576,64,2,0,0);
        box(768,512,0,832,576,96,2,0,0);
        anchor(352,544,176);anchor(736,576,192);
        anchor(544,640,128);anchor(832,736,192);
    } else if(scene==1) { /* A: FFT-inspired fixture, no tactical rules. */
        box(192,448,0,576,640,64,2,0,0);
        box(640,576,0,768,704,256,3,0,0);
        box(192,384,0,320,448,16,2,0,0);
        box(320,384,0,448,448,32,2,0,0);
        box(448,384,0,576,448,48,2,0,0);
        anchor(704,576,256);
    } else if(scene==2) { /* C: connected MGS corridor rooms. */
        box(64,448,0,320,512,160,1,0,0);
        box(448,448,0,960,512,160,1,0,0);
        box(512,512,0,576,704,160,1,0,0);
        box(512,832,0,576,960,160,1,0,0);
        box(320,448,0,448,512,160,3,0,2);
        te_world.door[(7<<4)+5]=te_world.door[(7<<4)+6]=1;
        anchor(384,448,128);
    } else { /* D: Vagrant Story room. */
        box(192,512,0,384,704,48,2,0,0);
        box(640,512,0,832,704,96,2,0,0);
        box(384,704,112,640,768,128,2,0,1);
        box(448,576,0,512,640,64,3,0,0); /* crate */
        anchor(288,512,48);anchor(736,512,96);
    }
    te_world.nactor=TE_MAX_SPRITES;
    for(i=0;i<TE_MAX_SPRITES;i++) {
        TeActor *a=&te_world.actor[i];
        a->x=192+(i%5)*128;a->y=352+(i/5)*384;
        a->z=(s16)te_world.floor[((u16)a->y>>6)*16+((u16)a->x>>6)]*8;
        a->pose=(u8)(i&1);
    }
    if(scene==1){te_world.actor[0].x=512;te_world.actor[0].y=192;te_world.actor[0].z=0;}
}
void te_camera_defaults(u8 scene) {
    TeCamera *c=&te_state.camera;
    c->x=512;c->y=192;c->z=48;c->yaw=64;c->pitch=0;
    if(scene==2)c->x=384;
    if(scene==1){c->x=te_world.actor[0].x;c->y=te_world.actor[0].y;c->z=te_world.actor[0].z+48;}
    te_state.feet_z=0;te_state.grapple_ticks=0;
}
u8 te_los(s16 x,s16 y,s16 z,s16 tx,s16 ty,s16 tz) {
    /* Bounded, conservative native adaptation. 32-unit samples, endpoints
     * excluded so a ledge anchor remains selectable. Thin geometry needs slabs. */
    s16 dx=tx-x,dy=ty-y,dz=tz-z;
    u16 i,j,n=32;
    if(te_state.training)return te_training_los(x,y,z,tx,ty,tz);
    for(i=1;i<n;i++) {
        s16 px=x+(s16)(te_mul(dx,(s16)i)>>5);
        s16 py=y+(s16)(te_mul(dy,(s16)i)>>5);
        s16 pz=z+(s16)(te_mul(dz,(s16)i)>>5);
        for(j=0;j<te_world.nbox;j++) {
            const TeBox *b=&te_world.box[j];
            if((b->flags&2)&&te_state.door_open)continue;
            if(px>b->x0&&px<b->x1&&py>b->y0&&py<b->y1&&pz>b->z0&&pz<surface(b,py))return 0;
        }
    }
    return 1;
}
u8 te_move(s16 dx,s16 dy) {
    s16 x=te_state.camera.x+dx,y=te_state.camera.y+dy,h=0;
    u16 index;
    if(te_state.training)return te_training_move(dx,dy);
    if(x<72||y<72||x>=952||y>=952)return 0;
    for(index=0;index<te_world.nbox;index++) {
        const TeBox *b=&te_world.box[index];
        if((b->flags&2)&&te_state.door_open)continue;
        if(x<b->x0||x>=b->x1||y<b->y0||y>=b->y1)continue;
        if(b->z0>0&&te_state.feet_z+64<=b->z0)continue; /* beneath bridge */
        {s16 top=surface(b,y);
         if(top>te_state.feet_z+16)return 0;
         if(top>h)h=top;}
    }
    te_state.camera.x=x;te_state.camera.y=y;
    te_state.feet_z=h;te_state.camera.z=h+48;return 1;
}
/* Segment/box intersection: Q8 parameter in [0,256], -1 = no hit. Shared by
 * aiming and interaction tests; a box is conservative for a sloped rooftop. */
s16 te_ray_box(const TeCamera *c,s16 dx,s16 dy,s16 dz,const TeBox *b) {
    s16 pos[3]={c->x,c->y,c->z},dir[3]={dx,dy,dz};
    s16 lo[3]={b->x0,b->y0,b->z0},hi[3]={b->x1,b->y1,b->z1},enter=0,exit=256;
    u16 axis;
    for(axis=0;axis<3;axis++) {
        s16 d=dir[axis],a,z; s32 aa,zz;
        if(!d){if(pos[axis]<lo[axis]||pos[axis]>hi[axis])return -1;continue;}
        if(d<-2047||d>2047)return -1;
        a=d<0?-d:d;
        aa=te_mul(lo[axis]-pos[axis],te_recip[a])>>6;
        zz=te_mul(hi[axis]-pos[axis],te_recip[a])>>6;
        if(d<0){aa=-aa;zz=-zz;}
        if(aa>zz){s32 t=aa;aa=zz;zz=t;}
        if(aa>256||zz<0)return -1;
        a=aa<0?0:(s16)aa;z=zz>256?256:(s16)zz;
        if(a>enter)enter=a;
        if(z<exit)exit=z;
        if(enter>exit)return -1;
    }
    return enter;
}
s8 te_aim_guard(void) {
    const TeCamera *c=&te_state.camera;u16 i;s16 nearest=257;s8 hit=-1;
    s16 cp=te_sin[(u8)(c->pitch+64)],sp=te_sin[(u8)c->pitch];
    s16 dx=(s16)(te_mul((s16)(te_mul(te_sin[(u8)(c->yaw+64)],cp)>>8),640)>>8);
    s16 dy=(s16)(te_mul((s16)(te_mul(te_sin[c->yaw],cp)>>8),640)>>8);
    s16 dz=(s16)(te_mul(sp,640)>>8);
    for(i=0;i<te_state.nsprites;i++) {
        const TeActor *a=&te_world.actor[i];TeBox b;s16 t;
        b.x0=a->x-16;b.x1=a->x+16;b.y0=a->y-16;b.y1=a->y+16;b.z0=a->z;b.z1=a->z+64;
        t=te_ray_box(c,dx,dy,dz,&b);
        if(t>=0&&t<nearest&&te_los(c->x,c->y,c->z,a->x,a->y,a->z+32)){nearest=t;hit=i;}
    }
    return hit;
}
