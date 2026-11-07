#include "combat_rules.h"
// Numeric responses own frozen amounts and never spend MP, stock or turn CT.
static void combat_response_launch(void)
{
    CoreAction *c=&st.action;u8 a=c->actor,i,count=0;u16 id=reaction_id(c->reaction[a]),mask,n;
    if(c->phase!=CTX_SECONDARY||st.primary.running)return;
    st.primary.running=1;
    action_pose_start(a,id==REACTION_DISTRIBUTE?a:c->root);st.abil=id;
    st.effect_kind=AB_DAMAGE;st.dmg=id==REACTION_MEATBONE?st.unit[a].max_hp:c->reaction[a]&1023;
    if(id!=REACTION_DISTRIBUTE){st.hit=magic_target(c->root)==MAGIC_VALID;return;}
    mask=distribute_targets(a);
    for(i=0;i<NUNIT;i++)if(mask&((u16)1<<i))count++;
    n=count?(st.dmg+count-1)/count:0;
    {CastBatch *b=&W->cast;memset(b,0,sizeof *b);b->actor=a;b->ability=id;b->ref=UNIT_CELL(&st.unit[a]);b->mask=mask;
     for(i=0;i<NUNIT;i++)if(mask&((u16)1<<i)){
        b->victim[i].amount=n;b->victim[i].effect=AB_HEAL;b->victim[i].hit=magic_target(i)==MAGIC_VALID;
     }
     b->phase=CAST_BATCH_READY;W->area_impact.centre=b->ref;W->area_impact.ready=0;W->area_impact.next=0;W->area_impact.oil_ready=0;}
}
