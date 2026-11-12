// Check fixtures write synthetic records into the monster ability interval (0x100..0x16F),
// which only a monster may use (can_use). The caster becomes a monster whose set holds the
// record: fixture_monster.py writes the monster sets 0xB0.. four records by four.
// Call it before the cast (check_batch.c wraps its cast entries).
#ifndef FIXTURE_MONSTER_H
#define FIXTURE_MONSTER_H
static void fixture_monster(u8 a,AbId ab)
{
    if(a<NUNIT&&(u16)(ab-0x100)<0x70U){
        Unit *u=&st.unit[a];u->job=94;u->set=(u8)(0xB0+((ab-0x100)>>2));u->learned=(LearnBits)1<<(ab&3);
    }
}
#endif
