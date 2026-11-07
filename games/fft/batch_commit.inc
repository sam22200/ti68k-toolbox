// Caller retains the prepared packet until this once-only commit returns.
// Runtime spell presentation/dispatch must use this entry, not single-target impact.
static u8 batch_commit(CastBatch *b)
{
    u8 i,changed=0;u16 bit;
    if(!b||b->phase!=CAST_BATCH_READY||b->actor>=NUNIT||
       (st.action.phase!=CTX_ROOT&&st.action.phase!=CTX_SECONDARY)||
       (st.action.phase==CTX_ROOT&&st.action.root!=b->actor)||
       st.action.actor!=b->actor||st.atk!=b->actor||st.abil!=b->ability)return 0;
    if(!core_action_impact(&st.action,b->actor))return 0;
    b->phase=CAST_BATCH_APPLIED;
    if(draw_out_formula(abilities[b->ability].formula)&&st.crit&&unit_home_team(&st.unit[b->actor])!=TEAM_ENEMY){
        u8 *stock=&battle_save.inv[b->ability-0x26];if(*stock)(*stock)--;
    }
    // Every formula is already frozen, including for the caster's own cell.
    for(i=0,bit=1;i<NUNIT;i++,bit<<=1)if(b->mask&bit){
        const CastVictim *v=&b->victim[i];u8 ko=!st.unit[i].hp;u16 old_mp=st.unit[i].mp,old_hp=st.unit[i].hp;
        // Chakra: MP heals half the HP amount (formula 0x34).
        st.tgt=i;st.hit=v->hit;st.dmg=v->amount;st.effect_mp=v->amount>>1;st.effect_kind=v->effect;
        if(abilities[b->ability].formula==0x20){
            u8 choice=st.dmg>>10;st.dmg&=1023;
            st.effect_mp=choice==1?0x304:choice==2?0x110:0x401;
        }
        if(v->effect==AB_REFLECT){core_action_queue(&st.action,i,REACTION_REFLECT,NUNIT);continue;}
#ifndef FFT_STATUS_BATCH
        reaction_before(i,1);
#else
        reaction_before(i,0);
#endif
        if(!st.hit||(st.effect_kind&EFFECT_KIND_MASK)==AB_NULL)continue;
        {u8 applied=action_magic_commit();changed|=applied;
         if((st.effect_kind&EFFECT_KIND_MASK)==AB_MP_DAMAGE&&(v->effect&EFFECT_KIND_MASK)!=AB_MP_DAMAGE){
            b->victim[i].effect=st.effect_kind;W->area_impact.ready&=(u16)~bit;W->area_impact.next=0;
         }
         // act_start clears push; only a sole linear victim prepares it.
         if(st.push)unit_place(&st.unit[i],st.push_cell);
         if(((v->effect&EFFECT_KIND_MASK)==AB_QUICK&&!applied)||
            (abilities[b->ability].formula==14&&old_hp==st.unit[i].hp))continue;}
        if((v->effect&EFFECT_KIND_MASK)!=AB_CATCH&&st.action.phase==CTX_IMPACT&&!(v->effect&EFFECT_RETURN)&&((v->effect&EFFECT_KIND_MASK)!=AB_MP_DAMAGE||old_mp!=st.unit[i].mp)){u8 reward=earned_exp(b->actor,i,!st.unit[i].hp&&!ko);
         if(reward>st.primary.reward)st.primary.reward=reward;}
    }
    // Existing response/reward drain and terminal checks run after this returns.
    if(W->nature_units){
#if MOD_CORE
        monster_refresh();
#else
        W->nature_pending|=1;
#endif
    }
    st.act_t=ACT_HIT;
    return 1|(changed?2:0);
}
