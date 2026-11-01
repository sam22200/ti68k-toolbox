# Validation by affected dependency

Run closures with `closure.py` (`make` per suite is unchanged): the
core-*-check suites run in parallel (`-j 12`, 20 cores here: about 6.5 min for
all 37 instead of tens of minutes), each under strace; a suite whose recorded
inputs (every workspace file opened or looked up, absent paths included) are
byte-identical to its last green run is skipped (`x/closure-cache/`, private).
A shared-core change still reruns every suite (each binary changes); a check,
oracle or doc change reruns only its own suites. `--first a,b` schedules suites
first, `--fail-fast` stops at the first red, `--force` ignores the cache (after
a toolchain change). banks/headroom run LeakSanitizer, which cannot run under
ptrace: they rerun untraced and are never cached. `ti-cc` now takes a lock
(tigcc writes .s/.o next to shared sources such as runtime/core/rt_core.o).
Validated: the first parallel run gave the serial run's hashes for all 37. `--all` adds the 26
older core-*-check suites (63 in all, about 2.5 min more); five of them were red
before 2026-11-01 (identical with or without Guts) and are repaired:
casting/incantation asserted Death still gated (enabled since CORE_TERMINAL;
Float is the gated control now); incantation's 68000 binary was 2136 bytes over,
it now leaves casting_trials out natively (core-casting runs them) and a PC twin
with the same define must print the same digest (429 checks, 08918B2C; full PC
799, FD122519); elements expected the reaction Potion's kind before
restore_prepare ran at launch (CORE_RESTORATION); magic assumed one AI draw,
now it replays ai_choose's ranking draw (one per ranked action until one passes);
jump's oracle modelled only the middle obstruction: it now also applies the
source's landing rule (battle_move_propagate_destination 8/9) and steep-panel
rule (init_source_panel / propagate_destination 3), and counts walk-edge
differences separately (core-step/surface): 536 and 544 changed jump edges agree.

Current equipment breaks closure (2026-11-01): `closure.py --all`, 66 suites
green; `core-equip-breaks-check` 3255 (0AF4C46E) joins the standard list. Every
hash equals the stat breaks closure except `actions` (2ECB73A6: the four new
records). The opening package's generated `ffcj.89y` must be rebuilt with the
catalogue (`make -C ../opening data/ffcj.89y`): `cell` loads it and the
validator now requires the item levels.

Historical stat breaks + chapter-loading closure (2026-11-01): `closure.py --all`,
65 suites green in 471 s; `core-breaks-check` 2016 (AE26B19D) joins the standard
list. Every hash equals the Steal Exp closure except `actions` (F091239D: the
two new break records and Speed Break's source record). banks/headroom/cell
check ModCtx ABI 47 (388 bytes: `rt_file`, `chapter_ugfx`); `check_banks.c`
expects them. Steal Exp's profiles now unequip the thief (the source rows are
bare-handed; Martial Arts is bare-handed only): hash unchanged.

Historical Guts + module rebalance closure (2026-11-01): `core-guts-check`
1546 (F8F2AD6B); all 37 suites green through `closure.py`, every hash equal to
the Spin Fist closure except `actions` (42026132: the three Guts entries only,
E0988BD3 without them). banks/headroom updated for ModCtx ABI 46 (384 bytes,
two shared helpers); their hashes are unchanged.

Historical Spin Fist closure: `core-punch-strikes-check` 6524 (BC005771);
all 37 family suites green (x/core-punch-strikes/spin-closure.txt), hashes
equal to the Talk closure except punch-art (AFA2F059, its gate list) and
`actions` (E0988BD3: the Spin Fist catalogue entry only; FCF28AB1 without it).
area-impact is green after `cast_area_supported` dropped a redundant 0x31
kind test (68000 binary back under the limit).

Historical Talk Skill closure: CORE_TALK_SKILL.md / `core-talk-skill-check`
3916 assertions (707B1244), PC = sanitizers = 68000, against 301 original
formula 0x2a profiles; with Death Sentence 4616 (4C2389A4), 344 profiles and
12 upkeep traces. 36-suite closures green; `actions` changes (904B320D, then
FCF28AB1) only through the Talk entries (A1B25B4E without them). area-impact's 68000
binary was 16 bytes over: it now also defines FFT_PRODUCTION_CORE (it calls
none of those entries); its hashes are unchanged.

Historical Punch Art strikes closure: `core-punch-strikes-check` (Wave/Repeating
Fist) 4412 assertions (1AFD349F), PC = sanitizers = 68000; the source formula
draws are replayed, native evasion keeps Attack's model. The relocation
(chapter_grid -> PARTY10, FFT_PRODUCTION_CORE) was closed with 41 suites,
including boundaries/headroom/grid/full-grid/cell/step/compact/area-ui/
area-forecast; family suites keep their hashes, `actions` changes (A1B25B4E)
only through the strike entries (48E92F7E with them removed), `punch-art`
reruns at 1256/6DAFCC6A (its gate list). Three stale checks, failing
identically on the previous tree, were repaired: knockback (recoilbattle
reused one action context across variants; each variant now begins a fresh
root action: 97 checks, 7F384DFF), targets (its "unsupported family" fixture
used formula 0x0a, now the status family: 0x24; disabled-record probe 0x150:
back to FC02B091) and batch (Float 0x26 replaces the enabled Fire: back to
94558E80).

Historical Punch Art support closure: CORE_PUNCH_ART.md / `core-punch-art-check` checks
Stigma Magic, Chakra and Revive: 1258 assertions (E18144C2), PC = sanitizers =
68000, against 99 original formula 0x33/0x34/0x35 profiles (75 reachable rows
in the native header, to keep the 68000 check under the program limit).
Scoped closure reran the shared rule, AI, action, reaction, area and surface
suites (28 green); `actions` changes hash (48E92F7E) only through the enabled
catalogue entries: the same binary with the catalogue minus Punch Art gives A7E386D6.

Historical protection closure: CORE_PROTECT.md / `core-protect-check` checks
Reraise and Golem: 460 assertions, 28 original profiles and six Reraise
upkeep traces. area-impact defines FFT_CHECK_NO_ZONES to fit its 68000 binary.

Historical control closure: CORE_CONTROL_STATUS.md / `core-control-status-check`
checks Frog, Blind Rage, Confusion Song and Foxbird together: 1509 assertions,
PC = sanitizers = 68000, against 56 original formula0a/0x61 profiles. Scoped
closure reran the shared rule, AI, action, reaction, area and surface suites.
`actions` changes hash (9032AAA6 with Foxbird) only through the enabled catalogue entries:
the same binary with the previous catalogue gives 8F6F297F. `control` masks the
new lost-control bit out of its Stop oracle comparison. Repaired stale checks:
sleep (prepared item effect kind, weapon-skill splash), incapacity (Petrify is
enabled), area-impact (native binary fits after the resident relocation).

