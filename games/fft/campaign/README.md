# Opening -> Academy -> Gariland

Current distribution (2026-11-07): [separate core and chapter packages](CORE_PACKAGES.md).
`make packages` writes `x/packages/fft-core.89g` and `chapter-01.89g` with matching
metadata, size reports and installation/removal lists. `make chapter-package`
assembles content against that core without compiling it. The current chapter
is the existing Orbonne/Academy/Gariland pilot. ABI80; save format FSV4 remains.
Install one core and one chapter; retain all saves when replacing chapter data.

[Shared-resource rules](CORE_RESOURCES.md) apply across every arena, scene and
introduction in the chapter. Packaging drops obsolete copies, pools scene blobs
and identical arena banks, and writes `resources.json` with a duplicate audit.
The core is393134 bytes and the pilot213249 bytes,9135 net transport bytes less
than the preceding distribution, including new reference code. Rebuild an ABI79
core once; later content-only packages keep the matching core byte-identical.

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
Interactions and subsequent TI stabilization remain open.

**Current integration (2026-11-06):** [Weapon panel victims and previews](CORE_PANEL_VICTIMS.md).
ABI75/348: PLANNER1 geometry -> deferred FORMULA14 frozen packets; PLANNER5
continuation and PLANNER3/4 recipient previews -> NUMBERS6. WEAPON6 keeps scalar
cache/impact. Work46770 adds138, zone8596 shrinks8; Unit112/State2224/FSV4 and
five allocations remain. Actual input64897 leaves39 bytes;25 images in three
shards.626 focused PC/sanitizer/68000 checks and22 unique regression suites
pass.201 generic actions/78 RSM remain complete; wider interactions and the
subsequent TI stabilization/release delivery stay open. See the linked scope
and HANDOVER.md before continuing; preserve live TiEmu, standard groups and
protected save/proof bytes AND dates.

**Current integration — physical weapon formulas, 2026-11-05:** [CORE_WEAPON_FORMULAS.md](CORE_WEAPON_FORMULAS.md).
Source PA/Speed, Brave, MA, variable and mixed-stat weapon bases now feed
Attack/Charge, previews/AI and responses. Ordinary guns71..73 use WP squared,
with guards before their evade bypass. Variable power precedes critical RNG;
previews use the source midpoint. Twenty-six more plain compatible weapons
support Two Swords. FCJH49306 changes27 low hand bits (including bare metadata),
without a format/layout change. ABI71 replaces the old preparation callback
with pure weapon_amount; RESPONSE/COMBAT own preparation and never evict an
executing image. Unit112/State2224/Work45608/Save3202/ModCtx352 and five blocks
stay fixed.22 images/zone8566, shards51140/51952/53456; actual TTPack64929
leaves7 bytes.2421 focused assertions agree PC/sanitizers/68000.
Next: weapon elements, healing/drain, magical guns and status/spell procs,
including the remaining post-formula RNG, plus the named whole-engine gates.
Generic counts remain201/78; recover resident headroom before effects grow.

**Historical integration — source weapon reach, 2026-11-05:** [CORE_WEAPON_REACH.md](CORE_WEAPON_REACH.md).
Attack and Charge now share source strike, cardinal lunge, direct and arc ranges
with AI, Counter/Hamedo and Mime. Effective height includes Float/water movement;
right-hand priority, left-only fallback and Frog are retained. Cardinal masks visit neighbors; ranged masks prepare
the origin once. ORDER owns the shared CT/pending loop, recovering786 resident
bytes before the extension. ABI70/ModCtx352 keeps22 images, zone8566,
Unit112/State2224/Work45608, Save3202/FSV4 and five allocation blocks.
Production TTPack64897/64936 leaves39 bytes. Weapon formulas/procs, exact
trajectories/panel victims, innate/AI fidelity and combined frame/stack/residency/
release remain open. Generic counts stay201 actions and78 R/S/M consumers.

**Historical integration — ranged Hamedo, 2026-11-04:** [CORE_HAMEDO.md](CORE_HAMEDO.md).
Hamedo now uses native weapon reach and projectile obstruction, with resident
preflight before effect images execute. It cancels the unpaid command even on
a missed return, retains pending/CT and cannot create a reaction/Mime chain.
Brave now precedes incapacity/return-range rejection as in the original.
ABI69/ModCtx352 keeps22 images, zone8566, Unit112/State2224/Work45608,
Save3202/FSV4 and five heap blocks. Actual TTPack64917/64936 leaves19 bytes.
Native weapon geometry and overlapping-panel target listing retain the explicit
bounds in CORE_HAMEDO.md. Next: broader weapon/proc/innate/AI fidelity and the
combined frame/stack/residency/release checkpoint, not a new generic-family queue.

**Historical integration — automatic Mime, 2026-11-04:** [CORE_MIME.md](CORE_MIME.md).
Allied Mimes now imitate accepted primary actions after responses/rewards, with
source-facing offsets, live eligibility, their own stats and borrowed weapons.
Copies reuse existing effect/response/render owners, do not chain, and spend no
second MP/item/Throw/katana stock. Math, Elements, due Charge/Jump and one-shot
performance pulses have focused coverage. ABI68 adds20 transient Work bytes
(45608); Unit112/State2224/Save3202/ModCtx352 and five heap blocks stay fixed.
NATURE21 op9 fits the existing22-image/8566-byte zone. Actual production input
64805/64936 leaves131 bytes. Generic counts remain201 actions and78 R/S/M.
Next: the named ranged-Hamedo/weapon/proc/AI fidelity limits and combined
frame/stack/residency/release checks. No missing-generic-family queue remains.
Evidence and explicit native bounds: CORE_MIME.md and `x/core-mime/verification.json`.

**Historical Discovery/monster delivery block D, 2026-11-04:** [CORE_DISCOVERY.md](CORE_DISCOVERY.md).
The last six generic R/S/M producers are implemented: Finger Guard448, Train470,
Secret Hunt471, Monster Talk473, Monster Skill478 and Move-Find Item509.
Generic actions remain201/201; bounded R/S/M consumers are78/78. Real source
monster jobs/lists and map/loot records feed executable Chocobo Attack/Cure/
Esuna, recruitment/poaching and once-only discovery. Train respects existing
Invite/immortal/immunity; Choco Attack shares physical forecasts and Golem.
Poaching directly credits capped blue inventory; no Fur Shop UI. Recruited
monsters remain in reserve pending compatible art/content, with no human
sprite/portrait or human job conversion.
FSV4/Save3202 appends64 discovery bytes and migrates FSV3/3138 and FSV2/3110;
old offsets and SaveUnit176 remain. FCJH49306 adds2720 source loot/map/alias/
price bytes. Unit112/State2224 stay fixed; Work45588 adds two16-bit masks;
five allocation blocks remain. ABI67/ModCtx352 has22 images, zone8566 and three
7/7/8 shards51204/51062/49714. Actual TTPack64581/64936 leaves355 bytes; packed data39579.
NATURE21 owns deferred monster updates, cold Math masks and chapter loading;
owners return before loading another image. Source2916 profiles feed17635
logic assertions(4880B193) plus35 management assertions(A633E2AD), equal PC,
ASan/UBSan and actual68000. All35 affected suites close; all nine campaign unit
suites and20 private campaign input/pixel/state/save replays pass. Final native
Gariland/Orbonne retain81/14 checks(AE9CFD31/2CD70391), with exact FSV4 VAT bytes.
Protected522 files retain bytes/dates; actual ABI66 banks reject before execution.
**Next: final generic-core integration**, especially automatic Mime imitation,
the named ranged-Hamedo/weapon/proc/AI limits and whole-frame/stack/residency/
release checks. Full monster encounters/art/actions and a full campaign remain
separate. Do not restart a missing-generic-family queue. Evidence:
`x/core-discovery/verification.json` and CORE_RSM_REMAINING.md.

**Historical Combat/protections delivery block C, 2026-11-04:** [CORE_COMBAT.md](CORE_COMBAT.md).
Twelve generic R/S/M IDs now have native consumers: Concentrate/Throw Item,
Weapon Guard/Catch/Blade Grasp/Arrow Guard, Meatbone Slash/Counter Magic/Hamedo,
and Distribute/MP Switch/Damage Split. Ordinary Attack also shares Abandon and
the four-stage physical evade path. Hamedo cancels the unpaid root, including
both hands and a due Charge; its current reactor policy is melee-only, explicitly
adapted. Catch settles inventory once; MP redirection and frozen sharing retain
secondary ownership, KO/Wall filters and root reward settlement. Throw Stone
also uses physical evasion. Applied area MP damage refreshes the cached label.
572 bounded retail profiles/outer Hamedo trials feed2975 logic+12 UI assertions
and four existing-renderer captures, equal PC, ASan/UBSan and actual68000.
Forty-two affected regression suites close, with six targeted final rechecks;
Gariland/Orbonne remain81/14 checks, AE9CFD31/2CD70391, with exact private VAT
saves, repeat payment and trophy overflow. Protected517 files retain bytes/dates.
ABI66/ModCtx352 uses21 images, zone8730, three7/7/7 FMB3/FMD2 shards:
50728/50240/44748 bytes. Actual TTPack64861/64936 leaves75 bytes;
private packed data39763. FCJG46586 exports weapon physical evade in existing
Item.sev bytes. Unit112, State2224, Work45584, Save3138/FSV3 and five heap blocks
stay fixed. COMBAT20 constructs scalar commands after COMMAND returns;
MOVEMENT18 owns reach/path/aim5/7/8/10; EQUIPMENT6/7 constructs numerical/self
responses. ACTION's sword-skill splash shares a small pure amount helper.
Next: blockD's six producers — Finger Guard448, Train470, Secret Hunt471,
Monster Talk473, Monster Skill478 and Move-Find Item509. Generic actions stay
201/201; native R/S/M consumers72/78. Full ranged Hamedo, broader weapon/proc/
AI fidelity, Mime imitation, whole-frame/stack/residency and release remain
explicit final integration limits. See `x/core-combat/verification.json`.

**Historical Equipment/progression delivery block B, 2026-11-04:** [CORE_EQUIPMENT.md](CORE_EQUIPMENT.md).
Seventeen generic R/S/M IDs now have consumers: eight Equip permissions,
Two Hands, Equip Change, Gained EXP/JP Up, four movement benefits and Gilgame
Heart. Equipment rules are shared by management/battle; a five-slot draft
commits stock atomically, spends Act and preserves live HP/MP. Source457 keeps
its Equip Knife label and grants katanas. Victory now persists the complete
live inventory and surviving deployed units' equipment, without duplicate
transfers. Movement benefits settle once and lock undo; responses freeze damage.
93273 focused assertions and10 interface captures agree PC/sanitizers/68000,
including an actual FSV3/VAT save round trip. Thirty-five affected regression
suites close, with targeted hand/fixture repairs and final Gariland/Orbonne flows.
Equip UI draw peaks293060 cycles; measured input302986; these are bounded
zones, not whole-tactical-frame certification. ABI65/ModCtx360 has20 images,
zone8532 and three FMB3/FMD2 shards7/7/6:50028/44928/42100 bytes.
Actual TTPack64477/64936 leaves459 bytes; private packed data39667.
Unit112, State2224, Work45584, Save3138/FSV3; five heap blocks remain.
FCJF46586 appends2048 archived equipment/support bytes; old FCJE is rejected.
Generic actions stay201/201. Eighteen R/S/M IDs remain: next blockC12, thenD6.
Native right-hand shields, broad weapon-formula/proc/art fidelity, Mime,
whole-frame/stack/residency and release gates remain separate/open.

