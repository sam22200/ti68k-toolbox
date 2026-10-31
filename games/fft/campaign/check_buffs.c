// Real production formula0a/0b family, controlled original traces and menu move.
#define REFLECT_CHECK
#include "check_offensive_magic.c"
#include "x/core-buffs/oracle/source.h"
#include "../status_policy.inc"
#include "../area_geometry.h"
#define win_lines reference_win_lines
#define window_label reference_window_label
#define make_window reference_make_window
#include "window_reference.inc"
#undef win_lines
#undef window_label
#undef make_window
static const AbId buffs[]={8,9,10,11,12,13,28,32,33,34,35,36,37,39,46,49,50,52,57,58,66,71};
static void prepare_buff(AbId ab)
{
    u8 j,k;prepare_spell(0x13);
    for(j=0x4a;j<0x5e;j++){
        st.unit[U_RAMZA].set=job_sets[j];
        for(k=0;k<SET_N;k++)if(SET_ABILITY(st.unit[U_RAMZA].set,k)==ab)break;
        if(k<SET_N)break;
    }
    CHECK(j<0x5e,1);st.unit[U_RAMZA].learned=(LearnBits)1<<k;
    CHECK(known(U_RAMZA,0),ab);CHECK(known(U_RAMZA,1),NO_ABIL);
    st.unit[10].team=abilities[ab].formula==11||ab==49?TEAM_PLAYER:TEAM_ENEMY;
    st.unit[10].set=11;st.unit[10].learned=1; // Faith utility requires a caster.
    st.unit[8].hp=0;st.unit[8].dc=DC_GONE;st.unit[8].x=255;
}
static void original_profiles(void)
{
    u16 i,c;u8 j;CastBatch b;u32 after;const u8 *r;
    for(i=0;i<sizeof source_buffs/sizeof source_buffs[0];i++){
        const u16 *p=source_buffs[i];Unit *a,*t;prepare_buff(p[0]);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->ma=p[1];a->faith=p[2];t->faith=p[3];st.support[U_RAMZA]=p[4]?SUPPORT_MAGIC_ATTACK_UP:0;
        st.support[10]=p[5]?SUPPORT_MAGIC_DEFENSE_UP:0;if(p[6])t->statuses.inflicted[3]|=16;
        a->zodiac=p[7];t->zodiac=p[8];t->female=p[9];t->eq[EQ_ACC]=p[11]?232:255;
        r=cj_section(common,CJ_STATUS)+6*abilities[p[0]].y;
        if(p[10])for(j=0;j<5;j++){
            if(p[10]==1)t->statuses.immune[j]|=r[j+1];
            if(p[10]==2)t->statuses.inflicted[j]|=r[j+1];
            if(p[10]==3)t->statuses.innate[j]|=r[j+1];
        }
        unit_status_refresh(a);unit_status_refresh(t);
        // Effect-stage fixture isolates one victim outside the caster's footprint.
        for(c=0;c<MAP_N;c++)if(abs((s16)(c%MAP_W)-t->x)+abs((s16)(c/MAP_W)-t->z)>4)break;
        CHECK(c<MAP_N,1);unit_place(a,c);
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=10;b.follow=1;b.ability=p[0];
        rng32=1;CHECK(cast_forecast(&b),1);CHECK(b.mask,(u16)1<<10);
        CHECK(b.victim[10].hit,p[12]?p[13]:0);CHECK(b.victim[10].amount,p[14]);CHECK(rng32==1,1);
        rng32=1;for(j=0;j<p[19];j++)rand15();after=rng32;rng32=1;
        b.phase=CAST_BATCH_EMPTY;ZB(61);CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);ZE(61);
        CHECK(b.victim[10].hit,p[16]);CHECK(b.victim[10].amount,p[18]);CHECK(rng32==after,1);
        CHECK(b.victim[10].effect,p[16]?AB_STATUS:AB_NULL);
    }
}
static void filter_equivalence(void)
{
    u8 j,mode,b,k;CoreStatusDelta d;u16 packed;const u8 *r;Unit *t;
    for(j=0;j<sizeof buffs/sizeof buffs[0];j++)for(mode=0;mode<9;mode++){
        prepare_buff(buffs[j]);t=&st.unit[10];r=cj_section(common,CJ_STATUS)+6*abilities[buffs[j]].y;
        memset(&d,0,sizeof d);memcpy(d.add,r+1,5);
        if(mode==1)memcpy(t->statuses.immune,r+1,5);
        if(mode==2)memcpy(t->statuses.inflicted,r+1,5);
        if(mode==3)memcpy(t->statuses.innate,r+1,5);
        if(mode==4)t->status_flags=STATUS_RIDER;
        if(mode==5)t->status_flags=STATUS_IMMORTAL;
        if(mode==6){t->statuses.inflicted[3]=4;t->statuses.inflicted[4]=64;}
        if(mode==7){t->statuses.inflicted[3]=8;t->statuses.inflicted[4]=128;}
        if(mode==8)t->statuses.immune[3]=16; // partial Wall succeeds for Protect.
        core_status_filter(&t->statuses,&d,t->status_flags,0,cj_status_rules(common));
        packed=0;for(b=0;b<5;b++)if(d.add[b])packed=((u16)b<<8)|d.add[b];
        CHECK(magic_heal(U_RAMZA,10,buffs[j],0),packed);
        for(k=0;k<16;k++)t->status_ct[k]=1;
    }
}
static void producers(void)
{
    u8 j,follow,b,mask,k;const u8 *r,*s;u16 packed,hp;
    for(j=0;j<sizeof buffs/sizeof buffs[0];j++)for(follow=0;follow<2;follow++){
        AbId ab=buffs[j];prepare_buff(ab);st.unit[U_RAMZA].ma=99;r=cj_range(common,ab);
        s=cj_section(common,CJ_STATUS)+6*abilities[ab].y;
        CHECK(abilities[ab].kind,AB_STATUS);packed=magic_heal(U_RAMZA,10,ab,0);CHECK(packed!=0,1);
        CHECK(cast_order(U_RAMZA,follow?10:second,ab,follow),2);
        CHECK(st.unit[U_RAMZA].mp,999);CHECK(st.pending[U_RAMZA].ticks,r[12]);
        st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);CHECK(st.unit[U_RAMZA].mp,999-r[13]);
        CHECK(W->cast.victim[10].effect,AB_STATUS);CHECK(W->cast.victim[10].hit,1);
        CHECK(W->cast.victim[10].amount,packed);hp=st.unit[10].hp;
        ZB(62);impact();ZE(62);CHECK(st.unit[10].hp,hp);
        for(b=0;b<5;b++)CHECK(core_status_current(&st.unit[10].statuses,b)&s[b+1],s[b+1]);
        for(b=3;b<5;b++)for(mask=128,k=0;mask;mask>>=1,k++)if(s[b+1]&mask){
            u8 id=(b<<3)+k;CHECK(st.unit[10].status_ct[id-24],cj_status_rules(common)[((u16)id<<4)+3]);
            st.unit[10].status_ct[id-24]=3;
        }
        impact();CHECK(st.unit[10].hp,hp);
        for(b=3;b<5;b++)for(mask=128,k=0;mask;mask>>=1,k++)if(s[b+1]&mask)CHECK(st.unit[10].status_ct[((b-3)<<3)+k],3);
        for(k=0;k<20&&st.action.phase!=CTX_IDLE;k++)end_anim();CHECK(st.action.phase,CTX_IDLE);
        CHECK(st.unit[U_RAMZA].mp,999-r[13]);
    }
}
static void eligibility_and_ai(void)
{
    u8 j,k,found;u16 steps;const u8 *r;
    for(j=0;j<sizeof buffs/sizeof buffs[0];j++){
        AbId ab=buffs[j];prepare_buff(ab);r=cj_range(common,ab);
        st.unit[U_RAMZA].mp=r[13]-1;CHECK(cast_order(U_RAMZA,10,ab,1),0);CHECK(st.acted,0);
        st.support[U_RAMZA]=SUPPORT_HALF_MP;st.unit[U_RAMZA].mp=r[13]>>1;
        CHECK(cast_order(U_RAMZA,10,ab,1),2);st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);
        CHECK(st.unit[U_RAMZA].mp,0);
        prepare_buff(ab);change(U_RAMZA,SID_SILENCE);CHECK(cast_order(U_RAMZA,10,ab,1),0);
        prepare_buff(ab);st.moved=1;W->ai_k=0;steps=0;
        while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);found=0;
        for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==ab)found=1;
        CHECK(found,1);CHECK(st.unit[U_RAMZA].mp,999);CHECK(st.unit[10].hp,999);
    }
}
static void reflected_status(void)
{
    u8 j,k;u16 packed,cost;
    for(j=0;j<sizeof buffs/sizeof buffs[0];j++){
        AbId ab=buffs[j];if(!(cj_range(common,ab)[5]&128))continue;
        prepare_buff(ab);st.unit[U_RAMZA].ma=99;
        st.unit[8]=st.unit[10];unit_place(&st.unit[8],third);change(10,SID_REFLECT);
        packed=magic_heal(U_RAMZA,8,ab,0); // Reflect itself may be refreshed.
        st.support[U_RAMZA]=SUPPORT_NON_CHARGE;cost=skill_cost(U_RAMZA,ab);
        CHECK(cast_order(U_RAMZA,10,ab,1),1);impact();CHECK(st.action.pending&((u16)1<<10),(u16)1<<10);
        memset(&st.unit[8].statuses,0,sizeof st.unit[8].statuses);change(8,SID_REFLECT);
        end_anim();CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.tgt,8);CHECK(st.abil,ab);
        CHECK(st.effect_kind,AB_STATUS);CHECK(st.hit,1);CHECK(st.dmg,packed);impact();
        CHECK(core_status_current(&st.unit[8].statuses,packed>>8)&(u8)packed,(u8)packed);
        CHECK(st.unit[8].hp,999);CHECK(st.action.pending&((u16)1<<8),0);
        for(k=0;k<20&&st.action.phase!=CTX_IDLE;k++){if(st.action.phase==CTX_SECONDARY)impact();end_anim();}
        CHECK(st.action.phase,CTX_IDLE);CHECK(st.unit[U_RAMZA].mp,999-cost);CHECK(st.unit[10].mp,999);
    }
}
static u32 window_hash(void)
{
    u32 h=17;u8 p,y,x;for(p=0;p<2;p++)for(y=0;y<WIN_ROWS;y++)for(x=0;x<8;x++)
        h=(h<<1|h>>31)^((u8 *)W->win[p])[y*RT_PBYTES+x];return h;
}
static void window_seed(void)
{
    u8 p,y,x;for(p=0;p<2;p++)for(y=0;y<WIN_ROWS;y++)for(x=0;x<8;x++)
        ((u8 *)W->win[p])[y*RT_PBYTES+x]=x+17*y+31*p;
}
static void same_windows(void)
{
    static const u8 modes[]={M_MENU,M_AUTO,M_QUIT,M_ACTS,M_ACT,M_SKILLS,M_ORDER};
    u8 j,n,w,h,m;u32 hash;void *l=rt_light,*d=rt_dark;
    for(j=0;j<sizeof modes;j++){
        prepare_buff(9);W->win_k[0]=modes[j];W->win_k[1]=0;W->win_k[2]=U_RAMZA;W->win_n=0;
        for(n=0;n<10;n++){
            window_seed();W->win_n=n;reference_make_window();hash=window_hash();w=W->win_w;h=W->win_h;m=W->win_m;
            window_seed();W->win_n=n;ZB(63);make_window();ZE(63);
            CHECK(window_hash()==hash,1);CHECK(W->win_w,w);CHECK(W->win_h,h);CHECK(W->win_m,m);
            CHECK(rt_light==l&&rt_dark==d,1);if(n+1>=m)break;
        }
    }
}
static void status_numbers(void)
{
    u8 j,k;u32 glyph;
    for(j=0;j<sizeof buffs/sizeof buffs[0];j++){
        prepare_buff(buffs[j]);st.unit[U_RAMZA].ma=99;st.support[U_RAMZA]=SUPPORT_NON_CHARGE;
        CHECK(cast_order(U_RAMZA,10,buffs[j],1),1);
        for(k=0;k<NUNIT+2;k++){
#ifdef FFT_MODULES
            mod_call(MOD_NUMBERS,2,0);
#else
            area_number_prepare();
#endif
        }
        CHECK(W->area_impact.ready&((u16)1<<10),(u16)1<<10);
        CHECK(W->area_impact.width[10]>0&&W->area_impact.width[10]<=32,1);
        glyph=0;for(k=0;k<7;k++)glyph|=W->area_impact.glyph[10][k];CHECK(glyph!=0,1);
    }
}
static void protect_profiles(void)
{
    u8 j;Unit *t;
    for(j=0;j<sizeof source_protect/sizeof source_protect[0];j++){
        const u16 *p=source_protect[j];prepare_buff(9);t=&st.unit[10];
        change(10,SID_PROTECT);if(p[1])change(10,SID_SLEEP);
        CHECK(unit_status_xa(t,p[0]),p[2]);
    }
}
static void buff_trials(void){original_profiles();filter_equivalence();producers();eligibility_and_ai();reflected_status();same_windows();}
#ifndef BUFF_CAPACITY_CHECK
#ifndef __m68k__
int main(void){sw_init(61);buff_trials();printf("buffs %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();buff_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif

#endif
