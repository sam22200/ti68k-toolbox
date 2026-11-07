// Existing action presentation, now reached by three equipped self reactions.
#define FFT_PRODUCTION_CORE
#define FFT_CHECK_NO_ZONES
#define FFT_CHECK_NATIVE_QUIET
#define BRAVE_FAITH_UI_CHECK
#define REACTION_FRAME_CHECK
#include "check_brave_faith_reactions.c"
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
    if(getenv("FFT_BRAVE_FAITH_PNG")){
        char name[256];sprintf(name,"%s/reaction-%u.png",getenv("FFT_BRAVE_FAITH_PNG"),id);sw_write_png(name,4);
    }
#else
    (void)id;
#endif
}
static void ui_reactions(void)
{
    static const u16 ids[3]={425,429,430};u8 i;
    for(i=0;i<7;i++){
        u16 id=ids[i%3];u8 wall=i>=3&&i<6,no_op=i==6;Unit *t;
        chapter_now=0;arena();st.banner=0;st.tick=0;rt_frame=200;t=&st.unit[10];
        t->hp=50;t->max_hp=100;t->mp=4;t->ct=37;t->brave=70;t->faith=50;st.reaction[10]=0;
        traits(t,19,0,wall?31:no_op?19:255);
        act_start(U_RAMZA,10);st.abil=NO_ABIL;st.hit=0;impact();
        CHECK(core_action_queue(&st.action,10,id,NUNIT),CTX_OK);st.reaction[10]=id;end_anim();
        CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.atk,10);CHECK(st.tgt,10);CHECK(st.hit,!wall);
        reaction_picture(i*2);impact();CHECK(st.action.phase,CTX_APPLIED);
        CHECK(t->brave,!wall&&id==429?73:70);CHECK(t->faith,!wall&&id==430?53:50);
        CHECK(unit_transparent(t)!=0,id==425&&!wall);CHECK(t->hp,50);CHECK(t->mp,4);CHECK(t->ct,37);
        reaction_picture(i*2+1);end_anim();finish_status();
    }
}
#ifndef __m68k__
int main(void){sw_init(61);ui_reactions();printf("brave_faith_reactions_ui %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();ui_reactions();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
