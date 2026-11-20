// Actual approved management/deployment pictures and portable archive boundaries.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main monster_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#else
#include <assert.h>
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#ifdef MONSTER_DEPLOY_REVIEW
#include "deploy.c"
#else
#define game_scenario source_scenario
#include "manage.c"
#undef game_scenario
#endif
static u16 failures,checks;
#define CHECK(x) do{checks++;if(!(x))failures++;}while(0)
static const u8 subjects[]={94,95,133,140,130,127};
static void formation_fixture(u16 n)
{
    u8 i;
    CHECK(bank&&common&&cache&&save.n==7);CHECK(monster_bank!=RT_NULL);
    for(i=0;i<48;i++)CHECK(monster_formation_frame(monster_bank,(u8)(94+i))==monster_formation_frame(monster_bank,(u8)(94+(i/3)*3)));
    CHECK(!monster_formation_frame(monster_bank,93));CHECK(!monster_formation_frame(monster_bank,142));
    CHECK(!monster_formation_name(monster_bank,255));
    if(!n||n>=100)monster_bank=RT_NULL;
    n%=100;save.n=10;
    for(i=1;i<8;i++){
        SaveUnit base=save.unit[1];save.unit[i]=base;
        save.unit[i].type=3;save.unit[i].job=i==7?94:subjects[i-1];save.unit[i].gfx=255;
        save.unit[i].name=(u8)(i*7);save.unit[i].level=1;save.unit[i].exp=0;
        memset(save.unit[i].eq,0,sizeof save.unit[i].eq);
        memset(save.unit[i].learned,0,sizeof save.unit[i].learned);
        world_party_clear_slot(&save.world,i);
    }
    save.world.egg[7]=0x32;
    CHECK(save.unit[1].job==94&&save.unit[2].job==95);
    CHECK(job_name(&save.unit[1],0)[0]!=job_name(&save.unit[2],0)[0]);
    save.unit[8]=save.unit[0];save.unit[9]=save.unit[0];
#ifdef MONSTER_DEPLOY_REVIEW
    // Visual injection only: actual deployment still rejects monsters pending battle art.
    memset(dp.placed,255,sizeof dp.placed);dp.n=3;
    dp.placed[12]=0;dp.placed[17]=3;dp.placed[19]=1;
    dp.unit=3;dp.row=3;dp.col=2;dp.note=dp.confirm=0;stats_unit=255;
#else
    mg.unit=n>=20?(u8)(n-19):n>=10?(u8)(n-9):3;
    mg.top=mg.slide=0;mg.screen=n>=10&&n<20?S_STATUS:S_FORM;
#endif
    dirty=1;
}
#ifndef MONSTER_DEPLOY_REVIEW
void game_scenario(u16 n){source_scenario(0);formation_fixture(n);}
#endif
#ifndef __m68k__
static void boundaries(void)
{
    u16 size,i;const u8 *raw=rt_file("ffma",&size);u8 *bad=malloc(size+8);
    CHECK(bad!=RT_NULL);if(!bad)return;
    CHECK(monster_formation_valid(raw,size));
    memcpy(bad,raw,size);memcpy(bad+size,"\0dat\0\xf8",6);CHECK(monster_formation_valid(bad,(u16)(size+6)));
    for(i=0;i<16;i++){
        memcpy(bad,raw,size);bad[i]^=0x80;CHECK(!monster_formation_valid(bad,size));
    }
    memcpy(bad,raw,size);bad[16]=16;CHECK(!monster_formation_valid(bad,size));
    memcpy(bad,raw,size);bad[MFA_NAMES]=bad[MFA_NAMES+1]=0;CHECK(!monster_formation_valid(bad,size));
    memcpy(bad,raw,size);memset(bad+MFA_POOL,'A',16);CHECK(!monster_formation_valid(bad,size));
    memcpy(bad,raw,size);bad[MFA_FRAMES]=bad[MFA_FRAMES+208]=255;CHECK(!monster_formation_valid(bad,size));
    memcpy(bad,raw,size);bad[size]=bad[size+1]=0;CHECK(!monster_formation_valid(bad,(u16)(size+2)));
    CHECK(!monster_formation_valid(raw,(u16)(size-1)));CHECK(!monster_formation_valid(RT_NULL,0));free(bad);
}
int main(int argc,char **argv)
{
    if(argc!=3)return 2;
    sw_init(
#ifdef MONSTER_DEPLOY_REVIEW
        0
#else
        (u16)(!strcmp(argv[1],"flow")?1:atoi(argv[1]))
#endif
    );
#ifdef MONSTER_DEPLOY_REVIEW
    formation_fixture((u16)(!strcmp(argv[1],"flow")?1:atoi(argv[1])));
#endif
    boundaries();
    {Save before=save;CHECK(sw_step(0));CHECK(!memcmp(&before,&save,sizeof save));}
    if(!strcmp(argv[1],"flow")){
        Save before=save;
#ifdef MONSTER_DEPLOY_REVIEW
        // An adult monster is a fighter now; an egg is still refused.
        Deployment placed=dp;u8 free_cell;
        for(free_cell=0;free_cell<25;free_cell++)
            if(deploy_cell(free_cell)[0]&&dp.placed[free_cell]==255)break;
        CHECK(free_cell<25);
        dp.unit=1;dp.row=(u8)(free_cell/5);dp.col=(u8)(free_cell%5);
        CHECK(deploy_place()&&!dp.note&&dp.placed[free_cell]==1);dp=placed;
        dp.unit=7;CHECK(!deploy_place()&&dp.note==6);dp=placed;
#else
        // Real input must open adult Status, retain job IDs, skip eggs and lock
        // human equipment/job changes. Rendering and denied actions never save.
#define KEY(k) do{CHECK(sw_step(k));CHECK(sw_step(0));}while(0)
        mg.unit=3;dirty=1;KEY(K_ENTER);CHECK(mg.screen==S_STATUS&&unit(mg.unit)->job==133);
        KEY(K_F5);CHECK(mg.screen==S_STATUS&&unit(mg.unit)->job==140);KEY(K_B);
        KEY(K_C);KEY(K_ENTER);CHECK(mg.note==NOTE_LOCKED&&!mg.changed);KEY(K_B);KEY(K_B);
        mg.unit=7;dirty=1;KEY(K_ENTER);CHECK(mg.screen==S_FORM);KEY(K_C);KEY(K_ENTER);
        CHECK(mg.confirm==1);KEY(K_ENTER);CHECK(!mg.confirm);KEY(K_B);
        CHECK(!memcmp(&before,&save,sizeof save));
        save.n=16;save.unit[15]=save.unit[2];mg.unit=15;mg.top=2;mg.slide=0;dirty=1;
        KEY(K_ENTER);CHECK(mg.screen==S_STATUS&&unit(15)->job==95);KEY(K_F5);CHECK(mg.unit==0);
        save=before;
        mg.unit=mg.top=mg.slide=0;mg.screen=S_FORM;dirty=1;
        KEY(K_C);KEY(K_ENTER);CHECK(mg.screen==S_EQUIP);
        KEY(K_F5);CHECK(mg.unit==8&&mg.screen==S_EQUIP);KEY(K_F1);CHECK(mg.unit==0&&mg.screen==S_EQUIP);
        KEY(K_ENTER);CHECK(mg.screen==S_PICK);
        KEY(K_B);KEY(K_B);KEY(K_B);CHECK(mg.screen==S_FORM&&!mg.changed);
#undef KEY
#endif
        CHECK(!memcmp(&before,&save,sizeof save));
    }
    printf("monster-formation %04X checks%u failures%u\n",sw_checksum(),checks,failures);
    if(failures)return 1;
    return sw_write_png(argv[2],1);
}
#else
void _main(void){monster_runtime_main();BENCH_VALUE(checks);BENCH_VALUE(failures);}
#endif
