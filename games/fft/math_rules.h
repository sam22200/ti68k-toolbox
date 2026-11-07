// Arithmeticks reuses real spell IDs and the batch's existing ref/follow bytes.
#ifndef FFT_MATH_RULES_H
#define FFT_MATH_RULES_H
#define MATH_BIND 128
#define MATH_DATA (charge_ticks+CJ_MATH-CJ_EXECUTABLES*12U)
static u8 math_spell(AbId ab)
{
    return ab<60&&MATH_DATA[256+ab]!=255;
}
static u8 math_filters(u8 who,u8 code)
{
    u8 bits=(u8)st.unit[who].learned;
    if(MIME_ACTIVE(who))return code<16;
    return code<16&&(bits&(1U<<(code>>2)))&&(bits&(1U<<(4+(code&3))));
}
#if MOD_CORE
static u8 math_available(u8 who,AbId ab)
{
    const Unit *u=&st.unit[who];u8 slot;
    if(ab>=60||!((u8)u->learned&15)||!((u8)u->learned&240))return 0;
    slot=MATH_DATA[256+ab];if(slot==255)return 0;
    if(u->slot<battle_save.n&&u->slot<PARTY_MAX)
        return (battle_save.unit[u->slot].learned[slot>>4][(slot&15)>>3]&(128>>(slot&7)))!=0;
    // Native acquisition policy for actors without a campaign learned row.
    return AB_JP(&abilities[ab])<=(u16)(u->level*20U);
}
static AbId math_known(u8 who,u8 k)
{
    u8 phase=st.mode==M_SKILLS&&who==st.act?st.sub&96:64,n=0,count=k==255,i;
    if(phase==64){
        for(i=1;i<60;i++)if(math_available(who,i)){if(count)n++;else if(!k--)return i;}
    }else{
        u8 end=phase?8:4;
        for(i=phase?4:0;i<end;i++)if((u8)st.unit[who].learned&(1U<<i)){if(count)n++;else if(!k--)return 414U+i;}
    }
    return count?n:NO_ABIL;
}
#endif
#if (MOD_CORE && !defined(FFT_MODULES)) || MOD_PART(MOD_TARGETS) || MOD_PART(MOD_TURN) || MOD_PART(MOD_NATURE)
static u8 math_height(u8 who)
{
    const Unit *u=&st.unit[who];u8 cell=UNIT_CELL(u),j,h;const u8 *p;
    for(j=0;j<NUNIT;j++)if(j!=who&&st.unit[j].dc!=DC_GONE&&st.unit[j].x<MAP_W&&st.unit[j].z<MAP_H&&UNIT_CELL(&st.unit[j])==cell){u=&st.unit[j];break;}
    p=W->surface+((u16)(W->grid_row[u->z]+u->x)<<3)+4*node_layer(cell);
    h=(u8)((p[0]<<1)+(p[1]&31));
    if(core_status_current(&u->statuses,2)&64)h+=(u8)(((p[1]>>5)<<1)+2);
    return h;
}
static s16 math_targets(u8 a,u8 code)
{
    u16 mask=0,selected=0,bit;u8 i,j,value;
    if(code>=16)return TARGETS_INVALID;
    for(i=0,bit=1;i<NUNIT;i++,bit<<=1){
        const Unit *u=&st.unit[i];
        if(!magic_target(i)||(u->status_flags&STATUS_RIDER))continue;
        if((code>>2)==0)value=i==a||u->ct>100?100:u->ct;
        else if((code>>2)==1)value=u->level;
        else if((code>>2)==2)value=u->exp;
        else {value=math_height(i);if(value&1)continue;value>>=1;}
        if(MATH_DATA[value]&(1U<<(code&3)))selected|=bit;
    }
    // The original selects panels, then lists every eligible occupant on them.
    for(i=0,bit=1;i<NUNIT;i++,bit<<=1)if(magic_target(i)&&!(st.unit[i].status_flags&STATUS_RIDER)){
        for(j=0;j<NUNIT;j++)if((selected&((u16)1<<j))&&UNIT_CELL(&st.unit[i])==UNIT_CELL(&st.unit[j])){mask|=bit;break;}
    }
    return (s16)mask;
}
#else
#if MOD_CORE
static s16 math_targets(u8 a,u8 code);
#endif
#endif
#endif
