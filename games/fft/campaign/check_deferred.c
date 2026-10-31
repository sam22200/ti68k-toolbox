// Same-tick deferred roots: original slot order, one clock phase, no recursion.
#define REACTION_SEQUENCE_CHECK
#include "check_reactions.c"
#include "x/core-deferred/oracle/source.h"
static u8 cells[13];
static void wave(void)
{
    u16 c;u8 i=0;Unit base;
    reset();CHECK(NUNIT,13);base=st.unit[U_RAMZA];
    memset(st.pending,0,sizeof st.pending);memset(st.reaction,0,sizeof st.reaction);
    for(c=0;c<MAP_N&&i<NUNIT;c++)if(map_tiles[c].walk==1)cells[i++]=(u8)c;
    CHECK(i,NUNIT);st.open_ct_phase=st.open_resolving=0;
    for(i=0;i<NUNIT;i++){
        st.unit[i]=base;unit_place(&st.unit[i],cells[i]);
        st.unit[i].hp=st.unit[i].max_hp=999;st.unit[i].dc=3;
        st.unit[i].ct=i==12?50:0;st.unit[i].sp=i==12?50:1;
        st.unit[i].eq[EQ_SHIELD]=255;st.support[i]=0;
        st.unit[i].team=i==U_RAMZA?TEAM_PLAYER:TEAM_ENEMY;
    }
}
static void arm(u8 a,u8 cell,u8 ticks)
{
    st.pending[a].state=DEFER_CHARGING;st.pending[a].ability=0x196;
    st.pending[a].ticks=ticks;st.pending[a].tile=cell;
    st.pending[a].origin=UNIT_CELL(&st.unit[a]);
    st.pending[a].weapon=hand_weapon(&st.unit[a],0);
    st.pending[a].off_weapon=hand_weapon(&st.unit[a],1);
    unit_status_action(&st.unit[a],8);
}
static u32 state_hash(void)
{
    const u8 *p=(const u8 *)&st;u16 n;u32 h=17;
    for(n=0;n<sizeof st;n++)h=(h<<1|h>>31)^*p++;
    return h;
}
static void ties(void)
{
    u8 i,j;u16 jp;u32 hash,seed;
    wave();change(U_RAMZA,SID_POISON);st.unit[U_RAMZA].status_ct[0]=2;
    for(i=0;i<NUNIT;i++)arm(i,cells[i],1);
    jp=st.unit[U_RAMZA].jp;turn_start();
    for(i=0;i<NUNIT;i++){
        CHECK(st.mode,M_ACT);CHECK(st.atk,source_due_order[i]);CHECK(st.tgt,i);
        CHECK(st.action.root,i);CHECK(st.open_ct_phase,1);
        CHECK(st.unit[U_RAMZA].status_ct[0],2);
        for(j=0;j<NUNIT;j++)CHECK(st.unit[j].ct,j==12?50:0);
        // A caller may poll while a root/impact/reaction owns the packet.
        hash=state_hash();seed=rng32;CHECK(charge_resolve(),1);
        CHECK(state_hash()==hash,1);CHECK(rng32==seed,1);
        st.hit=0;st.push=0;impact();
        if(i<12){ZB(33);end_anim();ZE(33);}else end_anim();
    }
    CHECK(st.action.phase,CTX_IDLE);CHECK(st.open_resolving,0);CHECK(st.act,12);
    CHECK(st.open_ct_phase,0);CHECK(st.unit[U_RAMZA].status_ct[0],1);
    CHECK(st.unit[U_RAMZA].jp,jp);
    for(i=0;i<NUNIT;i++){CHECK(st.pending[i].state,DEFER_NONE);CHECK(st.unit[i].ct,i==12?0:1);}
}
static void rejected_wave(void)
{
    u16 c;u8 i;u32 seed;u16 jp;
    wave();for(c=0;c<MAP_N;c++)if(map_tiles[c].walk==1&&!unit_at_cell((u8)c))break;
    CHECK(c<MAP_N,1);change(U_RAMZA,SID_POISON);st.unit[U_RAMZA].status_ct[0]=2;
    for(i=0;i<NUNIT;i++)arm(i,(u8)c,1);
    seed=rng32;jp=st.unit[U_RAMZA].jp;
    ZB(34);turn_start();ZE(34);
    CHECK(st.action.phase,CTX_IDLE);CHECK(st.open_resolving,0);CHECK(st.act,12);
    CHECK(st.open_ct_phase,0);CHECK(st.unit[U_RAMZA].status_ct[0],1);
    CHECK(rng32==seed,1);CHECK(st.unit[U_RAMZA].jp,jp);
    for(i=0;i<NUNIT;i++){CHECK(st.pending[i].state,DEFER_NONE);CHECK(st.unit[i].ct,i==12?0:1);}
    // Rejection at the direct dispatcher never advances CT or changes mode.
    wave();arm(0,(u8)c,0);seed=rng32;CHECK(charge_resolve(),0);
    CHECK(st.pending[0].state,DEFER_NONE);CHECK(st.mode,M_MENU);
    CHECK(st.open_ct_phase,0);CHECK(st.unit[0].ct,0);CHECK(rng32==seed,1);
}
static void cancellation(void)
{
    u8 mode;
    for(mode=0;mode<3;mode++){
        wave();arm(0,cells[1],1);arm(1,cells[0],1);arm(2,cells[2],1);
        turn_start();CHECK(st.atk,0);CHECK(st.pending[1].ticks,0);
        if(mode==0){st.hit=1;st.push=0;st.dmg=999;}
        else{
            st.hit=0;st.push=0;
            if(mode==1)unit_place(&st.unit[1],cells[3]);
            else st.unit[1].eq[EQ_WEAPON]=255;
        }
        impact();end_anim();CHECK(st.atk,2);CHECK(st.open_ct_phase,1);
        CHECK(st.pending[1].state,DEFER_NONE);CHECK(st.unit[12].ct,50);
    }
    // Existing ready ordinary turns precede the next decrement phase.
    wave();arm(1,cells[1],1);st.unit[12].ct=103;turn_start();
    CHECK(st.act,12);CHECK(st.pending[1].ticks,1);CHECK(st.open_ct_phase,0);
    // Stop holds a due record through this wave; expiry occurs afterwards.
    wave();arm(0,cells[0],1);arm(1,cells[1],1);change(0,SID_STOP);
    st.unit[0].status_ct[SID_STOP-SID_POISON]=1;
    turn_start();CHECK(st.atk,1);CHECK(st.pending[0].ticks,1);
    st.hit=0;st.push=0;impact();end_anim();CHECK(st.act,12);
    CHECK(st.pending[0].ticks,1);CHECK(st.unit[0].status_flags&STATUS_CT_FROZEN,0);
    turn_start();CHECK(st.atk,0);CHECK(st.pending[0].state,DEFER_NONE);
}
static void responses(void)
{
    u32 hash;
    // Next due root cannot overtake the current root's Counter presentation.
    arena();unit_place(&st.unit[0],third);st.unit[0].hp=999;st.unit[0].dc=3;
    st.unit[0].team=TEAM_ENEMY;st.unit[0].ct=0;st.unit[0].sp=1;
    st.reaction[10]=REACTION_COUNTER;arm(U_RAMZA,second,1);arm(10,first,1);
    st.unit[U_RAMZA].ct=st.unit[10].ct=0;st.open_ct_phase=0;
    turn_start();CHECK(st.atk,U_RAMZA);st.hit=1;st.push=0;st.dmg=1;
    impact();end_anim();CHECK(st.primary.running,1);CHECK(st.atk,10);
    CHECK(st.pending[10].state,DEFER_CHARGING);hash=state_hash();
    CHECK(charge_resolve(),1);CHECK(state_hash()==hash,1);
    st.hit=0;impact();end_anim();
    if(st.gain)end_anim();CHECK(st.action.root,10);CHECK(st.open_resolving,1);
    CHECK(st.pending[10].state,DEFER_NONE);CHECK(st.open_ct_phase,1);
    // A terminal first root never opens a second due root or another clock tick.
    arena();arm(U_RAMZA,second,1);arm(10,first,1);
    st.unit[U_RAMZA].ct=st.unit[10].ct=0;st.open_ct_phase=0;
    turn_start();st.hit=1;st.push=0;st.dmg=999;impact();end_anim();if(st.gain)end_anim();
    CHECK(st.end,END_VICTORY);CHECK(st.action.phase,CTX_IDLE);
    CHECK(st.atk,U_RAMZA);CHECK(st.open_ct_phase,1);
}
static void deferred_trials(void){ties();rejected_wave();cancellation();responses();}
#ifndef SCHEDULER_CHECK
#ifndef __m68k__
int main(void){sw_init(61);deferred_trials();printf("deferred %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();deferred_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
#endif
