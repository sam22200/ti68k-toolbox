// Cold entry only, after fixed guests and saved party identities are bound.
#include "objective_bank.h"
static u8 objective_load(u8 encounter)
{
    u16 size,n;u8 i;char route=chapter_route(encounter);
    const u8 *bank=resource_file("ffgo",&size),*e;
    W->goal=RT_NULL;
    if(!bank)return 1;
    if(!objective_bank_valid(bank,size))return 0;
    for(n=0;n<RD16(bank,3);n++){
        e=bank+8+16*n;if(e[0]!=(u8)route)continue;
        if(!core_objective_valid(GO_TARGET(e),GO_PROTECT(e),NUNIT))return 0;
        for(i=0;i<NUNIT;i++){
            u16 bit=(u16)1<<i,mask=GO_TARGET(e)|GO_PROTECT(e);
            const char *actual;const u8 *expected;
            if(!(mask&bit))continue;
            if((GO_TARGET(e)&bit)?unit_defs[i].team!=TEAM_ENEMY:unit_defs[i].team==TEAM_ENEMY)return 0;
            if(!st.unit[i].hp||DC_OBJ(st.unit[i].dc)||st.unit[i].dc==DC_GONE)return 0;
            actual=unit_name(i);expected=e+RD16(e,GO_TARGET(e)&bit?6:7);
            do{u8 c=(u8)*actual++;if(c>='a'&&c<='z')c-=32;if(c!=*expected)return 0;}while(*expected++);
        }
        W->goal=e;return 1;
    }
    return 1; // routes absent from the directory keep ordinary elimination
}
