// Source weather/Oil/Float through shared query, command, impact and packet ownership.
#define INCANTATION_CHECK
#include "check_casting.c"
static const u8 elements[9]={128,64,32,8,192,224,2,0,136};
#include "fixture_monster.h"
static AbId carrier(const u16 *p)
{
    u8 k;for(k=0;k<9;k++)if(elements[k]==p[2])break;
    return (p[1]==255?0x110:0x100)+k;
}
static void weather_prepare(const u16 *p)
{
    Unit *a,*t;prepared();a=&st.unit[U_RAMZA];t=&st.unit[10];
    a->ma=p[0];a->faith=p[13];t->faith=p[14];a->zodiac=t->zodiac=0;a->mp=999;t->hp=100;
    st.support[U_RAMZA]=st.support[10]=st.reaction[10]=0;
    memset(&a->statuses,0,sizeof a->statuses);memset(&t->statuses,0,sizeof t->statuses);
    memset(st.affinity[U_RAMZA],0,5);memset(st.affinity[10],0,5);
    if(p[5]==2||p[5]==3)t->statuses.innate[2]=128;
    if(p[5]==1||p[5]==3||p[5]==4||p[5]==5)t->statuses.inflicted[2]=128;
    if(p[5]==4)t->statuses.immune[2]=128;
    if(p[6]||p[5]==5)t->statuses.inflicted[2]|=64;
    st.affinity[10][0]=p[7]?p[2]:0;st.affinity[10][1]=p[8]?p[2]:0;
    st.affinity[10][2]=p[9]?p[2]:0;st.affinity[10][3]=p[10]?p[2]:0;
    st.affinity[U_RAMZA][4]=p[11]?p[2]:0;
    unit_status_refresh(a);unit_status_refresh(t);CHECK(battle_weather(p[3],p[4]),1);
}
static void source_profiles(void)
{
    u16 size,count,n,p[25],raw,amount,hp;u8 i,kind,oil;u32 seed,hash,after;
    const u8 *source=rt_file("fwq00",&size);char text[40];AbId ab;
    CHECK(source!=RT_NULL,1);if(!source)return;count=RD16(source,2);CHECK(count,254);
    for(n=0;n<count;n++){
        for(i=0;i<25;i++)p[i]=RD16(source+6+50*n,i);
        // The power-255 overflow profiles are not representable: bit 7 of the power marks
        // formula 0x4E (no Faith); retail powers stay below 128 (max 96).
        if(p[1]>127)continue;
        weather_prepare(p);fixture_monster(U_RAMZA,carrier(p));ab=carrier(p);CHECK(st.weather,p[15]);seed=rng32;hash=state_hash();
        kind=skill_kind(U_RAMZA,10,ab);CHECK(kind,p[17]&128?AB_DAMAGE:p[17]&64?AB_HEAL:AB_NULL);
        ZB(55);CHECK(magic_amount(U_RAMZA,10,(p[2]<<8)|p[1]),p[24]);ZE(55);
        CHECK(state_hash()==hash,1);
        amount=skill_amount(U_RAMZA,10,ab,0);CHECK(amount,p[17]==64?p[19]:p[18]);
        CHECK(skill_hit(U_RAMZA,10,ab),p[16]*100);CHECK(rng32==seed,1);
        ZB(56);forecast_text(U_RAMZA,10,ab,text);ZE(56);CHECK(rng32==seed,1);
        CHECK(st.unit[10].statuses.inflicted[2]&128,p[5]==1||p[5]==3||p[5]==4||p[5]==5?128:0);
        CHECK(magic_hp(kind,100,999,amount),p[20]);
        if(!p[12])continue;
        for(i=0;i<p[21];i++)rand15();after=rng32;rng32=seed;
        st.open_resolving=1;ZB(57);skill(U_RAMZA,10,ab);ZE(57);
        CHECK(st.hit,p[16]);CHECK(st.dmg,amount);CHECK(rng32==after,1);CHECK(st.push|st.crit,0);
        CHECK(st.effect_kind&EFFECT_KIND_MASK,kind);
        CHECK((st.effect_kind&EFFECT_OIL)!=0,p[16]&&p[22]);
        CHECK(st.unit[U_RAMZA].mp,999-skill_cost(U_RAMZA,ab));hp=st.unit[10].hp;
        ZB(58);impact();ZE(58);CHECK(st.unit[10].hp,p[20]);
        oil=p[5]!=0; // source's existing KO cancellation mask is tested separately
        if(p[16]&&p[22])oil=p[5]==2||p[5]==3;
        if(!p[20]&&(cj_status_rules(common)[(SID_DEAD<<4)+8]&128))oil=p[5]==2||p[5]==3;
        CHECK((core_status_current(&st.unit[10].statuses,2)&128)!=0,oil);
        impact();CHECK(st.unit[10].hp,p[20]);CHECK(st.unit[U_RAMZA].mp,999-skill_cost(U_RAMZA,ab));
    }
}
static void weather_boundary(void)
{
    u16 p[15]={5,14,128,3,0,0,0,0,0,0,0,0,1,70,70};u8 i,flags;
    weather_prepare(p);fixture_monster(U_RAMZA,carrier(p));
    for(i=0;i<5;i++)for(flags=0;flags<4;flags++){
        CHECK(battle_weather(i,flags),1);CHECK(st.weather,core_weather(i,flags));
    }
    st.weather=7;CHECK(battle_weather(5,0),0);CHECK(st.weather,7);
    CHECK(battle_weather(3,4),0);CHECK(st.weather,7);
    // Weather changes invalidate a forecast/AI query, not a prepared action.
    W->fc_key=12;W->ai_k=3;st.ai=AI_THINK;CHECK(battle_weather(3,0),1);CHECK(W->fc_key,0);CHECK(W->ai_k,0);
    st.ai=AI_OFF;CHECK(battle_weather(0,0),1);st.open_resolving=1;skill(U_RAMZA,10,carrier(p));CHECK(st.dmg,34);
    CHECK(battle_weather(3,0),1);CHECK(skill_amount(U_RAMZA,10,carrier(p),0),25);CHECK(st.dmg,34);
    impact();CHECK(st.unit[10].hp,66);
}
static void oil_latch_and_packet(void)
{
    u16 p[15]={5,14,128,0,0,1,0,0,0,0,0,0,1,70,70};AbId ab=carrier(p);char text[40];
    weather_prepare(p);fixture_monster(U_RAMZA,carrier(p));st.open_resolving=1;skill(U_RAMZA,10,ab);CHECK(st.effect_kind&EFFECT_OIL,EFFECT_OIL);
    // A query on another target must not overwrite the prepared removal/category.
    forecast_text(U_RAMZA,U_RAMZA,ab,text);CHECK(st.effect_kind&EFFECT_OIL,EFFECT_OIL);
    CHECK(battle_weather(4,0),1);impact();CHECK(st.unit[10].statuses.inflicted[2]&128,0);CHECK(st.unit[10].hp,66);
    // Native packet transport through a real Auto-Potion root response.
    weather_prepare(p);fixture_monster(U_RAMZA,carrier(p));st.reaction[10]=REACTION_POTION;st.open_resolving=1;skill(U_RAMZA,10,ab);impact();end_anim();
    CHECK(st.primary.running,1);CHECK(st.primary.effect_kind&EFFECT_OIL,EFFECT_OIL);CHECK(st.effect_kind&EFFECT_OIL,0);
    impact();end_anim();CHECK(st.effect_kind&EFFECT_OIL,EFFECT_OIL);CHECK(act_kind(),AB_DAMAGE);
    // Added after preparation: not a source removal packet. Protected at impact: retain it.
    p[5]=0;weather_prepare(p);fixture_monster(U_RAMZA,carrier(p));st.open_resolving=1;skill(U_RAMZA,10,ab);st.unit[10].statuses.inflicted[2]|=128;
    impact();CHECK(st.unit[10].statuses.inflicted[2]&128,128);
    p[5]=1;weather_prepare(p);fixture_monster(U_RAMZA,carrier(p));st.open_resolving=1;skill(U_RAMZA,10,ab);st.unit[10].statuses.innate[2]|=128;
    impact();CHECK(st.unit[10].statuses.inflicted[2]&128,128);
}
static void live_weather_and_ai(void)
{
    u16 p[15]={5,14,128,0,0,1,0,0,0,0,0,0,1,70,70},steps;u32 seed,after;
    u8 i,found; s16 ranks[2];
    weather_prepare(p);memset(st.unit[10].eq,255,5);st.acted=0;
    seed=rng32;for(i=0;i<5;i++)rand15();after=rng32;rng32=seed;
    CHECK(cast_order(U_RAMZA,10,0x10,1),2);CHECK(st.unit[U_RAMZA].mp,999);
    CHECK(battle_weather(3,0),1);st.pending[U_RAMZA].ticks=0;
    CHECK(charge_resolve(),1);CHECK(st.dmg,25);CHECK(rng32==after,1);CHECK(st.effect_kind&EFFECT_OIL,EFFECT_OIL);
    CHECK(battle_weather(0,0),1);CHECK(st.dmg,25);impact();CHECK(st.unit[10].hp,75);
    CHECK(st.unit[10].statuses.inflicted[2]&128,0);
    for(i=0;i<2;i++){
        weather_prepare(p);memset(st.unit[10].eq,255,5);st.unit[10].hp=999;
        st.unit[U_RAMZA].set=11;st.unit[U_RAMZA].learned=1;st.unit[U_RAMZA].defend=0; // isolate Fire from the bounded top-eight ranking
        st.acted=0;st.moved=1;W->ai_k=0;battle_weather(i?3:0,0);
        steps=0;while(!ai_think()&&++steps<1024){}CHECK(steps<1024,1);found=0;
        for(seed=0;seed<W->ai_n;seed++)if(W->ai_rank[seed].ab==0x10){found=1;ranks[i]=W->ai_rank[seed].pri;}
        CHECK(found,1);
    }
    CHECK(ranks[1]<ranks[0],1);
}
static void menu_sizing(void)
{
    u16 p[15]={5,14,128,0,0,0,0,0,0,0,0,0,1,70,70};weather_prepare(p);fixture_monster(U_RAMZA,carrier(p));
    W->win_k[0]=M_MENU;W->win_k[1]=0;W->win_k[2]=U_RAMZA;W->win_n=0;
#ifdef FFT_MODULES
    mod_call(MOD_ACTION,255,0);
#endif
    ZB(59);make_window();ZE(59);CHECK(W->win_n,1);CHECK(W->win_m,MN_N+1);
    CHECK(W->win_m-1,MN_N);
    W->win_k[0]=M_AUTO;W->win_n=0;make_window();CHECK(W->win_m-1,3);
    W->win_k[0]=M_QUIT;W->win_n=0;make_window();CHECK(W->win_m-1,3);
}
static u32 cache_hashes[4];
static void forecast_caches(void)
{
    u16 p[15]={5,14,128,3,0,0,0,0,0,0,0,0,1,70,70},j;u8 i;const u8 *q;
    for(i=0;i<4;i++){
        p[2]=i==2?8:128;p[3]=i==1?0:3;p[6]=i==2;p[7]=i==3;
        weather_prepare(p);fixture_monster(U_RAMZA,carrier(p));
#ifdef FFT_MODULES
        mod_call(MOD_RESULTS,255,0);
#endif
        ZB(60);make_forecast(U_RAMZA,10,carrier(p));ZE(60);
        cache_hashes[i]=17;q=(const u8 *)W->forecast;
        for(j=0;j<sizeof W->forecast;j++)cache_hashes[i]=(cache_hashes[i]<<1|cache_hashes[i]>>31)^*q++;
#ifndef __m68k__
        printf("forecastcache %u %08lX\n",i,(unsigned long)cache_hashes[i]);
#else
        BENCH_VALUE(cache_hashes[i]);
#endif
    }
}
static void weather_trials(void){source_profiles();weather_boundary();oil_latch_and_packet();live_weather_and_ai();menu_sizing();forecast_caches();}
#ifndef __m68k__
int main(void){sw_init(61);weather_trials();printf("weather %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();weather_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
