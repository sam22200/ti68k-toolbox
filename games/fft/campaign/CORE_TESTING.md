# Validation by affected dependency

Named objectives: `make campaign-objectives-check` runs original-MIPS policy,
bank validation and actual PC/sanitizer action/save/pixel tests. The optional
`check_objective_runtime.py --native --data-dir DIR` adds only actual ORDER/
PARTY/SAVE/archive handoffs and one combined-title render/budget carrier.
Approved LCD pixels remain exact; isolated save regressions cover receipt
idempotence. Scope, metadata and remaining authored content: [Objectives](OBJECTIVES.md).

Monster KO: `make campaign-monster-ko-check` uses PC/sanitizers and approved
pixels. `check_monster_ko.py --native` opts into one actual68000 renderer carrier.
`check_capacity_render.py --case` selects `monster-ko`, `monster-ko-morph` or
`monster-ko-transition` with `--ko-bank PATH` for dense, one-dissolve or first-KO/
revival budget fixtures; its defaults retain the original four cases.
Scope/evidence: [MONSTER_KO.md](MONSTER_KO.md). No emulator is involved.

Current default (user decision 2026-11-12): run affected PC tests and sanitizer
replays first, including complete combat and save flows. Native checks are
selected for changed ABI/layouts, arithmetic/pointer paths, overlay calls,
CPU budgets or hardware dependencies. Reuse unchanged native evidence; a new
lot does not require the 103-target native sweep. That completed sweep is the
2026-11-12 baseline in `x/core-sweep-20261112/`, not a per-edit checklist.

`make campaign-prerequisites-check` is PC/ASan/UBSan only, in private data trees:
map-scene controls, standalone scenes and persistent stats/KO/inventory. Its
`--only map|scene|save` option selects an affected subset. The opt-in
`campaign-prerequisites-native-check` runs one198-check actual-overlay scene/save
carrier; `check_campaign_native.py --data-dir DIR` accepts normalized shipping
banks. `check_scene_skip.py --data-dir DIR` selects just three standalone
prompt/resume/skip replays. `check_capacity_render.py --case ...` can now select
individual native frame cases. [Evidence and bounds](SCENE_SAVE_FIXES.md).

Traps: `make campaign-traps-check` runs a small original-MIPS fixture,
PC/sanitizer trap/save tests and current-raw management checks. It does not
compile overlays or touch TiEmu. `check_traps.py --native-numbers` opts into one
independent arithmetic/bitmap carrier. `--only raw-management` extends the
prerequisites runner with the affected FSV5 management test. Scope and evidence:
[Trap delivery](TRAPS.md). `check_traps_native.py --data-dir DIR` opts into one
83-check actual-overlay/save carrier on normalized data, with four equal screens.
632 PC/sanitizer checks, the complete reused Sweegy trace/save, bindings, two
frame-budget cases and final package checks pass; no whole sweep or TiEmu.

`make chapter1-archive-budget PACKAGE_OUTPUT=...` is host-only planning: real
group hashes/current core key plus dated estimates. It never builds or runs TI.
Allowances/depth overflow and duplicate maps are checked; full chapter/Flash
certification remains separate. See [Archive plan](CHAPTER1_ARCHIVE_PLAN.md).

`make archive-reach-check` defaults to PC/full-row reference/ASan/UBSan,
comparing per-phase full-scene and LCD hashes; `check_archive_reach.py --native`
adds one selected68000 carrier. It also checks R4 header bounds and byte sorting
at256 nodes. Complete ordinary Sweegy/save, normalized packages, affected render
checks, ABI rejection and13-actor native budgets pass. The two-pixel correction
from the old128-row overflow was explicitly reviewed and approved. Scope and
installed supervisor RAM limits: [Archive reach colours](CORE_REACH_ARCHIVE.md).

`make battle-ram-check` compares guarded100-row targets with the128-row reference
on PC/ASan/UBSan. `check_battle_planes.py --native` adds only primitive/Gariland
backend checks. Scope, one hardware session and remaining16KiB gate:
[Battle RAM](CORE_RAM_MARGIN.md).

`make chapter1-monster-battle-check` defaults to three complete Sweegy rules/save
replays on PC plus sanitizers. `check_monster_battle.py --native` adds one
selected PARTY/SAVE initialization/receipt carrier, not full native fights.
Source inputs, bounds and evidence: [Monster battle](CHAPTER1_MONSTER_BATTLE.md).

