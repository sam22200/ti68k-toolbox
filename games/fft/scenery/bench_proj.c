// Headless native 68000 experiment. Never run this RT_CYCLES image on a calculator.
// One cached orientation, prepared by runtime projection of shared face textures.
#include "proj.h"
#include "bank.h"
#include <stdlib.h>
#include <string.h>
#include "../../../tools/m68kbench/bench.h"

static SpScene scene;
static u8 *color,*cover,*seen,ok;

void game_init(void){ok=0;color=cover=seen=0;}

void game_scenario(u16 n)
{
    u16 bs,ms;const u8 *bank=rt_file("spgeo",&bs),*map=rt_file(n&1?"spmorb":"spmgar",&ms);
    if(!sp_bind(&scene,bank,bs,map,ms))return;
    color=malloc((u16)(2*scene.plane));cover=malloc((u16)(4*scene.plane));seen=malloc(scene.plane);
    ok=color&&cover&&seen;
}

void game_render(void){}

u8 game_update(void)
{
    u8 i,rot;
    if(!ok){BENCH_VALUE(0xBAD);return 0;}
    BENCH_NAME(10,"cold map preparation");BENCH_NAME(11,"rotation one");
    BENCH_NAME(12,"rotation two");BENCH_NAME(13,"rotation three");
    BENCH_NAME(14,"rotation back");
    BENCH_BEGIN(10);sp_compose(&scene,0,color,cover,seen);BENCH_END(10);
    BENCH_VALUE(sb_checksum(color,(u16)(2*scene.plane)));BENCH_VALUE(sb_checksum(cover,(u16)(4*scene.plane)));
    for(i=0;i<4;i++){
        rot=(i+1)&3;
        BENCH_BEGIN(11+i);sp_compose(&scene,rot,color,cover,seen);BENCH_END(11+i);
        BENCH_VALUE(sb_checksum(color,(u16)(2*scene.plane)));BENCH_VALUE(sb_checksum(cover,(u16)(4*scene.plane)));
    }
    free(seen);free(cover);free(color);
    return 0;
}
