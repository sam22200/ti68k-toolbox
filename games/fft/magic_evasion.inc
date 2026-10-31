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
    if(abilities[ab].formula==8)return evade;
    chance=(u8)magic_heal(a,t,ab,1);
    if(execute)return evade&&(chance>=100||(u16)(muls16((s16)rand15(),100)>>15)<chance);
    return (u8)(((u16)evade*chance)/100);
}
static u8 magic_hit(u8 a,u8 t,AbId ab,u8 execute)
{
    const Unit *u=&st.unit[t];u8 ev[3],k,base=100,scale=1,id,tag;
    u16 product;u8 factor[3];
    if(magic_reflects(t,ab))return 0;
    if(abilities[ab].formula==0x2a&&(core_status_current(&u->statuses,4)&0x10))return 0;   // Talk: a sleeper never listens, no draw
    if(abilities[ab].formula==0x31)return execute?roll_hit(a,t):hit_chance(a,t);       // Spin Fist: Attack's physical evasion
    if(abilities[ab].formula==12)return execute?1:100;
    if(!(abilities[ab].flags&AF_EVADE)||(abilities[ab].formula==14&&(core_status_current(&u->statuses,0)&16)))return magic_accuracy(a,t,ab,execute?1:100,execute);
    for(k=0;k<3;k++){
        id=u->eq[k==0?EQ_ACC:k==1?EQ_WEAPON:EQ_SHIELD];tag=id==255?0:items[id].pad;
        ev[k]=((tag&ITEM_MAGIC_SHIELD)!=0)==(k!=0)?(tag>>ITEM_MAGIC_SHIFT)&ITEM_MAGIC_MASK:0;
    }
    if((core_status_current(&st.unit[a].statuses,2)&0x10)||core_status_no_evade(&u->statuses))
        ev[0]=ev[1]=ev[2]=0;
    if(st.reaction[t]==REACTION_ABANDON&&!(core_status_current(&u->statuses,4)&4)){
        u8 restricted=0;
        if(W->surface){
            const u8 *col=W->surface+((u16)(W->grid_row[u->z]+u->x)<<3)+4*node_layer(UNIT_CELL(u));
            restricted=(col[1]>>5)>=2&&!(core_status_current(&u->statuses,2)&0x46)&&!(u->status_flags&STATUS_RIDER);
        }
        if(!restricted){base>>=1;scale<<=1;}
    }
    if(core_status_current(&u->statuses,0)&2){base>>=1;scale<<=1;}
    for(k=0;k<3;k++){
        if(execute){
            u16 r=rand15(); // Even a zero evade consumes its source draw.
            if(ev[k]&&(ev[k]>=base||(u16)(muls16((s16)r,base)>>15)<ev[k]))return 0;
        }else{
            if(ev[k]>=base)return 0;
            factor[k]=100-ev[k]*scale;
        }
    }
    if(execute){rand15();return magic_accuracy(a,t,ab,1,1);} // class evade is zero for magic, but still rolled
    product=(u16)(factor[0]*factor[1]);
    return magic_accuracy(a,t,ab,(u8)divs32_16(muls16(product,factor[2]),10000),0);
}
