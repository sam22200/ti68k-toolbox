// Shared scalar number raster/presentation, also used by the area cache.
#include "status_label.h"
static void make_number(u8 k, const char *s)
{
    u32 g[7], o = 0;
    u8 r;
    for (r = 8; r < HUD_ROWS; r++) memset(HUD_L + r * RT_PBYTES, 0, 4);
    text_into(W->hud[0], W->hud[1], 1, 8, s, C_BLACK);
    for (r = 0; r < 7; r++) {
        const u8 *b = HUD_L + (7 + r) * RT_PBYTES;
        g[r] = r && r < 6 ? (u32)b[0] << 24 | (u32)b[1] << 16 | (u32)b[2] << 8 | b[3] : 0;
        o |= g[r];
    }
    for (W->num_w[k] = 32; W->num_w[k] && !(o & (1UL << (32 - W->num_w[k]))); W->num_w[k]--) ;
    for (r = 0; r < 7; r++) {
        u32 m = g[r] | (r ? g[r - 1] : 0) | (r < 6 ? g[r + 1] : 0);
        W->num[k][0][r] = g[r];
        W->num[k][1][r] = ~(m | m << 1 | m >> 1);
    }
}

// the target's from ACT_HIT (HP lost or healed, the status removed, Miss), then the EXP its
// user earned from ACT_HIT + GAIN_T ("LvUp" for a level up) and its JP from ACT_HIT + JP_T
// above the user
static void number_single(void)
{
    u8 t = (u8)(st.act_t - ACT_HIT), k = 0;
    Feet f;
    RtSprite sp;
    char s[8];
    if (st.gain && t >= JP_T) { k = 2; t -= JP_T; }
    else if (st.gain && t >= GAIN_T) { k = 1; t -= GAIN_T; }
    if (t >= (k ? GAIN_N : NUM_T)) return;
    f = unit_feet(st.rot, k ? st.atk : st.tgt);
    if (!(W->num_ok & (1 << k))) {                 // one a frame (AMS text)
        u8 kind = act_kind(), n = 0;
        if (k == 2) { n = put_s(s, 0, "JP+"); n = put_n(s, n, st.jpg); }
        else if (k) {
            if (st.lvup) n = put_s(s, 0, "LvUp");
            else { n = put_s(s, 0, "EXP+"); n = put_n(s, n, st.gain); }
        } else if (!st.hit) n = put_s(s, 0, "Miss");
#ifdef FFT_CHAPTER
        else if (kind == AB_RAISE) n=put_s(s,0,"PA+1");
        else if (kind == AB_STEAL_EXP) { n=put_s(s,0,"EXP-"); n=put_n(s,n,st.dmg); }
        else if (kind == AB_EQUIP_BREAK||kind==AB_STEAL_EQUIP) n=put_s(s,0,kind==AB_EQUIP_BREAK?"Break":"Steal");
        else if (kind == AB_BREAK){n=put_s(s,0,st.abil==0x90?"PA-":st.abil==0x8f?"SP-":"MA-");n=put_n(s,n,abilities[st.abil].y);}
        else if (kind == AB_QUICK||kind==AB_GOLEM) n=put_s(s,0,kind==AB_GOLEM?"Golem":st.dmg>>8?"CT 0":"Quick");
        else if (kind == AB_STATUS) n=put_s(s,0,status_label(st.dmg));
        else if (kind == AB_CLEANSE) n=put_s(s,0,"Cured");
        else if (kind == AB_HP_MP_HEAL) n=put_s(s,0,"+HP/MP");
        else if (kind == AB_HP_DAMAGE_MP_HEAL) n=put_s(s,0,"-HP+MP");
        else if (kind == AB_MP_DAMAGE||kind==AB_MP_HEAL||(kind==AB_BREAK&&st.abil==0x8e)){s[n++]=kind==AB_MP_HEAL?'+':'-';n=put_n(s,n,st.dmg);n=put_s(s,n,"MP");}
        else if (kind == AB_BRAVE||kind==AB_FAITH||kind==AB_SPEED){s8 v=(s8)st.dmg;s[n++]=v<0?'+':'-';n=put_n(s,n,(u16)(v<0?-v:v));n=put_s(s,n,kind==AB_BRAVE?"Br":kind==AB_FAITH?"Fa":"Sp");}
#endif
        else if (kind == AB_DEFEND) n = put_s(s, 0, st_names[0]);
        else if (kind == AB_CURE) {                // the first status removed
            while (!(st.dmg & (1 << n))) n++;
            n = put_s(s, 0, st_names[n]);
        } else {
            u16 amount=st.dmg;u8 healing=kind!=AB_DAMAGE;
#ifdef FFT_CHAPTER
            if(st.abil<512&&abilities[st.abil].formula==8&&(s16)amount<0){
                healing=!healing;amount=(u16)(0U-amount);
            }
#endif
            if (healing) s[n++] = '+';
            n = put_n(s, n, amount);
        }
        s[n] = 0;
        make_number(k, s);
        W->num_ok |= (u8)(1 << k);
    }
    if (k) t <<= 1;                         // reward rise follows the doubled display pace
    sp.w = 32; sp.h = 7; sp.light = sp.dark = W->num[k][0]; sp.mask = W->num[k][1];
    draw_sprite(f.x - st.camx - ((W->num_w[k] + 2) >> 1), f.y - st.camy - UNIT_FOOT - 6 - (t < NUM_RISE ? t : NUM_RISE), &sp);
}

