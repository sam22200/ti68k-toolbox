// Pure forecast / source-order execution for ordinary unmounted formula08 targets.
// No overlay loads, equipment/stat rebuild or preview RNG. Monster/movement producers gated.
static u8 magic_target(u8 t)
{
    const Unit *u;if(t>=NUNIT)return MAGIC_ABSENT;u=&st.unit[t];
    if(u->x>=MAP_W||u->z>=MAP_H||UNIT_CELL(u)>=MAP_N||DC_OBJ(u->dc)||
       (core_status_current(&u->statuses,0)&0x44)||
       (core_status_current(&u->statuses,1)&1))return MAGIC_ABSENT;
    if(!u->hp||(core_status_current(&u->statuses,0)&0x20)||
       (core_status_current(&u->statuses,1)&0x80)||
       (core_status_current(&u->statuses,3)&1))return MAGIC_BLOCKED;
    return MAGIC_VALID;
}
static u8 magic_accuracy(u8 a,u8 t,AbId ab,u8 evade,u8 execute)
{
    u8 chance;
    if(abilities[ab].formula==8||abilities[ab].formula==0x24||abilities[ab].formula==99)return evade;
    chance=(u8)magic_heal(a,t,ab,1);
    if(execute)return evade&&(chance>=100||(u16)(muls16((s16)rand15(),100)>>15)<chance);
    return (u8)(((u16)evade*chance)/100);
}
// All four physical/magical evade sources share preview and execution order.
// Guard rolls run before those four stages, as in the original pre-formula setup.
static u8 combat_restricted(u8 t)
{
    const Unit *u=&st.unit[t];
    if(core_status_current(&u->statuses,4)&4)return 1;
    if(W&&W->surface){
        const u8 *p=W->surface+((u16)(W->grid_row[u->z]+u->x)<<3)+4*node_layer(UNIT_CELL(u));
        if((p[1]>>5)>=2&&!(core_status_current(&u->statuses,2)&0x46)&&!(u->status_flags&STATUS_RIDER))return 1;
    }
    return 0;
}
static u8 magic_hit(u8 a,u8 t,AbId ab,u8 execute)
{
    u8 returned=execute&2;execute&=1; // bit1: pure already-reflected forecast
    const Unit *u=&st.unit[t];const Ability *b=ab<NABIL?&abilities[ab]:RT_NULL;
    const u8 *raw=cj_range(common,ab>=512||ab==NO_ABIL?0:ab);
    u8 ev[4],k,base=100,scale=1,id,tag,guard=0,physical=!b||b->formula==99||b->formula==0x37||
        (u8)(b->formula-0x25)<=7U||(u8)(b->formula-0x31)<=1U||(b->flags&AF_WEAPON),chance=100;
    u16 product;u8 factor[4];
    if(!b&&(!unit_can_act(&st.unit[a])||unit_petrified(u)||unit_airborne(u)))return 0;
    if(b&&!returned&&magic_reflects_from(a,t,ab))return 0;
    if(b&&b->formula==0x29){
        base=magic_accuracy(a,t,ab,execute?1:100,execute);
        return st.unit[a].female!=u->female?base:0;
    }
    if(b&&performance_formula(b->formula)){
        if(core_status_current(&u->statuses,4)&16)return 0;
        base=(u8)magic_heal(a,t,ab,1);
        return execute?(u16)(muls16((s16)rand15(),100)>>15)<base:base;
    }
    if(b&&b->formula==0x2a){
        if((core_status_current(&u->statuses,4)&0x10)||(unit_monster(u)&&!cj_support(common,st.unit[a].job,st.support[a],473)))return 0;
        if(st.reaction[t]==448&&!returned&&st.action.phase<CTX_SECONDARY&&reaction_live(t)&&!(st.action.pending&((u16)1<<t))){
            u8 n=100-u->brave;
            if(!execute)return magic_accuracy(a,t,ab,n,0);
            if(!n||reaction_chance(u->brave,rand15()))return 0;
        }
    }
    if(b&&(b->formula==12||draw_out_formula(b->formula)||guts_formula(b->formula)))return execute?1:100;
    if(b&&(!(b->flags&AF_EVADE)||(b->formula==14&&(core_status_current(&u->statuses,0)&16))))
        return magic_accuracy(a,t,ab,execute?1:100,execute);
    if(!returned&&st.action.phase<CTX_SECONDARY&&reaction_live(t)&&!(st.action.pending&((u16)1<<t))){
        if(st.reaction[t]==REACTION_BLADE_GRASP&&raw&&(raw[6]&16))guard=1;
        if(st.reaction[t]==REACTION_ARROW_GUARD&&raw&&(raw[3]&32)){
            id=hand_weapon(&st.unit[a],0);
            if(id!=255){id=cj_equipment_item(common,id)[0];if(id==11||id==12)guard=1;}
        }
        // Catch has its own early-success path at Brave100 (no Brave draw).
        if(b&&b->formula==99&&st.reaction[t]==REACTION_CATCH&&st.unit[a].job!=SOURCE_JOB_MIME)guard=2;
        if(guard){
            chance=100-u->brave;
            if(execute&&((guard==2&&!chance)||reaction_chance(u->brave,rand15())))return guard==2?2:0;
        }
    }
    // Ordinary guns bypass the four evade stages, after pre-formula guards.
    if(!b||(b->flags&AF_CHARGE)){u8 f=cj_weapon(common,current_weapon(a))[2];if(f<8&&((0x58U>>f)&1))return execute?1:chance;}
    for(k=0;k<3;k++){
        id=u->eq[k==0?EQ_ACC:k==1?EQ_WEAPON:EQ_SHIELD];tag=id==255?0:items[id].pad;
        ev[k]=physical?(id!=255&&(k==0||(tag&ITEM_MAGIC_SHIELD)||
            (st.reaction[t]==REACTION_WEAPON_GUARD&&!combat_restricted(t)))?items[id].sev:0):
            ((tag&ITEM_MAGIC_SHIELD)!=0)==(k!=0)?(tag>>ITEM_MAGIC_SHIFT)&ITEM_MAGIC_MASK:0;
    }
    ev[3]=0;
    if(physical){
        u8 side=attack_side(a,t);ev[3]=side==SIDE_FRONT?jobs[u->job].cev:0;
        if(side==SIDE_BACK)ev[1]=ev[2]=0;
        if(cj_support(common,st.unit[a].job,st.support[a],469))ev[0]=ev[1]=ev[2]=ev[3]=0;
        if((st.unit[a].status&ST_DARK)||(core_status_current(&st.unit[a].statuses,1)&16))base>>=1;
    }
    if(st.reaction[t]==REACTION_ABANDON&&!combat_restricted(t)){base>>=1;scale<<=1;}
    if(core_status_no_evade(&u->statuses)||unit_transparent(&st.unit[a]))ev[0]=ev[1]=ev[2]=ev[3]=0;
    if(core_status_current(&u->statuses,0)&2){base>>=1;scale<<=1;}
    for(k=0;k<4;k++){
        if(execute){u16 r=rand15();if(ev[k]&&(ev[k]>=base||(u16)(muls16((s16)r,base)>>15)<ev[k]))return 0;}
        else {if(ev[k]>=base)return 0;factor[k]=physical?(u16)((base-ev[k])*100)/base:100-ev[k]*scale;}
    }
    if(execute)return physical?1:magic_accuracy(a,t,ab,1,1);
    product=(u16)(factor[0]*factor[1]);
    if(physical){
        s32 three=muls16(product,factor[2]);u16 q,rem;
        q=(u16)divs32_16(three,10000);rem=(u16)(three-muls16(q,10000));
        product=(u16)((q*factor[3]+(u16)divs32_16(muls16(rem,factor[3]),10000))/100);
        return (u8)((product*chance)/100);
    }
    return magic_accuracy(a,t,ab,(u8)divs32_16(muls16(product,factor[2]),10000),0);
}