Current optimization checks (2026-11-07): [Final pass](CORE_OPTIMIZATION.md).
`make test` includes58 actor-cache boundary checks and scene allocation/reentry
checks. Cache PC/sanitizers/68000 agree; scene lifetime sanitizers, nine scene
replays, four13-actor render carriers and actual installed pilot/save flows
pass. Native picture reservations are16800/12000/12000 bytes. Board/rotation
ordinary averages218905/248713, peaks332836; equipment peak351228. Production
input64911/64936, ABI80/zone8690/layouts remain. This is affected-cache/scene
integration, not a new all-family certification. Evidence: `x/final-opt-pass/`.

Current shared-resource checks (2026-11-07): [Resource ownership and sharing](CORE_RESOURCES.md).
`core-resources-check` is registered as `resources`.237 host checks and33 C
reference checks agree PC/sanitizers/68000; nine scene replays retain pixels and
state, including different text on one shared backdrop. Seven real encounter
routes include a second encounter borrowing every arena bank. Native binding
checks cover27 images and nine rejected archive variants. Actual normalized
pilot flows retain AE9CFD31/2CD70391 and exact FSV4 saves/rewards/overflow.
Four13-actor render carriers retain0546/ECF7/A87E/AA26 and stay below360k.
The host file directory grows without moving borrowed data; runtime renderer
self-tests remain green. Evidence: `x/chapter-dedup/verification.json`. This is
affected-format integration, not a new all-combat-family certification.

Previous package checks (2026-11-07): [Core and chapter packages](CORE_PACKAGES.md).
`core-packages-check` is registered as `packages` in closure.py. It checks
ownership/compatibility, corruption, supervisor resume/cancel behavior and
save-preserving replacement on PC/sanitizers/68000. With a fresh source group,
`check_package_routing.py` additionally boots real registered/remapped encounters
and rejects missing routes/packs. Native bindings/ABI78 rejection and exact
pilot pixel/rule/FSV4 flows pass under ABI79. This is targeted validation of
the changed cold loader; the prior100-suite combat result is historical and
is not presented as a new full closure run.

Previous storage checkpoint (2026-11-07): [Shared identities and module factorization](CORE_FACTORIZATION.md).
ABI78/348, FCP2/FMG3/FCD4;46 shared portraits and510 shared names preserve every
reviewed use. The packed package is611132 bytes/44 variables; controlled rebuilt
baseline622372, net-11240. Module shards55616/61352/55200, zone8690; Unit112,
State2224, Work37460, Save3202/FSV4 and48 actor frames stay. TTPack64913/64936
leaves23 bytes: preserve this bound and source/image format matching. Arithmetic
wrappers are retained by two-pass compiler discovery, never by editing opcodes.
Combat completion remains201 actions/78 RSM; no new family/mount queue.

Current combat integration (2026-11-07):
[Shared heights, AI evaluations and stabilization](CORE_FINAL_COMBAT.md) is the
active checkpoint; the remaining-work lists in older entries below are history.
Shared donor heights, friendly panel collateral, origin-dependent Reflect and
mutable/innate status utility now feed the native evaluator. AI work is sliced
and idle menus avoid unnecessary module loads. ABI77/348,26 images, Work37460,
zone8880; actual TTPack64797/64936 leaves139 bytes. Unit112/State2224/FSV4 stay.
All6605 focused checks agree PC/sanitizers/68000; ordinary13-actor frames and
planner quanta pass360k. Cold loads/rotations retain their documented latency.
RAM residency drops13810 bytes on Gariland without reducing the48-frame actor
cache. Status/AI and rotation/reach buffers share storage at exclusive phases;
real grayscale startup and two packed battle boots/exits pass. All99 campaign
regression suites pass without cached results.
Generic coverage is201/201 actions and78/78 R/S/M consumers: no implicit new
combat-family queue, and no mount/rider completion requirement. Content/tools,
future maps and exact PS1 AI remain separate scope. Preserve standard groups,
live TiEmu and protected proof/save bytes AND dates; use private outputs.

## Historical checkpoints

