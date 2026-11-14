// Allocation-free supervisor compatibility and minimum-length boundaries.
#ifdef __m68k__
#define USE_TI89
#include <tigcclib.h>
#include "../../../tools/m68kbench/bench.h"
#else
#include <stdio.h>
#endif
#include "pack_story.h"
#include "save_progress.h"
static u8 bytes[sizeof(Save)+6];static u16 checks,failures;static u32 digest=17;
static void check(u8 ok,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^ok;
    if(!ok){failures++;
#ifdef __m68k__
        BENCH_VALUE(line);
#else
        printf("progress failure line%u\n",line);
#endif
    }
}
#define CHECK(x) check(!!(x),__LINE__)
static u8 core[38],packs[2][74],active,calls;
static void put(u8 *p,u16 offset,u16 value){p[offset]=(u8)(value>>8);p[offset+1]=(u8)value;}
static const u8 *read_fixture(const char *name,u16 *size)
{
    static const u8 byte=42;
    u16 n=(u16)strlen(name);
    if(n==6&&!memcmp(name,"ffcore",6)){*size=sizeof core;return core;}
    if(n==6&&!memcmp(name,"ffpack",6)){*size=sizeof packs[0];return packs[active];}
    if(n==7&&!memcmp(name,"fixture",7)){*size=1;return &byte;}return RT_NULL;
}
static void progress_fixture(FpProgress *p){memset(p,0,sizeof *p);}
static void reset_fixture(void){CHECK(0);}
static u16 call_fixture(u8 op,u16 arg)
{
    u8 old=active;calls++;
    CHECK(calls==1?(op==FP_BATTLE&&arg==200):(calls==2&&op==FP_TITLE&&arg==3));
    active^=1;memcpy(packs[active],packs[old],sizeof packs[0]);
    memset(packs[old],0,sizeof packs[0]); // old archive view is invalid after every child
    return 1;
}
static void relocation(void)
{
    u8 *p=packs[0];FpNotice notice;
    memset(core,0,sizeof core);memset(packs,0,sizeof packs);active=calls=0;
    memcpy(core,"FKR1",4);put(core,4,sizeof core);put(core,6,1);
    memcpy(core+24,"fixture",7);put(core,32,1);put(core,34,0x2a2a);put(core,36,fp_check(core,36));
    memcpy(p,"FKP1",4);put(p,4,74);put(p,6,1);put(p,28,FP_NONE);
    put(p,30,1);put(p,32,1);put(p,34,1);put(p,36,48);put(p,38,52);put(p,40,60);put(p,44,FP_NONE);
    p[48]=2;p[49]='g';p[52]=FP_BATTLE;p[53]=FP_SAVE_PROMPT;put(p,54,200);
    memcpy(p+60,core+24,12);put(p,72,fp_check(p,72));
    CHECK(fp_story(read_fixture,call_fixture,progress_fixture,reset_fixture,2,&notice)==FP_OK);
    CHECK(calls==2); // the Save proposal survives both archive relocations
}
static void trials(void)
{
    static const u16 sizes[]={SAVE_V2_BYTES,SAVE_V3_BYTES,SAVE_V4_BYTES,SAVE_V5_BYTES,sizeof(Save)};
    u8 version,i;FpProgress p;
    for(version=0;version<5;version++){
        u16 n=sizes[version];memset(bytes,0,sizeof bytes);
        memcpy(bytes,"FSV2",4);bytes[3]+=version;bytes[36]=16;
        bytes[4]=0x82;bytes[5]=1;bytes[35]=0x80;bytes[SAVE_FLOW_OFFSET]=4;
        CHECK(save_progress_read(bytes,n,&p));
        CHECK(p.flags[0]==0x82&&p.flags[1]==1&&p.flags[31]==0x80&&p.flow==4);
        CHECK(save_progress_read(bytes,(u16)(n+6),&p));
        CHECK(!save_progress_read(bytes,(u16)(n-1),&p));
        CHECK(!p.flow);for(i=0;i<32;i++)if(p.flags[i])break;CHECK(i==32);
        bytes[36]=17;CHECK(!save_progress_read(bytes,n,&p));bytes[36]=16;
    }
    CHECK(!save_progress_read(RT_NULL,0,&p));CHECK(!p.flow&&!p.flags[0]);
    CHECK(!save_progress_read(bytes,3,&p));
    bytes[0]='X';CHECK(!save_progress_read(bytes,sizeof bytes,&p));
    memcpy(bytes,"FSV2",4);bytes[4]=1;CHECK(save_progress_read(bytes,SAVE_V2_BYTES,&p));CHECK(!p.flow);
    relocation();
}
#ifndef __m68k__
int main(void){trials();printf("save-progress %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
