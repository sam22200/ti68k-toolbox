// Ordinary HP/status magic. Pure immediate effects, current teams/positions;
// source future-turn/reflection/status-response simulation is not implemented here.
#include "area_ai_reduce.inc"
#include "status_ai.h"
static u8 area_ai_supported(AbId ab)
{
    const u8 *p=cj_primary(common,ab),*raw=cj_range(common,ab);
    if(!p||ab>=512||!cast_area_supported(&abilities[ab],raw))return 0;
    if(abilities[ab].kind==AB_CLEANSE)return (p[4]==0x21||p[4]==0x22)&&
        !(p[5]&~0x55)&&(p[6]==0||p[6]==2)&&(p[7]&128)&&!(p[7]&~0xe0);
    if(abilities[ab].formula==0x31)return p[4]==0x82;   // Spin Fist: hostile HP
    if(abilities[ab].formula==14||abilities[ab].formula==18||abilities[ab].formula==20||abilities[ab].formula==97)return 1;
    if(magic_percent_formula(abilities[ab].formula))return (p[4]==0x82||p[4]==0x42)&&
        !(p[5]&~0x75)&&p[6]==2&&(p[7]&128)&&!(p[7]&~0xe0);
    // Frog both adds and cancels (0x30): it toggles. Undoing an ally Frog has no immediate utility here.
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
static void area_ai_victim(u8 t)
{
    u8 a=st.act,same=(st.unit[a].team==TEAM_ENEMY)==(st.unit[t].team==TEAM_ENEMY);
    const Unit *u=&st.unit[t];CastVictim v;s16 gain;u8 kind;
    AreaAi *q=&W->area_ai;u16 bit=(u16)1<<t;
    magic_forecast(a,t,W->ab,&v);kind=skill_kind(a,t,W->ab);
    q->hit[t]=v.hit;q->gain[t]=0;q->useful&=(u16)~bit;
    if(kind==AB_CLEANSE){
        if(v.hit){q->gain[t]=cleanse_ai_gain(t,W->ab,v.amount,same);if(q->gain[t]>0)q->useful|=bit;}
        return;
    }
    if(kind==AB_QUICK){
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
    if(magic_applicable(t,W->ab)&&!(t==a&&(cj_range(common,W->ab)[4]&1))&&
       ((!same&&effect_damage(kind))||
        (same&&(abilities[W->ab].formula!=8||!(q->flags&32))&&
         (kind==AB_REVIVE||(kind==AB_HEAL&&(u16)(u->hp*2)<u->max_hp)))))q->useful|=bit;
    if(!v.hit)return;
    if(kind==AB_MP_DAMAGE)gain=area_magic_user(t)?-(s16)((v.amount>u->mp?u->mp:v.amount)>>3):0;
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
    q->gain[t]=gain-(same&&abilities[W->ab].formula==8?1:0);
}
static u8 area_ai_step(void)
{
    u8 a=st.act,t,home=UNIT_CELL(&st.unit[a]),scans=64,queries=6;
    u8 layers=(W->surface_flags&SURFACE_GRID_UPPER)?2:1;
    AreaAi *q=&W->area_ai;
    const u8 *raw;u16 c,bit;Unit *actor=&st.unit[a];
    if(W->ai_k==4){
        if(!area_ai_supported(W->ab)||!actor->hp||st.action.phase!=CTX_IDLE||!can_use(a,W->ab))return 1;
        memset(q,0,sizeof *q);q->flags=cj_primary(common,W->ab)[7];q->origin=256;
        for(t=0;t<NUNIT;t++)area_ai_victim(t);
        if(W->ai_self)q->useful&=(u16)1<<a;
        if(!q->useful)return 1;
        // 5 builds candidate centres; 6 evaluates resumable origin/centre pairs.
        W->ai_k=5;return 0;
    }
    raw=cj_range(common,W->ab);
    if(W->ai_k==5){
        for(;q->next<MAP_N&&scans;scans--,q->next++){
            c=q->next;if(!map_tiles[c].walk)continue;
            for(t=0,bit=1;t<NUNIT;t++,bit<<=1)if(q->useful&bit){
                s16 dx=(s16)TILE_X[c]-st.unit[t].x,dz=(s16)TILE_Z[c]-st.unit[t].z;
                if((q->useful&((u16)1<<a))||raw[1]==255||(dx<0?-dx:dx)+(dz<0?-dz:dz)<=raw[1]){
                    q->open[c>>3]|=(u8)(1U<<(c&7));break;
                }
            }
        }
        if(q->next<MAP_N)return 0;
        q->next=0;W->ai_k=6;return 0;
    }
    while(W->ab_k<W->ai_no&&scans&&queries){
        u8 o=W->ai_ord[W->ab_k];unit_place(actor,o);
        if(q->origin!=o){
            q->origin=o;q->x=q->z=q->layer=0;area_ai_victim(a);
            q->self_kind=(q->useful&((u16)1<<a))?1:(q->hit[a]?2:0);
        }
        while(q->layer<layers&&scans&&queries){
            u16 mask,counted;AiAct candidate;
            // Source order is layer/row/column, independent of compact node IDs.
            c=node_at(q->x,q->z,q->layer);scans--;
            if(++q->x==MAP_W){q->x=0;if(++q->z==MAP_H){q->z=0;q->layer++;}}
            if(c>=MAP_N)continue;
            if(!(q->open[c>>3]&(1U<<(c&7)))||!ai_reaches(a,W->ab,o,(u8)c))continue;
            mask=(u16)cast_targets(a,(u8)c,W->ab,0);queries--;
            if((s16)mask<0)continue;
            if(area_ai_reduce(q,mask,NUNIT,&candidate,&counted)){
                candidate.pri+=W->ai_gain[o];
                candidate.ab=W->ab;candidate.o=o;candidate.t=(u8)c;candidate.f=0;
                if(W->ab_best.pri==AI_BAD||ai_better(candidate.pri,candidate.hit,o,&W->ab_best))W->ab_best=candidate;
            }
            // First safe origin consumes this centre; adverse self-hit centres
            // remain available until the caster can escape their footprint.
            if((q->self_kind==1&&(counted&((u16)1<<a)))||
               (q->self_kind!=1&&!(counted&((u16)1<<a))))q->open[c>>3]&=(u8)~(1U<<(c&7));
            // A whole-map footprint is identical for every valid centre at this
            // origin. Keep its first legal centre and avoid a quadratic search.
            if(raw[1]==255){q->layer=layers;break;}
        }
        if(q->layer>=layers){W->ab_k++;q->origin=256;}
    }
    unit_place(actor,home);
    if(W->ab_k<W->ai_no)return 0;
    if(W->ab_best.pri!=AI_BAD)ai_insert(&W->ab_best);
    return 1;
}
