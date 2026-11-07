// Vulnerable cast lifecycle through actual COMMAND/effect/CT/status adapters.
#define INCANTATION_CHECK
#include "check_casting.c"
#include "x/core-incantation/oracle/source.h"
static void inc_prepare(void)
{
    prepared();st.unit[U_RAMZA].mp=14;st.unit[U_RAMZA].set=SET_BASIC;
    st.acted=st.moved=0;st.open_resolving=st.open_ct_phase=0;rt_keys=rt_prev=0;
    CHECK(abilities[0x94].flags,AF_CAST|AF_SELF);CHECK(cj_range(common,0x94)[13],7);
}
static void inc_cure(u8 who,u8 sid)
{
    CoreStatusDelta d;memset(&d,0,sizeof d);d.remove[sid>>3]=(u8)(0x80>>(sid&7));
    battle_status_apply(who,&d,1);
}
static void inc_miss(void){st.hit=0;st.push=0;impact();end_anim();}
static u8 inc_far(void)
{
    u16 c;for(c=0;c<MAP_N;c++)if(map_tiles[c].walk==1){
        s16 dx=(s16)TILE_X[c]-st.unit[U_RAMZA].x,dz=(s16)TILE_Z[c]-st.unit[U_RAMZA].z;
        if((dx<0?-dx:dx)+(dz<0?-dz:dz)>4)return (u8)c;
    }
    CHECK(0,1);return third;
}
static void paid_success(void)
{
    u16 hp,damage;
    // This failed in the previous resource milestone: paying7 left0, and the
    // execution incorrectly reran preview affordability on the remaining MP.
    inc_prepare();st.unit[U_RAMZA].mp=7;hp=st.unit[10].hp;
    skill(U_RAMZA,10,0x93);CHECK(st.unit[U_RAMZA].mp,0);CHECK(st.hit,1);
    damage=st.dmg;CHECK(damage>0,1);st.push=0;impact();
    CHECK(st.unit[10].hp,hp-damage);impact();CHECK(st.unit[10].hp,hp-damage);
    CHECK(st.unit[U_RAMZA].mp,0);
}
static void bindings(void)
{
    u8 follow,far;u16 hp,damage;Unit replacement;
    for(follow=0;follow<2;follow++){
        inc_prepare();far=inc_far();replacement=st.unit[10];
        CHECK(cast_order(U_RAMZA,follow?10:second,0x94,follow),2);
        CHECK(st.pending[U_RAMZA].state,follow?DEFER_CAST_UNIT:DEFER_CAST_TILE);
        CHECK(st.pending[U_RAMZA].tile,follow?10:second);CHECK(st.unit[U_RAMZA].mp,14);
        CHECK(st.moved,0);CHECK(st.acted,1);CHECK(st.action.phase,CTX_IDLE);
        unit_place(&st.unit[10],far);st.unit[8]=replacement;unit_place(&st.unit[8],second);
        st.pending[U_RAMZA].ticks=0;ZB(38);CHECK(charge_resolve(),1);ZE(38);
        CHECK(st.tgt,follow?10:8);CHECK(CURSOR_CELL,follow?far:second);
        CHECK(st.unit[U_RAMZA].mp,7);CHECK(st.hit,1);
        hp=st.unit[st.tgt].hp;damage=st.dmg;st.push=0;impact();
        CHECK(st.unit[st.tgt].hp,hp-damage);CHECK(st.unit[follow?8:10].hp,999);
    }
    // Empty, KO and absent centres still pay once, without retargeting to the
    // old unit tile or rewarding an action that affected nobody.
    for(follow=0;follow<3;follow++){
        inc_prepare();far=inc_far();CHECK(cast_order(U_RAMZA,follow?10:second,0x94,follow?1:0),2);
        if(!follow)unit_place(&st.unit[10],far);
        else if(follow==1)st.unit[10].hp=0;
        else{st.unit[10].dc=DC_GONE;st.unit[10].x=255;}
        st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),0);
        CHECK(st.unit[U_RAMZA].mp,7);CHECK(st.action.phase,CTX_IDLE);
        CHECK(st.pending[U_RAMZA].state,DEFER_NONE);CHECK(st.unit[U_RAMZA].gexp,0);
        CHECK(charge_resolve(),0);CHECK(st.unit[U_RAMZA].mp,7);
    }
}
static void interruption(void)
{
    u8 mode,far;u32 hash,seed;
    for(mode=0;mode<2;mode++){
        const u16 *p=source_cast_recoil+6*mode;
        inc_prepare();CHECK(p[0],mode?10:8);CHECK(p[3],!mode);
        if(mode)CHECK(cast_order(U_RAMZA,10,0x94,1),2);
        else skill(U_RAMZA,10,0x196);
        unit_place(&st.unit[U_RAMZA],third);st.unit[U_RAMZA].eq[EQ_WEAPON]=11;
        st.pending[U_RAMZA].ticks=0;
        CHECK(pending_next(),mode?U_RAMZA:255);
        if(mode){CHECK(charge_resolve(),1);CHECK(st.unit[U_RAMZA].mp,7);}
    }
    for(mode=0;mode<4;mode++){
        u8 sid=mode==0?SID_SILENCE:mode==1?SID_DONT_ACT:mode==2?SID_SLEEP:SID_STOP;
        inc_prepare();CHECK(cast_order(U_RAMZA,10,0x94,1),2);change(U_RAMZA,sid);
        st.pending[U_RAMZA].ticks=0;
        if(mode==2){CHECK(st.pending[U_RAMZA].state,DEFER_NONE);inc_cure(U_RAMZA,sid);CHECK(st.pending[U_RAMZA].state,DEFER_NONE);}
        else if(mode==3){CHECK(pending_next(),255);CHECK(st.pending[U_RAMZA].state,DEFER_CAST_UNIT);inc_cure(U_RAMZA,sid);CHECK(charge_resolve(),1);CHECK(st.unit[U_RAMZA].mp,7);continue;}
        CHECK(charge_resolve(),0);CHECK(st.unit[U_RAMZA].mp,14);
    }
    inc_prepare();CHECK(cast_order(U_RAMZA,10,0x94,1),2);st.unit[U_RAMZA].mp=6;
    st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),0);CHECK(st.unit[U_RAMZA].mp,6);
    inc_prepare();CHECK(cast_order(U_RAMZA,10,0x94,1),2);st.unit[U_RAMZA].hp=1;
    attack(10,U_RAMZA);st.hit=1;st.dmg=999;st.push=0;impact();
    CHECK(st.pending[U_RAMZA].state,DEFER_NONE);CHECK(st.unit[U_RAMZA].mp,14);
    // All invalid orders preserve request scratch as well as resources/RNG.
    for(mode=0;mode<5;mode++){
        inc_prepare();far=inc_far();hash=state_hash();seed=rng32;
        CHECK(cast_order(U_RAMZA,mode==0?NUNIT:mode==1?far:10,mode==2?0x1e:mode==3?0x95:0x94,mode==0?1:mode==1?0:mode==4?2:1),0);
        CHECK(state_hash()==hash,1);CHECK(rng32==seed,1);
    }
    inc_prepare();skill(U_RAMZA,10,0x93);hash=state_hash();seed=rng32;
    CHECK(cast_order(U_RAMZA,10,0x94,1),0);CHECK(state_hash()==hash,1);CHECK(rng32==seed,1);
}
static void clock_and_support(void)
{
    u8 i;u32 seed;
    inc_prepare();CHECK(cast_order(U_RAMZA,10,0x94,1),2);seed=rng32;
    game_update();CHECK(st.pending[U_RAMZA].ticks,3);CHECK(rng32==seed,1);
    for(i=0;i<NUNIT;i++){st.unit[i].ct=0;st.unit[i].sp=1;}
    st.unit[10].sp=50;
    for(i=0;i<3;i++){
        st.unit[10].ct=50;CHECK(ct_next(),i==2?254:10);
        CHECK(st.pending[U_RAMZA].ticks,2-i);CHECK(st.unit[U_RAMZA].mp,14);
    }
    CHECK(charge_resolve(),1);CHECK(st.unit[U_RAMZA].mp,7);
    for(i=0;i<3;i++){
        inc_prepare();st.support[U_RAMZA]=i==0?SUPPORT_SHORT_CHARGE:i==1?SUPPORT_NON_CHARGE:SUPPORT_HALF_MP;
        CHECK(cast_order(U_RAMZA,10,0x94,1),i==1?1:2);
        if(i==1){CHECK(st.pending[U_RAMZA].state,DEFER_NONE);CHECK(st.unit[U_RAMZA].mp,7);CHECK(st.hit,1);}
        else{CHECK(st.pending[U_RAMZA].ticks,i==0?2:3);st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);CHECK(st.unit[U_RAMZA].mp,i==0?7:11);}
    }
    // Standard skill/AI entry defaults to unit binding and preserves movement.
    inc_prepare();skill(U_RAMZA,10,0x94);CHECK(st.pending[U_RAMZA].state,DEFER_CAST_UNIT);
    CHECK(st.moved,0);CHECK(st.acted,1);
}
static void wave(void)
{
    Unit prototype;u16 c=0;u8 i;
    inc_prepare();prototype=st.unit[U_RAMZA];
    for(i=0;i<NUNIT;i++){
        while(c<MAP_N&&!map_tiles[c].walk)c++;
        CHECK(c<MAP_N,1);st.unit[i]=prototype;unit_place(&st.unit[i],(u8)c++);
        st.unit[i].mp=7;st.unit[i].ct=i==12?50:0;st.unit[i].sp=i==12?50:1;
        st.unit[i].team=i==U_RAMZA?TEAM_PLAYER:TEAM_ENEMY;st.support[i]=0;
        CHECK(cast_order(i,i,0x94,1),2);st.pending[i].ticks=1;
    }
    turn_start();
    for(i=0;i<NUNIT;i++){
        CHECK(st.atk,i);CHECK(st.unit[i].mp,0);CHECK(st.hit,1);CHECK(st.open_ct_phase,1);
        inc_miss();
    }
    CHECK(st.action.phase,CTX_IDLE);CHECK(st.open_resolving,0);CHECK(st.act,12);
    for(i=0;i<NUNIT;i++)CHECK(st.pending[i].state,DEFER_NONE);
}
static void response_order(void)
{
    u8 k;u32 hash;
    inc_prepare();st.unit[10].set=SET_BASIC;st.unit[10].mp=14;
    CHECK(cast_order(U_RAMZA,10,0x94,1),2);CHECK(cast_order(10,U_RAMZA,0x94,1),2);
    st.pending[U_RAMZA].ticks=st.pending[10].ticks=0;st.open_ct_phase=1;
    st.reaction[10]=REACTION_POTION;turn_start();CHECK(st.action.root,U_RAMZA);
    CHECK(st.unit[U_RAMZA].mp,7);st.push=0;impact();end_anim();
    CHECK(st.primary.running,1);CHECK(st.pending[10].state,DEFER_CAST_UNIT);
    CHECK(st.unit[10].mp,14);hash=state_hash();CHECK(charge_resolve(),1);CHECK(state_hash()==hash,1);
    impact();end_anim();
    for(k=0;k<4&&st.action.root!=10;k++)end_anim();
    CHECK(st.action.root,10);CHECK(st.action.phase,CTX_ROOT);
    CHECK(st.unit[U_RAMZA].mp,7);CHECK(st.unit[10].mp,7);
    CHECK(st.pending[10].state,DEFER_NONE);CHECK(st.open_ct_phase,1);
}
typedef struct {
    CoreGrid grid;CoreTilePosition pos[256];Tile tiles[256];
    u8 row[16],lookup[512],present[64],x[256],z[256];
} CastFixture;
static void node255(void)
{
    CastFixture *f;u16 i;u8 k;
    inc_prepare();f=(CastFixture *)malloc(sizeof *f);CHECK(f!=0,1);if(!f)return;
    memset(f,0,sizeof *f);f->grid.count=256;f->grid.width=8;f->grid.height=16;
    f->grid.row=f->row;f->grid.position=f->pos;f->grid.lookup=f->lookup;f->grid.present=f->present;
    for(i=0;i<16;i++)f->row[i]=(u8)(i<<3);
    for(i=0;i<256;i++){
        u8 x=i&7,z=(i>>3)&15,layer=i>=128;u16 at=(u16)f->row[z]+x+((u16)layer<<8);
        f->pos[i].x=f->x[i]=x;f->z[i]=z;f->pos[i].z_layer=z|(layer<<5);
        f->lookup[at]=(u8)i;f->present[at>>3]|=(u8)(1U<<(at&7));
        f->tiles[i].walk=1;f->tiles[i].stand=layer?2:0;
        for(k=0;k<4;k++)f->tiles[i].c[k]=layer?1:0;
    }
    CHECK(battle_bind_grid(&f->grid,f->tiles,f->x,f->z),1);CHECK(MAP_N,256);
    unit_place(&st.unit[U_RAMZA],254);unit_place(&st.unit[10],255);
    CHECK(cast_target_cell(0,255),255);CHECK(cast_target_cell(1,10),255);
    CHECK(cast_order(U_RAMZA,10,0x94,1),2);
    unit_place(&st.unit[10],127);CHECK(cast_target_cell(1,10),127);
    CHECK(st.pending[U_RAMZA].tile,10);
    CHECK(cast_order(U_RAMZA,255,0x94,0),2);CHECK(st.pending[U_RAMZA].tile,255);
    CHECK(st.pending[U_RAMZA].state,DEFER_CAST_TILE);CHECK(st.unit[U_RAMZA].mp,14);
    // Reset the engine while the borrowed fixture is still alive.
    prepared();free(f);
}
// The 68000 binary leaves casting_trials out (program size; core-casting runs them natively):
// a PC build with the same define must print exactly what the 68000 part prints.
#ifdef INCANTATION_NATIVE_SPLIT
static void incantation_trials(void){paid_success();bindings();interruption();clock_and_support();wave();response_order();node255();}
#else
static void incantation_trials(void){casting_trials();paid_success();bindings();interruption();clock_and_support();wave();response_order();node255();}
#endif
#ifndef __m68k__
int main(void){sw_init(61);incantation_trials();printf("incantation %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();incantation_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