Historical direct-interception integration (2026-11-06):
[CORE_DIRECT_INTERCEPTION.md](CORE_DIRECT_INTERCEPTION.md).508 logic checks
8B7A2337 and24 HUD pixel checks04F50DC5 match PC/sanitizers/68000. Only
PLANNER24 changes; ABI75/layouts/zone8596 and actual resident bytes remain.
All13 affected regression suites pass; former body-obstruction fixtures use
solid terrain without removing assertions. Panel preflight now accepts the
interceptor (527 checks8864C72B). Native Gariland81/AE9CFD31 and
Orbonne14/2CD70391 retain pixels/rules/exact saves. Interactions and subsequent
TI stabilization remain open.

Current weapon-panel integration (2026-11-06):527 behavior/preview checks
8864C62B,33 HUD pixel checks3FEC3500 and66 thirteen-slot checksC32255C0
match PC, ASan/UBSan and actual68000. ABI75/348 moves frozen packet preparation
to FORMULA14 after PLANNER1 and continuation to PLANNER5. Sixteen initial
regressions and five final dependencies cover17 unique suites; five dispatcher/bootstrap dependencies also pass,
for22 in total. Exact loader
bindings, actual ABI74 three-shard rejection and private native pilot flows
pass. Source scope, native timing and limits: CORE_PANEL_VICTIMS.md.

Native ballistic integration (2026-11-06):1533 focused assertionsAEF99E38
match PC, ASan/UBSan and actual68000.128 retail choice branches are distinct
from336 independent native-model controls. See CORE_ARCS.md for adaptation
bounds, affected regressions, unchanged resident bytes and release gates.

Current physical weapon formulas(2026-11-05):2421 focused assertions(FEEA07E7)
agree PC, ASan/UBSan and actual68000.288 retail scalar profiles plus26 source
hand initializers cover physical families, source midpoint previews, independent
variable/critical draws and ordinary guns; live commands add Charge, guard
misses, Counter/Hamedo and new two-hand combinations.1024 relative-facing cases
retain the previous geometry after its smaller axis selector.
All33 affected suites pass. Parser140/current bindings203 and actual ABI70
three-shard rejection pass; Gariland/Orbonne remain81/14 checks with identical
native pixels and exact FSV4 VAT bytes.555 protected save/group files retain
bytes and dates. ABI71 repurposes the same callback slot for pure weapon_amount;
preparation stays in each executing owner. Layouts/heap blocks and FCJH length
stay fixed;27 source hand bits change (26 weapons plus the bare marker).
Actual production TTPack64929/64936 leaves7 bytes. Source/query and constructor
zones are bounded measurements, not whole-frame/stack/residency certification.
Weapon effects/procs and their later original RNG remain explicit next work.
See CORE_WEAPON_FORMULAS.md and x/core-weapon-formulas/verification.json.

Historical source weapon reach integration(2026-11-05):55703 assertions(88CBF461)
agree PC, ASan/UBSan and actual68000.160 retail footprint profiles supply54080
panels;64 further original height cases cover Float/water movement/wrapping.
All22 affected suites close through the initial run and two focused repairs:
status-lifetime now expects the ORDER clock owner, and incapacity asserts the
existing Petrify pre-RNG rejection. The latter's old failure was reproduced with
preserved pre-task ABI69 sources; gameplay was not altered to satisfy it.
Six affected suites were rechecked after the resident batch-mask optimization.
The final sparse-cardinal improvement changes only TURN/MOVEMENT images; resident
bytes and20 other images are identical. The focused suite and five native campaign
carriers pass with final archives, identical pixels and exact FSV4 save bytes.
Current bindings203/parser140 checks and an actual three-shard ABI69 rejection
pass. ABI70 keeps all layouts/allocations; actual TTPack64897 leaves39 bytes.
Manual mask cold/warm average costs in the fixture: strike36328, lunge50324,
arc223688, direct134750 cycles. These are range zones, not complete frame budgets.
The100-tick clock stress case costs517844 and warm ready entry1896; combined
frame/stack/residency and release remain open. See CORE_WEAPON_REACH.md and
`x/core-weapon-reach/verification.json`.