**Historical Movement delivery block A, 2026-11-03:** [CORE_MOVEMENT.md](CORE_MOVEMENT.md).
All15 traversal capabilities in blockA now have equipped/innate consumers:
Move/Jump bonuses, Ignore Height, terrain/weather/water/lava, Float, Fly and
Teleport. Teleport rolls once, spends Move on failure and cannot be undone;
equipped Float grants innate Float/Earth protection. Source128 configurations,
16 bonus boundaries and24 Teleport trials feed shared native checks, with32
real-renderer pictures in four orientations. Bounded same-layer direct flight/
Teleport and the prior ordinary-water adapter remain explicit adaptations.
ABI64/ModCtx360 has19 images, zone8640 and three FMB3/FMD2 shards6/6/7:
43440/44018/44938 bytes. Actual TTPack64175/64936 leaves761 bytes;
private packed data39327. Unit112, State2224, Work45582, Save3138/FSV3;
five heap blocks remain. FCJE adds864 archived terrain/innate bytes.
Generic actions stay201/201;35 R/S/M IDs remain across B17/C12/D6.
Next: blockB equipment/progression, including the four movement benefits.
Full-frame/cold-query, stack/residency, unique actor and release gates stay open.

**Historical Counter reactions addition, 2026-11-03:** [CORE_COUNTER_REACTIONS.md](CORE_COUNTER_REACTIONS.md).
Counter Tackle436 reuses Dash; Counter Flood437 freezes a terrain-selected
Geomancy area response; Absorb Used MP438 restores the incoming effective MP
cost, frozen at staging. Secondary area commits are once-only, without nested
reactions or rewards. Source60 stages/nine effects/four constructor cases feed
813 logic+34 UI assertions/12 pictures, equal PC/sanitizers/68000.
ABI63/ModCtx360 keeps18 images/9+9 shards, zone8452. RESULTS3 selects reactions,
RESULTS4 owns strike target queries, TARGETS11 prepares Flood; ORDER16 now owns
the existing post-clock reset. Banks63962/63918; actual TTPack64867/64936
(69 bytes left), private packed data39969. Gameplay/save layouts, allocation
formulas, FCJD and201/201 generic coverage stay fixed. Full R/S/M, special actors,
whole-frame/stack/residency and release gates remain open.

**Historical Brave/Faith/Transparent reactions addition, 2026-11-03:** [CORE_BRAVE_FAITH_REACTIONS.md](CORE_BRAVE_FAITH_REACTIONS.md).
Brave Up429 and Faith Up430 add3, capped100; Sunken State425 grants Transparent.
Physical pre-impact eligibility and effective-MP eligibility stay distinct.
Transparent bypasses evasion, hides hostile AI utility and clears on HP damage,
a later acted turn or accepted Jump. The reviewed outlined sprites remain.
Source69 stages/37 effects/three target lists plus evade/removal fixtures feed
1372 logic+98 UI assertions/14 pictures, equal PC/sanitizers/68000.
ABI62/ModCtx360 keeps18 images/9+9 shards/zone8390. FORMULA14 prepares typed
self effects after RESPONSE returns; RESPONSE6/7 owns consecutive strikes;
ORDER16 handles Defend/Transparent turn entry. Banks62374/63840; actual TTPack
64927/64936 (9 bytes left), private packed data40122. Gameplay/save sizes and
allocation formulas stay fixed. FCJD and201/201 generic actions are unchanged;
full R/S/M, cold-frame/stack/residency and release gates remain open.

**Historical self-status reactions addition, 2026-11-03:** [CORE_STATUS_REACTIONS.md](CORE_STATUS_REACTIONS.md).
Caution426 adds Defending, Dragon Spirit427 adds Reraise and Regenerator428
adds Regen through the existing equipped-response/status owners. Caution and
Dragon use pre-impact physical eligibility (including a miss); Regenerator
uses post-HP-damage eligibility. Source60 stages/33 effects/three target lists
feed1159 logic+117 UI assertions with18 pictures, equal on PC/sanitizers/68000.
Status immunity/no-op/Wall, late filtering, timer refresh/expiry, KO revival,
root reward ownership and direct-command rejection are covered. FCJD and
201/201 generic coverage stay fixed; full R/S/M and release gates remain open.
ABI61/ModCtx360 retains18 images,9+9 FMB3/FMD2 shards and zone8390; banks
62170/62930, actual TTPack64905/64936 (31 bytes left). Gameplay/save layouts and all five allocation formulas are
unchanged. Actual packing and the scoped regression/preservation evidence are
in the family note and private verification.json.

**Historical Lancer Jump addition, 2026-11-03:** [CORE_LANCER_JUMP.md](CORE_LANCER_JUMP.md).
The user re-enabled Jump and authorized simpler terrain rules. All twelve source
upgrades now extend one Jump command: fixed target cell, Speed-based delay,
untargetable/hidden actor with reserved origin, then one ordinary damage/reward
resolution. Another occupant may be hit; an escaped target leaves an empty hit.
Targeting stays on the same terrain layer; no bridge/ceiling or new art pipeline.
Generic coverage is201/201; broader R/S/M and production gates remain open.
ABI60/ModCtx360 keeps gameplay/save layouts. RESPONSE5 owns cold Jump orders,
TURN4/5/6 unchanged target-picking queries. FMD2 omits only zero context bytes
from archives and reconstructs original code offsets, without decompression,
extra heap or a new execution model. Zone8390, banks62018/62010, actual packed
input64883/64936 (53 bytes left); gameplay/save layouts and allocation formulas
are unchanged. See the family note and private evidence.

**Historical critical HP/MP/Quick reactions addition, 2026-11-03:** [CORE_CRITICAL_REACTIONS.md](CORE_CRITICAL_REACTIONS.md).
Equipped HP Restore, MP Restore and Critical Quick now share post-damage
critical/Brave staging, frozen self effects and the bounded response queue.
Wall misses, Undead healing, immutable Critical traits and complete root/Quick
settlement are source-checked.87 original stages/24 effects/three target lists
feed1031 logic and72 UI assertions on PC/sanitizers/68000, with18 pictures.
ABI59/ModCtx360 shares the pure critical predicate through target_query253;
RESPONSE4 owns the unchanged ordinary-attack constructor, recovering resident
room, and drops an unused reward wrapper.18 images/9+9 shards/zone8254;
banks63722/63938. Actual TTPack64847 leaves89 bytes. Gameplay/save layouts
and the five allocations stay fixed.
FCJD/generic189 of201 and user-deferred Jump12 are unchanged. Source, packing,
regressions, preservation and the open R/S/M/full-frame gates are in the family
note and local x/core-critical-reactions/verification.json.

**Historical PA/MA/Speed Save reactions addition, 2026-11-03:** [CORE_STAT_SAVE.md](CORE_STAT_SAVE.md).
Three equipped reactions now reuse post-HP-damage staging, Brave, the bounded
secondary context and ordinary stat labels. They add one PA/MA (cap99) or Speed
(cap50), once per response, without stock, MP, turn credits or reaction EXP/JP.
Thirty-six original finalizer profiles and eighteen original secondary effects
cover the source staging/clamps. FCJD and189/201 generic action coverage stay
unchanged; the twelve Jump entries remain user-deferred.
ABI58/ModCtx360 removes two unused evaluator callbacks and shares pure reaction
eligibility through target_query254. AREA_UI14 now owns existing impact glyph
preparation/drawing; NUMBERS15 retains forecast text/panels. The18 images keep
9+9 shards, with an8254-byte zone and63684/63380-byte banks. Gameplay/save layouts
and five allocation formulas stay fixed. Production input, native checks,
picture equality, narrow margins and remaining R/S/M/full-frame gates are
recorded in the family note and local verification.json.

**Historical Arithmeticks / Math Skill addition, 2026-11-02:** [CORE_MATH.md](CORE_MATH.md).
Eight learned numeric selectors combine CT/Level/EXP/Height with Prime/5/4/3,
then apply one of45 eligible learned White/Black/Time/Yin-Yang spells globally.
The shared action is immediate and MP-free, bypasses Reflect/Counter and retains
magical evasion, frozen impacts, normal status effects and rewards. FCJD adds316
archived bytes (43674 total), with no new source spell IDs or gameplay/save RAM.
195 nonempty source rows include20 Throw/Math selectors;255 commands execute,
129 ordinary cast records are reused, and189/201 generic entries are enabled.
Only the12 Jump entries remain, still user-deferred; the full release gate is open.
ABI57/ModCtx368 replaces the target-validity callback with pure target_query and
removes unused bindings and derives job_sets from the catalogue. TURN1/2/3 own Math menu/Attack range/Math reach;
BEATS4 owns camera target calculation; SCREENS12 owns catalogue validation.
The18 images keep the9+9 split and8254-byte zone, with banks63990/63888 below
64000 each. Acquisition for actors without saved cross-job learning, bounded
stationary AI, original movement/mount limits and whole-frame budgets remain
explicit adaptations/limits. Validation and preservation evidence are in the
family note and local x/core-math/verification.json.

**Historical Throw addition, 2026-11-02:** [CORE_THROW.md](CORE_THROW.md).
The twelve Ninja learning selectors expose80 throwable source items through the
shared menu, stock, Speed/WP/physical/elemental formula, frozen effects and AI.
FCJC retains512 source metadata entries and extends its normalized prefix to640
archive records;187 source rows include12 selectors, while255 commands execute
(175 previous plus80 item adapters).129 owned casts;181/201 generic entries,
20 remain: Arithmeticks8 and Jump12 (user-deferred). ABI56/ModCtx388 reuses an
unused callback for pure resident eligibility. TARGETS owns native reach/path/
range-menu preparation; PARTY14 owns the cold scenario presentation reset.
4309+61 PC/sanitizer/68000 assertions,180 source profiles and60 scoped suites.
Zone8268, banks62300/62554, raw64489/actual TTPack64491/headroom445.
Gameplay/save layouts and all five allocations remain fixed. Source, native
checks, preservation and the explicitly open cold-overlay/RSM/full-core gates
are recorded in the family note and local verification.json.

**Historical Float addition — 2026-11-02:** [CORE_FLOAT.md](CORE_FLOAT.md).
The last ordinary generic spell (id38) reuses formula0x0b and owned status casts:
CT2, MP8, Float17, Earth protection and Dispel.16 original formula profiles,
12 Float moves plus12 normal controls and the actual movement configuration
feed PC/sanitizer/68000 checks. Native narrow/wide navigation uses bit2 in its
existing search flags, with no new state/cache/allocation.175 records,129 casts,
169/201 generic entries;32 remain (Throw12, Jump12 deferred, Arithmeticks8).
ABI55/ModCtx388,18 images/zone8312, banks63026/63708. Raw variable64771;
actual TTPack input64773/headroom163. Unit112/State2210/Work45576/Save3138/
FSV3 and five allocation formulas remain fixed. Tile-height rendering/targeting,
equipped movement abilities and the inherited normal-water adaptation retain
the explicit boundaries documented in the family note.

