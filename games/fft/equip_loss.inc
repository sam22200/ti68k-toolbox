// An equipment break's loss (CORE_EQUIP_BREAKS.md): COMMAND's slot (st.dmg) emptied and the
// item's bonuses taken off the target's single stat values; a lost weapon ends its Charge.
// Part of ACTION's commit (action_resolve.inc), before the rewards.
static void equip_loss(void)
{
    Unit *u=&st.unit[st.tgt];u8 s=(u8)st.dmg;const Item *it;
    if(s>=EQ_N||u->eq[s]==255)return;
    it=&items[u->eq[s]];u->eq[s]=255;
    u->max_hp-=it->hp;if(u->hp>u->max_hp)u->hp=u->max_hp;
    u->max_mp-=it->mp;if(u->mp>u->max_mp)u->mp=u->max_mp;
    u->pa=u->pa>it->pa?u->pa-it->pa:1;u->ma=u->ma>it->ma?u->ma-it->ma:1;
    u->sp=u->sp>it->sp?u->sp-it->sp:1;u->move-=it->move;u->jump-=it->jump;
    if(it->wp&&st.pending[st.tgt].state==DEFER_CHARGING){st.pending[st.tgt].state=DEFER_NONE;unit_status_action(u,0);}
}
