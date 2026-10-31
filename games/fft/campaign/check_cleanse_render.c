// Byte-exact relocation controls, without starting a UI emulator.
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#define wait_arrow reference_wait_arrow
#define draw_arrows reference_draw_arrows
#include "facing_reference.inc"
#undef wait_arrow
#undef draw_arrows
#undef FFT_FORECAST_BOX_INC
#define forecast_panel_text reference_panel_text
#include "forecast_box_reference.inc"
#undef forecast_panel_text
static u32 plane_hash(const u8 *p,u16 n)
{
    u32 h=17;for(;n;n--,p++)h=(h<<1|h>>31)^*p;return h;
}
static void seed_planes(u8 salt)
{
    u16 i;for(i=0;i<RT_PSIZE;i++){((u8*)rt_light)[i]=(u8)(i*13+salt);((u8*)rt_dark)[i]=(u8)(i*7+salt+3);}
}
static void relocation_trials(void)
{
    u8 rot,face,j;u8 *saved=(u8*)W->bk;u32 before,after;Feet f;char text[28];void *l,*d;
    CHECK(sizeof W->bk>=2*RT_PSIZE,1);
    for(rot=0;rot<4;rot++)for(face=0;face<4;face++)for(j=0;j<2;j++){
        st.rot=rot;st.act=U_RAMZA;st.unit[U_RAMZA].face=face;
        f=unit_feet(rot,U_RAMZA);st.camx=f.x-80;st.camy=f.y-50;dst_screen(0,0);
        seed_planes(j);before=plane_hash(rt_light,RT_PSIZE)^plane_hash(rt_dark,RT_PSIZE);
        reference_draw_arrows();after=plane_hash(rt_light,RT_PSIZE)^plane_hash(rt_dark,RT_PSIZE);
        CHECK(before!=after,1);digest=(digest<<1|digest>>31)^after;
        memcpy(saved,rt_light,RT_PSIZE);memcpy(saved+RT_PSIZE,rt_dark,RT_PSIZE);
        seed_planes(j);l=rt_light;d=rt_dark;draw_arrows();
        CHECK(rt_light==l&&rt_dark==d,1);CHECK(memcmp(saved,rt_light,RT_PSIZE),0);
        CHECK(memcmp(saved+RT_PSIZE,rt_dark,RT_PSIZE),0);
    }
    for(j=0;j<sizeof cleanses/sizeof cleanses[0];j++){
        prepare_cleanse(cleanses[j]);
        seed_profile(&st.unit[10],cj_section(common,CJ_STATUS)+6*abilities[cleanses[j]].y,1);
        forecast_text(U_RAMZA,10,cleanses[j],text);l=rt_light;d=rt_dark;
        reference_panel_text(text);memcpy(saved,W->forecast,sizeof W->forecast);
        make_forecast(U_RAMZA,10,cleanses[j]);CHECK(rt_light==l&&rt_dark==d,1);
        CHECK(memcmp(saved,W->forecast,sizeof W->forecast),0);
    }
}
#ifndef __m68k__
int main(void){sw_init(61);relocation_trials();printf("cleanse_render %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();relocation_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
