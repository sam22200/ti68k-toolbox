// Actual forecast/card pixels for clear, intercepted, stacked and blocked guns.
#define DIRECT_RENDER_CHECK
#define FFT_CHECK_NO_ZONES
#include "check_direct_interception.c"
static void direct_render_trials(void)
{
 u8 mode,i,target,ink;u16 x,y;u32 h;const u8 *l,*d;
 for(mode=0;mode<4;mode++){
  direct_arena();target=mode==0?10:mode==1?9:mode==2?8:255;
  if(mode){st.unit[9]=st.unit[10];unit_place(&st.unit[9],second);}
  if(mode==2){st.unit[8]=st.unit[10];unit_place(&st.unit[8],second);}
  if(mode==3)panel_surface[direct_column(first+1)]=20;
  st.mode=M_AIM;st.abil=NO_ABIL;st.act=U_RAMZA;st.ai=AI_OFF;st.banner=st.turn=0;st.tick=0;
  cursor_place(third);memset(st.reach,255,sizeof st.reach);st.reach[third]=1;
  for(i=0;i<24;i++){draw_rect(0,0,RT_W,RT_H,C_WHITE);W->left=30000;draw_hud();}
  CHECK(W->preview_target,target);CHECK(W->preview_count,mode==3?0:mode==2?2:1);
  if(target!=255)CHECK(W->pan_fk[1][0],target+1);
  else{
   l=rt_light;d=rt_dark;ink=0;for(y=PANEL_Y;y<100;y++)for(x=12;x<20;x++)ink|=l[y*RT_PBYTES+x]|d[y*RT_PBYTES+x];
   CHECK(ink,0); // A dormant cache key is allowed; no recipient card is drawn.
  }
  CHECK(W->fc_key!=0,1);
  h=17;l=rt_light;d=rt_dark;
  for(y=0;y<100;y++,l+=RT_PBYTES,d+=RT_PBYTES)for(x=0;x<20;x++){h=(h<<1|h>>31)^l[x];h=(h<<1|h>>31)^d[x];}
  digest=(digest<<1|digest>>31)^h;
#ifndef __m68k__
  if(getenv("FFT_DIRECT_PNG")){char name[256];sprintf(name,"%s/direct-%u.png",getenv("FFT_DIRECT_PNG"),mode);sw_write_png(name,4);}
#endif
  panel_restore();
 }
}
#ifndef __m68k__
int main(void){setbuf(stdout,NULL);sw_init(61);direct_render_trials();printf("direct_interception_render %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();direct_render_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