Historical decisive closure: CORE_TERMINAL.md / `core-terminal-check` checks
Death, Quick, Zombie and Petrify together. PC, sanitizers and 68000 agree on
1292/5DEE6FD1 rules and 410/C4AE09F8 edges (1702 total) against 56 original
formula/finalizer profiles and 12 upkeep traces. Scoped closure reran
restoration (2481), scheduler 263/3056BFD2, actions 295/8F6F297F, buffs/healing
on PC and bank bindings, plus both native pilot flows. Private report:
`x/core-terminal/verification.json` (18694 old paths, bytes and dates equal).

Historical resource closure: CORE_RESTORATION.md / `core-restoration-check`
checks seven real item producers together, five newly enabled. PC, sanitizers
and 68000 agree on 1683/AB2D4E2B resource assertions, 437/F9DBABDF boundaries
and 361/D10B5DD9 unchanged controls (2481 total). Reuse 80 original source
formula/finalizer profiles and 38 HP/MP store traces. Test frozen paired values,
Undead HP damage plus retained Elixir MP, source draw counts/caps, one stock
payment, no-effect rewards, response suspension, Auto-Potion upgrades, actual
MP-aware AI and action-start forecast invalidation. SCREENS owns cold input
without nested loads; resident loading requests and AI clock stay outside it.
ABI 43/376 requires prior ABI 42 export rejection, loader/bindings, 20 layouts,
12 old frames and both native pilot save/results flows. Actions/reactions,
cleansing, owned area work and pause are direct callers. After the final cache
fix, replay unchanged native carriers against final banks with zone 8172;
rebuild the changed resource carrier. Keep compilations serial. Reuse unchanged
geometry/bitmap/Jump evidence. Preserve live TiEmu, standard playtest, user saves
and old gallery/group bytes AND modification dates. Private final reports:
`x/core-restoration/verification.json` and `final-replays.json`.

Historical cleansing closure: CORE_CLEANSE.md / `core-cleanse-check` checks six
actual producers, 1711 PC/sanitizer/68000 assertions: 1296/92582E7A rules,
232/6A8077BD edges and 183/4421B5B6 relocated pixels. Reuse 78 unchanged-MIPS
cancellation/finalizer profiles for two spells and seven items; raw formula0x38
dispatch must match the actual original handler table. Test frozen five-byte
subsets packed in u16, the compiler's sixteen-bit bound, innate protection,
removal despite immunity, timers, Dead/Wall/absent/statue validity, one item
cost, no-effect items, Half MP/Silence, actual AI and a single Reflect endpoint.
NUMBERS uses a local pure evaluator, never the loading resident skill callbacks.
Exact controls cover 32 Wait-arrow cases and six new forecast boxes, retaining
plane pointers. ABI42 needs actual prior ABI41 rejection, 20 layouts, 12 old
frames and both native pilot save/results flows. Buffs/healing/percent effects,
actions/reactions/owned area impacts and banks are the direct closure. Reuse
unchanged bitmap-kernel/geometry/Jump inputs. Native builds remain serial.
The legacy buff fixture now exceeds the native variable cap by16 bytes: its
existing assertions run in rule/menu carriers. Recombine their rotating digests
and require the historical 5221/3C724297 result; this is test transport only,
without a shipping architecture or rule change. The actions aggregate enumerates
every enabled forecast: reuse its binaries with the prior catalogue to isolate
new records and require the old 295/0E56F883 digest. Preserve live TiEmu, standard
playtest/user saves and old gallery/group bytes AND modification dates.

Historical percentage/resource-transfer closure: CORE_PERCENT.md /
`core-percent-check` checks all five production entries in three bounded
carriers: 1710/82DE1E51 original paired-result assertions, 476/90D1C72C runtime
and 210/A45DC0D6 reflection/resource boundaries (2396 total). PC, sanitizers and
68000 must agree. Reuse 57 unchanged-MIPS oracle profiles; no full original AI
or live commit claim. Transfers need separate target/caster effects, geometric
versus effect masks, empty/miss/low-current/max999, Undead and actual MP-aware AI.
Pure shared MA/support/zodiac/element helpers require healing, buffs, Reflect,
elements and weather closure. Replay existing native carriers when only their
external code banks change. ABI 41 requires actual prior ABI 40 rejection,
20 layouts, 12 old frames and both native pilot save/result flows. SAVE5 must
validate a real catalogue and reject null/short input with Work null, before
heap allocation. No new heap block; mod_zone is static resident storage.
Actions/reactions/owned area impacts and banks are the direct callers. Keep
native compilations serial. Weather ranking learns only Fire to isolate its
rank; menu sizing checks the actual make_window path after compositor relocation.
Reuse unchanged geometry, status bitmap and Jump evidence. Preserve standard
playtest/live TiEmu/user saves and old gallery/group bytes AND modification dates.

Historical buff/status closure: CORE_BUFFS.md / `core-buffs-check` enables 22 source
formula0a/0b producers together. There are 5682 PC/sanitizer/native assertions
(5221/3C724297 + 461/E318A9A7);
67 controlled-RNG original formula/filter/finalizer profiles and 10 Protect
ordering cases. The 198 compatible states compare the optimized pure single-byte
query with the existing full bitmap filter. Check real learned lists, CT/MP,
Unit/Panel binding, immunity/innate/timers, AI, once-only commit, reflected
single endpoints and existing numeric-label cache. The changed sparse-byte
kernel also requires the existing 4843/262BF732 bitmap checks (177 source profiles); reuse unchanged geometry/Jump censuses.
The 28 seeded cached-menu parts must remain identical when RESULTS owns
composition; restore planes and do not nest overlay loads.
ABI 40/ModCtx 376 needs actual prior ABI 39 rejection, bindings, 20 layouts/12 old
frames and native pilot save/results flows. HP/Reflect/area/action/reaction
callers and physical Charge/hands are the affected closure. Preserve standard
playtest, user saves, live TiEmu and old gallery/group bytes AND timestamps.
The 13-victim impact improves 441270→296906 backend cycles, with unchanged
results and a two-byte smaller FORMULA image. This does not certify a complete
13-actor frame/stack/RAM peak.


Historical healing closure: `core-healing-check` checks8 retail producers,2110
PC/sanitizer/native assertions11072857,75 actual source formula/finalizer
profiles and32 exact relocated-banner plane comparisons. Reuse the unchanged
bitmap/geometry/Jump census. ABI39/ModCtx376 requires actual prior ABI38 export
rejection, binding162,20 layouts/12 old screens and both native pilot save/result
flows. Existing25 casts retain2736/77DAC288. Preserve all old proof bytes/dates;
no live TiEmu action. See CORE_HEALING.md for the affected dependency closure.

