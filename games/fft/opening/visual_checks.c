// Presentation regressions: use real frames and move paths, never a second renderer.
#include "opening.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
static void view_unit(u8 i,u8 rot)
{
    st.rot=rot;st.cx=st.unit[i].x;st.cz=st.unit[i].z;
    st.mode=M_BROWSE;st.ai=0;st.turn=0;W->frz=W->hold=0;
    cam_target(&st.camx,&st.camy);scene_rot=vbuf_rot=255;W->bg_ok=0;
}
int main(void)
{
    u8 i,r;unsigned f,frames,changes,pictures=0;u16 prev,max_latency=0;
    if(access("ffoa.bin",R_OK))assert(!chdir("data"));
    for(i=0;i<NUNIT;i++) {
        sw_init(1);view_unit(i,2);prev=W->actor_key[i];changes=0;
        for(f=0;f<96;f++) {
            assert(sw_step(0));
            if(prev!=W->actor_key[i]){changes++;prev=W->actor_key[i];}
            assert(W->actor_misses<=1);
        }
        assert(changes>=5);printf("idle,%u,changes,%u\n",i,changes);
    }
    // Cold enemy browsing: target complete panel is ready, not only its first pixels.
    sw_init(1);for(f=0;f<80;f++)assert(sw_step(0));
    for(i=1;i<NUNIT;i++) {
        st.mode=M_BROWSE;st.cx=st.unit[i].x;st.cz=st.unit[i].z;
        cam_target(&st.camx,&st.camy);W->frz=W->hold=0;
        for(f=1;f<=16;f++) {
            assert(sw_step(0));
            if(W->pan_ok[1]&&W->pan_fk[1][0]==i+1&&hud_ready())break;
        }
        assert(f<=16);if(f>max_latency)max_latency=f;
        printf("portrait,%u,frames,%u\n",i,f);
    }
    // The panel opacity mask must use the same trimmed portrait index as its pixels.
    for(i=0;i<NUNIT;i++)for(r=0;r<2;r++) {
        u8 gfx=st.unit[i].gfx,row;const u8 *p;
        if(gfx>1)gfx--;
        p=igfx+UI_PORTRAITS+(u16)gfx*348+232;
        W->pan_n[r]=0;make_panel(r,i);
        for(row=0;row<29;row++) {
            const u8 *m=(const u8 *)PAN_MAKE(r)[2]+row*RT_PBYTES;
            u32 bits=~RD32(p,row*2);
            if(r) {assert(RD16(m,3)==(u16)(bits>>29));assert(RD16(m,4)==(u16)(bits>>13));}
            else assert(RD32(m,0)==bits);
        }
    }
    // Priority actor: Gafgarion and every enemy, through actual reachable walks in each view.
    for(i=2;i<=9;i++)if(i==2||i>=5)for(r=0;r<4;r++) {
        u8 c,best=0;u16 maximum=0;
        sw_init(1);view_unit(i,r);st.act=st.sel=i;compute_reach(i);
        for(c=0;c<MAP_W*MAP_H;c++)if(st.reach[c]!=255&&st.reach[c]>maximum&&!unit_at(c%MAP_W,c/MAP_W)) {maximum=st.reach[c];best=c;}
        assert(maximum);assert(make_path(i,best%MAP_W,best/MAP_W)>1);
        st.path_i=st.walk_t=0;st.mode=M_WALK;frames=changes=0;prev=W->actor_key[i];
        while(st.mode==M_WALK&&frames<160) {
            assert(sw_step(0));frames++;
            if(st.mode==M_WALK) {
                u8 dir=unit_direction(st.rot,i),k=walk_seq[(u8)(st.tick+11*i)&15];
                if(dir&1)k+=5;
                if(dir&2)k+=UNIT_FRAMES;
                assert(W->actor_key[i]==RD16(ugfx,(u16)(st.unit[i].gfx*44+k)));
                assert(W->actor_misses<=1);pictures++;
            }
            if(prev!=W->actor_key[i]){changes++;prev=W->actor_key[i];}
        }
        assert(frames<160&&changes>=3);
    }
    for(r=0;r<4;r++) {
        u8 phases[4][2][RT_PSIZE];unsigned k,sx=0,sy=0;
        sw_init(1);view_unit(0,r);for(f=0;f<40;f++)assert(sw_step(0));
        assert(W->bg_ok);
        for(k=0;k<W->glints[0];k++){sx+=(unsigned)W->puddle[0][k*2]*2;sy+=W->puddle[0][k*2+1];}
        st.camx=(s16)(sx/W->glints[0])-80;st.camy=(s16)(sy/W->glints[0])-50;
        for(i=0;i<4;i++) {
            void *l=rt_light,*d=rt_dark;
            rt_light=phases[i][0];rt_dark=phases[i][1];memset(phases[i],0xFF,sizeof phases[i]);
            st.tick=(u16)(i*8);draw_glints();
            rt_light=l;rt_dark=d;
            assert(W->glints[i]>0&&W->glints[i]<=GLINTS);
        }
        for(i=1;i<4;i++)assert(memcmp(phases[i-1],phases[i],sizeof phases[i]));
    }
    printf("Presentation: all 11 idle actors, %u exact walking poses, portrait masks; max cold panel %u frames; four-view puddle phases passed\n",pictures,max_latency);
    return 0;
}
