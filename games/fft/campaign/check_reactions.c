// Actual root/secondary effects, packet restoration and original reaction policy.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main reaction_runtime_main
#ifndef REACTION_FRAME_CHECK
#define game_render reaction_no_render
#endif
#include "../../../runtime/platform-ti68k/rt_ti.c"
#ifndef REACTION_FRAME_CHECK
#undef game_render
void reaction_no_render(void){}
#endif
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "gariland.c"
#include "x/core-reactions/oracle/source.h"
static u16 checks,failures;static u32 digest=17;
static void check(u16 a,u16 b
#if !defined(__m68k__) || !defined(FFT_CHECK_NATIVE_ORDINAL)
                  ,u16 line
#endif
                  )
{
    checks++;digest=(digest<<1|digest>>31)^a;
    if(a!=b){failures++;
#ifndef __m68k__
        printf("line%u: %u expected%u\n",line,a,b);
#else
#ifdef FFT_CHECK_NATIVE_ORDINAL
#ifndef FFT_CHECK_NATIVE_QUIET
        BENCH_VALUE(checks); // bounded carriers report an assertion ordinal
#endif
#else
        BENCH_VALUE(line);
#endif
#endif
    }
}
#if defined(__m68k__) && defined(FFT_CHECK_NATIVE_ORDINAL)
#define CHECK(a,b) check((u16)(a),(u16)(b))
#else
#define CHECK(a,b) check((u16)(a),(u16)(b),__LINE__)
#endif
static void reset(void){game_scenario(61);st.act=U_RAMZA;st.ai=AI_OFF;st.banner=0;st.mode=M_MENU;rt_keys=rt_prev=0;}
static void change(u8 who,u8 sid)
{
    CoreStatusDelta d;memset(&d,0,sizeof d);d.add[sid>>3]=(u8)(0x80>>(sid&7));battle_status_apply(who,&d,0);
}
static void impact(void){st.act_t=ACT_HIT;attack_step();}
static void end_anim(void){st.act_t=(st.gain?ACT_GAIN_END:ACT_END)-1;attack_step();}
static u16 choose(u8 kind,u8 flags)
{
    u16 ab;const u8 *r;
    if(kind==AB_DAMAGE&&flags)return NO_ABIL;
    for(ab=0;ab<512;ab++)if(abilities[ab].kind==kind&&!(abilities[ab].flags&AF_CAST)){
        r=cj_range(common,ab);if((r?(r[6]&0x10):0)==flags)return ab;
    }
    return 0xfffd;
}
static void source(void)
{
    u16 i,ab,native=0;u8 bag[3],j;
    for(i=0;i<sizeof source_chance/sizeof source_chance[0];i++){
        const u16 *p=source_chance[i];CHECK(!reaction_chance(p[0],p[1]),p[2]);
    }
    for(i=0;i<sizeof source_potions/sizeof source_potions[0];i++){
        const u16 *p=source_potions[i];u8 item;
        for(j=0;j<3;j++)bag[j]=(p[1]>>j)&1;
        item=reaction_potion(bag,p[0]!=0);CHECK(item==255?65535:0xf0+item,p[2]);
    }
    for(i=0;i<sizeof source_stages/sizeof source_stages[0];i++){
        const u16 *p=source_stages[i];Unit *t;
        reset();ab=choose(p[2]==0x80?AB_DAMAGE:AB_HEAL,p[6]);
        if(ab==0xfffd)continue; // source-only combination lacks a native ability producer
        native++;t=&st.unit[10];t->hp=t->max_hp=100;t->brave=100;
        st.reaction[10]=p[0];if(p[4]!=255)change(10,p[4]);
        act_start(U_RAMZA,10);st.abil=ab;st.hit=p[1];st.dmg=p[3];st.push=st.self=0;
        // This fixture supplies an already-resolved source packet, not a live
        // formula. Keep its explicit HP category when the selected carrier is an item.
        st.effect_kind=p[2]==0x80?AB_DAMAGE:AB_HEAL;
        if(p[5])st.action.phase=CTX_SECONDARY;
        impact();CHECK(t->hp,p[8]);CHECK(st.action.pending&((u16)1<<10)?st.action.reaction[10]:0,p[7]);
    }
    CHECK(native,26);
}
static u8 first,second,third;
static void arena(void)
{
    u16 c;u8 i;reset();
    for(c=0;c+2<MAP_N;c++)if(TILE_Z[c]==TILE_Z[c+2]&&
        map_tiles[c].walk==1&&map_tiles[c+1].walk==1&&map_tiles[c+2].walk==1&&
        map_tiles[c].stand==map_tiles[c+1].stand&&map_tiles[c].stand==map_tiles[c+2].stand)break;
    CHECK(c+2<MAP_N,1);first=c;second=c+1;third=c+2;
    for(i=0;i<NUNIT;i++){st.unit[i].hp=0;st.unit[i].x=255;st.unit[i].dc=DC_GONE;}
    unit_place(&st.unit[U_RAMZA],first);unit_place(&st.unit[10],second);
    for(i=0;i<2;i++){
        Unit *u=&st.unit[i?10:U_RAMZA];u->hp=u->max_hp=999;u->dc=3;u->brave=100;
        u->pa=5;u->zodiac=0;u->eq[EQ_WEAPON]=255;u->ct=i?37:21;
    }
    st.unit[U_RAMZA].team=TEAM_PLAYER;st.unit[10].team=TEAM_ENEMY;
    st.acted=1;st.moved=0;rt_seed=17;
}
static void root(u16 damage)
{
    attack(U_RAMZA,10);st.hit=1;st.push=0;st.dmg=damage;impact();
}
static void counter(void)
{
    Unit *a,*t;u16 hp,jp;u8 ticks;
    arena();a=&st.unit[U_RAMZA];t=&st.unit[10];st.reaction[10]=st.reaction[U_RAMZA]=REACTION_COUNTER;
    // A reaction must preserve the reactor's independently armed Charge.
    st.pending[10].state=DEFER_CHARGING;st.pending[10].ticks=4;st.pending[10].origin=UNIT_CELL(t);
    st.pending[10].weapon=hand_weapon(t,0);st.pending[10].off_weapon=hand_weapon(t,1);st.pending[10].ability=0x196;unit_status_action(t,8);
    jp=a->jp;root(4);CHECK(st.primary.reward!=0,1);CHECK(st.gain,0);CHECK(a->jp,jp);
    change(U_RAMZA,SID_DONT_ACT);hp=a->hp;ticks=st.pending[10].ticks;
    ZB(27);end_anim();ZE(27);CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.atk,10);CHECK(st.tgt,U_RAMZA);
    CHECK(st.action.root,U_RAMZA);CHECK(st.act,U_RAMZA);CHECK(st.primary.running,1);
    CHECK(st.unit[10].ct,37);CHECK(st.unit[U_RAMZA].ct,21);CHECK(st.moved,0);CHECK(st.acted,1);
    impact();CHECK(st.action.phase,CTX_APPLIED);CHECK(a->hp<hp,1);hp=a->hp;
    CHECK(st.action.pending,0);CHECK(st.gain,0);CHECK(t->gexp,0);CHECK(t->gjp,0);
    impact();CHECK(a->hp,hp);CHECK(st.pending[10].ticks,ticks);CHECK(st.pending[10].state,DEFER_CHARGING);
    CHECK(core_status_current(&t->statuses,0)&8,8);
    end_anim();CHECK(st.atk,U_RAMZA);CHECK(st.tgt,10);CHECK(st.abil,NO_ABIL);CHECK(st.dmg,4);
    CHECK(st.primary.running,0);CHECK(st.primary.reward,0);CHECK(st.action.phase,CTX_DRAIN);CHECK(st.gain!=0,1);
    CHECK(st.act_t,ACT_HIT+GAIN_T-1);CHECK(a->jp>jp,1);
    end_anim();CHECK(st.action.phase,CTX_IDLE);CHECK(st.mode,M_MENU);CHECK(a->hp,hp);
    // A miss can still provoke Counter, but does not earn root rewards.
    arena();st.reaction[10]=REACTION_COUNTER;attack(U_RAMZA,10);st.hit=0;impact();CHECK(st.action.pending!=0,1);
    end_anim();CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.primary.reward,0);
    // Recoil can take a queued melee reactor out of range before preparation.
    arena();st.reaction[10]=REACTION_COUNTER;attack(U_RAMZA,10);st.hit=1;st.dmg=1;st.push=1;st.push_cell=third;
    impact();CHECK(UNIT_CELL(&st.unit[10]),third);end_anim();CHECK(st.primary.running,0);CHECK(st.action.pending,0);
    CHECK(st.atk,U_RAMZA);CHECK(st.gain!=0,1);
}
static void potions(void)
{
    u8 i,j;u16 hp,stock;
    for(i=0;i<5;i++){
        arena();st.unit[U_RAMZA].team=TEAM_ENEMY;st.unit[10].team=i==3?TEAM_ENEMY:i==4?TEAM_GUEST:TEAM_PLAYER;
        st.reaction[10]=REACTION_POTION;st.unit[10].hp=100;
        for(j=0;j<3;j++)st.bag[j]=0;
        if(i<3)st.bag[i]=2;if(i==4)st.bag[1]=2;
        root(7);hp=st.unit[10].hp;stock=st.bag[i<3?i:1];end_anim();
        CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.atk,10);CHECK(st.tgt,10);
        CHECK(st.abil,0x170+(i<3?i:i==3?0:1));CHECK(st.self,0);CHECK(st.push,0);
        CHECK(st.bag[i<3?i:1],i==3?stock:stock-1);
        stock=st.bag[i<3?i:1];
#ifdef FFT_MODULES
        mod_call(MOD_RESPONSE,1,2);
#else
        reaction_launch(2);
#endif
        CHECK(st.bag[i<3?i:1],stock); // preparation cannot consume twice
        impact();CHECK(st.unit[10].hp,hp+abilities[st.abil].y*10);CHECK(st.gain,0);CHECK(st.action.pending,0);
        end_anim();CHECK(st.atk,U_RAMZA);CHECK(st.dmg,7);CHECK(st.unit[10].ct,37);
        end_anim();CHECK(st.action.phase,CTX_IDLE);
    }
    // No stock: no secondary, no consumption; root rewards still settle once.
    arena();st.unit[10].team=TEAM_PLAYER;st.reaction[10]=REACTION_POTION;
    st.bag[0]=st.bag[1]=st.bag[2]=0;root(1);end_anim();CHECK(st.primary.running,0);CHECK(st.gain!=0,1);
    // Sleep is cured by the damage before the post-impact Auto-Potion check.
    arena();st.reaction[10]=REACTION_POTION;change(10,SID_SLEEP);root(0);
    CHECK(core_status_current(&st.unit[10].statuses,4)&0x10,0);CHECK(st.action.pending!=0,1);
}
// The shared Potion stock of two reactors used a weapon skill carrying AREA without being an
// incantation (Stasis Sword before Holy Sword); no such record remains. The batch form is in
// check_batch.c (responses_and_late_protection).
static void saved_reaction(void)
{
    reset();party_load();CHECK(st.party_in,1);CHECK(st.reaction[U_RAMZA],REACTION_COUNTER);
}
// A body redirects a direct weapon now. Obstruction controls use real solid
// terrain, in a temporary copy; the archived chapter is never modified.
static const u8 *solid_panel(u8 cell)
{
    const u8 *source=W->surface;u16 size=(u16)MAP_W*MAP_H*8;
    u8 *copy=malloc(size),*column;CHECK(copy!=RT_NULL,1);if(!copy)return source;
    memcpy(copy,source,size);W->surface=copy;
    column=copy+((u16)(W->grid_row[TILE_Z[cell]]+TILE_X[cell])<<3);
    column[0]=(W->hs[cell]>>1)+4;column[1]=column[3]=0;return source;
}
static void solid_restore(const u8 *source)
{
    if(W->surface!=source){u8 *copy=(u8 *)W->surface;W->surface=source;free(copy);}
}
static void ranged_counter(void)
{
    u8 blocked;u16 c,w;const u8 *source=RT_NULL;
    for(blocked=0;blocked<2;blocked++){
        arena();
        for(w=0;w<255;w++)if(items[w].wp&&items[w].reach>=4)break;
        CHECK(w<255,1);
        for(c=0;c+3<MAP_N;c++)if(TILE_Z[c]==TILE_Z[c+3]&&
            map_tiles[c].walk==1&&map_tiles[c+1].walk==1&&map_tiles[c+2].walk==1&&map_tiles[c+3].walk==1&&
            map_tiles[c].stand==map_tiles[c+1].stand&&map_tiles[c].stand==map_tiles[c+2].stand&&map_tiles[c].stand==map_tiles[c+3].stand)break;
        CHECK(c+3<MAP_N,1);unit_place(&st.unit[U_RAMZA],c);unit_place(&st.unit[10],c+3);
        st.unit[10].eq[EQ_WEAPON]=st.unit[U_RAMZA].eq[EQ_WEAPON]=w;st.reaction[10]=REACTION_COUNTER;
        CHECK(battle_line_clear(10,U_RAMZA),1);
        attack(U_RAMZA,10);st.hit=1;st.dmg=1;st.push=0;impact();CHECK(st.action.pending!=0,1);
        // A newly solid panel invalidates the return shot, after the
        // eligible original attack has committed on a clear line.
        if(blocked)source=solid_panel(c+1);
        CHECK(battle_line_clear(10,U_RAMZA),!blocked);
        end_anim();CHECK(st.primary.running,!blocked);
        if(blocked){CHECK(st.atk,U_RAMZA);CHECK(st.action.pending,0);}
        else{CHECK(st.atk,10);CHECK(st.action.phase,CTX_SECONDARY);}
        if(blocked)solid_restore(source);
    }
}
static void terminals(void)
{
    u16 jp;
    // Lethal root hit cancels the queued Counter at drain, before victory settles.
    arena();st.unit[10].hp=1;st.reaction[10]=REACTION_COUNTER;root(1);CHECK(st.end,END_NONE);
    end_anim();CHECK(st.primary.running,0);CHECK(st.end,END_NONE);end_anim();CHECK(st.end,END_VICTORY);
    // A lethal Counter prevents root EXP/JP and postpones defeat until it finishes.
    arena();st.unit[U_RAMZA].hp=1;st.reaction[10]=REACTION_COUNTER;jp=st.unit[U_RAMZA].jp;
    root(1);change(U_RAMZA,SID_DONT_ACT);end_anim();st.hit=1;st.dmg=999;impact();
    CHECK(st.unit[U_RAMZA].hp,0);CHECK(st.end,END_NONE);end_anim();CHECK(st.end,END_DEFEAT);
    CHECK(st.action.phase,CTX_IDLE);CHECK(st.unit[U_RAMZA].jp,jp);CHECK(st.unit[10].gjp,0);
}
static void trials(void){saved_reaction();source();counter();potions();ranged_counter();terminals();}
#if !defined(REACTION_FRAME_CHECK) && !defined(REACTION_SEQUENCE_CHECK)
#ifndef __m68k__
int main(void){sw_init(61);trials();printf("reactions %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
#endif
