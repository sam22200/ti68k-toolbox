// Actual cold settlement, source arithmetic and isolated save files; no UI needed.
#include "flow_battle.c"
#include "save.h"
#include "../../../runtime/platform-sw/rt_sw.h"
#include "x/tile-traps/oracle/fixture.h"
#include <stdio.h>
#include <assert.h>
static u16 checks,failures;
static u32 digest=17;
static void checked(u32 a,u32 b,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^a;
    if(a!=b){failures++;printf("trap line%u: %lu expected %lu\n",line,(unsigned long)a,(unsigned long)b);}
}
#define CHECK(a,b) checked((u32)(a),(u32)(b),__LINE__)
static Unit *prepare(u8 x)
{
    Unit *u;u16 cell;
    remove("ffsave.sav");sw_init(200);CHECK(W!=RT_NULL,1);
    st.open_stage=st.open_fade=st.banner=st.open_handoff=st.end=0;
    st.morph_mask=st.scene_hide=0;
    st.act=st.sel=U_RAMZA;st.ai=AI_OFF;st.mode=M_MENU;
    st.moved=st.acted=st.pick_t=0;st.want=st.prot=255;
    u=&st.unit[U_RAMZA];u->job=0x4a;u->level=1;u->exp=23;
    memset(&u->statuses,0,sizeof u->statuses);memset(u->status_ct,0,sizeof u->status_ct);
    memset(u->eq,255,sizeof u->eq);st.movement[U_RAMZA]=0;st.reaction[U_RAMZA]=0;
    st.pending[U_RAMZA].state=DEFER_NONE;
    unit_status_refresh(u);u->hp=u->max_hp=100;u->mp=u->max_mp=20;
    for(cell=0;cell<MAP_N;cell++)if(TILE_X[cell]==x&&TILE_Z[cell]==2&&!node_layer(cell))break;
    CHECK(cell<MAP_N,1);unit_place(u,cell);cursor_place(cell);
    return u;
}
static void trap_frame(void){rt_prev=rt_keys;rt_keys=0;rt_frame++;CHECK(game_update(),1);}
static void finish_message(void)
{
    u8 n=0;while(st.mode==M_PICK&&n++<100)trap_frame();CHECK(n<100,1);
}
static void oracle_checks(void)
{
    u16 i;
    for(i=0;i<sizeof trap_lookup_oracle/sizeof *trap_lookup_oracle;i++){
        const u8 *r=trap_lookup_oracle[i];
        CHECK(r[1]==r[2]?core_trap_id(r[0]):TRAP_NONE,r[3]);
    }
    for(i=0;i<sizeof trap_growth_oracle/sizeof *trap_growth_oracle;i++){
        CHECK(core_raw_level(trap_growth_oracle[i].before,trap_growth_oracle[i].growth,
                            trap_growth_oracle[i].level,trap_growth_oracle[i].down),trap_growth_oracle[i].after);
        CHECK(trap_growth_oracle[i].draws,5);
    }
}
static void effect_checks(void)
{
    Unit *u;u8 k;u32 before,after,raw[5];
    for(k=0;k<4;k++){
        u=prepare((u8)(2+k));before=rng32;
        CHECK(trap_apply(U_RAMZA,0),1);CHECK(W->trap_message,k+1);CHECK(st.mode,M_PICK);
        CHECK(u->exp,23);CHECK(st.acted,0);CHECK(st.action.phase,CTX_IDLE);
        if(k==0){CHECK(u->level,1);CHECK((u->raw[0]&RAW_CURRENT)!=0,1);CHECK(rng32!=before,1);}
        if(k==1){CHECK(core_status_current(&u->statuses,4)&1,1);CHECK(u->status_ct[15]>0,1);}
        if(k==2){CHECK(core_status_current(&u->statuses,4)&16,16);CHECK(u->status_flags&STATUS_CT_FROZEN,STATUS_CT_FROZEN);}
        if(k==3)CHECK(u->hp,80);
        if(k)CHECK(rng32,before);
    }
    u=prepare(3);u->statuses.immune[4]=1;CHECK(trap_apply(U_RAMZA,0),1);CHECK(core_status_current(&u->statuses,4)&1,0);
    u=prepare(4);u->statuses.immune[4]=16;CHECK(trap_apply(U_RAMZA,0),1);CHECK(core_status_current(&u->statuses,4)&16,0);
    u=prepare(4);st.pending[U_RAMZA].state=DEFER_CHARGING;unit_status_action(u,8);
    CHECK(trap_apply(U_RAMZA,0),1);CHECK(core_status_current(&u->statuses,0)&8,0);CHECK(st.pending[U_RAMZA].state,DEFER_NONE);
    u=prepare(5);u->hp=5;u->statuses.inflicted[4]=16;unit_status_refresh(u);
    CHECK(trap_apply(U_RAMZA,0),1);CHECK(u->hp,0);CHECK(u->dc,3);CHECK(u->kos,1);
    CHECK(core_status_current(&u->statuses,4)&16,0);
    u=prepare(5);u->max_hp=103;CHECK(trap_apply(U_RAMZA,0),1);CHECK(u->hp,80);
    u=prepare(5);u->max_hp=4;u->hp=2;u->statuses.inflicted[4]=16;unit_status_refresh(u);
    CHECK(trap_apply(U_RAMZA,0),1);CHECK(u->hp,2);CHECK(core_status_current(&u->statuses,4)&16,0);
    u=prepare(5);u->brave=100;u->mp=5;st.reaction[U_RAMZA]=REACTION_MP_SWITCH;
    CHECK(trap_apply(U_RAMZA,0),1);CHECK(u->hp,100);CHECK(u->mp,0);
    u=prepare(2);u->level=10;before=rng32;
    for(k=0;k<5;k++)raw[k]=core_raw_grown(u->raw[k],jobs[u->job].growth[k],10);
    for(k=0;k<5;k++)rand15();after=rng32;rng32=before;
    CHECK(trap_apply(U_RAMZA,0),1);CHECK(u->level,9);CHECK(u->exp,23);CHECK(rng32,after);
    for(k=0;k<5;k++)CHECK(u->raw[k]&RAW_MAX,core_raw_level(raw[k],jobs[u->job].growth[k],9,1));
    memcpy(raw,u->raw,sizeof raw);u->job=0x4d;unit_stats(u);CHECK(memcmp(raw,u->raw,sizeof raw),0);
    u->exp=99;before=rng32;for(k=0;k<5;k++)rand15();after=rng32;rng32=before;
    exp_gain(u,1);CHECK(u->level,10);CHECK(u->exp,0);CHECK(rng32,after);
    for(k=0;k<5;k++)CHECK(u->raw[k]&RAW_MAX,core_raw_level(raw[k]&RAW_MAX,jobs[u->job].growth[k],10,0));
}
static void precedence_checks(void)
{
    Unit *u=prepare(2);u32 seed=rng32;
    CHECK(trap_apply(U_RAMZA,2),0);CHECK(trap_apply(U_RAMZA,3),0);CHECK(trap_apply(U_RAMZA,255),0);CHECK(rng32,seed);
    st.movement[U_RAMZA]=24;u->brave=100;
    CHECK(discovery_find(U_RAMZA,1),1);CHECK(battle_save.inv[1],1);CHECK(battle_save.discovered[0]&8,8);
    CHECK(discovery_find(U_RAMZA,1),0);CHECK(trap_apply(U_RAMZA,1),1);CHECK(W->trap_message,2);
    // Actual committed movement queues the trap after the pickup message.
    u=prepare(3);st.unit[1].dc=DC_CRYSTAL;st.unit[1].hp=0;unit_place(&st.unit[1],UNIT_CELL(u));
    unit_place(u,(u16)(UNIT_CELL(u)-1));u->hp=1;
    CHECK(make_path_cell(U_RAMZA,UNIT_CELL(&st.unit[1]))>=2,1);
    st.mode=M_WALK;st.path_i=st.walk_t=0;
    while(st.mode==M_WALK)trap_frame();
    CHECK(st.mode,M_PICK);CHECK(st.pick,255);CHECK(W->move_post,1);
    CHECK(core_status_current(&u->statuses,4)&1,0);CHECK(u->hp,u->max_hp);
    // Stop on the next message to check that the trap cannot overwrite pickup.
    while(st.pick==255)trap_frame();CHECK(W->trap_message,2);CHECK(st.mode,M_PICK);
    CHECK(core_status_current(&u->statuses,4)&1,1);finish_message();CHECK(st.mode,M_MENU);
    // Sleep ends the mover's turn after the message, never reopening its menu.
    u=prepare(4);unit_place(u,(u16)(UNIT_CELL(u)-1));
    CHECK(make_path_cell(U_RAMZA,(u16)(UNIT_CELL(u)+1))>=2,1);st.mode=M_WALK;st.path_i=st.walk_t=0;
    while(st.mode==M_WALK)trap_frame();CHECK(W->trap_message,3);finish_message();
    CHECK(st.act!=U_RAMZA||st.mode!=M_MENU,1);
    // Movement HP benefit is its own first message, then the trap, once each.
    u=prepare(4);u->hp=20;st.movement[U_RAMZA]=8;W->move_post=1;W->move_owner=U_RAMZA;
    movement_settle(U_RAMZA);CHECK(st.mode,M_PICK);CHECK(st.abil,493);CHECK(u->hp,30);
    CHECK(W->move_post,2);CHECK(W->trap_message,0);st.pick_t=PICK_FRAMES-1;trap_frame();
    CHECK(W->move_post,0);CHECK(W->trap_message,3);CHECK(u->hp,30);
}
static void reaction_checks(void)
{
    u8 i,j;Unit *u;u32 before,after;
    for(i=0;i<sizeof trap_reaction_oracle/sizeof *trap_reaction_oracle;i++){
        const u16 *r=trap_reaction_oracle[i];u=prepare(5);
        st.reaction[U_RAMZA]=r[0];u->max_hp=r[1]?r[1]*5:4;u->hp=r[2];u->mp=r[3];u->brave=r[4];
        if(r[5]){u->statuses.inflicted[4]=4;unit_status_refresh(u);}
        before=rng32;for(j=0;j<r[8];j++)rand15();after=rng32;rng32=before;
        CHECK(trap_apply(U_RAMZA,0),1);CHECK(u->hp,r[6]);CHECK(u->mp,r[7]);CHECK(rng32,after);
        CHECK(st.action.phase,CTX_IDLE);CHECK(st.action.pending,0);CHECK(r[9],1);
    }
}
static void save_checks(void)
{
    Unit *u=prepare(2);Save old,back;u8 k,slot;u32 raw[5];
    save_campaign();CHECK(save_load(&old),1);CHECK(!memcmp(old.magic,"FSV5",4),1);
    // Migrate a real v4 prefix: no slot is accidentally treated as current raw.
    memcpy(old.magic,"FSV4",4);CHECK(rt_save(SAVE_NAME,&old,SAVE_V4_BYTES),1);
    CHECK(save_load(&back),1);CHECK(back.raw_current[0]|back.raw_current[1],0);
    party_load();u=&st.unit[U_RAMZA];u->level=8;u->exp=61;slot=u->slot;
    trap_level_down(U_RAMZA);memcpy(raw,u->raw,sizeof raw);save_campaign();
    CHECK(save_load(&back),1);CHECK(save_raw_current(&back,slot),1);CHECK(back.unit[slot].level,7);CHECK(back.unit[slot].exp,61);
    party_load();CHECK(u->level,7);CHECK(u->exp,61);CHECK(memcmp(raw,u->raw,sizeof raw),0);
    // Compaction carries the bitmap for reserves and clears the removed tail.
    back.unit[back.n]=back.unit[back.n-1];save_raw_set(&back,back.n,1);back.n++;
    CHECK(rt_save(SAVE_NAME,&back,sizeof back),1);party_load();
    for(k=0;k<NUNIT;k++)if(st.unit[k].slot==1)break;CHECK(k<NUNIT,1);
    st.unit[k].dc=DC_GONE;st.unit[k].hp=0;save_campaign();CHECK(save_load(&back),1);
    CHECK(save_raw_current(&back,0),1);CHECK(save_raw_current(&back,(u8)(back.n-1)),1);CHECK(save_raw_current(&back,back.n),0);
}
static void descriptor_checks(void)
{
    u16 size,interface_size;const u8 *source=resource_file("ffgd",&size);u8 *copy;
    resource_file("ffgi",&interface_size);copy=malloc(size);CHECK(copy!=RT_NULL,1);if(!copy)return;
    memcpy(copy,source,size);CHECK(chapter_surface_offset(copy,size,interface_size)!=0,1);
    copy[104]=0;copy[105]=127;CHECK(chapter_surface_offset(copy,size,interface_size)!=0,1);
    copy[105]=255;CHECK(chapter_surface_offset(copy,size,interface_size)!=0,1);
    copy[105]=128;CHECK(chapter_surface_offset(copy,size,interface_size),0);
    copy[104]=1;copy[105]=0;CHECK(chapter_surface_offset(copy,size,interface_size),0);
    copy[104]=copy[105]=0;copy[3]='4';CHECK(chapter_surface_offset(copy,size,interface_size),0);
    free(copy);
}
static void capture(const char *folder,const char *name)
{
    u8 i;char path[512];
    for(i=0;i<16;i++){rt_frame++;W->hold=W->frz=0;W->actor_budget=255;game_render();}
    snprintf(path,sizeof path,"%s/%s.png",folder,name);sw_write_png(path,3);
}
int main(int argc,char **argv)
{
    oracle_checks();effect_checks();precedence_checks();reaction_checks();save_checks();descriptor_checks();
    printf("traps %08lX checks %u failures %u\n",(unsigned long)digest,checks,failures);
    if(argc>1){u8 i;char label[16];for(i=0;i<4;i++){
        prepare((u8)(2+i));st.mode=M_PICK;st.pick=255;st.pick_t=1;
        snprintf(label,sizeof label,"before-%u",i);capture(argv[1],label);
        trap_apply(U_RAMZA,0);snprintf(label,sizeof label,"trap-%u",i);capture(argv[1],label);
    }}
    return failures!=0;
}