Current Reflect family closure: `core-reflect-check` exercises all25 production
casts,2736 PC/sanitizer/native checks77DAC288. Source oracle executes18 original
endpoints/four guards; do not regenerate unrelated bitmap/geometry/Jump censuses.
ABI38 requires loader rejection/binding,20 layouts/12 old frame hashes and both
pilot native save/results flows. Actions/reactions/owned CT/magical integration
are the dependency closure. Pure LOOKUP/math callbacks from RESPONSE must never
load another image. Preserve old proofs/images/dates. See CORE_REFLECT.md.

Current incapacity closure: CORE_INCAPACITY.md / `core-incapacity-check`:193
PC/ASan/UBSan/native checks55D44C55;30 original elimination cases/4 finalizers,
source-only cancellation/objective branches explicit. Check retained statue HP/
occupancy, final misses/no recoil/self-cost/reward, item payment/preparation RNG,
Counter revalidation, actual AI, off-field AT/outcome and node255/count256.
Pure ORDER1 queries must preserve State/RNG; terminal injection setup must stop
and preserve M_END. Restore the binding fixture's abstract1x1 dimensions before
playable setup. ABI37/17 images requires binding/rejection, layouts/12 old frames
and pilot saves. Direct actions/reactions/scheduler/tempo/owned area CT plus
`check_magic_evasion.py --integration-only`159/E8EE3DB3 cover callers while
reusing238 unchanged evasion profiles. No broad geometry/Jump/bitmap/catalogue
sweep. TURN/BEATS/RESULTS/AREA_AI compiler drift is declaration-only in isolated
controls. Native builds serial. Outcome15922/5172 and wake318536 are backend
blocks, not full-frame/stack/residency proofs. Petrify producers remain gated.

Historical tempo closure: CORE_HASTE_SLOW.md / `core-haste-slow-check`:748 PC/
ASan/UBSan/native assertions30A798F0,45 original gain profiles/4 charge phases/
33 AT lists. Exercise public request/cure/immune/cancel, rounding/precedence,
zero/odd/max Speed, expiry/Stop/Sleep, Slow1 idle/wake, Charge/KO/revive and13
actors. Check ORDER assembly: no nested loader/division/software32-bit math.
ABI36/17 images requires bindings/rejection, layouts/12 old frames/pilot saves;
scheduler/deferred/owned CT and public status adapters are the direct closure.
Use `check_status_state.py --integration-only` (58 checksE79EDE14), reusing the
unchanged bitmap oracle. Unsupported control now uses Reraise. Reuse unchanged
scalar/geometry/Jump/catalogue evidence. Native builds serial. Cold/warm AT
39086/29724, expiry81388, CT max8778 and wake266028 are backend blocks only;
full frame/stack/residency stays open. Compiler controls isolate same-size
RESULTS/AREA_AI drift from unused declarations.

Historical scheduler closure: `core-scheduler-check` checks263 PC/ASan/UBSan/native
assertions3056BFD2 against six original clock choices, two Quick interruptions,
three original stalled clocks and three AT lists. Test busy action ownership,
ordinary versus special CT, both freezes/timer underflow,13-actor wake, Speed0,
KO guests, absent/all-KO, resumed input/rotation/quit and dialog timer pause.
Two nonempty HUD hashesEECC35C3/4E934B27 match browse, with no new art/layout.
The first full-render checker exceeded transport size; test the only changed HUD
branch directly and reuse the12 ordinary/response full-frame certificates.
ABI35 requires fresh images/bindings/old-ABI rejection/layouts; deferred/control/
owned-CT/restarts/AI-pause and pilot save flows are the dependency closure.
TURN/AI_SETUP compiler drift is reproduced in isolated declaration-only builds;
FORMULA alone changes AT rules. No repeated scalar/geometry/Jump census. Idle8/
wake16 tick quanta bound the last wake to256892 backend cycles; do not label that
a full360K frame, stack or residency pass. See CORE_SCHEDULER.md.

Current approved impact closure: `core-area-impact-check` checks1012 actual
renderer assertions12D9C238,64 individual PC/68000 screens (aggregateD7A684DC),
and154 bounds assertionsC56EC8B9. The latter checks13 rows/two floors/node255,
signed quantities, deadline readiness, Oil, followed Crystal/Jump/vanished centres
and public preparation into Work. It is a number-backend picture, not a full
13-actor scene. The first suite uses a small portable fixture bank to stay below
the native checker size limit; no test data is shipped. Use canonical visible-byte
pixel hashes, not native-word planes_sum. The416 approved candidate captures match
actual staged rendering with the same fixture/cache history. ABI34/16 images and
scalar-number relocation require bindings, prior12 action/reaction frame hashes,
UI/owned-CT/batch/magic/AI/pause, fresh layouts and pilot save flows. No geometry/
Jump/source catalogue census. Keep full cold frames/stack/residency open.
Historical impact review: `area-impact-review` now returns accepted artifacts
read-only; approval.json records the verdict and the Decision badges are cleared.

Historical approved area-UI closure: CORE_AREA_UI.md / `core-area-ui-check` runs
499 PC/ASan/UBSan/native assertions67EED097, eight HUD hashes, four full tactical
frame/state replay checkpoints, and1321 relocated-camera checks4CC4400E. Exercise
real input/cancel/order paths, empty/no-follow/occupied centres,13 rows/node255/
two floors, pure inspection and position/status/stats/weather invalidation.
Because callbacks/modes/image split change, check bindings146/12CE8E3A, rejection
startups, forecast/CT/magic/area-AI/pause, twelve old frame hashes, fresh20-field
layouts and pilot victory/save flows. Do not rerun geography/Jump/catalogue
censuses. Native builds serial. UI228 aliases idle scratch, no heap/save growth;
ABI33/15 images/zone6264. HUD-only375252 and update279866 peaks do not certify a
360K full frame; interactive timing/stack/residency stay open. Accepted visual
prototype is read-only via approval.json; do not regenerate old review images.

Historical AI-pause closure: CORE_AI_PACING.md / `core-ai-pause-check` runs171
PC/ASan/UBSan/native assertions58CBDE4E. Inject elapsed ticks across a blocking
computation: zero/short/one-second/just-before/at/after deadline, wrap, repeated
same-clock fast calls, new turn, no-hold replan, cancellation/re-enable and frozen
planes. Compare one short ordinary-clock actual PC/native replay. Rebuild only
the resident battle for its package budget; verify all fourteen images/two code
shards byte-identical, reusing their mechanic/ABI/layout/save proofs. No full
geography/AI catalogue/campaign sweep follows a presentation-pause correction.
The user's frozen AI pause permits the measured one-second setup quantum; keep
full plan latency separate from a single quantum and interactive frame budgets.

