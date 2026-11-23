#ifndef FFT_PROPOSITION_RULES_H
#define FFT_PROPOSITION_RULES_H
#include <string.h>
// Portable cold service backend, durably owned by the campaign FSV9 Save.
// All fields are serialized bytes; callers supply canonical generic eligibility.
#include "proposition_shape.h"
typedef struct {u8 unlocked,location,tier,zodiac;} PropositionContext;
static const u8 *prop_row(const u8 *p,u8 id){return p+28+25*(u16)id;}
static const char *prop_title(const u8 *p,u8 id)
{return (const char *)p+prop_word(prop_row(p,id)+23);}
static u16 prop_fee(const u8 *p,u8 id)
{const u8 *r=prop_row(p,id);return (u16)(prop_word(p+8+2*r[4])+prop_word(p+8+2*r[5]));}
static u8 prop_bank_valid(const u8 *p,u16 size)
{
    u16 total,i;
    if(!p||size<PROP_FIRST||memcmp(p,"FPR1",4)||p[6]!=PROP_N||p[7]!=1)return 0;
    total=prop_word(p+4);if(total>size||total<PROP_FIRST)return 0;
    for(i=8;i<28;i+=2)if(prop_word(p+i)>10000)return 0;
    for(i=0;i<PROP_N;i++) {
        const u8 *r=prop_row(p,(u8)i);u16 at=prop_word(r+23),first=at;
        if(r[2]!=i+1||r[4]>=10||r[5]>=10||!r[6]||r[6]>r[7]||r[7]>32||r[16]>43||
           r[17]&~7U||((r[17]&2)&&(!r[22]||r[22]>12))||((r[17]&4)&&r[22]>=PROP_N))return 0;
        if(at<PROP_FIRST||at>=total)return 0;
        while(at<total&&p[at]){if(p[at]<32||p[at]>126)return 0;at++;}
        if(at==total||at==first||at-first>80)return 0;
    }
    return 1;
}
// Calendar conversion stays with the existing shared world-clock owner. Zodiac
// is source1-based Aries..Pisces, not calendar month or native0-based sign.
static u8 prop_available(const u8 *p,const PropositionJobs *s,u8 id,const PropositionContext *c)
{
    const u8 *r;
    if(!c->unlocked||c->location>=43||!c->zodiac||c->zodiac>12||id>=PROP_N||
       (prop_status(s,id)&(PROP_PROGRESS|PROP_COMPLETE))||(s->retry[id]&63))return 0;
    r=prop_row(p,id);
    if(r[16]&&r[16]!=c->location+1)return 0;
    if((r[17]&1)&&c->tier<r[21])return 0;
    if((r[17]&2)&&c->zodiac!=r[22])return 0;
    if((r[17]&4)&&!(prop_status(s,r[22])&PROP_COMPLETE))return 0;
    return 1;
}
static u8 prop_jobs_valid(const PropositionJobs *s,const u8 *p,u8 roster)
{
    u8 i;if(!prop_jobs_shape_valid(s,roster))return 0;
    for(i=0;i<s->n;i++) {
        const PropositionJob *j=&s->job[i];const u8 *r=prop_row(p,j->id);
        if(j->days<r[6]||j->days>r[7]||(r[16]&&r[16]!=j->location+1))return 0;
    }
    return 1;
}
// Drafts never touch this state. Final confirmation rechecks every precondition
// before touching funds, flags or participants. Eligibility is an explicit
// 16-bit caller mask, not inferred from palette/job/type alone.
static u8 prop_dispatch(const u8 *p,PropositionJobs *s,u8 id,const PropositionContext *c,
                        const u8 *team,u8 n,u8 days,u8 roster,u16 eligible,u32 *gil)
{
    u8 k;u16 selected=0,fee;PropositionJob *j;const u8 *r;
    if(!prop_jobs_valid(s,p,roster))return PROP_CORRUPT;
    if(!c->unlocked)return PROP_LOCKED;
    if(!prop_available(p,s,id,c))return PROP_UNAVAILABLE;
    if(s->n==PROP_JOBS)return PROP_FULL;
    if(!team||!n||n>PROP_WORKERS)return PROP_TEAM;
    r=prop_row(p,id);if(days<r[6]||days>r[7])return PROP_DAYS;
    for(k=0;k<n;k++) {
        u8 u=team[k];u16 bit;
        if(u>=roster)return PROP_TEAM;bit=(u16)(1U<<u);
        if(!(eligible&bit)||(selected&bit)||prop_away(s,u))return PROP_TEAM;
        selected|=bit;
    }
    fee=prop_fee(p,id);if(*gil<fee)return PROP_GIL;
    j=&s->job[s->n];memset(j,0,sizeof *j);memset(j->unit,255,sizeof j->unit);
    j->flags=PROP_SENT;j->id=id;j->days=days;j->location=c->location;j->n=n;
    for(k=0;k<n;k++)j->unit[k]=team[k];
    prop_set_status(s,id,PROP_PROGRESS|PROP_ATTEMPTED);s->n++;*gil-=fee;
    return PROP_OK;
}
static void prop_day(PropositionJobs *s)
{
    u8 i;
    for(i=0;i<s->n;i++) {
        PropositionJob *j=&s->job[i];if(j->flags&PROP_RETURNED)continue;
        j->elapsed++;
        if(j->elapsed==j->days||(j->flags&PROP_RECALLED))j->flags|=PROP_RETURNED;
    }
    for(i=0;i<PROP_N;i++)if(s->retry[i]&63)s->retry[i]--;
}
static u8 prop_recall(PropositionJobs *s,u8 job)
{if(job>=s->n||s->job[job].flags&PROP_RETURNED)return 0;s->job[job].flags|=PROP_RECALLED;return 1;}
// Removal of an away worker is rejected. Removing a lower free roster slot
// remaps every stored participant; unrelated slot metadata stays with Save's
// existing compaction owner. Reporting/reward settlement is a separate commit.
static u8 prop_remove_slot(PropositionJobs *s,u8 unit)
{
    u8 i,k;if(unit>=16||prop_away(s,unit))return 0;
    for(i=0;i<s->n;i++)for(k=0;k<s->job[i].n;k++)if(s->job[i].unit[k]>unit)s->job[i].unit[k]--;
    return 1;
}
#endif
