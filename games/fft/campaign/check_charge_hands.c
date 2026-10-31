// Dual deferred Charge: source arithmetic, live resolution and one root settlement.
#define REACTION_SEQUENCE_CHECK
#include "check_reactions.c"
#include "x/core-charge-hands/oracle/source.h"
static void pair(void)
{
    arena();st.unit[U_RAMZA].eq[EQ_WEAPON]=1;st.unit[U_RAMZA].eq[EQ_SHIELD]=11;
    st.support[U_RAMZA]=SUPPORT_TWO_SWORDS;st.unit[U_RAMZA].pa=5;st.unit[U_RAMZA].sp=9;
    st.unit[10].eq[EQ_SHIELD]=255;st.unit[10].face=0;
}
static void ordinary_hit(void)
{
    st.hit=1;st.crit=st.push=0;
    st.dmg=charge_query(st.atk,st.tgt,st.abil,st.hand,0);
}
static void source_math(void)
{
    u16 i;Unit *a,*t;u32 seed;
    for(i=0;i<sizeof source_charge_damage/sizeof source_charge_damage[0];i++){
        const u16 *p=source_charge_damage[i];u8 critical=p[7]==0;
        arena();a=&st.unit[U_RAMZA];t=&st.unit[10];a->eq[EQ_WEAPON]=p[1];a->eq[EQ_SHIELD]=255;
        a->pa=p[2];a->sp=p[3];a->brave=p[4];a->zodiac=0;a->female=t->female=0;t->zodiac=p[6];
        if(p[5]==4)unit_status_action(t,8);else if(p[5]!=255)change(10,p[5]);
        seed=rng32;CHECK(charge_query(U_RAMZA,10,p[0],critical<<2,p[8]),p[9]);
        CHECK(critical,p[10]);CHECK(rng32==seed,1);
        if(!critical)CHECK(skill_amount(U_RAMZA,10,p[0],0),p[9]);
    }
}
static void eight_powers(void)
{
    u8 i;char text[32];
    for(i=0;i<8;i++){
        const u16 *p=source_charge_hands[i];u16 hp,jp,expected;
        pair();CHECK(can_use(U_RAMZA,p[0]),1);hp=st.unit[10].hp;jp=st.unit[U_RAMZA].jp;
        expected=(7+p[2])*11;
        CHECK(skill_amount(U_RAMZA,10,p[0],FORMULA_FORECAST),expected);
        forecast_text(U_RAMZA,10,p[0],text);CHECK(text[0],'-');CHECK(st.unit[10].hp,hp);
        skill(U_RAMZA,10,p[0]);CHECK(st.pending[U_RAMZA].ticks,p[1]);
        CHECK(st.pending[U_RAMZA].weapon,p[4]);CHECK(st.pending[U_RAMZA].off_weapon,p[5]);
        CHECK(st.action.phase,CTX_IDLE);CHECK(st.moved|st.acted<<1,3);
        game_update();CHECK(st.pending[U_RAMZA].ticks,p[1]); // menus do not tick CT
        st.pending[U_RAMZA].ticks=0;charge_resolve();CHECK(st.open_resolving,1);
        CHECK(st.action.left,p[3]-1);CHECK(st.abil,p[0]);CHECK(st.hand,0);
        ordinary_hit();CHECK(st.dmg,(7+p[2])*3);impact();CHECK(st.unit[U_RAMZA].jp,jp);
        ZB(32);end_anim();ZE(32);CHECK(st.abil,p[0]);CHECK(st.hand,1);CHECK(st.action.phase,CTX_ROOT);
        ordinary_hit();CHECK(st.dmg,(7+p[2])*8);impact();end_anim();end_anim();
        CHECK(st.action.phase,CTX_IDLE);CHECK(st.open_resolving,0);CHECK(hp-st.unit[10].hp,expected);
        CHECK(st.unit[U_RAMZA].gexp,10);CHECK(st.unit[U_RAMZA].jp>jp,1);
    }
}
static void interactions(void)
{
    u8 mode;u16 hp,jp;
    for(mode=0;mode<4;mode++){
        pair();hp=st.unit[10].hp;jp=st.unit[U_RAMZA].jp;
        if(mode==0)change(10,SID_SLEEP);
        if(mode==1){change(10,SID_SLEEP);st.unit[10].statuses.innate[4]|=0x10;}
        if(mode==2)st.reaction[10]=REACTION_COUNTER;
        if(mode==3){st.reaction[10]=REACTION_POTION;st.unit[10].hp=100;hp=100;}
        CHECK(skill_amount(U_RAMZA,10,0x196,FORMULA_FORECAST),mode==0?100:mode==1?132:88);
        skill(U_RAMZA,10,0x196);st.pending[U_RAMZA].ticks=0;charge_resolve();ordinary_hit();impact();
        CHECK(st.gain,0);CHECK(st.unit[U_RAMZA].jp,jp);end_anim();CHECK(st.atk,U_RAMZA);
        ordinary_hit();impact();CHECK(hp-st.unit[10].hp,mode==0?100:mode==1?132:88);
        end_anim();
        if(mode>=2){CHECK(st.atk,10);CHECK(st.action.phase,CTX_SECONDARY);
            if(mode==2)st.hit=0;impact();end_anim();CHECK(st.abil,0x196);CHECK(st.hand,1);}
        if(st.gain)end_anim();CHECK(st.action.phase,CTX_IDLE);CHECK(st.open_resolving,0);
        CHECK(st.unit[U_RAMZA].gexp,10);
    }
}
static void invalidation(void)
{
    u8 mode;
    for(mode=0;mode<5;mode++){
        pair();skill(U_RAMZA,10,0x196);st.pending[U_RAMZA].ticks=0;
        if(mode==0)st.unit[U_RAMZA].eq[EQ_SHIELD]=5;
        if(mode==1)st.unit[U_RAMZA].eq[EQ_WEAPON]=19;
        if(mode==2)unit_place(&st.unit[U_RAMZA],third);
        if(mode==3)st.unit[U_RAMZA].hp=0;
        if(mode==4)change(U_RAMZA,SID_SLEEP);
        CHECK(pending_next(),255);CHECK(st.pending[U_RAMZA].state,DEFER_NONE);CHECK(st.action.phase,CTX_IDLE);
    }
    pair();skill(U_RAMZA,10,0x196);change(U_RAMZA,SID_STOP);st.pending[U_RAMZA].ticks=0;
    CHECK(pending_next(),255);CHECK(st.pending[U_RAMZA].state,DEFER_CHARGING);
    // Recoil or KO on strike1 cannot be followed to another tile.
    for(mode=0;mode<2;mode++){
        pair();skill(U_RAMZA,10,0x196);st.pending[U_RAMZA].ticks=0;charge_resolve();ordinary_hit();
        if(mode){st.dmg=st.unit[10].hp;}else{st.push=1;st.push_cell=third;}
        impact();end_anim();CHECK(st.action.left,0);CHECK(st.hand,0);
        if(st.gain)end_anim();CHECK(st.action.phase,CTX_IDLE);CHECK(st.open_resolving,mode?1:0);
        CHECK(st.end,mode?END_VICTORY:END_NONE);
    }
}
static u16 seed_for(u8 critical)
{
    u16 candidate;
    for(candidate=1;candidate<2048;candidate++){
        u8 got;rng32=candidate;rand15();got=((u32)rand15()*100>>15)<4;
        if(got==critical)return candidate;
    }
    CHECK(0,1);return 1;
}
static void actual_rolls(void)
{
    u8 hand;
    for(hand=0;hand<2;hand++){
        pair();rng32=seed_for(hand==0);skill(U_RAMZA,10,0x196);
        st.pending[U_RAMZA].ticks=0;charge_resolve();CHECK(st.hit,1);CHECK(st.crit,!hand);
        if(hand){ordinary_hit();impact();rng32=seed_for(1);end_anim();CHECK(st.hand,1);CHECK(st.hit,1);CHECK(st.crit,1);}
        CHECK(st.abil,0x196);CHECK(st.dmg>0,1);
    }
}
static void ai_prediction(void)
{
    u8 dual;u16 steps,hp;u32 seed,chosen_seed;s16 rank[2];
    for(dual=0;dual<2;dual++){
        pair();if(!dual)st.support[U_RAMZA]=0;
        st.unit[U_RAMZA].set=8;st.unit[U_RAMZA].learned=1;st.unit[U_RAMZA].defend=0;
        st.acted=0;st.moved=1;st.unit[10].ct=0;st.unit[10].sp=1;hp=st.unit[10].hp;seed=rng32;rand15();chosen_seed=rng32;rng32=seed;
        W->ai_k=0;steps=0;while(!ai_think()&&++steps<1024){}
        CHECK(steps<1024,1);CHECK(st.ai_ab,0x196);CHECK(W->ai_n>0,1);rank[dual]=W->ai_rank[0].pri;
        CHECK(st.unit[10].hp,hp);CHECK(rng32==chosen_seed,1);CHECK(W->ai_rank[0].hit,100);CHECK(st.pending[U_RAMZA].state,DEFER_NONE);
    }
    CHECK(rank[1]>rank[0],1);
}
static void cold_pickups(void)
{
    u16 count;u8 i;
    pair();st.unit[U_RAMZA].hp=1;st.unit[U_RAMZA].mp=0;st.unit[10].dc=DC_CRYSTAL;
    pick_up(U_RAMZA,10);CHECK(st.unit[U_RAMZA].hp,999);CHECK(st.unit[U_RAMZA].mp,st.unit[U_RAMZA].max_mp);
    CHECK(st.unit[10].dc,DC_GONE);CHECK(st.pick,255);
    pair();for(i=0;i<EQ_N;i++)st.unit[10].eq[i]=255;st.unit[10].eq[EQ_WEAPON]=19;st.unit[10].dc=DC_CHEST;
    count=st.nfound;pick_up(U_RAMZA,10);CHECK(st.pick,19);CHECK(st.nfound,count+1);CHECK(st.found[count],19);
}
static void trials_charge_hands(void){source_math();eight_powers();interactions();invalidation();actual_rolls();ai_prediction();cold_pickups();}
#ifndef __m68k__
int main(void){sw_init(61);trials_charge_hands();printf("chargehands %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();trials_charge_hands();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
