// Native opening adapter for the common tactical engine: the beats played on the battle map
// (gate, banter, aftermath) and the battle. The title and chapel before it and the chapter
// card after it are the scene player's (campaign/scene.c, ffs0/ffs1).
#define OPENING_BEATS RD16(opening_story, 0)   // the beats bank's count
typedef char OpeningDiskLayout[(sizeof(UnitDef)==28 && sizeof(Item)==30 && sizeof(Job)==14 && sizeof(Ability)==12)?1:-1];
#if MOD_CORE
static const u8 *opening_story, *pgfx;   // pgfx: the speakers' portraits (ffcp)
static void opening_battle(u8 direct);
static void opening_setup(u8 letter);
#endif
#if MOD_PART(MOD_BEATS)                   // with modules: the beats module (fft.h)
#if !MOD_CORE && defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define walk_seq CONTOUR_REF_walk_seq
#endif
static void opening_actor(u8 gfx,u8 slot,s16 x,s16 y,u8 face,u16 logical);
// Only map beats call this: it lives in BEATS, not in the resident core.
// after the battle, the chapter's own state (the charges waiting) put back
static void opening_after(void)
{
    memset(st.pending,0,sizeof st.pending);
    if(flow_bank) {
        const u8 *r=qpart(flow_bank,Q_REWARDS)+(CD(LOCATION)==7?25:0);
        s16 x=r[1]==255?st.unit[U_RAMZA].x:r[1],z=r[2]==255?st.unit[U_RAMZA].z:r[2];
        u8 i,saved=W->actor_budget;
        st.scene_hide=0;W->actor_budget=255;
        for(i=0;i<NUNIT;i++) {
            Unit *u=&st.unit[i];u8 dir;
            if(u->team==TEAM_ENEMY){st.scene_hide|=(u16)(1U<<i);continue;}
            if(u->hp) {
                s16 dx=x-u->x,dz=z-u->z;
                if(abs(dx)>abs(dz))u->face=dx>0?0:1;
                else if(dz)u->face=dz>0?2:3;
            }
            if(u->x>=MAP_W)continue;
            dir=unit_direction(st.rot,i);
            actor_frame((u16)(u->gfx*44+(u->hp?2:P_KO)+(u->hp?(dir&1?5:0):(dir&1))+(dir&2?22:0)),i);
        }
        // The script and surviving battle actors share the LRU frame bank, but
        // not their roles. Warm both while entering the scene, before fade-in.
        if(opening_story) {
            const u8 *b=opening_story+RD16(opening_story+2,st.open_beat),*a=b+4;
            for(i=0;i<b[1];i++,a+=8)actor_frame(RD16(a+6,0),a[0]>>4);
        }
        W->actor_budget=saved;
    }
}
#include "../beats.inc"
#include "../mapbeats.inc"
#if !MOD_CORE
u16 mod_entry(u8 op, u8 a)
{
    if(op==4){cam_target(M.camera_out,M.camera_out+1);return 0;}
    if (op == 0) return map_step();
    if (op == 1) map_render(); else map_start_step(a);
    return 0;
}
#endif
#endif
#if MOD_CORE
#ifndef FFT_MODULES
static void map_start(u8 after)
{
    map_start_step(after);
    if(st.open_stage==OPEN_BEGIN)opening_battle(0);
}
static u8 map_update(void)
{
    u8 active=map_step();
    if(st.open_stage==OPEN_BEGIN)opening_battle(0);
    return active;
}
#endif

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
    rt_seed=97;rand_seed(97);
#ifdef FFT_MODULES
    // RESOURCES returns before PARTY loads; the chapter's resolved prefix is not
    // retained in ModCtx's input union, which already holds chapter_ugfx.
    mod_call(MOD_PARTY,12,(u8)mod_call(MOD_RESOURCES,11,(u8)(chapter_now-1)));
#else
    opening_setup((u8)chapter_route((u8)(chapter_now-1)));
#endif
    if(!objective_bind())return;
    actor_reset();
    cam_target(&st.camx,&st.camy);
    if(n==5){st.open_stage=OPEN_DONE;return;} // AMS chapter-loader injection door: over at once
    if(n==1||n==2||n==3){
        opening_battle(n==1);
        if(n==2)st.ai_all=1;
        if(n==3)map_start(1);
    }else map_start(0);                              // in the scene player's fade
}


#endif