Historical cached area-AI closure: CORE_AREA_AI.md / `core-area-ai-check` verifies
676 PC/ASan/UBSan/native assertionsD04607FA against12 bounded original HP/accounting
profiles, five ranks and three empty panels. Exercise actual planning/AI_AIM,
empty centres, friendly fire/absorption/nulls, unsupported policy, late MP rejection,
weather invalidation, forecast eviction/resume, compact-ID ties, AREA-only weapon
compatibility and13 actors/256 nodes/two floors including whole-map targeting.
Check restored unit bytes/no area RNG and cached formula-call count. Measure BOTH
general and area stages: new area max319968 cycles; general setup stage0 reaches
12059828, failing the360K gate. Do not call backend timing a full frame/residency
certificate. ABI32/fourteen images and shared ranking/forecast helpers require
forecast/owned-CT/magic-AI/actions/loader/pilot save-flow, twelve old frame hashes
and fresh layouts. No geometry/catalogue/Jump census or live TiEmu follows.

Historical area-forecast closure: CORE_AREA_FORECAST.md / `core-area-forecast-check`
checks359 PC/sanitizer/native boundaries against four original13-slot preview
groups. Reuse unchanged footprint/scalar/environment evidence. Check pure output,
READY/APPLIED/live-owner alias rejection, refresh/zero MP/empty/moving/fixed centres,
node255/two floors/four views and native cold/warm query cost. Shared quantity and
ABI31 affect batch/owned CT/magic-AI/loader/pilot callers: run those,12 old frame
hashes and fresh layouts; do not rerun the geography/Jump census. The old60K task
heuristic is not a full-frame certificate (warm query max86504). Native AI must
not call an evicting resident forecast wrapper from an executing image.

Historical resident split closure: CORE_HEADROOM.md / `core-headroom-check` runs
loader spans/rejection, all13 bindings, three poisoned-context same-chapter
retries and both pilot/13-actor initialization boundaries. ABI30 has unchanged
layouts: rebuild images, reject29, compare native/host sizes, owned-area lifecycle,
pilot save-flow and12 existing frames. No scalar/geometry/Jump sweep is needed.
SAVE must return before another image loads; initialize lengths after preload.
Check the whole private group as well as resident input:634 resident bytes saved
cost74 archive bytes. This does not close full-frame or combined residency gates.

Historical owned-area closure: CORE_AREA_CT.md / `core-area-ct-check` checks353
PC/sanitizer/native command/real CT/owner/automatic impact/response boundaries
(11D6FCB4) and5 Orbonne same-chapter restart cases (00000230). Keep the60-byte
Work packet intact through queries/frames/reactions and thirteen equal-tick casts.
Run the prior batch/incantation/due/action/Charge and pilot save-flow closure,
12 old frame hashes, fresh Work/State/save layouts and near packet offsets. This
changes Work/ABI29, not catalogue data/scalars/geometry: do not rerun their sweeps.
Orbonne retry is a separate process to respect the software16-file cache limit.
No retail area spell, new visual approval or full source dispatcher trace implied.

Historical archive-bank closure: CORE_BANKS.md / `core-banks-check` checks the new
FMB3 envelope before executing images:3 offline partition/token/capacity tests,
130 PC/sanitizer/native span/parser checks,112 native full ModCtx byte comparisons
against the prior initializer, all13 images/two chapter-size sets/warm input, and
six real native startup failures. Verify every image differs only by its ABI28
signature, both matching export tokens and all package inputs include ffbn.
Then check affected batch/actions/Charge/pilot save-flow,12 old frame hashes and
fresh layouts. Reuse unchanged scalar/geometry/census/Jump evidence. Measure cold
copy and warm calls separately; the new template copy is not free. Preserve all
existing gallery bytes/dates and the live playtest group.

Historical compound-cast closure: CORE_BATCH.md / `core-batch-check` compares thirteen
original compound groups (including13 present actors) and1202 PC/sanitizer/68000
checks94558E80. Verify all preparations before HP, mixed eligibility/evasion/
absorption/nulls/Oil, exact-cost one MP payment, replay rejection, self-KO, empty/
null-only sets and actual two-response/reward drain. Shared magic preparation/
commit and ABI27 affect weather, targets, responses/actions/Charge and both pilot
save-flows; rerun those plus12 frame hashes/layouts/bank rejection. Reuse unchanged
geography/affinity/catalogue certification. No UI/producer or full original AoE
reaction/reward trace is claimed. Preserve playtest and gallery bytes/dates.

Historical radial-target closure: CORE_TARGETS.md / `core-targets-check` executes79
original footprints/42 column boundaries and1193 PC/sanitizer/68000 assertions.
Run pure membership/rotation/compact-ID/overlap/follow probes, native cold/warm
queries, fresh layouts and ABI26/thirteen-image bank validation. Then rerun direct
weather/AI/packet/forecast, actions/Charge and both pilot save-flows, with12 existing
frame hashes. No effect/RNG-order integration is claimed by an unordered mask.
Reuse unchanged geometry/affinity/catalogue/status evidence; keep gallery dates.

Historical environment closure: CORE_WEATHER.md / `core-weather-check` uses254
original profiles and6493 PC/sanitizer/68000 checks. Test sequential signed
narrowing, PS1 Oil's unchanged damage/protected removal, Float null, real due/
AI/response boundaries and pure queries. Native State grows2 bytes, packet flags
and ABI25 change shared semantics: rebuild all images, reject24 and probe layouts.
Compare the unchanged forecast drawing body/four cache hashes and12 action hashes.
Reuse unchanged FCJA/affinity/geography/movement evidence; do not rerun their full
sweeps. Cold forecast full preparation can exceed the existing60k HUD estimate;
keep C07/L4 timing accounting open rather than labelling a formatter-only zone
as a whole task/frame. Scope and limits: CORE_WEATHER.md.

Current magical-evasion closure: CORE_MAGIC_EVASION.md / `core-magic-evasion-check`
uses238 original profiles and7510 PC/sanitizer/native assertions. FCJA changes
only the header/packed item bytes, so rerun the shared reader and affected
management/deployment save-flow paths. ABI24 changes the context, so rebuild all
images and check rejection of23; compare twelve existing action frame hashes.
Do not repeat geography/census/Jump sweeps. Run the pure percentage/sequential
RNG/late-target fixtures before enabling a new spell or evasion producer.

