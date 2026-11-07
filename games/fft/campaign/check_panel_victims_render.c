// Existing forecast/portrait style, now fed the actual weapon recipient.
#define PANEL_RENDER_CHECK
#define FFT_CHECK_NO_ZONES
#include "check_panel_victims.c"
static u32 panel_picture(void)
{
 u8 i;u16 y,x;u32 h=17;const u8 *l,*d;
 // Actual HUD on a neutral background: isolate its pixels and recipient
 // cache from the independently budgeted tactical renderer and frame loop.
 draw_rect(0,0,RT_W,RT_H,C_WHITE);
 for(i=0;i<24;i++){W->left=30000;draw_hud();}
 l=rt_light;d=rt_dark;
 for(y=0;y<100;y++,l+=RT_PBYTES,d+=RT_PBYTES)for(x=0;x<20;x++){h=(h<<1|h>>31)^l[x];h=(h<<1|h>>31)^d[x];}
 return h;
}
static void panel_render_trials(void)
{
 u8 mode;u32 h;
 for(mode=0;mode<3;mode++){
  panel_arena(0);st.unit[U_RAMZA].eq[EQ_WEAPON]=99;unit_place(&st.unit[10],third);
  if(mode){st.unit[8]=st.unit[10];unit_place(&st.unit[8],second);}
  if(mode==2){st.unit[9]=st.unit[10];unit_place(&st.unit[9],second);}
  st.mode=M_AIM;st.abil=NO_ABIL;st.act=U_RAMZA;st.ai=AI_OFF;st.banner=st.turn=0;st.tick=0;
  cursor_place(third);memset(st.reach,255,sizeof st.reach);st.reach[third]=1;
  CHECK(st.open_stage,0);CHECK(st.open_fade,0);CHECK(st.config,0);CHECK(st.morph_mask,0);
  CHECK(unit_at_cell(CURSOR_CELL),11);CHECK(aim_ok(10),1);
  h=panel_picture();CHECK(W->preview_target,mode?8:10);CHECK(W->preview_count,mode==2?2:1);
  CHECK(W->pan_fk[1][0],(mode?8:10)+1);CHECK(W->fc_key!=0,1);
  digest=(digest<<1|digest>>31)^h;
#ifndef __m68k__
  if(getenv("FFT_PANEL_PNG")){char name[256];sprintf(name,"%s/panel-%u.png",getenv("FFT_PANEL_PNG"),mode);sw_write_png(name,4);}
#endif
  panel_restore();
 }
}
#ifndef __m68k__
int main(void){setbuf(stdout,NULL);sw_init(61);panel_render_trials();printf("panel_victims_render %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();panel_render_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
