// Original configuration and probability boundaries, no renderer or game state.
#ifdef __m68k__
#define USE_TI89
#include <tigcclib.h>
#include "../../../tools/m68kbench/bench.h"
#else
#include <stdio.h>
#include <string.h>
#endif
#include "../../../runtime/core/rt.h"
#include "../movement_rules.h"
#include "x/core-movement/oracle/source.h"
static u16 checks,failures;static u32 digest=17;
static void equal(u16 a,u16 b,u16 line){checks++;digest=(digest<<1|digest>>31)^a;if(a!=b){failures++;
#ifndef __m68k__
printf("line%u: %u expected%u\n",line,a,b);
#else
BENCH_VALUE(line);
#endif
}}
#define CHECK(a,b) equal(a,b,__LINE__)
static void trials(void)
{
 u16 n,j;u8 f[3];MoveConfig c;
 for(n=0;n<sizeof source_movement_config/sizeof source_movement_config[0];n++){
  const u16 *p=source_movement_config[n];memset(f,0,3);
  if(p[0]){j=p[0]-486;f[j>>3]=(u8)(128>>(j&7));}
  core_movement_config(&c,f,3,(u8)p[1]);
  CHECK(c.jump<<1,p[3]);CHECK(c.jump,p[4]);CHECK(c.jump_half,p[5]);CHECK(c.kind,p[7]);CHECK(c.row,p[8]);
  CHECK(c.flags[2],p[9]);CHECK(c.body,p[14]);CHECK(c.stored,p[15]);CHECK(c.kind<2,p[16]);CHECK(c.lava,p[18]);
  if(c.kind==1)CHECK(c.flags[1],p[17]);
  for(j=0;j<64;j++)CHECK(core_movement_cost(source_movement_costs[((u16)c.row<<6)+j],f[1],(u8)p[2]),p[19+j]);
 }
 for(n=0;n<sizeof source_movement_bonus/sizeof source_movement_bonus[0];n++){
  const u16 *p=source_movement_bonus[n];u8 move=(u8)p[1],jump=(u8)p[2];
  core_movement_bonus((u8)p[0],&move,&jump);CHECK(move,p[3]);CHECK(jump,p[4]);
 }
 for(n=0;n<sizeof source_movement_teleport/sizeof source_movement_teleport[0];n++){
  const u16 *p=source_movement_teleport[n];u8 chance=core_teleport_chance((u8)p[0],4);
  CHECK(((u32)p[1]*100>>15)>=100-chance,p[2]);CHECK(p[3],1);
 }
}
#ifndef __m68k__
int main(void){trials();printf("movement_rules %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
