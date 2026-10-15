/* ROM-derived data stays in the local tnmap archive bank. Two floor slots and
 * quantised wall intervals are a native collision ADAPTATION, not Tenchu code. */
#include "engine.h"
#include <string.h>
TeMesh te_mesh;
static const u8 *bank,*floors,*wallx,*wally;
u8 te_training_load(void) {
    u16 size;u32 expected;
    memset(&te_mesh,0,sizeof(te_mesh));
    bank=rt_file("tnmap",&size);
    if(!bank||size<24592||memcmp(bank,"TE02",4))return 0;
    te_mesh.nv=te_rd16(bank+4);te_mesh.nf=te_rd16(bank+6);te_mesh.na=te_rd16(bank+14);
    if(te_mesh.nv>2048||te_mesh.nf>2048||te_mesh.na>255)return 0;
    expected=24592UL+(u32)te_mesh.na*6+(u32)te_mesh.nv*6+(u32)te_mesh.nf*20;
    if(expected+TE_DATA_EXTRA!=size){memset(&te_mesh,0,sizeof(te_mesh));return 0;}
    floors=bank+16;wallx=floors+8192;wally=wallx+8192;
    te_mesh.anchors=wally+8192;
    te_mesh.vertices=te_mesh.anchors+te_mesh.na*6;
    te_mesh.faces=te_mesh.vertices+te_mesh.nv*6;
    return 1;
}
s16 te_ground(s16 x,s16 y,s16 maximum) {
    const u8 *p;s16 best=-1;u16 i,index;
    if(!te_mesh.nf||(u16)x>=1024||(u16)y>=1024)return -1;
    index=(((u16)y>>4)<<6)+((u16)x>>4);p=floors+(index<<1);
    for(i=0;i<2;i++)if(p[i]!=255) {
        s16 h=(u16)p[i]<<1;if(h<=maximum&&h>best)best=h;
    }
    return best;
}
static u8 wall_hit(const u8 *walls,u16 index,s16 low,s16 high) {
    const u8 *p=walls+(index<<1);
    return p[0]!=255&&high>(s16)((u16)p[0]<<1)&&low<(s16)((u16)p[1]<<1);
}
u8 te_training_barrier(s16 x,s16 y,s16 z,s16 tx,s16 ty,s16 tz) {
    u16 gx=(u16)x>>4,gy=(u16)y>>4,nx=(u16)tx>>4,ny=(u16)ty>>4;
    s16 low=z<tz?z:tz,high=z>tz?z:tz;
    if((u16)x>=1024||(u16)y>=1024||(u16)tx>=1024||(u16)ty>=1024)return 1;
    if(gx!=nx&&wall_hit(wallx,(gy<<6)+(gx>nx?gx:nx),low,high+1))return 1;
    if(gy!=ny&&wall_hit(wally,((gy>ny?gy:ny)<<6)+nx,low,high+1))return 1;
    return 0;
}
u8 te_training_move(s16 dx,s16 dy) {
    TeCamera *c=&te_state.camera;s16 x=c->x+dx,y=c->y+dy,h;
    if((u16)x>=1024||(u16)y>=1024)return 0;
    h=te_ground(x,y,te_state.feet_z+16);
    if(h<0&&!te_state.jump_frame)return 0;
    if(te_training_barrier(c->x,c->y,te_state.feet_z+3,x,y,te_state.feet_z+24))return 0;
    c->x=x;c->y=y;
    if(!te_state.jump_frame) {
        if(h<te_state.feet_z-16){te_state.jump_base=te_state.feet_z;te_state.jump_frame=12;}
        else {te_state.feet_z=h;c->z=h+32;}
    }
    return 1;
}
u8 te_training_los(s16 x,s16 y,s16 z,s16 tx,s16 ty,s16 tz) {
    s16 dx=tx-x,dy=ty-y,dz=tz-z,sx=1,sy=1,gx=x>>4,gy=y>>4,pz=z;
    u16 ax,ay,xt,yt,i;
    if(dx<0){dx=-dx;sx=-1;}if(dy<0){dy=-dy;sy=-1;}
    if(dx>2047||dy>2047)return 0;
    ax=dx?te_recip[dx]>>2:32767;ay=dy?te_recip[dy]>>2:32767;
    xt=dx?(u16)(te_mul(sx>0?16-(x&15):x&15,te_recip[dx])>>6):32767;
    yt=dy?(u16)(te_mul(sy>0?16-(y&15):y&15,te_recip[dy])>>6):32767;
    for(i=0;i<128;i++) {
        u16 t=xt<yt?xt:yt,index;const u8 *wall;s16 nz,low,high;
        if((u16)gx>=64||(u16)gy>=64)return 0;
        if(t>=255) {
            const u8 *f=floors+((((u16)gy<<6)+(u16)gx)<<1);u16 k;
            for(k=0;k<2;k++)if(f[k]!=255) {
                s16 h=(u16)f[k]<<1;
                if((pz<h&&tz>h)||(pz>h&&tz<h))return 0;
            }
            return 1;
        }
        nz=z+(s16)(te_mul(dz,t)>>8);
        low=nz<pz?nz:pz;high=nz>pz?nz:pz;
        if(xt<yt) {
            index=((u16)gy<<6)+(u16)(sx>0?gx+1:gx);wall=wallx;
            xt+=ax;gx+=sx;
        } else {
            index=((u16)(sy>0?gy+1:gy)<<6)+(u16)gx;wall=wally;
            yt+=ay;gy+=sy;
        }
        if(index>=4096||wall_hit(wall,index,nz,nz+1))return 0;
        if((u16)gx<64&&(u16)gy<64) {
            const u8 *f=floors+((((u16)gy<<6)+(u16)gx)<<1);u16 k;
            for(k=0;k<2;k++)if(f[k]!=255) {
                s16 h=(u16)f[k]<<1;
                if(low<h&&high>h)return 0;
            }
        }
        pz=nz;
    }
    return 0;
}