2026-10-25 user decision. Do not rerun the entire core audit for a local edit.
The large existing suites are certification evidence; unchanged modules keep
that evidence. Exhaustive counts are not a milestone.

| Change | Default evidence | Expand when |
|---|---|---|
|Owned area lifecycle|`core-area-ct-check`:353 PC/sanitizer/native command/CT/automatic impact checks plus5 Orbonne retries, Work packet lifetime/one payment/replay/queries/reactions and13 equal-tick roots|New producer: source-order encounter mapping, area forecast/AI policy, reviewed per-target/empty presentation and remaining source reaction/status/reward families; layout/loader change: targeted ABI/frame/flow probes|
|Executable archive partition|`core-banks-check`:FMB3 parser130 PC/sanitizer/native cases,112 actual context checks,6 native startup rejections, deterministic code/order/token and package checks|New module/layout/loader/format: corresponding parser and actual affected callback/flow/frame boundaries; no full geography/scalar audit for a packaging-only change|
|Compound formula08 effect stage|`core-batch-check`:13 source compound groups,1202 PC/sanitizer/native checks;60-byte caller packet, one-cost prepare/all HP later, replay guards, actual Auto-Potion drain and pure rejection|Live area producers: durable owner, source-order encounter format, CT/command/AI/impact routing, reviewed per-target presentation; broader source reaction/reward/status traces|
|Ordinary radial target sets|`core-targets-check`:79 original footprints/42 columns,1193 PC/sanitizer/native checks; presence, fixed/follow coordinates, panel teams, one-layer/ties, whole-map,13 actors/256 nodes, four rotations and State/RNG purity|Before live activation: source effect-slot mapping, one MP payment, all formula results before HP commits, per-target reactions/rewards and reviewed presentation|
|Elemental outcomes|`core-elements-check`:77 original formula/finalizer/signed-HP-prefix profiles, modifier order, caps/wrap, live absorbed/nullified casts, no damage-only wake/reaction on healing, prepared category/packet transport and actual native AI;1525 PC/sanitizer/68000 assertions|Magic evasion, target eligibility, weather/Oil/Float/mount/monster/Undead, area target sets or production spell presentation|
|Affinity data/cache|`core-affinity-check`:178 original job/equipment aggregations, cache replacement/scratch Units, malformed FCJ9 references, real13-slot stat rebuilds, local ACTION/RESPONSE level-up ownership, party reload;1881 PC/sanitizer/68000 assertions|Elemental effects, monster gear policy, stat/equipment producers or formats change: original typed effect and smallest affected lifecycle fixture|
|Magical scalar|`core-magic-check`:104 original Holy formula08 profiles, pure scalar/prediction, paid incantation/impact, Sleep wake, Auto-Potion, terminal settlement and actual native AI;1352 PC/sanitizer/68000 assertions|Element/innate producers, monsters, magic evasion, areas or display/capping: source trace and smallest affected native fixture|
|Vulnerable incantation|`core-incantation-check`: fixed tile/replacement occupant/live unit binding, actual CT/menu pause, caster movement/weapon survival, late MP/status failure, paid exact-cost hit,13 casts, Auto-Potion, node255/upper binding; PC/sanitizers/native|New magical formula/area/producer: original formula/traits/target list, per-target effects/reward/reaction and presentation trace|
|Casting resources/preparation|`core-casting-check`: original MP/prepare routines, shared player/preview/actual AI-list eligibility, synthetic nonzero cost carrier, pure invalid orders, misses/overlap/once-only debit, Half MP/enemy cost and cold range ownership; PC/sanitizers/native|New spell/caster: original effect/target/interruption/MP order, explicit tile/unit binding, area membership and response/reward semantics|
|Equal-tick due dispatch|`core-deferred-check`: source slot-order sequences,13 roots/13 empty records, busy-poll packet/RNG purity, one expiry/CT phase, later-caster invalidation, Counter before next root and terminal priority; PC/sanitizers/native|New casting/Quick/periodic producer: original cost/interruption/target-set trace, target binding and no-eligible-actor liveness|
|Dual Archer Charge|`core-charge-hands-check`: eight source initializations/sixty scalar cases; all powers, criticals on each hand, both deferred weapon IDs, fixed tile after recoil/KO, Sleep wake, CT/menu pause, unique rewards and delayed responses; native equality|New target/range/projectile or casting family: original full delayed target/cost/order trace; AoE/equal-tick and source unit-following policies|
|Ordinary weapon hands|`core-hands-check`: original hand/scalar queries, different first/second weapons, mutable/innate Sleep, both Counter hands, lethal returns, shared preview/AI purity, saved support and actual management removal; native equality|New weapon/skill family: source formula/proc/element/target trace, hand-specific deferred weapon tracking, shared prediction/AI and visuals|
|Consecutive ordinary strikes|`core-strikes-check`: original initialization/continue-phase/best-EXP fixtures; actual two impacts before one response, miss/miss, empty/KO/recoil/control cancellation and unique rewards; PC/sanitizers/native|Hand-specific equipment/ability producer: original target/weapon/formula trace, shared preview/AI, independent weapons and interrupted second strike|
|Counter/Auto-Potion execution|`core-reactions-check`: original staging/Brave/stock, actual secondary effects and packet restore, one-time stock/impact, deferred root rewards, ranged overlay return boundary; PC/sanitizers/native|New reaction/strike/affiliation/weapon family: original full target/cost/order trace, AI treatment, live eligibility, per-strike and terminal settlement|
|Root/response ownership|`core-context-check`: original reaction guards/selection; actual impact-once, rejected-order side effects and root settlement; native layout/ABI, action frames and pilot flow|New reaction executor or strike family: original effect/target/cost/order trace, packet restoration, eligibility at drain and terminal settlement|
|Sleep/vulnerability|`core-sleep-check`: original physical caller/finalize/AT, actual PC/sanitizer/native formulas, impact-once wake and timer/Charge/AI adapters; direct control/lifetime/state/action/Charge regressions|New producer/formula/reaction: source attack-type context, source support/elemental ordering and real effect presentation|
|Geometry helper|Focused semantic boundaries, source oracle for the new rule, direct battle caller on PC/native|Types/shared helper behavior changes or a boundary fails|
|Compact topology layout|`core-compact-check`: cost6 generic search,32-link actual Work storage and256-node allocation/rebind; benchmark against a captured same-state baseline|ABI changes: rebuild all images, affected scene/save/Status callers and actual view equality|
|Movement graph scratch/cache|`core-step-cache-check`: exact accepted graph and budgeted searches, partial-search abandonment,256 nodes|Buffer owner/layout/search changes: all affected consumers|
|Module responsibility split|Changed setup/save/Status calls on PC/native, matching bank count/signatures, packed input headroom and unchanged screenshots|Actual context/layout change: rebuild every image and exercise each changed consumer|
|Forward span costs|`core-jump-cost-check`:57 original spread queries/68 PC/sanitized/native checks, including cost6 and budget boundaries|Runtime cost/encoding change: full/budgeted search, producer/weather/water and reverse AI policies|
|Actor-column query|`core-jump-actors-check`:88 original queries/99 PC/sanitized/native checks|Runtime record generation or abilities: actual units/status/floor occupancy and full-span paths|
|Wider-cost search|`core-span-search-check`: focused graph oracle and actual Work-bound optional caller; pure fixtures include existing fast-path cases|Activation/shared layout: graph capacity, every actor span, resident packing limit and changed battle paths|
|Long-span geometry prototype|`core-long-jump-check`:22 original full-spread cases,278 portable/native checks|Runtime hookup: queue costs, graph capacity, actor spans and selected battle replay|
|Per-map flags|`core-profile-check`:2574 PC/sanitized/68000 checks|Classifier/domain/codec changes: explicit `core-profile-deep-check`|
|Attack/recoil|Targeted action state, placement/cache invalidation, chosen destination/occupancy and damage|Existing battle behavior changes: selected Orbonne/Gariland replay|
|Overlay/layout|Actual affected module calls, signature/loader rejection and native size|Shared ABI/layout change: every consumer rebuilt and integration once|
|Format/extractor|Malformed spans/IDs/version checks and deterministic output hashes|Schema/decoder change: affected assets and native consumers|
|Renderer|Same-state PC/native planes and relevant asymmetric views|Changed image: reviewed before/after; unchanged image: retain dates|
|UI-only|Affected input/state/layout fixture|Shared menu behavior actually changes|