#ifdef FFT_CHAPTER
// One cache miss per wind-up update. Actor/target state and battle RNG are read only.
static u8 area_number_prepare(void)
{
    AreaImpact *q=&W->area_impact;u8 i,r;char s[8];
    if(!area_impact_root()||W->cast.phase!=CAST_BATCH_READY)return 0;
    while(q->next<NUNIT){
        const CastVictim *v;u8 n=0,kind,heal;u16 amount;
        i=q->next++;if(!(W->cast.mask&((u16)1<<i)))continue;
        v=&W->cast.victim[i];kind=v->effect&EFFECT_KIND_MASK;
        if(!v->hit||kind==AB_NULL)n=put_s(s,0,"Miss");
        else if(kind==AB_QUICK||kind==AB_GOLEM)n=put_s(s,0,kind==AB_GOLEM?"Golem":v->amount>>8?"CT 0":"Quick");
        else if(kind==AB_STATUS)n=put_s(s,0,status_label(v->amount));
        else if(kind==AB_CLEANSE)n=put_s(s,0,"Cured");
        else if(kind==AB_MP_DAMAGE||kind==AB_MP_HEAL){s[n++]=kind==AB_MP_HEAL?'+':'-';n=put_n(s,n,v->amount);n=put_s(s,n,"MP");}
        else if(kind==AB_BRAVE||kind==AB_FAITH||kind==AB_SPEED){s8 w=(s8)v->amount;s[n++]=w<0?'+':'-';n=put_n(s,n,(u16)(w<0?-w:w));n=put_s(s,n,kind==AB_BRAVE?"Br":kind==AB_FAITH?"Fa":"Sp");}
        else{
            amount=v->amount;heal=kind==AB_HEAL;
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
static void area_number_glyph(const u32 *g,u8 width,s16 x,s16 y)
{
    u32 mask[7];u8 r;RtSprite s;
    for(r=0;r<7;r++){
        u32 m=g[r]|(r?g[r-1]:0)|(r<6?g[r+1]:0);
        mask[r]=~(m|m<<1|m>>1);
    }
    s.w=32;s.h=7;s.light=s.dark=g;s.mask=mask;
    draw_sprite(x-((width+2)>>1),y,&s);
}
#endif
static void numbers_draw(void)
{
#ifdef FFT_CHAPTER
    if(area_impact_root()&&(!st.gain||st.act_t<ACT_HIT+GAIN_T)){
        AreaImpact *q=&W->area_impact;u8 i,t=(u8)(st.act_t-ACT_HIT);
        if(W->cast.phase!=CAST_BATCH_APPLIED||t>=NUM_T)return;
        for(i=0;i<NUNIT;i++)if(W->cast.mask&q->ready&((u16)1<<i)){
            Feet f=unit_feet(st.rot,i);s16 x=f.x-st.camx;
            s16 y=f.y-st.camy-UNIT_FOOT-6-(t<NUM_RISE?t:NUM_RISE);
            area_number_glyph(q->glyph[i],q->width[i],x,y);
            if(q->oil_ready&&W->cast.victim[i].hit&&(W->cast.victim[i].effect&EFFECT_OIL))
                area_number_glyph(q->oil,q->oil_width,x,y+8);
        }
        return;
    }
#endif
    number_single();
}