Historical ranged Hamedo integration(2026-11-04):345 action/RNG assertions
(1AAFBA6F) and18 render assertions(3DD52CCA) agree PC, ASan/UBSan and actual
68000.36 retail outer Brave/status/return profiles and four early gates retain
explicit callee isolation. All13 affected suites pass; Mime remains1734 checks.
Native Gariland/Orbonne retain81/14 checks and exact private FSV4 VAT bytes.
ABI69 changes only the preflight semantics; layouts and five heap blocks stay
fixed. Current-bank parser/bindings and actual ABI68 rejection pass. Production
TTPack64917/64936 leaves19 bytes. See CORE_HAMEDO.md and its verification file.

Historical Mime integration(2026-11-04):382 action/resource assertions(EFEB855F)
and1344 source geometry assertions(84088419) agree PC, ASan/UBSan and actual
68000. Eight additional render assertions(BC452243) compare four action
frames pixel-for-pixel with the existing renderer. All23 affected suites pass;
final bank binding/corruption and real ABI67 rejection checks pass, as do five
final native campaign carriers with exact FSV4 VAT bytes. The source oracle verifies144 rotations,384 layers,384 isolated outer
selector profiles,40 status eligibility queries and four records. Work45608,
ABI68,22 images/zone8566 and actual TTPack64805/64936 retain the existing
allocation/save layout. See CORE_MIME.md and `x/core-mime/verification.json`.

Historical blockD closure(2026-11-04):17635 logic assertions(4880B193) and35
management assertions(A633E2AD) agree PC, ASan/UBSan and actual68000.
The2916 retail MIPS profiles cover Talk/Finger Guard, Train/poach, actual hidden
items/flags, Monster Skill and Chocobo formulas. All35 affected suites close;
focused repairs preserve every assertion and separate hostile Transparent
rejection from visible Dispel utility. Native Protect/Combat/bindings and five
campaign carriers are rechecked after final adjustments. All nine isolated unit
suites and20 campaign input/pixel/state/save replays pass; FSV4 VAT bytes agree.
Actual TTPack64581/64936,22 images/zone8566, ModCtx352 and source common-reader
hashB9686645 pass. Protected522 files retain bytes/dates. See CORE_DISCOVERY.md
and `x/core-discovery/verification.json`; full-frame/stack/residency is separate.

Historical blockC closure (2026-11-04): `core-combat-check` supplies2975 logic
assertions plus12 UI assertions/four framebuffer captures, identical across PC,
ASan/UBSan and actual68000. Its unchanged retail evidence covers116 physical
profiles,240 resource stages,96 Counter Magic stages,96 MP boundaries and24
outer Hamedo trials with explicitly isolated target/MP/preview callees.
Forty-two affected suites ran once; six focused rechecks close Throw Stone's
physical path, Meatbone range expiry, late-Wall critical-response fixtures and
the four-draw Charge critical seed helper. No assertion was removed. The final
Gariland/Orbonne native rules/pixels and private FSV3/VAT bytes still agree,
including repeat receipt and saturated trophies. Native ABI66 bindings pass195
checks across21 cold/warm images. Actual production TTPack64861 leaves75 bytes.
Full tactical frame/stack/residency gates remain separate. Evidence:
CORE_COMBAT.md and `x/core-combat/verification.json`.

Historical blockB closure (2026-11-04): `core-equipment-check` adds93273 focused
assertions/10 first-frame and settled UI captures, equal PC, ASan/UBSan and
actual68000. It includes real private FSV3/VAT serialization and repeat-write
idempotence. The35 affected suites were run once; focused final rechecks cover
hands/banks/Charge/range/AI/Jump. Repairs: removing Two Swords validates the
secondary hand first, the Counter fixture retains current innate support rows
and reuses one allocation, and impact bounds regenerates its modified catalogue
from a SHA256-stamped source. No assertion was removed. Gariland/Orbonne native
flows remain81/14 checks (AE9CFD31/2CD70391). UI own draw/input peaks stay under
360k; broader tactical whole-frame/stack/residency gates remain open.
See CORE_EQUIPMENT.md and `x/core-equipment/verification.json` for exact scope.

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

