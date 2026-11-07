// Same secondary shot frames for Hamedo and the existing Counter renderer.
#define COMBAT_UI_CHECK
#define REACTION_FRAME_CHECK
#define FFT_CHECK_NO_ZONES
#include "check_combat.c"
static u8 bow,far_cell;
static void ranged_arena(u8 blocked)
{
 u16 c,w;combat_arena();st.golem[0]=st.golem[1]=0;
 for(w=0;w<128;w++)if(items[w].knife==2&&items[w].reach>=4)break;
 CHECK(w<128,1);bow=(u8)w;
 for(c=0;c+3<MAP_N;c++)if(TILE_Z[c]==TILE_Z[c+3]&&map_tiles[c].walk==1&&map_tiles[c+1].walk==1&&map_tiles[c+2].walk==1&&map_tiles[c+3].walk==1&&map_tiles[c].stand==map_tiles[c+1].stand&&map_tiles[c].stand==map_tiles[c+2].stand&&map_tiles[c].stand==map_tiles[c+3].stand)break;
 CHECK(c+3<MAP_N,1);far_cell=c+3;unit_place(&st.unit[U_RAMZA],c);unit_place(&st.unit[10],far_cell);st.unit[10].eq[EQ_WEAPON]=bow;
 st.reaction[10]=REACTION_HAMEDO;
 if(blocked){st.unit[8]=st.unit[10];st.unit[8].eq[EQ_WEAPON]=255;unit_place(&st.unit[8],c+1);st.reaction[8]=0;}
 CHECK(ranged_reach(10,far_cell,UNIT_CELL(&st.unit[U_RAMZA])),1);CHECK(battle_line_clear(10,U_RAMZA),!blocked);
}

static u32 picture(void)
{
 u8 i;u16 y,x;u32 h=17;const u8 *l,*d;cam_target(&st.camx,&st.camy);
 for(i=0;i<16;i++){W->actor_budget=255;W->ac_now++;W->hold=W->frz=0;game_render();}
 l=rt_light;d=rt_dark;
 for(y=0;y<100;y++,l+=RT_PBYTES,d+=RT_PBYTES)for(x=0;x<20;x++){h=(h<<1|h>>31)^l[x];h=(h<<1|h>>31)^d[x];}
 return h;
}
static void render_trials(void)
{
 u8 mode,stage;u32 expected[2],h;
 for(mode=0;mode<2;mode++){
  ranged_arena(0);st.banner=0;rng32=1;
#ifdef __m68k__
  if(mode){BENCH_BEGIN(62);}else{BENCH_BEGIN(61);}
#endif
  if(mode){attack(U_RAMZA,10);CHECK(st.atk,10);CHECK(st.action.phase,CTX_SECONDARY);}
  else{st.reaction[10]=REACTION_COUNTER;attack(U_RAMZA,10);st.hit=0;st.push=0;impact();end_anim();CHECK(st.atk,10);CHECK(st.action.phase,CTX_SECONDARY);}
#ifdef __m68k__
  if(mode){BENCH_END(62);}else{BENCH_END(61);}
#endif
  st.hit=1;st.crit=st.push=st.self=st.gain=0;st.dmg=13;
  for(stage=0;stage<2;stage++){
   st.act_t=stage?ACT_HIT:ACT_HIT-4;if(stage)impact();h=picture();
   if(mode){CHECK(h>>16,expected[stage]>>16);CHECK(h,expected[stage]);}else expected[stage]=h;
   digest=(digest<<1|digest>>31)^h;
#ifndef __m68k__
   if(getenv("FFT_HAMEDO_PNG")){char name[256];sprintf(name,"%s/hamedo-%u-%u.png",getenv("FFT_HAMEDO_PNG"),mode,stage);sw_write_png(name,4);}
#endif
  }
 }
}
#ifndef __m68k__
int main(void){sw_init(61);render_trials();printf("hamedo_render %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();render_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
