#include "reaction_live.h"
// The first strike replaces the unexecuted primary command. The high cursor
// bit marks this pre-action context; no second primary impact/cost is permitted.
static u8 hamedo_begin(u8 a,u8 t,AbId ab)
{
    CoreAction *c=&st.action;const u8 *raw;ActionPacket *p=&st.primary;
    if(a==t||st.reaction[t]!=REACTION_HAMEDO)return 0;
    if(ab!=NO_ABIL){
        raw=cj_range(common,ab>=512?0:ab);
        if((abilities[ab].flags&(AF_CAST|AF_AREA))||!(abilities[ab].flags&AF_CHARGE)&&abilities[ab].formula!=99&&!(raw&&raw[3]&32))return 0;
    }
    // Retail draws Brave before checking incapacity and the return's geometry.
    if(!reaction_chance(st.unit[t].brave,rand15())||!reaction_live(t)||
       !ranged_reach(t,UNIT_CELL(&st.unit[t]),UNIT_CELL(&st.unit[a])))return 0;
    if(WEAPON_REACH(&st.unit[t])>1){
#ifdef FFT_MODULE
        if(!W->ai_traj)return 0; // prepared by resident code before this image runs
#else
        if(!battle_line_clear(t,a))return 0;
#endif
    }
    act_start(a,t);st.abil=ab;st.hit=0;
#define SAVE_PRE(n) p->n=st.n;
    SAVE_PRE(hand) SAVE_PRE(effect_kind) SAVE_PRE(atk) SAVE_PRE(tgt) SAVE_PRE(act_t)
    SAVE_PRE(hit) SAVE_PRE(crit) SAVE_PRE(push) SAVE_PRE(kx) SAVE_PRE(kz)
    SAVE_PRE(push_cell) SAVE_PRE(dmg) SAVE_PRE(self) SAVE_PRE(abil)
    SAVE_PRE(gain) SAVE_PRE(lvup) SAVE_PRE(jpg)
#undef SAVE_PRE
    p->running=1;c->actor=t;c->phase=CTX_SECONDARY;c->cursor=128;
    c->left=hand_count(t)-1;c->cell=UNIT_CELL(&st.unit[a]);
    action_pose_start(t,a);st.abil=NO_ABIL;attack_roll(t,a);return 1;
}