**Historical Charm/Invitation addition — 2026-11-02:** [CORE_ALLEGIANCE.md](CORE_ALLEGIANCE.md).
Steal Heart and Invitation share owned status effects, current/permanent camp,
Charm expiry/damage removal, source inventory ownership, and bounded victory
recruitment.236 original formula profiles,18 team traces, repeated Charm,
outcome/stock observations and416 source job/item immunity records feed the
PC/sanitizer/68000 checks. Production174 records,128 casts,168/201 generic
entries (33 remain). ABI54/ModCtx388,18 images/zone8312; input64683/headroom253.
The new Unit.allegiance uses the former byte73 padding: Unit112/State2210/
Work45576/Save3138/FSV3 and allocation requests stay fixed. PERFORMANCE7/8
owns social commits; COMMAND0 receives transient arguments and validates before
State mutation. Jump stays deferred; source and campaign-policy bounds are
explicit in the family note.

**Historical Songs/Dances / Bard/Dancer addition — 2026-11-02:** [CORE_PERFORMANCE.md](CORE_PERFORMANCE.md).
All fourteen performances reuse global frozen area effects and the existing
Performing/deferred slot: HP/MP, Speed/PA/MA, random statuses and CT effects
repeat at their source periods. Movement retains the performance; replacement
commands and cancelling statuses end it. Production172 records,126 casts,
166/201 generic entries (35 remain).180 original formula profiles, six footprints
and repeated clock/Quick traces feed5507 assertions7FF2F648 on PC, sanitizers
and68000. See the family note.
ABI53/ModCtx388,18 images/zone8240, input64665/headroom271. Gameplay/save
layouts and allocation requests stay fixed. PERFORMANCE owns persistent orders,
pulses and mixed commits; SCREENS11 owns cold allocation/bootstrap, PARTY10/13
cold chapter/scenario setup. Jump remains deferred.

**Historical Draw Out / Samurai addition — 2026-11-02:** [CORE_DRAW_OUT.md](CORE_DRAW_OUT.md).
All ten techniques share frozen area effects, HP/MP damage, Murasame healing,
Protect/Shell and Haste/Regen, conditional Slow/Confusion/Sentence, and the
source katana availability/break rules. Kikuichimonji reuses cardinal lines.
142 original formula profiles,36 break groups and12 footprints;4625 assertions
5AF7EF14 agree on PC, sanitizers and68000. Production158 records,112 casts,
152/201 generic entries (49 remain).
ABI52/ModCtx388,17 images/zone8240; input64653/headroom283. Gameplay/save
layouts and allocations remain fixed. SAVE7/8 owns ordinary batch commit;
SCREENS11 owns cold bootstrap with an explicit resident campaign-flow policy.
See CORE_DRAW_OUT.md for source bounds and verification; Jump stays deferred.

**Historical linear attacks / Earth Slash addition — 2026-11-01:** [CORE_LINE.md](CORE_LINE.md).
Shared origin-based line footprints, cardinal command/range/AI selection and
nearest-first formula preparation enable Earth Slash. Source Earth damage,
Float/null/absorption, critical/proc draws and frozen single-victim push reuse
the common packet/commit paths.27 original footprints and75 formula profiles;
2786 PC/sanitizer/68000 assertions59AFCB33. Production148 records,102 casts,
142/201 generic entries (59 remain). ABI51/ModCtx388,17 images/zone8236;
TTPack input64735/headroom201, unchanged gameplay/save layouts and allocations.
Draw Out still needs its full ten-producer/inventory family; Jump stays deferred.
See CORE_LINE.md for verification scope and the remaining source-policy limits.

**Historical Secret Fist addition — 2026-11-01:** [CORE_SECRET_FIST.md](CORE_SECRET_FIST.md).
Formula0x50's generic producer uses MA with physical supports/target modifiers,
no Faith and no evasion (the retail evade flag is clear), then Death Sentence.
Shared frozen status/timer/KO/reward and AI paths;185 original profiles and
4067 PC/sanitizer/68000 assertions1FCD638D. Production147 records,101 casts,
141/201 generic entries (60 remain). ABI50/ModCtx388: PARTY13 groups ordinary
cold setup; core RAM/save layouts and17 images/zone8186 stay fixed.
TTPack input64559/headroom377. All30 direct suites and both native pilot
flows pass; actual prior-ABI49 banks are rejected. Full core remains incomplete;
see the family note for scope and limitations.

**Historical Geomancy addition — 2026-11-01:** [CORE_GEOMANCY.md](CORE_GEOMANCY.md).
All twelve terrain attacks share source formula0x24, elemental damage and a
frozen19% single-status proc. FCJB holds the64-entry terrain mapping; FGS2
appends both source surface types without changing collision columns or core
RAM layouts. Player commands and each AI origin enforce terrain eligibility.
ABI49/ModCtx388; cold scripted setup moves to PARTY12. Production146 records,
100 casts,140/201 generic entries (61 remain). Source279 profiles/128 terrain
calls;8927 PC/sanitizer/68000 assertions4BF14699. Headroom195/zone8186.
Shared area AI now resets its candidate before dispatch. Final closure and
pilot-flow evidence: see CORE_GEOMANCY.md and x/core-geomancy/.

**Historical gil transfers addition — 2026-11-01:** [CORE_GIL.md](CORE_GIL.md).
Gil Taking and Negotiate share the original Speed x level amount, capped by
player-target funds, with physical and Talk accuracy respectively. The existing
Save tail holds the live balance; only victory persists it. No Unit/State/Work,
allocation or FSV3 growth. ABI 48 adds PARTY11 for ordinary/scripted initialization.
Production: 134 records, 88 casts, 128/201 generic entries (73 entries remain).
Source oracle: 134 profiles; 3262 PC/sanitizer/68000 assertions (2020738D).
All 69 suites pass, followed by the seven-suite guest correction closure and
both native pilot flows. Resident headroom211, zone8186; see CORE_GIL.md.

**Historical equipment steals addition — 2026-11-01:** [CORE_STEAL_EQUIP.md](CORE_STEAL_EQUIP.md).
Steal Helmet/Armor/Shield/Weapon/Accessory (formula 0x26): the breaks' slot plus
the accessory (nothing to steal: a forced failure), range 1, physical evasion,
Steal Exp's (Speed' + zodiac(X))%, Maintenance resists. The item and its
bonuses leave the target; a player's thief adds it to the campaign inventory
(99 owned: price / 4 gil). 320 original profiles; `core-steal-equip-check`
4478 assertions, F66BB1DC. Production: 132 records, 87 casts, 126/201 generic
entries. `charge_query`'s formula mask saves ~110 resident bytes: headroom 313,
zone 8148 (NUMBERS). All 68 suites green; only `actions` changes (69BA66A5).

**Historical Magic Break addition — 2026-11-01:** [CORE_MP_BREAK.md](CORE_MP_BREAK.md).
Magic Break (formula 0x2c) leaves its Orbonne adaptation: weapon reach,
physical evasion, the stat breaks' (PA' + zodiac(Y))%, then ceil(max MP x 50 /
100) MP lost, MP clamped at 0; the Knight's set is complete. 74 original
profiles; `core-mp-break-check` 790 assertions, C7D8B92A. Production
unchanged at 127 records, 87 casts, 121/201 generic entries. Headroom 251,
zone 8108. All 67 suites green; only `actions` changes (2B7773A6).

**Historical equipment breaks addition — 2026-11-01:** [CORE_EQUIP_BREAKS.md](CORE_EQUIP_BREAKS.md).
Head, Armor, Shield and Weapon Break (formula 0x25): the slot by ability id
(the weapon of the higher required level), weapon reach, physical evasion,
(PA' + zodiac(WP + Y))%, Maintenance resists; an empty slot becomes the
ordinary Attack. The item and its bonuses are lost (a lost weapon ends its
Charge). 228 original profiles; `core-equip-breaks-check` 3255 assertions,
0AF4C46E. Production: 127 records, 87 casts, 121/201 generic entries. Room:
the resident sends formulas 0x25..0x2b to FORMULA, so the AI/ACTION evaluator
copies drop the breaks, Steal Exp and ACTION's hit evaluation (ACTION 8162 ->
7314); `skill_range` moves from COMMAND to TARGETS. Zone 8108 (AI), headroom
255. FCJ carries the items' required levels. All 66 suites green; only
`actions` changes (2ECB73A6).

**Historical stat breaks addition — 2026-11-01:** [CORE_BREAKS.md](CORE_BREAKS.md).
Speed, Power and Mind Break (formula 0x2b): weapon reach, physical evasion,
then (PA' + zodiac(Y))%; the stat (by ability id) loses X, at least 1. Speed
Break leaves its Orbonne adaptation; Magic Break (0x2c) keeps its own. Martial
Arts applies bare-handed only (source; Steal Exp's hash unchanged). 213
original profiles; `core-breaks-check` 2016 assertions, AE26B19D. Production:
123 normalized records, 87 casts, 117/201 generic entries. All 65 suites green
(`closure.py --all`); only `actions` changes (F091239D, the new records).

**Chapter-loading relocation — 2026-11-01:** the cold part of `chapter_load`
(bank lookups, view footer and descriptor validation, chapter sizes) moves to
`chapter_files.inc`, the SAVE image's op 6 on the TI (resident on the PC); the
resident adopts the image's context afterwards. ModCtx ABI 47 (388 bytes:
`rt_file`, the `chapter_ugfx` output). Headroom 17 -> 331 before the breaks,
185 after (zone 8246). The rest of the start-up code mostly calls other images
(a module cannot call one), so it stays resident; the remaining resident is
hot engine code or 14-byte image stubs. talk-skill and area-impact fit again.

**Historical Steal Exp addition — 2026-11-01:** [CORE_STEAL_EXP.md](CORE_STEAL_EXP.md).
The Thief's Steal Exp: physical evasion, then (Speed' + zodiac(X))%; it steals
min(Speed + Y, 100, target EXP), the thief levelling up at once before the
ordinary reward. 100 original profiles; `core-steal-exp-check` 1038 assertions.
Production: 121 normalized records, 87 casts, 115/201 generic entries. 62 of 64
suites green (hashes unchanged except `actions`, B1F19D7E, from the new entry
only); talk-skill and area-impact check binaries exceed the 68000 limit again:
production headroom 17, a further relocation comes before the next family. The
five stale older suites (casting, incantation, elements, magic, jump) were
repaired first (CORE_TESTING.md).

**Historical module rebalance — 2026-11-01:** Guts had pushed the
FORMULA image to 8392 bytes; the module zone equals the largest image and is
part of every core program (the real production headroom was negative; the
83 bytes noted for Guts were measured against a stale `data/modules.h`).
Turn-end status runtime (ops 4/5/14/15: tick, upkeep, dead turn) moves from
FORMULA to ORDER, which the turn order loads next anyway (one image load less
per turn); the objective/result banners move from NUMBERS (3/4) to ORDER (6/7),
never drawn with damage numbers. `hand_weapon` and `restore_ability` are
shared through ModCtx instead of copied per image (ABI 46), and
`magic_formula`/`magic_status_formula` are 13-byte bitmaps (all 256 ids checked
equal). Images: FORMULA 7016, NUMBERS 7290, ORDER 5130, AI 8094 (now the
zone), ACTION 7940. Zone 8392 -> 8094; resident headroom 235 (scratch raw build
64701). All 37 family suites green with unchanged hashes (`closure.py`);
talk-skill and area-impact fit again. Behaviour unchanged.

**Historical Guts addition — 2026-10-31:** [CORE_GUTS.md](CORE_GUTS.md).
Ramza's Yell (Speed +1), Cheer Up (Brave +5) and Scream (Brave +10, PA/MA/Speed
+1) are immediate owned area casts that always hit, without a draw; a KO,
Petrified or Wall target is a cancelled miss. New AB_SPEED (cap 50); Scream
also raises PA/MA (cap 99). 117 original profiles; `core-guts-check` 1546
assertions. Production: 120 normalized records, 87 casts; Guts is not among the
201 generic entries (114/201). Resident headroom then negative, restored by the module rebalance. Full core remains
incomplete; Jump deferred. Preserve live TiEmu, standard playtest, saves and
old proof bytes AND dates.

