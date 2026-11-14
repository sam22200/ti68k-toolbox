#ifndef FFT_BREEDING_H
#define FFT_BREEDING_H
#include <string.h>
#include "guest_roster.h"
// Cold portable backend, not yet appended to FSV6 or called by worldmap.
// Include ../base.inc first for the existing verified muls16 primitive.
// Each egg uses an ordinary SaveUnit, generated once at laying. The saved
// level-one seeds use the existing raw-current convention; no growth loop
// or second Job table is needed while travelling.
#define WB_SLOTS (PARTY_MAX+GUEST_MAX)
#define WB_RULE_BYTES 285U
typedef struct {
    u8 month,day,era,reserved;          // source var67 increments on March21, cap99
    u8 egg[WB_SLOTS];                 // initial days in high nibble, remaining in low
    u8 birthday[WB_SLOTS][2];          // BE day1..365 after hatch;0 unknown/not born
    u8 egg_color[(WB_SLOTS+3)/4];      // source color0..3 retained through hatch/reload
} WorldParty;
enum { WB_INVALID,WB_DAY,WB_LAID,WB_FULL };
typedef u16 (*WorldDraw)(void *);       // caller supplies original-style15-bit draws
static u16 wb_word(const u8 *p){return (u16)((u16)p[0]<<8|p[1]);}
static u8 wb_monster(u8 job){return job>=94&&job<142;}
static u8 wb_birth_sign(const u8 *p,u16 birthday)
{
    u8 i,sign=9;
    for(i=0;i<12;i++)if(birthday>=wb_word(p+261+2*i))sign++;
    return sign>=12?(u8)(sign-12):sign;
}
static u8 world_egg_color(const WorldParty *w,u8 slot)
{return slot<WB_SLOTS?(w->egg_color[slot>>2]>>((slot&3)*2))&3:0;}
static void wb_color_set(WorldParty *w,u8 slot,u8 color)
{
    u8 shift=(slot&3)*2,mask=(u8)(3U<<shift);
    w->egg_color[slot>>2]=(w->egg_color[slot>>2]&(u8)~mask)|((color&3)<<shift);
}
// Cold validation at bank binding, never once per map animation frame.
static u8 world_rules_valid(const u8 *p,u16 size)
{
    static const u8 months[]={31,28,31,30,31,30,31,31,30,31,30,31};
    u8 i;u16 previous=0;
    if(!p||size!=WB_RULE_BYTES||memcmp(p,"FWB1",4)||wb_word(p+4)!=size||p[6]!=94||p[7]!=48||
       memcmp(p+8,months,12))return 0;
    for(i=0;i<13;i++){
        const u8 *r=p+20+3*i;u16 ordinal=r[1];u8 m;
        if(!r[0]||r[0]>12||!r[1]||r[1]>months[r[0]-1]||r[2]>=12)return 0;
        for(m=1;m<r[0];m++)ordinal+=months[m-1];
        if(ordinal<=previous)return 0;
        previous=ordinal;
    }
    if(previous!=365)return 0;
    for(i=0;i<48;i++){
        const u8 *r=p+59+4*i;
        if(!r[0]||r[0]>15||!wb_monster(r[1])||!wb_monster(r[2])||!wb_monster(r[3]))return 0;
    }
    previous=0;
    for(i=0;i<12;i++){u16 limit=wb_word(p+261+2*i);if(limit<=previous||limit>365)return 0;previous=limit;}
    // Both birthday algorithms must agree at each interval endpoint. A corrupt
    // main limit may not pass just because its entries are still ordered.
    previous=0;
    for(i=0;i<13;i++){
        const u8 *r=p+20+3*i;u16 ordinal=r[1];u8 m;
        for(m=1;m<r[0];m++)ordinal+=months[m-1];
        if(wb_birth_sign(p,(u16)(previous+1))!=r[2]||wb_birth_sign(p,ordinal)!=r[2])return 0;
        previous=ordinal;
    }
    return 1;
}
static u8 wb_present(const Save *s,u8 i)
{return i<PARTY_MAX?i<s->n:save_guest_id(s,(u8)(i-PARTY_MAX))!=0;}
static SaveUnit *wb_unit(Save *s,u8 i)
{return i<PARTY_MAX?&s->unit[i]:&s->guest[i-PARTY_MAX];}
static const SaveUnit *wb_read_unit(const Save *s,u8 i)
{return i<PARTY_MAX?&s->unit[i]:&s->guest[i-PARTY_MAX];}
static u8 world_party_valid(const WorldParty *w,const Save *s,const u8 *rules)
{
    u8 i;
    if(memcmp(s->magic,SAVE_MAGIC,4)||!save_guests_valid(s)||w->reserved||w->era>99||
       !w->month||w->month>12||!w->day||w->day>rules[7+w->month])return 0;
    for(i=0;i<WB_SLOTS;i++){
        u8 egg=w->egg[i],total=egg>>4,left=egg&15;u16 born=wb_word(w->birthday[i]);
        const SaveUnit *u=wb_read_unit(s,i);
        if(!wb_present(s,i)){if(egg||born||world_egg_color(w,i))return 0;continue;}
        if(!u->level||u->level>99||born>365)return 0;
        if(egg&&(total<2||left==0||left>total||born||u->type!=3||!wb_monster(u->job)))return 0;
    }
    return 1;
}
static u8 wb_zodiac(const u8 *p,u8 month,u8 day)
{
    u8 i;for(i=0;i<13;i++){
        const u8 *r=p+20+3*i;
        if(month<r[0]||(month==r[0]&&day<=r[1]))return r[2];
    }
    return 255; // only reached on invalid input
}
static u16 wb_ordinal(const u8 *p,u8 month,u8 day)
{u16 n=day;while(--month)n+=p[7+month];return n;}
static u16 wb_draw(WorldDraw draw,void *ctx){return draw(ctx)&32767U;}
// Source main_unit_generate_out_of_battle, monster subset. Temporary random
// birthday is overwritten by the egg, but its draw still precedes Brave/Faith.
static void wb_generate(Save *s,SaveUnit *u,const u8 *p,u8 job,u8 days,WorldDraw draw,void *ctx)
{
    u8 i,name,used;
    memset(u,0,sizeof *u);u->job=job;u->gfx=255;u->type=3;u->level=1;
    wb_draw(draw,ctx);
    u->brave=(u8)(muls16((s16)wb_draw(draw,ctx),31)>>15)+40;
    u->faith=(u8)(muls16((s16)wb_draw(draw,ctx),31)>>15)+40;
    for(i=0;i<5;i++){
        u32 raw=((u32)p[251+i]<<14)+(u32)(muls16((s16)wb_draw(draw,ctx),p[256+i])>>1);
        u->raw[i][0]=(u8)(raw>>16);u->raw[i][1]=(u8)(raw>>8);u->raw[i][2]=(u8)raw;
    }
    do{
        name=(u8)(muls16((s16)wb_draw(draw,ctx),255)>>15);used=0;
        for(i=0;i<s->n;i++)if(s->unit[i].type==3&&s->unit[i].name==name){used=1;break;}
    }while(used);
    u->name=name;
    u->zodiac=wb_birth_sign(p,(u16)days*17);
}
// Advance exactly one completed location-to-location leg. Original order:
// date -> existing eggs -> roll -> parent -> offspring -> days/color ->
// generate child -> donor level. Newly hatched units enter neither list today.
// The caller owns persistence/reentry and the daily reseed; this pure operation
// does not make travel/session writes atomic or choose a migration start date.
static u8 world_party_day(WorldParty *w,Save *s,const u8 *rules,WorldDraw draw,void *ctx)
{
    u8 parents[WB_SLOTS],levels[WB_SLOTS],np=0,nl=0,i,rarity,job,days,color;u16 roll;
    SaveUnit *child;
    if(!draw||!world_party_valid(w,s,rules))return WB_INVALID;
    if(++w->day>rules[7+w->month]){w->day=1;if(++w->month==13)w->month=1;}
    if(w->month==3&&w->day==21&&w->era<99)w->era++;
    for(i=0;i<WB_SLOTS;i++)if(wb_present(s,i)){
        SaveUnit *u=wb_unit(s,i);
        if(w->egg[i]){
            if(!(--w->egg[i]&15)){
                u16 born=wb_ordinal(rules,w->month,w->day);
                w->egg[i]=0;w->birthday[i][0]=(u8)(born>>8);w->birthday[i][1]=(u8)born;
                u->zodiac=wb_zodiac(rules,w->month,w->day);
            }
        }else{
            levels[nl++]=u->level;
            if(wb_monster(u->job))parents[np++]=i;
        }
    }
    if(!np)return WB_DAY;
    roll=wb_draw(draw,ctx)&255;
    rarity=roll<6?3:roll<16?2:roll<32?1:0;
    if(!rarity)return WB_DAY;
    i=parents[wb_draw(draw,ctx)%np];
    job=rules[59+4*(wb_unit(s,i)->job-94)+rarity];
    days=(u8)(wb_draw(draw,ctx)%rules[59+4*(job-94)]+1);if(days<2)days=2;
    color=(u8)(wb_draw(draw,ctx)&3);
    if(s->n==PARTY_MAX)return WB_FULL; // source consumes four draws even when full
    i=s->n;child=&s->unit[i];wb_generate(s,child,rules,job,days,draw,ctx);
    child->level=levels[wb_draw(draw,ctx)%nl];save_raw_set(s,i,0);
    w->egg[i]=(u8)(days*17);w->birthday[i][0]=w->birthday[i][1]=0;wb_color_set(w,i,color);s->n++;
    return WB_LAID;
}
// Roster producers must move side metadata together with a SaveUnit. Kept
// separate from FSV6 operations until every recruit/remove/compact path binds it.
static void world_party_clear_slot(WorldParty *w,u8 slot)
{if(slot<WB_SLOTS){w->egg[slot]=0;w->birthday[slot][0]=w->birthday[slot][1]=0;wb_color_set(w,slot,0);}}
static void world_party_move_slot(WorldParty *w,u8 to,u8 from)
{
    if(to>=WB_SLOTS||from>=WB_SLOTS||to==from)return;
    w->egg[to]=w->egg[from];memcpy(w->birthday[to],w->birthday[from],2);
    wb_color_set(w,to,world_egg_color(w,from));world_party_clear_slot(w,from);
}
#endif
