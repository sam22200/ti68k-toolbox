#ifndef FFT_PROPOSITION_SHAPE_H
#define FFT_PROPOSITION_SHAPE_H
#include <string.h>
#include "proposition_types.h"
static u16 prop_word(const u8 *p){return (u16)((u16)p[0]<<8|p[1]);}
static u8 prop_status(const PropositionJobs *s,u8 id)
{return id<PROP_N?(u8)((s->status[id>>1]>>((id&1)*4))&15):0;}
static void prop_set_status(PropositionJobs *s,u8 id,u8 value)
{u8 shift=(u8)((id&1)*4);s->status[id>>1]=(u8)((s->status[id>>1]&~(15U<<shift))|((u16)value<<shift));}
static u8 prop_jobs_shape_valid(const PropositionJobs *s,u8 roster)
{
    u8 i,k;u16 seen=0;u8 active[PROP_N/8];
    memset(active,0,sizeof active);
    if(s->n>PROP_JOBS||roster>16)return 0;
    for(i=0;i<s->n;i++) {
        const PropositionJob *j=&s->job[i];
        if(j->id>=PROP_N||j->location>=43||!j->n||j->n>PROP_WORKERS||
           !(j->flags&PROP_SENT)||(j->flags&~7U))return 0;
        if(!j->days||j->days>32||j->elapsed>j->days||
           (j->elapsed==j->days&&!(j->flags&PROP_RETURNED))||
           ((j->flags&PROP_RETURNED)&&j->elapsed<j->days&&!(j->flags&PROP_RECALLED)))return 0;
        if((active[j->id>>3]&(1U<<(j->id&7)))||
           prop_status(s,j->id)!=(PROP_PROGRESS|PROP_ATTEMPTED))return 0;
        active[j->id>>3]|=(u8)(1U<<(j->id&7));
        for(k=0;k<j->n;k++) {
            u8 u=j->unit[k];u16 bit;
            if(u>=roster)return 0;bit=(u16)(1U<<u);if(seen&bit)return 0;seen|=bit;
        }
        for(;k<PROP_WORKERS;k++)if(j->unit[k]!=255)return 0;
    }
    for(;i<PROP_JOBS;i++) {
        const u8 *q=(const u8 *)&s->job[i];for(k=0;k<sizeof(PropositionJob);k++)if(q[k])return 0;
    }
    for(i=0;i<PROP_N;i++) {
        u8 status=prop_status(s,i);
        if(status&1||((status&(PROP_COMPLETE|PROP_PROGRESS))&&!(status&PROP_ATTEMPTED))||
           ((status&PROP_PROGRESS)!=0)!=((active[i>>3]&(1U<<(i&7)))!=0)||
           (s->retry[i]&63)>32||(s->retry[i]&192)==192||
           ((s->retry[i]&63)&&!(s->retry[i]&192))||
           (s->retry[i]&&(!(status&PROP_ATTEMPTED)||(status&PROP_COMPLETE)||((status&PROP_PROGRESS)&&(s->retry[i]&63)))))return 0;
    }
    return 1;
}
static u8 prop_away(const PropositionJobs *s,u8 unit)
{
    u8 i,k;if(unit>=16)return 0;
    for(i=0;i<s->n;i++)for(k=0;k<s->job[i].n;k++)if(s->job[i].unit[k]==unit)return 1;
    return 0;
}
static u8 prop_report_ready(const PropositionJobs *s,u8 job,u8 location)
{return job<s->n&&(s->job[job].flags&PROP_RETURNED)&&s->job[job].location==location;}
static u16 prop_date(const u8 *p,u8 id)
{
    u16 base=(u16)((u16)id*9),value=0;u8 k;
    for(k=0;k<9;k++)if(p[(base+k)>>3]&(1U<<((base+k)&7)))value|=(u16)(1U<<k);
    return value;
}
static void prop_set_date(u8 *p,u8 id,u16 date)
{
    u16 base=(u16)((u16)id*9);u8 k;
    for(k=0;k<9;k++) {
        u16 bit=(u16)(base+k);u8 mask=(u8)(1U<<(bit&7));
        if(date&(1U<<k))p[bit>>3]|=mask;else p[bit>>3]&=(u8)~mask;
    }
}
static u8 prop_date_valid(u16 date)
{u8 month=(u8)(date>>5),day=(u8)(date&31);return month&&month<=12&&day&&day<=world_month_days[month-1];}
static u32 prop_long(const u8 *p){return (u32)prop_word(p)<<16|prop_word(p+2);}
static void prop_put_word(u8 *p,u16 n){p[0]=(u8)(n>>8);p[1]=(u8)n;}
static void prop_put_long(u8 *p,u32 n){p[0]=(u8)(n>>24);p[1]=(u8)(n>>16);p[2]=(u8)(n>>8);p[3]=(u8)n;}
static u8 prop_job_index(const PropositionJobs *s,u8 id)
{u8 i;for(i=0;i<s->n;i++)if(s->job[i].id==id)return i;return 255;}
static u8 prop_state_shape_valid(const PropositionState *s,u8 roster)
{
    u8 i,k;const PropositionReport *r=&s->report;
    if(!prop_jobs_shape_valid(&s->dispatches,roster)||(s->treasure_date[34]&128))return 0;
    for(i=0;i<PROP_N;i++) {
        u16 date=prop_date(s->completed_date,i);
        if((date!=0)!=((prop_status(&s->dispatches,i)&PROP_COMPLETE)!=0)||(date&&!prop_date_valid(date)))return 0;
    }
    for(i=0;i<31;i++){u16 date=prop_date(s->treasure_date,i);if(date&&!prop_date_valid(date))return 0;}
    for(i=0;i<16;i++){u16 date=prop_date(s->land_date,i);if(date&&!prop_date_valid(date))return 0;}
    if(!r->ready) {
        const u8 *b=(const u8 *)r;for(i=0;i<sizeof *r;i++)if(b[i])return 0;
        return 1;
    }
    if(r->ready!=1||r->id>=PROP_N||r->result>2||r->kind>3||
       (r->kind&&r->result!=PROP_SUCCESS)||
       (r->kind==PROP_TREASURE&&r->index>=31)||(r->kind==PROP_LAND&&r->index>=16)||
       (r->kind==PROP_RANDOM&&r->index>=24)||(!r->kind&&r->index)||prop_long(r->gil)>100000)return 0;
    i=prop_job_index(&s->dispatches,r->id);if(i==255)return 0;
    {
        const PropositionJob *j=&s->dispatches.job[i];u8 speaker=0;
        if(!prop_report_ready(&s->dispatches,i,r->location)||r->n!=j->n||memcmp(r->unit,j->unit,3))return 0;
        for(k=0;k<r->n;k++) {
            if(r->career[k]>=CAREER_N||prop_word(r->jp[k])>2000)return 0;
            if((j->flags&PROP_RECALLED)&&prop_word(r->jp[k]))return 0;
            if(r->unit[k]==r->speaker)speaker=1;
        }
        if(!speaker)return 0;
        for(;k<3;k++)if(r->career[k]||prop_word(r->jp[k]))return 0;
        if((j->flags&PROP_RECALLED)&&(r->result!=PROP_BAD_FAILURE||r->kind||prop_long(r->gil)))return 0;
    }
    return 1;
}
#endif