Current movement delivery-block check (2026-11-03): CORE_MOVEMENT.md /
core-movement-check.9556 rule+288 runtime+113 UI assertions and32 real-renderer
pictures match PC/sanitizers/68000. Exercise equipped/innate stat loading, source
terrain/weather costs, landing restrictions, wide/narrow search, native AI,
Teleport's one draw/spent failed Move/no undo and equipped Float's innate status.
SCREENS and portable input must share the undo guard. Ordinary actors retain
the graph fast path; dry slow terrain and severe weather activate source costs.
ABI64/ModCtx360 has19 images and three FMB3/FMD2 shards: test every cold/warm
binding, third-shard missing/corrupt rejection and a real ABI63 export. FCJE adds
864 archived bytes; native State/Work grow14/6, save stays FSV3. Validate layouts
and private flows with the new archive set. Actual production TTPack input comes
from non-BENCH raw output/TP header (64175), not the instrumented carrier63227.
Warm reach peaks316986 cycles; cold reach3243404 is not full-frame certification.
The broader scoped closure and protected-file evidence stay in verification.json.

Historical counter reaction check (2026-11-03): CORE_COUNTER_REACTIONS.md /
core-counter-reactions-check.813 logic assertions E904D4F4 and34 UI assertions
8BBF301F/12 pictures agree on PC/sanitizers/68000. Keep raw0x10/0x80 eligibility,
frozen effective MP and three Dash RNG draws; exercise all twelve terrain
responses, collateral, once-only commit and root restoration. The UI carrier
uses three transport partitions, recombined including picture events.
ABI63 relocates selectors/strike queries to RESULTS3/4, Flood to TARGETS11 and
post-clock reset to ORDER16. Test old-export rejection and direct consumers.
Area bounds now admit a secondary packet owned by the current actor/ability;
wrong owners and DRAIN remain excluded. Preserve the64 old frame samples.
Gameplay/save layouts and FCJD remain fixed; packed input has69 bytes left.
Scoped closure and full-frame limitations are in private verification.json.

Historical Brave/Faith/Transparent reaction check (2026-11-03):
CORE_BRAVE_FAITH_REACTIONS.md / core-brave-faith-reactions-check.1372 logic and98
UI assertions/14 pictures match PC/sanitizers/68000. Check physical vs effective
MP trigger semantics, status-only scalar/batch owners, source four evade draws,
Transparent's fresh-turn/Act/HP/Jump handling and actual native AI planning.
ABI62 moves typed self preparation to FORMULA14, strike queries to RESPONSE6/7
and turn entry to ORDER16; test callers as well as image bindings/old exports.
Layouts/FCJD/allocation formulas stay fixed. Packing has9 bytes left.
Affected closures and full-frame/release limits are in private verification.json.

Historical self-status reaction check (2026-11-03): CORE_STATUS_REACTIONS.md /
core-status-reactions-check.1159 logic and117 UI assertions,18 pictures,
PC/sanitizers/68000 equal. Keep pre-impact Caution/Dragon staging distinct from
post-HP Regenerator; compare the source RNG boundaries and status no-op/late
filtering. Use actual bank ABI consumers and previous-export rejection after
ABI61. Shared action/status/reaction callers require the scoped closure recorded
in x/core-status-reactions/verification.json; this is not a full census or a
whole-frame release measurement. Gameplay/save layouts remain unchanged.

Historical Lancer Jump check (2026-11-03): CORE_LANCER_JUMP.md /
core-lancer-jump-check. Source timing/formula, learned reach, cell binding,
untargetability/reserved origin, once-only landing, CT, AI and four-orientation
UI checks compare PC/sanitizers/68000. FMD2 changes archived image storage;
run parser/reconstruction/previous-export and all direct ABI consumers, retaining
logical MOD_ZONE and unchanged gameplay/save layouts. Include action/context,
strikes/reactions, clock/deferred, targeting, old UI and resource families.
322 family assertions (298 logic+24 UI),16 pictures and33 older suites pass.
Source control-flag comparisons exclude the new internal airborne-cache bit;
their594 assertions retain15C96953 in the targeted rerun. See verification.json
for the original closure and superseding evidence.
Whole-frame/release gates and live TiEmu remain separate.

Historical critical-reaction check (2026-11-03): CORE_CRITICAL_REACTIONS.md /
core-critical-reactions-check.1031 logic assertions5C677F7C plus72 UI assertions
0EDD7883/eighteen pictures agree on PC/sanitizers/68000. Original87 staging calls,
24 effect commits and three self-target lists cover the HP/5 boundary, immutable
Critical, Brave0/no RNG, Wall misses, positive Undead healing, frozen resource
amounts and CT255. Two queued Quick reactors must wait for full root settlement.
ABI59/360 moves ordinary attack construction to RESPONSE4 without nested loads;
its unused public reward wrapper is absent. Run direct action/context/strike/
reaction/resource/Quick/UI callers, previous-export rejection, layouts, actual
packing and private chapter flows. Full census/live TiEmu remain separate.

