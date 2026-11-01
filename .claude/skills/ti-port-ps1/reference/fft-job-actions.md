# FFT job actions: requested rules and implementation boundary

These are the user's FFT PS1 design requirements, preserved for future job work.
The complete table below is not a claim that all twenty jobs have been verified
against the original or implemented. Before implementing another ability, check
its source timing, targeting and interruption behavior; record any discrepancy
with this requested model in `games/fft/RE_NOTES.md`.

## Delivery rule and current coverage (2026-10-30)

CORE_GUTS.md adds Ramza's Yell, Cheer Up and Scream (always hit, Speed/Brave/
PA/MA gains): 120 normalized records, 87 casts; generic entries stay 114/201.
CORE_PUNCH_ART.md's Spin Fist (area physical hits, no knockback) came before it.

CORE_TALK_SKILL.md adds Persuade, Praise, Threaten, Preach, Solution, Insult
and Mimic Daravon: 115 normalized records, 82 casts, 112/201 generic entries.
Invitation, Death Sentence and Negotiate stay gated.

CORE_PUNCH_ART.md adds the Monk's Stigma Magic, Chakra, Revive, Wave Fist and
Repeating Fist: 108 normalized records, 75 casts, 105/201 generic entries.
Earth Slash (line) and Secret Fist stay gated (Spin Fist followed). CORE_PROTECT.md before it
adds Reraise and Golem (103 records, 72 casts, 100/201).

CORE_CONTROL_STATUS.md adds Frog, Blind Rage, Confusion Song and Foxbird
together: 101 normalized records, 70 casts, 98/201 generic entries. Lost
control (Berserk/Confusion/Chicken) is a cached status bit read by turn_start;
the AI policies are adapted, not an original AI replay.

CORE_TERMINAL.md then adds Death, Quick, Zombie and Petrify together:
97 normalized records, 66 casts, 94/201 generic entries (107 remaining entries
are not 107 effects). Quick is an explicit CT request, not a status bit; Death
retains nominal damage separately from its Dead infliction. Resident packed
input headroom is 7 bytes, so relocate cold code before adding resident code.

The user requires family batches: implement a shared rule, enable compatible
producers together, then run scoped integration. CORE_RESTORATION.md adds five
items together and shares all seven resource producers. All 14 ordinary Item
records are enabled; this is not a complete Chemist R/S/M certificate.
Production: 93 normalized executable records, 62 casts (47 spells/15 summons),
90/201 generic action-table entries. The 111 remaining entries include upgrades
and categories, not 111 distinct effects; R/S/M are separate. Explicit paired
HP/MP tags alias existing scalar cost bytes, with frozen response restoration.
Original Undead Elixir damages HP while retaining MP healing; do not infer the
numeric channels from attack_type flags alone. Action start invalidates scalar
forecasts; no-effect resource items pay stock without earning EXP/JP.
ABI 43/376, 17 images; gameplay/save layouts and core heap requests stay fixed.
SCREENS owns unchanged cold menu/view input, returning before loading code;
AI pause clock remains resident. The 2481 PC/sanitizer/native assertions and
80 original profiles/38 store traces are scoped evidence, not a full source
inventory/reward/AI simulation. Mutable Petrify cures exist; infliction/control/
stat handlers, remaining job/RSM/campaign/format/tool/full-budget gates stay open.
Keep Jump deferred; preserve live TiEmu/standard playtest/saves and old proof
bytes AND timestamps. Announce full-core completion only after production gates.

## Four different execution mechanisms

- Immediate: apply the effect during the action, without a deferred cast.
- Vulnerable preparation: remain on the map and targetable until resolution.
- Protected Jump: leave the battlefield while airborne and become untargetable;
  strike the stored tile on landing, missing if it is empty.
- Performance: remain in a persistent state with periodic effects on eligible
  targets; do not implement this as a single delayed attack.

Share the CT clock where appropriate, while keeping distinct `CHARGING`,
`AIRBORNE` and `PERFORMING` states. CT ticks are not completed character turns.
Menus and dialogue must not advance combat time. Define interruption and action
cost per mechanism rather than inheriting Archer's rules for every job.

## Target identity is independent of execution timing

