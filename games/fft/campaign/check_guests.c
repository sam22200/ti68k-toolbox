// Real guest binding, shared stat serialization and actual deferred native SAVE.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef off
#undef _main
#define _main guest_runtime_main
#define game_render guest_no_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void guest_no_render(void){}
#undef _main
#define _main __main
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "flow_battle.c"
#include "save.h"
#include "../guest_bank.h"
static u16 checks,failures;static u32 digest=17;
static void check(u32 a,u32 b,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^a;
    if(a!=b){failures++;
#ifdef __m68k__
        BENCH_VALUE(line);
#else
        printf("guest line%u: %lu expected%lu\n",line,(unsigned long)a,(unsigned long)b);
#endif
    }
}
#define CHECK(a,b) check((u32)(a),(u32)(b),__LINE__)
static void flush(void)
{
#ifdef __m68k__
    if(sv_data){save_write();sv_data=RT_NULL;}
#endif
}
static void persist(Save *s){CHECK(rt_save(SAVE_NAME,s,sizeof *s),1);flush();}
static void restore(void)
{
#ifdef FFT_MODULES
    mod_call(MOD_PARTY,15,0);
#else
    guests_restore();
#endif
}
static u8 bind(void)
{
#ifdef FFT_MODULES
    return (u8)mod_call(MOD_RESOURCES,16,1);
#else
    return guest_load(1);
#endif
}
static void trials(void)
{
    Save a,b;Unit *u;u8 i,g;u16 oldjp;u8 raw_bank[28];
    CHECK(sizeof(SaveUnit),176);CHECK(sizeof(Save),3918);
    CHECK(W!=RT_NULL,1);if(!W)return;
    CHECK(W->guest_key[0],4);CHECK(st.unit[0].team,TEAM_GUEST);CHECK(st.unit[0].slot,16);
    CHECK(save_load(&a),1);CHECK(a.n,16);CHECK(save_guest_find(&a,4),255);
    // Every old version keeps its original fields and gains four empty guests.
    for(i=0;i<4;i++){
        static const u16 sizes[]={SAVE_V2_BYTES,SAVE_V3_BYTES,SAVE_V4_BYTES,SAVE_V5_BYTES};
        memcpy(a.magic,"FSV2",4);a.magic[3]+=i;
        CHECK(rt_save(SAVE_NAME,&a,sizes[i]),1);flush();CHECK(save_load(&b),1);
        CHECK(memcmp(b.magic,SAVE_MAGIC,4),0);CHECK(b.n,16);
        CHECK(memcmp(b.unit,a.unit,sizeof a.unit),0);CHECK(save_guest_find(&b,4),255);CHECK(b.guest_raw,0);
    }
    memcpy(a.magic,SAVE_MAGIC,4);persist(&a);
    u=&st.unit[0];u->level=15;u->exp=99;u->jp=1234;u->tjp=2345;oldjp=u->jp;
    u->brave=(u8)(W->permanent_base[0][0]+7);u->faith=(u8)(W->permanent_base[0][1]-7);
    u->raw[0]|=RAW_CURRENT;u->raw[0]-=37; // same current-raw/Degenerator representation as players
    u->eq[2]=0xd1;st.reaction[0]=100;st.support[0]=0;
    save_campaign();flush();CHECK(save_load(&b),1);CHECK(b.n,16);g=save_guest_find(&b,4);CHECK(g,0);
    CHECK(b.guest[0].level,15);CHECK(b.guest[0].exp,99);CHECK(qword(b.guest[0].jp[0]),oldjp);
    CHECK(b.guest[0].eq[2],0xd1);CHECK(save_raw_current(&b,16),1);
    CHECK(b.guest[0].brave,u->brave);CHECK(b.guest[0].faith,u->faith);
    // Once settled, another save cannot reapply a quarter change or duplicate a guest.
    save_campaign();flush();CHECK(save_load(&a),1);CHECK(memcmp(&a,&b,sizeof a),0);
    // Cold reentry and a changed encounter slot recover the same identity/record.
    game_scenario(261);CHECK(W!=RT_NULL,1);if(!W)return;
    u=&st.unit[0];CHECK(u->level,15);CHECK(u->exp,99);CHECK(u->jp,1234);CHECK(u->tjp,2345);
    CHECK(u->brave,b.guest[0].brave);CHECK(u->faith,b.guest[0].faith);CHECK(u->eq[2],0xd1);
    CHECK((u->raw[0]&RAW_CURRENT)!=0,1);CHECK(u->team,TEAM_GUEST);
    CHECK(u->hp,u->max_hp);CHECK(u->dc,3);
    W->guest_key[0]=0;W->guest_key[2]=4;st.unit[2].team=TEAM_GUEST;restore();
    CHECK(st.unit[2].slot,16);CHECK(st.unit[2].jp,1234);CHECK(st.unit[2].exp,99);
    // The99-owned cap includes guest gear, also when the guest is in reserve.
    {u8 item=items[0xd1].id;u32 funds;
     b.guest[0].eq[2]=item;b.inv[item]=98;persist(&b);
     game_scenario(261);CHECK(W!=RT_NULL,1);if(!W)return;
     CHECK(save_owned_count(&battle_save,item),99);funds=qlong(b.money);
     st.stolen[st.nstolen++]=0xd1;save_campaign();flush();CHECK(save_load(&b),1);
     CHECK(b.inv[item],98);CHECK(save_owned_count(&b,item),99);
     CHECK(qlong(b.money),funds+(qword(qpart(flow_bank,Q_PRICES)+2*item)>>2));}
    // A guest absent from this encounter stays in reserve, with all other careers.
    b.guest[0].jp[3][0]=5;b.guest[0].jp[3][1]=77;persist(&b);
    game_scenario(261);CHECK(W!=RT_NULL,1);if(!W)return;
    memset(W->guest_key,0,sizeof W->guest_key);save_campaign();flush();
    CHECK(save_load(&a),1);CHECK(memcmp(a.guest,b.guest,sizeof a.guest),0);
    // All four slots can coexist with16 players; overflow fails before combat.
    for(i=1;i<GUEST_MAX;i++){a.guest_id[i][1]=(u8)(10+i);a.guest[i]=a.guest[0];}
    a.guest_id[0][1]=9;persist(&a);game_scenario(261);CHECK(W==RT_NULL,1);
    // Bind the existing fourth slot even when no free guest slot remains.
    a.guest_id[3][1]=4;persist(&a);chapter_now=0;game_scenario(261);CHECK(W!=RT_NULL,1);if(!W)return;
    CHECK(st.unit[0].slot,19);CHECK(st.unit[0].jp,1234);CHECK(a.n,16);
    // No malformed runtime role substitution, and no random positional fallback.
    st.unit[0].team=TEAM_ENEMY;CHECK(bind(),0);st.unit[0].team=TEAM_GUEST;
    CHECK(bind(),3);
    // In-memory story hooks are atomic with a caller's future flag update.
    b=a;CHECK(save_guest_recruit(&a,4),255);CHECK(memcmp(&a,&b,sizeof a),0);
    a.n=15;save_raw_set(&a,19,1);CHECK(save_guest_recruit(&a,4),15);CHECK(a.n,16);
    CHECK(save_guest_find(&a,4),255);CHECK(save_raw_current(&a,15),1);CHECK(save_raw_current(&a,19),0);
    CHECK(memcmp(&a.unit[15],&b.guest[3],sizeof(SaveUnit)),0);
    CHECK(save_guest_remove(&a,12),1);CHECK(save_guest_remove(&a,12),0);CHECK(a.n,16);
    CHECK(save_guests_valid(&a),1);a.guest_id[1][1]=9;CHECK(save_guests_valid(&a),0);
    b.guest_reserved=1;persist(&b);CHECK(save_load(&a),0);CHECK(a.n,0);
    // Parser boundaries are shared with shipping validation.
    memset(raw_bank,0,sizeof raw_bank);memcpy(raw_bank,"FGP1",4);raw_bank[5]=12;raw_bank[7]=1;
    raw_bank[8]='g';raw_bank[11]=4;CHECK(guest_bank_valid(raw_bank,12),1);
    raw_bank[9]=13;CHECK(guest_bank_valid(raw_bank,12),0);raw_bank[9]=0;
    raw_bank[5]=16;raw_bank[7]=2;memcpy(raw_bank+12,raw_bank+8,4);CHECK(guest_bank_valid(raw_bank,16),0);
}
#ifndef __m68k__
int main(void){sw_init(261);trials();printf("guest-runtime %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);final_release();return failures!=0;}
#else
void _main(void){guest_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
