// Jump timing/damage oracle, shared deferred owner and simplified layer policy.
#define REACTION_SEQUENCE_CHECK
#include "check_reactions.c"
#include "x/core-lancer-jump/oracle/source.h"
static void ready(void)
{
    arena();st.moved=st.acted=0;st.open_ct_phase=st.open_resolving=0;
    st.unit[U_RAMZA].set=18;st.unit[U_RAMZA].learned=0xffff;
    st.unit[U_RAMZA].sp=6;st.unit[U_RAMZA].mp=0;
    st.reaction[10]=0;
}
static void source_jump(void)
{
    u16 i;u8 k;Unit *a,*t;
    for(i=0;i<sizeof jump_times/sizeof jump_times[0];i++){
        ready();a=&st.unit[U_RAMZA];a->sp=jump_times[i][0];
        ZB(61);skill(U_RAMZA,10,JUMP_FIRST);ZE(61);
        CHECK(st.pending[U_RAMZA].ticks,jump_times[i][1]);
        CHECK(st.pending[U_RAMZA].ability,jump_times[i][3]);
        CHECK(st.pending[U_RAMZA].state,DEFER_AIRBORNE);
        CHECK(st.moved,1);CHECK(st.acted,1);CHECK(a->mp,0);
    }
    for(i=0;i<sizeof jump_damage_rows/sizeof jump_damage_rows[0];i++){
        const u16 *p=jump_damage_rows[i];u32 seed;
        ready();a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->pa=p[0];a->brave=p[1];a->eq[EQ_WEAPON]=p[2]?p[2]:255;
        a->zodiac=p[6];t->zodiac=p[7];a->female=0;t->female=p[8];
        memset(&a->statuses,0,sizeof a->statuses);memset(&t->statuses,0,sizeof t->statuses);
        for(k=0;k<2;k++)if(p[3+k]!=255){u8 sid=p[3+k];(k?t:a)->statuses.inflicted[sid>>3]|=0x80>>(sid&7);}
        unit_status_refresh(a);unit_status_refresh(t);st.support[10]=p[5]?SUPPORT_DEFENSE_UP:0;
        seed=rng32;ZB(62);CHECK(skill_amount(U_RAMZA,10,JUMP_FIRST,0),p[9]>999?999:p[9]);ZE(62);
        CHECK(rng32==seed,1);
    }
}
static void reach_and_learning(void)
{
    u8 i;u16 limits;
    ready();st.unit[U_RAMZA].learned=0;CHECK(jump_limits(U_RAMZA),0x0201);
    CHECK(known(U_RAMZA,255),1);CHECK(known(U_RAMZA,0),JUMP_FIRST);CHECK(known(U_RAMZA,1),NO_ABIL);
    for(i=0;i<12;i++){
        ready();CHECK(SET_ABILITY(18,i),JUMP_FIRST+i);
        st.unit[U_RAMZA].learned=(u16)1<<i;limits=jump_limits(U_RAMZA);
        CHECK((u8)limits,abilities[JUMP_FIRST+i].range>1?abilities[JUMP_FIRST+i].range:1);
        CHECK(limits>>8,abilities[JUMP_FIRST+i].vert>2?abilities[JUMP_FIRST+i].vert:2);
        CHECK(can_use(U_RAMZA,JUMP_FIRST+i),1);
    }
    ready();st.unit[U_RAMZA].learned=0;CHECK(jump_reaches(U_RAMZA,first,third),0);
    st.unit[U_RAMZA].learned=16;CHECK(jump_reaches(U_RAMZA,first,third),1);
    {CoreGrid grid;CoreTilePosition positions[MAP_NMAX];const CoreGrid *saved=W->grid;
     memset(&grid,0,sizeof grid);memset(positions,0,sizeof positions);
     positions[second].z_layer=32;grid.position=positions;W->grid=&grid;
     CHECK(jump_reaches(U_RAMZA,first,second),0);W->grid=saved;}
    ready();CHECK(jump_limits(U_RAMZA),0x1008);
    skill_range(U_RAMZA,JUMP_FIRST);CHECK(st.reach[first],255);CHECK(st.reach[second],1);
    CHECK(jump_reaches(U_RAMZA,first,second),1);
    CHECK(jump_reaches(U_RAMZA,first,first),0);
    st.unit[U_RAMZA].job=SOURCE_JOB_MIME;st.unit[U_RAMZA].learned=0;CHECK(jump_limits(U_RAMZA),0x1008);
}
static void delayed(void)
{
    Unit *a,*t;u16 hp,jp,amount;u8 i;u32 seed;
    ready();a=&st.unit[U_RAMZA];t=&st.unit[10];a->eq[EQ_WEAPON]=99;a->pa=5;a->zodiac=t->zodiac=0;
    hp=t->hp;jp=a->jp;seed=rng32;skill(U_RAMZA,10,JUMP_FIRST);
    CHECK(unit_airborne(a)!=0,1);CHECK(unit_can_act(a),0);CHECK(unit_can_move(a),0);
    CHECK(unit_at_cell(first),U_RAMZA+1);
    st.abil=NO_ABIL;CHECK(aim_ok(U_RAMZA),0);st.abil=JUMP_FIRST;CHECK(can_use(U_RAMZA,JUMP_FIRST),0);
    CHECK(hit_chance(10,U_RAMZA),0);CHECK(skill_hit(10,U_RAMZA,0x94),0);
    CHECK(t->hp,hp);CHECK(a->jp,jp);CHECK(rng32==seed,1);
    turn_order();for(i=1;i<ORDER_N;i++)CHECK(st.turns[i]==U_RAMZA,0);
    // Jump does not follow a unit: another occupant at the old tile is hit.
    unit_place(t,third);unit_place(&st.unit[1],second);st.unit[1]=*t;unit_place(&st.unit[1],second);
    st.unit[1].hp=999;st.unit[1].dc=3;st.reaction[1]=0;
    st.pending[U_RAMZA].ticks=0;ZB(63);CHECK(charge_resolve(),1);ZE(63);
    CHECK(st.tgt,1);CHECK(st.pending[U_RAMZA].state,DEFER_NONE);CHECK(unit_airborne(a),0);
    CHECK(unit_at_cell(first),U_RAMZA+1);CHECK(st.hit,1);CHECK(st.crit,0);CHECK(st.push,0);
    amount=st.dmg;CHECK(amount,56);CHECK(rng32==seed,1);
    impact();CHECK(st.unit[1].hp,999-amount);CHECK(t->hp,hp);hp=st.unit[1].hp;jp=a->jp;
    impact();CHECK(st.unit[1].hp,hp);CHECK(a->jp,jp);
    // Empty landing resolves without rewards, damage or an endless due slot.
    ready();skill(U_RAMZA,10,JUMP_FIRST);unit_place(&st.unit[10],third);jp=st.unit[U_RAMZA].jp;
    st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),0);CHECK(st.action.phase,CTX_IDLE);
    CHECK(st.pending[U_RAMZA].state,DEFER_NONE);CHECK(unit_airborne(&st.unit[U_RAMZA]),0);
    CHECK(st.unit[U_RAMZA].jp,jp);CHECK(pending_next(),255);
    // Shared CT clock: the separate Jump timer progresses, actor CT stays fixed.
    ready();a=&st.unit[U_RAMZA];a->ct=255;st.unit[10].sp=1;st.unit[10].ct=0;
    skill(U_RAMZA,10,JUMP_FIRST);st.pending[U_RAMZA].ticks=2;
    CHECK(ct_next(),CT_DUE);CHECK(a->ct,255);CHECK(st.pending[U_RAMZA].ticks,0);
    CHECK(charge_resolve(),1);CHECK(st.atk,U_RAMZA);
    // KO cancels flight through the existing lifecycle, never a stale due strike.
    ready();skill(U_RAMZA,10,JUMP_FIRST);a=&st.unit[U_RAMZA];a->hp=0;pending_next();
    CHECK(st.pending[U_RAMZA].state,DEFER_NONE);CHECK(unit_airborne(a),0);CHECK(pending_next(),255);
}
static void flight_collisions(void)
{
    u16 hp;
    ready();skill(U_RAMZA,10,JUMP_FIRST);
    st.unit[1]=st.unit[10];unit_place(&st.unit[1],third);st.reaction[1]=0;
    // A physical recoil cannot place a ground unit on the reserved origin.
    st.push=0;knockback(1,10);CHECK(st.push,0);CHECK(unit_at_cell(first),U_RAMZA+1);
    // Legacy weapon-skill splashes also honor airborne immunity.
    hp=st.unit[U_RAMZA].hp;act_start(1,10);st.abil=0x9b;st.hit=1;st.dmg=7;st.push=0;
    impact();CHECK(st.unit[U_RAMZA].hp,hp);CHECK(st.unit[10].hp,992);
    CHECK(st.pending[U_RAMZA].state,DEFER_AIRBORNE);
}
static void ai_jump(void)
{
    u16 steps=0,hp;u8 mp;
    ready();unit_place(&st.unit[10],third);st.unit[U_RAMZA].eq[EQ_WEAPON]=99;
    st.moved=1;W->ai_k=0;hp=st.unit[10].hp;mp=st.unit[U_RAMZA].mp;
    while(!ai_think()&&++steps<1024){}
    CHECK(steps<1024,1);CHECK(st.ai_ab,JUMP_FIRST);CHECK(st.ai_tgt,third);
    CHECK(st.unit[10].hp,hp);CHECK(st.unit[U_RAMZA].mp,mp);
}
static void guards(void)
{
    Unit *a;u32 seed;u16 cell;
    ready();a=&st.unit[U_RAMZA];a->sp=0;skill(U_RAMZA,10,JUMP_FIRST);
    CHECK(st.pending[U_RAMZA].ticks,50);
    ready();a=&st.unit[U_RAMZA];a->set=SET_BASIC;seed=rng32;skill(U_RAMZA,10,JUMP_FIRST);
    CHECK(st.pending[U_RAMZA].state,DEFER_NONE);CHECK(st.acted,0);CHECK(rng32==seed,1);
    ready();change(U_RAMZA,SID_FROG);skill(U_RAMZA,10,JUMP_FIRST);CHECK(st.pending[U_RAMZA].state,DEFER_NONE);
    ready();change(U_RAMZA,SID_SILENCE);st.support[U_RAMZA]=SUPPORT_SHORT_CHARGE;
    skill(U_RAMZA,10,JUMP_FIRST);CHECK(st.pending[U_RAMZA].ticks,8);
    ready();st.support[U_RAMZA]=SUPPORT_NON_CHARGE;skill(U_RAMZA,10,JUMP_FIRST);CHECK(st.pending[U_RAMZA].ticks,8);
    // Invalid new targeting cannot cancel a previously armed Jump.
    ready();skill(U_RAMZA,10,JUMP_FIRST);seed=rng32;skill(U_RAMZA,10,JUMP_FIRST);
    CHECK(st.pending[U_RAMZA].state,DEFER_AIRBORNE);CHECK(rng32==seed,1);
    ready();for(cell=0;cell<MAP_N;cell++)if(map_tiles[cell].walk&&node_layer((u8)cell)!=node_layer(first)){
        CHECK(jump_reaches(U_RAMZA,first,(u8)cell),0);break;
    }
}
static void jump_trials(void){source_jump();reach_and_learning();delayed();flight_collisions();ai_jump();guards();}
#ifndef JUMP_UI_CHECK
#ifndef __m68k__
int main(void){sw_init(61);jump_trials();printf("lancer_jump %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();jump_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif

#endif
