// FFT's AT list: the active unit, then each living unit's next turns, its clock run alone
// (no bonus at their end), sorted by tick then by the CT reached (the higher first), a tie in
// unit order. At each turn change only; reciprocal products skip constant-speed
// segments. Haste/Slow need at most three segments, never a per-tick simulation.
void turn_order(void)
{
    u16 tk[ORDER_N];
    u8 i, j, n = 1, t;
    Unit *u;
    st.turns[0] = st.act; tk[0] = 0;
    for (i = 0, u = st.unit; i < NUNIT; i++, u++) {
        u16 tick = 0, ct = u->ct;
        u16 haste=0,slow=0;
        u8 speed=u->sp?u->sp:1; // original AT forecast clamps0, the real clock does not
        if (!u->hp) continue;
#ifdef FFT_CHAPTER
        if(!unit_on_field(u)||unit_airborne(u))continue;
        if(u->status_flags&STATUS_CT_FROZEN){
            u8 stop=core_status_current(&u->statuses,3)&2,sleep=core_status_current(&u->statuses,4)&0x10;
            if((!stop&&!sleep)||(u->statuses.innate[3]&2)||(u->statuses.innate[4]&0x10))continue;
            if(stop)tick=u->status_ct[SID_STOP-SID_POISON];
            if(sleep&&tick<u->status_ct[SID_SLEEP-SID_POISON])tick=u->status_ct[SID_SLEEP-SID_POISON];
        }
        {
            u8 rate=core_status_current(&u->statuses,3),innate=u->statuses.innate[3];
            if(rate&8){haste=innate&8?512:u->status_ct[SID_HASTE-SID_POISON];haste=haste>tick?haste-tick:0;}
            if(rate&4){slow=innate&4?512:u->status_ct[SID_SLOW-SID_POISON];slow=slow>tick?slow-tick:0;}
        }
#endif
        for (t = 0; t < ORDER_N - 1; t++) {
            u16 k;
            while (ct < 100) {
                u16 gain=speed,span=0,k2;
                if(slow){gain>>=1;if(!gain)gain=1;span=slow;}
                else if(haste){gain+=gain>>1;span=haste;}
                // Base Speed is at most50. Haste51..75 needs at most two ticks
                // from any CT below100, so the existing51-entry table suffices.
                k2=gain>50?(100-ct>gain?2:1):
                    (u16)(((u32)(u16)(100-ct+gain-1)*ct_recip[gain])>>15);
                if(span&&k2>span)k2=span;
                tick+=k2;ct+=k2*gain;
                slow=slow>k2?slow-k2:0;haste=haste>k2?haste-k2:0;
            }
            if (tick >= 256) break;
            ct = ct >= 200 ? ct - 200 : ct - 100; // source AT keeps Quick255's55 remainder
            k = (u16)(tick << 8 | (100 - ct));
            for (j = n; j > 1 && tk[j - 1] > k; j--)
                if (j < ORDER_N) { tk[j] = tk[j - 1]; st.turns[j] = st.turns[j - 1]; }
            if (j >= ORDER_N) break;               // later than every turn kept
            tk[j] = k; st.turns[j] = i;
            if (n < ORDER_N) n++;
        }
    }
    for (; n < ORDER_N; n++) st.turns[n] = 0xFF;
}

// ORDER's mod_entry follows the status runtime in fft.c.

