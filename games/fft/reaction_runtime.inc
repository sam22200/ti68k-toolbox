#include "reaction_policy_runtime.inc"
// Every saved field belongs to presentation/effect payload; CT, turn history,
// inventory, pending casts and live unit positions are never rolled back.
#define PACKET_FIELDS(X) X(hand) X(effect_kind) X(atk) X(tgt) X(act_t) X(hit) X(crit) X(push) X(kx) X(kz) X(push_cell) X(dmg) X(self) X(abil) X(gain) X(lvup) X(jpg)
static void reaction_save(void)
{
    ActionPacket *p=&st.primary;
#define SAVE_FIELD(n) p->n=st.n;
    PACKET_FIELDS(SAVE_FIELD)
#undef SAVE_FIELD
    p->running=0;
}
static void reaction_return(void)
{
    ActionPacket *p=&st.primary;
    if(st.action.phase!=CTX_SECONDARY&&st.action.phase!=CTX_APPLIED)return;
#define RESTORE_FIELD(n) st.n=p->n;
    PACKET_FIELDS(RESTORE_FIELD)
#undef RESTORE_FIELD
    p->running=0;core_action_return(&st.action);
    if(W){W->num_ok=W->hold=W->frz=W->frz_h=0;W->fc_key=0;}
}
#undef PACKET_FIELDS
#include "reflect_launch.inc"
// Selection consumes invalid requests, revalidates range/stock after the root's
// damage and knockback, and returns before resident projectile validation runs.
static u8 reaction_select(void)
{
    CoreAction *c=&st.action;u8 who,kind;u16 id;
    while((who=core_action_take(c,0x1fff))!=255){
        id=c->reaction[who];kind=0;
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
        }
        if(!kind){core_action_return(c);continue;}
        reaction_save();return kind;
    }
    return 0;
}
static void reaction_launch(u8 kind)
{
    CoreAction *c=&st.action;u8 who=c->actor,potion;
    if(c->phase!=CTX_SECONDARY||st.primary.running||kind<1||kind>3)return;
    if(kind==3){reflect_launch();return;}
    st.primary.running=1;
    action_pose_start(who,kind==1?c->root:who);
    if(W){W->hold=W->frz=W->frz_h=0;W->fc_key=0;}
    if(kind==1){c->left=hand_count(who)-1;c->cell=UNIT_CELL(&st.unit[c->root]);st.abil=NO_ABIL;attack_roll(who,c->root);}
    else{
        potion=reaction_potion(st.bag,st.unit[who].team==TEAM_ENEMY);
        st.abil=(AbId)(0x170+potion);restore_prepare(who,st.abil);
        if(st.unit[who].team!=TEAM_ENEMY){st.bag[potion]--;W->win_k[0]=0;}
    }
}
static void reaction_rewards(void)
{
    u8 reward=st.primary.reward;st.primary.reward=0;
    if(!reward)return;
    award_rewards(st.action.root,reward);
    if(st.gain)st.act_t=ACT_HIT+GAIN_T-1;
}
