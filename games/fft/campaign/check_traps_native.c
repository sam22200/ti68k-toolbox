// Selected real NATURE/EQUIPMENT handoff and FSV5 VAT path; no hardware UI.
#define RT_DRAW_ROWS 100
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef off
#undef _main
#define _main trap_runtime_main
#define game_render trap_no_runtime_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void trap_no_runtime_render(void){}
#undef _main
#define _main __main
#else
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
#endif
#include "flow_battle.c"
#include "save.h"
static u16 checks,failures;static u32 digest=17;
static void check(u32 a,u32 b,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^a;
    if(a!=b){failures++;
#ifdef __m68k__
        BENCH_VALUE(line);
#else
        printf("trap-native line%u: %lu expected%lu\n",line,(unsigned long)a,(unsigned long)b);
#endif
    }
}
#define CHECK(a,b) check((u32)(a),(u32)(b),__LINE__)
static Unit *prepare(u8 x)
{
    Unit *u;u16 cell;
    game_scenario(200);CHECK(W!=RT_NULL,1);CHECK(CD(SOURCE_MAP),0);
    st.open_stage=st.open_fade=st.banner=st.open_handoff=st.end=0;st.morph_mask=st.scene_hide=0;
    st.act=st.sel=U_RAMZA;st.ai=AI_OFF;st.mode=M_MENU;st.moved=st.acted=st.pick_t=0;st.want=st.prot=255;
    u=&st.unit[U_RAMZA];u->job=0x4a;u->level=1;u->exp=23;
    memset(&u->statuses,0,sizeof u->statuses);memset(u->status_ct,0,sizeof u->status_ct);
    memset(u->eq,255,sizeof u->eq);st.movement[U_RAMZA]=0;st.reaction[U_RAMZA]=0;
    st.pending[U_RAMZA].state=DEFER_NONE;
    unit_status_refresh(u);u->hp=u->max_hp=100;u->mp=u->max_mp=20;
    cell=node_at(x,2,0);CHECK(cell<MAP_N,1);unit_place(u,cell);cursor_place(cell);
    return u;
}
static void screen(void)
{
    u8 i,plane;u16 y,x;u32 hash=17;
    for(i=0;i<16;i++){rt_frame++;W->hold=W->frz=0;W->actor_budget=255;game_render();}
    for(plane=0;plane<2;plane++)for(y=0;y<100;y++)for(x=0;x<20;x++)
        hash=(hash<<1|hash>>31)^((const u8 *)(plane?rt_dark:rt_light))[y*RT_PBYTES+x];
    digest=(digest<<1|digest>>31)^hash;
#ifdef __m68k__
    BENCH_VALUE(hash);
#else
    printf("trap-screen %08lX\n",(unsigned long)hash);
#endif
}
static void flush(void)
{
#ifdef __m68k__
    if(sv_data){save_write();sv_data=RT_NULL;}
#endif
}
static void trials(void)
{
    Unit *u;u8 i,k,slot;u32 before,after,raw[5];Save saved;
    CHECK(W!=RT_NULL,1);if(!W)return;
    for(i=0;i<4;i++){
        u=prepare((u8)(2+i));movement_settle(U_RAMZA);
        CHECK(st.mode,M_PICK);CHECK(W->trap_message,i+1);CHECK(W->move_post,0);
        CHECK(st.acted,0);CHECK(u->exp,23);CHECK(st.action.phase,CTX_IDLE);CHECK(W->rebuild,0);
        if(i==0){CHECK(u->level,1);CHECK((u->raw[0]&RAW_CURRENT)!=0,1);}
        if(i==1)CHECK(core_status_current(&u->statuses,4)&1,1);
        if(i==2)CHECK(core_status_current(&u->statuses,4)&16,16);
        if(i==3)CHECK(u->hp,80);
        screen();
    }
    u=prepare(5);st.reaction[U_RAMZA]=REACTION_MP_SWITCH;u->brave=100;u->mp=5;
    before=rng32;rand15();after=rng32;rng32=before;movement_settle(U_RAMZA);
    CHECK(u->hp,100);CHECK(u->mp,0);CHECK(rng32,after);
    u=prepare(5);u->hp=5;movement_settle(U_RAMZA);CHECK(u->hp,0);CHECK(u->dc,3);CHECK(u->kos,1);
    // Both directions of the resident/NATURE/EQUIPMENT boundary, including
    // deferred derived stats and a normal ACTION level-up after a job change.
    u=prepare(2);u->level=10;
    for(k=0;k<5;k++)raw[k]=core_raw_grown(u->raw[k],jobs[u->job].growth[k],10);
    movement_settle(U_RAMZA);CHECK(u->level,9);CHECK(W->rebuild,0);
    for(k=0;k<5;k++)CHECK(u->raw[k]&RAW_MAX,core_raw_level(raw[k],jobs[u->job].growth[k],9,1));
    memcpy(raw,u->raw,sizeof raw);u->job=0x4d;unit_stats(u);u->exp=99;
    rewards(U_RAMZA,U_RAMZA,0);CHECK(u->level,10);CHECK(W->rebuild,0);
    for(k=0;k<5;k++)CHECK(u->raw[k]&RAW_MAX,core_raw_level(raw[k]&RAW_MAX,jobs[u->job].growth[k],10,0));
    memcpy(raw,u->raw,sizeof raw);save_campaign();flush();CHECK(save_load(&saved),1);
    CHECK(!memcmp(saved.magic,"FSV5",4),1);party_load();slot=u->slot;
    CHECK(slot<saved.n,1);CHECK(save_raw_current(&saved,slot),1);CHECK(memcmp(raw,u->raw,sizeof raw),0);
    memcpy(saved.magic,"FSV4",4);CHECK(rt_save(SAVE_NAME,&saved,SAVE_V4_BYTES),1);flush();
    CHECK(save_load(&saved),1);CHECK(saved.raw_current[0]|saved.raw_current[1],0);
}
#ifndef __m68k__
int main(void){sw_init(200);trials();printf("trap-native %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){trap_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
