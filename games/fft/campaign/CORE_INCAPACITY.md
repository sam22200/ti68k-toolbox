# Ordinary elimination and Petrify target validity

Historical checkpoint. The current shared physical hit gate rejects an already
petrified ordinary Attack target before RNG. The2026-11-05 integration reproduced
the old preparation-RNG assertion failure with preserved ABI69 sources and
updated that fixture to check this existing native gate explicitly. The retained
finalizer/resource tests below still apply; this is not a new claim of complete
original physical RNG parity. See CORE_WEAPON_REACH.md.

Follow-up 2026-10-29: CORE_CLEANSE.md enables Soft, Esuna and Remedy to
remove a mutable Petrify bitmap using the source cancellation set. HP and cell
remain fixed while controls, clocks and turn order refresh. Retail Petrify
infliction, its special actor presentation and broader persistence remain gated;
the older boundaries below describe the original incapacity checkpoint.

The engine now separates occupying a tile, accepting an ordinary effect and
counting toward team survival. A petrified actor retains HP, identity and tile
occupancy while being excluded from ordinary elimination counts and AT turns.
An absent actor with positive HP cannot keep a battle alive or enter the AT list.

Petrify infliction/cure, persistence and visual presentation remain gated. No
spell, item, status producer or new artwork is enabled by this milestone.
This is not complete Petrify or full scenario-objective support.

## Original observations

`tools/incapacity_oracle.py` runs unchanged original MIPS team-outcome and
prepared-target-finalizer instructions.30 ordinary outcomes cover alive, Dead,
Petrify, Crystal, Treasure, Sleep, Stop, Blood Suck, Traitor, absent and Jump;
guest survival and simultaneous defeat/victory conditions are included. The
original checks defeat first and uses original team flags, not effective Charm
allegiance. The port currently supports its declared player/guest/enemy camps;
future allegiance producers must preserve original-team identity separately.

The original KO membership mask is60c5000000: Crystal, Dead, Petrify, Blood Suck,
Traitor and Treasure. Stop/Sleep/Jump are not elimination by themselves. Reading
these masks does not implement the remaining status handlers or producers.
Source `entd_slot==255` maps to the native off-field convention; native HP-owned
ordinary KO represents source Dead. Source coordinate/existence/script-object
lifecycle semantics are not fully represented by that adapter yet.

Four original finalizer fixtures retain prepared damage/hit with and without
Petrify cancellation. A petrified target without a cancelling effect becomes a
miss and its prepared HP delta is cleared. The cancelling branch is source-only
evidence for a future cure family. Graphical target sorting is isolated; no
original physical-RNG, HP commit/reward or complete spell trace is claimed.

Five original importance3 objective cases are recorded separately. Disabled
important actors and special provisional-loss masks are future campaign work;
they are not silently treated as implemented objective metadata. The existing
mandatory-Ramza permanent-loss rule is preserved in the current campaign.
All source inputs, masks, function/disc hashes and decompilation revision are in
`x/core-incapacity/oracle/source.json`.

## Shared native rules

- `unit_on_field` checks coordinates, compact node/count and the existing
  Crystal/chest/gone lifecycle. It is not a replacement for tile occupancy.
  Compact node255 remains valid when count256; counts remain16-bit.
- Ordinary elimination reads source KO membership in addition to native HP and
  presence. Petrify never sets HP0, starts a death counter or removes the actor.
- `unit_petrified` rejects ordinary hit forecasts, potential Attack damage and
  AI usefulness. Original scalar formula08 validity already rejects that target.
- Physical/item/ordinary-skill preparation retains existing resource/RNG work;
  the impact finalizer makes a statue a miss before HP/stat/status/self-cost,
  knockback, rewards or reactions. A queued reaction is revalidated before use.
  The existing weapon-splash path also leaves adjacent statues untouched.
  This does not certify physical RNG parity with the original; it preserves the
  native preparation sequence while applying the observed final validity rule.
