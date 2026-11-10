// Real update/render transitions: a hidden selected tile must be dashed on frame1.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main cursor_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#endif
#include "battle.c"
#ifndef __m68k__
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
static const char *output;
#endif
static u16 failures,checks;
static u32 digest=17;
#define CHECK(c) do{checks++;if(!(c))failures++;}while(0)
static void shot(u8 rot,u8 kind,u8 ai,u8 phase)
{
    u8 p,x;u16 y;
    for(p=0;p<2;p++)for(y=0;y<100;y++)for(x=0;x<20;x++)
        digest=(digest^(digest>>13))+(digest<<5)+((const u8 *)(p?rt_dark:rt_light))[y*RT_PBYTES+x];
#ifndef __m68k__
    if(output&&rot==0&&kind==2){char path[512];snprintf(path,sizeof path,"%s/cursor-%u-%u.png",output,ai,phase);sw_write_png(path,1);}
#else
    (void)rot;(void)kind;(void)ai;(void)phase;
#endif
}
static void trial(void)
{
    static const s8 dx[4]={-1,1,0,0},dy[4]={0,0,-1,1};
    static const u32 keys[4]={K_LEFT,K_RIGHT,K_UP,K_DOWN};
    u8 rot,kind,ai;
    for(rot=0;rot<4;rot++)for(kind=0;kind<4;kind++)for(ai=0;ai<2;ai++) {
        u8 c,best=255,bh=0,dir=0,startx=0,startz=0,i,r;
        Marker mk;
        game_scenario(101);st.rot=rot;st.turn=st.banner=st.ai=0;st.want=255;
        st.act=st.sel=U_RAMZA;st.tick=8;st.mode=kind?M_AIM:M_TARGET;
        st.abil=kind==2?0x94:kind==3?0x196:NO_ABIL;
        ensure_scene(rot);
        for(c=0;c<MAP_N;c++)if(map_tiles[c].walk&&!unit_at(c%MAP_W,c/MAP_W)) {
            const PlaneWord *mask;u8 hidden=0,j;
            marker(rot,c,0,&mk);W->cur_k=0;mask=cursor_mask(&mk,(u16)(c|0x1000));
            for(r=0;r<mk.rows;r++)hidden+=(mask[r]!=mk.m[r]);
            if(hidden<=bh)continue;
            for(j=0;j<4;j++) {
                cursor_place((u8)c);move_cursor(-dx[j],-dy[j]);
                if(st.cx==c%MAP_W&&st.cz==c/MAP_W)continue;
                if(!map_tiles[st.cz*MAP_W+st.cx].walk)continue;
                best=c;bh=hidden;dir=j;startx=st.cx;startz=st.cz;break;
            }
        }
        CHECK(best!=255);if(best==255)continue;
        for(c=0;c<MAP_N;c++)st.reach[c]=map_tiles[c].walk?1:255;
        cursor_place((u8)best);cam_target(&st.camx,&st.camy);
        cursor_place((u8)node_at(startx,startz,0));hl_seq++;
        for(i=0;i<24;i++){W->actor_budget=255;W->ac_now++;game_render();}
        shot(rot,kind,ai,0);
        st.ai=ai?(kind?AI_AIM:AI_MOVE):0;st.ai_o=st.ai_tgt=best;st.ai_ab=st.abil;
        st.ai_t=AI_GLIDE-1;W->hold=W->frz=0;rt_prev=0;rt_keys=ai?0:keys[dir];
        CHECK(game_update());game_render();
        CHECK(st.cx==best%MAP_W&&st.cz==best/MAP_W);
        CHECK(W->cur_k==(u16)(best|rot<<8|0x9000));
        marker(rot,best,0,&mk);
        for(r=0;r<mk.rows;r++) {
            u8 col;s16 y=mk.y+r;u32 expected=mk.m[r];
            for(col=0;col<24;col++) {
                s16 x=mk.x+col;
                if((u16)y<SC_H&&(u16)x<SC_W) {
                    u8 bit=(u8)(0x80>>(x&7));
                    if(((depth_byte(0,y,(u16)(x>>3))>mk.depth&&(depth_byte(1,y,(u16)(x>>3))&bit))
                       ||(depth_byte(2,y,(u16)(x>>3))>mk.depth&&(depth_byte(3,y,(u16)(x>>3))&bit)))
                       &&((y&1?0xCC:0x33)&bit))expected&=~(0x80000000UL>>col);
                }
            }
            CHECK(W->cur_m[r]==expected);
        }
        shot(rot,kind,ai,1);
        rt_prev=rt_keys;rt_keys=0;
        for(i=0;i<8;i++){CHECK(game_update());game_render();rt_prev=0;}
        CHECK(W->cur_k==(u16)(best|rot<<8|0x9000));shot(rot,kind,ai,2);
    }
}
#ifndef __m68k__
int main(int argc,char **argv){if(argc>1)output=argv[1];sw_init(101);trial();printf("cursor %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){cursor_runtime_main();trial();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
