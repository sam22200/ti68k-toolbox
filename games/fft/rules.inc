// The chapters' rules beyond Gariland's first battle (campaign/ENGINE.md M3), each in effect
// through the chapter's data: a weapon's reach and its projectile (WEAPON_REACH), Charge
// waiting on the CT clock (AF_CHARGE, charge_ticks), weapon skills, drain and a cross splash
// (AF_WEAPON, AF_DRAIN, AF_AREA), the units' fixed records (UNIT_RAW).
enum { DEFER_NONE, DEFER_CHARGING, DEFER_AIRBORNE, DEFER_PERFORMING };

// a unit whose charged action is due (255: none); a KO, moved or re-armed one loses it
static u8 pending_next(void)
{
    u8 i;
    for (i=0;i<NUNIT;i++) if (st.pending[i].state) {
        const Unit *u=&st.unit[i];
        if (!u->hp || u->z*MAP_W+u->x!=st.pending[i].origin || u->eq[EQ_WEAPON]!=st.pending[i].weapon) {
            st.pending[i].state=DEFER_NONE;continue;
        }
        if (!st.pending[i].ticks) return i;
    }
    return 255;
}

static void charge_resolve(void)
{
    u8 a=pending_next(),t,ab=st.pending[a].ability,c=st.pending[a].tile;
    st.pending[a].state=DEFER_NONE;
    t=unit_at(c%MAP_W,c/MAP_W);
    if (!t || !st.unit[t-1].hp) { turn_start(); return; }
    st.open_resolving=1;st.act=a;st.ai=AI_OFF;
    st.cx=c%MAP_W;st.cz=c/MAP_W;
    skill(a,t-1,ab);
}

// the units' records as the chapter gives them (stats, signs, equipment: no draw)
static void fixed_units(void)
{
    u8 i,k;
    for (i=0;i<NUNIT;i++) {
        const UnitDef *d=&unit_defs[i];Unit *u=&st.unit[i];
        memset(u,0,sizeof *u);
        u->x=d->x;u->z=d->z;u->gfx=d->gfx;u->job=d->job;u->team=d->team;u->face=d->face;
        u->level=u->lv0=d->level;u->brave=d->brave;u->faith=d->faith;u->female=d->type==1;
        u->zodiac=unit_zodiac[i];u->set=d->set;u->learned=d->learned;u->dc=3;
        for(k=0;k<5;k++){u->eq[k]=d->eq[k][0];u->raw[k]=RD32(igfx+CD(UNIT_RAW),(u16)(i*10+2*k));}
        unit_stats(u);
    }
}

// a weapon reaching farther than 1 (a bow): from 3 tiles to its reach, more downhill (FFT's
// bow range); terrain collision is checked separately during targeting
static u8 ranged_reach(u8 a,u8 o,u8 c)
{
    const Unit *u=&st.unit[a];
    u8 range=WEAPON_REACH(u);
    s16 dx=(s16)TILE_X[c]-TILE_X[o],dz=(s16)TILE_Z[c]-TILE_Z[o],h=(s16)map_tiles[c].stand-map_tiles[o].stand;
    u8 d=(u8)((dx<0?-dx:dx)+(dz<0?-dz:dz));
    if(!map_tiles[c].walk)return 0;
    if(range==1)return d==1 && h>=-6 && h<=5;
    range=(u8)(range+(h<0?(-h)>>2:0));
    return d>=3 && d<=range;
}

// a projectile in flight, a weapon skill's strike
static void weapon_effect(void)
{
    u8 t=st.act_t;
    if(st.mode!=M_ACT)return;
    if((st.abil==NO_ABIL||(abilities[st.abil].flags&AF_CHARGE))&&WEAPON_REACH(&st.unit[st.atk])>1){
        if(t>=ACT_HIT-8&&t<ACT_HIT){
            Feet a=unit_feet(st.rot,st.atk),b=unit_feet(st.rot,st.tgt);u8 k=t-(ACT_HIT-8);
            s16 x=a.x+((b.x-a.x)*k>>3)-st.camx,y=a.y+((b.y-a.y)*k>>3)-st.camy-12;
            static const u8 arc[8]={0,3,5,6,6,5,3,0};y-=arc[k];
            draw_rect(x-2,y-1,6,3,C_WHITE);draw_rect(x-1,y,4,1,C_BLACK);
        }
    }else if(st.abil!=NO_ABIL&&st.abil!=ABIL_DEFEND&&(abilities[st.abil].flags&AF_WEAPON)&&t>=ACT_HIT-5&&t<ACT_HIT+6){
        Feet b=unit_feet(st.rot,st.tgt);s16 x=b.x-st.camx,y=b.y-st.camy;
        u8 h=(u8)(6+(t%5)*4);draw_rect(x-3,y-h,6,h,C_WHITE);draw_rect(x-1,y-h,2,h,C_DGRAY);
    }
}

// a skill's effects beyond its damage: the user drains it, a cross splash around the target
static void skill_extra(void)
{
    const Ability *b;
    Unit *a=&st.unit[st.atk],*t=&st.unit[st.tgt];
    if(st.abil==NO_ABIL)return;
    b=&abilities[st.abil];
    if(b->flags&AF_DRAIN){u16 n=st.dmg; if(n>a->max_hp-a->hp)n=a->max_hp-a->hp; a->hp+=n;}
    if(b->flags&AF_AREA){
        u8 i;
        // Cross-shaped Stasis Sword splash, including allies (no team filtering).
        for(i=0;i<NUNIT;i++){
            Unit *v=&st.unit[i];s16 dx=(s16)v->x-t->x,dz=(s16)v->z-t->z;
            u8 d=(u8)((dx<0?-dx:dx)+(dz<0?-dz:dz));u16 n;
            if(i==st.tgt||!v->hp||d!=1)continue;
            n=skill_amount(st.atk,i,st.abil,0);v->hp=n>=v->hp?0:v->hp-n;
            if(!v->hp)v->kos++;
        }
    }
}