For a stable release, run the required integration gates once. Do not rerun
all decoder streams for a rule-only edit or every font/management/map fixture
after changing one geometry predicate. Headless native builds remain serial;
shared temporary save tests also run serially. Never drive the user's TiEmu
without a new launch/input request.

The profile runner defaults to nine curated profiles and22 boundary heights:
`make -C games/fft/campaign core-profile-check`. `--pc-only` skips TI compilation
for host-only edits. The explicit full certificate command is
`make -C games/fft/campaign core-profile-deep-check` (`--deep`), over972 retail
profile/column pairs and every byte height. Default artifacts live in
`x/core-profile-fast/`; exhaustive reruns use `x/core-profile-deep/`, preserving
the historical certificate in `x/core-single-floor/`.

## Primary action/state work

Use `make core-actions-check` for the common target transforms or impact owner:
source-recorded primary effects, saturation, cure/revival, one-time rewards,
Wish/terminal timing and three complete AI rankings on PC/sanitizers/68000.
When relocating the impact owner, use the explicit old snapshot comparison in
`benchmark_actions.py` for action pictures/forecasts and bounded commit cycles.
ABI/callback changes also require module-bank rejection tests and one campaign
setup/save/results replay; recoil relocation needs its existing actual caller.
Keep Charge as the direct deferred-action caller. No topology census or complete
font/management/input/decoder sweep follows from a primary-effect-only change.

## Existing status hooks and catalogue dispatch

For Darkness/Silence changes run `core-status-rules-check`:86 original MIPS
queries feed player/preview/committed-roll/AI and cure checks. Use `common-check`
when changing the archived menu dispatch table/version because later name/list
sections move; check missing-table and old-version rejection. Existing actions,
Charge and a scoped campaign flow replay cover direct callers. Compare the six
normal action frames against CORE_ACTIONS; changed mechanics in synthetic status
fixtures do not authorize an unrelated UI/art change. No geometry census follows.

## Canonical status state and protected effects

Use `make core-status-state-check` for the bitmap kernel or its Unit adapter:
177 original filter/remove/store queries, all40 status IDs and mixed policy
boundaries, then actual FORMULA application/cures/AI/command-state assertions.
The original oracle isolates external status-specific handlers; it does not
certify expiry, HP, team or control changes. Runtime capability rejection is
part of the test: unsupported additions, including Poison before upkeep, fail.

A Unit/State layout change requires every ABI consumer rebuilt, native sizeof,
module signatures/loader rejection, the13-actor recoil boundary and one scoped
campaign setup/save/results replay. Compare six ordinary action frames and the
existing flow pictures. Run common-check for FCJ7 source rows and old/short-bank
rejection; keep Charge and the earlier Darkness/Silence hook checks as direct
callers. No map census, exhaustive movement sweep, all-stream decompression or
live TiEmu follows from the status-state change. Current evidence and caveats:
CORE_STATUS_STATE.md and `x/core-status-state/verification.json`.

## Timed state and turn upkeep

Use `make core-status-lifetime-check`:205 original timer/bitmap/upkeep queries
and actual13-actor native adapters. Check renewed/cancelled timers, innate
protection, menu pause, Charge before equal-tick expiry, end-turn-only HP,
KO/Charge cancellation, Wish self-KO and last-player terminal handling. The
candidate mask must include actor12 and avoid loading FORMULA when empty.
Native cycles distinguish cold13-actor expiry from prepared upkeep; do not call
one CT step a full-frame certificate. Source HP/graphics/reaction callbacks are
outside the isolated original staging oracle.

After Unit/State/ABI changes, rebuild all images and packed consumers, measure
layout/packing, retain existing protected bitmap/action/Charge checks and one
setup/save/results replay. Compare six action frames and existing flow images.
Unchanged FCJ7/geometry/decoder source and bank hashes reuse earlier evidence;
no catalogue census or all-map sweep follows. Current artifacts:
`x/core-status-lifetime/verification.json`; remaining scope in CORE_STATUS_LIFETIME.md.

## Control flags and safe image boundaries

Use `make core-control-check`:48 original status-control,84 CT-end and two AT
fixtures feed actual13-actor menus/starts/AI/clock/Charge checks. Cover stopped
already-ready CT, paused and resumed charges, due-time Don't Act/cure, unspent
command restoration, no-evasion preview/roll, source AT wait and actor12. Keep
Sleep/other unsupported handlers rejected. Measure cold and warm AT separately.

A control-cache meaning change requires an ABI bump even with identical fields.
Rebuild images and packed consumers; run existing lifetime, protected status,
Darkness/Silence, actions and Charge as direct callers. For cold setup/AT
relocation compare normal action/flow pixels and save bytes, validate the scene
return boundary and invalidate the image tag before rebinding a new Work block.
Audit callbacks: a resident function newly loading an image must not be entered
while its caller's image is executing. No full geometry/decoder/catalogue sweep
follows from unchanged source/data. Current evidence: CORE_CONTROL.md and
`x/core-control/verification.json`.

