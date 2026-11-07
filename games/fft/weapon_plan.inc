// Cold bounded weapon utility, prepared with the existing scalar cache quanta.
#include "ai_worth.h"
#include "status_policy.inc"
static s16 weapon_status_worth(u8 sid)
{
    if(sid==SID_PETRIFY)return -96;
    if(sid==SID_DARK)return -64;
    if(sid==SID_CHARM)return -96;
    if(sid==SID_FROG||sid==SID_SLEEP||sid==SID_STOP)return -48;
    if(sid==SID_HASTE)return 48;
    if(sid==SID_SLOW)return -48;
    if(sid==SID_POISON)return -26;
    if(sid==SID_PROTECT||sid==SID_SHELL||sid==SID_REFLECT||sid==SID_REGEN)return 24;
    if(sid==SID_FAITH||sid==SID_INNOCENT)return 0; // depends on the future spell mix
    return -24;
}
static s16 weapon_status_bonus(u8 a,u8 t,const u8 *w)
{
    const Unit *u=&st.unit[t];const u8 *r=cj_section(common,CJ_STATUS)+(u16)w[7]*6;
    CoreStatusDelta d;u8 sid,count=0; s16 gain=0;
    if(!w[7])return 0;
    memset(&d,0,sizeof d);
    if(r[0]&16)memcpy(d.remove,r+1,5);
    else for(sid=0;sid<40;sid++)if(r[1+(sid>>3)]&(128>>(sid&7))){
        count++;d.add[sid>>3]|=128>>(sid&7);
        if(!(r[0]&64))break; // ordinary weapon packets select the first status
    }
    if(cj_allegiance_immune(common,u->job,u->eq)&2)d.add[4]&=(u8)~32;
    core_status_filter(&u->statuses,&d,u->status_flags,0,cj_status_rules(common));
    for(sid=0;sid<40;sid++){
        u8 b=sid>>3,m=128>>(sid&7);
        if((d.add[b]&m)&&!(core_status_current(&u->statuses,b)&m))gain+=weapon_status_worth(sid);
        if(d.remove[b]&m)gain-=weapon_status_worth(sid);
    }
    if((r[0]&64)&&count)gain/=count;
    return (u->team==TEAM_ENEMY)==(st.unit[a].team==TEAM_ENEMY)?gain:-gain;
}
static s16 weapon_proc_bonus_mode(u8 a,u8 t,const u8 *w,u16 hp,u8 returned)
{
    AbId ab=w[7];const Unit *u=&st.unit[t];u8 kind,hit;u16 amount; s16 gain;
    if(!ab||!hp||magic_target(t)!=MAGIC_VALID||(!returned&&magic_reflects_from(a,t,ab)))return 0;
    kind=skill_kind(a,t,ab);if(kind==AB_NULL)return 0;
    hit=magic_hit(a,t,ab,returned?2:0);if(!hit)return 0;
    amount=magic_amount(a,t,((u16)abilities[ab].element<<8)|abilities[ab].y);
    gain=worth(u,magic_hp(kind,hp,u->max_hp,amount),u->status)-worth(u,hp,u->status);
    if((u->team==TEAM_ENEMY)!=(st.unit[a].team==TEAM_ENEMY))gain=-gain;
    return (s16)divs32_16(muls16(gain,hit),100);
}
static s16 weapon_proc_bonus(u8 a,u8 t,const u8 *w,u16 hp)
{return weapon_proc_bonus_mode(a,t,w,hp,0);}
static s16 weapon_plan_bonus(u8 a,u8 t,AbId ab)
{
    const Unit *u=&st.unit[t],*actor=&st.unit[a];u16 hp=u->hp,self=actor->hp;u8 hand,awake=0; s16 gain=0;
    if(magic_target(t)!=MAGIC_VALID)return 0;
    for(hand=0;hand<hand_count(a)&&hp;hand++){
        const u8 *w=cj_weapon(common,hand_weapon(actor,hand));u8 kind=weapon_kind_hand(a,t,hand);u16 amount;
        if(kind==AB_NULL)continue;
        if(kind==AB_DAMAGE&&st.golem[u->team&1])continue;
        amount=weapon_forecast(a,t,ab,hand,awake);
        hp=magic_hp(kind,hp,u->max_hp,amount);
        if(w[2]==7)self=action_hp(kind==AB_HEAL?AB_DAMAGE:AB_HEAL,self,actor->max_hp,amount);
        if(!self)break;
        if(hp&&(w[2]==1||w[2]==2)){
            s16 proc=w[2]==1?weapon_status_bonus(a,t,w):weapon_proc_bonus(a,t,w,hp);
            gain+=(s16)divs32_16(muls16(proc,19),100);
        }
        if(kind==AB_DAMAGE)awake=1;
    }
    return gain+worth(actor,self,actor->status)-worth(actor,actor->hp,actor->status);
}
static u8 weapon_ai_prepare(void)
{
    u8 row=0,t=W->ai_weapon_at,left=4,a=st.act;u16 bit=1;AbId ab;
    if(!W->ai_weapon_left)return 1;
    while(!(W->ai_weapon_left&bit)){bit<<=1;row++;}
    ab=row?0x195+row:NO_ABIL;
    if(!row&&!t){
        u8 hand;W->ai_reflect_hands=0;
        for(hand=0;hand<hand_count(a);hand++){
            const u8 *w=cj_weapon(common,hand_weapon(&st.unit[a],hand));
            if(w[2]==2&&w[7]&&(cj_range(common,w[7])[5]&128))W->ai_reflect_hands|=1<<hand;
        }
    }
    do{
        if(!row){
            u8 hand;for(hand=0;hand<2;hand++){
                const u8 *w=cj_weapon(common,hand_weapon(&st.unit[a],hand));s16 value=0;
                if(hand<hand_count(a)&&w[2]==2)value=weapon_proc_bonus_mode(a,t,w,st.unit[t].hp,1);
                W->ai_proc_gain[hand][t]=(s16)divs32_16(muls16(value,19),100);
            }
        }
        W->ai_weapon_kind[t]=weapon_kind(a,t,NO_ABIL);
        W->ai_weapon_value[row][t]=st.unit[t].hp?weapon_total(a,t,ab):0;
        W->ai_weapon_bonus[row][t]=weapon_plan_bonus(a,t,ab);
        if(++t==NUNIT){W->ai_weapon_left&=(u16)~bit;t=0;break;}
    }while(--left);
    W->ai_weapon_at=t;return W->ai_weapon_left==0;
}
