#ifndef FFT_STORY_CHOICE_H
#define FFT_STORY_CHOICE_H
#include <string.h>
#include "guest_roster.h"
// FCX1: bounded, allocation-free choice/effect records. All offsets/IDs are BE
// bytes, including pooled/unaligned input; no native-word dereference is needed.
enum { SC_SET_FLAG=1,SC_CLEAR_FLAG,SC_RECRUIT_GUEST,SC_REMOVE_GUEST };
enum { SC_INVALID,SC_APPLIED,SC_REPLAY,SC_PARTY_FULL,SC_MISSING_GUEST,SC_GATED,SC_IO };
#define SC_OPTIONS 2
#define SC_EFFECT_MAX 8
static u16 sc_word(const u8 *p){return (u16)((u16)p[0]<<8|p[1]);}
static u8 sc_flag(const Save *s,u16 flag)
{return flag&&flag<=FLAG_N&&(s->flags[(flag-1)>>3]&(1U<<((flag-1)&7)))!=0;}
static void sc_flag_set(Save *s,u16 flag,u8 value)
{
    u8 bit=(u8)(1U<<((flag-1)&7));
    if(value)s->flags[(flag-1)>>3]|=bit;else s->flags[(flag-1)>>3]&=(u8)~bit;
}
static const u8 *sc_option(const u8 *p,u8 option){return p+12+8*option;}
static u8 sc_marker(const u8 *p,u16 flag)
{return sc_word(p+6)==flag||sc_word(sc_option(p,0))==flag||sc_word(sc_option(p,1))==flag;}
static u8 story_choice_valid(const u8 *p,u16 available)
{
    u16 size,done,guard;u8 option;
    if(!p||available<28||memcmp(p,"FCX1",4))return 0;
    size=sc_word(p+4);done=sc_word(p+6);guard=sc_word(p+8);
    if(size<28||size>available||!done||done>FLAG_N||guard>FLAG_N||p[10]>1||
       (!guard&&p[10])||p[11]!=SC_OPTIONS||(guard&&sc_marker(p,guard)))return 0;
    for(option=0;option<SC_OPTIONS;option++){
        const u8 *o=sc_option(p,option);u16 result=sc_word(o),at=sc_word(o+2),effects=sc_word(o+4);
        u8 i,n=0;
        if(!result||result>FLAG_N||result==done||(option&&result==sc_word(sc_option(p,0)))||
           o[7]||o[6]>SC_EFFECT_MAX||at<28||(at&1)||at>=size)return 0;
        while(at<size&&p[at]){if(p[at]<32||p[at]>126||++n>31)return 0;at++;}
        if(!n||at==size)return 0;
        if(!o[6]){if(effects)return 0;continue;}
        if(effects<28||(effects&1)||effects>size||(size-effects)/4<o[6])return 0;
        for(i=0;i<o[6];i++){
            const u8 *e=p+effects+4*i;u16 value=sc_word(e+2);u8 j;
            if(e[0]<SC_SET_FLAG||e[0]>SC_REMOVE_GUEST||e[1]||!value||
               (e[0]<=SC_CLEAR_FLAG&&(value>FLAG_N||sc_marker(p,value))))return 0;
            for(j=0;j<i;j++){
                const u8 *q=p+effects+4*j;
                if((e[0]<=SC_CLEAR_FLAG)==(q[0]<=SC_CLEAR_FLAG)&&value==sc_word(q+2))return 0;
            }
        }
    }
    return 1;
}
// For a validated record: 0 unresolved, 1/2 stored answer, 255 contradictory markers. A resolved
// answer remains authoritative even when its former guard no longer matches.
static u8 story_choice_result(const Save *s,const u8 *p)
{
    u8 a=sc_flag(s,sc_word(sc_option(p,0))),b=sc_flag(s,sc_word(sc_option(p,1)));
    if(sc_flag(s,sc_word(p+6)))return a!=b?(u8)(a?1:2):255;
    return a||b?255:0;
}
static u8 story_choice_apply(Save *s,const u8 *p,u16 available,u8 option)
{
    const u8 *o;u8 result,i,recruits=0;u16 guard;
    if(option>=SC_OPTIONS||!story_choice_valid(p,available)||
       memcmp(s->magic,SAVE_MAGIC,4)||!save_guests_valid(s))return SC_INVALID;
    result=story_choice_result(s,p);if(result==255)return SC_INVALID;if(result)return SC_REPLAY;
    guard=sc_word(p+8);if(guard&&sc_flag(s,guard)!=p[10])return SC_GATED;
    o=sc_option(p,option);
    // Preflight every effect before changing any byte; no rollback Save copy.
    for(i=0;i<o[6];i++){
        const u8 *e=p+sc_word(o+4)+4*i;
        if(e[0]>=SC_RECRUIT_GUEST){
            if(save_guest_find(s,sc_word(e+2))==255)return SC_MISSING_GUEST;
            if(e[0]==SC_RECRUIT_GUEST)recruits++;
        }
    }
    if(recruits>PARTY_MAX-s->n)return SC_PARTY_FULL;
    for(i=0;i<o[6];i++){
        const u8 *e=p+sc_word(o+4)+4*i;u16 value=sc_word(e+2);
        if(e[0]<=SC_CLEAR_FLAG)sc_flag_set(s,value,e[0]==SC_SET_FLAG);
        else if(e[0]==SC_RECRUIT_GUEST)save_guest_recruit(s,value);
        else save_guest_remove(s,value);
    }
    sc_flag_set(s,sc_word(o),1);sc_flag_set(s,sc_word(p+6),1);
    return SC_APPLIED;
}
// The caller supplies one durable Save (existing scene_save on the native
// player). store may borrow it until teardown, like rt_save. On rejected store,
// reread the baseline instead of reserving another3918-byte transaction buffer.
// One whole-record write gives logical atomicity; this is not power-loss repair.
// load must see previous accepted stores, including a queued native write.
// A rejecting store must leave an independent reloadable baseline. The native
// rt_save adapter always accepts; its load adapter must retain scene_save while
// dirty instead of rereading the older VAT record between two choices.
static u8 story_choice_commit(Save *s,const u8 *p,u16 available,u8 option,
                              u8 (*load)(Save *),u8 (*store)(const Save *))
{
    u8 result;if(!load(s))return SC_IO;
    result=story_choice_apply(s,p,available,option);
    if(result==SC_APPLIED&&!store(s)){load(s);return SC_IO;}
    return result;
}
#endif
