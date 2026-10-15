/* Phase 2: bounded convex flat polygons, camera yaw/pitch, near clipping,
 * screen clipping, backface rejection and painter ordering. No pixel Z buffer.
 * Mean-face ordering is deliberately approximate for intersecting polygons. */
#include "engine.h"
#include <stdlib.h>
#include <string.h>
#if defined(TE_ONLY_RENDERER) && TE_ONLY_RENDERER == 0
u8 te_poly_init(void) {return 1;}
u16 te_poly_bytes(void) {return 0;}
void te_poly(void) {}
void te_finish(void) {te_render_finish();te_sprites_finish();te_world_finish();}
#else
#define MAX_FACES 128
#define MAX_VERTS 12
typedef struct { s16 x,y,d; } Vertex;
typedef struct { s16 x,y; } Point;
typedef struct { Point p[MAX_VERTS]; u16 depth; u8 n,shade,cell,object; } Face;
typedef struct {s16 left[100],right[100];} CellClip;
typedef struct {
    Face face[MAX_FACES];u8 order[MAX_FACES];u16 count;
    CellClip clip[3],merged;
    u8 visible;
} PolyWork;
static PolyWork *work;
u8 te_poly_init(void) {
    if(!work)work=malloc(sizeof(*work));
    return work!=RT_NULL;
}
void te_finish(void) {
    if(work){free(work);work=RT_NULL;}
    te_render_finish();te_sprites_finish();te_world_finish();
}
u16 te_poly_bytes(void) {return sizeof(*work);}
/* Q8 parameter. Division lookup accepts signed numerator/denominator.
 * All fixture coordinate differences <= 2047, no variable divide in loops. */
