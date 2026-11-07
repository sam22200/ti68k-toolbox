// Shared deferral selection; no image loads.
static u8 pending_next(void)
{
    u8 i;
    for (i=0;i<NUNIT;i++) if (st.pending[i].state) {
        Unit *u=&st.unit[i];
        u8 kind=st.pending[i].state;
        if (!u->hp || u->x>=MAP_W || u->z>=MAP_H ||
            (kind==DEFER_CHARGING&&(UNIT_CELL(u)!=st.pending[i].origin||hand_weapon(u,0)!=st.pending[i].weapon||hand_weapon(u,1)!=st.pending[i].off_weapon))||
            (kind!=DEFER_CHARGING&&(kind==DEFER_AIRBORNE?!unit_airborne(u):kind==DEFER_PERFORMING?!(core_status_current(&u->statuses,0)&1):((kind!=DEFER_CAST_TILE&&kind!=DEFER_CAST_UNIT)||!(core_status_current(&u->statuses,0)&8))))) {
            st.pending[i].state=DEFER_NONE;unit_status_action(u,0);continue;
        }
        if(u->status_flags&STATUS_CT_FROZEN)continue;
        if (!st.pending[i].ticks) return i;
    }
    return 255;
}