- AT filtering excludes absent/object actors without conflating mutable
  Stop/Sleep wait or the distinct source Blood Suck/Traitor rules with Petrify.

Ordinary outcome counting moves into ORDER1, beside ORDER0's turn forecast.
Only resident callers load it after action/status modules return; the query
does not advance CT, change State/RNG or build the turn list. This keeps the
resident program within its packer limit instead of adding a predicate loader
call per actor. ABI37/ModCtx372/signature65171 rejects36;17 images remain split8/9.
ORDER grows to1248, ACTION to6316 (new largest execution zone, +52).
Core State/Work/Unit/FSV3 and the five heap requests are unchanged. Full combined
resident-code/zone/stack/heap certification remains open.

TURN grows6 bytes and BEATS/RESULTS/AREA_AI have same-size compiler changes
from unused inline declarations. Isolated controls reproduce all four with
the previous rule bodies; seven other images change only their ABI signature.
See `x/core-incapacity/declaration-control.json`. ORDER assembly has no nested
module-loader call.

Private43-variable package518827 bytes (+245): ffbcore-79, ffbm+20, ffbn+304.
Raw shards40664/39940; all28 non-code banks and archive attributes are unchanged.
Resident input64765 shrinks38 bytes, leaving171 below64936, including the52-byte
execution-zone increase. Archive size and heap requests are not total peak RAM.

## Scoped checks and limits

`core-incapacity-check`:193 PC/ASan/UBSan/68000 checks55D44C55 compare the source outcomes/finalizer subset and actual
native entry points. It covers live-HP stone enemies, both sides incapacitated,
guest survival, absent/object actors, retained occupancy/HP/death counter,
forecast/AT purity, normal versus stone preparation RNG, item payment, no
self-cost/recoil, splash, queued Counter cancellation, actual AI ranking and
13-unit cold/warm outcome queries. A synthetic count256/node255 versus count255
boundary checks the new presence query without regenerating map geometry.
The injection door now stops when the initial formation is terminal and returns
before scenario-specific setup can replace M_END. A deliberately clipped1x1
door reproduces both failures. The binding fixture restores real map dimensions
after its abstract1x1 context test, before entering playable scenario setup.

The affected closure is primary actions/reactions, scheduler/tempo, owned-area
CT/restarts, magical validity/AI/preview adapters, ABI bindings, old action frames,
fresh layouts and both pilot save/result flows. Magical integration uses
`check_magic_evasion.py --integration-only`: reuse unchanged238-profile evasion
and scalar evidence, rather than rerun its unrelated full matrix. No geometry,
Jump, catalogue or broad bitmap census is needed. Native builds are serial.

Native measurements: ordinary outcome average2206/max2530 cycles across
30 cases;13-actor query cold15922, warm5172; terminal turn entry51212. Existing
13-actor wake now peaks318536 backend cycles because outcome/AT and status work
can change overlay residency. This is not a full tactical-frame, stack or total
residency pass. No hardware path or new live TiEmu test is involved.

Closure: primary actions295/EDCEBFAE, reactions267/EF8107A7, scheduler263/
3056BFD2, tempo748/30A798F0, magical integration159/E8EE3DB3 and area CT353/
11D6FCB4 plus5 Orbonne retry checks/00000230 pass. Parser130/00000047,162 exact
context checks/7DF38F11,34 cold/warm preloads,3 stale retries and7 rejected
startups include old ABI36 rejection. Fresh20-field layouts,12 historical
action/response frame hashes and Gariland81/AE9CFD31 plus Orbonne14/2CD70391
match, with settled saves, no duplicate rewards and trophy overflow checked.

Package, final fixture counts/hashes, image comparisons and closure results are
recorded in `x/core-incapacity/verification.json`. Standard playtest, live TiEmu,
user saves and all earlier gallery bytes and dates stay unchanged. Broader
statuses/response/targeting/AI, actual Petrify producers, original objective
metadata, campaign, formats/editors and worst-case budgets remain open. Jump is
deferred. Full core completion and mass content production remain gated.