| Requested action | Target after movement | Caster during delay |
|---|---|---|
| Archer Charge | Stored tile; can miss if it becomes empty | Present and vulnerable |
| Lancer Jump | Stored tile; can miss if it becomes empty | Absent and untargetable |
| Mage, unit selected | Follow that unit | Present and vulnerable |
| Mage, tile selected | Keep that tile | Present and vulnerable |
| Samurai Draw Out | Immediate effect | No delayed protection state |
| Bard/Dancer | Reevaluate eligible targets at each pulse | Persistent performance |

Do not let a normal unit query, hit test, AI target list or line-of-sight target
selection include an airborne unit. Protected Jump and ordinary casting are
separate mechanics even if both resolve through the same scheduler.

## Twenty-job requirements supplied by the user

| Job | Requested examples | Execution category |
|---|---|---|
| Squire | Accumulate, Throw Stone; Monster Skill | Immediate; passive support |
| Knight | Weapon Break, Speed Break, Armor Break | Immediate |
| Archer | Charge +1 through +20 | Vulnerable preparation, fixed tile |
| Monk | Chakra, Earth Slash, Revive | Immediate |
| Thief | Steal Weapon, Steal Heart, Steal EXP | Immediate |
| Geomancer | Geomancy and terrain effects; Attack Up | Immediate; passive support |
| Lancer | Jump; Horizontal/Vertical Jump upgrades | Protected Jump, fixed tile |
| Samurai | Offensive, healing and buff Draw Out | Immediate |
| Ninja | Two Swords; Throw; Vanish | Passive; immediate; reaction |
| Dancer | Wiznaibus, Slow Dance, Nameless Dance | Periodic performance |
| Chemist | Auto-Potion; Throw Item; Phoenix Down | Reaction; immediate; immediate |
| Priest | Raise, Holy, Protect, Shell | Vulnerable casting |
| Wizard | Fire, Ice, Bolt, area spells, Frog | Vulnerable casting |
| Time Mage | Haste, Slow, Stop, Quick | Vulnerable casting |
| Oracle | Petrify, Life Drain, Foxbird | Vulnerable casting |
| Mediator | Invite, Praise, Preach, Threaten | Immediate |
| Summoner | Offensive summons, Golem, Lich | Vulnerable casting |
| Bard | Battle Song, Cheer Song, Angel Song | Periodic performance |
| Calculator | Math Skill and filters | Immediate, without casting delay |
| Mime | Imitation of compatible actions | Automatic after the copied action |

## Current implementation and evidence

`games/fft/opening/` exposes Archer Charge +1/+2. The US executable's table gives
4/5 CT ticks for those two actions. It stores a tile, leaves the archer targetable,
permits another unit's turn before impact, and cancels on death, displacement or
weapon change. PC regressions and actual-68000 state/pixel replays cover this
implementation; see `games/fft/opening/BUGFIXES.md` and `regressions.c`.

`DEFER_AIRBORNE` and `DEFER_PERFORMING` are reserved identifiers, not working
Lancer/Bard/Dancer implementations. Retail magic now uses the production
families listed above; the historical lifecycle study below predates activation. `campaign/CORE_INCANTATION.md` now tests
the vulnerable tile/unit-bound lifecycle with private carriers: arming preserves
movement, MP pays at due execution, ordinary caster displacement/weapon changes
survive, and Sleep/KO versus Stop use distinct cancellation/freeze policy. This
is not an enabled Holy/Fire formula, area or player binding-choice interface.
Existing Squire/Chemist/basic combat support is not proof of the complete table.


### Source traits before magic enablement (2026-10-27)

CORE_MAGIC.md adds source formula08 scalar/prediction/AI with private Holy;
production Holy/Fire remain disabled. CORE_AFFINITY.md supplies FCJ9/FEA1
job/equipment masks and a derived13x5 battle cache, preserving Unit112 and
FSV3. Native stat producers are local to PARTY/ACTION/RESPONSE with no evicting
callback; ABI22. This cache is not elemental damage or absorption/healing.
Next is a typed result shared by forecast/AI/impact/response packets, including
source narrowing/caps and paid nullified effects. Monster equipment/type,
magic evasion, areas and reviewed casting presentation remain open. Keep
Jump low priority; do not infer full job support from cached trait metadata.


### Typed elemental outcomes (2026-10-27)

