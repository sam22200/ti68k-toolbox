// Shared scalar number raster/presentation, also used by the area cache.
#include "status_label.h"
#ifdef FFT_CHAPTER
#include "stat_text.h"
#endif
static void make_number(u8 k, const char *s)
{
    u32 g[7], o = 0;
    u8 r;
    for (r = 8; r < HUD_ROWS; r++) memset(HUD_L + r * RT_PBYTES, 0, 4);
    text_into((u16 *)HUD_L, (u16 *)HUD_D, 1, 8, s, C_BLACK);
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

#if MOD_CORE || MOD_PART(MOD_AREA_UI)
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
        else if(kind==AB_CATCH)n=put_s(s,0,"Catch");
        else if(st.abil==REACTION_GILGAME){n=put_s(s,0,"Gil+");n=put_n(s,n,st.dmg);}
        else if(reaction_stat(st.abil))n=put_s(s,0,"PA+1\0MA+1\0Sp+1"+5U*(u16)(st.abil-REACTION_PA_SAVE));
        else if(kind==AB_RAISE)n=put_s(s,0,"PA+1");
        else if (kind == AB_STEAL_EXP) { n=put_s(s,0,"EXP-"); n=put_n(s,n,st.dmg); }
        else if (kind == AB_EQUIP_BREAK||kind==AB_STEAL_EQUIP||kind==AB_STEAL_GIL) n=put_s(s,0,kind==AB_EQUIP_BREAK?"Break":"Steal");
        else if (kind == AB_BREAK){n=put_s(s,0,st.abil==0x90?"PA-":st.abil==0x8f?"SP-":"MA-");n=put_n(s,n,abilities[st.abil].y);}
        else if (kind == AB_QUICK||kind==AB_GOLEM) n=put_s(s,0,kind==AB_GOLEM?"Golem":st.dmg>>8?"CT 0":"Quick");
        else if (kind == AB_STATUS) n=put_s(s,0,status_label(st.dmg));
        else if (kind == AB_CLEANSE) n=put_s(s,0,"Cured");
        else if (kind == AB_HP_MP_HEAL) n=put_s(s,0,"+HP/MP");
        else if (kind == AB_HP_DAMAGE_MP_HEAL) n=put_s(s,0,"-HP+MP");
        else if (kind == AB_MP_DAMAGE||kind==AB_MP_HEAL||(kind==AB_BREAK&&st.abil==0x8e)){s[n++]=kind==AB_MP_HEAL?'+':'-';n=put_n(s,n,st.dmg);n=put_s(s,n,"MP");}
        else if (kind == AB_BRAVE||kind==AB_FAITH||kind==AB_SPEED)n=stat_text(s,kind,st.abil,st.dmg,0);
#endif
        else if (kind == AB_DEFEND) n = put_s(s, 0, st_names[0]);
        else if (kind == AB_CURE) {                // the first status removed
            while (!(st.dmg & (1 << n))) n++;
            n = put_s(s, 0, st_names[n]);
        } else {
            u16 amount=st.dmg;u8 healing=kind!=AB_DAMAGE;
#ifdef FFT_CHAPTER
            if((st.abil==NO_ABIL||(st.abil<512&&((abilities[st.abil].flags&AF_CHARGE)||abilities[st.abil].formula==8)))&&(s16)amount<0){
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
static u8 area_number_prepare(void);
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
        if(q->next<=NUNIT)area_number_prepare();
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

#endif
#if defined(FFT_CHAPTER) && (MOD_CORE || MOD_PART(MOD_AREA_UI))
#include "area_number_prepare.inc"
#endif
