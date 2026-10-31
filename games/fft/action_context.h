// One root action, at most one pending response per actor, no recursive reactions.
#ifndef FFT_ACTION_CONTEXT_H
#define FFT_ACTION_CONTEXT_H
#include "core_limits.h"
#include "status_policy.h"
enum { CTX_IDLE,CTX_ROOT,CTX_IMPACT,CTX_DRAIN,CTX_SECONDARY,CTX_APPLIED };
enum { CTX_OK,CTX_BUSY,CTX_INVALID,CTX_DUPLICATE,CTX_NESTED };
typedef struct {
    u16 pending,reaction[FFT_CORE_UNIT_MAX];
    u8 root,actor,phase,cursor;
    u8 left,cell;                     // extra primary strikes and fixed tactical tile
} CoreAction;
typedef char CoreActionSize[(sizeof(CoreAction)==34)?1:-1];
static u8 core_action_begin(CoreAction *c,u8 actor,u8 count)
{
    if(!count||count>FFT_CORE_UNIT_MAX||actor>=count)return CTX_INVALID;
    if(c->phase!=CTX_IDLE)return CTX_BUSY;
    c->pending=0;c->root=c->actor=actor;c->phase=CTX_ROOT;c->cursor=0;c->left=0;
    return CTX_OK;
}
static u8 core_action_impact(CoreAction *c,u8 actor)
{
    if(actor!=c->actor)return 0;
    if(c->phase==CTX_ROOT)c->phase=CTX_IMPACT;
    else if(c->phase==CTX_SECONDARY)c->phase=CTX_APPLIED;
    else return 0;
    return 1;
}
static u8 core_action_queue(CoreAction *c,u8 actor,u16 ability,u8 count)
{
    u16 bit;
    if(!count||count>FFT_CORE_UNIT_MAX||actor>=count||actor==c->root||
       (u16)(ability-0x1a6)>=32)return CTX_INVALID;
    if(c->phase>=CTX_DRAIN)return CTX_NESTED;
    if(c->phase==CTX_IDLE)return CTX_BUSY;
    bit=(u16)1<<actor;if(c->pending&bit)return CTX_DUPLICATE;
    c->reaction[actor]=ability;c->pending|=bit;return CTX_OK;
}
// Caller revalidates live eligibility before supplying the allowed actor mask.
// Excluded requests expire with this root; they never leak to the next action.
static u8 core_action_take(CoreAction *c,u16 allowed)
{
    u16 bit;u8 id;
    if(c->left||(c->phase!=CTX_IMPACT&&c->phase!=CTX_DRAIN))return 255;
    c->phase=CTX_DRAIN;c->pending&=allowed;
    for(id=c->cursor;id<FFT_CORE_UNIT_MAX;id++){
        bit=(u16)1<<id;if(!(c->pending&bit))continue;
        c->pending&=(u16)~bit;c->cursor=id+1;c->actor=id;c->phase=CTX_SECONDARY;
        return id;
    }
    return 255;
}
static u8 core_action_return(CoreAction *c)
{
    if(c->phase!=CTX_SECONDARY&&c->phase!=CTX_APPLIED)return 0;
    c->actor=c->root;c->phase=CTX_DRAIN;return 1;
}
static u8 core_action_close(CoreAction *c)
{
    if((c->phase!=CTX_IMPACT&&c->phase!=CTX_DRAIN)||c->pending||c->left)return 0;
    c->phase=CTX_IDLE;return 1;
}
// Source PREVENT_REACTION set, built by main_status_init_check_data. Don't Act
// is a separate source action restriction. Dry/unmounted policy only here.
static u8 core_reaction_blocked(const CoreStatus *s,u8 secondary,u16 current,u16 proc,u16 pending)
{
    if(secondary||(core_status_current(s,0)&0x64)||(core_status_current(s,1)&0x95)||
       (core_status_current(s,2)&0x0e)||(core_status_current(s,3)&2)||
       (core_status_current(s,4)&0x14))return 1;
    if(current&&!proc)return 1;
    if(pending)return 2;
    return core_status_current(s,4)&4?1:0;
}
#endif