CORE_ELEMENTS.md connects cached traits to shared formula08/forecast/native AI/
impact.1525 checks and77 original typed/capped/signed-HP-prefix profiles pass.
Keep the prepared category through response packets; source signed quantities
may reverse actual HP direction without changing event type. Positive999 caps
apply only after Half/Weak/Faith. Null wins over absorb and has no effect RNG/
reward; normal absorbed healing has no damage-type wake/Auto-Potion. ABI23;
State/ActionPacket padding holds the category without heap/layout growth.
FORMULA10 owns cold formatting from resident callers, with no evicting pure
math callback. Retail Holy/Fire remain disabled until magic evasion/target
eligibility/area/presentation gates pass. Weather/Oil/Float/mount/Undead/monster
policy and broader job families remain open; Jump stays low priority.


## Current magical-evasion boundary (2026-10-27)

`games/fft/campaign/CORE_MAGIC_EVASION.md` adds ordinary formula08 magical
percentages, sequential source evade draws and late target finalization;7510
PC/sanitizer/68000 assertions use238 original profiles. Preview/AI are pure;
execution includes zero channels. Never substitute physical facing/class evade.
Dead/Petrify/Wall are present but finalized as misses after formula/proc draws;
Crystal/Treasure/Jump are absent. Nullification follows evasion, so Fire null can
still draw four times. FCJA packs MEV/shield tag into existing Item.pad, preserving
HAND_DUAL. ABI24/ModCtx360, unchanged battle/save structs; source maxMEV50.
No retail spell/status/monster/mount producer is newly enabled. Weather/Oil/Float,
areas, per-target reactions/rewards and presentation remain next. Keep Jump low
priority, tests dependency-scoped, live TiEmu/standard group/art dates untouched.


## Formula08 environment boundary (2026-10-27)

CORE_WEATHER.md adds254 source profiles/6493 PC/sanitizer/native checks: signed
weather before resistance/Faith/caps, PS1 Oil's no-double-damage ordering bug with
protected mutable removal, and Float/earth typed null. Packet flags must be masked
before category classification and survive actual Auto-Potion transport. Due
casts read live weather; later queries/climate changes cannot alter preparation.
ABI25/ModCtx364, native State2192(+2), stable Unit/Work/packet/save layouts.
Retail spells, climate/GNS/calendar/bow-weather and Oil/Float infliction/movement
remain gated; no full original status/reward/area trace or cold-HUD/frame/residency
certificate. Next areas/per-target effects/reactions/rewards/presentation; Jump
stays low priority. Preserve playtest group/live TiEmu and gallery dates.


## Ordinary radial target-set boundary (2026-10-27)

CORE_TARGETS.md / core-targets-check adds79 original-MIPS footprint profiles,
42 column boundaries and1193 PC/sanitizer/native checksFC02B091. Raw flag offsets
are3/4/5/6, not4/5/6/7. Finite areas pick nearest raw layer per column, ties lower;
highest-only precedes doubled tolerance, u8 reference and signed16 candidate
height/-255 sentinel. Whole-map255 bypasses vertical. Hidden/no-cursor upper
surfaces can suppress lower cells; use SURFACE_MOVE_UPPER to skip reads only on
truly one-floor maps. KO/Petrify/Wall stay in target lists; Crystal/Treasure/Jump
are absent but a followed centre still reads their coordinates. Team filtering
marks panels: rejected overlap occupants do not clear an earlier selected mark.
Current team scope is uncharmed blue/Enemy; Charm needs initial-team evidence.

Source ordinary list order is ascending slots, with all formulas prepared before
HP commits. Native Unit.slot is a save slot; the returned native mask is unordered.
Do not loop skill() for AoE: it repays MP and changes preparation inputs. Batch
mapping/payment/effects/reactions/rewards/presentation and real command/AI use
remain next; retail AF_CAST stays disabled. ABI26/13 images/ModCtx364/largest6172;
State2192/Unit112/Work45516/packet22/FSV3 unchanged. Private500606(+1564), input64749/
headroom187, raw bank62070. No new heap, art or authored assembly. Keep C07/L4
cold-HUD/frame/residency open, Jump low priority and playtest/gallery untouched.


## Compound formula08 preparation and impact (2026-10-27)

