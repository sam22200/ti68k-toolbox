// Shared clock loop; ORDER owns its status and pending helpers on TI.
u8 ct_next(void)
{
#ifdef FFT_CHAPTER
    u8 ticks=0,limit=8,live_limit=st.mode==M_CLOCK?16:128;
    // Keep ordinary turn entry synchronous; an idle-clock wake resumes in small quanta.
#endif
    for (;;) {
        u8 i, best = 0xFF, max = 99;
        Unit *u;
#ifdef FFT_CHAPTER
        // After a due root, only Quick may interrupt the remaining charge wave.
        if (st.open_ct_phase) max=254;
#endif
        for (i = 0, u = st.unit; i < NUNIT; i++, u++)
            if (!DC_OBJ(u->dc)
#ifdef FFT_CHAPTER
                && !(u->status_flags&(STATUS_CT_FROZEN|STATUS_AIRBORNE))
#endif
                && u->ct > max) { max = u->ct; best = i; }
        if (best == 0xFF) {                        // a tick
#ifdef FFT_CHAPTER
            if(ticks>=limit)return CT_WAIT;
            if (!st.open_ct_phase) {
                for (i = 0; i < NUNIT; i++)
                    if (st.pending[i].state && st.pending[i].ticks && !(st.unit[i].status_flags&STATUS_CT_FROZEN)) st.pending[i].ticks--;
                st.open_ct_phase = 1;
            }
            if (pending_next() != 255) return CT_DUE;
            st.open_ct_phase = 0;
            // All due Charge actions finish before this tick expires statuses.
            if(st.status_clock){
#ifdef FFT_MODULES
                mod_call(MOD_ORDER,4,0);
#else
                status_tick();
#endif
            }
#endif
            for (i = 0, u = st.unit; i < NUNIT; i++, u++)
                if (!DC_OBJ(u->dc)
#ifdef FFT_CHAPTER
                    && !(u->status_flags&(STATUS_CT_FROZEN|STATUS_AIRBORNE))
#endif
                ) {
#ifdef FFT_CHAPTER
                    u8 gain=unit_ct_gain(u);
                    if(u->hp&&gain)limit=live_limit;
#else
                    u8 gain=u->sp;
#endif
                    u16 c=u->ct+gain;u->ct=(u8)(c>254?254:c);
                }
#ifdef FFT_CHAPTER
            ticks++;
#endif
            continue;
        }
        u = &st.unit[best];
        u->ct = max==255 ? 0 : (u8)(max >= 200 ? max - 200 : max - 100);
        if (u->hp) return best;
#ifdef FFT_CHAPTER
#ifdef FFT_MODULES
        if(mod_call(MOD_ORDER,14,best))return best;
#else
        if(status_dead_turn(best))return best;
#endif
#else
        if (u->team == TEAM_GUEST) continue;
        if (u->dc) u->dc--;
        else {
            u->dc = rand15() & (u->team == TEAM_PLAYER ? 0x110 : 0x10) ? DC_CRYSTAL : DC_CHEST;
            st.morph_mask |= (u16)(1U << best); // retain all events even if one clock pass converts several
        }
#endif
    }
}
