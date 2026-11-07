#include "combat_rules.h"
// Reuse the completed primary cast owner: its scalar packet is suspended, and
// root FX never resume during DRAIN. Freeze every secondary victim before HP.
static void reaction_area_launch(u8 counter_magic)
{
    CoreAction *c=&st.action;CastBatch *b=&W->cast;u8 a=c->actor,i;u16 mask;AbId ab;
    if(c->phase!=CTX_SECONDARY||st.primary.running)return;
    ab=counter_magic?(c->reaction[a]&0x1ff):magic_heal(a,c->root,126,4);
    if(counter_magic){if(!counter_magic_ready(a,ab))return;st.unit[a].mp-=counter_magic_cost(a,ab);}
    st.primary.running=1;action_pose_start(a,c->root);st.abil=ab;
    memset(b,0,sizeof *b);b->actor=a;b->ability=ab;b->ref=UNIT_CELL(&st.unit[c->root]);
    mask=(u16)cast_targets(a,b->ref,ab,0);if((s16)mask<0)mask=0;b->mask=mask;
    for(i=0;i<NUNIT;i++)if(mask&((u16)1<<i))magic_prepare(a,i,ab,&b->victim[i]);
    b->phase=CAST_BATCH_READY;W->area_impact.centre=b->ref;W->area_impact.ready=0;W->area_impact.next=0;W->area_impact.oil_ready=0;
    W->hold=W->frz=W->frz_h=0;W->fc_key=0;
}

#define reaction_flood_launch() reaction_area_launch(0)
