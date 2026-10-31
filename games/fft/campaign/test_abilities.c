// Shared source IDs and sixteen action bits, exercised through the real battle.
#include "gariland.c"
#include "../oracle.h"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    // The old original-game fixture uses this compact ordering; the engine no
    // longer does. Its observed previews remain an independent formula oracle.
    static const AbId source_id[9] = { 0x93, 0x94, 0x95, 0x98, 0x170, 0x176, 0x177, 0x178, 0x17D };
    u8 i;
    Unit *u;
    if (access("ffgd.bin", R_OK)) assert(!chdir("data"));
    remove("ffsave.sav");
    sw_init(61);
    assert(sizeof(AbId) == 2 && sizeof(LearnBits) == 2 && sizeof(Ability) == 12 && sizeof(UnitDef) == 28);
    u = &st.unit[U_CHEMIST];
    assert(u->set == 6 && u->job == 0x4B);
    u->learned = 0xA000;               // source position 13: Phoenix Down; bit 15 must survive too
    assert(known(U_CHEMIST, 0) == 0x17D && known(U_CHEMIST, 1) == NO_ABIL);
    assert(can_use(U_CHEMIST, 0x17D));
    assert(!can_use(U_CHEMIST, 0x100) && !can_use(U_CHEMIST, NO_ABIL));
    assert(!skill_hit(U_CHEMIST, U_RAMZA, 0x100));
    assert(!skill_amount(U_CHEMIST, U_RAMZA, 0x100, 0));
    save_campaign();
    u->learned = 0;
    party_load();
    assert(u->learned == 0xA000 && known(U_CHEMIST, 0) == 0x17D);
    remove("ffsave.sav");

    for (i = 0; i < ORACLE_SKILLS; i++) {
        AbId ab;
        Unit *actor, *target;
        sw_init(61);
        actor = &st.unit[U_RAMZA]; target = &st.unit[10];
        actor->zodiac = 9; target->zodiac = 3; actor->female = target->female = 0;
        actor->max_hp = actor->hp = 52; actor->pa = 5; actor->sp = 6;
        target->max_hp = 44; target->hp = oracle_skills[i].hp; unit_status_seed(target,oracle_skills[i].status);
        if (oracle_skills[i].abil == 0xFF) {
            assert(hit_chance(U_RAMZA, 10) == oracle_skills[i].hit);
            continue;
        }
        ab = source_id[oracle_skills[i].abil];
        assert(skill_hit(U_RAMZA, 10, ab) == oracle_skills[i].hit);
        if (oracle_skills[i].hit && abilities[ab].kind != AB_CURE) {
            u16 lo = skill_amount(U_RAMZA, 10, ab, 0), hi = skill_amount(U_RAMZA, 10, ab, 32767);
            assert(oracle_skills[i].amount >= lo && oracle_skills[i].amount <= hi);
            if (abilities[ab].kind == AB_DAMAGE) assert(!(oracle_skills[i].amount % lo));
        }
    }
    // A command sentinel must never index the ability bank, including its effects renderer.
    sw_init(61); defend(U_RAMZA); weapon_effect();
    assert(st.abil == ABIL_DEFEND);
    puts("Abilities: source IDs, action bits 13/15, save/load, unavailable formulas, Defend and 13 original previews");
    return 0;
}
