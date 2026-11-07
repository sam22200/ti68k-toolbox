// One cache miss per wind-up update. Actor/target state and battle RNG are read only.
static u8 area_number_prepare(void)
{
    AreaImpact *q=&W->area_impact;u8 i,r;char s[8];
    if(!area_impact_root()||(W->cast.phase!=CAST_BATCH_READY&&W->cast.phase!=CAST_BATCH_APPLIED))return 0;
    while(q->next<NUNIT){
        const CastVictim *v;u8 n=0,kind,heal;u16 amount;
        i=q->next++;if(!(W->cast.mask&((u16)1<<i))||(q->ready&((u16)1<<i)))continue;
        v=&W->cast.victim[i];kind=v->effect&EFFECT_KIND_MASK;
        if(!v->hit||kind==AB_NULL)n=put_s(s,0,"Miss");
        else if(kind==AB_STEAL_GIL)n=put_s(s,0,"Steal");
        else if(kind==AB_QUICK||kind==AB_GOLEM)n=put_s(s,0,kind==AB_GOLEM?"Golem":v->amount>>8?"CT 0":"Quick");
        else if(kind==AB_STATUS)n=put_s(s,0,status_label(v->amount));
        else if(kind==AB_CLEANSE)n=put_s(s,0,"Cured");
        else if(kind==AB_MP_DAMAGE||kind==AB_MP_HEAL){s[n++]=kind==AB_MP_HEAL?'+':'-';n=put_n(s,n,v->amount);n=put_s(s,n,"MP");}
        else if(kind==AB_BRAVE||kind==AB_FAITH||kind==AB_SPEED||kind==AB_BREAK)n=stat_text(s,kind,W->cast.ability,v->amount,0);
        else{
            amount=abilities[W->cast.ability].formula==0x20?v->amount&1023:v->amount;heal=kind==AB_HEAL;
            if((s16)amount<0){heal=!heal;amount=(u16)(0U-amount);}
            if(heal)s[n++]='+';
            n=put_n(s,n,amount);
        }
        s[n]=0;make_number(0,s);q->width[i]=W->num_w[0];
        for(r=0;r<7;r++)q->glyph[i][r]=W->num[0][0][r];
        q->ready|=(u16)1<<i;W->num_ok&=6;
        return 0;
    }
    if(!q->oil_ready){
        for(i=0;i<NUNIT;i++)if((W->cast.mask&((u16)1<<i))&&W->cast.victim[i].hit&&
           (W->cast.victim[i].effect&EFFECT_OIL))break;
        if(i<NUNIT){
            make_number(0,"Oil-");q->oil_width=W->num_w[0];
            for(r=0;r<7;r++)q->oil[r]=W->num[0][0][r];
            W->num_ok&=6;
        }
        q->oil_ready=1;
    }
    q->next=NUNIT+1;return 1;
}
