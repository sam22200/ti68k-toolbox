// Pure data/effect boundaries: no scene UI, hardware, new Save or overlay ABI.
#ifdef __m68k__
#define USE_TI89
#include <tigcclib.h>
#include "../../../tools/m68kbench/bench.h"
#else
#include <stdio.h>
#endif
#include "story_choice.h"
#include "x/story-choices/fixture.h"
static Save working,before,file_save;
static u8 scratch[512],load_fail,store_fail,deferred;
static const Save *pending;
static u16 checks,failures,loads,stores;static u32 digest=17;
static void check(u32 actual,u32 expected,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^actual;
    if(actual!=expected){failures++;
#ifdef __m68k__
        BENCH_VALUE(line);
#else
        printf("story-choice line%u: %lu expected%lu\n",line,(unsigned long)actual,(unsigned long)expected);
#endif
    }
}
#define CHECK(a,b) check((u32)(a),(u32)(b),__LINE__)
static void seed(u8 n)
{
    u8 i;memset(&working,0,sizeof working);memcpy(working.magic,SAVE_MAGIC,4);working.n=n;
    for(i=0;i<PARTY_MAX;i++){working.unit[i].name=(u8)(i+30);working.unit[i].level=(u8)(i+1);}
    for(i=0;i<3;i++){
        working.guest_id[i][1]=i?(u8)(10+i):4;working.guest[i]=working.unit[i];
        working.guest[i].exp=(u8)(70+i);working.guest[i].jp[2][0]=3;working.guest[i].jp[2][1]=99;
    }
    save_raw_set(&working,16,1);sc_flag_set(&working,3,1);sc_flag_set(&working,70,1);
    before=file_save=working;pending=RT_NULL;loads=stores=load_fail=store_fail=deferred=0;
}
static u8 load_choice(Save *out)
{
    loads++;if(load_fail)return 0;
    if(pending){if(out!=pending)memcpy(out,pending,sizeof *out);}
    else memcpy(out,&file_save,sizeof *out);
    return 1;
}
static u8 store_choice(const Save *in)
{
    stores++;if(store_fail)return 0;
    if(deferred)pending=in;else memcpy(&file_save,in,sizeof file_save);
    return 1;
}
static void unchanged(void){CHECK(memcmp(&working,&before,sizeof working)==0,1);}
static void trials(void)
{
    u8 i;u16 at;
    CHECK(story_choice_valid(recruit_choice,sizeof recruit_choice),1);
    CHECK(story_choice_valid(multi_choice,sizeof multi_choice),1);
    seed(15);CHECK(story_choice_result(&working,recruit_choice),0);
    CHECK(story_choice_apply(&working,recruit_choice,sizeof recruit_choice,1),SC_APPLIED);
    CHECK(working.n,16);CHECK(save_guest_find(&working,4),255);
    CHECK(memcmp(&working.unit[15],&before.guest[0],sizeof(SaveUnit)),0);
    CHECK(save_raw_current(&working,15),1);CHECK(save_raw_current(&working,16),0);
    CHECK(sc_flag(&working,40)&&sc_flag(&working,42)&&!sc_flag(&working,41),1);
    CHECK(sc_flag(&working,71)&&!sc_flag(&working,70),1);
    CHECK(memcmp(working.unit,before.unit,15*sizeof(SaveUnit)),0);
    CHECK(memcmp(working.guest+1,before.guest+1,2*sizeof(SaveUnit)),0);
    before=working;sc_flag_set(&working,3,0);before=working;
    CHECK(story_choice_apply(&working,recruit_choice,sizeof recruit_choice,0),SC_REPLAY);unchanged();
    CHECK(story_choice_result(&working,recruit_choice),2); // guard/answer changes cannot rewrite history
    seed(16);CHECK(story_choice_apply(&working,recruit_choice,sizeof recruit_choice,1),SC_PARTY_FULL);unchanged();
    CHECK(story_choice_result(&working,recruit_choice),0);
    CHECK(story_choice_apply(&working,recruit_choice,sizeof recruit_choice,0),SC_APPLIED);
    CHECK(working.n,16);CHECK(save_guest_find(&working,4),255);CHECK(story_choice_result(&working,recruit_choice),1);
    CHECK(sc_flag(&working,71),0);CHECK(memcmp(working.unit,before.unit,sizeof working.unit),0);
    seed(15);sc_flag_set(&working,3,0);before=working;
    CHECK(story_choice_apply(&working,recruit_choice,sizeof recruit_choice,1),SC_GATED);unchanged();
    seed(14);CHECK(story_choice_apply(&working,multi_choice,sizeof multi_choice,1),SC_APPLIED);
    CHECK(working.n,16);CHECK(sc_flag(&working,256),1);CHECK(sc_flag(&working,70),0);
    CHECK(working.unit[14].exp,70);CHECK(working.unit[15].exp,72);
    for(i=0;i<3;i++)CHECK(save_guest_id(&working,i),0);
    seed(15);CHECK(story_choice_apply(&working,multi_choice,sizeof multi_choice,1),SC_PARTY_FULL);unchanged();
    memcpy(scratch,multi_choice,sizeof multi_choice);at=sc_word(sc_option(scratch,1)+4);
    scratch[at+4*3+2]=scratch[at+4*3+3]=255; // missing guest after earlier flag/recruit/remove effects
    CHECK(story_choice_apply(&working,scratch,sizeof multi_choice,1),SC_MISSING_GUEST);unchanged();
    // Corruption/contradictory markers are not a refusal or a successful skip.
    for(i=0;i<2;i++){
        seed(15);sc_flag_set(&working,i?40:42,1);before=working;
        CHECK(story_choice_apply(&working,recruit_choice,sizeof recruit_choice,0),SC_INVALID);unchanged();
    }
    seed(15);working.guest_id[1][1]=4;before=working;
    CHECK(story_choice_apply(&working,recruit_choice,sizeof recruit_choice,1),SC_INVALID);unchanged();
    seed(15);CHECK(story_choice_apply(&working,recruit_choice,sizeof recruit_choice,2),SC_INVALID);unchanged();
    CHECK(story_choice_valid(recruit_choice,(u16)(sizeof recruit_choice-1)),0);
    memcpy(scratch+1,recruit_choice,sizeof recruit_choice);
    CHECK(story_choice_valid(scratch+1,sizeof recruit_choice),1); // byte-safe even on68000
    for(i=0;i<6;i++){
        static const u8 positions[]={0,11,19,14,17,6};
        static const u8 values[]={0,3,1,1,255,255};
        memcpy(scratch,recruit_choice,sizeof recruit_choice);scratch[positions[i]]=values[i];
        CHECK(story_choice_valid(scratch,sizeof recruit_choice),0);
    }
    // Whole-record commit, reload, retry and opposite-answer replay.
    seed(15);store_fail=1;
    CHECK(story_choice_commit(&working,recruit_choice,sizeof recruit_choice,1,load_choice,store_choice),SC_IO);
    CHECK(stores,1);unchanged();CHECK(memcmp(&file_save,&before,sizeof before),0);
    store_fail=0;
    CHECK(story_choice_commit(&working,recruit_choice,sizeof recruit_choice,1,load_choice,store_choice),SC_APPLIED);
    CHECK(file_save.n,16);CHECK(stores,2);before=file_save;
    CHECK(story_choice_commit(&working,recruit_choice,sizeof recruit_choice,0,load_choice,store_choice),SC_REPLAY);
    CHECK(stores,2);unchanged();load_fail=1;
    CHECK(story_choice_commit(&working,recruit_choice,sizeof recruit_choice,0,load_choice,store_choice),SC_IO);
    unchanged();CHECK(stores,2);
    // Native-like deferred persistence: retain the caller's durable Save and
    // read its accepted changes before a second choice, without a VAT reread.
    seed(14);deferred=1;
    CHECK(story_choice_commit(&working,recruit_choice,sizeof recruit_choice,1,load_choice,store_choice),SC_APPLIED);
    CHECK(pending==&working,1);CHECK(file_save.n,14);
    CHECK(story_choice_commit(&working,multi_choice,sizeof multi_choice,0,load_choice,store_choice),SC_APPLIED);
    CHECK(stores,2);CHECK(sc_flag(pending,40)&&sc_flag(pending,100),1);CHECK(pending->n,15);
    memcpy(&file_save,pending,sizeof file_save);pending=RT_NULL;memset(&working,0,sizeof working);
    CHECK(load_choice(&working),1);CHECK(story_choice_result(&working,recruit_choice),2);
    CHECK(story_choice_result(&working,multi_choice),1);CHECK(save_guest_find(&working,4),255);
    CHECK(working.unit[14].exp,70);
    for(at=0;at<sizeof working;at++)digest=(digest<<1|digest>>31)^((const u8 *)&working)[at];
}
#ifndef __m68k__
int main(void){trials();printf("story-choice %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
