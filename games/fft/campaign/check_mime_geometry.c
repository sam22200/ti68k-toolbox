#include "../../../runtime/core/rt.h"
#include "../mime_rules.h"
#include "x/core-mime/oracle/rotations.h"
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#include "../../../tools/m68kbench/bench.h"
#else
#include <stdio.h>
#endif
static u16 checks,failures;static u32 digest=17;
static void check(u16 a,u16 b){checks++;digest=(digest<<1|digest>>31)^a;if(a!=b)failures++;}
#define CHECK(a,b) check((u16)(a),(u16)(b))
static void geometry(void)
{
 static const u8 faces[4]={3,1,2,0};u16 i;const s16 *v;s16 dx,dz;u8 layer;
 for(i=0;i<sizeof source_rotations/sizeof source_rotations[0];i++){
  v=source_rotations[i];dx=v[2];dz=v[3];core_mime_rotate(&dx,&dz,(faces[v[1]]-faces[v[0]])&3);
  CHECK(dx+5,v[5]);CHECK(dz+5,v[6]);CHECK(v[4],0);CHECK(v[7],0);
 }
 for(i=0;i<sizeof source_floors/sizeof source_floors[0];i++){
  v=source_floors[i];layer=core_mime_layer(v[0],v[1],v[2],v[3]|(v[4]<<1),v[5]);
  CHECK(layer?0:65535,v[6]);CHECK(layer?layer-1:65535,v[7]);
 }
}

#ifndef __m68k__
int main(void){geometry();printf("mime_geometry %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){geometry();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
