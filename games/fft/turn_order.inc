// FFT's AT list: the active unit, then each living unit's next turns, its clock run alone
// (no bonus at their end), sorted by tick then by the CT reached (the higher first), a tie in
// unit order. At each turn change only; reciprocal products skip constant-speed
// segments. Haste/Slow need at most three segments, never a per-tick simulation.
u8 turn_forecast(u8 *out, u8 depth)
{
    u16 tk[ORDER_DEEP];
    u8 i, j, n = 1, t;
    Unit *u;
    out[0] = st.act; tk[0] = 0;
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
        for (t = 0; t + 1 < depth; t++) {
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
                if (j < depth) { tk[j] = tk[j - 1]; out[j] = out[j - 1]; }
            if (j >= depth) break;                 // later than every turn kept
            tk[j] = k; out[j] = i;
            if (n < depth) n++;
        }
    }
    for (j = n; j < depth; j++) out[j] = 0xFF;
    return n;
}

// The six an ordinary turn change forecasts.
void turn_order(void) { turn_forecast(st.turns, ORDER_N); }

// The Turn window's page (fft.c, M_ORDER), kept in this cold image so the resident
// carries none of it. The whole list is made on the stack and only the six rows shown
// are kept, so the battle holds no second list and Work keeps its measured size. The
// window's own cache key carries the page and, above it, the whole list's length.
// Returns the cell the selected row's unit stands on; the resident moves the cursor.
u8 turn_page(u8 row)
{
    u8 top = (u8)(row < ORDER_N ? 0 : row - ORDER_N + 1), i;
    if ((u8)W->win_k[1] != top) {
        u8 deep[ORDER_DEEP], n = turn_forecast(deep, ORDER_DEEP);
        for (i = 0; i < ORDER_N; i++) st.turns[i] = (u8)(top + i) < n ? deep[top + i] : 0xFF;
        W->win_k[1] = (u16)(n << 8 | top);
        W->win_k[0] = 0;                       // its rows are other units now
    }
    return UNIT_CELL(&st.unit[st.turns[row - top]]);
}

// [F2] / [F4]: the previous or next unit, FFT's START button (manual p.9, battlefield:
// "Cursor will point to units not associated with friendly/enemy forces"; with a menu
// open it does nothing, as there it points at no unit). The source's own unit list
// groups the player's units before the enemy's rather than following the encounter's
// order, so the walk is ranked the same way: the enemy's side after every other, each
// side in unit order, and it closes on itself. The original walks it one way, we walk
// it both. Returns the cell to move the cursor to.
#define CYCLE_RANK(i) ((s16)((st.unit[i].team == TEAM_ENEMY ? NUNIT : 0) + (i)))
u8 unit_cycle(u8 forward)
{
    u8 i, n = unit_at_cell(CURSOR_CELL), best = 0xFF, wrap = 0xFF;
    s16 here = n ? CYCLE_RANK(n - 1) : forward ? -1 : (s16)(2 * NUNIT);
    for (i = 0; i < NUNIT; i++) {
        s16 r;
        if (st.unit[i].x >= MAP_W || st.unit[i].z >= MAP_H || DC_OBJ(st.unit[i].dc)) continue;
        r = CYCLE_RANK(i);
        if (forward) {
            if (r > here && (best == 0xFF || r < CYCLE_RANK(best))) best = i;
            if (wrap == 0xFF || r < CYCLE_RANK(wrap)) wrap = i;
        } else {
            if (r < here && (best == 0xFF || r > CYCLE_RANK(best))) best = i;
            if (wrap == 0xFF || r > CYCLE_RANK(wrap)) wrap = i;
        }
    }
    if (best == 0xFF) best = wrap;
    return best == 0xFF ? (u8)CURSOR_CELL : (u8)UNIT_CELL(&st.unit[best]);
}

// ORDER's mod_entry follows the status runtime in fft.c.

