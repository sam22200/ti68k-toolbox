// Shared encounter generation, overlaid with campaign saves on the TI.

#ifdef FFT_CHAPTER
// Unused capacity is never a tactical occupant or a persistent party member.
// Active loops use NUNIT; clearing spare slots also makes repeated loads safe.
static void clear_spare_units(void)
{
    u8 i;
    for(i=NUNIT;i<UNIT_CAPACITY;i++) {
        Unit *u=&st.unit[i];memset(u,0,sizeof *u);
        memset(st.affinity[i],0,5);
        u->x=u->z=u->gfx=u->slot=255;u->dc=DC_GONE;
    }
}
static void unit_reward_fields(void)
{
    u8 i;const u8 *p;
    if(!CD(UNIT_REWARDS))return;
    p=cdesc+CD(UNIT_REWARDS);
    for(i=0;i<NUNIT;i++){st.unit[i].war_trophy=*p++;st.unit[i].money_modifier=*p++;}
}
static void fixed_units(void)
{
    u8 i,k;
    W->cast.phase=CAST_BATCH_EMPTY; // opening/retry replaces the action owner too
    clear_spare_units();
    for (i=0;i<NUNIT;i++) {
        const UnitDef *d=&unit_defs[i];Unit *u=&st.unit[i];
        memset(u,0,sizeof *u);
        u->x=d->x;u->z=d->z;u->gfx=d->gfx;u->job=d->job;u->team=d->team;u->face=d->face;
        u->cell=(u8)(d->z*MAP_W+d->x);
        u->level=u->lv0=d->level;u->brave=d->brave;u->faith=d->faith;u->female=d->type==1;
        u->zodiac=unit_zodiac[i];u->set=d->set;u->learned=DEF_LEARN(d);u->dc=3;
        for(k=0;k<5;k++){u->eq[k]=d->eq[k][0];u->raw[k]=RD32(igfx+CD(UNIT_RAW),(u16)(i*10+2*k));}
        unit_stats(u);
    }
    unit_reward_fields();
}
#endif
void battle_units(void)
{
#ifdef FFT_CHAPTER
    W->cast.phase=CAST_BATCH_EMPTY; // scenario/retry replaces the action owner too
    clear_spare_units();
    if (CD(UNIT_RAW)) { fixed_units(); return; }
#endif
    u8 i, k;
    for (i = 0; i < NUNIT; i++) {
        const UnitDef *d = &unit_defs[i];
        Unit *u = &st.unit[i];
        u16 day = d->day;
#ifdef FFT_CHAPTER                     // the descriptor's big-endian records
        day=RD16((const u8*)d+6,0);
#endif
        u->x = d->x; u->z = d->z; u->gfx = d->gfx; u->team = d->team; u->face = d->face;
#ifdef FFT_CHAPTER
        u->cell=(u8)(d->z*MAP_W+d->x);
#endif
        u->job = d->job; u->level = d->level; u->ct = 0; u->dc = 3; u->female = d->type == 1;
        u->brave = d->brave ? d->brave : (u8)(45 + ((30UL * rand15()) >> 15));
        u->faith = d->faith ? d->faith : (u8)(45 + ((30UL * rand15()) >> 15));
        if (!day) day = (u16)(1 + ((365UL * rand15()) >> 15));
        for (k = 0; k < 12 && day >= ZODIAC_DAY(k); k++) ;
        u->zodiac = (u8)((k + 9) % 12);
        for (k = 0; k < 5; k++)
            u->raw[k] = ((u32)gen_base[d->type][k] << 14) + (((u32)rand15() * gen_var[d->type][k]) >> 1);
        for (k = 0; k < EQ_N; k++)                 // one of the two candidates (FFT: of n)
            u->eq[k] = d->eq[k][d->eq[k][1] != 0xFF ? rand15() >> 14 : 0];
        unit_stats(u);
    }
    for (i = 0; i < NUNIT; i++) {                  // drawn after the stats (theirs kept per seed)
        const UnitDef *d = &unit_defs[i];
        Unit *u = &st.unit[i];
        u->set = d->set; unit_status_seed(u,0); u->learned = DEF_LEARN(d);
        if (DEF_LEARN(d) == DRAW_LEARNED) {                  // its job's JP, then each ability in order:
            u16 jp = (u16)(100 + ((100UL * rand15()) >> 15));   // its learn rate, if JP are left
            LearnBits mask = DEF_DRAW(d);
            u->learned = 0; u->tjp = jp;
            for (k = 0; k < SET_N; k++) if (mask & (1U << k)) {
                AbId ab = SET_ABILITY(d->set, k);
                if (HAS_ABILITY(ab) && ((100UL * rand15()) >> 15) < abilities[ab].rate && jp >= AB_JP(&abilities[ab])) {
                    jp -= AB_JP(&abilities[ab]); u->learned |= (LearnBits)(1U << k);
                }
            }
            u->jp = jp;
        }
    }
    for (i = 0; i < NUNIT; i++) {                  // then the JP of those whose abilities are
        const UnitDef *d = &unit_defs[i];          // fixed (Ramza), the EXP (ENTD: 0..99; a new
        Unit *u = &st.unit[i];                     // game's Ramza 0)
        if (DEF_LEARN(d) != DRAW_LEARNED) u->jp = u->tjp = (u16)(100 + ((100UL * rand15()) >> 15));
        u->exp = d->type == 2 ? 0 : (u8)((100UL * rand15()) >> 15);
        u->kos = 0; u->lv0 = u->level; u->gexp = u->gjp = 0;
    }
    for (i = 0; i < NUNIT; i++) {                  // the support ability (not a new game's Ramza):
        Unit *u = &st.unit[i];                     // Defend bought with the Squire job's JP after
        u16 jp = u->jp;                            // the reactions and supports listed before it,
        u->defend = 0;                             // then one of the two it knows (what was learned
        u->sq_jp = u->sq_tjp = 0;                  // kept for the campaign save)
        u->sq_learned = u->rsm = 0;
        if (unit_defs[i].type == 2) continue;
        if (jobs[u->job].row) {                    // another job: the Squire's JP and abilities
            LearnBits mask = DRAW_BASIC;
            jp = u->sq_tjp = (u16)(100 + ((100UL * rand15()) >> 15));
            for (k = 0; k < SET_N; k++) if (mask & (1U << k)) {
                AbId ab = SET_ABILITY(SET_BASIC, k);
                if (HAS_ABILITY(ab) && ((100UL * rand15()) >> 15) < abilities[ab].rate && jp >= AB_JP(&abilities[ab])) {
                    jp -= AB_JP(&abilities[ab]); u->sq_learned |= (LearnBits)(1U << k);
                }
            }
        }
        for (k = 0; k < NRSM_PRE; k++)
            if (((100UL * rand15()) >> 15) < rsm_pre[k][1] && jp >= rsm_pre[k][0]) {
                jp -= rsm_pre[k][0]; u->rsm |= (u8)(1 << k);
            }
        if (((100UL * rand15()) >> 15) < DEFEND_RATE && jp >= DEFEND_JP) {
            u->rsm |= 0x80; u->defend = !(rand15() >> 14);
        }
        if (jobs[u->job].row) u->sq_jp = jp;
    }
    {
        u16 seed = (u16)(rt_seed ^ 0x5A17);
        for (i = 0; i < NUNIT; i++) {
            u8 j, clash;
            if (st.unit[i].gfx <= UG_DELITA) continue;
            do {
                seed = (u16)(seed * 109U + 89U);
                st.unit[i].name = (u8)seed;
                if (st.unit[i].name == 255) st.unit[i].name = 0;
                clash = 0;
                for (j = 1; j < i; j++) if (st.unit[j].gfx > UG_DELITA && st.unit[j].female == st.unit[i].female
                    && st.unit[j].name == st.unit[i].name) clash = 1;
            } while (clash);
        }
    }

#ifdef FFT_CHAPTER
    unit_reward_fields();
#endif
}