## Reaction effects and restored primary presentation

Use `make core-reactions-check` for Counter/Auto-Potion. The scoped original MIPS
oracle executes finalize/staging, stock selection, Brave and EXP contexts; it
does not certify full secondary animation/JP/multistrike. Check pre-impact
Counter versus post-wake Auto-Potion, misses/zero damage/KO, one item shared by
two actors, duplicate preparation, delayed primary rewards, lethal Counter and
preserved independent Charge/CT/turn history. Counter eligibility comes from
the source flag, which Dash/Throw Stone do not set. Reactions cannot recurse.

Keep ACTION-local evaluation local. A resident Counter trajectory query may
load AI only after ACTION has returned; invalid LoS restores the packet and
yields before another expensive query. Exercise clear/blocked ranged returns
on native68000, plus existing actions/context/Charge. An ABI change requires
all images and packed consumers rebuilt, layout/old-bank rejection, six normal
action hashes, response-state plane equality and one setup/save/results replay.
Preserve unchanged image dates. No geometry census, full decoder sweep or live
TiEmu follows from unchanged data/renderers. Current scoped measurements:
CORE_REACTIONS.md and `x/core-reactions/verification.json`.

## Consecutive primary impacts

Use `core-strikes-check` for the two-strike engine entry and best-earned-EXP
accumulator. Source phase-control fixtures isolate UI/animation/announcement
callbacks; do not label them an original end-to-end Two Swords trace. Keep a
pending reaction across the first primary impact, prohibit early drain/close,
rearm exactly one second impact, then settle one response and one reward.
Miss/miss must not award EXP; an earlier success survives a final miss.
An invalid count rejects before RNG/cost/Charge mutation. Empty/KO fixed tiles,
recoil and an incapacitated/dead actor must terminate without hanging.

CoreAction's two-byte extension and changed reward-byte meaning require ABI16,
all images/packed consumers rebuilt and old-bank rejection. Recheck reactions,
context, actions and Charge as direct callers, six normal action hashes and
one setup/save/results flow. Inspect source hashes and reuse unchanged bitmap/
timer/geometry evidence; no full census or live emulator follows. Current
scope, including gated hand-specific producers: CORE_STRIKES.md.

## Hand policy and cold preparation images

Use `core-hands-check` for ordinary Two Swords and dual Counter. Keep the source
initializer's single-weapon promotion/count policy separate from equipment
legality. Verify left/right numerical differences, protected versus wakeable
Sleep, no preview/AI mutation and one final reward. Charge with only the left
weapon uses that actual weapon and cancels when it changes; two-strike weapon
skills remain rejected by both player and AI. Management must refund a left
weapon when support/job/right-hand compatibility no longer allows it.

COMMAND relocation requires rejection before even the resident formula-request
fields change: preserve whole-State hashes on invalid/overlapping orders, not
just HP/RNG. The image must return before a charged action advances AI.
RESPONSE relocation requires packet/stock/duplicate/terminal regressions and
both Counter hands before restore. Rebuild every ABI17 consumer, reject old
module/catalogue versions, compare normal/response pictures and run one scoped
save/results flow. Check changed catalogue metadata and reader boundaries once;
unchanged timers/geometry do not need another exhaustive sweep. Evidence:
CORE_HANDS.md and `x/core-hands/verification.json`.

## Deferred hands, pure math and cold pickup

CORE_CHARGE_HANDS.md adds `core-charge-hands-check`; it supersedes the earlier
dual-Charge gate above. Other unsupported weapon skills remain rejected.
Seed both `pending.weapon` and `pending.off_weapon` with `hand_weapon`, including
lone-left promotion, in manual pending fixtures. This adds meaning to the old
padding byte without changing the eight-byte record: rebuild ABI18 images and
reject ABI17 even though native State/Work/save sizes are unchanged.

Use `rng32` to check battle-RNG purity; `rt_seed` belongs to the portable runtime.
Forecast/scalar queries consume no battle randomness. Actual AI selection uses
its existing candidate-choice draw even at100% hit chance: compare the expected
post-choice rng32, rather than incorrectly requiring unchanged RNG. Historical
CORE_HANDS.md checked only rt_seed; its old wording overstated that assertion.

Run the changed hands/strikes/reactions/context/actions/Charge/control/Sleep
callers, one13-unit boundary integration and one flow/layout/packing check.
Check pickup through PARTY6 cold entry; no callback may evict an executing image.
Preserve the twelve accepted normal/response plane hashes and eleven flow PNG
bytes/dates. Existing catalogue, topology, timers and management keep their
unchanged evidence; no new map census or live TiEmu run is required. Final scoped
report: `x/core-charge-hands/verification.json`.

## Due-wave dispatch

Use `core-deferred-check` for iterative Charge dispatch and its turn-entry caller.
The internal `charge_resolve` returns0 after consuming a rejected/missing due
record; its caller resumes CT. Direct test callers must not expect recursive
turn entry. A return1 means an existing/new action owns presentation. Cover
thirteen records without a queue, source slot order, busy whole-State/RNG purity,
late invalidation and one expiry/CT advance after the wave. The stress actor uses
Speed50/CT50, within the reciprocal-table domain; Speed100 is invalid for a
`turn_start` fixture, even if a raw `ct_next` test can use it.

No shared type/ABI changes: rebuild the actual resident binary, verify the image
bank/data hashes remain unchanged and reuse prior native layout/action-frame
certificates. Dual Charge/control/single Charge plus one native pilot flow are
the affected closure. Target-kind MIPS observations are preparation for magic,
not native casting/AoE/MP certification. Quick255 and all-permanent-freeze clock
liveness remain open. Evidence: CORE_DEFERRED.md and
`x/core-deferred/verification.json`.

## Casting costs and image budget

CORE_CASTING.md separates original MP/preparation observations from native spell
support. `check_casting.py` changes only a private Dash range record's cost and
Silence flag; never label the fixture as retail Dash or an enabled Holy spell.
Preview world/RNG purity permits native formula-request scratch writes; rejected
commands still preserve the whole State. Test real AI setup/candidate lists,
rather than calling a PC-only static helper absent from native AI images.

Cost queries execute through the pure resident callback while AI/FORMULA/COMMAND
remains loaded. Range construction belongs to COMMAND2; do not call it from an
executing image. Rebuild all ABI19 consumers, reject ABI18, check the actual
packing limit and the cold range caller. Direct actions/dual Charge/control/due-
wave/single Charge and one pilot flow cover integration; unchanged geometry/
bitmap/timer evidence is reused. No current spell producer implies live Short
Charge/Non-Charge/performance scheduling merely because their pure kernel passes.