**Historical Spin Fist addition — 2026-10-31:** [CORE_PUNCH_ART.md](CORE_PUNCH_ART.md).
Punch Art's area strike: formula 0x31 on radius 1 around the user (excluded),
Attack's physical evasion per victim, a 4% critical without knockback (the
source knocks back only single targets). It rides the owned area pipeline,
the damage reusing `punch_strike`. `core-punch-strikes-check` 6524 assertions.
All 37 family suites green; `actions` changes (E0988BD3) only through the
Spin Fist entry (FCF28AB1 without it).
Production: 117 normalized records, 84 casts, 114/201 generic entries (87
remaining entries are not 87 effects). Resident headroom about 153.

**Historical Talk Skill family — 2026-10-31:** [CORE_TALK_SKILL.md](CORE_TALK_SKILL.md).
Persuade, Praise, Threaten, Preach, Solution, Death Sentence, Insult and Mimic
Daravon are enabled as immediate owned area casts: hit (zodiac(MA) + zodiac(X))%, no
Magic AttackUP or Faith, a sleeping target misses without a draw. Brave/Faith
records carry a signed loss (Foxbird shares the commit), new AB_FAITH clamps
0..100, Persuade writes CT 0 through AB_QUICK; Death Sentence counts the
unit's turns in the end-of-turn upkeep (FORMULA15) and kills non-Undead units
at 0. 344 original profiles and 12 upkeep traces; `core-talk-skill-check` 4616
assertions. Production: 116 normalized records, 83 casts, 113/201 generic
entries (88 remaining entries are not 88 effects). Invitation and Negotiate
stay gated. Three stale checks
(knockback, targets, batch) were repaired first. Resident headroom about 227,
zone 8182. Full core remains incomplete. Preserve live TiEmu, standard
playtest, saves and old proof bytes AND dates.

**Historical Punch Art family — 2026-10-30:** [CORE_PUNCH_ART.md](CORE_PUNCH_ART.md).
Stigma Magic, Chakra, Revive, Wave Fist and Repeating Fist are enabled.
Support (formulas 0x33/0x34/0x35): immediate owned area casts from PA with
Attack UP x4/3 and Martial Arts x3/2, no Faith (99 original profiles,
`core-punch-art-check` 1256 assertions). Strikes (0x31/0x32): single-target
physical skills with Attack's evasion, a 4% critical with knockback and
Repeating Fist's 1..9 hits, behind the exported `charge_query` (276 original
profiles, `core-punch-strikes-check` 4412 assertions). Resident room first:
chapter grid tables moved into PARTY op 10, and `FFT_PRODUCTION_CORE` drops
twelve test-only exported entries from fftbat. Production: 108 normalized
records, 75 casts, 105/201 generic entries (96 remaining entries are not 96
effects). Spin Fist, Earth Slash and Secret Fist stay gated. Resident headroom
about 323, zone 8170. Full core remains incomplete. Preserve live TiEmu,
standard playtest, saves and old proof bytes AND dates.