Historical stat-save check (2026-11-03): CORE_STAT_SAVE.md / core-stat-save-check.
PA/MA/Speed Save adds697 logic assertions and33 UI assertions/nine sampled
pictures. Original finalizer/staging and secondary stat payloads are captured
without modifying their MIPS instructions. Pure reaction_live is now resident
and target_query254 shares it across effect owners. Unsupported source IDs stay
unavailable as commands. ABI58/360 also removes two unused evaluator callbacks.
AREA_UI14 owns impact numbers and wind-up glyph preparation; NUMBERS15 keeps
forecasts. Previous area/Math/Throw pictures must retain their historical hashes.
Run the directly affected reaction/context/strike/cast/UI closures plus old-export
rejection, layouts, private chapter flows and actual packing. No full census,
live TiEmu or standard playtest group is required for this slice.

Historical Math Skill check (2026-11-02): CORE_MATH.md / core-math-check.
1360 logic assertions9163135B plus238 UI assertions138E34BC agree on PC,
sanitisers and68000;88 UI pictures and four bounded native carriers. Digest
recombination counts both assertions and each separate per-frame picture step.
Reset chapter Work between independent UI fixtures. Original targeting/height,
Reflect/Counter guards and magical evasion are captured; all45 eligible spells
use shared owned effects. The16 numeric masks reuse the idle32-byte centre
bitmap, cutting the same native frozen-AI trial15.15M to4.84M cycles. ABI57/368,
FCJD; five gameplay allocation formulas and serialized layouts stay fixed.
The31 direct older suites, targeted final module/camera/AI replays, actual prior
ABI56 export and FCJC rejection, private chapter flows and preservation evidence
are recorded in verification.json. Native area-impact transport validates its
FIP envelope on the host and keeps null/truncation guards, all1012 assertions
and64 reviewed samples on the calculator. No full census or live TiEmu follows.

Historical Throw check (2026-11-02): CORE_THROW.md / core-throw-check.
4309/27CD6D9B plus61/38B87D4D UI assertions (4370 total);60 scoped suites.
Original formula99/finalizer profiles, all80 item records and12 isolated learning
selectors, stock/payment/cancel/MP/status/Counter/AI/range ownership; PC,
sanitizers and68000. A separate80-item UI carrier checks real inputs and36
pictures, with an existing Throw Stone cold-overlay cost control. FCJC, ABI56,
unchanged gameplay/save layouts; TARGETS cold reach/path/aim and pure resident
eligibility are covered by the shared closure, exact previous-export rejection,
both private chapter flows and catalogue rollback. See verification.json for
final counts, digests, sizes and explicit limits.

Historical Float check (2026-11-02): CORE_FLOAT.md / core-float-check.
525 assertionsBB53A5B9, PC/sanitizers/68000.
16 original formula profiles and the actual movement configuration;12 Float
adjacent moves match native narrow/wide searches.12 normal source controls
record the inherited dry/wet entry-cost adaptation separately. The producer,
Dispel/Earth protection, damage retention, MP/CT/control gates, actual AI and
real reach/restoration run PC/sanitizers/68000. Navigation regression retains
its independent cost/predecessor controls. ABI55, unchanged layouts/allocations;
raw64771/actual TTPack64773/headroom163. Scoped results in verification.json.

Current Charm/Invitation check (2026-11-02): CORE_ALLEGIANCE.md / core-allegiance-check.
5866 assertionsCF70C96D on PC/sanitizers/68000.
236 unchanged-MIPS formula/finalizer profiles,18 storage/control traces, repeated
Charm, ordinary victory and initial-team stock cases, all416 job/item immunity
rows; owned preparation/once-only commit, expiry/cure/damage, allegiance/AI,
private victory/recruitment/loss/full-roster/repeated-save paths. Native transport
splits contiguous math/runtime assertions and recombines counts and digests.
The source's current/permanent camp distinction does not authorize a wholesale
replacement of team queries: target/AI/gil, inventory/roster/victory and fixed
spawn-level bonuses have separate policies. ABI54 reuses Unit byte73 padding;
fresh baseline/final layouts match (State2210/storage5348; prior note sizes
were stale), and heap requests/save version stay fixed. Native COMMAND0 validates its
transient request before mutation, returning4 on rejection; capability queries
are local except the checker-only PERFORMANCE5 wrapper. Scoped closure and
exact measurements are in the family note and x/core-allegiance/verification.json.