**OBSERVED:**Original ordinary strike prepares all ascending-slot target formulas
before HP. batch_oracle.py drives presence/formula08/finalizer on13 compound groups,
including13 present actors, with a supplied deterministic RNG/normal weather and
no intervening HP commits. It records copied per-group draw lists, not references
to a subsequently cleared list. This reconstructs observed order; it does not
execute the full original cost/HP/reaction/reward/presentation dispatcher.

**TARGET, implemented:**CORE_BATCH.md:60-byte caller-owned CastBatch, shared
magic_prepare/magic_commit, one MP debit, all outcomes frozen before HP, once-only
commit, self-KO cannot skip later victims, late Oil protection recheck, empty/null
sets and existing two-Auto-Potion/deferred maximum-reward drain.1202 PC/sanitizer/
68000 checks94558E80. ABI27/ModCtx364/signature62533; TARGETS1 preparation/RESPONSE10
commit avoid nested image loads. State2192/Unit112/Work45516/FSV3 unchanged; largest
6204. Private503633(+3027), raw bank64984, input64783/headroom153;28 non-code banks
unchanged. New batch entry remains unused by retail commands/CT/AI/attack_step;
retain packet until explicit commit, then existing State owns response drain.
Do not use the old single-target impact path in between. Next durable owner,
source-order encounter mapping, live area CT/command/AI/impact routing and reviewed
per-target presentation; full original magical response/reward families remain
open. C07/L4 full-frame/cold-HUD/residency stays open; Jump deferred. Playtest group,
live TiEmu and existing gallery bytes/dates stay untouched.


## Owned area command, CT and impact (2026-10-27)

**OBSERVED, reused:**CORE_INCANTATION original initializer pays before target
setup, including empty/KO/absent centres. CORE_TARGETS original green/list kernels
retain followed Crystal/Jump coordinates while excluding them as victims.
CORE_BATCH prepares every target before HP; prior CT/cost/binding fixtures remain
the timing/order evidence. This milestone adds native composition, not a full
original spell/campaign/HP/reaction/reward/presentation dispatcher trace.

**TARGET, implemented:**CORE_AREA_CT.md routes supported ordinary formula08 areas
through cast_order, real CT and attack_step ACT_HIT. One60-byte Work owner survives
wind-up/queries/image loads/responses; COMMAND returns3 before TARGETS2 preparation,
RESPONSE11 owns impact, then existing response/reward/terminal drain completes.
No nested image loads, new heap allocation or retail spell activation.353 PC/
sanitizer/68000 checks11D6FCB4 and5 Orbonne retries00000230 pass. State-only scenario
reset left a stale READY packet; both battle_units/fixed_units now clear Work
phase. Missing raw geometry is invalid, not a paid empty spell; vanished centres
follow existing payment-before-target-setup policy. Work45576(+60), State2192/
Unit112/packet22/FSV3 stable,5 allocations/totals+60. ABI29/ModCtx364/signature63107/
zone6204, private504139(+533), shards33782/31632, input64927/headroom9. Next area
forecast/AI and reviewed per-target/empty presentation; broader status/reaction/
reward families and C07/L4 cold-HUD/frame/residency remain open. Jump deferred.
Keep TiEmu/playtest/gallery unchanged. Separate PC chapter processes respect the
runtime's16-file cache; no runtime change was made solely for the retry fixture.

## Compound current-effect forecasts (2026-10-28)

**OBSERVED, bounded:** tools/area_forecast_oracle.py executes original presence,
formula08 and finalizer in preview mode2 for four groups of13 ordinary human
slots, reusing hashed CORE_BATCH inputs. Per-target probability, signed capped
quantity, null/absorption/status eligibility and Oil flags are recorded. Source
preview still invokes evasion RNG helpers; failure is not an execute miss in
this mode. Calls are observed/stubbed pass. The native pure-preview policy
intentionally omits them. This is not a full original AoE UI/AI dispatcher trace.

