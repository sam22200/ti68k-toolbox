// Consecutive primary impacts share one root and settle responses/rewards once.
#define REACTION_SEQUENCE_CHECK
#include "check_reactions.c"
#include "x/core-strikes/oracle/source.h"
static void begin_pair(void)
{
    CHECK(attack_sequence(U_RAMZA,10,2),1);
    CHECK(st.action.left,1);CHECK(st.action.cell,second);
    st.hit=1;st.crit=st.push=0;st.dmg=4;
}
static void pair_order(u16 reaction)
{
    u16 hp,jp,stock;u8 i;
    arena();st.unit[10].team=TEAM_PLAYER;st.unit[10].hp=100;
    st.reaction[10]=reaction;st.bag[0]=2;st.bag[1]=st.bag[2]=0;
    begin_pair();hp=st.unit[10].hp;jp=st.unit[U_RAMZA].jp;impact();
    CHECK(st.unit[10].hp,hp-4);CHECK(st.action.pending,(u16)1<<10);
    CHECK(st.gain,0);CHECK(st.unit[U_RAMZA].jp,jp);
    CHECK(core_action_close(&st.action),0);
    CHECK(core_action_take(&st.action,0x1fff),255);
    CHECK(st.action.phase,CTX_IMPACT);
    // The first response stays pending; the second primary animation starts first.
    ZB(28);end_anim();ZE(28);
    CHECK(st.atk,U_RAMZA);CHECK(st.tgt,10);CHECK(st.action.phase,CTX_ROOT);
    CHECK(st.action.left,0);CHECK(st.primary.running,0);CHECK(st.bag[0],2);
    st.hit=1;st.dmg=7;st.push=0;impact();CHECK(st.unit[10].hp,hp-11);
    hp=st.unit[10].hp;impact();CHECK(st.unit[10].hp,hp);
    CHECK(st.gain,0);end_anim();CHECK(st.atk,10);CHECK(st.action.phase,CTX_SECONDARY);
    CHECK(st.primary.running,1);stock=st.bag[0];impact();
    CHECK(st.action.pending,0);CHECK(st.gain,0);
    if(reaction==REACTION_POTION){CHECK(stock,1);CHECK(st.unit[10].hp,hp+30);}
    end_anim();CHECK(st.atk,U_RAMZA);CHECK(st.tgt,10);CHECK(st.dmg,7);
    CHECK(st.action.phase,CTX_DRAIN);CHECK(st.unit[U_RAMZA].jp>jp,1);
    jp=st.unit[U_RAMZA].jp;end_anim();CHECK(st.action.phase,CTX_IDLE);CHECK(st.unit[U_RAMZA].jp,jp);
    for(i=0;i<3;i++)CHECK(st.bag[i],i?0:stock);
}
static void best_exp(void)
{
    u16 i;
    for(i=0;i<sizeof source_strike_exp/sizeof source_strike_exp[0];i++){
        const u16 *p=source_strike_exp[i];Unit *a,*t;
        arena();a=&st.unit[U_RAMZA];t=&st.unit[10];a->level=p[0];t->level=p[1];
        begin_pair();st.hit=p[3]!=0;st.dmg=0;impact();end_anim();
        t->level=p[2];st.hit=p[4]!=0;st.push=0;
        st.dmg=p[4]==2?t->hp:0;if(p[4]==2)t->kos=p[6]-1;
        impact();CHECK(st.primary.reward?st.primary.reward:st.gain,p[7]);
        end_anim();CHECK(st.gain,p[7]);CHECK(a->gexp,p[7]);
        end_anim();CHECK(st.action.phase,CTX_IDLE);CHECK(a->gexp,p[7]);
    }
}
static void rejected_and_empty(void)
{
    u32 seed;u16 hp,jp;u8 i;
    arena();seed=rt_seed;st.pending[U_RAMZA].state=DEFER_CHARGING;
    CHECK(attack_sequence(U_RAMZA,10,0),0);CHECK(attack_sequence(U_RAMZA,10,3),0);
    CHECK(st.pending[U_RAMZA].state,DEFER_CHARGING);CHECK(rt_seed==seed,1);CHECK(st.action.phase,CTX_IDLE);
    for(i=0;i<4;i++){
        arena();st.reaction[10]=REACTION_COUNTER;begin_pair();jp=st.unit[U_RAMZA].jp;
        if(i==0){st.push=1;st.push_cell=third;}
        if(i==1)st.dmg=st.unit[10].hp;
        impact();hp=st.unit[10].hp;
        if(i==2)change(U_RAMZA,SID_DONT_ACT);
        if(i==3)st.unit[U_RAMZA].hp=0;
        end_anim();CHECK(st.action.left,0);CHECK(st.unit[10].hp,hp);
        CHECK(st.action.phase==CTX_ROOT,0);CHECK(st.atk, i==2?10:U_RAMZA);
        if(i==2){st.hit=0;impact();end_anim();}
        if(st.gain)end_anim();CHECK(st.action.phase,CTX_IDLE);
        CHECK(st.unit[U_RAMZA].jp>=jp,1);
    }
    // An empty fixed tile is not a reason to follow the displaced unit.
    arena();begin_pair();st.push=1;st.push_cell=third;impact();end_anim();
    if(st.gain)end_anim();
    CHECK(st.action.phase,CTX_IDLE);CHECK(UNIT_CELL(&st.unit[10]),third);
}
static void misses(void)
{
    u32 seed;u16 hp;
    arena();st.reaction[10]=REACTION_COUNTER;begin_pair();hp=st.unit[10].hp;
    st.hit=0;impact();CHECK(st.action.pending,(u16)1<<10);end_anim();
    CHECK(st.atk,U_RAMZA);st.hit=0;seed=rt_seed;impact();
    CHECK(rt_seed==seed,1);CHECK(st.unit[10].hp,hp);CHECK(st.primary.reward,0);
    end_anim();CHECK(st.atk,10);st.hit=0;impact();end_anim();
    CHECK(st.action.phase,CTX_IDLE);CHECK(st.unit[U_RAMZA].gexp,0);
}
static void fresh_target(void)
{
    u16 hp;
    arena();begin_pair();st.push=1;st.push_cell=third;impact();
    unit_place(&st.unit[12],second);st.unit[12].hp=st.unit[12].max_hp=999;st.unit[12].dc=3;
    hp=st.unit[12].hp;end_anim();CHECK(st.tgt,12);CHECK(st.action.phase,CTX_ROOT);
    st.hit=1;st.dmg=4;st.push=0;impact();CHECK(st.unit[12].hp,hp-4);
    end_anim();end_anim();CHECK(st.action.phase,CTX_IDLE);
}
static void ranged_pair(void)
{
    u8 blocked;u16 c,w,hp;const u8 *source=RT_NULL;
    for(blocked=0;blocked<2;blocked++){
        arena();
        for(w=0;w<255;w++)if(items[w].wp&&items[w].reach>=4)break;
        CHECK(w<255,1);
        for(c=0;c+3<MAP_N;c++)if(TILE_Z[c]==TILE_Z[c+3]&&
            map_tiles[c].walk==1&&map_tiles[c+1].walk==1&&map_tiles[c+2].walk==1&&map_tiles[c+3].walk==1&&
            map_tiles[c].stand==map_tiles[c+1].stand&&map_tiles[c].stand==map_tiles[c+2].stand&&map_tiles[c].stand==map_tiles[c+3].stand)break;
        CHECK(c+3<MAP_N,1);unit_place(&st.unit[U_RAMZA],c);unit_place(&st.unit[10],c+3);
        st.unit[U_RAMZA].eq[EQ_WEAPON]=w;CHECK(attack_sequence(U_RAMZA,10,2),1);
        st.hit=1;st.dmg=1;st.push=0;impact();hp=st.unit[10].hp;
        if(blocked)source=solid_panel(c+1);
        end_anim();CHECK(st.action.phase,blocked?CTX_IMPACT:CTX_ROOT);CHECK(st.action.left,0);
        if(!blocked){st.hit=1;st.dmg=1;st.push=0;impact();CHECK(st.unit[10].hp,hp-1);end_anim();}
        else CHECK(st.unit[10].hp,hp);
        end_anim();CHECK(st.action.phase,CTX_IDLE);
        if(blocked)solid_restore(source);
    }
}
static void trials_strikes(void){pair_order(REACTION_COUNTER);pair_order(REACTION_POTION);best_exp();rejected_and_empty();misses();fresh_target();ranged_pair();}
#ifndef __m68k__
int main(void){sw_init(61);trials_strikes();printf("strikes %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();trials_strikes();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
