// Existing reach rings and actor renderer, four views on the actual Gariland map.
#define FFT_PRODUCTION_CORE
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main movement_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "gariland.c"
static u16 checks,failures,pictures;static u32 digest=17;
static void equal(u16 a,u16 b){checks++;digest=(digest<<1|digest>>31)^a;if(a!=b)failures++;}
static void picture(void)
{
 const u8 *l,*d;u16 y,x;u8 warm;u32 h=17;
 cam_target(&st.camx,&st.camy);
 for(warm=0;warm<16;warm++){W->actor_budget=255;W->ac_now++;W->hold=W->frz=0;game_render();}
 l=rt_light;d=rt_dark;
 for(y=0;y<100;y++,l+=RT_PBYTES,d+=RT_PBYTES)for(x=0;x<20;x++){h=(h<<1|h>>31)^l[x];h=(h<<1|h>>31)^d[x];}
 digest=(digest<<1|digest>>31)^h;pictures++;
}
static void trials(void)
{
 const u16 ids[4]={488,506,507,498};u8 kind,rot,i;u16 c;Unit *u;u32 seed;
 equal(W!=0,1);if(!W)return;
 for(kind=0;kind<4;kind++)for(rot=0;rot<4;rot++){
  game_scenario(61);st.act=st.sel=U_RAMZA;st.ai=AI_OFF;st.banner=0;st.mode=M_TARGET;st.rot=rot;st.want=st.prot=255;
  st.tick=0;rt_frame=200;u=&st.unit[U_RAMZA];st.movement[U_RAMZA]=(u8)(ids[kind]-485);
#ifdef FFT_MODULES
  mod_call(MOD_PARTY,255,0);mod_context(MOD_PARTY)->stats_unit=u;mod_call(MOD_PARTY,9,0);
#else
  unit_stats(u);
#endif
  unit_status_seed(u,0);unit_status_refresh(u);cursor_place(UNIT_CELL(u));seed=rng32;
  ZB(61);compute_reach(U_RAMZA);ZE(61);equal(rng32==seed,1);picture();
  ZB(62);compute_reach(U_RAMZA);ZE(62);equal(rng32==seed,1);
  for(c=0;c<MAP_N;c++)if(c!=UNIT_CELL(u)&&st.reach[c]!=255&&abs((s16)TILE_X[c]-u->x)+abs((s16)TILE_Z[c]-u->z)==1)break;
  equal(c<MAP_N,1);if(c>=MAP_N)continue;
  equal(make_path_cell(U_RAMZA,(u8)c)>=2,1);st.mode=M_WALK;st.path_i=st.walk_t=0;
  for(i=0;i<100&&st.mode==M_WALK;i++){rt_prev=rt_keys;rt_keys=0;game_update();rt_frame++;}
  equal(UNIT_CELL(u),c);equal(st.moved,1);equal(i<100,1);picture();
 }
}
#ifndef __m68k__
int main(void){sw_init(61);trials();printf("movement_ui %08lX failures %u checks %u pictures %u\n",(unsigned long)digest,failures,checks,pictures);return failures!=0;}
#else
void _main(void){movement_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
