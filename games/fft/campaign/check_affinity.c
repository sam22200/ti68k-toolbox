// Source-backed archive/cache producers; no elemental formula enablement.
#define REACTION_SEQUENCE_CHECK
#include "check_reactions.c"
#include "x/core-affinity/oracle/source.h"
static void source_cache(void)
{
    u16 n;u8 i,expected[5];Unit *u;
    reset();
    for(n=0;n<sizeof source_affinity/sizeof source_affinity[0];n++){
        const u8 *p=source_affinity[n];u=&st.unit[n%NUNIT];u->job=p[0];memcpy(u->eq,p+1,5);
        memset(st.affinity[n%NUNIT],255,5);
        cj_affinity(common,u->job,u->eq,expected);
        ZB(43);unit_stats(u);ZE(43);
        for(i=0;i<5;i++){CHECK(expected[i],p[6+i]);CHECK(st.affinity[n%NUNIT][i],p[6+i]);}
    }
    // Removing equipment/job changes replaces the cache, not ORs over old traits.
    u=&st.unit[U_RAMZA];u->job=0x4a;memset(u->eq,255,5);unit_stats(u);
    for(i=0;i<5;i++)CHECK(st.affinity[U_RAMZA][i],0);
    // The stat API also accepts scratch Units without overwriting a live cache.
    {Unit scratch=*u;u8 snapshot[UNIT_CAPACITY][5];
        memcpy(snapshot,st.affinity,sizeof snapshot);unit_stats(&scratch);
        CHECK(memcmp(snapshot,st.affinity,sizeof snapshot),0);CHECK(scratch.ma,u->ma);}
}
static void validators(void)
{
    u16 size=cj_word(common+CJ_LENGTH),off;u8 *bad=malloc(size);const u8 *p;
    CHECK(bad!=RT_NULL,1);if(!bad)return;
    CHECK(cj_valid(common,size),1);memcpy(bad,common,size);memcpy(bad,"FCJ8",4);CHECK(cj_valid(bad,size),0);
    off=(u16)(cj_section(common,CJ_SPECIAL)-common)+CJ_AFFINITY;p=common+off;
    memcpy(bad,common,size);bad[off+4]=0;CHECK(cj_valid(bad,size),0);
    memcpy(bad,common,size);bad[off+5]=1;CHECK(cj_valid(bad,size),0);
    memcpy(bad,common,size);bad[off+CJ_AFFINITY_INDEX]=p[4];CHECK(cj_valid(bad,size),0);
    memcpy(bad,common,size);bad[off+CJ_AFFINITY_INDEX+415]=p[4];CHECK(cj_valid(bad,size),0);
    memcpy(bad,common,size);bad[off+CJ_AFFINITY_ROWS]=1;CHECK(cj_valid(bad,size),0);
    memcpy(bad,common,size);bad[off+4]=255;CHECK(cj_valid(bad,size),0);
    CHECK(cj_valid(common,size-1),0);free(bad);
}
static void local_levelup(u8 reaction)
{
    u8 i,expected[5],j;Unit *a;
    arena();a=&st.unit[U_RAMZA];a->job=0x4a;memcpy(a->eq,source_affinity[160]+1,5);
    unit_stats(a);cj_affinity(common,a->job,a->eq,expected);a->exp=99;
    st.unit[10].hp=st.unit[10].max_hp=999;
    if(reaction){st.reaction[10]=REACTION_POTION;st.bag[0]=3;}
    attack(U_RAMZA,10);st.hit=1;st.dmg=1;st.push=0;
    ZB(44);impact();ZE(44);
    if(!reaction){
#ifdef FFT_MODULES
        // Immediate level-up has already drained its derived-stat rebuild.
        CHECK(mod_id==MOD_EQUIPMENT+1,1);
#else
        CHECK(1,1);
#endif
    }
    // Settlement returns before the cold equipment owner rebuilds derived stats.
    for(j=0;j<8&&a->level==a->lv0;j++){
        ZB(45);end_anim();ZE(45);
    }
    CHECK(a->level,a->lv0+1);CHECK(st.lvup,1);
#ifdef FFT_MODULES
    if(reaction)CHECK(mod_id==MOD_EQUIPMENT+1,1);
#else
    if(reaction)CHECK(1,1);
#endif
    CHECK(W->rebuild,0);
    for(i=0;i<5;i++)CHECK(st.affinity[U_RAMZA][i],expected[i]);
}
static void loaded_party(void)
{
    u8 i,k,expected[5];reset();party_load();CHECK(st.party_in,1);
    for(i=0;i<NUNIT;i++){
        if(st.unit[i].slot==255)continue;
        cj_affinity(common,st.unit[i].job,st.unit[i].eq,expected);
        for(k=0;k<5;k++)CHECK(st.affinity[i][k],expected[k]);
    }
}
static void affinity_trials(void){source_cache();validators();local_levelup(0);local_levelup(1);loaded_party();}
#ifndef __m68k__
int main(void){sw_init(61);affinity_trials();printf("affinity %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();affinity_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
