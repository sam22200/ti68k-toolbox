// NATURE-owned cold effects. The resident returns here only after prior
// crystal/chest and movement-benefit messages have finished; no nested overlays.
static void trap_level_down(u8 who)
{
    Unit *u=&st.unit[who];
    const Job *j=&jobs[u->job];u8 k;u16 hp=u->hp,mp=u->mp;
    if(!(u->raw[0]&RAW_CURRENT))for(k=0;k<5;k++)
        u->raw[k]=core_raw_grown(u->raw[k],j->growth[k],u->level);
    if(u->level>1)u->level--;
    // Even level1 loses raw growth. EXP is retained; the source draws five times.
    for(k=0;k<5;k++){rand15();u->raw[k]=core_raw_level(u->raw[k]&RAW_MAX,j->growth[k],u->level,1);}
    u->raw[0]|=RAW_CURRENT;
#if MOD_CORE
    unit_stats(u);
    u->hp=hp<u->max_hp?hp:u->max_hp;u->mp=mp<u->max_mp?mp:u->max_mp;
#else
    // The resident rebuilds through EQUIPMENT after NATURE returns. Never
    // evict this executing image by calling the PARTY/stat callback here.
    (void)hp;(void)mp;W->rebuild|=(u16)1<<who;
#endif
}
static void trap_needle(u8 who)
{
    Unit *u=&st.unit[who];u16 n=u->max_hp/5U,id=st.reaction[who];u8 redirected=0;
    if(n&&reaction_live(who)){
        if(id==REACTION_MP_SWITCH&&u->mp)redirected=reaction_chance(u->brave,rand15());
        else if(id==REACTION_SPLIT)rand15(); // staged by the source, acting slot skipped during drain
    }
    if(redirected)u->mp=n>=u->mp?0:u->mp-n;
    else{
        status_damage(who);u->hp=action_hp(AB_DAMAGE,u->hp,u->max_hp,n);
        if(!u->hp){u->kos++;status_knockout(who);return;}
        // Source post-impact checks still consume their roll, even at zero
        // damage/Brave. All queued self responses are skipped by retail drain.
        if(reaction_live(who)&&(reaction_stat(id)||id==REACTION_REGENERATOR||
           id==REACTION_POTION||id==REACTION_GILGAME||
           (reaction_critical(id)&&((u->statuses.innate[2]&1)||
             (!(u->statuses.immune[2]&1)&&u->hp<=u->max_hp/5U)))))rand15();
    }
}
static u8 trap_apply(u8 who,u8 map)
{
    const u8 *row;Unit *u=&st.unit[who];u8 i,trap;
    row=move_find_row(who,map,&i);if(!row)return 0;
    trap=core_trap_id(row[1]);if(trap==TRAP_NONE)return 0;
    if(trap==TRAP_DEGENERATOR)trap_level_down(who);
    else if(trap==TRAP_STEEL_NEEDLE)trap_needle(who);
    else{
        CoreStatusDelta d;memset(&d,0,sizeof d);d.add[4]=trap==TRAP_DEATHTRAP?1:16;
        core_status_apply(&u->statuses,&d,u->status_flags,0,cj_status_rules(common),u->status_ct);
        unit_status_refresh(u);status_clock_update(who);
        if((st.pending[who].state==DEFER_CHARGING||st.pending[who].state>=DEFER_CAST_TILE)&&
           !(core_status_current(&u->statuses,0)&8))st.pending[who].state=DEFER_NONE;
        if(st.pending[who].state==DEFER_PERFORMING&&!(core_status_current(&u->statuses,0)&1))st.pending[who].state=DEFER_NONE;
    }
    status_changed();W->trap_message=(u8)(trap+1);W->win_k[0]=0;
    st.abil=NO_ABIL;st.pick=254;st.pick_t=1;st.mode=M_PICK;
    return 1;
}