## Vulnerable cast lifecycle

Use `core-incantation-check` for cast_order and mixed Charge/cast dispatch.
Retain the resource fixture and add a naturally successful exact-cost command:
forcing a miss after payment did not catch the previous second affordability
check. Execution must use skill_effect_hit after one debit; preview retains
skill_hit affordability. Source effect math remains separate from timing policy.

Native cast arguments belong to ModCtx, not State/formula scratch. Rejected
orders may load COMMAND but cannot mutate State/resources/RNG. Return from
COMMAND before preloading ACTION or continuing AI. Due polling remains resident;
only actual due preparation loads COMMAND4. Validate actual node255 and unit
layer changes with the256-node adapter, without repeating its old exhaustive
search suite. Source initializer payment before binding is not a full original
empty-spell/refund/presentation certificate.

Private Dash/Throw Stone resource/cast carriers and a persevering rejection
record are synthetic; production normalized spells remain disabled. Rebuild
ABI20 consumers, reject19, recheck native layout and one pilot flow. Reuse
unchanged geometry/bitmap/catalogue evidence. Jump stays low priority.

## Magical scalar and cold setup

`core-magic-check` uses a private normalized Holy record with source power50,
MP56, CT6 and no magic-evasion flag. Production normalized Holy/Fire remain
kind255 with no AF_CAST entry. Keep this distinction in reports. Its104 original
formula08 profiles cover modifier rounding, Faith/Innocent precedence, zodiac,
zero XA, actual shield/Defend bypass and preview/execute proc draws. Normal
weather and zero elemental traits are explicit fixture conditions, not general
support. Formula scalar equality does not prove full target-list/HP/cap behavior.

Pure magic math is resident: AI/ACTION/FORMULA must never call a wrapper which
evicts their executing image. Preserve the generic execution draw even when
Holy has no evasion or status proc; preview takes none. Magical damage does not
inherit physical vulnerability, critical or Brave knockback.

ABI21 rejects20. PARTY7 owns unchanged banner metadata and PARTY8 owns unchanged
coordinate-cache setup. Invalidate the image tag before cache setup binds new
Work/dimensions. Do not move end-turn CT logic into an overlay merely to gain
bytes: keep its frequent path resident. Check13-actor end-turn credits, native
layout, cast/node255 adapter, direct action/Charge/control regressions and one
pilot flow; reuse unchanged geometry/timer/bitmap evidence. Preserve identical
image bytes and dates. TTPack input64919 has only17 bytes headroom; the next
resident addition needs an explicit module budget before it can ship.

## Affinity data and stat residency

`core-affinity-check` validates source job bases and equipped-item OR aggregation
without conflating them with formula enablement. Source actual-stat arithmetic
is isolated in this oracle; existing stat/level/party regressions supply native
integration. Arbitrary source equipment combinations do not certify legal gear.
All160 internal job templates are data records, not160 playable careers.

FCJ9 keeps existing14 section offsets, appending FEA1 after the innate Two Swords
bits in SPECIAL. Validate the entire416-index table, reserved byte, count, spans
and neutral row once at bank load. Reject FCJ8. Existing raw section bodies,
names and normalized spell enablement must remain equal. Only ffcj changes among
non-code banks; geometry/art evidence is reused. Verify deterministic row sharing.

Keep Unit112 and its hot indexing. Derived affinities occupy13x5 bytes in State;
rebuild from job base before ORing current equipment and clear absent/spare
slots. Refresh via battle/party/stat producers, not by scanning gear in every AI
query. Scratch Units cannot overwrite a live actor's cache. Future noninitial
job/equipment producers must preserve live HP/MP when rebuilding stats.

Native stats compile locally only in PARTY/ACTION/RESPONSE. Remove the old
unit_stats callback: an accidental image caller should fail linkage rather
than overwrite itself. A resident-only wrapper preloads PARTY9 and passes a
Unit pointer through the existing transient input union. Source unsigned growth
division uses the existing compiler helper through a stack-ABI callback. Test
actual primary and response-owned level-up, returned image IDs, party load and
all13 slots; no new assembler is needed.

Rebuild ABI22, reject21, refresh layout once and run shared catalogue reader,
management loader, scalar/incantation/actions/Charge and one pilot flow. Skip
unchanged geometry/timer/status censuses and preserve identical image dates.
No current test proves elemental typed effects, source halfword narrowing/caps,
magic evasion, monster equipment rules or reviewed cast presentation.

## Typed elemental outcomes

`core-elements-check` keeps production data unchanged and normalizes only private
formula08 carriers. Holy0x0f uses source power50/element2/no-evasion; other
power/element combinations are synthetic arithmetic probes, including255 for
negative-before-half rounding. Do not label those as enabled retail spells.
The original damage pipeline/finalizer run unchanged; source commit stops after
signed HP/MP stores, with reaction/MP-split isolated. Full original status/KO/
EXP/target-list behavior is separate evidence.

Keep raw s16 storage boundaries before Half/Weak/Faith, then cap positive results
at999. Negative intermediates remain signed fields; magic_hp clamps actual HP
without changing the event category. Forecast/AI must use this same transform.
A prepared category and amount survive later trait/forecast queries and response
save/restore. Healing does not offer Auto-Potion or damage-type Sleep wake;
nullification pays once but offers no effect RNG/impact/reward. The healing
response-packet test is an explicit injected transport fixture, not a claim that
Holy healing naturally triggers Auto-Potion.

The earlier magic scalar fixture retains104 original raw profiles; only its
shared skill_amount expectation now applies the source finalizer's positive999
cap. Its new1352-check hash is B5895FED, intentionally replacing4DCFDD40.
Check math at new signed bounds using the existing verified MULS16/DIVS32_16;
never widen to software32-bit helpers silently. Reuse unchanged trait-data,
status/geometry and original MP/timing evidence rather than a full census.

ABI23/ModCtx352 rejects22. effect_kind occupies former State/ActionPacket padding:
refresh native/host layout and packet-size assertions. FORMULA10 owns formatting
and local evaluators; the resident wrapper passes its output pointer through
transient ModCtx input. Only resident code may invoke that loader. The actual
HUD requests forecasts in M_AIM, before action wind-up; an injected later query
can change idle residency but cannot rewrite the prepared payload. Check normal
and response frame hashes, Charge forecast/menu checks and one pilot flow.
Cold formatting46180 cycles fits the60k task envelope; this excludes the rest
of the frame. Packer input64795/headroom141 and isolated CPU zones are not full
stack/fragmentation/residency/worst-frame certificates.