**TARGET, implemented:** CORE_AREA_FORECAST.md adds cast_forecast on caller-owned
60-byte CastBatch storage, FORECAST probability phase and pure rejected/refresh
queries. READY/APPLIED and W->cast aliases reject, protecting a live paid root.
Initial command eligibility is separate; exact-cost post-payment inspection
still works. Shared magic_quantity preserves signed narrowing/positive999 cap in
old single-target prediction, preparation and the area API.359 PC/sanitizer/native
checks7E9BBEF7, original52 target inputs, node255/two floors/four views pass.
Layouts/FSV3/five allocations stable; ABI31/ModCtx364/signature63621/zone6204,
private504604(+391), resident64295/headroom641. Warm query max86504/cold110258
are backend costs, not full-frame/residency certification. Next source-backed
AI area candidate/score/cache/scheduling and reviewed targeting/presentation;
retail casts remain disabled, Jump deferred and playtest/live TiEmu/gallery kept.

## Cached current-world area AI (2026-10-28)

**OBSERVED, bounded:** tools/area_ai_oracle.py runs original score-state phase,
hostile hit accounting, ranking and empty-panel scanning:12 thirteen-slot HP/
Dead/Critical profiles, five comparisons and three empty panels on two layers.
Source averaging uses counted hit probabilities; allied hits subtract one even
when absorbed. Preparation/effects/reactions, range/coordinates and VSync are
isolated supplied boundaries. This is not the full original delayed-turn chooser,
MP utility or broad status/Reflect/Charm simulation. Hashes and source commit are
recorded in campaign/x/core-area-ai/oracle/source.json.

**TARGET, implemented:** CORE_AREA_AI.md connects actual ai_think/AI_AIM to cached
immediate hostile formula08 area evaluation. Empty panels, friendly fire,
absorption and whole-map footprints use the shared selector/effect math. An84-byte
cache borrows idle256-byte ai_dist after setup. Stage quanta restore the actor,
preserve area RNG and survive forecast image eviction; rejected casts replan.
Require CAST+AREA together so AREA-only weapon splash keeps its legacy execution.
676 PC/sanitizer/68000 checksD04607FA pass; ABI32/fourteen images, Work/State/FSV3/
five allocations unchanged. Native area max319968 cycles, but existing general
setup/reach stage0 reaches12059828 (~1.005s at12MHz): the full planner360K gate
fails, and general setup must be bounded next. Private509442(+4838), input64447/
headroom489. Retail casts/reviewed area UI and broader core gates remain closed;
Jump stays deferred; playtest/live TiEmu/gallery bytes and dates are preserved.

**User priority update2026-10-28:** CORE_AI_PACING.md accepts the roughly1s
setup quantum inside the requested frozen AI pause. Do not split it just to meet
30fps. Thinking now counts toward a1.5s elapsed256Hz minimum; a slow ready plan
adds no second full hold.171 PC/sanitizer/native checks58CBDE4E pass; all fourteen
images/ABI32/layouts unchanged. Next reviewed area targeting/per-target/empty
presentation; full plan latency, interactive budgets and residency remain open.

**Approved UI integration2026-10-28:** CORE_AREA_UI.md supersedes the historical
PC candidate after user approval.499 PC/sanitizer/native checks67EED097/eight HUD
hashes and four actual full-frame/state checkpoints pass. List rows inspect
victims; Unit/Panel chooses centre binding. Raw range byte4 bit0x10 disables
follow; it is not a primary flag. AreaUi228 aliases idle256-byte ai_dist; existing
Work/State/FSV3/five allocations remain fixed. ABI33/ModCtx372/sig64143,15 images,
zone6264. AREA_UI returns to resident before TARGETS/COMMAND loading; pure card
callback never loads images. AI camera kernel moves unchanged to AI_SETUP3/4
with1321 native-equal checks4CC4400E. Private515564(+6146), resident64719/headroom217,
28 non-code banks unchanged. First13-unit whole-map HUD375252 is not a360K frame
pass; full interactive/stack/residency gates remain open. Reviewed per-target/
empty-centre impact comes next; retail and full core still gated, Jump deferred.
Accepted prototype images/dates are immutable, decision badges cleared.


