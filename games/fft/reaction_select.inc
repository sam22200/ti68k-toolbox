#include "combat_rules.h"
// Request selection and primary packet suspension. No image loads.
#define PACKET_FIELDS(X) X(hand) X(effect_kind) X(atk) X(tgt) X(act_t) X(hit) X(crit) X(push) X(kx) X(kz) X(push_cell) X(dmg) X(self) X(abil) X(gain) X(lvup) X(jpg)
static void reaction_save(void)
{
    ActionPacket *p=&st.primary;
#define SAVE_FIELD(n) p->n=st.n;
    PACKET_FIELDS(SAVE_FIELD)
#undef SAVE_FIELD
    p->running=0;
}
#undef PACKET_FIELDS
// Selection consumes invalid requests, revalidates range/stock after the root's
// damage and knockback, and returns before resident projectile validation runs.
static u8 reaction_select(void)
{
    CoreAction *c=&st.action;u8 who,kind;u16 id;
    while((who=core_action_take(c,0x1fff))!=255){
        id=reaction_id(c->reaction[who]);kind=0;
        // Retail stages self requests but skips the acting slot during drain.
        if(who==c->root&&(id==REACTION_SPLIT||id==REACTION_DISTRIBUTE||id==REACTION_COUNTER_MAGIC)){core_action_return(c);continue;}
        // Reflect bypasses normal Brave/reaction-incapacity checks, but the
        // source action restriction (Don't Act/deep water) still applies.
        if(id==REACTION_REFLECT&&unit_on_field(&st.unit[who])&&
           !(core_status_current(&st.unit[who].statuses,4)&4)){
            const Unit *u=&st.unit[who];u8 restricted=0;
            if(W->surface){
                const u8 *p=W->surface+((u16)(W->grid_row[u->z]+u->x)<<3)+4*node_layer(UNIT_CELL(u));
                restricted=(p[1]>>5)>=2&&!(core_status_current(&u->statuses,2)&0x46)&&!(u->status_flags&STATUS_RIDER);
            }
            if(!restricted)kind=3;
        }
        if(reaction_live(who)){
            if(id==REACTION_COUNTER&&st.unit[c->root].hp&&
               ranged_reach(who,UNIT_CELL(&st.unit[who]),UNIT_CELL(&st.unit[c->root])))kind=1;
            if(id==REACTION_POTION&&reaction_potion(st.bag,st.unit[who].team==TEAM_ENEMY)!=255)kind=2;
            if(reaction_stat(id))kind=4;
            if(reaction_resource(id)&&id!=REACTION_MEATBONE)kind=5;
            if(id==REACTION_TACKLE&&st.unit[c->root].hp&&unit_on_field(&st.unit[c->root])&&!unit_airborne(&st.unit[c->root])){
                const Tile *a=&map_tiles[UNIT_CELL(&st.unit[who])],*t=&map_tiles[UNIT_CELL(&st.unit[c->root])];
                s16 dx=(s16)st.unit[who].x-st.unit[c->root].x,dz=(s16)st.unit[who].z-st.unit[c->root].z,h=(s16)t->stand-a->stand;
                if((dx<0?-dx:dx)+(dz<0?-dz:dz)==1&&h>=-2&&h<=2)kind=8;
            }
            if(id==REACTION_FLOOD&&unit_on_field(&st.unit[c->root])&&!unit_airborne(&st.unit[c->root])&&
               (u16)(magic_heal(who,c->root,126,4)-126)<12U)kind=7;
            if(reaction_self(id))kind=6;
            if(id==REACTION_MEATBONE&&st.unit[c->root].hp&&ranged_reach(who,UNIT_CELL(&st.unit[who]),UNIT_CELL(&st.unit[c->root])))kind=9;
            if(id==REACTION_SPLIT&&unit_on_field(&st.unit[c->root]))kind=9;
            if(id==REACTION_DISTRIBUTE&&distribute_targets(who))kind=9;
            if(id==REACTION_COUNTER_MAGIC&&unit_on_field(&st.unit[c->root])&&counter_magic_ready(who,c->reaction[who]&0x1ff))kind=10;
        }
        if(!kind){core_action_return(c);continue;}
        reaction_save();return kind;
    }
    return 0;
}
