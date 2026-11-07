// FORMULA-owned status lifetime/upkeep. No nested overlay calls.
static void status_clock_update(u8 id)
{
    const CoreStatus *s=&st.unit[id].statuses;u16 bit=(u16)1<<id;
    if(core_status_removable(s,3)|(core_status_removable(s,4)&0xfe))st.status_clock|=bit;
    else st.status_clock&=(u16)~bit;
}
static void status_changed(void)
{
    if(W){
        if(W->nature_units){
#if MOD_CORE
#ifndef FFT_MODULES
            monster_refresh();
#else
            W->nature_pending|=1;
#endif
#else
            W->nature_pending|=1;
#endif
        }
        if(st.mode==M_AREA)W->area_ui.dirty=1;
        W->fc_key=0;W->status_i=0;W->frz=0;W->frz_h=0;
        W->win_k[0]=0;W->from_who=0;
        if(st.ai==AI_THINK)W->ai_k=0;
    }
}
#include "cleanse_rules.h"
// Decode the prepared subset; the source bitmap/timer primitive preserves innate
// effects. No overlay call or HP/reaction hook is involved in cancellation.
static u8 status_cleanse(u8 t,AbId ab,u16 packed)
{
    Unit *u=&st.unit[t];u8 remove[5],b,changed=0;
    const u8 *r=cj_section(common,CJ_STATUS)+6*abilities[ab].y;
    core_cleanse_unpack(r+1,packed,remove);
    for(b=0;b<5;b++){remove[b]&=core_status_removable(&u->statuses,b);changed|=remove[b];}
    core_status_remove(&u->statuses,remove,u->status_ct);
    unit_status_refresh(u);status_clock_update(t);if(changed)status_changed();
    return changed!=0;
}
// Successful HP-damage-type impacts wake mutable Sleep and clear mutable
// Confusion, even at zero damage. Misses, healing, Poison upkeep and self-cost
// never call this primary hook.
static void status_damage(u8 id)
{
    Unit *u=&st.unit[id];
    u8 sleep=core_status_removable(&u->statuses,4)&0x30,confused=core_status_removable(&u->statuses,1)&0x10;
    u8 hidden=core_status_removable(&u->statuses,2)&16;
    if(!(sleep|confused|hidden))return;
    u->statuses.inflicted[2]&=(u8)~hidden;
    u->statuses.inflicted[1]&=(u8)~confused;
    if(sleep){
        u->statuses.inflicted[4]&=(u8)~sleep;
        if(sleep&0x10)u->status_ct[SID_SLEEP-SID_POISON]=0;
        if(sleep&0x20)u->status_ct[SID_CHARM-SID_POISON]=0;
    }
    unit_status_refresh(u);status_clock_update(id);status_changed();
}
// Source finalize: a Brave change clamps to 0..100 (a rider's floor 10 is not
// modelled: no mounts), then Chicken follows Brave below 10. delta is signed.
static u8 status_brave(u8 id,u8 delta)
{
    Unit *u=&st.unit[id];s16 v=(s16)u->brave+(s8)delta;u8 old=u->brave;CoreStatusDelta d;
    u->brave=(u8)(v<0?0:v>100?100:v);
    memset(&d,0,sizeof d);if(u->brave<10)d.add[2]=4;else d.remove[2]=4;
    core_status_apply(&u->statuses,&d,u->status_flags,0,cj_status_rules(common),u->status_ct);
    unit_status_refresh(u);status_clock_update(id);status_changed();
    return old!=u->brave;
}
static void status_tick(void)
{
    u8 id;u16 bit=1;u8 remove[5];
    for(id=0;id<NUNIT;id++,bit<<=1)if(st.status_clock&bit){
        Unit *u=&st.unit[id];
        if(u->x>=MAP_W||u->z>=MAP_H||UNIT_CELL(u)>=MAP_N||
           (core_status_current(&u->statuses,0)&4))continue;
        core_status_tick(&u->statuses,u->status_ct,remove);
        if(remove[3]|remove[4]){
            core_status_remove(&u->statuses,remove,u->status_ct);
            unit_status_refresh(u);status_changed();
        }
        status_clock_update(id);
    }
}
// HP-based KO stays the battle's existing lifecycle. Apply its source Dead
// cancellation mask to mutable effects without claiming other Dead handlers.
static void status_knockout(u8 id)
{
    Unit *u=&st.unit[id];u8 remove[5],i;
    const u8 *cancel=cj_status_rules(common)+(SID_DEAD<<4)+6;
    for(i=0;i<5;i++)remove[i]=cancel[i]&core_status_removable(&u->statuses,i);
    core_status_remove(&u->statuses,remove,u->status_ct);
    st.pending[id].state=DEFER_NONE;u->dc=3;
    unit_status_refresh(u);status_clock_update(id);
}
static u8 status_upkeep(u8 id)
{
    Unit *u=&st.unit[id];u8 kind;u16 hp=u->hp;
    if(!hp||u->x>=MAP_W||u->z>=MAP_H||UNIT_CELL(u)>=MAP_N||DC_OBJ(u->dc))return 0;
    // Source upkeep: Death Sentence counts this unit's turns (timer slot 15). At 0
    // it is removed; a non-Undead unit dies and skips the rest of the upkeep.
    if((core_status_current(&u->statuses,4)&1)&&!--u->status_ct[15]){
        static const u8 sentence[5]={0,0,0,0,1};
        core_status_remove(&u->statuses,sentence,u->status_ct);
        if(!(core_status_current(&u->statuses,0)&16)){u->hp=0;u->kos++;status_knockout(id);status_changed();return 1;}
        unit_status_refresh(u);status_clock_update(id);status_changed();
    }
    if(u->brave<10)status_brave(id,1);    // source upkeep: Brave +1 below 10
    kind=core_status_upkeep_kind(&u->statuses,1);
    if(kind==STATUS_UPKEEP_DAMAGE)u->hp=action_hp(AB_DAMAGE,hp,u->max_hp,u->max_hp>>3);
    else if(kind==STATUS_UPKEEP_HEAL)u->hp=action_hp(AB_HEAL,hp,u->max_hp,u->max_hp>>3);
    if(hp==u->hp)return 0;
    status_changed();
    if(!u->hp){u->kos++;status_knockout(id);return 1;}
    return 0;
}

