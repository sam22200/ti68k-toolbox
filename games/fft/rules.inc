// The chapters' rules beyond Gariland's first battle (campaign/ENGINE.md M3), each in effect
// through the chapter's data: a weapon's reach and its projectile (WEAPON_REACH), Charge
// waiting on the CT clock (AF_CHARGE, charge_ticks), weapon skills, drain and a cross splash
// (AF_WEAPON, AF_DRAIN, AF_AREA), the units' fixed records (UNIT_RAW).

// A due deferred actor (255: none). Origin/weapon changes cancel only Charge.
static u8 pending_next(void)
{
    u8 i;
    for (i=0;i<NUNIT;i++) if (st.pending[i].state) {
        Unit *u=&st.unit[i];
        u8 kind=st.pending[i].state;
        if (!u->hp || u->x>=MAP_W || u->z>=MAP_H ||
            (kind==DEFER_CHARGING&&(UNIT_CELL(u)!=st.pending[i].origin||hand_weapon(u,0)!=st.pending[i].weapon||hand_weapon(u,1)!=st.pending[i].off_weapon))||
            (kind!=DEFER_CHARGING&&((kind!=DEFER_CAST_TILE&&kind!=DEFER_CAST_UNIT)||!(core_status_current(&u->statuses,0)&8)))) {
            st.pending[i].state=DEFER_NONE;unit_status_action(u,0);continue;
        }
        if(u->status_flags&STATUS_CT_FROZEN)continue;
        if (!st.pending[i].ticks) return i;
    }
    return 255;
}

// Return one while an action owns presentation; zero resumes the same clock
// phase. Each rejected due slot is consumed once, with no recursive dispatch.
static u8 charge_resolve(void)
{
    u8 a,result;
    if(st.action.phase!=CTX_IDLE)return 1;
    a=pending_next();
    if(a>=NUNIT)return 0;
#ifdef FFT_MODULES
    result=(u8)mod_call(MOD_COMMAND,4,a);
#else
    result=command_pending(a);
#endif
    return cast_dispatch(result);
}

// the units' records as the chapter gives them (stats, signs, equipment: no draw)


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
    if(st.abil==ABIL_DEFEND)return;
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
