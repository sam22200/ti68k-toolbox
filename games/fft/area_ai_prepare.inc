// Cold victim caches and Math, separate from origin/centre enumeration.
#include "area_ai_reduce.inc"
#include "status_ai.h"
static u8 area_ai_supported(AbId ab)
{
    const u8 *p=cj_primary(common,ab),*raw=cj_range(common,ab);
    if(!p||ab>=512||!cast_area_supported(&abilities[ab],raw))return 0;
    if(ab==109||ab==116)return 1;
    if(performance_formula(abilities[ab].formula))return 1;
    if(draw_out_formula(abilities[ab].formula))return p[4]==0x82||p[4]==0x42||p[4]==0x81||p[4]==0x11;
    if(abilities[ab].kind==AB_STEAL_GIL)return 0;   // conservative economy policy, as for equipment steals
    if(abilities[ab].kind==AB_CLEANSE)return (p[4]==0x21||p[4]==0x22)&&
        !(p[5]&~0x55)&&(p[6]==0||p[6]==2)&&(p[7]&128)&&!(p[7]&~0xe0);
    if(abilities[ab].formula==0x31||abilities[ab].formula==0x4F)return p[4]==0x82;   // Spin Fist, Goblin Punch: hostile HP
    if((abilities[ab].formula|2)==0x2F)return p[4]==0x82||p[4]==0x42;   // Holy Sword: hostile HP; Dark Sword: MP
    if((u8)(abilities[ab].formula-0x1A)<=1||abilities[ab].formula==0x55)return 1;   // Ruins, Beaking: stat losses as the Dances, MP damage
    if(abilities[ab].formula==0x24)return p[4]==0x82;   // Geomancy: immediate HP, conditional status unscored
    if(abilities[ab].formula==0x50)return p[4]==0x12&&!(p[5]&~2)&&(p[6]==1||((raw[6]&32)&&(p[6]&~0x10)==0x41))&&(p[7]|1)==0x81;   // Secret Fist's status utility; monster strikes (AI byte 5: 2); direct targeting (AI byte 6: 0x40)
    if(abilities[ab].formula==14||abilities[ab].formula==18||abilities[ab].formula==20||(u8)(abilities[ab].formula-97)<=1)return 1;
    if(magic_percent_formula(abilities[ab].formula))return (p[4]==0x82||p[4]==0x42)&&
        !(p[5]&~0x75)&&p[6]==2&&(p[7]&128)&&!(p[7]&~0xe0);
    // Frog toggles; status_ai_gain handles mutable removal and innate protection.
    if(magic_status_formula(abilities[ab].formula))return (p[4]==0x10||p[4]==0x11||p[4]==0x12||p[4]==0x30||p[4]==0x91)&&
        !(p[5]&~0x55)&&(p[6]==0||p[6]==2)&&(p[7]&128)&&!(p[7]&~0xe0);
    if(abilities[ab].formula!=8)return p[4]==(abilities[ab].formula==12?0x81:abilities[ab].formula==0x34?0xc1:0x21)&&
        !(p[5]&~0x75)&&!p[6]&&(p[7]&0x80)&&!(p[7]&~0xe0);
    return p[4]==0x82&&!(p[5]&~0x55)&&p[6]==2&&(p[7]&~0x20)==0x80;
}
// MP denial matters for a caster; keep its utility below immediate HP/KO values.
static u8 area_magic_user(u8 t)
{
    const Unit *u=&st.unit[t];u8 k;
    for(k=0;k<SET_N;k++)if((u->learned&((LearnBits)1<<k))&&
        (abilities[SET_ABILITY(u->set,k)].flags&AF_CAST))return 1;
    return 0;
}
static void area_ai_victim_mode(u8 t,u8 returned)
{
    u8 a=st.act,same=(st.unit[a].team==TEAM_ENEMY)==(st.unit[t].team==TEAM_ENEMY);
    const Unit *u=&st.unit[t];CastVictim v;s16 gain;u8 kind;
    AreaAi *q=&W->area_ai;u16 bit=(u16)1<<t;
    magic_forecast_mode(a,t,W->ab,&v,returned);kind=skill_kind(a,t,W->ab);
    q->hit[t]=v.hit;q->gain[t]=0;q->useful&=(u16)~bit;
    if(unit_hidden_enemy(&st.unit[a],u))return;
    if(W->ab==109||W->ab==116){
        if(v.hit&&!same){q->gain[t]=64;q->useful|=bit;}
        return;
    }
    if(kind==AB_CLEANSE){
        if(v.hit){q->gain[t]=cleanse_ai_gain(t,W->ab,v.amount,same);if(q->gain[t]>0)q->useful|=bit;}
        return;
    }
    if(stat_loss_formula(abilities[W->ab].formula)&&(kind==AB_SPEED||kind==AB_BREAK)){
        u8 value=kind==AB_SPEED?u->sp:stat_loss_pa(W->ab)?u->pa:u->ma,max=kind==AB_SPEED?50:99;
        if(v.hit&&((same&&(s8)v.amount<0&&value<max)||(!same&&(s8)v.amount>0&&value>1))){q->gain[t]=12;q->useful|=bit;}
        return;
    }
    if(kind==AB_QUICK){
        if(W->ab==99){if(v.hit&&!same&&u->ct){q->gain[t]=u->ct>>1;q->useful|=bit;}return;}
        if(v.hit&&same&&t!=a&&u->ct<100){q->gain[t]=(100-u->ct)>>1;q->useful|=bit;}
        return;
    }
    if(kind==AB_GOLEM){
        if(v.hit&&same&&!st.golem[u->team&1]){q->gain[t]=48;q->useful|=bit;}
        return;
    }
    if(kind==AB_BRAVE){   // immediate value: only a hostile drop below Chicken's 10
        if(v.hit&&!same&&u->brave>=10&&u->brave<10+v.amount){q->gain[t]=24;q->useful|=bit;}
        return;
    }
    if(kind==AB_STATUS){
        if(v.hit){q->gain[t]=status_ai_gain(t,v.amount,same);if(q->gain[t]>0)q->useful|=bit;}
        return;
    }
    if(kind==AB_HP_MP_HEAL)kind=AB_HEAL;   // Chakra: immediate HP value only
    if(magic_applicable(t,W->ab)&&!(!returned&&t==a&&(cj_range(common,W->ab)[4]&1))&&
       ((!same&&(effect_damage(kind)||((abilities[W->ab].formula==0x21||performance_formula(abilities[W->ab].formula))&&area_magic_user(t))))||
        (same&&(abilities[W->ab].formula!=8||!(q->flags&32))&&
         (kind==AB_REVIVE||(kind==AB_MP_HEAL&&area_magic_user(t)&&u->mp<u->max_mp)||(kind==AB_HEAL&&(u16)(u->hp*2)<u->max_hp)))))q->useful|=bit;
    if(!v.hit)return;
    if(kind==AB_MP_HEAL)gain=(s16)((v.amount>u->max_mp-u->mp?u->max_mp-u->mp:v.amount)>>3);
    else if(kind==AB_MP_DAMAGE)gain=area_magic_user(t)?-(s16)((v.amount>u->mp?u->mp:v.amount)>>3):0;
    else gain=worth(u,magic_hp(kind,u->hp,u->max_hp,v.amount),u->status)-worth(u,u->hp,u->status);
    if(!same)gain=-gain;
    // Original hostile-HP hit accounting penalizes an allied target, even absorption.
    if(magic_transfer_formula(abilities[W->ab].formula)){
        const Unit *caster=&st.unit[a];s16 returned;
        if(kind==AB_MP_DAMAGE){u16 space=caster->max_mp-caster->mp;returned=(s16)((v.amount>space?space:v.amount)>>3);}
        else returned=worth(caster,magic_hp(kind==AB_DAMAGE?AB_HEAL:AB_DAMAGE,caster->hp,caster->max_hp,v.amount),caster->status)-worth(caster,caster->hp,caster->status);
        gain+=returned;
        if(!same&&gain>0)q->useful|=bit;
    }
    q->gain[t]=gain-(same&&magic_damage_formula(abilities[W->ab].formula)?1:0);
}
static void area_ai_victim(u8 t){area_ai_victim_mode(t,0);}
#include "math_ai.inc"
static u8 area_ai_prepare(void)
{
    u8 a=st.act,t,queries;u16 bit;AreaAi *q=&W->area_ai;const Unit *actor=&st.unit[a];
    if(actor->set==21)return math_ai_step();
    if(W->ai_k==4){
        if(st.pending[a].state==DEFER_PERFORMING||!area_ai_supported(W->ab)||!actor->hp||st.action.phase!=CTX_IDLE||
           (abilities[W->ab].formula!=0x24&&!can_use(a,W->ab)))return 1;
        memset(q,0,sizeof *q);q->flags=cj_primary(common,W->ab)[7];q->origin=256;
        W->ai_k=8;return 0;
    }
    if(W->ai_k==8){
        // Freeze both ordinary and already-reflected values incrementally.
        // Only routing changes with an origin; stats/status utility stays cached.
        queries=2;
        while(q->next<NUNIT&&queries--){
            t=(u8)q->next++;bit=(u16)1<<t;
            area_ai_victim_mode(t,1);q->returned_gain[t]=q->gain[t];q->returned_hit[t]=q->hit[t];
            if(q->useful&bit)q->returned_useful|=bit;
            area_ai_victim(t);
            if(magic_applicable(t,W->ab)&&magic_reflects_from(a,t,W->ab))q->reflect|=bit;
        }
        if(q->next<NUNIT)return 0;q->next=0;q->useful|=q->reflect;
        if(W->ai_self)q->useful&=(u16)1<<a;
        if(!q->useful)return 1;
        // 5 builds candidate centres; 6 evaluates resumable origin/centre pairs.
        W->ai_k=5;return 0;
    }
    return 0;
}
