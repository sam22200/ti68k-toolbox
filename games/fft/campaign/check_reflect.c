// All eleven new source producers, plus the unchanged first fourteen.
#define REFLECT_CHECK
#include "check_offensive_magic.c"
#include "x/core-reflect/oracle/source.h"
static const AbId reflected_spells[]={0x0f,0x10,0x11,0x12,0x14,0x15,0x16,0x18,0x19,0x1a,0x1f};
static void reflect_prepare(AbId ab)
{
    u8 k;prepare_spell(0x13);st.unit[U_RAMZA].set=ab==0xf?10:11;
    for(k=0;k<SET_N;k++)if(SET_ABILITY(st.unit[U_RAMZA].set,k)==ab)break;
    CHECK(k<SET_N,1);st.unit[U_RAMZA].learned=(LearnBits)1<<k;
    CHECK(known(U_RAMZA,0),ab);CHECK(known(U_RAMZA,1),NO_ABIL);
}
static void original_cases(void)
{
    u16 i;u8 col[8];
    for(i=0;i<sizeof source_reflect/sizeof source_reflect[0];i++){
        const u16 *p=source_reflect[i];s16 x=2*(s16)p[2]-p[0],z=2*(s16)p[3]-p[1];
        memset(col,0,8);col[0]=p[6];col[4]=p[7];col[3]=p[8]?128:0;col[7]=p[9]?128:0;
        if(p[10]){CHECK(x,p[11]);CHECK(z,p[13]);CHECK(core_reflect_layer(col,p[5],1),p[12]+1);}
        else CHECK(x<0||z<0||x>=5||z>=5||(p[0]==p[2]&&p[1]==p[3])||!core_reflect_layer(col,p[5],1),1);
    }
    for(i=0;i<sizeof source_reflect_guard/sizeof source_reflect_guard[0];i++){
        const u16 *p=source_reflect_guard[i];CHECK(p[2],p[1]&&p[0]!=21?REACTION_REFLECT:0);
        CHECK(p[3],p[2]?9:0);CHECK(p[4],p[2]?0x10:0);
    }
}
static void producer_batch(void)
{
    u8 j,protect,k,area;AbId ab;u16 hp,damage,cost;u32 seed;
    for(j=0;j<sizeof reflected_spells/sizeof reflected_spells[0];j++)for(protect=0;protect<3;protect++){
        ab=reflected_spells[j];reflect_prepare(ab);cost=cj_range(common,ab)[13];
        area=(abilities[ab].flags&AF_AREA)!=0;damage=5*cj_range(common,ab)[10];
        st.unit[10].ma=20;st.unit[8].ma=30;st.unit[10].face=3;
        if(protect){change(10,SID_REFLECT);CHECK(core_status_current(&st.unit[10].statuses,4)&2,2);}
        st.support[U_RAMZA]=SUPPORT_NON_CHARGE;seed=rng32;
        CHECK(cast_order(U_RAMZA,10,ab,1),1);CHECK(st.unit[U_RAMZA].mp,999-cost);
        if(protect){
            CHECK(st.hit,0);if(!area)CHECK(st.effect_kind,AB_REFLECT);
            if(!area)CHECK(rng32==seed,1);
        }
        impact();CHECK(st.unit[10].hp,protect?999:999-damage);
        if(protect){
            if(protect==2)change(8,SID_REFLECT); // the secondary must not bounce again
            CHECK(st.action.pending&((u16)1<<10),(u16)1<<10);
            hp=st.unit[8].hp;ZB(61);end_anim();ZE(61);CHECK(st.action.phase,CTX_SECONDARY);
            CHECK(st.action.actor,10);CHECK(st.atk,10);CHECK(st.tgt,8);
            CHECK(st.unit[10].face,3);
            CHECK(st.abil,ab);CHECK(st.hit,1);CHECK(st.dmg,damage);
            CHECK(st.unit[10].mp,999);impact();CHECK(st.unit[8].hp,hp-damage);
            impact();CHECK(st.unit[8].hp,hp-damage);
            CHECK(st.action.pending&((u16)1<<8),0);
        }
        for(k=0;k<20&&st.action.phase!=CTX_IDLE;k++){if(st.action.phase==CTX_SECONDARY)impact();end_anim();}
        CHECK(k<20,1);CHECK(st.action.phase,CTX_IDLE);CHECK(st.unit[U_RAMZA].mp,999-cost);
        CHECK(st.unit[10].mp,999);CHECK(st.action.pending,0);
    }
}
static void late_status_and_expiry(void)
{
    u8 j,k;CoreStatusDelta d;
    for(j=0;j<sizeof reflected_spells/sizeof reflected_spells[0];j++){
        AbId ab=reflected_spells[j];reflect_prepare(ab);
        CHECK(cast_order(U_RAMZA,10,ab,1),2);change(10,SID_REFLECT);
        CHECK(st.unit[10].status_ct[14],cj_status_rules(common)[(SID_REFLECT<<4)+3]);
        st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);impact();
        CHECK(st.unit[10].hp,999);CHECK(st.action.pending&((u16)1<<10),(u16)1<<10);
        for(k=0;k<20&&st.action.phase!=CTX_IDLE;k++){if(st.action.phase==CTX_SECONDARY)impact();end_anim();}
        reflect_prepare(ab);change(10,SID_REFLECT);memset(&d,0,sizeof d);d.remove[4]=2;
        CHECK(battle_status_apply(10,&d,1)!=STATUS_REJECTED,1);CHECK(st.unit[10].status_ct[14],0);
        CHECK(core_status_current(&st.unit[10].statuses,4)&2,0);
        st.unit[10].statuses.immune[4]=2;change(10,SID_REFLECT);
        CHECK(core_status_current(&st.unit[10].statuses,4)&2,0);
    }
}
static void no_target_and_restrictions(void)
{
    u8 mode,k;u32 seed;u16 hp;
    for(mode=0;mode<4;mode++){
        reflect_prepare(0xf);change(10,SID_REFLECT);st.support[U_RAMZA]=SUPPORT_NON_CHARGE;
        if(mode==0){st.unit[8].hp=0;st.unit[8].dc=DC_GONE;st.unit[8].x=255;}
        if(mode==1)change(10,SID_DONT_ACT);
        if(mode==2)change(10,SID_STOP); // ordinary reaction incapacity does not stop Reflect
        CHECK(cast_order(U_RAMZA,10,0xf,1),1);impact();seed=rng32;hp=st.unit[8].hp;end_anim();
        if(mode<=1){CHECK(st.primary.running,0);CHECK(st.unit[10].hp,999);}
        else{CHECK(st.primary.running,1);CHECK(st.hit,mode==2||mode==3?1:0);}
        if(mode==0)CHECK(rng32==seed,1);
        if(mode==0||mode==1)CHECK(st.unit[8].hp,hp);
        for(k=0;k<20&&st.action.phase!=CTX_IDLE;k++){if(st.action.phase==CTX_SECONDARY)impact();end_anim();}
        CHECK(k<20,1);CHECK(st.unit[U_RAMZA].mp,943);
    }
    // Nonreflectable producers remain unaffected by the shared status.
    prepare_spell(0x3f);change(10,SID_REFLECT);st.support[U_RAMZA]=SUPPORT_NON_CHARGE;
    CHECK(cast_order(U_RAMZA,10,0x3f,1),1);impact();CHECK(st.unit[10].hp,879);
    CHECK(st.action.pending,0);
}
static void new_producer_ai(void)
{
    u8 j,protected,k,found;u16 steps;
    for(j=0;j<sizeof reflected_spells/sizeof reflected_spells[0];j++)for(protected=0;protected<2;protected++){
        AbId ab=reflected_spells[j];reflect_prepare(ab);st.moved=1;W->ai_k=0;
        if(protected)change(10,SID_REFLECT);
        steps=0;while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);found=0;
        for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==ab)found=1;
        CHECK(found,!protected);CHECK(st.unit[U_RAMZA].mp,999);CHECK(st.unit[10].hp,999);
    }
}
static void reflection_trials(void){original_cases();spell_trials();producer_batch();late_status_and_expiry();no_target_and_restrictions();new_producer_ai();}
#ifndef __m68k__
int main(void){sw_init(61);reflection_trials();printf("reflect %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();reflection_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
