// Whole production Cure/Raise family and original bounded formula records.
#define REFLECT_CHECK
#include "check_offensive_magic.c"
#include "x/core-healing/oracle/source.h"
#define bn_width reference_bn_width
#define bn_line reference_bn_line
#define bn_shade1 reference_bn_shade1
#define bn_shade reference_bn_shade
#define bn_draw reference_bn_draw
#include "banner_reference.inc"
#undef bn_width
#undef bn_line
#undef bn_shade1
#undef bn_shade
#undef bn_draw
static const AbId healing[]={1,2,3,4,5,6,60,72};
static void prepare_heal(AbId ab)
{
    u8 k;prepare_spell(0x13);st.unit[U_RAMZA].set=ab<7?10:13;
    for(k=0;k<SET_N;k++)if(SET_ABILITY(st.unit[U_RAMZA].set,k)==ab)break;
    CHECK(k<SET_N,1);st.unit[U_RAMZA].learned=(LearnBits)1<<k;
    CHECK(known(U_RAMZA,0),ab);CHECK(known(U_RAMZA,1),NO_ABIL);
    st.unit[10].team=TEAM_PLAYER;st.unit[10].hp=100;st.unit[10].ct=77;
    st.unit[8].team=TEAM_ENEMY;st.unit[8].hp=100;
    if(abilities[ab].formula==13){st.unit[10].hp=0;st.unit[10].dc=1;st.unit[10].statuses.inflicted[0]=32;}
    unit_status_refresh(&st.unit[10]);
}
static void original_profiles(void)
{
    u16 i;u8 j;CastBatch b;u32 after;
    for(i=0;i<sizeof source_healing/sizeof source_healing[0];i++){
        const u16 *p=source_healing[i];Unit *a,*t;prepare_heal(p[0]);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->ma=p[1];a->faith=p[2];t->faith=p[3];st.support[U_RAMZA]=p[4]?SUPPORT_MAGIC_ATTACK_UP:0;
        st.support[10]=p[5]?SUPPORT_MAGIC_DEFENSE_UP:0;
        memset(&t->statuses,0,sizeof t->statuses);if(p[6])t->statuses.inflicted[3]|=16;
        if(p[7])t->statuses.inflicted[0]|=16;if(p[8])t->statuses.inflicted[0]|=32;
        if(p[9])t->statuses.innate[0]|=32;if(p[10])t->statuses.inflicted[1]|=128;
        if(p[11])t->statuses.inflicted[3]|=1;
        a->statuses.inflicted[4]=(p[12]&1?128:0)|(p[12]&2?64:0);
        t->statuses.inflicted[4]=(p[13]&1?128:0)|(p[13]&2?64:0);
        a->zodiac=p[14];t->zodiac=p[15];t->female=p[16];t->hp=p[8]?0:500;
        unit_status_refresh(a);unit_status_refresh(t);
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=10;b.follow=1;b.ability=p[0];
        CHECK(cast_forecast(&b),1);CHECK(b.victim[10].hit,p[20]?p[21]:0);
        if(p[20]){CHECK(b.victim[10].amount,p[18]);CHECK(b.victim[10].effect,p[19]);}
        rng32=1;for(j=0;j<p[22];j++)rand15();after=rng32;rng32=1;
        st.support[U_RAMZA]=p[4]?SUPPORT_MAGIC_ATTACK_UP:0;
        CHECK(cast_order(U_RAMZA,10,p[0],1),2);st.pending[U_RAMZA].ticks=0;
        CHECK(charge_resolve(),1);CHECK(W->cast.victim[10].hit,p[20]);
        CHECK(W->cast.victim[10].amount,p[18]);
        if(p[20])CHECK(W->cast.victim[10].effect,p[19]);
        // Original preview need not draw accuracy, native execution must.
        if(p[17])CHECK(rng32==after,1);
    }
}
static void producers(void)
{
    u8 j,follow,k;u16 expected,enemy;AbId ab;const u8 *r;
    for(j=0;j<8;j++)for(follow=0;follow<2;follow++){
        ab=healing[j];prepare_heal(ab);r=cj_range(common,ab);
        CHECK(cast_order(U_RAMZA,follow?10:second,ab,follow),2);
        CHECK(st.unit[U_RAMZA].mp,999);CHECK(st.pending[U_RAMZA].ticks,r[12]);
        expected=abilities[ab].formula==13?(r[10]==100?999:500):5*r[10];
        enemy=st.unit[8].hp;st.pending[U_RAMZA].ticks=0;ZB(61);CHECK(charge_resolve(),1);ZE(61);
        CHECK(st.unit[U_RAMZA].mp,999-r[13]);CHECK(W->cast.victim[10].amount,expected);
        ZB(62);impact();ZE(62);CHECK(st.unit[10].hp,abilities[ab].formula==13?expected:100+expected);
        CHECK(st.unit[10].ct,77);CHECK(core_status_current(&st.unit[10].statuses,0)&32,0);
        if(abilities[ab].formula==13)CHECK(st.unit[10].dc,3);
        CHECK(st.unit[8].hp,(r[4]&128)||abilities[ab].formula==13?enemy:enemy+expected);
        impact();CHECK(st.unit[10].hp,abilities[ab].formula==13?expected:100+expected);
        for(k=0;k<12&&st.action.phase!=CTX_IDLE;k++)end_anim();
        CHECK(st.action.phase,CTX_IDLE);CHECK(st.unit[U_RAMZA].mp,999-r[13]);
    }
}
static void resource_and_ai(void)
{
    u8 j,k,found;u16 steps;const u8 *r;
    for(j=0;j<8;j++){
        AbId ab=healing[j];prepare_heal(ab);r=cj_range(common,ab);
        st.unit[U_RAMZA].mp=r[13]-1;CHECK(cast_order(U_RAMZA,10,ab,1),0);CHECK(st.acted,0);
        st.support[U_RAMZA]=SUPPORT_HALF_MP;st.unit[U_RAMZA].mp=r[13]>>1;
        CHECK(cast_order(U_RAMZA,10,ab,1),2);st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);
        CHECK(st.unit[U_RAMZA].mp,0);
        prepare_heal(ab);change(U_RAMZA,SID_SILENCE);CHECK(cast_order(U_RAMZA,10,ab,1),0);
        prepare_heal(ab);st.moved=1;W->ai_k=0;steps=0;
        while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);found=0;
        for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==ab)found=1;
        CHECK(found,1);CHECK(st.unit[U_RAMZA].mp,999);
    }
}
static void exceptional_targets(void)
{
    u8 j,k;u16 hp;
    for(j=0;j<8;j++){
        AbId ab=healing[j];prepare_heal(ab);st.unit[10].hp=999;
        st.unit[10].statuses.inflicted[0]=16;st.unit[10].statuses.inflicted[4]=16;
        unit_status_refresh(&st.unit[10]);st.support[U_RAMZA]=SUPPORT_NON_CHARGE;
        CHECK(cast_order(U_RAMZA,10,ab,1),1);CHECK(W->cast.victim[10].effect,AB_DAMAGE);
        hp=st.unit[10].hp;impact();CHECK(st.unit[10].hp,hp-W->cast.victim[10].amount);
        CHECK(core_status_current(&st.unit[10].statuses,4)&16,0);
        prepare_heal(ab);st.support[U_RAMZA]=SUPPORT_NON_CHARGE;
        st.unit[10].hp=0;st.unit[10].statuses.inflicted[0]=16|32;unit_status_refresh(&st.unit[10]);
        CHECK(cast_order(U_RAMZA,10,ab,1),1);CHECK(W->cast.victim[10].hit,0);impact();CHECK(st.unit[10].hp,0);
        prepare_heal(ab);CHECK(cast_order(U_RAMZA,10,ab,1),2);
        st.unit[10].hp=0;st.unit[10].dc=DC_CRYSTAL;st.pending[U_RAMZA].ticks=0;
        CHECK(charge_resolve(),1);CHECK(W->cast.victim[10].hit,0);
        impact();CHECK(st.unit[10].hp,0);
        for(k=0;k<12&&st.action.phase!=CTX_IDLE;k++)end_anim();CHECK(st.action.phase,CTX_IDLE);
    }
}
static void reflected_healing(void)
{
    u8 j;u16 expected;
    for(j=0;j<6;j++){
        AbId ab=healing[j];if(!(cj_range(common,ab)[5]&128))continue;
        prepare_heal(ab);st.unit[10].hp=500;st.unit[10].statuses.inflicted[0]=0;
        if(abilities[ab].formula==13){st.unit[8].hp=0;st.unit[8].statuses.inflicted[0]=32;}
        st.unit[8].team=TEAM_PLAYER;change(10,SID_REFLECT);st.support[U_RAMZA]=SUPPORT_NON_CHARGE;
        CHECK(cast_order(U_RAMZA,10,ab,1),1);impact();CHECK(st.unit[10].hp,500);
        end_anim();CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.tgt,8);
        expected=abilities[ab].formula==13?(abilities[ab].y==100?999:500):5*abilities[ab].y;
        CHECK(st.dmg,expected);impact();CHECK(st.unit[8].hp,abilities[ab].formula==13?expected:100+2*expected);
    }
}
static u32 banner_hash(void)
{
    u32 h=17;u8 y,x,p;for(p=0;p<2;p++)for(y=0;y<100;y++)for(x=0;x<20;x++)
        h=(h<<1|h>>31)^((u8 *)(p?rt_dark:rt_light))[y*RT_PBYTES+x];return h;
}
static void banner_seed(void)
{
    u8 y,x;for(y=0;y<100;y++)for(x=0;x<20;x++){
        ((u8 *)rt_light)[y*RT_PBYTES+x]=x*17+y;((u8 *)rt_dark)[y*RT_PBYTES+x]=x+y*3;
    }
}
static void identical_banners(void)
{
    u8 kind,part,stage;u32 before;
    for(kind=0;kind<4;kind++){
        bn_start(kind);
        for(stage=0;stage<4;stage++)for(part=0;part<2;part++){
            st.banner_t=stage==0?1:stage==1?W->bn_t0[0]+2:stage==2?W->bn_end/2:W->bn_end-1;
            banner_seed();if(part)reference_bn_draw();else reference_bn_shade();before=banner_hash();
            banner_seed();ZB(63);if(part)bn_draw();else bn_shade();ZE(63);CHECK(banner_hash()==before,1);
        }
    }
}
static void healing_trials(void){original_profiles();producers();resource_and_ai();exceptional_targets();reflected_healing();identical_banners();}
#ifndef __m68k__
int main(void){sw_init(61);healing_trials();printf("healing %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();healing_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
