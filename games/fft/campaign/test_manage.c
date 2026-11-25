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
    assert(bank && save.n == 5);
    r = unit(0); c = unit(3);
    assert(r->type == 2 && career(r) == 0 && c->job == 0x4B && career(c) == 1);
    // levels and unlocks: Ramza 242 JP earned as a Squire: level 2, Knight and Archer open
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
        assert(learn(&chemist, 1, 13) && learned(&chemist, 1, 13) && jp(&chemist, 1) == 210);
        assert(!learn(&chemist, 1, 13));                  // already learned: no second charge
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
                assert(learn(r, 0, k) && learned(r, 0, k) && jp(r, 0) == left - ability_cost(job_ability(j, k))); done = 1;
            }
        }
        assert(done);
    }
    // the slots: the Chemist knows Squire actions (a secondary), Equip Change (a support)
    choices(c, 0); assert(find(mg.pick, mg.pick_n, 0x05) != 0xFF);
    choices(c, 2); assert(find(mg.pick, mg.pick_n, 0x1E0) != 0xFF);
    choices(c, 1); assert(mg.pick_n == 1);                 // no reaction learned
    // the tabs: Formation -> the unit's sheet -> Job tab -> Knight -> yes, then ESC saves
    sw_init(0);
    assert(sw_step(K_RIGHT)&&mg.slide==4&&sw_step(0)&&mg.slide==3);
    for(i=0;i<4;i++)assert(sw_step(0));
    assert(!mg.slide&&mg.unit==1);assert(sw_step(K_LEFT)&&sw_step(0));
    for(i=0;i<4;i++)assert(sw_step(0));
    assert(mg.unit==0&&!mg.slide);
    assert(sw_step(K_ENTER)); assert(sw_step(0)); assert(mg.screen == S_EQUIP);
    assert(sw_step(K_RIGHT)); assert(sw_step(0)); assert(mg.screen == S_SET);
    assert(sw_step(K_RIGHT)); assert(sw_step(0)); assert(mg.screen == S_JOBTABLE);
    assert(sw_step(K_RIGHT)); assert(sw_step(0)); assert(mg.screen == S_JOBS);
    assert(sw_step(K_LEFT)); do { assert(sw_step(0)); } while (mg.spin);        // the wheel keeps its arrows
    assert(mg.screen == S_JOBS);
    assert(sw_step(K_B)); assert(sw_step(0)); assert(mg.screen == S_FORM);
    assert(sw_step(K_ENTER)); assert(sw_step(0)); assert(mg.screen == S_EQUIP);
    for (i = 0; i < 3; i++) { assert(sw_step(K_RIGHT)); assert(sw_step(0)); }
    assert(mg.screen == S_JOBS);
    for (i = 0; i < 2; i++) { assert(sw_step(K_DOWN)); do {assert(sw_step(0));}while(mg.spin); }
    assert(sw_step(K_ENTER)); assert(sw_step(0)); assert(mg.confirm);
    assert(sw_step(K_ENTER)); assert(sw_step(0)); assert(unit(0)->job == 0x4C);
    assert(mg.job_anim);for(i=0;i<JOB_CHANGE_FRAMES;i++)assert(sw_step(0));assert(!mg.job_anim);
    assert(sw_step(K_B)); assert(sw_step(0)); assert(mg.screen == S_FORM);
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
    assert(sw_step(K_ENTER));assert(sw_step(0));assert(mg.screen==S_EQUIP);
    for(i=0;i<6;i++){assert(sw_step(K_F5));assert(sw_step(0));}
    assert(mg.unit==15);
    assert(sw_step(K_B));assert(sw_step(0));assert(mg.screen==S_FORM&&mg.top==2);
    // Remove Unit: the source returns the equipment to the bag and closes the gap.
    {
        u8 n0 = save.n, id, before_count;
        u8 target = 1;
        char keep[13];
        const char *nm = unit_name(unit(2));
        for (i = 0; i < 12 && nm[i]; i++) keep[i] = nm[i];
        keep[i] = 0;
        id = unit(target)->eq[EQ_HEAD];
        before_count = id ? save.inv[id] : 0;
        assert(!roster_remove(&save, 0) || unit(0)->type != 2);   // Ramza stays
        assert(roster_remove(&save, target));
        assert(save.n == n0 - 1);
        if (id) assert(save.inv[id] == before_count + 1);
        assert(!strcmp(unit_name(unit(1)), keep));                // the next unit moved up
    }
    // Order Unit: Level ranks the strongest first, Number leaves the roster alone.
    {
        u8 k;
        for (k = 0; k < save.n; k++) save.unit[k].level = (u8)(k + 1);
        for (i = 0; i + 1 < save.n; i++) {
            u8 best = i, j2;
            for (j2 = (u8)(i + 1); j2 < save.n; j2++) {
                Stats a = stats(&save.unit[best]), b = stats(&save.unit[j2]);
                if (roster_key(&save, j2, ORD_LEVEL, b.hp, b.mp) <
                    roster_key(&save, best, ORD_LEVEL, a.hp, a.mp)) best = j2;
            }
            if (best != i) roster_slot_swap(&save, i, best);
        }
        for (i = 0; i + 1 < save.n; i++) assert(save.unit[i].level >= save.unit[i + 1].level);
    }
    // Inventory: Eqp. keeps what this unit could wear, All the whole bag.
    {
        u8 all, fit;
        memset(save.inv, 0, sizeof save.inv);
        save.inv[0x13] = 2; save.inv[0x9D] = 1; save.inv[0xFD] = 3;   // sword, hat, Phoenix Down
        mg.unit = 0; mg.top = mg.row = 0;
        mg.inv_all = 1; build_inventory(unit(0)); all = mg.inv_n;
        mg.inv_all = 0; build_inventory(unit(0)); fit = mg.inv_n;
        assert(all == 3 && fit < all && fit >= 1);
    }
    // Best fit: never worse than what the unit wears, and the bag stays consistent.
    {
        u8 k, held[5];
        u16 before, after;
        memset(save.inv, 0, sizeof save.inv);
        save.inv[0x13] = 1; save.inv[0x1D] = 1; save.inv[0x9D] = 1;
        for (k = 0; k < SLOTS; k++) held[k] = unit(0)->eq[k];
        before = mg_fit(unit(0));
        best_fit_unit(unit(0));
        after = mg_fit(unit(0));
        assert(after >= before);
        for (k = 0; k < SLOTS; k++) if (unit(0)->eq[k] && unit(0)->eq[k] != held[k])
            assert(save.inv[unit(0)->eq[k]] < 99);
        remove_all_equipment(unit(0));
        for (k = 0; k < SLOTS; k++) assert(!unit(0)->eq[k]);
    }
    // Optimum Equipment follows the source's per-slot criterion, not one aggregate:
    // with the whole item table in the bag -- where the two rules really diverge,
    // a bag of a few Gariland items cannot tell them apart -- nothing the unit
    // could wear in a ranked slot beats what it ended up wearing there.
    {
        static const u8 ranked[4] = { EQ_WEAPON, EQ_SHIELD, EQ_HEAD, EQ_BODY };
        u8 kept[256], i, id, seen = 0;
        SaveUnit *u = unit(0);
        memcpy(kept, save.inv, sizeof kept);
        for (id = 1; id; id++) if (!save.inv[id]) save.inv[id] = 1;
        remove_all_equipment(u);
        best_fit_unit(u);
        for (i = 0; i < 4; i++) {
            u8 slot = ranked[i];
            u16 worn = u->eq[slot] ? slot_rank(slot, u->eq[slot]) : 0;
            for (id = 1; id; id++) {
                if (!save.inv[id] || !can_equip(u, slot, id)) continue;
                assert(slot_rank(slot, id) <= worn);
                if (slot == EQ_SHIELD && slot_rank(slot, id)) seen = 1;
            }
        }
        assert(seen);          // the shield slot really had ranked candidates
        memcpy(save.inv, kept, sizeof kept);
    }
    // The Remove Unit screens: the check, the unit's own answer, then the party is shorter.
    {
        u8 n0;
        mg.unit = 0;
        while (mg.unit < save.n && unit(mg.unit)->type == 2) mg.unit++;
        assert(mg.unit < save.n);
        n0 = save.n;
        mg.screen = S_FORM; mg.row = 0;
        assert(sw_step(K_C)); assert(sw_step(0)); assert(mg.screen == S_MENU);
        assert(mg.row == 0);
        assert(sw_step(K_ENTER)); assert(sw_step(0)); assert(mg.remove == 1);
        assert(sw_step(K_ENTER)); assert(sw_step(0)); assert(mg.remove == 2);
        assert(sw_step(K_DOWN)); assert(sw_step(0));            // Quit
        assert(sw_step(K_ENTER)); assert(sw_step(0));
        assert(!mg.remove && save.n == n0);
        assert(sw_step(K_ENTER)); assert(sw_step(0)); assert(mg.remove == 1);
        assert(sw_step(K_ENTER)); assert(sw_step(0)); assert(mg.remove == 2);
        assert(sw_step(K_ENTER)); assert(sw_step(0));
        assert(!mg.remove && save.n == n0 - 1 && mg.screen == S_FORM);
    }
    // The job table opens on the unit's own jobs and leads to that job's abilities.
    {
        mg.unit = 0; mg.screen = S_EQUIP; mg.row = 0; mg.top = 0; mg.ljob = career(unit(0));
        for (i = 0; i < 2; i++) { assert(sw_step(K_RIGHT)); assert(sw_step(0)); }
        assert(mg.screen == S_JOBTABLE);
        assert(jobtable_count(unit(0)) >= 2 && jobtable_entry(unit(0), 0) < CAREERS);
        assert(sw_step(K_ENTER)); assert(sw_step(0));
        assert(mg.screen == S_LEARN && mg.ljob == jobtable_entry(unit(0), 0));
        assert(sw_step(K_B)); assert(sw_step(0)); assert(mg.screen == S_JOBTABLE);
    }
    puts("Management: levels, unlocks, statistics, job change, equipment, learning, slots,\n            Remove Unit, Order Unit, Best fit, the inventory, the job table and the save");
    return 0;
}