**Historical protection family — 2026-10-30:** [CORE_PROTECT.md](CORE_PROTECT.md).
Reraise and Golem are enabled together. Reraise (status set 33) survives KO;
at a KO unit's turn it revives with max HP/10 (one upkeep draw; 0 HP spends it
and keeps the counter). Golem (formula 0x14) fills its side's pool with the
caster's max HP; a primary Attack/Charge on that side is absorbed whole and
drains the pool (source BLOCKED_BY_GOLEM, ability 0's range data). State gains
`golem[2]` (4 bytes): MOD_SIG changes, older banks are rejected. 28 original
formula/finalizer profiles and six Reraise upkeep traces; `core-protect-check`
460 PC/sanitizer/68000 assertions. Production: 103 normalized records, 72
casts, 100/201 generic entries (101 remaining entries are not 101 effects).
End-of-turn upkeep moves into FORMULA15; resident headroom about 443. Float
waits for L1 movement with Jump. Full core remains incomplete. Preserve live
TiEmu, standard playtest, saves and old proof bytes AND dates.

**Historical control family — 2026-10-30:** [CORE_CONTROL_STATUS.md](CORE_CONTROL_STATUS.md).
Frog, Blind Rage (Berserk), Confusion Song and Foxbird (Brave -30, Chicken
below 10) are enabled together. Shared handlers: a cached STATUS_NO_CONTROL bit
hands Berserk/Confusion/Chicken units to the AI (Chicken flees, Berserk Attacks
its nearest enemy, Confusion draws ability and target of any team); source
physical XA order (Berserk x3/2, Frog XA 1, target Chicken/Frog x3/2);
Confusion halves base hit and is cleared by HP damage; a Frog fights
bare-handed (reach 1) and may only Attack or cast Frog, Frog toggles; Brave
clamps 0..100 and gains +1 per turn below 10. 56 original formula/finalizer
profiles; `core-control-status-check` 1509 PC/sanitizer/68000 assertions.
Production: 101 normalized records, 70 casts, 98/201 generic entries (103
remaining entries are not 103 effects). Resident room was recovered first:
`opening_after` compiles into BEATS, Wait arrows into ORDER, `ai_choose` into
AI_SETUP5, `ai_clear` uses MULS. Resident input about 64451 (headroom about
485, from 7); zone 8116 (AI). ABI 45 rejects older banks. Stale sleep,
incapacity and area-impact checks were repaired (area-impact's native binary
fits again). No frog/chicken sprite. Next grouped family: Reraise and Golem
(Float belongs with deferred L1 movement). Full core remains incomplete.
Preserve live TiEmu, standard playtest, saves and old proof bytes AND dates.

**Historical decisive family — 2026-10-30:** [CORE_TERMINAL.md](CORE_TERMINAL.md).
Death, Quick, Zombie and Petrify are enabled together as one shared target/
status/clock family. 56 original formula/finalizer profiles and 12 upkeep
traces pin Death (Dead plus nominal max-HP damage, Undead healing), Quick
(CT255 handed to the Quick-priority scheduler), Petrify (statue keeps HP/CT/
tile, leaves ordinary elimination, curable) and Zombie (Undead bit; KO-expiry
revival draw). 1702 family assertions (1292/5DEE6FD1 rules, 410/C4AE09F8
edges) agree on PC, sanitizers and 68000. KO-counter and end-turn CT-credit
policies move unchanged from resident code to FORMULA14/15; the Wait arrows
use a 16-byte Jacobian table (-34 bytes). Production: 97 normalized records,
66 casts, 94/201 generic action entries (107 remaining entries are not 107
effects). ABI 44/signature 1438, 17 images split 8+9, static zone 8228 (not
heap). Private group 539377 bytes (+1571); resident input 64929, headroom 7:
the next resident growth needs a relocation first. Unit/State/Work/FSV3 and
five core allocations fixed; 27 other data banks identical. Both native pilot
save/results flows, old frames and 18694 preserved paths pass. Next grouped
family: Reraise/Frog/Float/Berserk/Confusion and Brave/Faith control. Full core
remains incomplete; Jump remains deferred. Preserve live TiEmu, standard
playtest, saves and old proof bytes AND dates.

**Historical resource-restoration family — 2026-10-30:**
[CORE_RESTORATION.md](CORE_RESTORATION.md). Hi-Potion, X-Potion, Ether, Hi-Ether
and Elixir are enabled together; Potion/Phoenix Down use the same seven-producer
scalar pipeline. All 14 ordinary Item records are enabled. Frozen paired HP/MP
quantities reuse existing packet bytes; action-start forecast invalidation and
no-effect item rewards are fixed. Original evidence: 80 formula/finalizer
profiles and 38 actual HP/MP store traces. Undead Elixir damages HP and still
restores MP. The 2481 family assertions agree on PC, sanitizers and 68000;
relocated menu/view controls retain State/Work and the 1.5-second AI pause.
Production: 93 normalized records, 62 casts, 90/201 generic action entries.
The 111 remaining entries include upgrades/categories, not 111 distinct effects.
ABI 43/376/signature 1181, 17 images split 8+9, static resident zone 8172
(+966, not heap). Private group 537806 bytes (+5982); resident input 64829,
headroom 107. Unit/State/Work/FSV3 and five core allocations stay fixed;
27 other data banks stay identical. Scoped callers, 20 layouts, 12 old frames
and both native pilot save/results flows pass. Full core remains incomplete:
remaining control/barrier/job families, R/S/M, special actors, campaign/objectives,
formats/tools and worst-case RAM/stack/frame gates. Jump remains deferred.
Preserve live TiEmu, standard playtest, saves and old proof bytes AND dates.

**Historical cleansing family — 2026-10-29:** [CORE_CLEANSE.md](CORE_CLEANSE.md).
Esuna, Dispel Magic, Maiden's Kiss, Soft, Holy Water and Remedy ship together.
A frozen ordinal subset spans all five status bytes inside the existing effect
quantity: no new Unit/State/Work/save field or heap block. Innate protection,
source cancellation eligibility, timers, statues, one item debit, CT/MP,
Reflect and actual AI have 1711 PC/sanitizer/68000 assertions and 78 original
profiles. Production has 88 executable records, 62 casts (47 spells/15 summons)
and 85/201 generic action entries. The 116 unavailable entries include upgrades
and categories, not 116 distinct effects. ABI 42/ModCtx 376/signature 924,
17 images, split 9+8, static resident zone 7206 (+86, not a heap allocation).
Cold forecast formatting/cache and unchanged Wait arrows move to NUMBERS;
32 arrow cases and six new cache boxes retain exact pixels. Private group:
531824 bytes (+4415), resident input 64885/headroom 51; 27 other banks unchanged.
Full core/content gates remain open: HP/MP item restoration, remaining status/control/barrier and
job families, R/S/M, special actors, campaign/objectives, formats/tools and full
frame/stack/residency budgets. Keep families grouped and Jump deferred. Preserve
live TiEmu, standard playtest, saves and old proof bytes AND modification dates.

**Historical percentage/resource-transfer batch — 2026-10-29:**
[CORE_PERCENT.md](CORE_PERCENT.md). Demi, Demi 2, Lich, Spell Absorb and Life
Drain ship together: 60 casts, 82 normalized records and 79/201 generic action
entries. The 122 remaining entries include upgrades/categories, not 122 effects.
2396 PC/sanitizer/68000 assertions cover 57 original paired-result profiles,
production records, CT/MP, Reflect, Undead, AI, capped/empty pools and bootstrap.
Caster returns reuse the existing packet; MP effects remain separate from HP.
ABI 41/ModCtx 376/signature 667, 17 images, zone 7120. The unchanged catalogue
validator moves to SAVE5 before Work allocation. The image zone is static
resident storage, not a heap allocation; Unit/State/Work/FSV3 and the five heap
allocations stay fixed. See the linked note and private verification report for
sizes and scoped closure. Private group: 527409 bytes (+1392); resident packed
input: 64845, with 91 bytes of headroom. Full core/content gates remain open:
remaining status/control/barrier and job families, R/S/M, campaign/objectives,
formats/tools
and full frame/stack/residency budgets. Keep effects grouped and Jump deferred.
Preserve live TiEmu, standard playtest, saves and old proof bytes AND dates.

**Historical compatible buff/status batch —2026-10-29:** [CORE_BUFFS.md](CORE_BUFFS.md).
22 retail producers ship together:55 casts,77 normalized actions and 74/201
career action-table entries (127 unavailable entries, not 127 effects).
5682 PC/sanitizer/native checks cover the batch, including 13 distinct actors;
67 original formula profiles/10 Protect cases and 4843 bitmap-kernel checks pass.
Skipping empty status bytes reduces 13-victim impact 441270→296906 backend cycles.
ABI 40/376/sig 410,17 images/zone 7020; shared menu composition moves unchanged to
RESULTS. Private 526017 (+2753), resident 64915/headroom 21.
Full core/content gates remain open. Next compatible family: five
percentage/resource-transfer entries. Keep Jump deferred and preserve live
TiEmu, standard playtest, user saves and old gallery/group bytes AND dates.

**Historical healing family —2026-10-29:** [CORE_HEALING.md](CORE_HEALING.md).
Cure1–4/Raise1–2/Moogle/Fairy are enabled together:33 executable casts,
55 normalized actions and52/201 generic entries (149 unavailable entries,
not149 effects). Source formula0c/0d, Undead, ceil resurrection, source RNG,
Reflect, shared area/AI and once-only MP/impact use one common pipeline.
2110 PC/sanitizer/native assertions11072857 plus75 original formula/finalizer
profiles pass; original25 retain2736/77DAC288. Unchanged banners move to NUMBERS,
with32 exact plane comparisons; old12 screens/20 layouts/native pilot saves
pass. ABI39/376/sig153,17 images/zone6650; private523264(+2925), resident64927/
headroom9.27 other data banks/assets unchanged. Next compatible buffs/status
batch must include required resident relocation, not a separate empty milestone.
Broader core/RSM/campaign/formats/tools/full budgets remain open; Jump deferred.
Preserve live TiEmu/standard group/saves/gallery bytes AND dates.

**Previous Reflect batch —2026-10-29:** [CORE_REFLECT.md](CORE_REFLECT.md).
All25 ordinary offensive casts are executable (15 spells/10 summons), including
11 newly enabled reflectable spells. Generic actions44/201,157 entries remain;
entries are not distinct effects. Shared source Reflect uses one MP payment,
original caster stats/origin, one endpoint/no recursive bounce and the existing
response queue, with no extra state/heap.2736 PC/sanitizer/native assertions
77DAC288,22 bounded original endpoint/guard cases, affected callers/loader,
20 layouts/12 old screens and native pilot saves pass. ABI38/372/sig65428,
17 images/zone6398; private520339(+1512), resident64887/headroom49. Reflect/Carbunkle
producers, remaining magic/status/RSM/campaign/formats/tools/full budgets remain
open; core is incomplete. Next healing/resurrection batch8; Jump deferred.
Preserve live TiEmu/standard group/saves and gallery bytes AND dates.

**Previous family-batch checkpoint —2026-10-29:** [CORE_OFFENSIVE_MAGIC.md](CORE_OFFENSIVE_MAGIC.md).
Retail casts are now enabled: Fire4/Bolt4/Ice4, Meteor and ten offensive summons,
through one source-field classifier and the existing approved area pipeline.
1290 PC/sanitizer/68000 checks08E75784 exercise all14 production records, real
learned lists, unit/panel orders, CT/payment, Silence, HalfMP, team/self filters,
once-only impacts and actual AI inclusion. Generic careers:33/201 action-table
entries enabled,168 remain (entries include upgrades/categories, not distinct
mechanics). All normalized records:36 enabled, including14 casts. Eleven
reflectable formula08 actions remain one gated batch. Other magic/status/heal/
special families remain incomplete; full core is not done. Invocations use
approved generic impacts, not original summon cinematics. Private group518827,
zero growth: only ffcj changes; all runtime code, ABI37/372/sig65171, RAM and
27 other non-code banks are unchanged. Follow the grouped backlog in the linked
note; do not return to a milestone per named ability. Preserve live TiEmu,
standard playtest, user saves and old gallery bytes AND dates. Jump deferred.

**Previous incapacity checkpoint —2026-10-29:** [CORE_INCAPACITY.md](CORE_INCAPACITY.md).
Ordinary elimination now respects source KO membership and native presence;
statues keep HP/tile occupancy but do not sustain a camp. Petrify forecasts,
ordinary impact/splash immunity, queued reactions and actual AI targeting pass
193 PC/sanitizer/68000 checks55D44C55 against30 original outcomes/4 finalizers.
Node255/count256 and terminal injection doors are checked; clipped map contexts
are restored before playable setup. ORDER1 owns the pure outcome query, ORDER0
keeps AT. ABI37/372/sig65171,17 images/split8+9/zone6316(+52), same core layouts/
FSV3/five allocations. Private518827(+245), raw banks40664/39940, resident64765/
headroom171;28 data banks stable. TURN/BEATS/RESULTS/AREA_AI compiler drift has
isolated declaration controls. Scoped actions/reactions/scheduler/tempo/magic/
area CT/bindings/layouts/12 old frames/pilot saves pass. Outcome cold/warm15922/
5172 and wake318536 are backend-only; full frame/stack/residency stays open.
Petrify infliction/cure/presentation/persistence and general source importance/
original-team objective metadata remain gated, as do broader core/content gates.
Jump deferred. Standard playtest/live TiEmu/saves/gallery bytes AND dates stay
unchanged. Evidence: x/core-incapacity/verification.json.

**Historical Haste/Slow checkpoint —2026-10-28:** [CORE_HASTE_SLOW.md](CORE_HASTE_SLOW.md).
Public tempo states, real CT gain and finite/innate AT segments pass748 PC/
sanitizer/68000 checks30A798F0 (45 original gains,4 charge phases,33 AT lists).
Slow wins; real Slow1 can gain0 while AT retains source minimum1. Expiry,
Stop/Sleep waits, cures/immunity/cancel, KO/revive and unscaled Charge work.
ORDER0 replaces FORMULA6: ABI36/372/sig64914,17 images/split8+9/zone6264; directory
+4, same core layouts/FSV3/five allocations. No new reciprocal data. Private
518582(+650), raw banks40644/39636, resident64803/headroom133;28 data banks
unchanged. Cold/warm AT39086/29724, CT phase max8778 and wake266028 are backend
costs, not full-frame/stack/residency certificates. Scoped scheduler/deferred/
public status58/area CT/bindings/12 old frames/layouts/pilot saves pass.
RESULTS/AREA_AI compiler drift is reproduced with unused-declaration controls.
Retail spells and full-core/content gates remain closed; Jump deferred. Next
other status/incapacitation/response/AI, campaign, formats/tools and budgets.
Preserve standard playtest/live TiEmu/saves/gallery bytes AND dates. Evidence:
x/core-haste-slow/verification.json.

**Historical scheduler checkpoint —2026-10-28:** [CORE_SCHEDULER.md](CORE_SCHEDULER.md).
Quick255 now consumes0 CT and can interrupt a settled charge wave before the
next due root. Permanent-freeze/Speed0/KO-guest clocks yield safely; mutable
Stop/Sleep resume without duplicate turns or credits. Ordinary entry remains
synchronous, idle searches use8 ticks and waking updates at most16. Shared Quit
and rotation remain usable.263 PC/sanitizer/68000 checks3056BFD2 and two nonempty
browse-identical HUD hashes pass; wake quantum max256892 cycles is not a full
frame/stack certificate. ABI35/ModCtx372/sig64657,16 images/split8+8/zone6264;
State/Work/Unit/FSV3/five allocations unchanged. FORMULA+14, AI_SETUP-6 and
same-size TURN changes are isolated with compiler controls. Raw banks41080/38606,
private517932(+195), resident64683/headroom253;28 data banks unchanged. Deferred,
controls, owned area CT/restarts, AI pause, bindings/rejection,12 old frame hashes,
layouts and both pilot save flows pass. Next broader status/reaction/targeting
and AI families, campaign contracts, formats/tools and worst-case budgets.
Retail Quick/casts remain gated; full core/content production remains incomplete.
Jump deferred. Standard playtest/live TiEmu/user saves and old gallery bytes/dates
are preserved. Evidence: x/core-scheduler/verification.json.

**Historical approved area-impact integration —2026-10-28:**
[CORE_AREA_IMPACT.md](CORE_AREA_IMPACT.md). Shared per-victim quantities/Miss,
absorbed healing, hit poses/flash, covered centre pulse and Oil feedback now
reuse existing sprites/font. No fake caster result on an empty footprint;
secondary reactions suppress root effects and retain EXP/JP/terminal draining.
416 candidate captures match the accepted review. Native full-render1012 checks
12D9C238/64 individual screens agree;13-victim/two-floor/node255/signed/centre
bounds pass154 checksC56EC8B9. Cache412 aliases idle ai_gain512; State/Work/Unit/
FSV3/five core allocations unchanged. ABI34/ModCtx372/sig64400,16 images/split8+8,
zone6264; NUMBERS2416 also owns unchanged scalar number drawing. Raw shards
41072/38606, private517737(+2173), resident64473/headroom463. Package/affected regression evidence is
in x/core-area-impact/verification.json. No new bitmap bank or retail unlock.
Cold view decompression/full-frame/stack/residency gates stay open;13-number
backend timing is not a full13-actor frame certificate. Next shared scheduler/
status edges (Quick/clock liveness), broader families/AI/campaign/formats/tools.
Full core remains incomplete; mass content production is gated. Jump deferred. Standard playtest,
old review/gallery bytes and dates, user saves and live TiEmu remain untouched.

**Historical approved area-UI integration —2026-10-28:** [CORE_AREA_UI.md](CORE_AREA_UI.md).
The accepted panel/target-list/Unit-or-Panel flow now runs in the shared PC/68000
engine.499 PC/sanitizer/native checks67EED097, eight HUD hashes and four actual
full-screen/state replay checkpoints agree. The list only inspects victims;
Unit/Panel selects centre binding. AreaUi228 aliases idle256-byte AI scratch;
State2192/Work45576/Unit112/FSV3/five allocations stay fixed. ABI33/ModCtx372/
sig64143,15 images/zone6264, shards41066/36084. Private515564(+6146), resident64719/
headroom217;28 non-code banks unchanged. AI camera moved unchanged to AI_SETUP
(1321 native-equal checks); old actions, forecast/CT/magic/AI/pause and pilot saves
pass. First13-unit whole-map HUD peaks375252 cycles: interactive full-frame/
stack/residency gates remain open. Next reviewed per-victim/empty-centre impact
presentation, then remaining families/campaign/formats/tools. Retail casts and
full core/content gates stay closed; Jump deferred. Approved review PNG/GIF and
standard playtest/live TiEmu/gallery bytes/dates remain intact.

**Historical AI-pause checkpoint —2026-10-28:** [CORE_AI_PACING.md](CORE_AI_PACING.md).
User accepts roughly one-second thinking inside the requested frozen AI pause.
The1.5-second elapsed-clock minimum now includes planning instead of adding48
frames afterwards; slow plans add no second full pause.171 PC/sanitizer/native
checks58CBDE4E pass. All14 images/ABI32/State/Work/FSV3/five allocations unchanged;
resident timestamp adds2 static bytes. Private509418(-24), resident64467/headroom469.
The prior general setup peak remains measured, but is not a30fps blocker while
frozen. Next reviewed area targeting/per-target/empty presentation. Worst total
plan latency, other core and production gates remain open; Jump deferred.

**Historical cached area-AI checkpoint —2026-10-28:** [CORE_AREA_AI.md](CORE_AREA_AI.md).
The actual AI now chooses ordinary hostile formula08 area centres, including
empty panels, with cached current effects, friendly-fire and absorption scoring.
676 PC/sanitizer/native checksD04607FA;12 bounded original HP/accounting profiles,
five rank comparisons and three empty-panel visits. This is the immediate-HP
subset, not a complete original future-CT/status/Reflect/MP-utility chooser.
ABI32/ModCtx364/signature63878,14 images/zone6218; State/Work/FSV3/five allocations
unchanged, idle scratch reused. Private509442(+4838), input64447/headroom489,
28 non-code banks unchanged. New area quanta max319968 cycles; general setup
stage0 reaches12059828, so C07/L4 performance explicitly remains open. Next bound
general AI setup/reach, then reviewed targeting/per-target/empty presentation.
Retail casts/full production gates stay closed; Jump deferred. Standard playtest,
live TiEmu and existing gallery bytes/dates are preserved.

**Historical owned-area checkpoint —2026-10-27:** [CORE_AREA_CT.md](CORE_AREA_CT.md).
Supported formula08 areas now use command/CT/automatic impact and existing response/
reward drain, with one60-byte Work packet.353 PC/sanitizer/native checks11D6FCB4,
5 Orbonne retry checks00000230;13 equal-tick roots, explicit unit/tile centres and
pure late rejection. Retail area producers/AI/forecast/presentation remain gated.
ABI29/ModCtx364/signature63107/largest6204; Work45576(+60), State2192/Unit112/packet22/
FSV3 stable; same5 allocations, scene+Work and totals+60. Private504139(+533), shards
33782/31632, input64927/headroom9;28 non-code banks unchanged. Next area forecast/AI
and reviewed per-target/empty presentation, with a resident budget first. Broader
families and C07/L4 frame/residency remain open. Jump deferred; standard group/
live TiEmu/gallery bytes and dates untouched.

**Historical archive-bank checkpoint —2026-10-27:** [CORE_BANKS.md](CORE_BANKS.md).
FMB3 splits the same13 images into33772/31234 archived bytes; one6204-byte execution
zone remains. All image code is unchanged except ABI28 signature62790/ModCtx364.
Startup validates both ranges/export token before execution; six bad/missing/mixed
pair cases reject.130 PC/sanitizer/native parser checks and112 native full-context
checks pass; fixed callbacks share a const template, dynamic fields still refresh.
Native directory52 replaces6 bytes; State2192/Unit112/Work45516/packet22/FSV3 and five
allocations unchanged. Private503606(-27), input64845/headroom91;28 non-code banks
unchanged. Next durable area-batch ownership/CT/command/AI/impact routing and reviewed
presentation. Resident growth and C07/L4 frame/residency still need budgets; Jump
stays deferred. Standard group/live TiEmu/gallery bytes and dates untouched.

**Historical compound-cast checkpoint —2026-10-27:** [CORE_BATCH.md](CORE_BATCH.md).
Formula08 batches prepare all outcomes before HP, pay once, commit once and reuse
stable response/reward drain.13 original compound groups/1202 PC/sanitizer/68000
checks94558E80; private13-slot carrier, no retail area command/AI/presentation.
Caller packet60 bytes; no State/Work/Unit/FSV3 growth or new batch heap allocation.
ABI27/thirteen images/ModCtx364/largest6204; private503633 (+3027), resident input64783/
headroom153, raw bank64984.28 non-code banks unchanged; future growth needs a bank
budget. Next durable batch ownership, area CT/command/AI/impact routing and reviewed
presentation; remaining families and C07/L4 full-frame/residency gates stay open.
Jump deferred; live TiEmu, standard group and gallery bytes/dates untouched.

**Historical radial-target checkpoint —2026-10-27:** [CORE_TARGETS.md](CORE_TARGETS.md).
Read-only ordinary formula08 target sets pass79 original footprints/42 column
boundaries and1193 PC/sanitizer/68000 checksFC02B091. Nearest-layer/ties, whole-map,
follow coordinates, presence and panel-level team filtering are source-checked.
One-floor maps skip upper reads; no damage/MP/RNG/State changes or live area producer.
ABI26/thirteen images/ModCtx364/largest6172; State2192/Unit112/Work45516/packet22/FSV3
unchanged. Private500606 (+1564), input64749/headroom187;28 non-code banks unchanged.
Next batch preparation/commit in source order, one payment, per-target responses/
rewards/presentation before retail activation. C07/L4 cold-HUD/frame/residency and
broader core/content gates remain open. Jump deferred; playtest group/live TiEmu/
existing gallery bytes and dates untouched.

**Historical formula08 environment checkpoint —2026-10-27:** [CORE_WEATHER.md](CORE_WEATHER.md).
Weather/Float and prepared protected Oil removal now share prediction/AI/
execution.254 original profiles and6493 PC/sanitizer/68000 checks pass. Preserve
PS1 Oil's no-double-damage bug and signed per-step weather narrowing. Retail
spells/climate/status producers remain gated; weather/Float movement are separate.
ABI25/ModCtx364/largest6172; State2192 (+2), Unit112/Work45516/packet22/FSV3 stable.
Private499042 (+279), input64735/headroom201; all28 non-code banks unchanged.
FORMULA11 owns unchanged forecast-cache drawing; full cold forecast costs up to
92032 in the fixture, so full-frame/cold-HUD/residency gates stay open. Next area
target sets and per-target effects/reactions/rewards/presentation. Standard group,
live TiEmu and existing image bytes/dates remain untouched.

**Historical affinity-data checkpoint —2026-10-27:** [CORE_AFFINITY.md](CORE_AFFINITY.md).
Source job/equipment affinities now rebuild a65-byte derived battle cache;
Unit stays112 bytes, State grows66 including alignment, FSV3/Work stay unchanged.
178 original aggregation profiles and1881 PC/sanitizer/68000 checks pass. FCJ9
stores416 references to32 shared masks in582 bytes, rejects FCJ8/bad references
and preserves all existing ability semantics. This is data/cache preparation:
elemental damage, absorption/healing, nullification and retail spells stay gated.
Native stats now execute locally in PARTY/ACTION/RESPONSE; no stat callback
can evict an executing image. ABI22/ModCtx348/largest6060; five allocation requests.
Private495758 (+2709), input64859/headroom77;27 non-code banks unchanged.
Next: typed elemental outcomes through prediction/AI/impact/responses, with
source rounding/capping, then magic evasion/areas/presentation. Full core and
residency gates remain open. Standard group/live TiEmu untouched; Jump deferred.

**Historical magical-scalar checkpoint —2026-10-27:** [CORE_MAGIC.md](CORE_MAGIC.md).
Formula08 now shares source-checked MA/support/Shell/zodiac/Faith arithmetic
between prediction, execution and actual AI. A private normalized Holy record
exercises vulnerable casting, one payment, one impact and Auto-Potion settlement;
production Holy/Fire and all AF_CAST producers remain disabled.104 original-MIPS
profiles and1352 PC/sanitizer/68000 assertions pass. No element affinities,
magic-evasion spells, monster zodiac or new status/trait producers are enabled.
ABI21/ModCtx348/largest5882; battle/save layouts and five allocations unchanged.
Private493049 (+503), input64919/headroom17;28 non-code banks unchanged.
Pure math remains resident; cold banner/cache setup uses PARTY7/8. Next:
element/target eligibility and magic evasion, then areas and reviewed casting
presentation. L2/L3/L4 and residency gates remain open; Jump stays deferred.
Standard playtest group and live TiEmu remain untouched.

**Historical incantation checkpoint —2026-10-27:** [CORE_INCANTATION.md](CORE_INCANTATION.md).
Single-centre casts now retain a fixed tile or stable unit slot through actual
CT, interruption, one MP payment and root/reaction settlement. Exact-cost paid
effects also fix the preceding preview-affordability recheck. Private carriers
exercise the backend; no retail Holy/Fire formula or binding UI is enabled.
ABI20/twelve images/ModCtx344/largest5862; State/Unit/Work/FSV3 unchanged.
Private492546 (+1481), input64859/headroom77;28 non-code banks unchanged.
COMMAND3/4 own validation/arming/due preparation, with arguments outside State.
Next magical formulas/traits, area target sets and presentation. Broader core/
residency gates stay open; standard group/live TiEmu untouched. Jump deferred.

**Historical casting-resource checkpoint —2026-10-27:** [CORE_CASTING.md](CORE_CASTING.md).
Player/forecast/AI share effective MP eligibility; COMMAND pays once at effect
start, after arming returns. Half MP and preparation timing have original-MIPS
fixtures. This enables no new Holy/Fire effect or caster producer. ABI19/twelve
images, ModCtx336/largest5860; Unit/State/Work/FSV3/five allocations unchanged.
Private491065 (+686), input64743/headroom193;28 non-code banks unchanged.
Cold range construction moves unchanged to COMMAND2; pure cost callback cannot
evict its caller. Spell targeting/formulas/AoE and broader scheduler gates remain
open. Standard group/live TiEmu untouched; earlier checkpoints are history.

**Historical deferred-dispatch checkpoint —2026-10-26:** [CORE_DEFERRED.md](CORE_DEFERRED.md).
Due Charges drain iteratively, in source slot order, before expiry/next CT.
Busy roots preserve the packet; later invalidated casters are rechecked. No new
State/Work/save fields, queue or ABI: ABI18/twelve images unchanged. Private490379
(+8), input64857/headroom79. Source unit-versus-tile coordinates are observed;
magic/MP/AoE, Quick255 priority and all-permanent-freeze liveness remain open.
Jump stays deferred; standard group/live TiEmu untouched. Earlier notes are history.

**Historical dual-Charge checkpoint —2026-10-26:** [CORE_CHARGE_HANDS.md](CORE_CHARGE_HANDS.md).
All eight Archer Charge powers now resolve per hand with source criticals,
shared potential forecast/AI and delayed validation of both weapons.586 targeted
PC/sanitizer/68000 checks; eight original initializations and sixty scalar cases.
Pending's spare byte holds the off weapon: State2124/Unit112/Work45516/FSV3 and
five allocations unchanged. ABI18/twelve images, ModCtx332, largest5814.
Private490371 (+1570), packed input64845/headroom91;28 non-code banks unchanged.
Pure math stays resident; cold pickup moves to PARTY6. Broader targeting,
weapon procs/elements, action/status families and combined residency remain
open. Jump stays deferred; standard group/live TiEmu untouched. Earlier dated
checkpoints below are historical; see CORE_ROADMAP.md for current priorities.

**Historical hands checkpoint —2026-10-26:** [CORE_HANDS.md](CORE_HANDS.md).
Ordinary supported Two Swords and dual Counter now use independent hands;
forecast/AI share potential damage and Sleep wake. Management returns an illegal
left weapon to stock.196+16 targeted PC/sanitizer/native checks; FSV3 unchanged.
FCJ8 adds20 innate bytes/reuses Item.pad; ABI17/twelve images split COMMAND and
RESPONSE from impact. Native State2124 (+30), Unit112/Work45516 unchanged.
Private488801 (+5387), input64821/headroom115, zone5750. Further resident growth
still needs a module budget. Broader weapon/skill/element/proc and original AI
families remain open; full L2/L3/content/residency gates stay open. Jump deferred.
Standard group/live TiEmu untouched. Earlier checkpoint numbers are historical.

**Current consecutive-strike checkpoint —2026-10-26:** [CORE_STRIKES.md](CORE_STRIKES.md).
The bounded ordinary-Attack engine entry resolves two primary impacts before
one response drain and best-EXP/one-JP settlement;250 PC/sanitizer/native checks.
Normal commands still request one: independent hands, preview/AI and the actual
Two Swords producer remain open. ABI16/ten images, CoreAction34/State2094 (+2),
Unit112/Work45516/FSV3 unchanged. Private483414 (+836), input64923/headroom13,
zone5830; data/pictures unchanged. Relocate further resident growth first.
Next: hand-specific policies and more reaction families; full L2/L3/content and
residency gates remain open. Jump deferred; standard group/live TiEmu untouched.
Earlier checkpoint numbers below are historical.

**Current reaction checkpoint —2026-10-26:** [CORE_REACTIONS.md](CORE_REACTIONS.md).
Counter/Auto-Potion now queue, revalidate, execute and restore the primary packet.
Equipped party IDs load from FSV3. Shared stock, secondary suppression, deferred
root rewards and terminal ordering are covered by267 PC/sanitizer/native checks.
ABI15/ten images, State2092 (+46), Unit112/Work45516/FSV3 and allocation count
unchanged. Private482578 (+4008), input64605/headroom251, zone5742; data unchanged.
Next: source strike sequencing and more reaction families; general L2/L3/content
and timing/residency gates remain open. Jump deferred; live TiEmu untouched.
Earlier checkpoint numbers below are historical.

**Current action-context checkpoint —2026-10-26:** [CORE_CONTEXT.md](CORE_CONTEXT.md).
Live roots protect costs/impact/settlement; a hit or miss commits once, and a new
command cannot reset an active action.357 PC/sanitizer/68000 checks cover source
reaction guards and bounded per-actor response primitives. Reaction effects,
packet restoration, producers and multistrike remain next; full L2 stays open.
ABI14/nine images, State2046 (+32), unchanged Unit112/Work45516/FSV3 and allocation
count. Private478570 (+434), input64727/headroom209, zone5742. FCJ7/data unchanged.
Jump deferred; standard group/live TiEmu untouched. Older notes are historical.

**Current Sleep checkpoint —2026-10-26:** [CORE_SLEEP.md](CORE_SLEEP.md).
Sleep now shares source60-tick CT freeze, Charge cancellation, physical XA
vulnerability before zodiac and protected wake at successful HP-damage impact.
1070 PC/sanitizer/68000 assertions pass. No new producer/art or allocation;
Unit112/State2014/Work45516/FSV3 unchanged. ABI13/nine images, private478136
(+112), input64837/headroom99, zone5742. FCJ7/data banks unchanged.
Next: explicit action/reaction contexts; broader effects, timing/residency and
content-production gates remain open. Jump deferred. Earlier notes are history.

**Current control checkpoint —2026-10-26:** [CORE_CONTROL.md](CORE_CONTROL.md).
Stop pauses CT/Charge; Don't Move/Don't Act share player/preview/AI restrictions,
source CT credit and due-time Charge validation.594 targeted PC/sanitizer/native
checks pass. Cached flags reuse existing fields; ABI12/nine images, unchanged
Unit/State/Work/FSV3. Cold AT/glyph setup moved to existing images, with a safe
scene-to-battle return boundary. Private478024 (+1340), input64847/headroom89;
FCJ7/data banks unchanged. Next: Sleep damage/wake and action/reaction contexts.
Full L2/content gates remain open; Jump deferred. Older notes are historical.

**Current Poison/Regen lifetime checkpoint —2026-10-26:** [CORE_STATUS_LIFETIME.md](CORE_STATUS_LIFETIME.md).
Source counters, application/renewal/removal, clock expiry and end-turn HP upkeep
now pass4140 PC/sanitizer/native assertions with13 actors. KO cancels mutable
timed/command effects; menus pause CT. FCJ7 remains unchanged; ABI11/nine images,
Unit112/State2014 (+210 RAM bytes), unchanged Work/FSV3 and allocation count.
Private group476684 (+1168), packed input64789/headroom147. Next: incapacitation,
cast interruption and action/reaction contexts. Full L2/content gates stay open;
Jump deferred. Earlier checkpoints below are historical.

**Current status-state checkpoint —2026-10-26:** [CORE_STATUS_STATE.md](CORE_STATUS_STATE.md).
Canonical forty-bit storage/filtering, immunity and innate-protected cures now
share source policy; Charge/Defend command state is consistent. Gameplay effects
remain a restricted subset; expiry/upkeep and reaction contexts are next.
FCJ7/ABI10, Unit96/State1804 (+218 RAM bytes), Work45516/FSV3 unchanged.
Private group475516 (+1877), packed input64673/headroom263.4842 targeted
PC/sanitizer/native checks pass. Earlier checkpoints below are historical;
Jump stays deferred. Standard bundle and live TiEmu are unchanged.

**Darkness/Silence checkpoint —2026-10-26:** [CORE_STATUS_RULES.md](CORE_STATUS_RULES.md).
Physical preview and commit now apply attacker Darkness and target Defend through
one rule. Player/preview/AI share the source Silence flag/menu restriction.
FCJ6 adds224 archived menu bytes and strict length/version rejection; no new
battle/save fields or heap allocation.433 native status checks plus existing
primary actions/Charge pass. Private group473639 (+1174), ABI9/nine images,
packed input64575/headroom361. Full status policy, poison upkeep and reaction
contexts remain open; Jump deferred. Standard bundle/live TiEmu stay unchanged.

**Shared primary actions checkpoint —2026-10-25:** [CORE_ACTIONS.md](CORE_ACTIONS.md).
Impact commit is separated from animation; combat, AI and healing forecasts share
pure HP/status transitions. Existing primary effects and six action pictures
remain identical before/after on PC/68000. ABI8/nine images; no State/Work/Save or
heap growth. Private group472465 bytes (+464); resident packed input64177,
headroom759. Impact relocation costs extra CPU (scoped peak33480 cycles).
Next: source-backed status policy/action contexts. Full L2/L1 and production
gates remain open; Jump refinements deferred; standard group/live TiEmu untouched.

**Common compact topology closed —2026-10-25:** [CORE_COMPACT.md](CORE_COMPACT.md).
ABI7/nine images use36-byte lower-only or68-byte two-floor row storage, with
four cached neighbors per node. Native Gariland heap requests fall1798 bytes;
Orbonne1678. Actual ordinary search CPU is preserved; wider searches are about
1.6% cheaper in the scoped two-map comparison. Generic/Work search now represents
cost6 and32-link rows; live gameplay still emits one-gap links. Work45516 (+2),
State1586 and FSV3 unchanged. Private group472001 bytes (+119),28 identical data
banks, accepted Status/range views preserved; packed input64825/headroom111.
**User priority: Jump is minor. Finish this common-storage verification, then
advance L2 actions/statuses; defer Jump refinements.** Full L1 and production
readiness remain open; standard group/live TiEmu are unchanged.

**Current consolidated roadmap:** [CORE_ROADMAP.md](CORE_ROADMAP.md). L1 is
advanced but incomplete; L2/L3 still require major rules work, and L4 production
gates remain open. Dated checkpoint notes below retain their historical scope.

**Resident capacity / actor-column checkpoint —2026-10-25:** [CORE_RESIDENT.md](CORE_RESIDENT.md).
SAVE is split from encounter/party initialization (new PARTY image); unchanged
Status keys now run in SCREENS. ABI6/nine FMB2 images reduce the reserved zone
6074→5734. Wider-cost search is now linked by default, with unchanged Work/State/
Save layouts and fast ordinary movement. Private group471882 bytes (+1438),28
unchanged data hashes. Packed input64831 bytes leaves105 below TTPack's limit;
new build reports expose this separately from compressed size. Targeted flow,
save, Status, search and accepted-view checks pass.88 original actor-column
queries also feed a99-check PC/sanitized/68000 prototype. Runtime still has
one-gap links: bounded longer links, actor-record production and wet costs
remain next. Standard group and live TiEmu are unchanged.

**Latest test build —2026-10-23:** Gariland now offers1px/2px scenery edges in
MODE (CATALOG remains available): select Edges and press ENTER/2nd. Default1px,
both retained for actual user testing. Orbonne has no2px candidate. Perspective
is explicitly outside the game under experiments. See [EDGE_WIDTH.md](EDGE_WIDTH.md).

**Title A/B and QA follow-up —2026-10-23:** [QA_TITLE_FOLLOWUP.md](QA_TITLE_FOLLOWUP.md) records the normal-start memory failure reproduced on private Titanium and the corrected packed path reaching Agrias, plus native Continue/Data2. Academy pan clamping, shortcut99 joining the full Gariland/map route, and the first-frame premature-victory fix are implemented. Serif title/result reveals and1px/2px scenery are marked review candidates. A/B remain isolated; campaign perspective is explicitly still absent and B06/B07 remain open.29 campaign +18 opening/chapel native replays,18 current/A and29 B title cases and split native flow checks pass.

**Package memory check —2026-10-23:** [PACKAGE_CHECK.md](PACKAGE_CHECK.md) records an isolated Titanium run. Plain archived map/management retained36550bytes under the supervisor; packed stubs reduce that pair to2020bytes. Deployment → battle → map/management twice → battle passes, with repeated returns stable at186980bytes free and the second battle visibly playable. The pre-title candidate group is452879bytes. Title-enabled full packaging/presentation and B06/B07 remain open; no live session or real save was changed.

**Title/three-save candidate —2026-10-23:** see [TITLE.md](TITLE.md). Source PS1 title art, New Game/Continue, shared animated hand and three isolated slots are implemented. Legacy saves are preserved/imported explicitly; overwrite defaults to Return.18 title PC/68000 cases and the88-assertion battle flow on slot2 pass.29 campaign +18 opening/scene replays retain all139 reference PNGs. Requested work order is2(package),1(QA/title),4(core mechanisms). Review images are under `x/qa/title/review-1`; final title packaging/presentation awaits the visual verdict.

Shared portrait/stat presentation: [UNIT_CARD.md](UNIT_CARD.md). Formation,
deployment, management headers, battle HUD and battle Status now use configurable
layouts of one renderer, retaining the existing caches and pictures. The new
Status callback changes the module-context ABI: rebuild core and `ffbm` together.
The latest placement review adds the flat5x5 grid, bottom-left orientation arrow
and complete slab rims; its marked comparison is under `x/qa/flow/review-4/new`.

Current flow candidates: [FLOW.md](FLOW.md). The actual runtime now includes
dark Academy narration/chapter/location titles, playable Gariland deployment,
pre/post battle dialogue, three source-derived result pages with item icons and
aftermath staging. Orbonne has no result pages. Four marked gallery sheets await
visual review; complete packaging/hardware checks are pending. FSV3 adds funds
and selected placements while retaining old FSV2 parties. The bundle sizes below
are historical and do not describe this candidate. `make pc` plus `python3 play.py`
uses the complete candidate path and a private `x/play-data` save; `--new` opens the New Game file chooser with overwrite confirmation. No live TiEmu session is modified by these commands.

Latest working-tree review: [playtest QA and design proposals](QA.md). Current
banks use FCJ5 (complete item names); candidate battle/camera/scenery fixes pass
headless native checks. The group below still contains FCJ4 and has not been
replaced in the user's live TiEmu. Do not mix new individual banks into it.

Previous review: [all eight Charges and long action lists](CHARGE.md),
FCJ4 banks. Five-row scrolling is approved; its requested above/below indicators
await review. Remaining core work is listed in [CORE_STATUS.md](CORE_STATUS.md).
At the user's request,
`x/fftcampaign.89g` is now a matching FCJ4 review package (33 variables,
430,593 bytes) for manual TiEmu testing; the FCJ3 measurements below describe
the preceding validated checkpoint. Keep programs and banks together.

Current core checkpoint: [shared abilities and modular evaluation](ABILITIES.md),
**FCJ3 / FCD2 / FMG2**. Battle and management share source ability IDs and learned
positions; existing encounters use the formula module. Further formulas remain
unavailable until checked. The group has 33 variables, **430,437 bytes**; update
programs and banks together. The uncompressed battle is **63,427 bytes**, with
1,509 bytes below TTPack's input limit. The KO/party and Ramza 51/52 HP remarks
are checked in ABILITIES.md, alongside 45 native replays and frame equivalence.
An isolated Titanium AMS 3.10 boot of the archived group also reaches playable
Gariland through `fftstory(99)`; details and limits are in ABILITIES.md.

Local-disc campaign slice, sharing the existing tactical engine. The sequence is
Orbonne's chapel and encounter, battle, abduction, Chapter 1 Academy dialogue,
then the existing playable Gariland battle. The flashback correctly starts a
new level-one party rather than carrying Orbonne's level-nine stats forward.
Gariland's results end this slice; they do not restart Orbonne.

The Academy is a fixed native staging with the source MAP024 background, source
actors and all 15 observed messages. It is not a recreation of the world map,
deployment screen or every PS1 camera/actor movement. Orbonne's final punch,
collapse and Chocobo escape are sprite adaptations of movie choreography.
See [opening corrections](../opening/BUGFIXES.md).

## Build

With the local US disc, first generate/observe the opening as described in
`../opening/README.md`, including the added scene oracle:

```sh
tools/pyenv/bin/python games/fft/tools/opening_scene_oracle.py \
  'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'
make -C games/fft/opening assets check
make -C games/fft/campaign assets
make -C games/fft/campaign check bundle
```

Serialize GCC4TI builds. The bundle is `x/fftcampaign.89g`: 33 variables, 430,437 bytes:
the small `fftstory` supervisor, two packed launchers (`fftbat`, the battle program of every
chapter, and `fftscn`, the scene player), their archived cores (`ffbcore`, `ffscore`), the
management menus (`fftmng`, 18,627 bytes, and their bank `ffmg`), the world map (`fftmap`,
13,031 bytes, and its bank `ffwm`), the battle modules (`ffbm`)
and the chapter banks. All data
and packed cores have the archive attribute; do not move all of them into RAM.
Retail art, text, binary data and captures remain generated/ignored.

On the calculator, send that group and run **`fftstory()`**. ENTER/2nd advances
dialogue; SHIFT skips the current Orbonne section; CATALOG inverts its dialogue
window. During battle, CATALOG selects **Move: full tiles / Move: contour**
(default full); ENTER toggles and ESC/CATALOG closes settings. Ordinary Gariland controls apply after the Academy. ESC cancels a chapter
without automatically starting the next. `fftbat(n)` alone plays one chapter (100 + an
Orbonne door: `fftbat(100)` its gate; 200 + a Gariland door: `fftbat(200)` its battle) and
returns to HOME; use `fftstory()` for the automatic continuation.

**Campaign save** (`save.h`, the variable `ffsave`, archived): `fftbat` writes it when it
leaves after a victory: the chapter's story flag, and after Gariland the new party (the
units still with it, including ordinary KO before crystal/chest conversion, FFT's job/item/ability ids, their jobs' JP and learned abilities, their
ability slots), the bag and the chests' items. `fftstory()` resumes after the last victory
saved (after Orbonne: the chapter card; after Gariland: the world map); `fftstory(1)`
deletes the save and starts a new game. `python3 games/fft/tools/save_dump.py ffsave.sav`
decodes one.