static s16 fraction(s16 numerator,s16 denominator) {
    s16 sign=1;
    if(denominator<0){denominator=-denominator;sign=-1;}
    if(!denominator)return 0;
    while(denominator>2047){denominator>>=1;numerator>>=1;}
    {s16 t=(s16)(te_mul(numerator,te_recip[denominator])>>6);return sign<0?-t:t;}
}
static Vertex mix(Vertex a,Vertex b,s16 t) {
    Vertex c;
    c.x=a.x+(s16)(te_mul(b.x-a.x,t)>>8);
    c.y=a.y+(s16)(te_mul(b.y-a.y,t)>>8);
    c.d=a.d+(s16)(te_mul(b.d-a.d,t)>>8);return c;
}
static u16 near_clip(const Vertex *v,u16 n,Vertex *out) {
    u16 i,m=0;Vertex a=v[n-1];u8 ain=a.d>=8;
    for(i=0;i<n;i++) {
        Vertex b=v[i];u8 bin=b.d>=8;
        if(ain!=bin) {
            out[m]=mix(a,b,fraction(8-a.d,b.d-a.d));out[m++].d=8;
        }
        if(bin)out[m++]=b;
        a=b;ain=bin;
    }
    return m;
}
static u16 screen_clip(Point *v,u16 n,u8 axis,s16 edge,u8 less,Point *out) {
    u16 i,m=0;Point a=v[n-1];s16 av=axis?a.y:a.x;u8 ain=less?av<=edge:av>=edge;
    for(i=0;i<n;i++) {
        Point b=v[i];s16 bv=axis?b.y:b.x;u8 bin=less?bv<=edge:bv>=edge;
        if(ain!=bin) {
            s16 t=fraction(edge-av,bv-av);Point c;
            c.x=a.x+(s16)(te_mul(b.x-a.x,t)>>8);
            c.y=a.y+(s16)(te_mul(b.y-a.y,t)>>8);
            if(axis)c.y=edge;else c.x=edge;
            out[m++]=c;
        }
        if(bin)out[m++]=b;
        a=b;av=bv;ain=bin;
    }
    return m;
}
static void polygon_spans(const Point *p,u16 n,CellClip *clip) {
    u16 i;s16 y;
    for(y=0;y<(s16)te_height;y++){clip->left[y]=32767;clip->right[y]=-32767;}
    for(i=0;i<n;i++) {
        Point a=p[i],b=p[i+1==n?0:i+1];s32 x,dx;
        if(a.y==b.y)continue;
        if(a.y>b.y){Point t=a;a=b;b=t;}
        dx=te_mul(b.x-a.x,te_recip[b.y-a.y])>>6;x=te_mul(a.x,256);
        for(y=a.y;y<b.y;y++,x+=dx) {
            s16 ix=x>>8;
            if(ix<clip->left[y])clip->left[y]=ix;
            if(ix>clip->right[y])clip->right[y]=ix;
        }
    }
}
static u8 cell_at(s16 x,s16 y) {return y<480?0:x<544?1:2;}
static u8 box_cells(u16 i) {
    if(te_world.scene!=2)return 1;
    if(i<4)return 7; /* bounding walls meet all three rooms */
    if(te_world.box[i].flags&2)return 3;
    if(i==6||i==7)return 6;
    return (u8)(1<<cell_at((te_world.box[i].x0+te_world.box[i].x1)>>1,
                           (te_world.box[i].y0+te_world.box[i].y1)>>1));
}
static void portals(void) {
    /* Hand-authored convex-room connectivity for the indoor fixture. Outdoor
     * fixtures are one cell. Bounding walls are shared between touching cells.
     * This prototype uses a tree: cyclic/multiple-path unions need revisiting. */
    static const s16 points[2][4][3]={
        {{320,480,0},{448,480,0},{448,480,160},{320,480,160}},
        {{544,704,0},{544,832,0},{544,832,160},{544,704,160}}
    };
    u8 queue[3],head=0,tail=0,start;
    u16 y;
    work->visible=0;
    for(start=0;start<3;start++)for(y=0;y<te_height;y++) {
        work->clip[start].left[y]=32767;work->clip[start].right[y]=-32767;
    }
    start=te_world.scene==2?cell_at(te_state.camera.x,te_state.camera.y):0;
    for(y=0;y<te_height;y++){work->clip[start].left[y]=0;work->clip[start].right[y]=te_width-1;}
    work->visible=(u8)(1<<start);queue[tail++]=start;
    while(head<tail) {
        u8 cell=queue[head++],portal;
        te_metrics.cells++;
        if(te_world.scene!=2)continue;
        for(portal=0;portal<2;portal++) {
            u8 child,i;u16 n;Vertex v[4],near[6];Point p[MAX_VERTS],q[MAX_VERTS];CellClip candidate;
            if(cell!=portal&&cell!=portal+1)continue;
            child=cell==portal?portal+1:portal;
            if(work->visible&(1<<child))continue;
            if(portal==0&&!te_state.door_open)continue;
            te_metrics.portals++;
            for(i=0;i<4;i++)te_transform(points[portal][i][0],points[portal][i][1],points[portal][i][2],&v[i].x,&v[i].y,&v[i].d,1);
            n=near_clip(v,4,near);if(n<3)continue;
            for(i=0;i<n;i++){p[i].x=(te_width>>1)+te_project(near[i].x,near[i].d);p[i].y=(te_height>>1)-te_project_y(near[i].y,near[i].d);}
            n=screen_clip(p,n,0,0,0,q);if(n<3)continue;
            n=screen_clip(q,n,0,te_width-1,1,p);if(n<3)continue;
            n=screen_clip(p,n,1,0,0,q);if(n<3)continue;
            n=screen_clip(q,n,1,te_height-1,1,p);if(n<3)continue;
            polygon_spans(p,n,&candidate);
            n=0;
            for(y=0;y<te_height;y++) {
                if(candidate.left[y]<work->clip[cell].left[y])candidate.left[y]=work->clip[cell].left[y];
                if(candidate.right[y]>work->clip[cell].right[y])candidate.right[y]=work->clip[cell].right[y];
                if(candidate.left[y]<=candidate.right[y])n++;
            }
            if(!n)continue;
            work->clip[child]=candidate;work->visible|=(u8)(1<<child);queue[tail++]=child;
        }
    }
}
static void choose_clip(u8 cells) {
    u16 y;u8 c;
    u8 start=te_world.scene==2?cell_at(te_state.camera.x,te_state.camera.y):0;
    if(cells&(1<<start)){te_clip_left=te_clip_right=RT_NULL;return;}
    for(y=0;y<te_height;y++) {
        s16 left=32767,right=-32767;
        for(c=0;c<3;c++)if((cells&work->visible)&(1<<c)) {
            if(work->clip[c].left[y]<left)left=work->clip[c].left[y];
            if(work->clip[c].right[y]>right)right=work->clip[c].right[y];
        }
        work->merged.left[y]=left;work->merged.right[y]=right;
    }
    te_clip_left=work->merged.left;te_clip_right=work->merged.right;
}
static void add_face(Vertex *vertex,const u8 *indices,u8 n,u8 shade,u8 cell,u8 object) {
    Vertex a[6],b[8];Point p[MAX_VERTS],q[MAX_VERTS];
    u16 i,m,depth=0; s32 area=0;
    Face *f;
    for(i=0;i<n;i++)a[i]=vertex[indices[i]];
    m=near_clip(a,n,b);if(m<3){te_metrics.rejected++;return;}
    for(i=0;i<m;i++) {
        p[i].x=(te_width>>1)+te_project(b[i].x,b[i].d);
        p[i].y=(te_height>>1)-te_project_y(b[i].y,b[i].d);
        depth+=b[i].d;
    }
    /* Mean depth for painter sorting is one hardware divide, outside hot
     * pixel/scanline loops. It remains a measured baseline strategy. */
    depth=(u16)(te_mul(depth,te_recip[m])>>14);
    for(i=0;i<m;i++) {
        Point c=p[i+1==m?0:i+1];
        area+=te_mul(p[i].x,c.y)-te_mul(c.x,p[i].y);
    }
    if(area>=0&&!(te_renderer==4&&te_state.style==1)){te_metrics.rejected++;return;}
    m=screen_clip(p,m,0,0,0,q);if(m<3)return;
    m=screen_clip(q,m,0,te_width-1,1,p);if(m<3)return;
    m=screen_clip(p,m,1,0,0,q);if(m<3)return;
    m=screen_clip(q,m,1,te_height-1,1,p);if(m<3)return;
    if(work->count==MAX_FACES){te_metrics.segment_overflow++;return;}
    f=&work->face[work->count++];f->n=m;f->shade=shade;f->cell=cell;f->depth=depth;f->object=object;
    for(i=0;i<m;i++)f->p[i]=p[i];
}
static void transform_box(const TeBox *b,Vertex *v,u16 needed) {
    u16 i;
    for(i=0;i<8;i++) {
        s16 x=(i&3)==0||(i&3)==3?b->x0:b->x1;
        s16 y=(i&3)<2?b->y0:b->y1,z=i<4?b->z0:b->z1;
        if(needed&(1<<i))te_transform(x,y,z,&v[i].x,&v[i].y,&v[i].d,1);
    }
    if(b->roof&&(needed&768)) {
        te_transform(b->x0,(b->y0+b->y1)>>1,b->z1+b->roof,&v[8].x,&v[8].y,&v[8].d,1);
        te_transform(b->x1,(b->y0+b->y1)>>1,b->z1+b->roof,&v[9].x,&v[9].y,&v[9].d,1);
    }
}
static void project_world(void) {
    static const u8 face[6][4]={{0,1,5,4},{1,2,6,5},{2,3,7,6},{3,0,4,7},{4,5,6,7},{3,2,1,0}};
    static const u8 roof[4][4]={{4,5,9,8},{8,9,6,7},{4,8,7,7},{5,6,9,9}};
    u16 i,j;
    work->count=0;
    if(te_state.training) {
        static const u8 idx[4]={0,1,2,3};
        const u8 *p=te_mesh.faces;
        for(i=0;i<te_mesh.nf;i++,p+=20) {
            Vertex v[4];u16 index=te_rd16(p);const u8 *src=te_mesh.vertices+index*6;
            s16 x=te_rd16(src),y=te_rd16(src+2),z=te_rd16(src+4);u8 axis=p[9];
            const TeCamera *c=&te_state.camera;
            s16 xmin=te_rd16(p+12),ymin=te_rd16(p+14),xmax=te_rd16(p+16),ymax=te_rd16(p+18);
            s16 side,up,depth,radius=((xmax-xmin)+(ymax-ymin))>>1;
            te_transform((xmin+xmax)>>1,(ymin+ymax)>>1,c->z,&side,&up,&depth,0);
            if(depth-radius>512||depth+radius<8) {te_metrics.rejected++;continue;}
            if((side<0?-side:side)>2*(depth+radius)+512){te_metrics.rejected++;continue;}
            if(!(te_renderer==4&&te_state.style==1)&&
               ((axis==1&&c->x<=x)||(axis==2&&c->x>=x)||(axis==3&&c->y<=y)||(axis==4&&c->y>=y)||
                (axis==5&&c->z<=z)||(axis==6&&c->z>=z))){te_metrics.rejected++;continue;}
            for(j=0;j<p[8];j++) {
                index=te_rd16(p+j*2);src=te_mesh.vertices+index*6;
                te_transform(te_rd16(src),te_rd16(src+2),te_rd16(src+4),&v[j].x,&v[j].y,&v[j].d,1);
            }
            add_face(v,idx,p[8],p[10],1,255);
        }
        return;
    }
    for(i=0;i<te_world.nbox;i++) {
        const TeBox *b=&te_world.box[i];Vertex v[10];u8 cells=box_cells(i),visible=0,roof_visible=0;
        const TeCamera *c=&te_state.camera;u16 needed=0,k;
        if((b->flags&2)&&te_state.door_open)continue;
        if(te_renderer==2&&!(cells&work->visible)){te_metrics.rejected+=6;continue;}
        /* Exact world-plane rejection BEFORE transforms/clipping. */
        if(c->y<b->y0)visible|=1;
        if(c->x>b->x1)visible|=2;
        if(c->y>b->y1)visible|=4;
        if(c->x<b->x0)visible|=8;
        if(c->z>b->z1&&!b->roof)visible|=16;
        if(c->z<b->z0)visible|=32;
        if(b->roof) {
            s16 half=(b->y1-b->y0)>>1;
            if(te_mul(-b->roof,c->y-b->y0)+te_mul(half,c->z-b->z1)>0)roof_visible|=1;
            if(te_mul(b->roof,c->y-b->y1)+te_mul(half,c->z-b->z1)>0)roof_visible|=2;
            if(c->x<b->x0)roof_visible|=4;
            if(c->x>b->x1)roof_visible|=8;
        }
        if(te_renderer==4&&te_state.style==1){visible=63;roof_visible=15;}
        for(j=0;j<6;j++)if(visible&(1<<j))for(k=0;k<4;k++)needed|=(u16)(1<<face[j][k]);
        for(j=0;j<4;j++)if(roof_visible&(1<<j))for(k=0;k<4;k++)needed|=(u16)(1<<roof[j][k]);
        transform_box(b,v,needed);
        for(j=0;j<6;j++) {
            if(j==4&&b->roof)continue;
            if(!(visible&(1<<j))){te_metrics.rejected++;continue;}
            add_face(v,face[j],4,j==4?2:((j&1)?1:3),cells,(u8)i);
        }
        if(b->roof)for(j=0;j<4;j++)if(roof_visible&(1<<j))add_face(v,roof[j],j<2?4:3,j<2?2:3,cells,(u8)i);
    }
    for(i=0;i<te_state.nsprites;i++) {
        s16 x,y,d;Face *f;u8 cells=te_world.scene==2?(1<<cell_at(te_world.actor[i].x,te_world.actor[i].y)):1;
        if(te_renderer==2&&!(cells&work->visible))continue;
        te_transform(te_world.actor[i].x,te_world.actor[i].y,te_world.actor[i].z+32,&x,&y,&d,1);
        if(d<8||work->count==MAX_FACES)continue;
        f=&work->face[work->count++];f->n=0;f->shade=128+i;f->depth=d;f->cell=cells;
    }
}
static void sort_faces(void) {
    u16 i;
    for(i=0;i<work->count;i++) {
        s16 j=i;u8 idx=(u8)i;u16 d=work->face[i].depth;
        while(j>0&&work->face[work->order[j-1]].depth<d){work->order[j]=work->order[j-1];j--;}
        work->order[j]=idx;
    }
}
static void fill_face(const Face *f) {
    s16 left[100],right[100],y,miny=100,maxy=-1;
    u16 i;
    for(i=0;i<f->n;i++) {
        if(f->p[i].y<miny)miny=f->p[i].y;
        if(f->p[i].y>maxy)maxy=f->p[i].y;
    }
    for(y=miny;y<=maxy;y++){left[y]=32767;right[y]=-32767;}
    for(i=0;i<f->n;i++) {
        Point a=f->p[i],b=f->p[i+1==f->n?0:i+1];s32 x,dx;
        if(a.y==b.y)continue;
        if(a.y>b.y){Point t=a;a=b;b=t;}
        dx=te_mul(b.x-a.x,te_recip[b.y-a.y])>>6;
        x=te_mul(a.x,256);
        for(y=a.y;y<b.y;y++,x+=dx) {
            s16 ix=(s16)(x>>8);
            if(ix<left[y])left[y]=ix;
            if(ix>right[y])right[y]=ix;
        }
    }
    for(y=miny;y<=maxy;y++) {
        u8 pattern=f->shade==1?255:f->shade==2?((y&1)?0xAA:0x55):0;
        if(te_renderer==3&&te_state.style!=2&&f->shade==2&&f->object<te_world.nbox&&!te_world.box[f->object].roof) {
            TE_BEGIN(11);te_floor_span(y,left[y],right[y],te_world.box[f->object].z1);TE_END(11);
        } else {
            if(te_renderer==4) {
                if(te_state.style==1)continue; /* full wireframe, intentionally transparent */
                if(te_state.style==4&&f->depth>256)continue; /* mixed distant lines */
                pattern=te_state.style==2?255:te_state.style==3?((y&3)?0:0x88):0;
            }
            te_span(left[y],right[y],y,pattern);
        }
    }
    if(te_renderer==4?te_state.style!=2:(te_renderer!=3&&te_state.style!=0))for(i=0;i<f->n;i++) {
        Point a=f->p[i],b=f->p[i+1==f->n?0:i+1];te_line(a.x,a.y,b.x,b.y,1);
    }
    te_metrics.polygons++;
}
void te_poly(void) {
    u16 i;
    if(!work)return;
    if(te_renderer==2){TE_BEGIN(9);portals();TE_END(9);}
    TE_BEGIN(8);project_world();TE_END(8);
    TE_BEGIN(9);sort_faces();TE_END(9);
    TE_BEGIN(10);
    for(i=0;i<work->count;i++) {
        const Face *f=&work->face[work->order[i]];
        if(te_renderer==2)choose_clip(f->cell);
        if(!f->n){TE_END(10);TE_BEGIN(4);te_actor(f->shade-128);TE_END(4);TE_BEGIN(10);}
        else fill_face(f);
    }
    TE_END(10);
    te_clip_left=te_clip_right=RT_NULL;
}
#endif
