// Reuse approved sprites/UI; Jump only hides the airborne actor.
#define FFT_PRODUCTION_CORE
#define FFT_CHECK_NO_ZONES
#define FFT_CHECK_NATIVE_QUIET
#define JUMP_UI_CHECK
#define REACTION_FRAME_CHECK
#include "check_lancer_jump.c"
static void jump_picture(u8 id)
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
    if(getenv("FFT_JUMP_PNG")){
        char name[256];sprintf(name,"%s/jump-%u.png",getenv("FFT_JUMP_PNG"),id);sw_write_png(name,4);
    }
#else
    (void)id;
#endif
}
static void jump_ui(void)
{
    u8 rot;
    for(rot=0;rot<4;rot++){
        chapter_now=0;ready();st.rot=rot;st.banner=0;st.tick=0;rt_frame=200;
        st.unit[U_RAMZA].eq[EQ_WEAPON]=99;st.unit[U_RAMZA].zodiac=st.unit[10].zodiac=0;
        aim(JUMP_FIRST);cursor_place(second);CHECK(st.reach[second],1);jump_picture(4*rot);
        skill(U_RAMZA,10,JUMP_FIRST);CHECK(unit_airborne(&st.unit[U_RAMZA])!=0,1);
        st.mode=M_BROWSE;st.sel=10;jump_picture(4*rot+1);
        st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);CHECK(st.hit,1);jump_picture(4*rot+2);
        impact();CHECK(st.unit[10].hp,943);jump_picture(4*rot+3);
    }
}
#ifndef __m68k__
int main(void){sw_init(61);jump_ui();printf("lancer_jump_ui %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();jump_ui();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
