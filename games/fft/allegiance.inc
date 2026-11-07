#ifndef FFT_ALLEGIANCE_RUNTIME_INC
#define FFT_ALLEGIANCE_RUNTIME_INC
// Owned social effects: local to PERFORMANCE on the TI, shared portable source.
static void unit_allegiance(Unit *u,u8 team,u8 permanent)
{
    if(team<=TEAM_GUEST){
        if(!(u->allegiance&ALLEGIANCE_ORIGIN))u->allegiance|=ALLEGIANCE_ORIGIN|(unit_home_team(u)==TEAM_ENEMY?ALLEGIANCE_ENEMY:0);
        if(permanent){
            // Invite copies the source camp and clears permanent player control.
            team=team==TEAM_ENEMY?TEAM_ENEMY:TEAM_PLAYER;
            u->allegiance=(u->allegiance&(48|UNIT_TRANSPARENT_ARMED))|ALLEGIANCE_INVITED|((u->allegiance&ALLEGIANCE_CHARM)?ALLEGIANCE_CHARM|team:0);
        }else if(!(u->allegiance&ALLEGIANCE_CHARM)){
            u->allegiance=(u->allegiance&~7)|ALLEGIANCE_CHARM|u->team;
        }
        u->team=team;
    }
}

#endif
