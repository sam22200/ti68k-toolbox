#ifndef FFT_PROPOSITION_TRANSACTIONS_H
#define FFT_PROPOSITION_TRANSACTIONS_H
#include "proposition_rewards.h"
// The integration owner supplies an atomic whole-session writer: a false
// return guarantees the old durable image is still intact. No file names,
// platform I/O, or second mutable receipt live in the service kernel.
typedef u8 (*PropositionStore)(const Save *,const PropositionState *,void *);
enum { PROP_STORAGE=8 };
static u8 prop_prepare_saved(const u8 *rules,const u8 *rewards,PropositionState *state,Save *save,
                             u8 id,u8 location,PropositionDraw draw,void *rng,
                             PropositionStore store,void *disk)
{
    u8 before[4],result,ready=state->report.ready;
    if(!store)return PROP_STORAGE;
    memcpy(before,save->world_rng,4);
    result=prop_prepare(rules,rewards,state,save,id,location,draw,rng);
    if(result!=PROP_OK||ready)return result;
    if(store(save,state,disk))return PROP_OK;
    // Before the first preparation, validity guarantees an entirely empty
    // receipt. Only it and the caller-owned world RNG can have changed.
    memset(&state->report,0,sizeof state->report);memcpy(save->world_rng,before,4);
    return PROP_STORAGE;
}
typedef struct {
    PropositionReport report;
    PropositionJob job[PROP_JOBS];
    u8 n,status,retry,completed[2],discovery[2],money[4],points[3][4];
} PropositionUndo;
static u8 prop_collect_saved(PropositionState *state,const u8 *rules,Save *save,u8 id,u8 location,
                             PropositionStore store,void *disk)
{
    PropositionUndo before;u8 k,result;u16 date_at;u8 *discovery=RT_NULL;
    if(!store)return PROP_STORAGE;
    if(!prop_state_valid(state,rules,save->n))return PROP_CORRUPT;
    if(!state->report.ready||state->report.id!=id||state->report.location!=location)return PROP_UNAVAILABLE;
    before.report=state->report;memcpy(before.job,state->dispatches.job,sizeof before.job);before.n=state->dispatches.n;
    before.status=state->dispatches.status[id>>1];before.retry=state->dispatches.retry[id];
    date_at=(u16)(((u16)id*9)>>3);memcpy(before.completed,state->completed_date+date_at,2);
    if(before.report.kind==PROP_TREASURE)discovery=state->treasure_date+((u16)before.report.index*9>>3);
    else if(before.report.kind==PROP_LAND)discovery=state->land_date+((u16)before.report.index*9>>3);
    if(discovery)memcpy(before.discovery,discovery,2);
    memcpy(before.money,save->money,4);
    for(k=0;k<before.report.n;k++) {
        const SaveUnit *u=&save->unit[before.report.unit[k]];u8 career=before.report.career[k];
        memcpy(before.points[k],u->jp[career],2);memcpy(before.points[k]+2,u->tjp[career],2);
    }
    result=prop_collect(state,rules,save,id,location);if(result!=PROP_OK)return result;
    if(store(save,state,disk))return PROP_OK;
    state->report=before.report;memcpy(state->dispatches.job,before.job,sizeof before.job);state->dispatches.n=before.n;
    state->dispatches.status[id>>1]=before.status;state->dispatches.retry[id]=before.retry;
    memcpy(state->completed_date+date_at,before.completed,2);
    if(discovery)memcpy(discovery,before.discovery,2);
    memcpy(save->money,before.money,4);
    for(k=0;k<before.report.n;k++) {
        SaveUnit *u=&save->unit[before.report.unit[k]];u8 career=before.report.career[k];
        memcpy(u->jp[career],before.points[k],2);memcpy(u->tjp[career],before.points[k]+2,2);
    }
    return PROP_STORAGE;
}
#endif