**World map candidate** (`fftmap()`, `worldmap.c`, after Gariland): the selected source
texture and lettering, circular locations, outlined source roads and animated Ramza.
Hold arrows to move the hand freely; release near a location to snap, then ENTER to
travel/enter. Hover alone never starts a journey. ALPHA/DIAMOND opens the main menu:
Formation opens management; Data saves; the remaining unfinished entries are gray.
ESC returns from the menu, or exits the map. The user selected67%(2/3), including
reduced existing Ramza poses, with the same source texture and stronger roads.
FWM3 `ffwm` is18,066bytes, with lossless8-row ZX0 strips and five unique Ramza
poses reused for eight idle steps. The decoded cache/sprites use11,532bytes; no
whole-map or255-tile approximation. Roads/markers are cached in those same strips.
The integrated bank and Gariland picture exactly match the approved67% preview;
696 PC/68000 storage/decor checks and7 map replays pass. Other pending UI verdicts
and the complete campaign package remain separate; the live group is unchanged.

**Management candidate** (`fftmng()`, `manage.c`): five columns/two rows, neutral
poses, selected portrait/details, floor diamond, HP/MP bars and compact title.
ENTER: Status; ALPHA/DIAMOND: Equip, Abilities, Learn, Change Job. The job wheel uses
left/right (up/down aliases), a short rotation and explicit confirmation. Jobs
unlocked by another roster member are gray; unknown jobs are hidden, with sex
restrictions preserved. F1/F5 changes units, SHIFT goes back, ESC saves/exits.
The clarified Change Job screen has no diamond or gray floor bars; Formation uses a thicker floor
diamond and no gray floor lines. Its refined picture still needs review. `ffmg` remains FMG2,41,231bytes; its41 sprites use neutralSHP11. Ability
names/lists/costs still come from FCJ5 `ffcj`. All other rule families remain listed
in CORE_STATUS.md; showing a learned passive does not implement its battle effect.

