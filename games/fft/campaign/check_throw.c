// Original Throw formula and production item command ownership.
#define REFLECT_CHECK
#define FFT_CHECK_NATIVE_ORDINAL
#include "check_offensive_magic.c"
#include "x/core-throw/oracle/source.h"
static void prepare_throw(u8 item)
{
    u8 i;prepare_spell(19);memset(battle_save.inv,0,256);
    st.unit[U_RAMZA].set=20;st.unit[U_RAMZA].learned=0xfff;
    st.unit[U_RAMZA].level=99;st.unit[U_RAMZA].sp=7;
    st.unit[10].hp=st.unit[10].max_hp=999;st.unit[10].face=0;
    for(i=0;i<NUNIT;i++)if(i!=U_RAMZA&&i!=10){st.unit[i].dc=DC_GONE;st.unit[i].x=255;}
    battle_save.inv[item]=3;st.unit[10].team=TEAM_ENEMY;
}
static void finish_throw(void)
{
    u8 n;for(n=0;n<20&&st.action.phase!=CTX_IDLE;n++)end_anim();CHECK(st.action.phase,CTX_IDLE);
}
static void profiles(void)
{
    u16 i;u8 k;u32 after;char text[80];
    for(i=0;i<sizeof source_throw/sizeof source_throw[0];i++){
        const typeof(source_throw[0])*p=&source_throw[i];AbId ab=512U+p->item;Unit *a,*t;
        prepare_throw(p->item);a=&st.unit[U_RAMZA];t=&st.unit[10];a->sp=p->sp;
        a->zodiac=p->az;t->zodiac=p->tz;t->female=p->sex;
        st.support[U_RAMZA]=p->up==1?SUPPORT_ATTACK_UP:p->up==2?SUPPORT_MARTIAL_ARTS:p->up==3?SUPPORT_MAGIC_ATTACK_UP:0;
        st.support[10]=p->defense?SUPPORT_DEFENSE_UP:0;
        memcpy(t->statuses.inflicted,source_throw_status[p->mode],5);unit_status_refresh(t);
        if(p->gear){t->job=0x4a;t->eq[EQ_SHIELD]=p->gear&1?128:255;t->eq[EQ_ACC]=p->gear&2?232:255;t->face=p->side==1?1:p->side==2?2:0;}
        if(p->affinity)st.affinity[10][p->affinity-1]=abilities[ab].element;
        rng32=p->seed;ZB(61);CHECK(skill_amount(U_RAMZA,10,ab,0),p->preview_amount);ZE(61);
        CHECK(skill_kind(U_RAMZA,10,ab),p->preview_kind);CHECK(skill_hit(U_RAMZA,10,ab),p->chance);forecast_text(U_RAMZA,10,ab,text);
        CHECK(rng32==p->seed,1);CHECK(battle_save.inv[p->item],3);
        for(k=0;k<p->draws;k++)rand15();after=rng32;rng32=p->seed;
        ZB(62);skill(U_RAMZA,10,ab);ZE(62);
        CHECK(st.mode,M_ACT);CHECK(st.hit,p->hit);CHECK(st.dmg,p->amount);
        CHECK(st.effect_kind,p->kind);CHECK(rng32==after,1);CHECK(battle_save.inv[p->item],2);
        CHECK(st.crit,0);CHECK(st.push,0);CHECK(a->mp,999);
        ZB(63);impact();ZE(63);CHECK(t->hp,p->kind==AB_DAMAGE?999-p->amount:999);
        impact();CHECK(battle_save.inv[p->item],2);finish_throw();
    }
}
static void choices(void)
{
    u8 item,k,n=0;u16 c;u32 seed;State before;AbId ab;
    prepare_throw(122);CHECK(known(U_RAMZA,0),634);CHECK(known(U_RAMZA,1),NO_ABIL);
    CHECK(can_use(U_RAMZA,634),1);CHECK(can_use(U_RAMZA,382),0);CHECK(can_use(U_RAMZA,640),0);
    st.unit[U_RAMZA].learned=0;CHECK(known(U_RAMZA,0),NO_ABIL);CHECK(can_use(U_RAMZA,634),0);
    before=st;seed=rng32;skill(U_RAMZA,10,634);CHECK(memcmp(&st,&before,sizeof st)==0,1);CHECK(rng32==seed,1);CHECK(battle_save.inv[122],3);
    st.unit[U_RAMZA].learned=1;st.bag[0]=77;st.pending[U_RAMZA].state=DEFER_CAST_TILE;
    battle_save.inv[122]=0;before=st;skill(U_RAMZA,10,634);CHECK(memcmp(&st,&before,sizeof st)==0,1);CHECK(st.bag[0],77);
    battle_save.inv[122]=1;st.pending[U_RAMZA].state=0;
    skill_range(U_RAMZA,634);for(c=0;c<MAP_N;c++)if(st.reach[c]!=255){
        CHECK(abs((s16)TILE_X[c]-st.unit[U_RAMZA].x)+abs((s16)TILE_Z[c]-st.unit[U_RAMZA].z)<=st.unit[U_RAMZA].move,1);
    }
    CHECK(st.reach[UNIT_CELL(&st.unit[U_RAMZA])],255);
    skill(U_RAMZA,10,634);CHECK(battle_save.inv[122],0);CHECK(st.bag[0],77);CHECK(can_use(U_RAMZA,634),0);
    impact();finish_throw();CHECK(known(U_RAMZA,0),NO_ABIL);
    prepare_throw(122);st.reaction[10]=REACTION_COUNTER;st.unit[10].brave=100;
    skill(U_RAMZA,10,634);impact();CHECK(st.action.pending&((u16)1<<10),(u16)1<<10);
    end_anim();CHECK(st.action.phase,CTX_SECONDARY);impact();end_anim();CHECK(st.abil,634);
    CHECK(battle_save.inv[122],2);finish_throw();
    prepare_throw(125);st.unit[10].statuses.inflicted[2]|=128;unit_status_refresh(&st.unit[10]);
    skill(U_RAMZA,10,637);CHECK(st.effect_kind&EFFECT_KIND_MASK,AB_DAMAGE);CHECK(act_kind(),AB_DAMAGE);
    CHECK(st.effect_kind&EFFECT_OIL,EFFECT_OIL);impact();CHECK(core_status_current(&st.unit[10].statuses,2)&128,0);finish_throw();
    prepare_throw(125);st.unit[10].hp=500;st.affinity[10][0]=128;
    skill(U_RAMZA,10,637);CHECK(st.effect_kind,AB_HEAL);CHECK(st.dmg,56);impact();CHECK(st.unit[10].hp,556);finish_throw();
    prepare_throw(122);st.unit[U_RAMZA].status|=ST_SILENCE;CHECK(can_use(U_RAMZA,634),1);
    st.unit[U_RAMZA].statuses.inflicted[2]|=2;unit_status_refresh(&st.unit[U_RAMZA]);CHECK(can_use(U_RAMZA,634),0);
    prepare_throw(122);st.unit[U_RAMZA].team=TEAM_ENEMY;battle_save.inv[122]=0;CHECK(can_use(U_RAMZA,634),1);
    skill(U_RAMZA,10,634);CHECK(battle_save.inv[122],0);impact();finish_throw();
    prepare_throw(122);st.unit[U_RAMZA].learned=0xfff;
    for(item=0;item<128;item++)battle_save.inv[item]=1;
    while((ab=known(U_RAMZA,n))!=NO_ABIL){CHECK(ab>=512&&ab<640,1);CHECK(abilities[ab].kind,AB_DAMAGE);CHECK(AB_JP(&abilities[ab]),AB_JP(&abilities[SET_ABILITY(20,abilities[ab].rate)]));n++;}
    CHECK(n,80);CHECK(known(U_RAMZA,255),80);
    for(k=0;k<12;k++){
        st.unit[U_RAMZA].learned=(LearnBits)1<<k;n=0;
        while((ab=known(U_RAMZA,n))!=NO_ABIL){
            CHECK(abilities[ab].rate,k);CHECK(SET_ABILITY(20,k)>=382&&SET_ABILITY(20,k)<394,1);n++;
        }
        CHECK(n>0,1);CHECK(known(U_RAMZA,255),n);
    }
    st.unit[U_RAMZA].learned=0xfff; // exact current source throwable catalogue.
    st.moved=1;W->ai_k=0;for(c=0;c<2048&&!ai_think();c++){}CHECK(c<2048,1);CHECK(W->ai_nab<=17,1);
    n=0;for(k=0;k<W->ai_nab;k++)if(W->ai_list[k]>=512&&W->ai_list[k]<640)n++;
    CHECK(n>0,1);CHECK(n<=8,1);
}
static void throw_trials(void){profiles();choices();}
#ifndef __m68k__
int main(void){sw_init(61);throw_trials();printf("throwing %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();throw_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
