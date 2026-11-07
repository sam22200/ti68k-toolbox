# Arithmeticks / Math Skill

2026-11-02. The eight Calculator learning entries414..421 now select an
attribute (CT, Level, EXP, Height) and a predicate (Prime,5,4,3), followed by a
learned eligible spell. Generic coverage is189/201. The remaining12 entries are
Jump, explicitly deferred by the user. This does not complete the full core or
release gate.

## Rules and original evidence

`tools/math_oracle.py` executes unmodified retail calculator targeting, target
listing and unit-height functions on Gariland's actual terrain, with thirteen
controlled humans and all sixteen criteria. It also captures magical-evasion
profiles and the original Reflect/Counter guards. Function/disc hashes and the
controlled external RNG/chance boundaries are recorded in the local oracle JSON.
These are bounded function trials, not an original whole-menu/AI/campaign replay.

CT is capped at100 and forced to100 for the caster. Zero never qualifies;1 is
not prime. Half-unit walking heights never qualify. Height uses the source raw
terrain, including source Float/water elevation and same-panel status borrowing,
independently of the renderer's standing-height convention. All camps participate;
ordinary range, radius, centre-height, self and team restrictions are bypassed.
The source first marks panels, then lists their eligible occupants, so another
occupant can be included even when its own value fails the criterion. Absent,
Crystal, Treasure, Jump and mounted actors are excluded. The existing native
human model does not implement original Chocobo/mount or equipped water movement.

Math is immediate, costs no MP and permits Silence. Frog and Don't Act retain
the shared gates. It bypasses Reflect and Counter eligibility; a Calculator's
ordinary Attack still permits Counter. Magical evasion retains source order,
rounding and RNG, including zero-evade rolls. Forecasts retain the established
native pure-query policy even though original previews draw RNG. Effects,
status/element/support filtering, KO/rewards and once-only settlement use the
same frozen shared packets as ordinary spells.

The eligible spell sets are source10/11/12/16 (White,Black,Time,Yin-Yang), and
eligibility is range-record byte5 bit0x40. There are45 eligible spells; Fire4
(id19) is deliberately excluded. Learned spells for roster actors are read from
the existing four SaveUnit career rows. Actors without a roster learning row
use the native JP<=level*20 acquisition policy, explicitly an adaptation.
The native AI evaluates learned criteria/spells incrementally, keeps all16
spell-independent numeric masks in the existing32-byte centre-bitmap scratch,
and chooses the current panel as the Math command origin; it does not reproduce original future-turn/movement or
full Calculator AI ranking.

## Shared representation and budgets

FCJD retains512 source metadata/name/learning rows and640 normalized records.
The8 Math rows are selectors, and reject direct commands without a spell and
criterion. There are195 nonempty source rows,20 Throw/Math selectors,255 executable
commands,129 reused ordinary cast records and189/201 generic entries. No virtual
Math ability IDs are saved. The extra316 bytes are a256-byte numeric predicate
lookup and60-byte source career/list-position lookup, read in place from Flash.
The catalogue grows43358 to43674 bytes and rejects the previous FCJC contract.

The existing CastBatch.follow byte128 identifies Math; ref packs the two
2-bit choices. State/Work/Unit/Save/FSV3, pending slots and all five allocation
formulas remain unchanged. Manual input reuses M_SKILLS/sub and the approved
shared target/confirmation menu. Cache invalidation happens on stage transitions;
Math stage/page rows finish on the first rendered frame, with their additional
work charged against the existing frame budget. Direct preview entry avoids
building the same global footprint twice. Known speakers retain the common portrait path.

ABI57 uses a368-byte ModCtx after removing unused portrait/gauge/opening_battle
callbacks and the unused job_names binding, and deriving the single cold PARTY
job_sets lookup directly from the catalogue. Pure resident target_query returns ordinary validity or
a Math mask, so an executing image never loads another image. TURN1/2/3 own
Math input, Attack range and global reach; TARGETS10 returns dispatch codes
before those loads. BEATS4 calculates camera targets after its caller returns;
easing remains resident. SCREENS12 reuses bootstrap catalogue validation, removing
the duplicate SAVE5 entry. Native/PC camera calculations share one C body and
handedness is unchanged.

The18 images retain9+9 shards and a8254-byte static execution zone. Banks are
63990/63888 bytes, leaving only10/112 below the64000 archive limits. Image sizes:

```
8250 4518 7406 7244 7910 8246 6550 6176 7568
8156 8226 5652 7610 8254 4614 8254 5192 7808
```

## Verification

`core-math-check` passes1360 logic assertions9163135B plus238 UI assertions
138E34BC, equal on PC, ASan/UBSan and68000. The88 UI pictures cover all three
selector stages, paging/return, global targets, confirm/cancel, ordinary Attack
and Status. Four bounded native carriers retain the full aggregate PC check;
recombination includes the separate picture-digest step per frame. Independent
fixtures reset chapter Work as well as State. Four private PC captures were
read to check names, fitted margins and visible first-frame menus.

Production raw variable64809 bytes; the actual TTPack input is64811 (its two-byte
length prefix included), leaving125 below64936. A private packed build succeeds.
Unit112/State2210/Work45576/Save3138 and all five allocation formulas compare
exactly with the previous Throw checkpoint.

Original evidence includes96 footprints,12 magical-evasion profiles, five invalid
selector pairs and the Reflect/Counter dispatch guards. All45 eligible spells
prepare and settle through the shared owned packet; native AI execution also
covers both zero-radius eligible spells (Holy/Flare), not only radial magic.
Forecasts, learned criteria/spells, MP/Silence/Frog/Don't Act, duplicate impact,
Counter retention for ordinary Attack and shared source acquisition are checked.

The same native checker binary and fixture measure the frozen Math AI at
15150562 cycles before numeric-mask reuse and4838708 after (68.1% less), with
all1360 assertions and the same digest. This is a controlled three-live-actor
fixture, not a worst-case whole-battle certificate. Numeric-mask queries peak
89714 cycles; owned preparation161630 and marked impact156034. UI update/render
zones are separate: preview/confirmation render peaks exceed360k, so the full
interactive/cold-overlay budget remains open. Existing Throw Stone pictures and
the prior approved area pictures are retained; source R/S/M and full original
UI/AI/campaign fidelity remain separate.

The ordinary Throw Stone control retains61 assertions/3EA15536 and36 pictures.
A private per-frame probe measures update plus render together, including image
loads: its eleven warm idle skill-menu frames peak at358116 cycles; initial
entry reaches2606182, and subsequent transitions also exceed360k. These bounded
control frames distinguish warm menu costs from the still-open cold-overlay
gate; they do not certify every interactive frame.

All31 scoped older suites pass, including a final area-impact replay after
repairing its oversized native transport. Its1012 assertions/12D9C238 and64
approved pictures/F9BACFF5 remain unchanged. Six final cache-dependent suites
also pass. The loader rejects the actual previous ABI56 export and FCJC
catalogue; both private chapter flows pass81/AE9CFD31 and14/2CD70391.
All514 protected save/group files retain their hashes and modification dates;
111 of120 data files remain byte-identical, with only the catalogue pairs,
code-bank pairs and generated module header changing.

Final assertion, pixel, size, layout, source rollback and preservation evidence
is recorded in `x/core-math/verification.json`. Tests use private PC directories
and native VAT/save paths. Live TiEmu, the standard campaign group and user
saves are not used; nothing is committed or pushed. Full-frame and cold-overlay
costs remain separate from function-zone cycle measurements.