On the PC:

```sh
make -C games/fft/campaign pc
tools/pyenv/bin/python games/fft/campaign/play.py
```

The supervisor starts a new process per chapter. Native `NG_execute` similarly
returns from one packed program before opening another: their frame buffers and
archive pointers never have to coexist. Jobs, items, templates, abilities and
skill sets are read from archive; birthday and JP fields use explicit big-endian
reads on both targets. Non-shipping research injection scenarios are omitted
from the Gariland executable to respect TTPack's 64,936-byte input limit; the
battle, direct Ramza (61) and full-AI (16) doors remain.

## Memory and validation

An isolated Titanium TiEmu test initially reached the Academy but Gariland
reported `not enough memory`. PC/68000 framebuffer checks alone did not catch
this AMS heap constraint. Sharing the full Status screen with the inactive
tactical portrait buffers saves **7,680 bytes**. Leaving Status invalidates both
portrait caches, so the old Status text cannot appear over combat. No menu,
ability or combat action was removed.

After this correction, a fresh archived-group boot on **Titanium AMS 3.10**
passes the actual packed-program sequence: prologue return -> Academy return ->
Gariland, ending at Ramza's usable menu. The bounded `fftstory(99)` injection
uses the final prologue screen, final Academy message and direct Ramza door;
it verifies the AMS/heap/program lifetime, not a complete manual playthrough.
Capture: `x/tiemu-check/handoff-final.gif`, final LCD `handoff-final.png`.
That capture validates the prior 357,955-byte checkpoint; the new reach-cache
package is headless-checked, with no new TiEmu heap-lifetime claim.
The user's live emulator and keyboard were never touched. No original HW2 or
physical-calculator validation is claimed for this group.

