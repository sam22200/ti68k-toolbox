// The management menus' rules (manage.c) on a party saved by a Gariland victory
// (x/check/ffsave_won.sav, campaign check.py), headless.
#include "manage.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
static u8 find(u16 *list, u8 n, u16 v) { u8 i; for (i = 0; i < n; i++) if (list[i] == v) return i; return 0xFF; }
int main(void)
{
    SaveUnit *r, *c;
    Stats s;
    u8 what, i, before;
    if (access("ffmg.bin", R_OK)) assert(!chdir("data"));
    remove("ffsave.sav");
    {   // the fixture save as this program's own
        FILE *f = fopen("../x/check/ffsave_won.sav", "rb"); assert(f);
        assert(fread(&save, 1, sizeof save, f) == sizeof save); fclose(f);
        assert(rt_save(SAVE_NAME, &save, sizeof save));
    }
    sw_init(0); assert(sw_step(0));
    assert(bank && save.n == 3);
    r = unit(0); c = unit(1);
    assert(r->type == 2 && career(r) == 0 && c->job == 0x4B && career(c) == 1);
    // levels and unlocks: Ramza 258 JP earned as a Squire: level 2, Knight and Archer open
    assert(job_level(r, 0) == 2 && unlocked(r, 2, RT_NULL) && unlocked(r, 3, RT_NULL));
    assert(!unlocked(r, 4, &what) && what == 2);           // Monk: Knight first
    assert(!unlocked(r, 18, &what) && what == 0xFF);       // Dancer: women only
    assert(!unlocked(c, 2, &what) && what == 0);           // the Chemist's Squire: 169 JP, level 1
    // the battle's statistics, from the same records
    s = stats(r); assert(s.move == 5 && s.jump == 3 && s.hp > 30);   // Squire Move 4, Battle Boots +1
    // a job change keeps what the job can equip, takes off the rest into the inventory
    before = save.inv[r->eq[EQ_WEAPON]];
    change_job(r, 3);                                     // Archer: no sword
    assert(r->job == 0x4D && r->eq[EQ_WEAPON] == 0 && save.inv[0x13] == before + 1 && r->gfx == UG_RAMZA);
    change_job(c, 3); assert(c->gfx == 0xFF && picture(c, 3) == 6);   // a recruit: the Archer's
    assert(!can_equip(r, EQ_WEAPON, 0x13));
    change_job(r, 0); assert(r->job == 0x01 && r->gfx == UG_RAMZA);
    equip(r, EQ_WEAPON, 0x13); assert(r->eq[EQ_WEAPON] == 0x13 && save.inv[0x13] == before);
    // learning: Ramza's JP pay for the job's next ability
    {
        u16 left = jp(r, 0); u8 k, done = 0;
        for (k = 0; k < 22 && !done; k++) {
            const u8 *j = job_rec(record(r, 0));
            if (b16(j + J_ABIL + 2 * k) && !learned(r, 0, k) && b16(j + J_COST + 2 * k) <= left) {
                assert(learn(r, k) && learned(r, 0, k) && jp(r, 0) == left - b16(j + J_COST + 2 * k)); done = 1;
            }
        }
        assert(done);
    }
    // the slots: the Chemist knows Squire actions (a secondary), Equip Change (a support)
    choices(c, 0); assert(find(mg.pick, mg.pick_n, 0x05) != 0xFF);
    choices(c, 2); assert(find(mg.pick, mg.pick_n, 0x1E0) != 0xFF);
    choices(c, 1); assert(mg.pick_n == 1);                 // no reaction learned
    // the menus: Formation -> menu -> Change Job -> Knight -> yes, then ESC writes the save
    sw_init(0);
    assert(sw_step(K_C)); assert(sw_step(0)); assert(mg.screen == S_MENU);
    for (i = 0; i < 3; i++) { assert(sw_step(K_DOWN)); assert(sw_step(0)); }
    assert(sw_step(K_ENTER)); assert(sw_step(0)); assert(mg.screen == S_JOBS);
    for (i = 0; i < 2; i++) { assert(sw_step(K_DOWN)); assert(sw_step(0)); }
    assert(sw_step(K_ENTER)); assert(sw_step(0)); assert(mg.confirm);
    assert(sw_step(K_ENTER)); assert(sw_step(0)); assert(unit(0)->job == 0x4C);
    assert(sw_step(K_B)); assert(sw_step(0)); assert(sw_step(K_B)); assert(sw_step(0));
    assert(!sw_step(K_ESC));
    {
        Save back; assert(rt_load(SAVE_NAME, &back, sizeof back) && back.unit[0].job == 0x4C);
    }
    remove("ffsave.sav");
    puts("Management: levels, unlocks, statistics, job change, equipment, learning, slots and the save");
    return 0;
}
