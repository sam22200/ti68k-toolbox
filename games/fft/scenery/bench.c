// Headless native 68000 experiment. Never run this RT_CYCLES image on a calculator.
#include "bank.h"
#include <stdlib.h>
#include <string.h>
#include "../../../tools/m68kbench/bench.h"
#include "../../../lib/unpack68k.h"

static u8 policy,exact,orbonne,current,original_four;
static SbScene scene;
static const u8 *patches,*originals[4];
static u8 *colors[4],*covers[4],*seen,*whole;
static u8 scratch[256];
static u8 ok;

void game_init(void)
{
    u8 i;ok=0;whole=seen=0;patches=0;
    for(i=0;i<4;i++)colors[i]=covers[i]=0;
}

void game_scenario(u16 n)
{
    u16 bs,ms;u8 i,count;const u8 *bank,*map;
    policy=n&3;exact=(n>>2)&1;orbonne=(n>>3)&1;current=0;original_four=(n>>4)&1;
    bank=rt_file("sbcommon",&bs);map=rt_file(orbonne?"sborb":"sbgar",&ms);
    if(!sb_bind(&scene,bank,bs,map,ms))return;
    if(exact){patches=rt_file(orbonne?"sporb":"spgar",0);if(!patches)return;}
    if(policy==3){
        const char *names[4];
        names[0]=orbonne?"ffov0":"ffgv0";names[1]=orbonne?"ffov1":"ffgv1";
        names[2]=orbonne?"ffov2":"ffgv2";names[3]=orbonne?"ffov3":"ffgv3";
        count=original_four?4:1;
        for(i=0;i<count;i++){
            colors[i]=malloc((u16)(6*scene.plane));if(!colors[i])return;
            covers[i]=colors[i]+2*scene.plane;
        }
        whole=colors[0];
        for(i=0;i<4;i++){originals[i]=rt_file(names[i],0);if(!originals[i])return;}
    }else{
        seen=malloc(scene.plane);if(!seen)return;
        count=policy?4:1;
        for(i=0;i<count;i++){
            colors[i]=malloc((u16)(2*scene.plane));if(!colors[i])return;
            if(policy==1||!i){covers[i]=malloc((u16)(4*scene.plane));if(!covers[i])return;}
            else covers[i]=covers[0];
        }
    }
    ok=1;
}

static void prepare(u8 rot,u8 coverage_only)
{
    u8 index=(policy&&policy!=3)||original_four?rot:0;
    if(policy==3){zx0_asm(originals[rot],colors[index]);return;}
    sb_compose(&scene,rot,coverage_only?0:colors[index],covers[index],seen);
    if(exact&&!sb_patch(patches,rot,coverage_only?0:colors[index],covers[index],scene.plane,scratch))ok=0;
}

static void fingerprint(u8 rot)
{
    u8 index=(policy&&policy!=3)||original_four?rot:0;
    BENCH_VALUE(sb_checksum(colors[index],(u16)(2*scene.plane)));
    BENCH_VALUE(sb_checksum(covers[index],(u16)(4*scene.plane)));
}

void game_render(void)
{
    u8 index=(policy&&policy!=3)||original_four?current:0;u16 row;
    const u8 *l=colors[index]+40*60+10,*d=l+scene.plane;
    u8 *outl=rt_light,*outd=rt_dark;
    for(row=0;row<100;row++,l+=40,d+=40,outl+=30,outd+=30){
        memcpy(outl,l,20);memcpy(outd,d,20);
    }
}

u8 game_update(void)
{
    u8 i,rot,count;
    if(!ok){BENCH_VALUE(0xBAD);return 0;}
    BENCH_NAME(10,"cold map preparation");BENCH_NAME(11,"rotation one");
    BENCH_NAME(12,"rotation two");BENCH_NAME(13,"rotation three");
    BENCH_NAME(14,"rotation back");BENCH_NAME(15,"warm viewport copy");
    count=policy==1||policy==2||original_four?4:1;
    BENCH_BEGIN(10);
    // Leave rotation zero's coverage resident for the mixed policy.
    for(i=0;i<count;i++){rot=count==4?(i+1)&3:0;prepare(rot,0);}
    BENCH_END(10);if(!ok){BENCH_VALUE(0xBAD);return 0;}
    fingerprint(0);
    for(i=0;i<4;i++){
        rot=(i+1)&3;
        BENCH_BEGIN(11+i);
        if(policy==0||(policy==3&&!original_four))prepare(rot,0);
        else if(policy==2)prepare(rot,1);
        current=rot;
        BENCH_END(11+i);fingerprint(rot);
        if(!ok){BENCH_VALUE(0xBAD);return 0;}
    }
    BENCH_BEGIN(15);for(i=0;i<32;i++)game_render();BENCH_END(15);
    if(whole){for(i=0;i<(original_four?4:1);i++)free(colors[i]);}
    else {
        count=policy?4:1;
        for(i=0;i<count;i++){
            free(colors[i]);if(policy==1||!i)free(covers[i]);
        }
    }
    if(seen)free(seen);
    return 0;
}
