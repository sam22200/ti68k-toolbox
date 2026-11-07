// Team elimination and ordinary stone immunity without a new status/art producer.
#define TEMPO_CHECK
#include "check_scheduler.c"
#include "x/core-incapacity/oracle/source.h"
static void stone(u8 who)
{
    st.unit[who].statuses.inflicted[1]|=0x80;unit_status_refresh(&st.unit[who]);
}
static void state_kind(u8 who,u8 kind,u8 cell,u8 team)
{
    static const u8 sid[]={255,2,8,1,15,35,30,9,13,255,5};
    Unit *u=&st.unit[who];unit_place(u,cell);u->team=team;u->hp=100;u->dc=3;
    memset(&u->statuses,0,sizeof u->statuses);
    if(sid[kind]!=255)u->statuses.inflicted[sid[kind]>>3]=(u8)(0x80>>(sid[kind]&7));
    if(kind==1||kind==3||kind==4)u->hp=0;
    if(kind==3)u->dc=DC_CRYSTAL;if(kind==4)u->dc=DC_CHEST;
    if(kind==9)u->x=255;
    unit_status_refresh(u);
}
static void outcomes(void)
{
    u8 i,k;
    for(k=0;k<sizeof source_outcomes/sizeof source_outcomes[0];k++){
        const u16 *p=source_outcomes[k];wave();
        for(i=0;i<NUNIT;i++){st.unit[i].hp=0;st.unit[i].dc=DC_GONE;st.unit[i].x=255;}
        state_kind(U_RAMZA,p[0],cells[U_RAMZA],TEAM_PLAYER);
        state_kind(12,p[1],cells[12],TEAM_ENEMY);state_kind(0,p[2],cells[0],TEAM_GUEST);
        ZB(45);CHECK(outcome(),p[3]);ZE(45);
    }
    // The existing mandatory-Ramza permanent-loss rule remains in force.
    wave();st.unit[U_RAMZA].hp=0;st.unit[U_RAMZA].dc=DC_CRYSTAL;CHECK(outcome(),END_DEFEAT);
    wave();st.unit[U_RAMZA].hp=0;st.unit[U_RAMZA].dc=DC_CHEST;CHECK(outcome(),END_DEFEAT);
    // Last stone enemy ends the fight before any CT/death-counter search.
    arena();stone(10);CHECK(st.unit[10].hp,999);CHECK(st.unit[10].dc,3);
    CHECK(unit_at_cell(second),11);CHECK(unit_can_act(&st.unit[10]),0);
    turn_start();CHECK(st.mode,M_END);CHECK(st.end,END_VICTORY);
    CHECK(st.unit[10].hp,999);CHECK(st.unit[10].dc,3);CHECK(st.morph_mask,0);
    arena();stone(U_RAMZA);stone(10);turn_start();CHECK(st.end,END_DEFEAT);
}
static void forecast_and_at(void)
{
    u8 i;u32 seed;u16 hp;
    arena();stone(10);seed=rng32;hp=st.unit[10].hp;
    CHECK(hit_chance(U_RAMZA,10),0);CHECK(attack_prediction(U_RAMZA,10),0);
    CHECK(skill_hit(U_RAMZA,10,0x170),0);CHECK(magic_target(10),MAGIC_BLOCKED);
    CHECK(rng32==seed,1);CHECK(st.unit[10].hp,hp);CHECK(unit_at_cell(second),11);
    turn_order();for(i=0;i<ORDER_N;i++)CHECK(st.turns[i],U_RAMZA);
    // An absent unit with positive HP must neither extend a battle nor appear in AT.
    arena();st.unit[10].x=255;CHECK(outcome(),END_VICTORY);
    turn_order();for(i=0;i<ORDER_N;i++)CHECK(st.turns[i],U_RAMZA);
    // CORE_TERMINAL enabled the public Petrify producer: the request is accepted.
    {CoreStatusDelta d;arena();memset(&d,0,sizeof d);d.add[1]=0x80;
     CHECK(battle_status_apply(10,&d,0)!=STATUS_REJECTED,1);CHECK(unit_petrified(&st.unit[10]),0x80);}
}
static void keep_enemy(void)
{
    unit_place(&st.unit[12],third);st.unit[12].hp=st.unit[12].max_hp=999;
    st.unit[12].dc=3;st.unit[12].team=TEAM_ENEMY;
}
static void last_node(void)
{
    u8 width=map_w,height=map_h;u16 count=map_n;Unit *u;
    arena();u=&st.unit[10];u->x=u->z=15;u->cell=255;
    // Outcome uses identity/presence only, not geometry or the coordinate tables.
    map_w=map_h=16;map_n=256;
#ifdef FFT_MODULES
    mod_id=0;
#endif
    CHECK(unit_on_field(u),1);CHECK(outcome(),END_NONE);
    map_n=255;
#ifdef FFT_MODULES
    mod_id=0;
#endif
    CHECK(unit_on_field(u),0);CHECK(outcome(),END_VICTORY);
    map_w=width;map_h=height;map_n=count;
#ifdef FFT_MODULES
    mod_id=0;
#endif
}
static void terminal_door(void)
{
    u8 width=map_w,height=map_h;u16 count=map_n;
    // A deliberately clipped preparation cannot start a usable party. The
    // injection door must return the terminal state rather than seek Ramza forever.
    map_w=map_h=1;map_n=1;
#ifdef FFT_MODULES
    mod_id=0;
#endif
    game_scenario(61);CHECK(st.mode,M_END);CHECK(st.end,END_DEFEAT);
    map_w=width;map_h=height;map_n=count;
#ifdef FFT_MODULES
    mod_id=0;
#endif
}
static void ordinary_effects(void)
{
    u8 i;u32 random_before[2],random_after[2],seed;u16 hp,jp,bag;
    for(i=0;i<2;i++){
        arena();keep_enemy();if(i)stone(10);
        random_before[i]=rng32;attack(U_RAMZA,10);random_after[i]=rng32;
        hp=st.unit[10].hp;jp=st.unit[U_RAMZA].jp;
        // Supply a prepared hit, matching the original finalizer's fixture input.
        st.hit=1;st.push=0;st.dmg=7;seed=rng32;impact();
        CHECK(st.hit,source_stone_final[i?2:0][2]);CHECK(st.dmg,source_stone_final[i?2:0][3]);
        CHECK(st.unit[10].hp,hp-(i?0:7));
        if(i){CHECK(st.unit[U_RAMZA].jp,jp);CHECK(rng32==seed,1);CHECK(st.gain,0);}
    }
    // Petrify is rejected by hit preparation before a random draw; the
    // ordinary live-target strike still consumes its preparation prefix.
    CHECK(random_before[0],random_before[1]);
    CHECK(random_after[0]!=random_before[0],1);
    CHECK(random_after[1],random_before[1]);
    // A prepared root is also harmless if the target petrifies before HP commit.
    arena();keep_enemy();attack(U_RAMZA,10);stone(10);st.hit=1;st.dmg=7;st.push=1;st.push_cell=third;
    st.self=999;hp=st.unit[U_RAMZA].hp;impact();
    CHECK(st.unit[10].hp,999);CHECK(st.unit[U_RAMZA].hp,hp);CHECK(st.push,0);CHECK(st.self,0);
    CHECK(UNIT_CELL(&st.unit[10]),second);
    // Payment remains in preparation; a potion cannot heal the statue.
    arena();keep_enemy();stone(10);st.unit[10].hp=20;st.bag[abilities[0x170].bag]=2;
    skill(U_RAMZA,10,0x170);bag=st.bag[abilities[0x170].bag];CHECK(bag,1);
    impact();CHECK(st.hit,0);CHECK(st.unit[10].hp,20);CHECK(st.bag[abilities[0x170].bag],bag);
    // An already-queued Counter is dropped after the reactor petrifies.
    arena();keep_enemy();st.reaction[10]=REACTION_COUNTER;root(1);CHECK(st.action.pending!=0,1);
    stone(10);end_anim();CHECK(st.primary.running,0);CHECK(st.action.pending,0);
}
static void splash(void)
{
    AbId ab;u16 hp;
    arena();keep_enemy();stone(12);
    for(ab=0;ab<512;ab++)if(abilities[ab].kind==AB_DAMAGE&&(abilities[ab].flags&AF_AREA)&&!(abilities[ab].flags&AF_CAST))break;
    CHECK(ab<512,1);hp=st.unit[12].hp;skill(U_RAMZA,10,ab);st.hit=1;st.dmg=1;st.push=0;impact();
    CHECK(st.unit[10].hp,998);CHECK(st.unit[12].hp,hp);CHECK(unit_at_cell(third),13);
}
static void ai_targeting(void)
{
    u8 i,petrify;
    for(petrify=0;petrify<2;petrify++){
        u16 steps=0;arena();keep_enemy();if(petrify)stone(10);
        st.moved=st.acted=0;st.ai=AI_THINK;W->ai_k=0;
        while(!ai_think()&&++steps<1024){}CHECK(steps<1024,1);
        if(!petrify)CHECK(W->ai_n!=0,1);
        else for(i=0;i<W->ai_n;i++)CHECK(W->ai_rank[i].t!=second,1);
        CHECK(st.unit[10].hp,999);CHECK(unit_at_cell(second),11);
    }
}
static void thirteen(void)
{
    u8 i;u32 seed,before;u16 hp;
    wave();for(i=0;i<NUNIT;i++)if(st.unit[i].team==TEAM_ENEMY)stone(i);
    seed=rng32;hp=st.unit[12].hp;before=state_hash();
#ifdef FFT_MODULES
    mod_id=0;
#endif
    ZB(46);CHECK(outcome(),END_VICTORY);ZE(46);ZB(47);CHECK(outcome(),END_VICTORY);ZE(47);
    CHECK(state_hash()==before,1);
    CHECK(rng32==seed,1);CHECK(st.unit[12].hp,hp);CHECK(st.unit[12].dc,3);
    for(i=0;i<NUNIT;i++)stone(i);CHECK(outcome(),END_DEFEAT);
    ZB(48);turn_start();ZE(48);CHECK(st.mode,M_END);CHECK(st.end,END_DEFEAT);CHECK(st.morph_mask,0);
}
static void incapacity_trials(void){outcomes();forecast_and_at();last_node();terminal_door();ordinary_effects();splash();ai_targeting();thirteen();}
#ifndef __m68k__
int main(void){sw_init(61);incapacity_trials();printf("incapacity %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();incapacity_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
