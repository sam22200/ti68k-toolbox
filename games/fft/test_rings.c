// Compare the cached scene with a clean composition, including invalidations
// while the old reach is still being removed, using the existing asset banks.
#ifdef TEST_OPEN
#include "opening/opening.c"
#elif defined(TEST_CAMPAIGN)
#include "campaign/gariland.c"
#else
#include "fft.c"
#endif
#include "../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

static u8 *reference;
static unsigned comparisons;

#ifdef FFT_REACH_CONTOUR
// Independent topology check: project each missing world neighbour and infer
// the outside screen edge from its quadrant, without the renderer's bit LUT.
static u8 reference_edges(u8 tile)
{
    static const s8 dx[4]={0,1,0,-1},dz[4]={-1,0,1,0};
    s16 u,v,cx,cy;u8 k,bits=0;
    view_of(st.rot,tile%MAP_W,tile/MAP_W,&u,&v);
    cx=scene_x(st.rot,u,v);cy=scene_y(u,v);
    for(k=0;k<4;k++){
        s16 x=(s16)(tile%MAP_W)+dx[k],z=(s16)(tile/MAP_W)+dz[k],sx,sy;
        if((u16)x<MAP_W&&(u16)z<MAP_H&&st.reach[z*MAP_W+x]!=0xFF)continue;
        view_of(st.rot,x,z,&u,&v);sx=scene_x(st.rot,u,v)-cx;sy=scene_y(u,v)-cy;
        bits|=(u8)(1U<<(sx>0?(sy<0?0:1):(sy>0?2:3)));
    }
    return bits;
}
#endif

static void verify(void)
{
    u8 *saved_l=scene_l,*saved_d=scene_d;u16 i;
    W->left=30000;ensure_scene(st.rot);
    assert(W->ring_i==MAP_W*MAP_H && W->drop_y0>=W->drop_y1);
    memcpy(reference,vbuf,2*SC_PLANE);
    scene_l=reference;scene_d=reference+SC_PLANE;
    if(RINGS(st.mode))for(i=0;i<MAP_W*MAP_H;i++)if(st.reach[i]!=0xFF){
        Marker mk;
#ifdef FFT_REACH_CONTOUR
        if(st.reach_contour && contour_gfx && !AIMING(st.mode)){
            u8 edges=reference_edges((u8)i);
            assert(edges==reach_edges((u8)i,st.rot));
            if(!edges)continue;
            marker(st.rot,(u8)i,1,&mk);mk.edges=edges;
        }else marker(st.rot,(u8)i,0,&mk);
#else
        marker(st.rot,(u8)i,0,&mk);
#endif
        ring_into_scene(&mk);
    }
    scene_l=saved_l;scene_d=saved_d;
    assert(!memcmp(scene_l,reference,SC_PLANE));
    assert(!memcmp(scene_d,reference+SC_PLANE,SC_PLANE));
    // Exercise partially-built and completed screen caches too. The PC
    // screen_scene oracle checks every cache hit against an uncached copy.
    for(i=0;i<7;i++){W->left=30000;screen_scene();}
    assert(!bg_bad);comparisons++;
}

int main(void)
{
    u8 rot,i;u16 c,version;
#ifdef FFT_REACH_CONTOUR
    u8 style;
#endif
#ifdef TEST_OPEN
#ifdef TEST_CONTOUR_DATA
    assert(!chdir(TEST_CONTOUR_DATA));sw_init(1);
#else
    assert(!chdir("opening/data"));sw_init(1);
#endif
#elif defined(TEST_CAMPAIGN)
#ifdef TEST_CONTOUR_DATA
    assert(!chdir(TEST_CONTOUR_DATA));sw_init(61);
#else
    assert(!chdir("campaign/data"));sw_init(61);
#endif
#else
    sw_init(10);
#endif
#ifdef FFT_REACH_CONTOUR
    // Real key routing: settings freeze the battle, consume keys and persist
    // through Move/AT-list switches; APPS stays distinct from CATALOG.
    {
        State saved;u32 rng;u8 k;
        assert(contour_gfx && !st.reach_contour);
        for(k=0;k<60;k++)assert(sw_step(0));
        assert(sw_step(K_ENTER));assert(sw_step(0));assert(st.mode==M_TARGET);
        saved=st;rng=rng32;
        assert(sw_step(K_CATALOG|K_APPS));assert(st.config && st.config_row==CONFIG_MOVE);
        for(k=0;k<8;k++)assert(sw_step(K_CATALOG|K_APPS));
        assert(sw_step(0));assert(sw_step(K_ENTER));assert(sw_step(0));
        assert(st.reach_contour && st.config);
        assert(!memcmp(saved.unit,st.unit,sizeof st.unit));
        assert(!memcmp(saved.bag,st.bag,sizeof st.bag));
        assert(!memcmp(saved.reach,st.reach,sizeof st.reach));
        assert(saved.tick==st.tick && rng==rng32 && saved.mode==st.mode);
        assert(sw_step(K_ESC));assert(!st.config && st.mode==M_TARGET);
        assert(sw_step(0));assert(sw_step(K_APPS));assert(st.mode==M_ORDER);
        assert(sw_step(0));assert(sw_step(K_APPS));assert(sw_step(0));
        assert(st.mode==M_TARGET && st.reach_contour);
        assert(sw_step(K_CATALOG));assert(sw_step(0));
        assert(sw_step(K_ENTER));assert(sw_step(0));assert(!st.reach_contour);
        assert(sw_step(K_CATALOG));assert(sw_step(0));assert(!st.config && st.mode==M_TARGET);
        assert(sw_step(K_B));assert(sw_step(0));assert(st.mode==M_MENU);
    }
#endif
    reference=malloc(2*SC_PLANE);assert(reference);
#ifdef FFT_REACH_CONTOUR
    assert(contour_gfx);
    for(style=0;style<2;style++){
        st.reach_contour=style;hl_seq++;
#endif
    for(rot=0;rot<4;rot++){
        st.rot=rot;st.turn=0;st.mode=M_TARGET;st.over=0;
        compute_reach(U_RAMZA);verify();
        // Different costs with identical membership must leave the visual
        // cache intact, with no ring drawing charged to the frame budget.
        for(c=0;c<MAP_W*MAP_H;c++)if(st.reach[c]!=0xFF)st.reach[c]=2;
        version=scene_ver;hl_seq++;verify();
        assert(W->left==30000 && scene_ver==version);
        // Arbitrary additions/removals include slopes and overlapping boxes.
        for(i=0;i<12;i++){
            c=(u16)((i*17+rot*7)%(MAP_W*MAP_H));
            st.reach[c]=st.reach[c]==0xFF?1:0xFF;hl_seq++;verify();
        }
        st.over=1;hl_seq++;verify();
        st.mode=M_AIM;hl_seq++;verify();
        st.over=0;hl_seq++;verify();
        // Cancel a range, then open another before row restoration finishes.
        st.mode=M_MENU;hl_seq++;W->left=-1;ensure_scene(rot);
        assert(W->drop_y0<W->drop_y1);
        st.mode=M_AIM;memset(st.reach,0xFF,sizeof st.reach);
        st.reach[rot+MAP_W]=1;hl_seq++;verify();
        st.mode=M_MENU;hl_seq++;verify();
        st.mode=M_TARGET;memset(st.reach,0xFF,sizeof st.reach);hl_seq++;verify();
    }
#ifdef FFT_REACH_CONTOUR
    }
#endif
    printf("Reach cache: %u clean scene comparisons, four views, styles, costs and interrupted removal passed\n",comparisons);
    free(reference);return 0;
}
