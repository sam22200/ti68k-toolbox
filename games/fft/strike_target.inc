// Pure bounded follow-up target; caller validates the trajectory after returning.
static u8 strike_target(void)
{
    CoreAction *c=&st.action;u8 i;
    if((c->phase!=CTX_IMPACT&&c->phase!=CTX_APPLIED)||!c->left)return 0;
    if(c->left&WEAPON_PANEL_MORE){
        WeaponPanel *p=&W->weapon_panel;
        for(i=0;i<NUNIT;i++)if(p->todo&((u16)1<<i)){
            p->todo&=(u16)~((u16)1<<i);if(!p->todo)c->left&=127;
            p->resume=1;return i+1;
        }
        c->left&=127;if(!c->left)return 0;
    }
    c->left--;
    if(!st.unit[c->actor].hp||!unit_can_act(&st.unit[c->actor])){c->left=0;return 0;}
    for(i=0;i<NUNIT;i++)if(st.unit[i].hp&&st.unit[i].x<MAP_W&&st.unit[i].z<MAP_H&&
        UNIT_CELL(&st.unit[i])==c->cell&&ranged_reach(c->actor,UNIT_CELL(&st.unit[c->actor]),c->cell))return i+1;
    return 0;
}
