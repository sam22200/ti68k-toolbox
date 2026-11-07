// Private migrated save round trip through the real victory writer, PC and native VAT.
#define EQUIPMENT_UI_CHECK
#include "check_equipment.c"
#include "save.h"
static void persist(void)
{
#ifdef FFT_MODULES
 mod_call(MOD_SAVE,0,0);
#else
 save_campaign();
#endif
}
static void save_trials(void)
{
 Unit *u;u8 i,eq[5];u16 k;u32 h;
 arena();CHECK(save_load(&battle_save),1);CHECK(battle_save.n,1);
 for(i=0;i<NUNIT;i++)st.unit[i].slot=255;
 st.party_in=1;st.reward_ready=st.nfound=st.nstolen=0;u=&st.unit[U_RAMZA];u->slot=0;u->job=76;u->hp=3;u->mp=2;
 for(i=0;i<5;i++)u->eq[i]=battle_save.unit[0].eq[i]?battle_save.unit[0].eq[i]:255;
 for(i=0;i<BAG_MAX;i++)st.bag[i]=battle_save.inv[240+i];
 st.support[U_RAMZA]=480;memcpy(eq,u->eq,5);eq[3]=20;CHECK(commit(eq),1);CHECK(battle_save.inv[19],1);CHECK(battle_save.inv[20],0);
 battle_save.inv[112]=2;battle_save.inv[38]=1;qput(battle_save.money,1017);u->exp=23;u->jp=99;u->tjp=250;
 persist();CHECK(battle_save.n,1);CHECK(battle_save.unit[0].eq[3],20);CHECK(battle_save.inv[19],1);CHECK(battle_save.inv[20],0);
 CHECK(battle_save.inv[112],2);CHECK(battle_save.inv[38],1);CHECK(qlong(battle_save.money),1017);
 CHECK(battle_save.unit[0].exp,23);CHECK(cj_word(battle_save.unit[0].jp[2]),99);CHECK(cj_word(battle_save.unit[0].tjp[2]),250);
 h=17;for(k=0;k<sizeof(Save);k++)h=(h<<1|h>>31)^((u8 *)&battle_save)[k];
 persist();{u32 again=17;for(k=0;k<sizeof(Save);k++)again=(again<<1|again>>31)^((u8 *)&battle_save)[k];CHECK(h==again,1);}
 digest=(digest<<1|digest>>31)^h;
#ifdef __m68k__
 save_write();
#endif
}
#ifndef __m68k__
int main(void){sw_init(61);save_trials();printf("equipment_save %08lX failures %lu checks %lu\n",(unsigned long)digest,(unsigned long)failures,(unsigned long)checks);return failures!=0;}
#else
void _main(void){equipment_runtime_main();save_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
