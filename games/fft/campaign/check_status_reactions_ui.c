// Existing action presentation, now reached by three equipped self reactions.
#define FFT_PRODUCTION_CORE
#define FFT_CHECK_NO_ZONES
#define FFT_CHECK_NATIVE_QUIET
#define STATUS_REACTIONS_UI_CHECK
#define REACTION_FRAME_CHECK
#include "check_status_reactions.c"
static void status_picture(u8 id)
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
    if(getenv("FFT_STATUS_REACTIONS_PNG")){
        char name[256];sprintf(name,"%s/status-%u.png",getenv("FFT_STATUS_REACTIONS_PNG"),id);sw_write_png(name,4);
    }
#else
    (void)id;
#endif
}
static void ui_status(void)
{
    u8 i;
    for(i=0;i<9;i++){
        u8 r=i%3,mode=i/3,sid=r==0?6:r==1?18:25;Unit *t;
        chapter_now=0;arena();st.banner=0;st.tick=0;rt_frame=200;t=&st.unit[10];
        t->hp=50;t->max_hp=100;t->mp=4;t->ct=37;st.reaction[10]=0;
        traits(t,sid,mode==1?1:0,mode==2?31:255);
        root(0);CHECK(core_action_queue(&st.action,10,REACTION_CAUTION+r,NUNIT),CTX_OK);
        st.reaction[10]=REACTION_CAUTION+r;end_anim();
        CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.atk,10);CHECK(st.tgt,10);
        CHECK(st.hit,mode!=2);CHECK(st.dmg!=0,mode==0);
        status_picture(i*2);impact();CHECK(st.action.phase,CTX_APPLIED);
        CHECK(core_status_current(&t->statuses,sid>>3)&(0x80>>(sid&7)),mode==0?0x80>>(sid&7):0);
        CHECK(t->hp,50);CHECK(t->mp,4);CHECK(t->ct,37);status_picture(i*2+1);
        end_anim();finish_status();
    }
}
#ifndef __m68k__
int main(void){sw_init(61);ui_status();printf("status_reactions_ui %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();ui_status();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
