// Counter Tackle/Flood and frozen Absorb Used MP through the real owners.
#define STATUS_REACTIONS_UI_CHECK
#include "check_status_reactions.c"
#include "x/core-counter-reactions/oracle/source.h"
#include "x/core-geomancy/oracle/source.h"
static void put16(u8 *p,u16 v){p[0]=(u8)(v>>8);p[1]=(u8)v;}
static void counter_stages(void)
{
    u16 i;u8 n;u8 *cj=malloc(cj_word(common+CJ_LENGTH));
    CHECK(cj!=RT_NULL,1);if(!cj)return;
    for(i=0;i<sizeof source_counter_stages/sizeof source_counter_stages[0];i++){
        const u16 *p=source_counter_stages[i];Ability local[21];const Ability *saved;const u8 *old;u16 at;AbId ab;u32 seed,expected;
        arena();st.unit[10].hp=st.unit[10].max_hp=100;st.unit[10].brave=p[9];st.reaction[10]=p[0];
        traits(&st.unit[10],0,0,(u8)p[4]);st.unit[U_RAMZA].set=p[7];
        saved=abilities;memcpy(local,saved,sizeof local);ab=p[7]==21?20:1;
        local[ab].formula=p[8];local[ab].kind=AB_DAMAGE;local[ab].flags=0;abilities=local;
        // A bounded archive view supplies only the three sections read by this
        // frozen-packet boundary:21 range rows, status rules and action menus.
        old=common;memcpy(cj,old,cj_word(old+CJ_LENGTH));at=CJ_HEADER;put16(cj+4+2*CJ_RANGE,at);
        memcpy(cj+at,cj_section(old,CJ_RANGE),21*14);cj[at+14*ab+6]=p[6];cj[at+14*ab+13]=p[10];at+=21*14;
        put16(cj+4+2*CJ_STATUS,at);memcpy(cj+at,cj_section(old,CJ_STATUS),1418);at+=1418;
        put16(cj+4+2*CJ_SPECIAL,at);memcpy(cj+at,cj_section(old,CJ_SPECIAL),190);common=cj;
#ifdef FFT_MODULES
        mod_id=0;
#endif
        act_start(U_RAMZA,10);st.abil=ab;st.hit=p[1];st.dmg=p[3];st.push=st.self=0;
        st.effect_kind=p[2]==128?AB_DAMAGE:p[2]==64?AB_HEAL:AB_NULL;
        if(p[5])st.action.phase=CTX_SECONDARY;
        else{CastBatch *b=&W->cast;memset(b,0,sizeof *b);b->phase=CAST_BATCH_READY;b->actor=U_RAMZA;b->ability=ab;b->mask=(u16)1<<10;
            b->victim[10].effect=st.effect_kind;b->victim[10].hit=st.hit;b->victim[10].amount=st.dmg;}
        seed=rng32;for(n=0;n<p[14];n++)rand15();expected=rng32;rng32=seed;impact();
        CHECK(st.unit[10].hp,p[13]);CHECK(st.action.pending&((u16)1<<10)?reaction_id(st.action.reaction[10]):0,p[11]);CHECK(rng32==expected,1);
        if(p[11]==REACTION_ABSORB_MP)CHECK(st.action.reaction[10]&255,p[12]);
        common=old;abilities=saved;
#ifdef FFT_MODULES
        mod_id=0;
#endif
    }
    free(cj);
}
static void absorb_effects(void)
{
    u16 i;State once;u32 seed;
    for(i=0;i<sizeof source_absorb_effects/sizeof source_absorb_effects[0];i++){
        const u16 *p=source_absorb_effects[i];Unit *u;u16 jp;
        arena();u=&st.unit[10];u->hp=u->max_hp=100;u->mp=p[0];u->max_mp=p[1];traits(u,0,0,(u8)p[3]);
        st.reaction[10]=0;act_start(U_RAMZA,10);st.abil=NO_ABIL;st.hit=0;impact();
        CHECK(core_action_queue(&st.action,10,REACTION_ABSORB_MP,NUNIT),CTX_OK);st.action.reaction[10]=0x8000U|p[2];st.reaction[10]=REACTION_ABSORB_MP;
        st.pending[10].state=DEFER_CHARGING;st.pending[10].ticks=7;unit_status_action(u,8);jp=u->jp;seed=rng32;
        ZB(61);end_anim();ZE(61);CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.abil,REACTION_ABSORB_MP);CHECK(st.atk,10);CHECK(st.tgt,10);
        CHECK(st.hit,p[5]);CHECK(st.dmg,p[6]);CHECK(act_kind(),p[5]?AB_MP_HEAL:AB_NULL);
        if(p[4]!=65535)u->mp=p[4];ZB(62);impact();ZE(62);CHECK(u->mp,p[7]);CHECK(u->hp,100);CHECK(u->ct,37);CHECK(u->jp,jp);
        CHECK(st.pending[10].state,DEFER_CHARGING);CHECK(st.pending[10].ticks,7);CHECK(rng32==seed,1);CHECK(st.gain,0);
        once=st;impact();CHECK(memcmp(&once,&st,sizeof st)==0,1);finish_status();CHECK(st.atk,U_RAMZA);CHECK(st.abil,NO_ABIL);
    }
    // Stage the real Cure6 cost, then change the caster's support. Preparation
    // must retain six, including after another queued response could intervene.
    arena();st.unit[U_RAMZA].set=11;st.reaction[10]=REACTION_ABSORB_MP;st.unit[10].mp=0;st.unit[10].max_mp=20;
    act_start(U_RAMZA,10);st.abil=1;st.hit=0;impact();CHECK(st.action.reaction[10],0x8006);
    st.support[U_RAMZA]=SUPPORT_HALF_MP;end_anim();CHECK(st.dmg,6);impact();CHECK(st.unit[10].mp,6);finish_status();
}
static void tackles(void)
{
    u16 expected,hp,jp;u32 seed,after;u8 mode;
    for(mode=0;mode<3;mode++){
        arena();st.reaction[10]=REACTION_TACKLE;st.unit[10].pa=13;st.unit[10].eq[EQ_WEAPON]=99;
        if(mode==1)unit_place(&st.unit[U_RAMZA],third);
        root(1);CHECK(st.action.pending,(u16)1<<10);jp=st.unit[10].jp;hp=st.unit[U_RAMZA].hp;
        seed=rng32;expected=skill_amount(10,U_RAMZA,0x93,rand15());rand15();rand15();after=rng32;rng32=seed;
        ZB(61);end_anim();ZE(61);CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.abil,0x93);CHECK(st.tgt,U_RAMZA);CHECK(st.atk,10);CHECK(st.action.left,0);
        CHECK(st.dmg,expected);CHECK(rng32==after,1);seed=rng32;ZB(62);impact();ZE(62);CHECK(st.unit[U_RAMZA].hp<hp,1);CHECK(st.unit[10].jp,jp);CHECK(rng32==seed,1);finish_status();
    }
    // Weapon reach is not Dash reach; a distant incoming shot cannot stretch it.
    arena();st.reaction[10]=REACTION_TACKLE;unit_place(&st.unit[U_RAMZA],third);root(1);
    unit_place(&st.unit[U_RAMZA],first);unit_place(&st.unit[10],third);end_anim();CHECK(st.action.phase!=CTX_SECONDARY,1);
    arena();st.reaction[10]=REACTION_TACKLE;root(1);change(10,SID_DONT_ACT);end_anim();CHECK(st.action.phase!=CTX_SECONDARY,1);
}
static u8 *counter_surface;
static void terrain_at(u8 actor,u8 surface)
{
    u16 n=(u16)MAP_W*MAP_H;
    memcpy(counter_surface,W->surface-8,8+(n<<3));counter_surface[3]='2';memset(counter_surface+8+(n<<3),0,n<<1);
    counter_surface[8+(n<<3)+((W->grid_row[st.unit[actor].z]+st.unit[actor].x)<<1)+node_layer(UNIT_CELL(&st.unit[actor]))]=surface;
#ifdef FFT_PRODUCTION_CORE
    // Logic tests validate this same FGS2 envelope through the public binder.
    // The production renderer leaves test-only API wrappers out of its image.
    W->surface=counter_surface+8;W->surface_flags|=SURFACE_TERRAIN;
    CHECK(magic_heal(actor,actor,126,4),source_terrain[surface]);
#else
    CHECK(battle_bind_surface(counter_surface,8+10*n),1);
#endif
}
static void floods(void)
{
    u8 ab,i,surface;u16 c,mask,jp,root_jp;u32 seed;State once;
    counter_surface=malloc(8+10*MAP_NMAX);CHECK(counter_surface!=RT_NULL,1);if(!counter_surface)return;
    for(ab=126;ab<138;ab++){
        arena();st.reaction[10]=REACTION_FLOOD;st.unit[10].pa=13;st.unit[10].ma=17;
        st.unit[10].mp=4;st.unit[10].max_mp=20;st.support[10]=st.support[U_RAMZA]=0;memset(st.affinity,0,sizeof st.affinity);
        unit_place(&st.unit[U_RAMZA],second);unit_place(&st.unit[10],first);
        st.unit[8]=st.unit[U_RAMZA];unit_place(&st.unit[8],third);st.unit[8].team=TEAM_PLAYER;
        for(surface=0;source_terrain[surface]!=ab;surface++){}terrain_at(10,surface);
        root_jp=st.unit[U_RAMZA].jp;jp=st.unit[10].jp;root(1);CHECK(st.unit[U_RAMZA].jp,root_jp);CHECK(st.action.pending,(u16)1<<10);
        rng32=1;ZB(61);end_anim();ZE(61);CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.abil,ab);CHECK(W->cast.phase,CAST_BATCH_READY);
        CHECK(W->cast.actor,10);mask=((u16)1<<U_RAMZA)|((u16)1<<8);CHECK(W->cast.mask,mask);CHECK(area_impact_root(),1);
        CHECK(W->cast.victim[U_RAMZA].amount,119);CHECK(W->cast.victim[8].amount,119);seed=rng32;
        ZB(62);impact();ZE(62);CHECK(st.action.phase,CTX_APPLIED);CHECK(W->cast.phase,CAST_BATCH_APPLIED);
        CHECK(st.action.pending,0);CHECK(st.unit[10].jp,jp);CHECK(st.unit[10].mp,4);CHECK(rng32==seed,1);
        for(i=0;i<2;i++){u8 t=i?8:U_RAMZA;CHECK(st.unit[t].hp,ab==137&&(W->cast.victim[t].effect&EFFECT_STATUS)?0:880);}
        once=st;impact();CHECK(memcmp(&once,&st,sizeof st)==0,1);finish_status();CHECK(st.abil,NO_ABIL);CHECK(st.atk,U_RAMZA);CHECK(st.tgt,10);CHECK(W->cast.phase,CAST_BATCH_EMPTY);
    }
    free(counter_surface);counter_surface=RT_NULL;
}
static void gates(void)
{
    u16 id;State before;u32 seed;
    for(id=436;id<=438;id++){
        arena();before=st;seed=rng32;CHECK(can_use(U_RAMZA,id),0);skill(U_RAMZA,10,id);CHECK(memcmp(&before,&st,sizeof st)==0,1);CHECK(rng32==seed,1);
    }
}
static void counter_trials(void){counter_stages();absorb_effects();tackles();floods();gates();}
#ifndef COUNTER_REACTIONS_UI_CHECK
#ifndef __m68k__
int main(void){sw_init(61);counter_trials();printf("counter_reactions %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();counter_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
#endif
