// Item commands are archived records512..639; source selector ids stay382..393.
#ifndef FFT_THROW_RULES_H
#define FFT_THROW_RULES_H
#if MOD_CORE
static u8 __attribute__((noinline)) throw_available(u8 who,AbId ab)
{
    const Unit *u=&st.unit[who];const Ability *b=&abilities[ab];
    return b->kind!=255&&u->set==20&&b->rate<12&&(u->learned&((LearnBits)1<<b->rate))&&
        (unit_home_team(u)==TEAM_ENEMY?u->level>=b->lv:battle_save.inv[b->bag]!=0);
}
#else
#define throw_available(who,ab) can_use(who,ab)
#endif
#endif
