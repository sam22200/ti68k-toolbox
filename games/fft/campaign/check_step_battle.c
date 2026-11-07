// Affected callers only: edge construction, reach and canonical path extraction.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main steps_battle_unused_main
#define game_render steps_no_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void steps_no_render(void){}
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "battle.c"
static u16 checks,failures;static u32 digest=17;
static void equal(u16 a,u16 b,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^a;if(a!=b){failures++;
#ifndef __m68k__
        printf("line%u: %u expected%u\n",line,a,b);
#else
        BENCH_VALUE(line);
#endif
    }
}
#define CHECK(a,b) equal((u16)(a),(u16)(b),__LINE__)
static void trials(void)
{
    static const u8 xs[6]={0,1,2,0,1,2},zs[6]={0},row[1]={0};
    static const CoreTilePosition pos[6]={{0,0},{1,0},{2,0},{0,32},{1,32},{2,32}};
    u8 lookup[512],present[64],bank[32],variant,i;Tile tiles[6];CoreGrid grid;
    for(variant=0;variant<16;variant++) {
        u8 origin=0,target=1,jump=2,expected=1,count=3,dst=16;
        memset(bank,0,sizeof bank);memcpy(bank,"FGS1",4);bank[4]=3;bank[5]=1;bank[7]=3;
        for(i=0;i<3;i++)bank[15+8*i]=128;bank[27]=128;
        bank[8]=bank[16]=2;
        if(variant==1){bank[8]=1;bank[9]=3;bank[10]=0xe4;bank[16]=2;jump=1;}
        if(variant==2){bank[9]=3;bank[10]=1;jump=5;expected=255;}
        if(variant==3){bank[17]=3;bank[18]=1;jump=5;expected=255;}
        if(variant==4){bank[9]=3;bank[10]=0x10;expected=255;}
        if(variant==5){bank[17]=3;bank[18]=0x10;}
        if(variant==6||variant==7){bank[8]=0;bank[16]=2;bank[20]=variant==6?5:6;bank[23]=1;jump=3;expected=variant==6?255:1;}
        if(variant==8){bank[12]=3;bank[15]=0;bank[8]=0;origin=3;}
        if(variant==9){bank[16]=0;bank[20]=3;bank[23]=0;target=4;}
        if(variant==10){bank[8]=0;bank[9]=64;bank[16]=4;jump=3;expected=255;}
        if(variant==11){bank[8]=0;bank[16]=0;bank[17]=64;jump=1;expected=255;}
        if(variant==12){bank[8]=0;bank[16]=0;bank[17]=32;jump=1;expected=1;}
        if(variant>=13){dst=24;target=2;bank[19]=128;bank[27]=0;bank[8]=3;bank[24]=1;expected=2;}
        if(variant==14){bank[8]=1;bank[24]=2;expected=255;}
        if(variant==15){bank[8]=bank[24]=2;jump=1;expected=255;}
        for(i=0;i<3;i++)if(!(bank[15+8*i]&128))count=6;
        memset(lookup,0,sizeof lookup);memset(present,0,sizeof present);memset(tiles,0,sizeof tiles);memset(&grid,0,sizeof grid);
        for(i=0;i<count;i++) {
            const u8 *floor=bank+8+8*xs[i]+(i>=3?4:0);u16 address=xs[i]+(i>=3?256:0);u8 k;
            lookup[address]=i;present[address>>3]|=(u8)(1U<<(address&7));
            tiles[i].stand=2*floor[0];tiles[i].water=floor[1]>>5;tiles[i].walk=!(floor[3]&128);
            for(k=0;k<4;k++)tiles[i].c[k]=floor[0];
        }
        if(variant==1)tiles[0].stand=11; // cached center cannot override the source-facing edge
        grid.count=count;grid.width=3;grid.height=1;grid.position=pos;grid.lookup=lookup;grid.present=present;grid.row=row;
        CHECK(battle_bind_grid(&grid,tiles,xs,zs),1);CHECK(battle_bind_surface(bank,sizeof bank),1);
        for(i=0;i<NUNIT;i++){st.unit[i].x=255;st.unit[i].hp=0;st.unit[i].dc=DC_GONE;}
        unit_place(&st.unit[0],origin);st.unit[0].hp=100;st.unit[0].dc=3;st.unit[0].jump=jump;st.unit[0].move=3;
        st.act=st.sel=0;st.mode=M_MENU;st.open_stage=0;compute_reach(0);
        CHECK(st.reach[target],expected);CHECK(make_path_cell(0,target),expected==255?0:2);
        if(expected!=255){CHECK(st.path[0],origin);CHECK(st.path[1],target);}
        (void)dst;
    }
}
#ifndef __m68k__
int main(void){sw_init(261);trials();printf("stepbattle %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){game_init();game_scenario(261);trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
