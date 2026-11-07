// Existing action presentation, now reached by three equipped self reactions.
#define FFT_PRODUCTION_CORE
#define FFT_CHECK_NO_ZONES
#define FFT_CHECK_NATIVE_QUIET
#define COUNTER_REACTIONS_UI_CHECK
#define REACTION_FRAME_CHECK
#include "check_counter_reactions.c"
static void reaction_picture(u8 id)
{
    const u8 *l=rt_light,*d=rt_dark;u16 y,x;u8 warm;u32 h=17;
    cam_target(&st.camx,&st.camy);
    for(warm=0;warm<16;warm++){
        W->actor_budget=255;W->ac_now++;W->hold=W->frz=0;
#ifdef __m68k__
    BENCH_BEGIN(63);
#endif
    game_render();
#ifdef __m68k__
    BENCH_END(63);
#endif
    }
    for(y=0;y<100;y++,l+=RT_PBYTES,d+=RT_PBYTES)for(x=0;x<20;x++){
        h=(h<<1|h>>31)^l[x];h=(h<<1|h>>31)^d[x];
    }
    digest=(digest<<1|digest>>31)^h;rt_frame++;
#ifndef __m68k__
    if(getenv("FFT_COUNTER_REACTIONS_PNG")){
        char name[256];sprintf(name,"%s/reaction-%u.png",getenv("FFT_COUNTER_REACTIONS_PNG"),id);sw_write_png(name,4);
    }
#else
    (void)id;
#endif
}
#ifndef COUNTER_UI_FIRST
#define COUNTER_UI_FIRST 0
#define COUNTER_UI_LAST 4
#endif
static void ui_reactions(void)
{
    u8 i;
    for(i=COUNTER_UI_FIRST;i<COUNTER_UI_LAST;i++){
        u8 n;Unit *u;u16 hp,mp;
        chapter_now=0;arena();st.banner=0;st.tick=0;rt_frame=200;u=&st.unit[10];u->mp=0;u->max_mp=20;
#if COUNTER_UI_FIRST == 0
        if(i==0){st.reaction[10]=REACTION_TACKLE;root(1);}
#endif
#if COUNTER_UI_FIRST <= 1 && COUNTER_UI_LAST > 1
        if(i==1){
            counter_surface=malloc(8+10*MAP_NMAX);CHECK(counter_surface!=RT_NULL,1);if(!counter_surface)return;
            st.reaction[10]=REACTION_FLOOD;u->pa=13;u->ma=17;
            unit_place(&st.unit[U_RAMZA],second);unit_place(u,first);st.unit[8]=st.unit[U_RAMZA];unit_place(&st.unit[8],third);
            memset(st.affinity,0,sizeof st.affinity);terrain_at(10,0);root(1);
        }
#endif
#if COUNTER_UI_LAST > 2
        if(i>=2){
            st.unit[U_RAMZA].set=11;st.reaction[10]=REACTION_ABSORB_MP;
            if(i==3)traits(u,0,0,31);
            act_start(U_RAMZA,10);st.abil=1;st.hit=0;impact();
        }
#endif
        rng32=1;end_anim();CHECK(st.action.phase,CTX_SECONDARY);hp=st.unit[U_RAMZA].hp;mp=u->mp;
        for(n=0;n<ACT_HIT&&st.act_t<ACT_HIT;n++)game_update();
        reaction_picture(i*3);impact();CHECK(st.action.phase,CTX_APPLIED);
        CHECK(st.unit[U_RAMZA].hp<=hp,1);CHECK(u->mp,i==2?6:mp);reaction_picture(i*3+1);
        end_anim();finish_status();CHECK(st.atk,U_RAMZA);CHECK(st.tgt,10);reaction_picture(i*3+2);
#if COUNTER_UI_FIRST <= 1 && COUNTER_UI_LAST > 1
        if(i==1){free(counter_surface);counter_surface=RT_NULL;}
#endif
    }
}
#ifndef __m68k__
int main(void){sw_init(61);ui_reactions();printf("counter_reactions_ui %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();ui_reactions();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
