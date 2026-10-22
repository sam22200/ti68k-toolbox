// Native opening adapter for the common tactical engine: the beats played on the battle map
// (gate, banter, aftermath) and the battle. The title and chapel before it and the chapter
// card after it are the scene player's (campaign/scene.c, ffs0/ffs1).
#include "data/story.h"
typedef char OpeningDiskLayout[(sizeof(UnitDef)==24 && sizeof(Item)==23 && sizeof(Job)==14 && sizeof(Ability)==24 && sizeof(SkillSet)==18)?1:-1];
enum { OPEN_PLAY, OPEN_TITLE, OPEN_CHAPEL, OPEN_GATE, OPEN_AFTER, OPEN_DONE, OPEN_BANTER };
#if MOD_CORE
static const u8 *opening_story, *pgfx;   // pgfx: the speakers' portraits (ffcp)
static void opening_battle(u8 direct);
static void opening_after(void);
#endif
#if MOD_PART(MOD_BEATS)                   // with modules: the beats module (fft.h)
#if !MOD_CORE && defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define walk_seq CONTOUR_REF_walk_seq
#endif
static void opening_actor(u8 gfx,u8 slot,s16 x,s16 y,u8 face,u16 logical);
#include "../beats.inc"
#include "../mapbeats.inc"
#if !MOD_CORE
u16 mod_entry(u8 op, u8 a)
{
    if (op == 0) return map_update();
    if (op == 1) map_render(); else map_start(a);
    return 0;
}
#endif
#endif
#if MOD_CORE

enum { DEFER_NONE, DEFER_CHARGING, DEFER_AIRBORNE, DEFER_PERFORMING };
static u8 opening_pending(void)
{
    u8 i;
    for (i=0;i<NUNIT;i++) if (st.pending[i].state) {
        const Unit *u=&st.unit[i];
        if (!u->hp || u->z*MAP_W+u->x!=st.pending[i].origin || u->eq[EQ_WEAPON]!=st.pending[i].weapon) {
            st.pending[i].state=DEFER_NONE;continue;
        }
        if (!st.pending[i].ticks) return i;
    }
    return 255;
}

static void opening_resolve(void)
{
    u8 a=opening_pending(),t,ab=st.pending[a].ability,c=st.pending[a].tile;
    st.pending[a].state=DEFER_NONE;
    t=unit_at(c%MAP_W,c/MAP_W);
    if (!t || !st.unit[t-1].hp) { turn_start(); return; }
    st.open_resolving=1;st.act=a;st.ai=AI_OFF;
    st.cx=c%MAP_W;st.cz=c/MAP_W;
    skill(a,t-1,ab);
}


static void opening_units(void)
{
    u8 i,k;
    for (i=0;i<NUNIT;i++) {
        const UnitDef *d=&unit_defs[i];Unit *u=&st.unit[i];
        memset(u,0,sizeof *u);
        u->x=d->x;u->z=d->z;u->gfx=d->gfx;u->job=d->job;u->team=d->team;u->face=d->face;
        u->level=u->lv0=d->level;u->brave=d->brave;u->faith=d->faith;u->female=d->type==1;
        u->zodiac=opening_zodiac[i];u->set=d->set;u->learned=d->learned;u->dc=3;
        for(k=0;k<5;k++){u->eq[k]=d->eq[k][0];u->raw[k]=RD32(igfx+OPENING_RAW,(u16)(i*10+2*k));}
        unit_stats(u);
    }
}

static u8 opening_attack_range(u8 a,u8 o,u8 c)
{
    const Unit *u=&st.unit[a];
    u8 range=u->eq[EQ_WEAPON]==255 ? 1:opening_range[u->eq[EQ_WEAPON]];
    s16 dx=(s16)TILE_X[c]-TILE_X[o],dz=(s16)TILE_Z[c]-TILE_Z[o],h=(s16)map_tiles[c].stand-map_tiles[o].stand;
    u8 d=(u8)((dx<0?-dx:dx)+(dz<0?-dz:dz));
    if(!map_tiles[c].walk)return 0;
    if(range==1)return d==1 && h>=-6 && h<=5;
    // FFT bow range grows downhill. Minimum range three; terrain collision is
    // checked separately during targeting (the bounded native projectile rule).
    range=(u8)(range+(h<0?(-h)>>2:0));
    return d>=3 && d<=range;
}

static u8 opening_hit(u8 a,u8 t)
{
    const Unit *u=&st.unit[t];u8 side=attack_side(a,t),base=u->status&ST_DEFEND?50:100;
    u16 hit=100;u8 ev=side==SIDE_FRONT?jobs[u->job].cev:0;
    if(ev>base)ev=base;
    hit=(u16)((base-ev)*100/base);
    if(side!=SIDE_BACK&&u->eq[EQ_SHIELD]!=255){
        ev=opening_evade[u->eq[EQ_SHIELD]];if(ev>base)ev=base;
        hit=(u16)(hit*(base-ev)/base);
    }
    return (u8)hit;
}