Current Songs/Dances check (2026-11-02): CORE_PERFORMANCE.md / core-performance-check.
5507 assertions7FF2F648; math3727/3CCC9CCD plus runtime1780/B2313A81
recombine exactly. All fourteen source rows have strict mutation rejection.
Unchanged original MIPS
formulas/finalizer, six global masks, fourteen three-pulse clock traces and a
Quick wave feed shared PC/sanitizer/68000 assertions. Native transport partitions
math/runtime and recombines every assertion and rolling digest; no cases removed.
The direct36-suite closure covers changed targeting/formulas/commit, deferred
clock/order, AI/UI, module/boot boundaries and existing family controls. The
new family, targets and allocation-failure carrier have targeted retries; see
the evidence for exact results. No all74-suite census or live TiEmu follows.
Native allocation injection intercepts ti-cycles' synthetic ROM allocator in C
so the archived bootstrap sees it; the original table entry is restored.
Original geometry has eleven actors even in the thirteen-slot capacity bank.

Current Draw Out check (2026-11-02): CORE_DRAW_OUT.md / core-draw-out-check.
4625 assertions5AF7EF14 agree on PC, ASan/UBSan and68000
(math3202/84AED311 plus runtime1423/3377AD43).
All ten producers compare142 source formula/finalizer profiles,36 compound
break groups and12 Kikuichimonji footprints. Separate bounded native carriers
retain every assertion and recombine the full PC digest. Production/inventory
checks cover pure forecasts, frozen impacts, status timers, command gates,
actual AI, blue guests/enemy stock and private save/retry/victory roundtrips.
ABI52/388 keeps gameplay/save/allocation layouts; SAVE7/8 commit and SCREENS11
cold bootstrap are validated through the direct family closure and both native
pilot flows. The full census now has73 suites; this change uses a scoped closure.
All32 selected suites pass after Talk/area-impact switch only their native
failure diagnostics from source line to assertion ordinal (42/76-byte native
carrier overflow; no checks removed). Removing the obsolete RESPONSE preload
then reruns eight directly affected suites green; command preparation saves
37380 native cycles per tested action, with unchanged impact totals/images.
The final input64653/headroom283 and both rebuilt native pilot flows pass.
See CORE_DRAW_OUT.md and x/core-draw-out/verification.json for exact results.

Historical line-family check (2026-11-01): CORE_LINE.md / core-line-check
compares27 original green/list footprints and75 formula31/finalizer profiles.
2786 assertions59AFCB33 agree on PC, ASan/UBSan and actual68000; every native
panel, nearest-first draws, typed Earth/Float, frozen impact/push, gates and
actual AI are checked. ABI51/388,17 images/zone8236, input64735/headroom201;
FCJB/FGS2/FCD3/FSV3 and gameplay/allocation layouts remain fixed.
The26-suite direct closure passes (26 run,0 cached,0 red,235s). A final
monotonic radial-slot cursor removes redundant rescans: seven affected suites
are rerun green (224s): line,batch,Geomancy,area-forecast,area-impact,buffs,healing.
Their digests are unchanged; this scoped rerun supersedes their earlier
carriers. Other unchanged-family evidence is reused. All72 suites were not
rerun. The first closure's Talk native carrier exceeded the limit by14 bytes;
removing redundant packet formula guards shrank the largest image28 bytes.
No assertion or source profile was removed to make that carrier fit.
Actions changes to5324E3C7 only through id103; its previous normalized bytes
recover the exact old catalogue SHA and current-engine actionsD4899EEB.
Punch Art/strikes drop only their obsolete gated103 assertion (1253/BE27214C,
6522/891FD13C). Existing rendered area hashesF9BACFF5/9DE20073 and manual-area
replays remain equal. Final native pilot/bank and protected-file checks are
recorded in CORE_LINE.md and x/core-line/verification.json. Standard group and
live TiEmu are preserved; Jump remains deferred.

