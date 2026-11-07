// Existing tactical presentation: Catch and applied scalar/batch MP results.
#define COMBAT_UI_CHECK
#define REACTION_FRAME_CHECK
#define FFT_CHECK_NO_ZONES
#include "check_combat.c"
static u16 pictures;
static void combat_picture(void)
{
 u8 i;u16 y,x;u32 h=17;const u8 *l,*d;
 cam_target(&st.camx,&st.camy);
 for(i=0;i<16;i++){
  W->actor_budget=255;W->ac_now++;W->hold=W->frz=0;
  ZB(63);game_render();ZE(63);
 }
 l=rt_light;d=rt_dark;
 for(y=0;y<100;y++,l+=RT_PBYTES,d+=RT_PBYTES)for(x=0;x<20;x++){h=(h<<1|h>>31)^l[x];h=(h<<1|h>>31)^d[x];}
 digest=(digest<<1|digest>>31)^h;pictures++;
#ifndef __m68k__
 if(getenv("FFT_COMBAT_PNG")){char name[256];sprintf(name,"%s/combat-%u.png",getenv("FFT_COMBAT_PNG"),pictures);sw_write_png(name,4);}
#endif
}
static void ui_combat(void)
{
 u8 i;u32 hp[7],mp[7];chapter_now=0;
 combat_arena();st.banner=0;st.unit[U_RAMZA].set=20;st.unit[U_RAMZA].learned=0xfff;st.reaction[10]=450;battle_save.inv[122]=2;
 skill(U_RAMZA,10,634);impact();CHECK(act_kind(),AB_CATCH);combat_picture();CHECK(W->num_ok&1,1);
 combat_arena();st.reaction[10]=445;root(17);CHECK(act_kind(),AB_MP_DAMAGE);combat_picture();CHECK(W->num_ok&1,1);
 combat_arena();st.reaction[10]=445;frozen(U_RAMZA,10,AB_DAMAGE,17,16);W->area_impact.ready=0;W->area_impact.next=0;W->area_impact.oil_ready=0;
 for(i=0;i<NUNIT+2;i++){
#ifdef FFT_MODULES
 mod_call(MOD_AREA_UI,AREA_NUMBERS,0);
#else
 area_number_prepare();
#endif
 }
 memcpy(hp,W->area_impact.glyph[10],sizeof hp);impact();CHECK(W->area_impact.ready,0);combat_picture();
 CHECK(W->area_impact.ready,(u16)1<<10);memcpy(mp,W->area_impact.glyph[10],sizeof mp);CHECK(memcmp(hp,mp,sizeof hp)!=0,1);
 combat_arena();st.reaction[10]=446;root(17);end_anim();CHECK(st.abil,446);impact();combat_picture();
}
#ifndef __m68k__
int main(void){sw_init(61);ui_combat();printf("combat_ui %08lX failures %u checks %u pictures %u\n",(unsigned long)digest,failures,checks,pictures);return failures!=0;}
#else
void _main(void){reaction_runtime_main();ui_combat();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