static void opening_effect(void)
{
    u8 t=st.act_t;
    if(st.mode!=M_ACT)return;
    if((st.abil==NO_ABIL||(abilities[st.abil].flags&AF_CHARGE))&&st.unit[st.atk].eq[EQ_WEAPON]!=255&&opening_range[st.unit[st.atk].eq[EQ_WEAPON]]>1){
        if(t>=ACT_HIT-8&&t<ACT_HIT){
            Feet a=unit_feet(st.rot,st.atk),b=unit_feet(st.rot,st.tgt);u8 k=t-(ACT_HIT-8);
            s16 x=a.x+((b.x-a.x)*k>>3)-st.camx,y=a.y+((b.y-a.y)*k>>3)-st.camy-12;
            static const u8 arc[8]={0,3,5,6,6,5,3,0};y-=arc[k];
            draw_rect(x-2,y-1,6,3,C_WHITE);draw_rect(x-1,y,4,1,C_BLACK);
        }
    }else if(st.abil!=NO_ABIL&&st.abil!=ABIL_DEFEND&&(abilities[st.abil].flags&AF_WEAPON)&&t>=ACT_HIT-5&&t<ACT_HIT+6){
        Feet b=unit_feet(st.rot,st.tgt);s16 x=b.x-st.camx,y=b.y-st.camy;
        u8 h=(u8)(6+(t%5)*4);draw_rect(x-3,y-h,6,h,C_WHITE);draw_rect(x-1,y-h,2,h,C_DGRAY);
    }
}

static void opening_damage(void)
{
    const Ability *b;
    Unit *a=&st.unit[st.atk],*t=&st.unit[st.tgt];
    if(st.abil==NO_ABIL)return;
    b=&abilities[st.abil];
    if(b->flags&AF_DRAIN){u16 n=st.dmg; if(n>a->max_hp-a->hp)n=a->max_hp-a->hp; a->hp+=n;}
    if(b->flags&AF_AREA){
        u8 i;
        // Cross-shaped Stasis Sword splash, including allies (no team filtering).
        for(i=0;i<NUNIT;i++){
            Unit *v=&st.unit[i];s16 dx=(s16)v->x-t->x,dz=(s16)v->z-t->z;
            u8 d=(u8)((dx<0?-dx:dx)+(dz<0?-dz:dz));u16 n;
            if(i==st.tgt||!v->hp||d!=1)continue;
            n=skill_amount(st.atk,i,st.abil,0);v->hp=n>=v->hp?0:v->hp-n;
            if(!v->hp)v->kos++;
        }
    }
}


static void opening_battle(u8 direct)
{
    u8 i;
    st.open_stage=0;st.mode=M_BROWSE;st.end=0;st.banner=0;st.ai=0;
    scene_rot=vbuf_rot=0xFF;W->bg_ok=W->frz=W->hold=0;
    for(i=0;i<NUNIT;i++){W->cover[i].rot=0xFF;W->feet_k[i]=0;}
    turn_start();
    if(direct){
        while(st.act!=U_RAMZA)turn_end(0,0);
        st.ai=AI_OFF;st.mode=M_MENU;
    }else bn_start(BN_START);
    {u8 saved=W->actor_budget;W->actor_budget=255;for(i=0;i<NUNIT;i++)unit_frame(st.rot,i);W->actor_budget=saved;}
    cam_target(&st.camx,&st.camy);hl_seq++;
}

static void opening_start(u16 n)
{
#if defined(FFT_MEASURE) && defined(RT_CYCLES)
    frame_stall=1; // restart/scene-bank preparation, not an ordinary animation frame
#endif
    memset(&st,0,sizeof st);if(!W)return;
    st.want=st.prot=st.ai_view=255;rt_seed=97;rand_seed(97);
    opening_story=rt_file("ffost",RT_NULL);
    pgfx=rt_file("ffcp",RT_NULL);                    // (none: name-only boxes)
    if(opening_story)opening_story+=2;
    opening_units();memcpy(st.bag,bag_init,NBAG);
    actor_reset();W->dialog_key=0xFFFF;
    W->bg_ok=W->hold=W->frz=W->status_i=0;W->pan_ok[0]=W->pan_ok[1]=0;
    W->win_k[0]=0;W->pan_k[0][0]=W->pan_k[1][0]=W->fc_key=0;
    scene_rot=vbuf_rot=0xFF;
    st.rot=2;st.act=st.sel=U_RAMZA;st.cx=st.unit[0].x;st.cz=st.unit[0].z;st.mode=M_BROWSE;
    cam_target(&st.camx,&st.camy);
    if(n==5){st.open_stage=OPEN_DONE;return;} // AMS chapter-loader injection door: over at once
    if(n==1||n==2||n==3){
        opening_battle(n==1);
        if(n==2)st.ai_all=1;
        if(n==3)map_start(1);
    }else map_start(0);                              // in the scene player's fade
}


// after the battle, the chapter's own state (the charges waiting) put back
static void opening_after(void)
{
    memset(st.pending,0,sizeof st.pending);
}
#endif
