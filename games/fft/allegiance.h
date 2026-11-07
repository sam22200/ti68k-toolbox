// Current battle camp and permanent ownership are different under Charm.
#ifndef FFT_ALLEGIANCE_H
#define FFT_ALLEGIANCE_H
enum { ALLEGIANCE_CHARM=4, ALLEGIANCE_INVITED=8, ALLEGIANCE_ENEMY=16, ALLEGIANCE_ORIGIN=32, UNIT_TRANSPARENT_ARMED=64 };
static inline u8 unit_home_team(const Unit *u)
{
    return u->allegiance&ALLEGIANCE_CHARM?u->allegiance&3:u->team;
}
static inline u8 unit_original_enemy(const Unit *u)
{
    return u->allegiance&ALLEGIANCE_ORIGIN?(u->allegiance&ALLEGIANCE_ENEMY)!=0:u->team==TEAM_ENEMY;
}
static inline u8 unit_party_member(const Unit *u)
{
    return unit_home_team(u)==TEAM_PLAYER;
}
#endif
