// Actual recoil caller: layer selection, occupancy, hit application and feet cache.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main recoil_battle_unused_main
#define game_render recoil_no_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void recoil_no_render(void){}
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
    for(variant=0;variant<15;variant++) {
        u8 origin=variant==1||variant==7?4:1,destination=5,push=1,damage=10,count=variant==12?3:6;
        u16 feet_before;
        memset(lookup,0,sizeof lookup);memset(present,0,sizeof present);memset(&grid,0,sizeof grid);
        memset(bank,0,sizeof bank);memcpy(bank,"FGS1",4);bank[4]=3;bank[5]=1;bank[7]=3;
        for(i=0;i<3;i++)bank[15+8*i]=128;
        bank[8+8]=4;bank[24]=1;bank[28]=3;bank[31]=0;
        if(origin==4){bank[20]=4;bank[23]=0;bank[19]=128;}
        if(variant==2||variant==3||variant==4||variant==5||variant==9||variant==10||variant==11||variant==13||variant==14)push=0;
        if(variant==4){bank[24]=5;bank[28]=6;}
        if(variant==5||variant==6){bank[16]=3;bank[24]=0;bank[28]=6;bank[31]=variant==5?3:2;destination=2;damage=20;}
        if(variant==7||variant==8||variant==12){bank[31]=128;destination=2;damage=20;}
        if(variant==8)bank[25]=32;
        if(variant==10)damage=100;
        if(variant==13){bank[16]=2;bank[17]=3;bank[18]=0xe4;bank[24]=3;bank[31]=128;}
        memset(tiles,0,sizeof tiles);
        for(i=0;i<count;i++) {
            const u8 *floor=bank+8+8*xs[i]+(i>=3?4:0);u16 address=xs[i]+(i>=3?256:0);u8 k;
            lookup[address]=i;present[address>>3]|=(u8)(1U<<(address&7));
            tiles[i].stand=2*floor[0];tiles[i].water=floor[1]>>5;tiles[i].walk=!(floor[3]&128);
            for(k=0;k<4;k++)tiles[i].c[k]=floor[0];
        }
        if(variant==3)tiles[5].walk=0; // selected upper is untargetable: no lower fallback
        if(variant==13)tiles[origin].stand=13; // source edge, not center/max stand, decides
        grid.count=count;grid.width=3;grid.height=1;grid.position=pos;grid.lookup=lookup;grid.present=present;grid.row=row;
        CHECK(battle_bind_grid(&grid,tiles,xs,zs),1);CHECK(battle_bind_surface(bank,sizeof bank),1);
        for(i=0;i<NUNIT;i++){st.unit[i].x=255;st.unit[i].hp=0;st.unit[i].dc=DC_GONE;}
        unit_place(&st.unit[0],0);unit_place(&st.unit[1],origin);
        for(i=0;i<2;i++){st.unit[i].hp=st.unit[i].max_hp=100;st.unit[i].dc=3;st.unit[i].jump=2;}
        if(variant==2||variant==14){unit_place(&st.unit[2],5);st.unit[2].hp=variant==14?0:100;st.unit[2].dc=3;}
        if(variant==9){unit_place(&st.unit[0],1);unit_place(&st.unit[1],2);}
        if(variant==11)unit_place(&st.unit[0],origin==4?1:4);
        if(variant==12)CHECK(W->surface_flags&SURFACE_ROOM,0);
        st.atk=st.act=0;st.tgt=st.sel=1;st.abil=NO_ABIL;st.dmg=variant==10?100:10;
        // Each variant is an independent root action (CORE_CONTEXT lifecycle).
        memset(&st.action,0,sizeof st.action);CHECK(core_action_begin(&st.action,0,NUNIT),CTX_OK);
        st.push=st.self=st.gain=0;st.hit=1;st.act_t=ACT_HIT;st.mode=M_ACT;
        feet_before=unit_feet_key(1,0);W->feet_k[1]=feet_before;
        knockback(0,1);CHECK(st.push,push);
        if(push){CHECK(st.push_cell,destination);CHECK(st.dmg,damage);attack_step();
            CHECK(UNIT_CELL(&st.unit[1]),destination);CHECK(st.unit[1].hp,100-damage);
            CHECK(unit_feet_key(1,0)!=feet_before,1);CHECK(W->feet_k[1]!=unit_feet_key(1,0),1);
        }
    }
}
#ifndef __m68k__
int main(void){sw_init(261);trials();printf("recoilbattle %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){game_init();game_scenario(261);trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