void te_training_anchors(void) {
    u16 i,dist[TE_MAX_ANCHORS],n=0;
    for(i=0;i<te_mesh.na;i++) {
        const u8 *p=te_mesh.anchors+i*6;s16 x=te_rd16(p),y=te_rd16(p+2),z=te_rd16(p+4);
        s16 dx=x-te_state.camera.x,dy=y-te_state.camera.y,dz=z-te_state.feet_z;u16 d,j;
        if(dz<8||dz>192)continue;
        d=(dx<0?-dx:dx)+(dy<0?-dy:dy);if(d>640)continue;
        j=n;if(j==TE_MAX_ANCHORS)j--;
        if(n==TE_MAX_ANCHORS&&d>=dist[j])continue;
        while(j>0&&dist[j-1]>d) {
            if(j<TE_MAX_ANCHORS){dist[j]=dist[j-1];te_world.anchor[j]=te_world.anchor[j-1];}j--;
        }
        dist[j]=d;te_world.anchor[j].x=x;te_world.anchor[j].y=y;te_world.anchor[j].z=z;
        if(n<TE_MAX_ANCHORS)n++;
    }
    te_world.nanchor=n;
}
void te_training_scene(void) {
    u16 x,y,k;
    te_revision++;memset(&te_world,0,sizeof(te_world));te_world.scene=4;
    for(y=0;y<16;y++)for(x=0;x<16;x++) {
        s16 h=te_ground((x<<6)+32,(y<<6)+32,32767);u16 high=h<0?0:h;
        /* Lossy ray adapter: walls are thickened to 64-unit blocks. The mesh
         * itself retains their true positions. Gameplay uses 16-unit collision. */
        for(k=0;k<16;k++) {
            u16 xx=(x<<2)+(k&3),yy=(y<<2)+(k>>2),idx=(yy<<6)+xx;
            const u8 *a=wallx+(idx<<1),*b=wally+(idx<<1);
            if(a[0]!=255&&((u16)a[1]<<1)>high)high=(u16)a[1]<<1;
            if(b[0]!=255&&((u16)b[1]<<1)>high)high=(u16)b[1]<<1;
        }
        te_world.floor[(y<<4)+x]=(u8)(high>>3);
    }
    te_training_anchors();
}
