// Focused service-backend boundaries, independent of campaign Save integration.
#ifdef __m68k__
#define USE_TI89
#include <tigcclib.h>
#include "../../../tools/m68kbench/bench.h"
#else
#include <stdio.h>
#endif
#include "save_types.h"
#include "../base.inc"
#include "breeding.h"
#include "proposition_rules.h"
#include "fixture.h"
#ifndef __m68k__
#include "original_adapter.h"
#endif
static PropositionJobs jobs,before;
static u8 corrupt[sizeof rules+7];
static u16 checks,failures;
static u32 digest=17,gil,prior;
static void check(u32 actual,u32 expected,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^actual;
    if(actual!=expected){failures++;
#ifdef __m68k__
        BENCH_VALUE(line);
#else
        printf("Propositions line%u: %lu expected%lu\n",line,(unsigned long)actual,(unsigned long)expected);
#endif
    }
}
#define CHECK(a,b) check((u32)(a),(u32)(b),__LINE__)
static void unchanged(void){CHECK(memcmp(&jobs,&before,sizeof jobs),0);CHECK(gil,prior);}
static PropositionContext context={1,6,5,1};
static u8 team[3]={1,2,3};
static void seed(void){memset(&jobs,0,sizeof jobs);gil=2000;context.unlocked=1;context.location=6;context.tier=5;context.zodiac=1;}
static void reject(u8 expected,u8 id,u8 n,u8 days,u16 eligible)
{before=jobs;prior=gil;CHECK(prop_dispatch(rules,&jobs,id,&context,team,n,days,16,eligible,&gil),expected);unchanged();}
static void bounds(void)
{
    u16 i;u8 k;
    CHECK(sizeof(PropositionJob),9);CHECK(sizeof(PropositionJobs),217);CHECK(sizeof(Save),4684);
    CHECK(prop_bank_valid(rules,sizeof rules),1);CHECK(prop_bank_valid(RT_NULL,0),0);
    memcpy(corrupt+1,rules,sizeof rules);CHECK(prop_bank_valid(corrupt+1,sizeof rules),1);
    memcpy(corrupt,rules,sizeof rules);memcpy(corrupt+sizeof rules,"\0dat\0\xf8",6);
    CHECK(prop_bank_valid(corrupt,sizeof rules+6),1);CHECK(prop_bank_valid(rules,sizeof rules-1),0);
    for(k=0;k<8;k++) {
        static const u16 offsets[]={0,4,6,7,8,30,34,51};
        memcpy(corrupt,rules,sizeof rules);corrupt[offsets[k]]=255;
        CHECK(prop_bank_valid(corrupt,sizeof rules),0);
    }
    CHECK(memcmp(prop_title(rules,30),"Testimony of Ex-miner",sizeof "Testimony of Ex-miner"),0);CHECK(prop_fee(rules,30),600);
    seed();CHECK(prop_jobs_valid(&jobs,rules,16),1);
    context.unlocked=0;reject(PROP_LOCKED,30,2,8,65535);context.unlocked=1;
    reject(PROP_UNAVAILABLE,96,2,8,65535);
    context.location=5;reject(PROP_UNAVAILABLE,30,2,8,65535);context.location=6;
    context.tier=4;reject(PROP_UNAVAILABLE,30,2,8,65535);context.tier=5;
    reject(PROP_TEAM,30,0,8,65535);reject(PROP_TEAM,30,4,8,65535);
    reject(PROP_DAYS,30,2,7,65535);reject(PROP_DAYS,30,2,12,65535);
    team[1]=1;reject(PROP_TEAM,30,2,8,65535);team[1]=16;reject(PROP_TEAM,30,2,8,65535);team[1]=2;
    reject(PROP_TEAM,30,2,8,2);gil=599;reject(PROP_GIL,30,2,8,65535);gil=600;
    CHECK(prop_dispatch(rules,&jobs,30,&context,team,2,8,16,65535,&gil),PROP_OK);
    CHECK(gil,0);CHECK(jobs.n,1);CHECK(jobs.job[0].unit[2],255);CHECK(prop_status(&jobs,30),10);
    CHECK(prop_jobs_valid(&jobs,rules,16),1);CHECK(prop_away(&jobs,1),1);CHECK(prop_away(&jobs,0),0);
    reject(PROP_UNAVAILABLE,30,2,8,65535); // duplicate input cannot pay twice
    for(k=0;k<PROP_N;k++) {
        const u8 *r=prop_row(rules,k);context.location=r[16]?(u8)(r[16]-1):0;
        context.tier=255;context.zodiac=(r[17]&2)?r[22]:1;
        if(prop_available(rules,&jobs,k,&context))break;
    }
    CHECK(k<PROP_N,1);reject(PROP_TEAM,k,1,prop_row(rules,k)[6],65535);
    context.location=6;context.tier=5;context.zodiac=1;
    CHECK(prop_report_ready(&jobs,0,6),0);
    for(i=0;i<7;i++){prop_day(&jobs);CHECK(jobs.job[0].elapsed,i+1);CHECK(prop_report_ready(&jobs,0,6),0);}
    prop_day(&jobs);CHECK(prop_report_ready(&jobs,0,6),1);CHECK(prop_report_ready(&jobs,0,5),0);
    CHECK(prop_away(&jobs,1),1);CHECK(prop_recall(&jobs,0),0);CHECK(prop_jobs_valid(&jobs,rules,16),1);
    before=jobs;for(i=0;i<255;i++)prop_day(&jobs);CHECK(memcmp(&jobs,&before,sizeof jobs),0);
    // Slot compaction cannot remove an absent participant or keep stale indices.
    CHECK(prop_remove_slot(&jobs,1),0);CHECK(memcmp(&jobs,&before,sizeof jobs),0);
    CHECK(prop_remove_slot(&jobs,0),1);CHECK(jobs.job[0].unit[0],0);CHECK(jobs.job[0].unit[1],1);
    CHECK(prop_jobs_valid(&jobs,rules,15),1);
    seed();CHECK(prop_dispatch(rules,&jobs,30,&context,team,1,11,16,65535,&gil),PROP_OK);
    CHECK(prop_recall(&jobs,0),1);CHECK(prop_report_ready(&jobs,0,6),0);
    prop_day(&jobs);CHECK(jobs.job[0].elapsed,1);CHECK(prop_report_ready(&jobs,0,6),1);
    CHECK(prop_away(&jobs,1),1);CHECK(prop_jobs_valid(&jobs,rules,16),1);
    // Cold-load validation rejects inconsistent active flags and overlapping workers.
    before=jobs;jobs.job[0].n=4;CHECK(prop_jobs_valid(&jobs,rules,16),0);jobs=before;
    jobs.status[15]=0;CHECK(prop_jobs_valid(&jobs,rules,16),0);jobs=before;
    jobs.job[0].flags=PROP_SENT|PROP_RETURNED;CHECK(prop_jobs_valid(&jobs,rules,16),0);jobs=before;
    jobs.retry[30]=65;CHECK(prop_jobs_valid(&jobs,rules,16),0);jobs=before;
    jobs.n=9;CHECK(prop_jobs_valid(&jobs,rules,16),0);jobs=before;
    // Nibble neighbors, completed prerequisites and retry severity survive days.
    seed();prop_set_status(&jobs,30,12);prop_set_status(&jobs,31,8);
    CHECK(prop_status(&jobs,30),12);CHECK(prop_status(&jobs,31),8);
    CHECK(prop_available(rules,&jobs,30,&context),0);CHECK(prop_jobs_valid(&jobs,rules,16),1);
    jobs.retry[31]=0x60;jobs.retry[32]=0x90;prop_set_status(&jobs,32,8);
    for(i=0;i<32;i++)prop_day(&jobs);
    CHECK(jobs.retry[31],0x40);CHECK(jobs.retry[32],0x80);CHECK(prop_jobs_valid(&jobs,rules,16),1);
    // Fill all eight job slots with disjoint pairs, using actual valid rows.
    seed();gil=200000;context.tier=255;
    for(k=0;k<8;k++) {
        u8 id;
        for(id=0;id<PROP_N;id++) {
            const u8 *r=prop_row(rules,id);context.location=r[16]?(u8)(r[16]-1):0;
            context.zodiac=(r[17]&2)?r[22]:1;
            if(prop_available(rules,&jobs,id,&context))break;
        }
        CHECK(id<PROP_N,1);team[0]=(u8)(2*k);team[1]=(u8)(2*k+1);
        CHECK(prop_dispatch(rules,&jobs,id,&context,team,2,prop_row(rules,id)[6],16,65535,&gil),PROP_OK);
    }
    CHECK(jobs.n,8);CHECK(prop_jobs_valid(&jobs,rules,16),1);
    for(k=0;k<16;k++)CHECK(prop_away(&jobs,k),1);
    for(k=0;k<PROP_N;k++) {
        const u8 *r=prop_row(rules,k);context.location=r[16]?(u8)(r[16]-1):0;
        context.zodiac=(r[17]&2)?r[22]:1;
        if(prop_available(rules,&jobs,k,&context))break;
    }
    reject(PROP_FULL,k,1,prop_row(rules,k)[6],65535);
}
#ifndef __m68k__
static void original_comparison(void)
{
    u8 id,k,location;u16 decisions=0;PropositionJobs s={0};PropositionContext c={1,0,0,1};
    for(id=0;id<PROP_N;id++) {
        const u8 *r=prop_row(rules,id);
        for(k=0;k<6;k++) {
            c.location=r[16]?(u8)(r[16]-1):0;c.tier=r[21];c.zodiac=(r[17]&2)?r[22]:1;
            memset(&s,0,sizeof s);
            if(k==0)prop_set_status(&s,r[22]<PROP_N?r[22]:0,12);
            if(k==1&&c.tier)c.tier--;
            if(k==2)c.zodiac=c.zodiac==12?1:(u8)(c.zodiac+1);
            if(k==3)c.location=(u8)((c.location+1)%43);
            if(k==4){s.retry[id]=0x41;prop_set_status(&s,id,8);}
            if(k==5)prop_set_status(&s,id,12);
            CHECK(prop_available(rules,&s,id,&c),original_available(rules,&s,id,&c));decisions++;
        }
        // Every actual town ID at a baseline date; catches source1/native0 errors.
        for(location=0;location<43;location++) {
            c.location=location;c.tier=255;c.zodiac=1;memset(&s,0,sizeof s);
            CHECK(prop_available(rules,&s,id,&c),original_available(rules,&s,id,&c));decisions++;
        }
        CHECK(prop_fee(rules,id),original_fee(rules,id));
    }
    // Use existing calendar sign owner; compare original conversion at every
    // sign's before/on/after boundary, including January/December wrap.
    for(k=0;k<12;k++) {
        u8 day;
        for(day=(u8)(source_zodiac[2*k+1]-1);day<=source_zodiac[2*k+1]+1;day++)
            CHECK(wb_zodiac(breed_rules,source_zodiac[2*k],day)+1,
                  original_zodiac(source_zodiac[2*k],day));
    }
    // Original daily counters, including recall and the two retry severities.
    seed();CHECK(prop_dispatch(rules,&jobs,30,&context,team,1,8,16,65535,&gil),PROP_OK);
    jobs.retry[31]=0x60;jobs.retry[32]=0x90;
    prop_set_status(&jobs,31,8);prop_set_status(&jobs,32,8);before=jobs;
    original_load(&jobs);for(k=0;k<9;k++){prop_day(&jobs);original_day();CHECK(original_equal(&jobs),1);}
    jobs=before;prop_recall(&jobs,0);original_load(&jobs);
    for(k=0;k<3;k++){prop_day(&jobs);original_day();CHECK(original_equal(&jobs),1);}
    printf("Original rules: %u availability decisions,96 fees,36 sign boundaries,12 daily comparisons\n",decisions);
}
static void persistence(const char *path)
{
    FILE *f;PropositionJobs restored;Save campaign={0},readback;
    seed();team[0]=1;team[1]=2;
    CHECK(prop_dispatch(rules,&jobs,30,&context,team,2,8,16,65535,&gil),PROP_OK);
    prop_day(&jobs);campaign.n=5;memcpy(campaign.magic,SAVE_MAGIC,4);
    campaign.money[2]=(u8)(gil>>8);campaign.money[3]=(u8)gil;
    campaign.profiles[4]=0x11;campaign.world_rng[3]=71;
    f=fopen(path,"wb");CHECK(f!=RT_NULL,1);if(!f)return;
    CHECK(fwrite(&campaign,1,sizeof campaign,f),sizeof campaign);
    CHECK(fwrite(&jobs,1,sizeof jobs,f),sizeof jobs);CHECK(fclose(f),0);
    memset(&readback,0,sizeof readback);memset(&restored,0,sizeof restored);
    f=fopen(path,"rb");CHECK(f!=RT_NULL,1);if(!f)return;
    CHECK(fread(&readback,1,sizeof readback,f),sizeof readback);
    CHECK(fread(&restored,1,sizeof restored,f),sizeof restored);CHECK(fgetc(f)==EOF,1);CHECK(fclose(f),0);
    CHECK(memcmp(&campaign,&readback,sizeof campaign),0);CHECK(memcmp(&jobs,&restored,sizeof jobs),0);
    CHECK(prop_jobs_valid(&restored,rules,16),1);CHECK(prop_away(&restored,2),1);
    for(gil=0;gil<7;gil++)prop_day(&restored);
    CHECK(prop_report_ready(&restored,0,6),1);CHECK(prop_away(&restored,2),1);
}
int main(int argc,char **argv)
{
    bounds();printf("bounds %u %u %08lX %u\n",checks,failures,(unsigned long)digest,(unsigned)sizeof jobs);
    team[0]=1;team[1]=2;context.location=6;context.tier=5;
    original_comparison();if(argc==2)persistence(argv[1]);else failures++;
    printf("Propositions backend: %u checks, %u failures, digest %08lX; state%u Save%u\n",
           checks,failures,(unsigned long)digest,(unsigned)sizeof jobs,(unsigned)sizeof(Save));
    return failures?1:0;
}
#else
void _main(void){bounds();BENCH_VALUE(checks);BENCH_VALUE(failures);BENCH_VALUE(digest);BENCH_VALUE(sizeof jobs);}
#endif
