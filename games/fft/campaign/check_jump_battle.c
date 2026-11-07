// Actual battle reach/path through a source column with and without compact nodes.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main jump_battle_unused_main
#define game_render jump_battle_no_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void jump_battle_no_render(void){}
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "battle.c"
static u16 checks,failures;static u32 digest=17;
static void check(u16 value,u16 expected,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^value;
    if(value!=expected){failures++;
#ifndef __m68k__
        printf("line%u: %u expected%u\n",line,value,expected);
#else
        BENCH_VALUE(line);
#endif
    }
}
#define CHECK(v,e) check((u16)(v),(u16)(e),__LINE__)
static void trials(void)
{
    // Dense permutation and sparse topology share the same three SOURCE columns.
    static const u8 dense_x[6]={1,2,0,1,2,0},sparse_x[4]={2,0,0,2},zs[6]={0},row[1]={0};
    static const CoreTilePosition dense_pos[6]={{1,0},{2,0},{0,0},{1,32},{2,32},{0,32}};
    static const CoreTilePosition sparse_pos[4]={{2,0},{0,32},{0,0},{2,32}};
    u8 lookup[512],present[64],bank[32],sparse,source_floor,variant,i;
    Tile tiles[6];CoreGrid grid;
    for(sparse=0;sparse<2;sparse++)for(source_floor=0;source_floor<2;source_floor++) {
        const CoreTilePosition *pos=sparse?sparse_pos:dense_pos;
        const u8 *xs=sparse?sparse_x:dense_x;
        u8 count=sparse?4:6,origin=source_floor?(sparse?1:5):2,target=sparse?0:1;
        memset(lookup,0,sizeof lookup);memset(present,0,sizeof present);
        memset(tiles,0,sizeof tiles);memset(&grid,0,sizeof grid);
        for(i=0;i<count;i++) {
            u16 address=xs[i]+((pos[i].z_layer&32)?256:0);u8 k;
            lookup[address]=i;present[address>>3]|=(u8)(1U<<(address&7));
            tiles[i].stand=6;for(k=0;k<4;k++)tiles[i].c[k]=3;
            tiles[i].walk=(i==origin||i==target);
        }
        grid.count=count;grid.width=3;grid.height=1;grid.position=pos;
        grid.lookup=lookup;grid.present=present;grid.row=row;
        for(variant=0;variant<6;variant++) {
            u8 allowed=variant==0||variant==2||variant==3;
            memset(bank,0,sizeof bank);memcpy(bank,"FGS1",4);bank[4]=3;bank[5]=1;bank[7]=3;
            // Source and destination tops are3, intermediate ground0. Empty
            // upper endpoint records are blocked (not a fictitious zero floor).
            bank[15]=bank[31]=128;bank[8+4*source_floor]=3;
            if(source_floor){bank[11]=128;bank[15]=0;}
            bank[24]=3;
            bank[20]=variant==0?8:variant==1?4:variant==2?6:4;
            bank[23]=variant==3?129:1;
            if(variant==2)bank[23]=0; // bottom exactly touches body's head
            if(variant==4){bank[16]=4;bank[23]=128;} // lower terrain intersects body
            if(variant==5){bank[16]=2;bank[17]=3;bank[18]=0x44;bank[23]=128;} // sloped middle intersects
            CHECK(battle_bind_grid(&grid,tiles,xs,zs),1);
            CHECK(battle_bind_surface(bank,sizeof bank),1);
            for(i=0;i<NUNIT;i++){st.unit[i].hp=0;st.unit[i].dc=DC_GONE;st.unit[i].x=255;}
            unit_place(&st.unit[U_RAMZA],origin);st.unit[U_RAMZA].hp=100;
            st.unit[U_RAMZA].dc=3;st.unit[U_RAMZA].move=2;st.unit[U_RAMZA].jump=2;
            st.act=st.sel=U_RAMZA;st.open_stage=0;st.mode=M_MENU;
            compute_reach(U_RAMZA);CHECK(st.reach[target],allowed?2:255);
            CHECK(make_path_cell(U_RAMZA,target),allowed?2:0);
            if(allowed){CHECK(st.path_n,2);CHECK(st.path[0],origin);CHECK(st.path[1],target);}
            // Jump1 cannot cross the non-enterable intermediate column.
            st.unit[U_RAMZA].jump=1;compute_reach(U_RAMZA);CHECK(st.reach[target],255);
        }
    }
}
#ifndef __m68k__
int main(void){sw_init(261);trials();printf("jumpbattle %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){game_init();game_scenario(261);trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
