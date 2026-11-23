#ifndef FFT_PROPOSITION_REWARDS_H
#define FFT_PROPOSITION_REWARDS_H
#include "proposition_rules.h"
// Serialized service state appended to campaign FSV9.
// No mutable report is supplied by a caller: prepare stores one frozen receipt,
// collect consumes that owned receipt and removes its job atomically in memory.
typedef u16 (*PropositionDraw)(void *);
static const u8 *prop_reward_table(const u8 *p,u8 k){return p+prop_word(p+8+2*k);}
static u8 prop_rewards_valid(const u8 *p,u16 size)
{
    static const u8 lengths[10]={176,66,31,15,16,16,16,48,20,20};u8 k;u16 total,i;
    if(!p||size<28||memcmp(p,"FPE1",4)||p[6]!=10||p[7]!=1)return 0;
    total=prop_word(p+4);if(total<28||total>size)return 0;
    for(k=0;k<10;k++) {
        u16 at=prop_word(p+8+2*k);const u8 *t;
        if(at<28||at>=total||lengths[k]>total-at)return 0;t=p+at;
        if(k<4){for(i=0;i<lengths[k];i++)if(t[i]>100)return 0;}
        else if(k<6){for(i=0;i<16;i+=2)if(!prop_word(t+i)||prop_word(t+i)>(k==4?200:10000))return 0;}
        else if(k<8){for(i=0;i<lengths[k];i+=2)if(t[i]>8||t[i+1]>8||(!t[i])!=(!t[i+1])||(k==6&&!t[i]))return 0;}
        else for(i=0;i<20;i+=2)if(prop_word(t+i)<100||prop_word(t+i)>150)return 0;
    }
    return 1;
}
static u8 prop_reward_rows_valid(const u8 *p,const u8 *rewards)
{
    u8 i;for(i=0;i<PROP_N;i++) {
        const u8 *r=prop_row(p,i);
        if(!r[0]||r[0]>8||!r[1]||r[1]>3||!r[9]||r[9]>8||r[10]>2||r[11]>1||
           !r[12]||r[12]>8||!r[19]||r[19]>4||r[20]>20)return 0;
        if(!r[10]&&r[11]) {
            const u8 *pairs=prop_reward_table(rewards,7)+6*(r[0]-1);u8 k;
            for(k=0;k<6;k++)if(!pairs[k])return 0;
        }
    }
    return 1;
}
// Exact /100 on the validated0..32767 range, using a16x16 reciprocal multiply.
// Reward percentage products are split so both factors fit the shared muls16.
static u16 prop_hundred(u16 n){return (u16)((u32)muls16((s16)n,5243)>>19);}
static u16 prop_scale(u16 n,u16 percent)
{
    u16 q=prop_hundred(n),rest=(u16)(n-((q<<6)+(q<<5)+(q<<2)));
    return (u16)(muls16((s16)q,(s16)percent)+prop_hundred((u16)muls16((s16)rest,(s16)percent)));
}
static u8 prop_state_valid(const PropositionState *s,const u8 *p,u8 roster)
{return prop_state_shape_valid(s,roster)&&prop_jobs_valid(&s->dispatches,p,roster);}
static u8 prop_state_remove_slot(PropositionState *s,u8 unit)
{return s->report.ready?0:prop_remove_slot(&s->dispatches,unit);}
// Input-bound validation before the first draw. Do not use this as a generic
// identity test: the dispatch owner still supplies the source eligibility mask.
static u8 prop_report_units_valid(const Save *save,const PropositionJob *j)
{
    u8 k;for(k=0;k<j->n;k++) {
        const SaveUnit *u=&save->unit[j->unit[k]];
        if(u->type>=2||u->job<74||u->job>93||!u->level||u->level>99||u->brave>100||u->faith>100)return 0;
    }
    return 1;
}
static u8 prop_speaker(const u8 *row,const Save *save,const PropositionJob *j)
{
    u8 k,slot=j->unit[0],best=row[19]&1?0:101;
    for(k=0;k<j->n;k++) {
        const SaveUnit *u=&save->unit[j->unit[k]];u8 value=row[19]<=2?u->brave:u->faith;
        if((row[19]&1)?value>best:value<best){best=value;slot=j->unit[k];}
    }
    return slot;
}
static u8 prop_tier20(u8 n)
{return n>80?4:n>60?3:n>40?2:n>20?1:0;}
static u8 prop_tier10(u8 n)
{u8 k=0;while(n>10){n-=10;k++;}return k;}
static u16 prop_score(const u8 *bank,const u8 *row,const SaveUnit *u)
{
    u8 job=(u8)(u->job-74),list=(u8)(row[1]-1);u16 score;
    score=prop_reward_table(bank,0)[(u16)(row[0]-1)*22+job];
    score+=prop_reward_table(bank,1)[(u16)list*22+job];
    score+=prop_reward_table(bank,2)[(u16)list*5+prop_tier20(u->brave)];
    score+=prop_reward_table(bank,3)[(u16)list*5+prop_tier20(u->faith)];
    score+=prop_reward_table(bank,2)[(u16)list*10+prop_tier10(u->level)];
    return score;
}
static u8 prop_roll(PropositionDraw draw,void *ctx,u8 count)
{return (u8)((u32)muls16((s16)(draw(ctx)&32767U),count)>>15);}
static u8 prop_discovery(const u8 *dates,u8 count,PropositionDraw draw,void *ctx)
{
    u8 i,unowned=0,pick;
    for(i=0;i<count;i++)if(!prop_date(dates,i))unowned++;
    if(!unowned)return prop_roll(draw,ctx,count);
    pick=prop_roll(draw,ctx,unowned);
    for(i=0;i<count;i++)if(!prop_date(dates,i)&&!pick--)return i;
    return 0;
}
static u8 prop_prepare(const u8 *rules,const u8 *rewards,PropositionState *state,const Save *save,
                       u8 id,u8 location,PropositionDraw draw,void *ctx)
{
    PropositionReport r;const PropositionJob *j;const u8 *row;u16 scores[3]={0},total=0,gil[3]={0};
    u8 i,slot;
    if(!prop_state_valid(state,rules,save->n))return PROP_CORRUPT;
    if(state->report.ready)return state->report.id==id&&state->report.location==location?PROP_OK:PROP_UNAVAILABLE;
    slot=prop_job_index(&state->dispatches,id);
    if(slot==255||!prop_report_ready(&state->dispatches,slot,location))return PROP_UNAVAILABLE;
    j=&state->dispatches.job[slot];if(!prop_report_units_valid(save,j))return PROP_TEAM;
    if(!(j->flags&PROP_RECALLED)&&!draw)return PROP_CORRUPT;
    row=prop_row(rules,id);memset(&r,0,sizeof r);r.ready=1;r.id=id;r.location=location;r.n=j->n;
    memcpy(r.unit,j->unit,3);r.speaker=prop_speaker(row,save,j);
    for(i=0;i<j->n;i++)r.career[i]=(u8)(save->unit[j->unit[i]].job-74);
    if(j->flags&PROP_RECALLED)r.result=PROP_BAD_FAILURE;
    else {
        u16 jp_base=prop_word(prop_reward_table(rewards,4)+2*(row[9]-1));
        u16 gil_base=prop_word(prop_reward_table(rewards,5)+2*(row[12]-1));
        for(i=0;i<j->n;i++){scores[i]=prop_score(rewards,row,&save->unit[j->unit[i]]);total+=scores[i];}
        r.result=total>=99?PROP_SUCCESS:total>=59?PROP_FAILURE:PROP_BAD_FAILURE;
        // Safe native adaptation: scan assigned units, preserving the original
        // <61 percent threshold. The source fallback reads past its participant
        // list; do not reproduce that unrelated/stale-slot access.
        if(r.result&&row[20])for(i=0;i<j->n;i++)
            if(row[20]==r.career[i]+1&&prop_roll(draw,ctx,100)<61){r.result=PROP_SUCCESS;break;}
        if(r.result==PROP_FAILURE){jp_base=prop_scale(jp_base,10);gil_base=prop_scale(gil_base,20);}
        else if(r.result==PROP_BAD_FAILURE){jp_base=prop_scale(jp_base,5);gil_base=prop_scale(gil_base,5);}
        for(i=0;i<j->n;i++){prop_put_word(r.jp[i],(u16)(jp_base+scores[i]));gil[i]=(u16)(gil_base+scores[i]);}
        if(!r.result) {
            const u8 *pair=RT_NULL;
            if(row[10]) {
                r.kind=row[10];r.index=prop_discovery(r.kind==PROP_TREASURE?state->treasure_date:state->land_date,
                                                     r.kind==PROP_TREASURE?31:16,draw,ctx);
                pair=prop_reward_table(rewards,6)+2*(row[0]-1);
            } else if(row[11]) {
                u8 roll=prop_roll(draw,ctx,100);
                r.kind=PROP_RANDOM;r.index=(u8)(3*(row[0]-1)+(roll<10?2:roll<40?1:0));
                pair=prop_reward_table(rewards,7)+2*r.index;
            }
            if(pair) {
                u16 bonus=prop_word(prop_reward_table(rewards,4)+2*(pair[1]-1));
                for(i=0;i<j->n;i++)prop_put_word(r.jp[i],(u16)(prop_word(r.jp[i])+bonus));
                prop_put_long(r.gil,prop_word(prop_reward_table(rewards,5)+2*(pair[0]-1)));
            }
        }
        {
            u8 day=(u8)(j->elapsed-row[6]);u16 jp_scale,gil_scale;
            if(day>9)day=9;
            jp_scale=prop_word(prop_reward_table(rewards,8)+2*day);
            gil_scale=prop_word(prop_reward_table(rewards,9)+2*day);
            for(i=0;i<j->n;i++){prop_put_word(r.jp[i],prop_scale(prop_word(r.jp[i]),jp_scale));gil[i]=prop_scale(gil[i],gil_scale);}
        }
        {
            u32 amount=prop_long(r.gil);for(i=0;i<j->n;i++)amount+=gil[i];prop_put_long(r.gil,amount);
        }
    }
    state->report=r;return PROP_OK;
}
static u8 prop_collect(PropositionState *state,const u8 *rules,Save *save,u8 id,u8 location)
{
    const PropositionReport *r=&state->report;u8 slot,k;u16 date=(u16)((u16)save->world.month<<5|save->world.day);u32 money;
    if(!prop_state_valid(state,rules,save->n)||!prop_date_valid(date)||prop_long(save->money)>99999999UL)return PROP_CORRUPT;
    if(!r->ready||r->id!=id||r->location!=location)return PROP_UNAVAILABLE;
    slot=prop_job_index(&state->dispatches,id);if(slot==255)return PROP_CORRUPT;
    for(k=0;k<r->n;k++)if(save->unit[r->unit[k]].type>=2)return PROP_TEAM;
    money=prop_long(save->money)+prop_long(r->gil);if(money>99999999UL)money=99999999UL;
    prop_put_long(save->money,money);
    for(k=0;k<r->n;k++) {
        SaveUnit *u=&save->unit[r->unit[k]];u8 career=r->career[k];u16 amount=prop_word(r->jp[k]);u32 n;
        n=(u32)prop_word(u->jp[career])+amount;prop_put_word(u->jp[career],n>9999?9999:(u16)n);
        n=(u32)prop_word(u->tjp[career])+amount;prop_put_word(u->tjp[career],n>9999?9999:(u16)n);
    }
    prop_set_status(&state->dispatches,id,r->result==PROP_SUCCESS?12:8);
    if(r->result==PROP_SUCCESS){prop_set_date(state->completed_date,id,date);state->dispatches.retry[id]=0;}
    else state->dispatches.retry[id]=r->result==PROP_FAILURE?0x60:0x90;
    if(r->kind==PROP_TREASURE)prop_set_date(state->treasure_date,r->index,date);
    else if(r->kind==PROP_LAND)prop_set_date(state->land_date,r->index,date);
    for(k=slot;k+1<state->dispatches.n;k++)state->dispatches.job[k]=state->dispatches.job[k+1];
    state->dispatches.n--;memset(&state->dispatches.job[state->dispatches.n],0,sizeof(PropositionJob));
    memset(&state->report,0,sizeof state->report);return PROP_OK;
}
#endif