**Approved impact integration2026-10-28:** CORE_AREA_IMPACT.md now implements the
accepted candidate in shared PC/68000 code.1012 checks12D9C238/64 full screens
D7A684DC and154 bounds checksC56EC8B9 pass;416 accepted candidate captures match.
Per-victim execution labels/flash, covered sloped centre pulse and Oil feedback
retain response/reward ordering. Wait for APPLIED, suppress root FX during
secondary actions, do not display a fake caster result with zero victims.
Cache412 aliases idle ai_gain512; one glyph per wind-up step (max20178 cycles).
Shared area_resolution_cell preserves Crystal/Jump follow coordinates and node255;
public and ordered Work-owner preparation both initialize the cache. ABI34/
ModCtx372/sig64400,16 images/zone6264. NUMBERS2416 owns unchanged scalar drawing;
no executing-image eviction or new callback. Resident64473/headroom463. Native
cold view/full-frame/stack/residency, remaining families/AI/campaign/formats/tools
stay open. Generic effects are a TI adaptation, not original spell artwork or a
full PS1 timeline. Retail casts remain gated, Jump deferred. Next scheduler/
status edges, Quick priority and clock liveness. Old artifacts/dates, standard
playtest/live TiEmu remain unchanged; approval badges cleared.


**Scheduler follow-up2026-10-28:** CORE_SCHEDULER.md closes the previous Quick/
liveness note. The original Quick-only resume phase is9 (phase5 collects pending
attacks); NONE is0xff00, returned after1201 internal phases. Quick255 consumes0
in real selection, but AT forecasts55; Speed0 clamps only in AT. Preserve charge
phase across intervening Quick turns and settle root/responses first. Native
idle8/wake16-tick yields retain the last valid actor without turn credits and
keep the shared quit dialog/rotation usable.263 PC/sanitizer/native assertions
3056BFD2 pass; two nonempty HUD hashes equal ordinary browse. Original callbacks
for expiry/upkeep are isolated; no full Quick spell/animation certification or
retail unlock. Backend wake max256892 is not a full-frame/stack/residency pass.
ABI35/ModCtx372, same core layouts/five allocations, private517932(+195),
resident64681/headroom255. Broader status/reaction/targeting/AI/campaign/tools
remain open; Jump deferred and full-core completion signal still pending.


**Haste/Slow follow-up2026-10-28:** CORE_HASTE_SLOW.md closes tempo CT/AT rules.
Original45 gain profiles/4 charge phases/33 AT lists pin rounding, Slow priority,
Dead exclusion and finite/innate durations after Stop/Sleep wait. Real Slow1 can
stall; AT deliberately uses minimum1. Expiry/upkeep callbacks are isolated;
reuse lifetime evidence without claiming a full spell dispatcher. Mutable tempo
refresh/cure/immune/cancel/KO/revive pass, Charge is unscaled, base Speed stays
stored.748 PC/sanitizer/native checks30A798F0 pass. AT skips constant-rate spans;
Haste gains51..75 need1–2 ticks, so no reciprocal bank expansion. ORDER0 replaces
FORMULA6: ABI36/372/17 images/zone6264, directory+4, same core layouts/FSV3/five
allocations. Private518582(+650), resident64803/headroom133,28 data banks stable.
AT39086/29724 and wake266028 are backend costs; full-frame/stack/residency stays
open. Retail spells/full core remain gated. Next other status/incapacitation/
response/AI/campaign/formats/tools; Jump deferred.


**Incapacity/outcome follow-up2026-10-29:** CORE_INCAPACITY.md separates tile
occupancy from ordinary effect validity and survival. Source KO mask60c5000000,
original camps and defeat priority pass30 original outcomes;4 prepared finalizers
pin stone immunity without cancelling Petrify. Five importance3 cases/cancelling
branch remain source-only.193 PC/sanitizer/native checks55D44C55 cover retained
HP/cell/counter, forecasts/final misses, item payment/preparation RNG, splash,
Counter/AI, off-field AT/outcome, node255 and terminal injection setup. Stop a
terminal game_scenario before waiting for Ramza or applying test-door overrides;
restore real dimensions after abstract binding contexts. ORDER1 pure outcome,
ORDER0 AT, no nested loader; ABI37/372/17 images/zone6316(+52), same core layouts/
FSV3/five allocations. Private518827(+245), resident64765/headroom171. Native
outcome15922/5172 and wake318536 are not full-frame/stack/residency certificates.
No public Petrify infliction/cure/art/persistence or general objective metadata;
full-core gates stay open. Existing mandatory-Ramza permanent-loss rule remains;
future effective allegiance must preserve original camps. Jump deferred.
