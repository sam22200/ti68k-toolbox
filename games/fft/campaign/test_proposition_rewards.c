// Reward arithmetic, owned frozen receipts and one-shot settlement.
#ifdef __m68k__
#define USE_TI89
#include <tigcclib.h>
#include "../../../tools/m68kbench/bench.h"
#else
#include <stdio.h>
#endif
#include "save_types.h"
#include "../base.inc"
#include "proposition_transactions.h"
#include "fixture.h"
#ifndef __m68k__
#include "original_adapter.h"
#endif
static PropositionState state,prior_state;
static Save save,prior_save;
static u16 checks,failures,draw_count,draw_value;
static u8 store_failure,store_calls;
static u32 digest=19;
static void check(u32 a,u32 b,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^a;
    if(a!=b){failures++;
#ifdef __m68k__
        BENCH_VALUE(line);
#else
        printf("Reward line%u: %lu expected%lu\n",line,(unsigned long)a,(unsigned long)b);
#endif
    }
}
#define CHECK(a,b) check((u32)(a),(u32)(b),__LINE__)
static u16 draw(void *p){(void)p;draw_count++;return draw_value;}
static u16 world_draw(void *p)
{
    Save *s=(Save *)p;u32 n=prop_long(s->world_rng);
    n^=n<<13;n^=n>>17;n^=n<<5;prop_put_long(s->world_rng,n);draw_count++;
    return (u16)(n>>17);
}
static u8 store(const Save *s,const PropositionState *p,void *ctx)
{(void)s;(void)p;(void)ctx;store_calls++;return !store_failure;}
static void seed(u8 id,u8 count,u8 stat,u8 last)
{
    const u8 *r=prop_row(rules,id);u8 i;u32 gil=50000;u8 team[3]={0,1,2};
    PropositionContext c={1,0,255,1};
    memset(&state,0,sizeof state);memset(&save,0,sizeof save);
    memcpy(save.magic,SAVE_MAGIC,4);save.n=5;save.world.month=3;save.world.day=21;
    save.world_rng[3]=71;save.profiles[4]=0x11;
    for(i=0;i<save.n;i++) {
        save.unit[i].type=i&1;save.unit[i].job=(u8)(74+(id+i)%20);
        save.unit[i].level=stat?99:1;save.unit[i].brave=save.unit[i].faith=stat?100:0;
    }
    c.location=r[16]?(u8)(r[16]-1):0;c.zodiac=(r[17]&2)?r[22]:1;
    if(r[17]&4){prop_set_status(&state.dispatches,r[22],12);prop_set_date(state.completed_date,r[22],(3<<5)|20);}
    CHECK(prop_dispatch(rules,&state.dispatches,id,&c,team,count,last?r[7]:r[6],save.n,65535,&gil),PROP_OK);
    prop_put_long(save.money,gil);for(i=0;i<state.dispatches.job[0].days;i++)prop_day(&state.dispatches);
    draw_count=0;draw_value=32767;
}
static void unchanged(void)
{CHECK(memcmp(&state,&prior_state,sizeof state),0);CHECK(memcmp(&save,&prior_save,sizeof save),0);}
static u8 find_kind(u8 kind)
{u8 i;for(i=0;i<96;i++)if(prop_row(rules,i)[10]==kind)return i;return 255;}
static void bounds(void)
{
    u8 id,k;u16 n;static u8 scratch[sizeof rewards+7];
    CHECK(sizeof(PropositionReport),24);CHECK(sizeof(PropositionState),402);
    CHECK(sizeof(PropositionUndo),119);
    CHECK(prop_bank_valid(rules,sizeof rules),1);CHECK(prop_rewards_valid(rewards,sizeof rewards),1);
    CHECK(prop_reward_rows_valid(rules,rewards),1);CHECK(prop_rewards_valid(RT_NULL,0),0);
    memcpy(scratch+1,rewards,sizeof rewards);CHECK(prop_rewards_valid(scratch+1,sizeof rewards),1);
    memcpy(scratch,rewards,sizeof rewards);memcpy(scratch+sizeof rewards,"\0dat\0\xf8",6);
    CHECK(prop_rewards_valid(scratch,sizeof rewards+6),1);CHECK(prop_rewards_valid(rewards,sizeof rewards-1),0);
    for(k=0;k<5;k++) {
        static const u16 offsets[]={0,4,6,8,12};
        memcpy(scratch,rewards,sizeof rewards);scratch[offsets[k]]=255;
        CHECK(prop_rewards_valid(scratch,sizeof rewards),0);
    }
    for(k=0;k<9;k++) {
        static const u16 values[]={0,1,99,100,101,327,9999,20000,32767};n=values[k];
        CHECK(prop_hundred(n),n/100U);
        CHECK(prop_scale(n,150),(u32)n*150U/100U);CHECK(prop_scale(n,5),(u32)n*5U/100U);
    }
    // Recall gives no JP/gil/bonus, consumes no random draw, and still releases
    // only when its origin-town report is collected.
    seed(30,2,1,0);state.dispatches.job[0].flags|=PROP_RECALLED;
    CHECK(prop_prepare(rules,rewards,&state,&save,30,6,draw,RT_NULL),PROP_OK);
    CHECK(draw_count,0);CHECK(state.report.result,PROP_BAD_FAILURE);CHECK(prop_long(state.report.gil),0);
    CHECK(prop_word(state.report.jp[0]),0);CHECK(prop_state_valid(&state,rules,save.n),1);
    CHECK(prop_collect(&state,rules,&save,30,6),PROP_OK);CHECK(state.dispatches.retry[30],0x90);
    // Full successful result is frozen across repeated open and consumed once.
    seed(30,3,1,1);draw_value=0;
    CHECK(prop_prepare(rules,rewards,&state,&save,30,6,draw,RT_NULL),PROP_OK);
    CHECK(state.report.result,PROP_SUCCESS);CHECK(prop_state_valid(&state,rules,save.n),1);
    prior_state=state;prior_save=save;n=draw_count;
    CHECK(prop_prepare(rules,rewards,&state,&save,30,6,draw,RT_NULL),PROP_OK);unchanged();CHECK(draw_count,n);
    CHECK(prop_prepare(rules,rewards,&state,&save,30,5,draw,RT_NULL),PROP_UNAVAILABLE);unchanged();
    CHECK(prop_collect(&state,rules,&save,30,5),PROP_UNAVAILABLE);unchanged();
    prop_put_word(save.unit[0].jp[state.report.career[0]],9990);
    prop_put_word(save.unit[0].tjp[state.report.career[0]],9998);prop_put_long(save.money,99999998UL);
    k=state.report.career[0];CHECK(prop_collect(&state,rules,&save,30,6),PROP_OK);
    CHECK(prop_long(save.money),99999999UL);CHECK(prop_word(save.unit[0].jp[k]),9999);
    CHECK(prop_word(save.unit[0].tjp[k]),9999);CHECK(state.dispatches.n,0);CHECK(prop_status(&state.dispatches,30),12);
    CHECK(prop_date(state.completed_date,30),(3<<5)|21);CHECK(prop_away(&state.dispatches,0),0);
    CHECK(prop_state_valid(&state,rules,save.n),1);prior_state=state;prior_save=save;
    CHECK(prop_collect(&state,rules,&save,30,6),PROP_UNAVAILABLE);unchanged();
    // Two returned jobs retain their own identities when one is consumed.
    seed(30,3,1,0);
    {
        PropositionContext c={1,0,255,1};u8 team[2]={3,4},other;u32 gil=prop_long(save.money);
        for(other=0;other<96;other++) {
            const u8 *r=prop_row(rules,other);c.location=r[16]?(u8)(r[16]-1):0;c.zodiac=(r[17]&2)?r[22]:1;
            if(prop_available(rules,&state.dispatches,other,&c))break;
        }
        CHECK(other<96,1);
        CHECK(prop_dispatch(rules,&state.dispatches,other,&c,team,2,prop_row(rules,other)[6],save.n,65535,&gil),PROP_OK);
        prop_put_long(save.money,gil);for(k=0;k<32;k++)prop_day(&state.dispatches);
        CHECK(prop_prepare(rules,rewards,&state,&save,other,c.location,draw,RT_NULL),PROP_OK);
        prior_state=state;prior_save=save;
        CHECK(prop_prepare(rules,rewards,&state,&save,30,6,draw,RT_NULL),PROP_UNAVAILABLE);unchanged();
        CHECK(prop_collect(&state,rules,&save,30,6),PROP_UNAVAILABLE);unchanged();
        CHECK(prop_state_remove_slot(&state,4),0);unchanged();
        CHECK(prop_collect(&state,rules,&save,other,c.location),PROP_OK);
        CHECK(state.dispatches.n,1);CHECK(state.dispatches.job[0].id,30);CHECK(prop_away(&state.dispatches,0),1);
        CHECK(prop_away(&state.dispatches,4),0);CHECK(prop_state_valid(&state,rules,save.n),1);
        CHECK(prop_prepare(rules,rewards,&state,&save,30,6,draw,RT_NULL),PROP_OK);
        CHECK(prop_collect(&state,rules,&save,30,6),PROP_OK);CHECK(state.dispatches.n,0);
    }
    // Discover the only unowned treasure/land; once full, fall back to any
    // original index rather than overflowing an empty candidate list.
    for(k=1;k<=2;k++) {
        u8 i,count=k==1?31:16;u8 *dates;
        id=find_kind(k);CHECK(id<96,1);seed(id,3,1,1);dates=k==1?state.treasure_date:state.land_date;
        for(i=0;i<count-1;i++)prop_set_date(dates,i,(2<<5)|20);
        CHECK(prop_prepare(rules,rewards,&state,&save,id,state.dispatches.job[0].location,draw,RT_NULL),PROP_OK);
        CHECK(state.report.kind,k);CHECK(state.report.index,count-1);
        CHECK(prop_collect(&state,rules,&save,id,state.report.location),PROP_OK);
        CHECK(prop_date(dates,count-1),(3<<5)|21);CHECK(prop_state_valid(&state,rules,save.n),1);
        // A different undispatched row can still discover a duplicate after
        // the collection is full; exercise the source fallback directly.
        draw_count=0;draw_value=32767;
        CHECK(prop_discovery(dates,count,draw,RT_NULL),count-1);CHECK(draw_count,1);
    }
    // Every packed date neighbor, including final crossing-byte records.
    memset(&state,0,sizeof state);
    for(id=0;id<96;id++)prop_set_date(state.completed_date,id,(u16)(((id%12+1)<<5)|1));
    for(id=0;id<96;id++)CHECK(prop_date(state.completed_date,id),((id%12+1)<<5)|1);
    // Invalid receipt/collection date are atomic failures.
    seed(30,3,1,0);CHECK(prop_prepare(rules,rewards,&state,&save,30,6,draw,RT_NULL),PROP_OK);
    state.report.career[0]=20;prior_state=state;prior_save=save;
    CHECK(prop_collect(&state,rules,&save,30,6),PROP_CORRUPT);unchanged();
    state.report=prior_state.report;state.report.career[0]=0;save.world.month=2;save.world.day=30;
    prior_state=state;prior_save=save;CHECK(prop_collect(&state,rules,&save,30,6),PROP_CORRUPT);unchanged();
    // A failed atomic save restores RNG/preparation and every changed
    // settlement field. Retry persists the same receipt once, with no reroll.
    seed(30,3,1,1);store_failure=1;store_calls=0;prior_state=state;prior_save=save;
    CHECK(prop_prepare_saved(rules,rewards,&state,&save,30,6,world_draw,&save,store,RT_NULL),PROP_STORAGE);
    CHECK(store_calls,1);unchanged();store_failure=0;
    CHECK(prop_prepare_saved(rules,rewards,&state,&save,30,6,world_draw,&save,store,RT_NULL),PROP_OK);
    prior_state=state;prior_save=save;n=draw_count;k=store_calls;
    CHECK(prop_prepare_saved(rules,rewards,&state,&save,30,6,world_draw,&save,store,RT_NULL),PROP_OK);
    unchanged();CHECK(draw_count,n);CHECK(store_calls,k);
    store_failure=1;
    CHECK(prop_collect_saved(&state,rules,&save,30,6,store,RT_NULL),PROP_STORAGE);unchanged();CHECK(draw_count,n);
    store_failure=0;CHECK(prop_collect_saved(&state,rules,&save,30,6,store,RT_NULL),PROP_OK);
    prior_state=state;prior_save=save;k=store_calls;
    CHECK(prop_collect_saved(&state,rules,&save,30,6,store,RT_NULL),PROP_UNAVAILABLE);
    unchanged();CHECK(store_calls,k);CHECK(draw_count,n);
}
#ifndef __m68k__
static void source_cases(void)
{
    u8 id,stat,k;u16 compared=0;
    for(id=0;id<96;id++)for(stat=0;stat<2;stat++) {
        const u8 *row=prop_row(rules,id);u8 match=0;
        seed(id,stat?3:1,stat,stat);draw_value=stat?0:32767;
        for(k=0;k<state.dispatches.job[0].n;k++)if(row[20]==save.unit[k].job-73)match=1;
        if(match)continue; // unsafe original preferred-job fallback is a labelled adaptation
        CHECK(prop_prepare(rules,rewards,&state,&save,id,state.dispatches.job[0].location,draw,RT_NULL),PROP_OK);
        CHECK(original_result(&state,&save,draw_value),1);compared++;
        CHECK(prop_state_valid(&state,rules,save.n),1);
    }
    // Exact safe preferred-job61% threshold, including the first rejected draw.
    for(id=0;id<96;id++)if(prop_row(rules,id)[20])break;
    for(k=0;k<4;k++) {
        static const u16 rolls[]={0,19988,19989,32767};
        seed(id,1,0,0);save.unit[0].job=(u8)(73+prop_row(rules,id)[20]);draw_value=rolls[k];
        CHECK(prop_prepare(rules,rewards,&state,&save,id,state.dispatches.job[0].location,draw,RT_NULL),PROP_OK);
        CHECK(draw_count>=1,1);CHECK(state.report.result==PROP_SUCCESS,k<2?1:0);
    }
    for(k=0;k<4;k++) {
        static const u16 rolls[]={3276,3277,13107,13108};
        seed(30,3,1,1);draw_value=rolls[k];
        CHECK(prop_prepare(rules,rewards,&state,&save,30,6,draw,RT_NULL),PROP_OK);
        CHECK(state.report.index,3+(k==0?2:k==3?0:1));CHECK(original_result(&state,&save,draw_value),1);
    }
    // Verify the constant reciprocal over its declared domain once on PC.
    {u16 n;for(n=0;n<32768U;n++)if(prop_hundred(n)!=n/100U){failures++;break;}}
    printf("Original reward pipelines: %u; safe preferred-job adaptation checked separately\n",compared);
}
static u8 disk_store(const Save *s,const PropositionState *p,void *ctx)
{
    char temp[512];const char *path=(const char *)ctx;FILE *f;u8 good;
    store_calls++;if(store_failure==1)return 0;
    snprintf(temp,sizeof temp,"%s.tmp",path);f=fopen(temp,"wb");if(!f)return 0;
    good=fwrite(s,1,sizeof *s,f)==sizeof *s&&fwrite(p,1,sizeof *p,f)==sizeof *p;
    if(fclose(f))good=0;
    if(store_failure==2)good=0;
    if(good&&rename(temp,path)==0)return 1;
    remove(temp);return 0;
}
static void load_snapshot(const char *path)
{
    FILE *f;
    memset(&state,0,sizeof state);memset(&save,0,sizeof save);f=fopen(path,"rb");CHECK(f!=RT_NULL,1);if(!f)return;
    CHECK(fread(&save,1,sizeof save,f),sizeof save);CHECK(fread(&state,1,sizeof state,f),sizeof state);CHECK(fclose(f),0);
}
static void snapshots(const char *path)
{
    u8 count;u16 draws;
    seed(30,3,1,1);store_failure=0;CHECK(disk_store(&save,&state,(void *)path),1);
    prior_state=state;prior_save=save;store_failure=2;
    CHECK(prop_prepare_saved(rules,rewards,&state,&save,30,6,world_draw,&save,disk_store,(void *)path),PROP_STORAGE);
    unchanged();load_snapshot(path);unchanged();store_failure=0;
    CHECK(prop_prepare_saved(rules,rewards,&state,&save,30,6,world_draw,&save,disk_store,(void *)path),PROP_OK);
    prior_state=state;prior_save=save;draws=draw_count;count=store_calls;load_snapshot(path);unchanged();
    CHECK(prop_prepare_saved(rules,rewards,&state,&save,30,6,world_draw,&save,disk_store,(void *)path),PROP_OK);
    unchanged();CHECK(draw_count,draws);CHECK(store_calls,count);store_failure=2;
    CHECK(prop_collect_saved(&state,rules,&save,30,6,disk_store,(void *)path),PROP_STORAGE);
    unchanged();load_snapshot(path);unchanged();CHECK(draw_count,draws);store_failure=0;
    CHECK(prop_collect_saved(&state,rules,&save,30,6,disk_store,(void *)path),PROP_OK);
    prior_state=state;prior_save=save;count=store_calls;load_snapshot(path);unchanged();
    CHECK(prop_collect_saved(&state,rules,&save,30,6,disk_store,(void *)path),PROP_UNAVAILABLE);
    unchanged();CHECK(draw_count,draws);CHECK(store_calls,count);
}
int main(int argc,char **argv)
{
    bounds();printf("bounds %u %u %08lX %u\n",checks,failures,(unsigned long)digest,(unsigned)sizeof state);
    source_cases();if(argc==2)snapshots(argv[1]);else failures++;
    printf("Propositions rewards: %u checks, %u failures, digest%08lX, state%u Save%u\n",
           checks,failures,(unsigned long)digest,(unsigned)sizeof state,(unsigned)sizeof save);
    return failures?1:0;
}
#else
void _main(void){bounds();BENCH_VALUE(checks);BENCH_VALUE(failures);BENCH_VALUE(digest);BENCH_VALUE(sizeof state);}
#endif
