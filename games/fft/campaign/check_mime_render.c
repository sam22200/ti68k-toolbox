// Imitation reuses the ordinary outlined actor, action and impact renderer.
#define COMBAT_UI_CHECK
#define REACTION_FRAME_CHECK
#define FFT_CHECK_NO_ZONES
#include "check_combat.c"
static void render_arena(void)
{
 combat_arena();st.banner=0;st.unit[U_RAMZA].face=0;st.unit[U_RAMZA].eq[EQ_WEAPON]=19;
 st.unit[8]=st.unit[U_RAMZA];unit_place(&st.unit[8],third);st.unit[8].job=93;st.unit[8].face=1;st.unit[8].set=24;st.unit[8].slot=255;
 st.unit[8].eq[EQ_WEAPON]=255;
}
static u32 picture(void)
{
 u8 i;u16 y,x;u32 h=17;const u8 *l,*d;
 cam_target(&st.camx,&st.camy);
 for(i=0;i<16;i++){W->actor_budget=255;W->ac_now++;W->hold=W->frz=0;game_render();}
 l=rt_light;d=rt_dark;
 for(y=0;y<100;y++,l+=RT_PBYTES,d+=RT_PBYTES)for(x=0;x<20;x++){h=(h<<1|h>>31)^l[x];h=(h<<1|h>>31)^d[x];}
 return h;
}
static void render_trials(void)
{
 u8 mode,i,stage;u32 expected[2]={0,0},h;
 for(mode=0;mode<2;mode++){
  render_arena();
  if(mode){attack(U_RAMZA,10);st.hit=0;st.push=0;impact();for(i=0;i<8&&!W->mime.active;i++)end_anim();CHECK(W->mime.active,1);CHECK(st.atk,8);}
  else{st.unit[8].eq[EQ_WEAPON]=19;st.unit[8].set=0;attack(8,10);}
  st.hit=1;st.crit=st.push=st.self=st.gain=0;st.dmg=13;
  for(stage=0;stage<2;stage++){
   st.act_t=stage?ACT_HIT:3;if(stage)impact();h=picture();
   if(mode){CHECK(h>>16,expected[stage]>>16);CHECK(h,expected[stage]);}
   else expected[stage]=h;
   digest=(digest<<1|digest>>31)^h;
#ifndef __m68k__
   if(getenv("FFT_MIME_PNG")){char name[256];sprintf(name,"%s/mime-%u-%u.png",getenv("FFT_MIME_PNG"),mode,stage);sw_write_png(name,4);}
#endif
  }
 }
}
#ifndef __m68k__
int main(void){sw_init(61);render_trials();printf("mime_render %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();render_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