Independent checks:

- Opening: 16 exact PC/actual-68000 pixel/state checkpoints, CT/tile-lock and
  interruption regressions, complete AI victory and non-repeating conclusion.
- All 11 Orbonne actors animate; 648 walking poses match their requested bank
  frames, cold portraits take 5–12 PC updates, and four water phases are distinct.
- Academy and Gariland: ten PC/68000 replays including Status, Move settings
  and the contour actually on screen; `x/check/checked.json` and `x/check/status.json`.
- Academy dialogue: all words from 15 source messages survive across 17 fitted
  pages. Unnamed shouts remain body text, and speaker whitespace is trimmed.
  The final text-only bank correction also passes both native Academy replays
  (`x/check/academy.json`); the TiEmu capture predates that correction.
- `test_status.c` checks all three restored portrait planes after using the
  shared storage. Existing Gariland and Auto-battle tests still pass.

These are correctness checks and datasheet CPU measurements, not PC-derived
hardware FPS. The camera handedness fix is enabled in this campaign and Orbonne;
historical standalone research controls retain their old projection/banks.

## Chapter storage

Data the two chapters share is stored once in the campaign group:

- **Interface, `ffcu`** (`tools/common_ui.py`, run by both chapter generators): character
  advances, font, slope markers and banners (7,916 bytes) were byte-identical in `ffoi`
  and `ffgi`. Both banks drop them; `fft.c` reads them through `cgfx` (`UI_SHARED`), and
  every later `igfx` offset (tables, Orbonne's name directory) is shifted.
  `campaign/data/interface.h` is the stripped copy of the root header.
- **Actors, `ffca`/`ffcb`** (`tools/common_actors.py`): every distinct 208-byte actor
  frame of both chapters (characters, armed bodies, crystals, chests) is one ZX0 stream,
  numbered across the two variables (64,893 + 19,550 bytes, 597 frames, 71 used by both
  chapters). A frame that mirrors another is stored once: bit 15 of a chapter's logical
  frame entry (`opening/data/ffoa.bin`, `campaign/data/ffga.bin`) flips it after decoding
  (ExtGraph `SpriteX8_MIRROR_H_R` on the TI). Blades alone are `ffcw` (3,024 bytes).
  Orbonne's actors and Gariland's raw `fftu`/`fftw` (125.6 KB) become 89.7 KB.
- **Decoded frames:** one LRU cache of 48 frames (`AC_N`, 10.2 KB, its own block) shared by
  every actor, at most one ordinary decode a frame; each decode is charged to the frame
  budget. Gariland's AI battle wins at frame ~9,073 (raw frames: 8,882; 32 cached frames:
  10,445; one frame per actor: 12,615). `FFT_RAW_ACTORS` builds the raw-bank control.
- **Scenery views** (`ffov0..3`, `ffgv0..3`) store each occlusion-mask row XORed with the
  row above before ZX0 (`tools/extract.py:rowxor_masks`). `fft.c:unxor_masks` undoes it after
  each decode, one long at a time: Orbonne −3,031 bytes, Gariland −2,940. A view decode
  (load or turn) costs 78–114k more datasheet cycles (6.5–9.5 ms at 12 MHz). Colour and
  depth planes stay plain: row XOR would save only ~1 KB / ~0.3 KB per map for the same CPU.
- **Gariland's views drop their 16 top rows** (`SC_TOP`, empty in all four orientations;
  `tools/campaign_assets.py` checks them): 202 scene rows, 5,120 bytes less RAM for the view
  and scene buffers. The camera no longer climbs into that white band (it stopped once 5
  pixels lower in the AI battle; user-validated). Orbonne's views were already cropped.

RAM (TiEmu Titanium, every bank archived, `fftgar(61)` launched alone): with the crop,
Gariland starts with 48 and with 56 cached frames; before it, 36 or more ran out of memory.
All 16 opening and 10 campaign PC/TI checks pass; Gariland's keep the baseline's screen
checksums and state. Eight Orbonne screens change (poses no longer lag a frame behind),
awaiting visual review in `x/actors/opening/avant-apres-orbonne.png`. Orbonne's packed program (65,189 bytes) is still over TTPack's 64,936-byte
input limit: the next step merges the chapters into one shared battle engine and scene player
with chapter data files.

Shared command-window presentation and deployment confirmation: [CHOICE_MENU.md](CHOICE_MENU.md).