Historical Secret Fist check (2026-11-01): CORE_SECRET_FIST.md /
core-secret-fist-check compares185 original formula50 profiles, including its
retail evasion bypass.4067 assertions1FCD638D agree on PC, sanitizers and
actual68000. The shared MA/physical-modifier path preserves pure forecasts,
frozen status packets, no HP/MP damage, command/control gates, countdown/Undead,
refresh/immunity and actual AI policy. ABI50/ModCtx388 groups ordinary cold
setup in PARTY13; input64559/headroom377/zone8186. FCJB/FGS2/FCD3/FSV3,
gameplay layouts and allocation requests stay fixed. All30 direct suites pass (30 run,0 cached,0 red,373s), including native
renderer/area callers, statuses/magic/physical families and chapter/hand paths.
Both native pilot flows and the catalogue reader remain equal. Eight bad-bank
starts include the real prior ABI49 export;162 bindings pass. Actions changes
to D4899EEB solely through id104 (60795172 with its old bytes); Punch Art/strikes
drop the obsolete gated-entry assertion (1254/4EA46E40 and6523/65EAACF8).
Other checked summaries stay equal. Evidence: CORE_SECRET_FIST.md and
x/core-secret-fist/; historical geometry/Jump evidence is reused.

Historical Geomancy check (2026-11-01): CORE_GEOMANCY.md / core-geomancy-check
covers279 original formula profiles and128 source terrain calls. Its8927
assertions4BF14699 agree on PC/sanitizers and recombined68000 carriers:
math6403/229FDD40 plus runtime2524/59D8BB4C. Production146 records,100 casts,
140/201 generic entries; ABI49/ModCtx388. FCJB appends64 source mapping bytes;
FGS2 appends both layer surface types in archive, retaining collision stride.
The wrapper checks both chapter extractions, while native tests cover all64
surfaces/layer, commands/AI origins, frozen procs, timers, terminal effects,
interruption and twelve stacked victims. PARTY12/10 cold relocation preserves
core layouts/FSV3/allocations; input64741/headroom195/zone8186. The70-suite run
and corrected rejection carrier preserve all69 historical summaries except
catalogue actions60795172. Ten final direct suites pass after bounding AI
origin scans; native Orbonne/Gariland save/results flows and the catalogue
reader remain equal, with an isolated management test. The final pure-lookup control passes Geomancy/area-AI/banks again unchanged;
64 rejected origins fall294256 ->167456 native cycles. Exact evidence lives
in CORE_GEOMANCY.md / x/core-geomancy/.

Historical gil-transfer closure (2026-11-01): `core-gil-check` compares 134 original
profiles; 3262 assertions (2020738D) agree on PC, sanitizers and 68000, including
blue-camp guests (Delita's ENTD flags 0x84), frozen amounts, empty funds, caps,
no HP/MP effect, one commit, source gates and conservative actual AI. The wrapper
runs the existing save tests in a temporary data directory, including uncommitted
funds, victory/reload, enemy losses and repeated saving. Full `closure.py --all`:
69 suites green, all historical summaries unchanged except `actions` (D164D1B0,
the two records). The final guest-camp correction has a separate direct closure
(gil/actions/talk-skill/banks/area-impact/steal-equip/steal-exp) and both native
pilot save/results flows. ABI 48/ModCtx 388: PARTY11 initializes the existing
Save tail in ordinary and scripted starts; Unit/State/Work/FSV3 and allocations
stay fixed. Production input64725/headroom211/zone8186. Evidence lives under
`x/core-gil/` (full and final closures, final-flow, oracle, verification.json).

Historical equipment steals closure (2026-11-01): `closure.py --all`, 68 suites
green; `core-steal-equip-check` 4478 (F66BB1DC) joins the standard list. Every
hash equals the Magic Break closure except `actions` (69BA66A5: five records).
`make save-test` (outside the closure) checks a stolen item reaching the inventory.

Magic Break closure (2026-11-01): `closure.py --all`, 67 suites green;
`core-mp-break-check` 790 (C7D8B92A) joins the standard list. Every hash equals
the equipment breaks closure except `actions` (2B7773A6: Magic Break's record).

Equipment breaks closure (2026-11-01): `closure.py --all`, 66 suites
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
