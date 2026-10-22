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
    fixed_units();memcpy(st.bag,bag_init,NBAG);
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
