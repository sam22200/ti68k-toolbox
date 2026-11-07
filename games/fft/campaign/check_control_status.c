// Frog, Berserk, Confusion and Foxbird: production casts, source formula oracle, physical
// modifiers, Frog commands/reach and lost-control AI policies (CORE_CONTROL_STATUS.md).
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#include "x/core-control-status/oracle/source.h"
static const AbId controls[]={29,53,55};
static const u8 control_bits[][2]={{2,2},{2,8},{1,16}};   // status byte, mask per spell
static void prepare_control(AbId ab)
{
    prepare_cleanse(ab);st.unit[10].team=TEAM_ENEMY;st.unit[10].ct=10;
}
// The source keeps a Frog toggle in its removal bytes (packed 0 here); the
// prepared native packet carries the requested Frog bit, removed on commit.
static u16 toggle(const u16 *p,u16 packed){return p[0]==29&&p[10]==2&&p[14]==AB_STATUS?0x202:packed;}
static void profiles(void)
{
    u16 i,c;u8 k;u32 expected;CastBatch b;CastVictim v;
    for(i=0;i<sizeof source_control/sizeof source_control[0];i++){
        const u16 *p=source_control[i];AbId ab=p[0];Unit *a,*t;const u8 *r;
        prepare_control(ab);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->ma=p[1];a->faith=p[2];t->faith=p[3];st.support[U_RAMZA]=p[4]?SUPPORT_MAGIC_ATTACK_UP:0;
        st.support[10]=p[5]?SUPPORT_MAGIC_DEFENSE_UP:0;if(p[6])t->statuses.inflicted[3]|=16;
        a->zodiac=p[7];t->zodiac=p[8];t->female=p[9];t->eq[EQ_ACC]=p[11]?232:255;
        r=cj_section(common,CJ_STATUS)+6*cj_range(common,ab)[11];
        if(p[10]==1)memcpy(t->statuses.immune,r+1,5);
        if(p[10]==2||p[10]==3)memcpy(t->statuses.inflicted,r+1,5);
        if(p[10]==3)memcpy(t->statuses.innate,r+1,5);
        if(p[10]==4)t->statuses.inflicted[0]|=32;
        if(p[10]==5)t->statuses.inflicted[1]|=128;
        if(p[10]==6)t->statuses.inflicted[3]|=1;
        if(p[10]==7)t->statuses.inflicted[0]|=16;
        if(p[10]==8)t->status_flags|=STATUS_IMMORTAL;
        if(p[10]==9){t->statuses.inflicted[0]|=8;t->statuses.inflicted[3]|=8;t->statuses.inflicted[4]|=16;}
        unit_status_refresh(t);
        for(c=0;c<MAP_N;c++)if(abs((s16)TILE_X[c]-t->x)+abs((s16)TILE_Z[c]-t->z)>4)break;
        CHECK(c<MAP_N,1);unit_place(a,(u8)c);memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=10;b.follow=1;b.ability=ab;
        rng32=1;CHECK(cast_forecast(&b),1);v=b.victim[10];
        CHECK(v.hit,p[12]?p[13]:0);CHECK(v.effect,p[14]);CHECK(v.amount,toggle(p,p[15]));CHECK(rng32==1,1);
        rng32=1;for(k=0;k<p[21];k++)rand15();expected=rng32;rng32=1;
        b.phase=CAST_BATCH_EMPTY;CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);v=b.victim[10];
        CHECK(v.hit,p[17]);CHECK(v.effect,p[19]);CHECK(v.amount,toggle(p,p[20]));CHECK(rng32==expected,1);
    }
}
// Each cast lands its source bit once; a second Frog toggles the Frog back off.
static void control_production(void)
{
    u8 j,k,again;Unit *t;
    for(j=0;j<3;j++)for(again=0;again<2;again++){
        AbId ab=controls[j];prepare_control(ab);t=&st.unit[10];st.unit[U_RAMZA].ma=99;
        st.unit[U_RAMZA].faith=t->faith=100;
        if(again)t->statuses.inflicted[control_bits[j][0]]|=control_bits[j][1];
        unit_status_refresh(t);
        CHECK(cast_order(U_RAMZA,10,ab,1),2);st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);
        impact();
        CHECK(core_status_current(&t->statuses,control_bits[j][0])&control_bits[j][1],
              again&&j==0?0:control_bits[j][1]);
        CHECK((t->status_flags&STATUS_NO_CONTROL)!=0,j!=0);   // Frog keeps control
        for(k=0;k<16&&st.action.phase!=CTX_IDLE;k++)end_anim();CHECK(st.action.phase,CTX_IDLE);
    }
}
// Source physical XA order: Berserk x3/2 then Frog 1 (attacker), then the
// target's Chicken/Frog x3/2. A Frog fights bare-handed: reach 1, PA*Br/100.
static void physical(void)
{
    Unit *a,*t;u16 i;u8 bow=255,sword=255;
    arena();a=&st.unit[U_RAMZA];t=&st.unit[10];
    for(i=0;i<255;i++){
        if(bow==255&&items[i].reach>1&&items[i].wp)bow=i;
        if(sword==255&&items[i].reach==1&&items[i].wp>5&&items[i].knife==0)sword=i;
    }
    CHECK(bow!=255&&sword!=255,1);
    CHECK(attack_prediction(U_RAMZA,10),25);
    change(U_RAMZA,SID_BERSERK);CHECK(attack_prediction(U_RAMZA,10),35);
    change(10,SID_FROG);CHECK(attack_prediction(U_RAMZA,10),50);
    arena();change(10,SID_CHICKEN);CHECK(attack_prediction(U_RAMZA,10),35);
    arena();a->eq[EQ_WEAPON]=sword;CHECK(attack_prediction(U_RAMZA,10),5*items[sword].wp);
    change(U_RAMZA,SID_FROG);CHECK(hand_weapon(a,0),255);CHECK(attack_prediction(U_RAMZA,10),5);
    arena();a->eq[EQ_WEAPON]=bow;CHECK(WEAPON_REACH(a),items[bow].reach);
    change(U_RAMZA,SID_FROG);CHECK(WEAPON_REACH(a),1);
    // Darkness and Confusion halve the attacker's base hit in the same way.
    arena();t->face=(u8)((st.unit[U_RAMZA].x<t->x)?2:0);
    for(i=0;i<2;i++){
        u8 h;arena();if(i)change(U_RAMZA,SID_DARK);else change(U_RAMZA,SID_CONFUSION);h=hit_chance(U_RAMZA,10);
        arena();CHECK(h<=hit_chance(U_RAMZA,10),1);
        arena();change(U_RAMZA,i?SID_CONFUSION:SID_DARK);CHECK(hit_chance(U_RAMZA,10),h);
    }
    // A confused target cannot evade; HP damage clears Confusion, not Berserk.
    arena();change(10,SID_CONFUSION);change(10,SID_BERSERK);CHECK(hit_chance(U_RAMZA,10),100);
    root(1);CHECK(core_status_current(&t->statuses,1)&16,0);CHECK(core_status_current(&t->statuses,2)&8,8);
}
// A Frog may only Attack or cast Frog; other learned spells are greyed out.
static void commands(void)
{
    u8 k,fire=255;Unit *u;
    prepare_control(29);u=&st.unit[U_RAMZA];
    for(k=0;k<SET_N;k++)if(SET_ABILITY(u->set,k)==0x10)fire=k;
    CHECK(fire!=255,1);u->learned|=(LearnBits)1<<fire;
    CHECK(can_use(U_RAMZA,29),1);CHECK(can_use(U_RAMZA,0x10),1);
    change(U_RAMZA,SID_FROG);CHECK(can_use(U_RAMZA,29),1);CHECK(can_use(U_RAMZA,0x10),0);
    CHECK(cast_order(U_RAMZA,10,0x10,1),0);
}
static u8 plan(u8 who)
{
    u16 steps=0;st.act=who;st.moved=0;st.acted=0;W->ai_k=0;
    while(!ai_think()&&++steps<2048){}
    return steps<2048;
}
// Lost control: the AI plays the unit. Berserk Attacks its nearest enemy only,
// a Chicken flees without acting (no Defend), Confusion may strike an ally.
static void control(void)
{
    u8 i,ally_hit=0,frog_cast=0;u16 seed;Unit *u;
    prepare_control(29);u=&st.unit[U_RAMZA];u->defend=1;
    change(U_RAMZA,SID_BERSERK);CHECK(u->status_flags&STATUS_NO_CONTROL,STATUS_NO_CONTROL);
    for(i=0;i<NUNIT;i++)st.unit[i].ct=0;u->ct=100;
    turn_start();CHECK(st.act,U_RAMZA);CHECK(st.ai,AI_THINK);CHECK(st.mode,M_BROWSE);
    CHECK(plan(U_RAMZA),1);for(i=0;i<W->ai_n;i++)CHECK(W->ai_rank[i].ab,NO_ABIL);
    CHECK(st.ai_ab,NO_ABIL);CHECK(st.ai_tgt,UNIT_CELL(&st.unit[10]));
    prepare_control(29);u=&st.unit[U_RAMZA];u->defend=1;change(U_RAMZA,SID_CHICKEN);
    CHECK(plan(U_RAMZA),1);CHECK(W->ai_n,0);CHECK(st.ai_ab,AI_NONE);
    // An unaffected player unit keeps its menu.
    prepare_control(29);for(i=0;i<NUNIT;i++)st.unit[i].ct=0;st.unit[U_RAMZA].ct=100;
    turn_start();CHECK(st.act,U_RAMZA);CHECK(st.ai,AI_OFF);CHECK(st.mode,M_MENU);
    // An adjacent ally and a remote enemy: some seeds strike the ally.
    for(seed=1;seed<=24;seed++){
        prepare_control(29);u=&st.unit[U_RAMZA];
        st.unit[10].team=TEAM_PLAYER;st.unit[0].hp=st.unit[0].max_hp=999;st.unit[0].dc=3;
        st.unit[0].team=TEAM_ENEMY;
        for(i=0;i<MAP_N;i++)if(abs((s16)TILE_X[i]-st.unit[10].x)+abs((s16)TILE_Z[i]-st.unit[10].z)>8&&
            map_tiles[i].walk==1&&!unit_at_cell(i))break;
        CHECK(i<MAP_N,1);unit_place(&st.unit[0],i);
        change(U_RAMZA,SID_CONFUSION);rand_seed(seed);CHECK(plan(U_RAMZA),1);
#ifdef CONTROL_TRACE
        {u8 q;printf("seed %u n %u ab %u tgt %u ally %u enemy %u me %u |",seed,W->ai_n,st.ai_ab,st.ai_tgt,UNIT_CELL(&st.unit[10]),UNIT_CELL(&st.unit[0]),UNIT_CELL(u));
         for(q=0;q<W->ai_n;q++)printf(" %u@%u",W->ai_rank[q].ab,W->ai_rank[q].t);printf("\n");}
#endif
        if(st.ai_ab!=AI_NONE&&st.ai_tgt==UNIT_CELL(&st.unit[10]))ally_hit=1;
        if(st.ai_ab==29)frog_cast=1;
    }
    CHECK(ally_hit,1);CHECK(frog_cast,1);
}
// Ordinary AI casts each new spell on an enemy; MP and Silence still gate it.
static void ai_and_guards(void)
{
    u8 j,k,found;u32 seed;
    for(j=0;j<3;j++){
        AbId ab=controls[j];prepare_control(ab);st.unit[U_RAMZA].ma=99;st.moved=1;
        CHECK(plan(U_RAMZA),1);found=0;
        for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==ab)found=1;
#ifdef CONTROL_TRACE
        printf("guard %u n %u nab %u:",ab,W->ai_n,W->ai_nab);for(k=0;k<W->ai_nab;k++)printf(" %u",W->ai_list[k]);
        printf(" | use? %u st10 %02x%02x%02x\n",skill_hit(U_RAMZA,10,ab),st.unit[10].statuses.inflicted[1],st.unit[10].statuses.inflicted[2],st.unit[10].statuses.inflicted[4]);
#endif
        CHECK(found,1);
        prepare_control(ab);st.unit[U_RAMZA].mp=0;seed=rng32;CHECK(cast_order(U_RAMZA,10,ab,1),0);CHECK(rng32==seed,1);
        prepare_control(ab);change(U_RAMZA,SID_SILENCE);CHECK(cast_order(U_RAMZA,10,ab,1),0);
    }
}
// Foxbird: Brave -30 (floor 0), Chicken below 10 and lost control; the end of
// a Chicken's turn gives Brave +1 and lifts Chicken again at 10.
static void foxbird(void)
{
    u8 k;Unit *t;
    prepare_control(54);t=&st.unit[10];st.unit[U_RAMZA].ma=99;st.unit[U_RAMZA].faith=t->faith=100;t->brave=35;
    CHECK(cast_order(U_RAMZA,10,54,1),2);st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);
    CHECK(W->cast.victim[10].effect&EFFECT_KIND_MASK,AB_BRAVE);CHECK(W->cast.victim[10].amount,30);impact();
    CHECK(t->brave,5);CHECK(core_status_current(&t->statuses,2)&4,4);CHECK(t->status_flags&STATUS_NO_CONTROL,STATUS_NO_CONTROL);
    for(k=0;k<16&&st.action.phase!=CTX_IDLE;k++)end_anim();CHECK(st.action.phase,CTX_IDLE);
    prepare_control(54);t=&st.unit[10];st.unit[U_RAMZA].ma=99;st.unit[U_RAMZA].faith=t->faith=100;t->brave=20;
    CHECK(cast_order(U_RAMZA,10,54,1),2);st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);impact();
    CHECK(t->brave,0);for(k=0;k<16&&st.action.phase!=CTX_IDLE;k++)end_anim();
    // An unaffected target above 39 keeps control.
    prepare_control(54);t=&st.unit[10];st.unit[U_RAMZA].ma=99;st.unit[U_RAMZA].faith=t->faith=100;t->brave=70;
    CHECK(cast_order(U_RAMZA,10,54,1),2);st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);impact();
    CHECK(t->brave,40);CHECK(t->status_flags&STATUS_NO_CONTROL,0);for(k=0;k<16&&st.action.phase!=CTX_IDLE;k++)end_anim();
    // Upkeep: 9 -> 10 lifts Chicken.
    prepare_control(54);t=&st.unit[10];t->brave=9;change(10,SID_CHICKEN);CHECK(t->status_flags&STATUS_NO_CONTROL,STATUS_NO_CONTROL);
    st.act=10;turn_end(1,1);CHECK(t->brave,10);CHECK(core_status_current(&t->statuses,2)&4,0);CHECK(t->status_flags&STATUS_NO_CONTROL,0);
    prepare_control(54);t=&st.unit[10];t->brave=3;change(10,SID_CHICKEN);st.act=10;turn_end(1,1);
    CHECK(t->brave,4);CHECK(core_status_current(&t->statuses,2)&4,4);
}
static void control_trials(void){profiles();control_production();physical();commands();control();ai_and_guards();foxbird();}
#ifndef __m68k__
int main(void){sw_init(61);control_trials();printf("control_status %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();control_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
