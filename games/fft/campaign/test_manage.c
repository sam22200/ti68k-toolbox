// The management menus' rules (manage.c) on a party saved by a Gariland victory
// (x/check/ffsave_won.sav, campaign check.py), headless.
#include "manage.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include "fixture_save.h"
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
        assert(read_save_fixture(f,&save)); fclose(f);
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
    assert(!job_visible(r,4));                         // no one has discovered Monk
    { u16 old=tjp(c,2),sq=tjp(c,0);set16(c->tjp[2],200);set16(c->tjp[0],200);
      assert(job_visible(r,4)&&!unlocked(r,4,RT_NULL));  // known by another unit only
      set16(c->tjp[2],old);set16(c->tjp[0],sq); }
    // the battle's statistics, from the same records
    s = stats(r); assert(s.move == 5 && s.jump == 3 && s.hp == 51);   // this native party's generated HP
    {
        SaveUnit original = *r;
        // Original Gariland oracle (oracle.h): raw HP 0x07E86D, same job/equipment,
        // 52 HP. The native seed drew 0x07C272 (51); neither loses HP in the save.
        original.raw[0][0] = 7; original.raw[0][1] = 0xE8; original.raw[0][2] = 0x6D;
        assert(stats(&original).hp == 52);
    }
    {
        SaveUnit chemist = *c;
        // An action above 255, beyond the old battle's six-entry lists: learn the
        // Chemist's slot 13 (Phoenix Down), paying its shared FFT cost of 90 JP.
        memset(chemist.learned[1], 0, sizeof chemist.learned[1]);
        set16(chemist.jp[1], 300);
        assert(job_ability(job_rec(1), 13) == 0x17D);
        assert(learn(&chemist, 13) && learned(&chemist, 1, 13) && jp(&chemist, 1) == 210);
        assert(!learn(&chemist, 13));                  // already learned: no second charge
    }
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
            if (job_ability(j, k) && !learned(r, 0, k) && ability_cost(job_ability(j, k)) <= left) {
                assert(learn(r, k) && learned(r, 0, k) && jp(r, 0) == left - ability_cost(job_ability(j, k))); done = 1;
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
    assert(sw_step(K_RIGHT)&&mg.slide==4&&sw_step(0)&&mg.slide==3);
    for(i=0;i<4;i++)assert(sw_step(0));
    assert(!mg.slide&&mg.unit==1);assert(sw_step(K_LEFT)&&sw_step(0));
    for(i=0;i<4;i++)assert(sw_step(0));
    assert(mg.unit==0&&!mg.slide);
    assert(sw_step(K_C)); assert(sw_step(0)); assert(mg.screen == S_MENU);
    for (i = 0; i < 3; i++) { assert(sw_step(K_DOWN)); assert(sw_step(0)); }
    assert(sw_step(K_ENTER)); assert(sw_step(0)); assert(mg.screen == S_JOBS);
    for (i = 0; i < 2; i++) { assert(sw_step(K_DOWN)); do {assert(sw_step(0));}while(mg.spin); }
    assert(sw_step(K_ENTER)); assert(sw_step(0)); assert(mg.confirm);
    assert(sw_step(K_ENTER)); assert(sw_step(0)); assert(unit(0)->job == 0x4C);
    assert(mg.job_anim);for(i=0;i<JOB_CHANGE_FRAMES;i++)assert(sw_step(0));assert(!mg.job_anim);
    assert(sw_step(K_B)); assert(sw_step(0)); assert(sw_step(K_B)); assert(sw_step(0));
    assert(!sw_step(K_ESC));
    {
        Save back; assert(rt_load(SAVE_NAME, &back, sizeof back) && back.unit[0].job == 0x4C);
    }
    remove("ffsave.sav");
    // Dense pages: all 16 units, five columns, last row and portrait selection.
    save.n=PARTY_MAX;for(i=3;i<PARTY_MAX;i++)save.unit[i]=save.unit[i%3];
    mg.screen=S_FORM;mg.unit=mg.top=0;
    for(i=0;i<15;i++){assert(sw_step(K_RIGHT));assert(sw_step(0));}
    assert(mg.unit==15&&mg.top==2);
    assert(sw_step(K_UP));assert(sw_step(0));assert(mg.unit==10);
    assert(sw_step(K_LEFT));assert(sw_step(0));assert(mg.unit==9&&mg.top==1);
    assert(sw_step(K_ENTER));assert(sw_step(0));assert(mg.screen==S_STATUS);
    for(i=0;i<6;i++){assert(sw_step(K_F5));assert(sw_step(0));}
    assert(mg.unit==15);
    assert(sw_step(K_B));assert(sw_step(0));assert(mg.screen==S_FORM&&mg.top==2);
    puts("Management: levels, unlocks, statistics, job change, equipment, learning, slots and the save");
    return 0;
}
