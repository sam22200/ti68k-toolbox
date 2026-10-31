// Shared magical HP/status/response commit; payment and rewards stay outside.
static u8 action_magic_commit(void)
{
    Unit *ut=&st.unit[st.tgt];u8 kind,death,ko=!ut->hp;
    kind=st.effect_kind&EFFECT_KIND_MASK;
    if(kind==AB_NULL)return 0;
    if(kind==AB_MP_DAMAGE||kind==AB_MP_HEAL){
        ut->mp=action_hp(kind==AB_MP_DAMAGE?AB_DAMAGE:AB_HEAL,ut->mp,ut->max_mp,st.dmg);return 0;
    }
    if(kind==AB_HP_MP_HEAL||kind==AB_HP_DAMAGE_MP_HEAL){
        ut->mp=action_hp(AB_HEAL,ut->mp,ut->max_mp,st.effect_mp);
        kind=kind==AB_HP_MP_HEAL?AB_HEAL:AB_DAMAGE;
    }
    if((st.effect_kind&EFFECT_OIL)&&(core_status_removable(&ut->statuses,2)&128)){
        ut->statuses.inflicted[2]&=127;status_changed();
    }
    if(kind==AB_DEATH&&!magic_heal(st.atk,st.tgt,st.abil,3))kind=AB_DAMAGE;
    death=kind==AB_DEATH;ut->hp=magic_hp(kind,ut->hp,ut->max_hp,st.dmg);
    if(death)kind=AB_DAMAGE;
    if(kind==AB_REVIVE&&ko&&ut->hp){
        ut->statuses.inflicted[0]&=0xdf;ut->dc=3;
        unit_status_refresh(ut);status_clock_update(st.tgt);status_changed();
    }
    if(!ut->hp&&!ko){ut->kos++;status_knockout(st.tgt);}
    else if(kind==AB_DAMAGE)status_damage(st.tgt);
    if(kind==AB_DAMAGE)reaction_offer(st.tgt,REACTION_POTION);
    return (u8)!ut->hp!=ko;
}
