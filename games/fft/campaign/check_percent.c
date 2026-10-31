// Production percent and HP/MP transfer families, against original paired packets.
#define REFLECT_CHECK
#include "check_offensive_magic.c"
#include "x/core-percent/oracle/source.h"
static const AbId percent[]={42,43,47,48,73};
static void prepare_percent(AbId ab)
{
    u8 j,k;prepare_spell(0x13);
    for(j=0x4a;j<0x5e;j++){
        st.unit[U_RAMZA].set=job_sets[j];
        for(k=0;k<SET_N;k++)if(SET_ABILITY(st.unit[U_RAMZA].set,k)==ab)break;
        if(k<SET_N)break;
    }
    CHECK(j<0x5e,1);st.unit[U_RAMZA].learned=(LearnBits)1<<k;
    CHECK(known(U_RAMZA,0),ab);CHECK(known(U_RAMZA,1),NO_ABIL);
    st.unit[U_RAMZA].hp=100;st.unit[10].team=TEAM_ENEMY;
    st.unit[8].hp=0;st.unit[8].x=255;st.unit[8].dc=DC_GONE;
}
static void original_profiles(void)
{
    u16 i,c;u8 j,m;u32 after;CastBatch b;
    for(i=0;i<sizeof source_percent/sizeof source_percent[0];i++)for(m=0;m<2;m++){
        const u16 *p=source_percent[i],*v=p+17+12*m;Unit *a,*t;u8 kind;u16 amount;
        prepare_percent(p[0]);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->ma=p[1];a->faith=p[2];t->faith=p[3];
        st.support[U_RAMZA]=p[4]?SUPPORT_MAGIC_ATTACK_UP:0;st.support[10]=p[5]?SUPPORT_MAGIC_DEFENSE_UP:0;
        t->statuses.inflicted[3]=p[6]?16:0;a->zodiac=p[7];t->zodiac=p[8];t->female=p[9];
        t->statuses.inflicted[0]=p[10]?16:0;t->max_hp=p[11];t->max_mp=p[12];t->hp=p[13];t->mp=p[14];
        t->eq[EQ_ACC]=p[15]?232:255;
        for(j=0;j<4;j++)st.affinity[10][j]=p[16]&(1<<j)?abilities[p[0]].element:0;
        st.affinity[U_RAMZA][4]=p[16]&16?abilities[p[0]].element:0;
        unit_status_refresh(a);unit_status_refresh(t);
        for(c=0;c<MAP_N;c++)if(abs(TILE_X[c]-t->x)+abs(TILE_Z[c]-t->z)>4)break;
        CHECK(c<MAP_N,1);unit_place(a,c);
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=10;b.follow=1;b.ability=p[0];
        rng32=1;for(j=0;j<v[11];j++)rand15();after=rng32;rng32=1;
        ZB(61);CHECK(m?cast_batch(CAST_BATCH_PREPARE,&b):cast_forecast(&b),1);ZE(61);
        CHECK(b.victim[10].hit,m?v[0]:v[0]?v[1]:0);
        kind=p[0]==47?AB_MP_DAMAGE:v[3]?AB_HEAL:AB_DAMAGE;
        amount=p[0]==47?v[4]:v[2]+v[3];
        CHECK(b.victim[10].amount,v[0]?amount:0);
        CHECK(b.victim[10].effect,v[0]?kind:AB_NULL);
        CHECK(rng32==(m?after:1),1);
        if(magic_transfer_formula(abilities[p[0]].formula)&&v[6]){
            CHECK(b.mask&((u16)1<<U_RAMZA),(u16)1<<U_RAMZA);
            CHECK(b.victim[U_RAMZA].hit,m?1:v[1]);
            CHECK(b.victim[U_RAMZA].amount,v[7]+v[8]+v[9]+v[10]);
            CHECK(b.victim[U_RAMZA].effect,EFFECT_RETURN|(p[0]==47?AB_MP_HEAL:v[7]?AB_DAMAGE:AB_HEAL));
        }else CHECK(b.mask&((u16)1<<U_RAMZA),0);
    }
}
static void production(void)
{
    static const u16 source_amounts[5]={250,500,330,250,500};
    u8 j,follow,k;const u8 *r;u16 hp,mp,ahp,amp,amount;
    for(j=0;j<5;j++)for(follow=0;follow<2;follow++){
        AbId ab=percent[j];prepare_percent(ab);r=cj_range(common,ab);st.unit[U_RAMZA].ma=99;
        st.unit[10].hp=1;st.unit[10].mp=0;st.unit[U_RAMZA].mp=100;
        hp=1;mp=0;ahp=st.unit[U_RAMZA].hp;amount=source_amounts[j];
        CHECK(cast_order(U_RAMZA,follow?10:second,ab,follow),2);
        CHECK(st.unit[U_RAMZA].mp,100);CHECK(st.pending[U_RAMZA].ticks,r[12]);
        st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);CHECK(st.unit[U_RAMZA].mp,100-r[13]);
        CHECK(W->cast.victim[10].hit,1);CHECK(W->cast.victim[10].amount,amount);
        ZB(62);impact();ZE(62);
        CHECK(st.unit[10].hp,ab==47?hp:0);CHECK(st.unit[10].mp,0);
        CHECK(st.unit[U_RAMZA].hp,ab==42||ab==43?0:ab==48?ahp+amount:ahp);
        CHECK(st.unit[U_RAMZA].mp,ab==47?100-r[13]+amount:100-r[13]);
        if(ab==47)CHECK(st.primary.reward,0);
        hp=st.unit[10].hp;ahp=st.unit[U_RAMZA].hp;amp=st.unit[U_RAMZA].mp;
        impact();CHECK(st.unit[10].hp,hp);CHECK(st.unit[U_RAMZA].hp,ahp);CHECK(st.unit[U_RAMZA].mp,amp);
        for(k=0;k<20&&st.action.phase!=CTX_IDLE;k++)end_anim();CHECK(st.action.phase,CTX_IDLE);
    }
}
static void eligibility_ai(void)
{
    u8 j,k,found;u16 steps;const u8 *r;
    for(j=0;j<5;j++){
        AbId ab=percent[j];prepare_percent(ab);r=cj_range(common,ab);
        st.unit[U_RAMZA].mp=r[13]-1;CHECK(cast_order(U_RAMZA,10,ab,1),0);CHECK(st.acted,0);
        prepare_percent(ab);change(U_RAMZA,SID_SILENCE);CHECK(cast_order(U_RAMZA,10,ab,1),0);
        prepare_percent(ab);CHECK(cast_order(U_RAMZA,U_RAMZA,ab,1),magic_transfer_formula(r[8])?0:2);
        prepare_percent(ab);if(ab==47)st.unit[U_RAMZA].mp=100;st.unit[10].mp=500;st.moved=1;W->ai_k=0;steps=0;
        while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);found=0;
        for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==ab)found=1;
        CHECK(found,1);CHECK(st.unit[U_RAMZA].hp,100);CHECK(st.unit[U_RAMZA].mp,ab==47?100:999);
    }
}
static void reversal_and_sleep(void)
{
    u8 mode;u16 amount,hp;
    for(mode=0;mode<4;mode++){
        prepare_percent(48);st.unit[U_RAMZA].ma=99;st.support[U_RAMZA]=SUPPORT_NON_CHARGE;
        st.unit[10].statuses.inflicted[0]=16;unit_status_refresh(&st.unit[10]);st.unit[10].hp=100;
        st.unit[U_RAMZA].hp=mode==0?1:mode==1?999:100;hp=st.unit[U_RAMZA].hp;
        if(mode==2)change(U_RAMZA,SID_SLEEP);
        // Sleep prohibits starting a spell; apply it only after preparation.
        if(mode==2){st.unit[U_RAMZA].statuses.inflicted[4]=0;unit_status_refresh(&st.unit[U_RAMZA]);}
        CHECK(cast_order(U_RAMZA,10,48,1),1);amount=W->cast.victim[10].amount;
        if(mode==2)change(U_RAMZA,SID_SLEEP);
        impact();CHECK(st.unit[10].hp,100+amount);
        CHECK(st.unit[U_RAMZA].hp,hp>amount?hp-amount:0);
        CHECK(core_status_current(&st.unit[U_RAMZA].statuses,4)&16,0);
        if(mode==0){CHECK(st.unit[U_RAMZA].dc,3);CHECK(st.end,END_NONE);}
    }
    prepare_percent(47);st.unit[U_RAMZA].ma=99;st.support[U_RAMZA]=SUPPORT_NON_CHARGE;
    change(10,SID_SLEEP);CHECK(cast_order(U_RAMZA,10,47,1),1);impact();
    CHECK(core_status_current(&st.unit[10].statuses,4)&16,16);CHECK(st.unit[10].hp,999);
}
static void presentation_and_boot(void)
{
    u8 k;char text[40];u32 target=0,caster=0;u16 size;const u8 *bank=common;
    prepare_percent(47);st.unit[U_RAMZA].ma=99;st.support[U_RAMZA]=SUPPORT_NON_CHARGE;
    forecast_text(U_RAMZA,10,47,text);CHECK(memcmp(text,"-330MP ",7),0);CHECK(memcmp(text+7,"100%",5),0);
    CHECK(cast_order(U_RAMZA,10,47,1),1);
    for(k=0;k<NUNIT+2;k++){
#ifdef FFT_MODULES
        mod_call(MOD_NUMBERS,2,0);
#else
        area_number_prepare();
#endif
    }
    CHECK(W->area_impact.ready&((u16)1<<10),(u16)1<<10);
    CHECK(W->area_impact.ready&((u16)1<<U_RAMZA),(u16)1<<U_RAMZA);
    for(k=0;k<7;k++){target|=W->area_impact.glyph[10][k];caster|=W->area_impact.glyph[U_RAMZA][k];}
    CHECK(target!=0&&caster!=0,1);
    CHECK(memcmp(W->area_impact.glyph[10],W->area_impact.glyph[U_RAMZA],7*sizeof(u32))!=0,1);
    // The same validator now runs natively before any Work buffer exists.
    rt_file("ffcj",&size);
    for(k=0;k<4;k++){
        u16 bytes=k==0?size:k==1?0:k==2?31:size-1;u8 valid;
#ifdef FFT_MODULES
        mod_call(MOD_SAVE,255,0);mod_context(MOD_SAVE)->w=RT_NULL;
        mod_context(MOD_SAVE)->common=k==3?RT_NULL:bank;mod_context(MOD_SAVE)->desc_size=bytes;
        valid=(u8)mod_call(MOD_SAVE,5,0);mod_id=0;
#else
        valid=cj_valid(k==3?RT_NULL:bank,bytes);
#endif
        CHECK(valid,k==0);
    }
}
static void percent_trials(void){original_profiles();}
#ifndef PERCENT_RUNTIME_CHECK
#ifndef __m68k__
int main(void){sw_init(61);percent_trials();printf("percent %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();percent_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif

#endif