// A KO actor's own CT advances its counter; Undead may revive only at expiry.
// Ordinary pilot RNG is retained; Undead consumes the source upkeep roll per pick.
static u8 status_dead_turn(u8 id)
{
    Unit *u=&st.unit[id];u8 undead=core_status_current(&u->statuses,0)&16;u16 roll;
    if(u->team==TEAM_GUEST||(u->status_flags&STATUS_IMMORTAL))return 0;
    // Source upkeep (one draw): Reraise (not Undead) revives with max HP/10 instead
    // of a counter tick; at 0 HP it is spent and the unit stays KO, counter kept.
    if(!undead&&(core_status_current(&u->statuses,2)&0x20)){
        rand15();u->hp=(u16)divs32_16(u->max_hp,10);u->statuses.inflicted[2]&=0xdf;
        if(u->hp){u->statuses.inflicted[0]&=0xdf;u->dc=3;}
        unit_status_refresh(u);status_clock_update(id);status_changed();return u->hp!=0;
    }
    if(u->dc){u->dc--;if(undead)rand15();return 0;}
    roll=rand15();
    if(undead&&(roll&1)&&!(u->statuses.innate[0]&32)){
        u->hp=(u16)((muls16((s16)rand15(),(s16)u->max_hp)>>15)+1);
        u->statuses.inflicted[0]&=0xdf;u->dc=3;
        unit_status_refresh(u);status_clock_update(id);status_changed();return 1;
    }
    u->dc=roll&(u->team==TEAM_PLAYER?0x110:0x10)?DC_CRYSTAL:DC_CHEST;
    st.morph_mask|=(u16)1<<id;return 0;
}

// Reuse an unsaved spare packed flag: source turn-entry arms removal, while a
// newly inflicted Transparent clears it. No per-unit stride or save growth.
static void status_transparent_remove(u8 id)
{
    Unit *u=&st.unit[id];
    if(!(core_status_removable(&u->statuses,2)&16))return;
    u->statuses.inflicted[2]&=(u8)~16;unit_status_refresh(u);status_changed();
}
static void status_turn_start(u8 id)
{
    Unit *u=&st.unit[id];
    if(unit_transparent(u))u->allegiance|=UNIT_TRANSPARENT_ARMED;
    else u->allegiance&=(u8)~UNIT_TRANSPARENT_ARMED;
    u->statuses.innate[0]&=(u8)~2;u->statuses.inflicted[0]&=(u8)~2;u->status&=(u8)~ST_DEFEND;
}
static void status_turn_action(u8 id)
{
    if(st.unit[id].allegiance&UNIT_TRANSPARENT_ARMED)status_transparent_remove(id);
}
