# Encounter objectives: prepared rules and PC visual review

2026-11-13. **Not integrated.** The shipping engine still uses ordinary
elimination and its mandatory Ramza permanent-loss contract. The prepared
policy, bounded source study and four-view art comparison await the user's
visual verdict before descriptor/runtime/banner integration. ABI89/FCD5/FSV5,
production groups, saves and the user's emulator remain unchanged.

## Observed in original instructions

`tools/objective_oracle.py` executes12 HP-script cases and13 generic-importance
outcomes in the original US MIPS code on PC. HP unit lookup is isolated; the
comparison and generic outcome/status routines run unmodified. This is not an
authored event-chain or full action/Reraise-order replay.

`battle_script_check_scenario_condition` condition6 tests a resolved unit's HP
at or below a threshold; threshold0 succeeds at0 HP, fails at1 HP, and a missing
unit fails the condition. Condition5 with threshold1 supplies the complementary
living-unit test. Identity/misc-slot binding itself is outside this fixture.

`battle_action_check_battle_outcome` has separate importance3 handling before
ordinary elimination. It uses **initial** team flags and existence/status
state, not simply current allegiance or an important unit's HP. A named enemy
merely having Dead does not force a generic win while another enemy lives;
the scripted HP condition is distinct. Disabled/permanently lost important
units do decide the generic outcome. Simultaneous important blue/red loss
returns defeat first. Do not implement scripted protection by counting all
allies, or treat a guest flag as an objective on its own.

## Prepared target policy

`objective_rules.h` is an allocation-free helper, currently unused by fft.c.
It accepts validated encounter-slot masks, including slot0: one optional named
target and any explicitly protected allies. Overlapping/out-of-range masks,
zero/excess capacities and multiple target bits are rejected.

At an existing **settled** outcome boundary:

1. Preserve mandatory loss and give protected KO/permanent loss priority.
2. A named target reaching0 HP or explicit permanent removal wins, regardless
   of unrelated living enemies. If it still lives, ordinary victory cannot
   accidentally satisfy that objective; ordinary defeat still applies.
3. With no named target, ordinary elimination remains the win condition. A
   protection-only battle therefore also requires defeating the enemies.

This is an explicit native policy for the requested goal types, not a claim
that every retail scene uses this exact event priority. Hidden, airborne,
off-camera and not-yet-spawned are not synonyms for permanent removal. Bind
identities after party/fixed-guest setup; never select objectives at runtime
by enemy order. The review's first enemy/guest selections are visual fixtures,
not authored encounter metadata.

Eighteen PC/sanitizer boundaries cover legacy outcomes, named target slot0,
highest supported protected slot, invalid masks, a living named target with
all other foes incapacitated, permanent removal and simultaneous win/loss.
Actual action queues, Reraise, encounter binding, scripted aftermath, defeat
without rewards and exactly-once victory/save settlement remain integration
work after the visual verdict. No TI check is warranted for unused policy.

## Proposed presentation and reuse

`tools/objective_review.py` produces
`x/objectives/review/objective-before-after.png`: current elimination, named
target, protected ally and combined objectives, all at the same scale/view.
It reuses the map/actors, Lato title font/rasterizer and reviewed victory/defeat
strip bytes. The user rejected the first small-name labels and requested the
actual names directly in large text: `DEFEAT KEANE!` and `PROTECT DELITA!`.
The revised goals use cap12, the same as the existing Battle Start title;
fitting allows11/10 for longer lines, never the old small label font. Custom
goals replace the redundant Battle Start heading. Protection-only also shows
`DEFEAT ALL FOES!`; combined objectives keep both named goals. Only relevant
actors pulse. These are objective titles, not spoken dialogue; actual named
dialogue must continue to have portraits.

The tool compiles a temporary engine copy with a single focus-highlight
condition changed and supplies only temporary banner bytes. No production
source, shared bank, format, allocation or timing policy is modified. The
single fully revealed frame is a visual proposal, not tested banner timing or
measured TI performance. Do not integrate, optimize or benchmark the new art
until approved. Mark the comparison with ti-view's Decision badge; clear only
that marker after a verdict. Other pending gallery decisions remain intact.

## Next integration, after approval

- Add validated encounter metadata and compatible package/version guards.
  Keep the default pilot elimination behavior and existing slots/party saves.
- Call the policy only at coherent outcome boundaries, after relevant action/
  reaction/automatic-revival handling; do not end on a temporary intermediate KO.
- Bind target/protect names and pulse masks from that same metadata. Share
  universal goal strips in core and retain encounter identity/content by ID;
  never duplicate whole UI or portrait banks per arena.
- Preserve banner fit/reveal/hold time, save/reward idempotence and the360k
  ordinary frame budget. Keep stack, archive, executable-size and RAM growth
  explicit; the16KiB gate is already open and difficult recovery deferred.
- Use focused PC/sanitizer event/save flows first. Only changed native layout,
  ABI/overlay, arithmetic and rendering/budget paths need selected68000 checks;
  no whole sweep or TiEmu session per edit.

```sh
rtk proxy make -C games/fft/campaign campaign-objectives-check
rtk proxy tools/pyenv/bin/python games/fft/tools/objective_review.py
```

The rejected small-label comparison is retained under
`x/objectives/review-small-labels/`; only the revised comparison is marked for
Decision. The large-title version still needs its visual verdict.

The separate [breeding/Bar/Fur Shop readiness extension](CAMPAIGN_SIDE_SYSTEMS.md)
remains required before campaign extraction, alongside the rest of
CAMPAIGN_PREREQUISITES.md. Do not lose that queue while completing objectives.
