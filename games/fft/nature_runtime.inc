#include "nature_rules.h"
#include "allegiance.inc"
// Post-impact consumer; the scalar owner supplies exactly one accepted hit.
static void nature_hit_commit(void)
{
 Unit *a=&st.unit[st.atk],*t=&st.unit[st.tgt];u8 item;
 if(st.hit&&unit_on_field(t)&&(st.abil==NO_ABIL||MONSTER_ATTACK(st.abil))&&unit_monster(t)&&
    (st.effect_kind&EFFECT_KIND_MASK)==AB_DAMAGE){
  if(!t->hp&&!(t->status_flags&STATUS_IMMORTAL)&&cj_support(common,a->job,st.support[st.atk],471)){
   const u8 *p=NATURE_DATA+((u16)(t->job-94)<<1);
   item=p[(rand15()>>7)<31];
   // Native inventory policy: direct blue-party receipt, capped99; no Fur Shop UI.
   if(unit_home_team(a)!=TEAM_ENEMY&&item&&item!=255&&battle_save.inv[item]<99){
    battle_save.inv[item]++;if(item>=240)st.bag[item-240]=battle_save.inv[item];
   }
   t->dc=DC_GONE;t->x=t->z=255;t->war_trophy=t->money_modifier=0;st.push=0;
  }else if(t->hp&&!(t->status_flags&STATUS_IMMORTAL)&&t->hp<=t->max_hp/5U&&cj_support(common,a->job,st.support[st.atk],470)&&
           !(cj_allegiance_immune(common,t->job,t->eq)&1)&&!((t->statuses.immune[1]|core_status_current(&t->statuses,1))&64)){
   t->statuses.inflicted[1]|=64;unit_allegiance(t,a->team,1);unit_status_refresh(t);
   W->ai_k=W->fc_key=0;
  }
 }
 monster_refresh();
}
