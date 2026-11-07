# Final Fantasy Tactics (PS1): study notes

## Throw family (2026-11-02)

**OBSERVED IN FFT:** source formula0x63 uses current Speed times the selected
throwable weapon power, physical target modifiers/zodiac and elemental effects,
without Attack Up, Martial Arts, Faith, critical or knockback draws. Four physical
evade stages retain zero-evade draws and stop at a failure. Physical preview
rounding retains all factors until the final division. Source skillset20 puts
Ball before Knife in learning order. See tools/throw_oracle.py and its unchanged
original function hashes/180 bounded profiles; Catch/RSM/full dispatcher remain
separate.

**TARGET IMPLEMENTATION:** FCJC keeps source learning metadata and supplies80
archive-only item commands at512+item. Existing menus, stock ownership, frozen
category/commit, responses and bounded AI share these records; no new live
cache/layout/allocation. TARGETS owns native cold reach/path/aim. Full cold
UI/render budget and broader RSM/source equipment fidelity remain explicit
open gates in campaign/CORE_THROW.md, including a matched existing-command cost
control. Live TiEmu, user saves and the standard group remain untouched.

- OBSERVED IN FFT, 2026-11-02: `tools/float_oracle.py` executes16 Float
  formula/finalizer profiles, the real movement configuration and24 adjacent
  moves (12 Float,12 normal controls). Float status selects class2/terrain row2
  without increasing Jump or changing the no-equipped-movement clearance/water
  flags. Row2 costs1 except surface63. Effective unit height rises2 plus water
  depth; equipped Float's body/water configuration follows a different path.
- TARGET IMPLEMENTATION, 2026-11-02: CORE_FLOAT.md enables id38 through owned
  status effects and a flag in the existing navigation search. Both queue types
  match the12 Float moves. Earth nullification, Dispel, delayed payment, MP/control
  gates and AI are shared. Existing tile-height targeting/rendering remains;
  original hover/height and equipped movement/monster/mount policies are separate.
  The normal source dry-to-water control costs2 while the existing native adapter
  charges1 with a spare-point rule; this preexisting normal-movement difference
  stays explicit, and ordinary navigation regression controls are retained.

- OBSERVED IN FFT, 2026-11-02: `tools/allegiance_oracle.py` pins236 unchanged
  Steal Heart/Invitation formula/finalizer profiles,18 source camp/control traces,
  repeated Charm refreshing32 ticks, two ordinary outcomes and four initial-camp
  item-decrement cases. Charm retains original allegiance; Invite changes it.
  Source job/item data supplies416 immunity rows. Current camp, original camp
  and original enemy spawn identity serve different targeting/ownership/bonus rules.
- TARGET IMPLEMENTATION, 2026-11-02: CORE_ALLEGIANCE.md enables both commands
  through shared owned status effects and AI. Blue Invite converts are player-camp
  units with forced AI, preserving generic KO rules. Original retention of manual
  control for already player-controlled recipients is outside this native model.
  Victory automatically enrols into a free roster slot, keeps a full roster intact,
  and removes permanently enemy-converted members; source confirmation/dismissal
  menus are not implemented. Four-camp/monster/Finger Guard and full original
  action/menu/recruitment dispatch remain separate.5866 PC/sanitizer/68000 assertions
  and the affected closure pass; no live emulator or user save is used.

- OBSERVED IN FFT, 2026-10-30: unchanged source handlers 0x48..0x4b and
  finalizers give 80 controlled-RNG item profiles; 38 actual numerical HP/MP
  store traces are pinned by tools/restoration_oracle.py. The dispatcher table
  is checked. Undead Elixir sets only HP_DAMAGE in attack_type but retains
  mp_healing: HP 100/300 and MP 20/100 become HP 0 and MP 100. The original
  commit reads numeric channels independently of those flags. Store traces
  isolate reaction dispatch and MP Switch/Distribute/Damage Split, stopping
  before unrelated status/reward handlers. Source outcome/EXP code is read
  separately: no resource/stat/status/timer change means no earned outcome.
- LIKELY INTERPRETATION, 2026-10-30: source attack_type flags alone are not a
  complete description of an item's numerical resource result. A model that
  drops Elixir's MP quantity after Undead reversal loses observed behavior.
- OBSERVED, 2026-10-30 (decompiled MIPS): Punch Art formulas 0x33/0x34/0x35
  use PA with Attack UP x4/3 and Martial Arts x3/2; Punch Art has no weapon
  flags, so the strike is always unarmed. No Faith. Chakra (0x34) heals PA*Y
  HP and half that MP without Undead reversal; Revive (0x35) is Raise with
  (PA+X)% accuracy. Stigma's attacker Berserk/Frog XA is natively unreachable.
- OBSERVED, 2026-10-30 (decompiled MIPS): Talk Skill 0x2a hits (zodiac(MA) +
  zodiac(X))% without Magic AttackUP or Faith; a sleeper is a forced miss
  before any draw; effects by ability id (Persuade CT zero, Praise/Preach +Y,
  Threaten/Solution -Y, Insult/Mimic Daravon status). TARGET IMPLEMENTATION:
  CORE_TALK_SKILL.md (301 profiles); Invitation/Death Sentence/Negotiate gated.
- OBSERVED, 2026-10-30 (decompiled MIPS): Wave Fist 0x31 = PA' x (PA+Y)/2,
  Repeating Fist 0x32 = (PA' + zodiac((PA+Y)/2)) x (rand x X / 32768 + 1);
  both use Attack's physical evasion (four evade draws), the physical support
  stage, a 4% critical after the zodiac; knockback only for a single target.
  0x31 draws a conditional status proc after the damage.
- TARGET IMPLEMENTATION, 2026-10-30: CORE_PUNCH_ART.md enables Stigma Magic,
  Chakra and Revive (99 original profiles, controlled RNG), then Wave and
  Repeating Fist (276 profiles; native evasion keeps Attack's draw model). CORE_PROTECT.md
  enables Reraise (KO-turn revival) and Golem (side pool guard).
- OBSERVED, 2026-10-30 (decompiled MIPS): Foxbird is formula 0x61
  (`battle_formula_lower_brave_y`), formula 0x0a's evasion/accuracy with a
  Brave loss; finalize sets Chicken below 10 Brave and upkeep adds +1 Brave.
  Frog's AI flags are 0x30 (adds and cancels: it toggles). HP damage removes
  Confusion. Frog attacks with no weapon (XA 1, reach 1).
- TARGET IMPLEMENTATION, 2026-10-30: CORE_CONTROL_STATUS.md enables Frog,
  Blind Rage, Confusion Song and Foxbird with shared lost-control handlers
  (56 original formula/finalizer profiles; adapted AI policies).
- TARGET IMPLEMENTATION, 2026-10-30: CORE_TERMINAL.md enables Death, Quick,
  Zombie and Petrify through the source formula dispatchers and final target
  clamp (56 profiles, controlled RNG; 12 KO-upkeep traces for Zombie revival).
  Not a claim of full original upkeep RNG, animation, reward or AI replay.
- TARGET IMPLEMENTATION, 2026-10-30: CORE_RESTORATION.md adds five compatible
  items together and shares preparation/commit with Potion and Phoenix Down.
  Explicit paired-resource tags alias existing scalar self-cost bytes; no
  gameplay/save layout or heap growth. Forecasts invalidate on action start;
  no-effect resource items pay stock but grant no EXP/JP. This does not claim
  a full original inventory/reward/AI/Auto-Potion-selection or graphics replay.

- OBSERVED2026-10-29: original MIPS outcome query uses original team flags,
  defeat priority and KO mask60c5000000 (Crystal/Dead/Petrify/Blood Suck/Traitor/
  Treasure).30 ordinary cases/4 prepared finalizers are pinned by
  tools/incapacity_oracle.py; graphical sorting isolated. Five importance3
  branches and the Petrify-cancelling finalizer are source-only future work.
- TARGET/CHECKED2026-10-29: CORE_INCAPACITY.md separates occupancy, effect
  eligibility and team survival.193 PC/sanitizer/68000 checks55D44C55 cover stone/
  off-field outcomes, ordinary misses/AI, node255 and terminal injection setup.
  ORDER1 owns pure outcome; ABI37/17 images/zone6316. Private518827(+245),
  resident64765/headroom171, same core layouts/save/five heap requests. Old
  frames/pilot saves pass. No Petrify producer/cure/art/persistence or general
  objective metadata is enabled; full-core/frame/stack/residency gates stay open.

- OBSERVED2026-10-28:45 original MIPS gain profiles,4 charge phases/33 AT
  lists pin Haste floor(3*Speed/2), Slow floor(Speed/2), Slow priority and Dead
  exclusion. Real Slow1 gains0; AT clamps base/gain to1. Finite/innate duration,
  Stop/Sleep wait and gains51..75 are covered. Upkeep/expiry callbacks isolated;
  prior lifetime staging reused. tools/haste_slow_oracle.py is not a full spell trace.
- TARGET/CHECKED2026-10-28: CORE_HASTE_SLOW.md integrates tempo requests/CT/AT/
  cure/cancel/KO with748 PC/sanitizer/68000 checks30A798F0. AT skips constant-rate
  intervals with the existing table; ORDER1018 replaces FORMULA6. No retail/data/
  art change. ABI36/17 images, private518582(+650), resident64803/headroom133;
  same core layouts/save/five heap requests. Old frames/pilot saves pass; full
  core and full-frame/stack/residency gates remain open.

- OBSERVED2026-10-28: unchanged original clock/check/AT instructions consume
  Quick255 to0, allow it in phase9 before another due charge, and yield event
  NONE0xff00 after1201 phases when stalled. Expiry/upkeep callbacks are isolated;
  CT255 and completed-turn phase9 are injected inputs. AT clamps Speed0 to1 and
  forecasts Quick remainder55, unlike real consumption. Six choices/one wave/
  three stalled clocks/three AT lists: tools/scheduler_oracle.py.
- TARGET/CHECKED2026-10-28: CORE_SCHEDULER.md adds bounded native idle/wake
  resumption, shared Quit/rotation and original Quick ordering;263 PC/sanitizer/
  68000 checks3056BFD2, two unchanged browse HUD hashes and affected pilot/action
  regressions pass. No retail Quick or full-core production claim. ABI35,
  resident64681/headroom255, private517932(+195), no heap/save/data growth.

- OBSERVED2026-10-26: ten unchanged-MIPS initializer/weapon selections verify
  both hands and lone-left promotion. Twenty original base-XA/status/support/
  zodiac cases cover unarmed, dagger, ninja blade and plain sword scalars.
  The fixture multiplies original XA/YA without crit/element/proc; not a full
  original battle/target/animation/probabilistic-AI trace. Source job records
  15/45/46/89 carry innate Two Swords; source flags select compatible weapons.
- TARGET/CHECKED2026-10-26: ordinary supported Two Swords and dual Counter use
  distinct hands without equipment swaps; preview/AI share potential damage
  and first-hit mutable Sleep wake. Management capability removal returns stock;
  FSV3 sizes remain unchanged. FCJ8 adds20 innate bytes/reuses Item.pad; ABI17
  splits COMMAND/RESPONSE from impact. See
  [campaign/CORE_HANDS.md](campaign/CORE_HANDS.md).

- OBSERVED2026-10-26:32 unchanged-MIPS strike initializers distinguish support,
  menu and empty/single/dual-hand combinations. Five resume branches execute
  continuation before reaction-phase dispatch and rewards after that phase.
  UI/animation/getters/announcement/reward callbacks are isolated and logged;
  not an original full dual-weapon/retargeting battle. Ten original EXP pairs
  retain the maximum award rather than adding each strike's result.
- TARGET/CHECKED2026-10-26:250 PC/sanitizer/68000 assertions verify a bounded
  two-impact ordinary-Attack engine entry, one reaction drain, best EXP/one JP,
  duplicate/miss and early KO/recoil/control paths. ABI16/CoreAction34 adds two
  State bytes. Normal commands remain single-strike; independent hands and
  shared preview/AI must precede a Two Swords producer. See
  [campaign/CORE_STRIKES.md](campaign/CORE_STRIKES.md).

- OBSERVED2026-10-26:27 original MIPS finalize/staging cases,16 stock choices,
  20 Brave boundaries and two EXP contexts cover Counter/Auto-Potion dispatch.
  Counter uses range flag4 bit0x10 and pre-impact status; Dash/Throw Stone lack
  that flag. Auto-Potion checks successful HP-damage type after wake/KO, even
  zero damage. Stock selects Potion/Hi/X in order; initial affiliation owns
  inventory. Source action phases select responses before post-action rewards.
  Graphics/equipment/MP Switch/unrelated bookkeeping and BIOS RNG are isolated;
  full secondary animation/JP/weapon-target/multistrike trace remains open.
- TARGET/CHECKED2026-10-26:267 PC/sanitizer/68000 assertions execute Counter and
  all three Auto-Potions, restore a20-byte primary packet, revalidate shared
  stock/range/status and defer root rewards. Secondary actions cannot recurse
  or earn EXP/JP. FSV3 equipped IDs supply party reactions; other producers and
  families remain gated. ACTION/ABI15 adds46 transient State bytes; local
  evaluation avoids overlay copy ping-pong. See
  [campaign/CORE_REACTIONS.md](campaign/CORE_REACTIONS.md).

- OBSERVED2026-10-26:53 original MIPS reaction-block cases verify the initializer's
  prevention set64950e0214, primary/secondary suppression and pending/global
  reaction conflicts. Three original selection cases verify unit-slot priority,
  root/Stop skips and actor12; counter construction/MP/strike setup are isolated.
  Not an original complete reaction-effect, Brave or multistrike trace.
- TARGET/CHECKED2026-10-26: explicit32-byte root/impact/response context; active
  commands cannot be overwritten and commit consumes each hit/miss once. The
  bounded per-actor response primitives and source guard are checked, without
  enabling Counter/Auto-Potion/Hamedo/Reflect/Mime or a multistrike executor.
  See [campaign/CORE_CONTEXT.md](campaign/CORE_CONTEXT.md).

- OBSERVED2026-10-26:140 original physical-status/support XA cases verify
  Sleeping/Charging3/2 once before zodiac. Nine finalize cases distinguish
  hit/HP-damage-type wake (including zero) from misses/heals/MP/stat effects;
  actual bitmap/timer resolution runs with external reaction/equipment/reward
  and graphics handlers isolated. Four AT lists use max(Stop,Sleep) wait.
- TARGET/CHECKED2026-10-26:1070 PC/sanitizer/68000 assertions enable Sleep's
  API effect:60-tick freeze, Charge cancellation, protected wake at impact,
  actual evaluator/AI/CT/AT and cross-splash adapters. No new producer or Sleep
  art; reaction/elemental/MP Switch behavior stays open. See
  [campaign/CORE_SLEEP.md](campaign/CORE_SLEEP.md).

- OBSERVED2026-10-26:48 original MIPS control profiles,84 end-turn cases and two
  Stop AT lists verify freeze/evasion/command flags, blocked-command CT credit
  and remaining-wait ordering. Dry terrain/no mounts; not Sleep damage/wake or
  full original scheduling. Charging and Don't Act targets lose evasion too.
- TARGET/CHECKED2026-10-26:594 PC/sanitizer/68000 assertions with13 actors share
  Stop/Don't Move/Don't Act across menus/starts/forecasts/AI and deferred Charge.
  Stop pauses rather than cancels Charge; Don't Act is checked when due. Existing
  flags carry the cache, ABI12. Cold AT/glyph setup relocation requires the
  scene module to return before battle entry loads FORMULA. See
  [campaign/CORE_CONTROL.md](campaign/CORE_CONTROL.md).

- OBSERVED2026-10-26: original MIPS78 expiry,85 timed bitmap commit and42
  Poison/Regen staging cases verify counter reset/cancellation/innate protection,
  zero underflow, rounding and blocked actors. External status-specific handlers
  are isolated; original timer setters/removers execute. The between-turn source
  phases put expiry after charges and Poison/Regen after a unit's turn.
- TARGET/CHECKED2026-10-26:4140 PC/sanitizer/68000 assertions exercise13 actors,
  timed masks, menus, actual FORMULA/CT/end-turn adapters and KO cleanup. ABI11,
  +210 RAM bytes; sparse-bit13-actor cold expiry57674 cycles versus88166 before.
  Not a full scheduler/HP-original/reaction/presentation certificate. Details:
  [campaign/CORE_STATUS_LIFETIME.md](campaign/CORE_STATUS_LIFETIME.md).

- OBSERVED2026-10-26:177 original MIPS filter/remove/store bitmap trials cover
  all40 IDs, immunity/innate protection, cancellation, Frog and rider/immortal
  context. External status-specific handlers return without side effects;
  bitmap instructions are unchanged. Timers, team/control, HP and graphics
  are outside this oracle's native certification scope.
- TARGET/CHECKED2026-10-26: canonical15-byte status state plus derived hot cache,
  existing protected cures/AI/preview and Charge/Defend consistency pass4842
  PC/sanitizer/native assertions. FCJ7/ABI10; +218 native State bytes, unchanged
  FSV3. Public application rejects unsupported effects (including new Poison
  until upkeep); equipment trait production and status lifetime/handlers remain
  open. See [campaign/CORE_STATUS_STATE.md](campaign/CORE_STATUS_STATE.md).

- OBSERVED/CHECKED2026-10-26: original Darkness halves physical base-hit before
  Defend halves it again; source Silence restrictions depend on ability flags
  and skillset menu dispatch.86 original MIPS queries feed433 actual-core
  PC/sanitizer/68000 checks. The native hooks share player/preview/AI policy;
  FCJ6 adds224 menu bytes in the archive without extra allocation. Full status
  application/expiry/upkeep remains open. See campaign/CORE_STATUS_RULES.md.

- TARGET/CHECKED2026-10-25: first shared-primary-action L2 slice separates impact
  commit from animation, with common pure HP/status transitions in combat/AI/
  healing forecasts. Existing original-recorded primary effects and six PC/TI
  action pictures/forecasts are unchanged. ABI8, nine images; no new battle/save
  fields or heap allocation. This does not establish full PS1 status/reaction/
  multistrike order. See [campaign/CORE_ACTIONS.md](campaign/CORE_ACTIONS.md).

## Common compact topology —2026-10-25

**TARGET IMPLEMENTATION, VERIFIED HEADLESS68000:** cached four-neighbor rows
replace per-link via storage. ABI7/count9;36-byte lower-only and68-byte two-floor
node allocation. Ordinary paths/costs/pixels are preserved, pilot heap requests
fall1798/1678 bytes, and ordinary search CPU stays at baseline in same-state
native comparisons. Direct neighbor reconstruction was measured and rejected.
Cost6 and32-link capacity pass generic/actual Work fixtures; gameplay still
emits one-gap links. Private group472001 bytes (+119), packed input64825.
Jump refinements are now low priority by user decision; proceed to L2
shared actions/statuses. See [campaign/CORE_COMPACT.md](campaign/CORE_COMPACT.md).


## Long-span cost boundary —2026-10-25

**OBSERVED IN ORIGINAL INSTRUCTIONS:**57 original full-spread queries with
supplied terrain costs/budgets verify remaining = source remaining - gap -
destination cost, and require a positive result. Cost3 plus three intervening
cells costs6; remaining7 succeeds with1, remaining6 fails. The portable prototype
matches68 PC/sanitized/native checks. Source/weather producer was not executed:
the raw384-byte tables and severe-weather→3 mapping are static source evidence.
The current1..5 link/evaluator needs extension before runtime hookup. No game
behavior or assets changed. See [campaign/CORE_JUMP_COSTS.md](campaign/CORE_JUMP_COSTS.md).
The consolidated overall scope is [campaign/CORE_ROADMAP.md](campaign/CORE_ROADMAP.md).


## Resident capacity and actor-column query —2026-10-25

**TARGET IMPLEMENTATION, VERIFIED HEADLESS68000:** splitting PARTY from SAVE
and moving unchanged Status input to SCREENS reduces the resident zone6074→5734.
ABI6/nine images; wider-cost search is linked by default, while graph generation
still has one-gap links. Targeted flow/save/Status/search and accepted pixels
pass. Private group471882 bytes (+1438);28 data hashes unchanged. Packed input
64831 bytes leaves105 below TTPack's64936 ceiling; reports now expose this.

**OBSERVED IN ORIGINAL INSTRUCTIONS:**88 queries execute the intermediate-column
routine with explicit records/frontier flags on both layers. Other-team bodies,
blocked occupants, mountable flags, strict endpoints, standing+3 and one shared
lowest-span reduction are checked.99 PC/sanitized/native checks agree with the
pure prototype, which is not linked to runtime actor generation. One synthetic
lowest-span pair is a byte-contract test, not a played placement. Details:
[campaign/CORE_RESIDENT.md](campaign/CORE_RESIDENT.md).


## Wider-cost search prerequisite —2026-10-25

**TARGET IMPLEMENTATION, VERIFIED HEADLESS68000:** optional linked FIFO buckets
support abstract costs1..5, stable ties and256 IDs in existing queue storage.
27592 graph and1219/1065 actual battle-caller checks pass PC/sanitizers/native.
Search1054 and Work45514 bytes are unchanged. Ordinary production costs retain
the four-bucket algorithm. Enabling the new routine in the resident program
exceeds TTPack's64936-byte input limit, so `FFT_NAV_SPANS` stays test-only for
bound builds. Private ordinary group470444 bytes (+37),28 identical data hashes;
accepted movement views unchanged. See
[campaign/CORE_SPAN_SEARCH.md](campaign/CORE_SPAN_SEARCH.md).

**STATIC ORIGINAL EVIDENCE, NOT NEW EXECUTED TRIALS:**
`battle_move_check_horizontal_jump` checks other-team body records on both
layers, frontier flags and an extra standing-height+3 boundary. The current
single-via Boolean blocker cannot be generalized by checking only the mover's
floor. Execute targeted original actor trials before runtime long-span hookup.


## Forward movement and long-span evidence —2026-10-25

**OBSERVED IN ORIGINAL INSTRUCTIONS:**112 focused terrain-flag/source-panel/
forward-propagation cases verify asymmetric steep exit/entry gates, facing
edges, destination self-headroom and ordinary water height. Horizontal-jump
endpoints may be level/lower, not higher.22 additional full horizontal-spread
trials execute original MIPS in an eastward lane, covering gaps, intervening
solids, bridges, both layers and bounds. Terrain costs are1, no actors present.

**STATIC ORIGINAL EVIDENCE:** unit movement configuration caps ordinary Jump
at7, then derives `jump_half`. The fixtures configure this policy but do not
execute that unit-config function. Fly/IgnoreHeight override it separately.

**TARGET IMPLEMENTATION, VERIFIED HEADLESS68000:**451 endpoint and80 battle
checks pass. User accepted Gariland's ring change; Orbonne is unchanged. Scratch
caching preserves accepted graphs/pixels and saves30.77%/28.18% mean cold graph
CPU at Jump1..4, no extra workspace. Private package470407 bytes (+178),
28 unchanged non-code hashes. See [campaign/CORE_STEPS.md](campaign/CORE_STEPS.md).
The long-span prototype passes278 geometry checks, including native directional
symmetry checks, but is NOT linked to shipping search. Storage, queue costs and
actor spans must be extended before enabling it. See
[campaign/CORE_LONG_JUMPS.md](campaign/CORE_LONG_JUMPS.md).

## Source recoil landing —2026-10-25

**OBSERVED IN ORIGINAL INSTRUCTIONS:**68 focused queries select the highest
eligible floor by facing edges, lower on ties, and four-half-unit headroom.
This query subtracts ceiling depth once; chosen-floor occupancy/untargetability
rejects without another-floor fallback. Original occupancy lookup executes in
explicit fixture RAM. No complete played battle trace is claimed.

**TARGET IMPLEMENTATION, VERIFIED HEADLESS68000:**172 kernel and82 actual
hit/placement/feet-key checks pass. Source-bound recoil uses that drop for fall
HP; the per-map lower-only fast path remains. ABI5/layouts unchanged, private
470229-byte group (+395),28 unchanged non-code variable hashes. Remaining actor
lifecycle/raster/special-movement gates are explicit in
[campaign/CORE_KNOCKBACK.md](campaign/CORE_KNOCKBACK.md). Validation is targeted;
unchanged exhaustive certificates are retained rather than rerun.

## Per-map layer profile —2026-10-25

**OBSERVED IN RETAIL DATA:**109 maps are referenced by ATTACK events or active
WLDCORE random routes;214 terrain variants.77 have no upper stop tile,66 no
unblocked upper movement geometry,64 no positive-height upper ray geometry.
Static references include scenes, not a complete played conditional trace.

**TARGET IMPLEMENTATION, VERIFIED HEADLESS68000:** cached independent flags
skip absent upper kernels;3,234,825 equivalence checks and169 unchanged accepted
PNGs pass. ABI5/eight overlays reduce the resident zone from7406 to6074. Work/
save/allocation layouts stay unchanged; private group469834 bytes (+734 against
CORE_SURFACES, including accepted jump rules). Source upper paths, raster bridge
cover and complete action/status handling remain open. Details:
[campaign/CORE_MAP_PROFILE.md](campaign/CORE_MAP_PROFILE.md).

## Source solid spans and headroom —2026-10-25

**OBSERVED IN ORIGINAL INSTRUCTIONS:**9294 controlled MIPS queries establish
solid-span inclusive top/exclusive bottom, distinct zero-depth lower/upper
fallbacks, blocked-upper projectile collision, byte-valued slope ceilings and
ordinary headroom equality boundaries. Four function byte hashes and fixture
RAM are recorded; this is not a full PS1 movement/render trace.

**TARGET IMPLEMENTATION, VERIFIED HEADLESS68000:**9462 shared kernel/schema
checks and55 permuted-floor battle integration checks pass; source geometry is
required in both retail FCD3 descriptors via FGS1/SGE1. AI straight rays and
ordinary endpoint headroom use it, with a proof-based simple-map bypass.
ABI4, Work45514 (+4), private group469100 bytes (+3033);169 images unchanged.
Source upper-floor traversal/rendering, special movement, knockback and complete
action/status handling remain open. See [campaign/CORE_SURFACES.md](campaign/CORE_SURFACES.md).

## Compact geometry/search kernel —2026-10-24

**TARGET IMPLEMENTATION, VERIFIED HEADLESS68000:**8-bit compact IDs support256
nodes across two floors with separate presence and16-bit full counts. Graph
edges carry leap intermediates instead of averaging IDs.13887 kernel/oracle/
sanitizer checks pass;91186 actual-map distance/predecessor comparisons preserve
current behavior. Battle search/path extraction now use the shared implementation.
Unit/cursor/target/cover/count adapters still use the current lower-floor model.
Synthetic inter-floor links are not original-game movement evidence.

**OBSERVED IN THE NATIVE RUNTIME:** rt_save borrows its source through teardown.
The3138-byte FSV3 buffer now lives after State in a4632-byte core allocation,
surviving victory's Work release and overlay eviction. Native flow verifies
ownership and saved bytes after handoff; no serialized-save migration. Work is
44434 bytes; four battle allocations request107722 bytes, excluding combined
residency/stack/allocator overhead. Details: [campaign/CORE_GEOMETRY.md](campaign/CORE_GEOMETRY.md).

## Native thirteen-actor milestone —2026-10-24

**TARGET IMPLEMENTATION, VERIFIED HEADLESS68000:** the core accepts1..13 actors
from FCD3 descriptors, with13-capacity storage and count-aware modules, AI,
rendering and result metadata. Existing Orbonne/Gariland counts remain11.
1813 boundary checks, four actual13-actor rendering cases,37 campaign and24
opening/scene/control replays pass. Nine fixed11 pictures are byte-identical.
No original Zirekile conditional trace or layered gameplay is claimed.
State now lives in a1494-byte heap allocation so the expanded native program
fits the packer; its lifetime includes the supervisor handoff. Private candidate
467425 bytes, +1380; four requested battle heap blocks total102936 bytes, excluding
combined program/module/grayscale residency, stack and allocator overhead.
Details, limits and reproducer: [campaign/CORE_CAPACITY.md](campaign/CORE_CAPACITY.md).
No live TiEmu input, standard-bundle replacement or new handwritten ASM.

## Reduced port envelope —2026-10-24

**TARGET IMPLEMENTATION, user decision:**13 battle actors; the eleven-Monk
random encounter is excluded.256 compact8-bit tile IDs across both floors,
16-bit counts, explicit validity and separate coordinates/elevation. Original
source addresses can remain16-bit offline; no512-node runtime requirement.
`core_limits.h` is the shared target-limit declaration. Current gameplay supports
13 actors, existing encounters use11, and terrain remains one floor. See
[campaign/PORT_LIMITS.md](campaign/PORT_LIMITS.md).

**OBSERVED IN FFT DATA:** ENTD195 (MAP083 Zirekile) has nine always-present
records and two initially absent definitions. Prior live-overlay squad270 has
four deployment slots. Retained source resources, excluding Test Maps0..4,
fit225 complete lower-coordinate plus unblocked-upper candidates. Largest
stoppable upper-bearing resource: MAP029155+4=159; most upper cells: MAP05318.
These are static source counts, not complete conditional encounter/movement traces.

**COMMUNITY COMPARISON:** reproduced122 JSON files at pinned revisionc4f8252.
Their MAP120 is explicitly labelled fake; some no_walk classifications and
test geometry differ from retail US. Both reports are retained; the community
table is not silently used as the source collision oracle.

## Core envelope audit —2026-10-24

**OBSERVED IN FFT DATA:** the full retail-US census reads121 GNS files,
228 terrain resources,512 ENTD encounter definitions and480 scenario records.
MAP125 is16x16; observed axes reach18.47 map IDs have unblocked upper-layer
cells. Terrain layers use a fixed256-cell stride, not the grid's populated
area. Source status policy has40 entries and five-byte masks. Full evidence,
scope and commands: [campaign/CORE_AUDIT.md](campaign/CORE_AUDIT.md).

**STATIC CODE OBSERVATION:** pinned decompilation reserves21 battle records:
16 ENTD plus five normal deployed records. Persistent party storage is16
Formation plus four guest records. Initial-load presence/sprite/count filters
prevent equating allocated slots with simultaneously active actors. The existing
controlled Gariland state has present slots0..5 and16..20. Raw deployment
event445 declares5+2; its actual runtime interpretation remains unverified.

**HISTORICAL TARGET, superseded above:**21 stable battle identities,16-actor
minimum stress and layer-aware16-bit TileId were the initial audit proposal.
The accepted reduced scope is13 actors and256 compact8-bit nodes.1070 current
eleven-unit PC/68000 boundary assertions and48 native ZX0 decode repetitions
establish a baseline. Hamedo requires a pre-action hook; multistrike/reaction/
KO/victory order needs original traces before implementation.

**OBSERVED IN FFT (title,2026-10-23):** cold-boot controller inputs reach the source logo/map title with New Game, Continue, Tutorial and Sound. Continue enters the zodiac-wheel card screen; a blank card reports No saved data. Source card code iterates15 records with four visible rows. `tools/title_assets.py --observe` reproduces the captures.

**TARGET IMPLEMENTATION:** [campaign/TITLE.md](campaign/TITLE.md): source logo/map plus68k, two requested choices, shared animated glove and three independent TI save slots. Three is the user's adaptation. Legacy ffsave survives explicit migration; metadata is written separately after native teardown so it cannot replace a pending save. Visual review remains pending.

## Deployment, scene order and rewards —2026-10-23

**OBSERVED IN FFT (multiple areas):** script variables0x34..0x38 in the live
Gariland deployment are1/256/0/0/0. Both squad fields at scenario offsets0x09/
0x0b are nonzero in85 source records, not85 distinct maps. Event39/MAP034 uses
IDs260/261 with limits2/2, centers(0,5)/(5,9), flags0x02/0x23. The original
ATTACK controller keeps four25-cell squad buffers, advances groups on Start
and returns on Cross. L1/R1 selects previous/next roster members. The cursor
marker and orientation arrow are drawn even with a single squad. These findings
come from the disc/live table and local controller study; no MAP034 playthrough
or precise meaning of every source arrow graphic is claimed. Reproduce with
`tools/deployment_research.py DISC campaign/x/mandalia/gariland-deployment.state`.

**TARGET IMPLEMENTATION (review refinements):** Gariland uses a white title on
black, the exact shared Formation stats layout, raised white-rimmed placement
slabs and correctly sorted actors. F1/F5 is verified using actual keys; trophy
icons/text are centred as one row. A separate four-view Gariland colour proposal
prefers white geometric edges on gray/dark faces and black only on white ones,
without changing edge visibility, geometry or depth/masks. Active map colours
are retained pending review. Future multi-area encounters must carry actual
grids, capacities, upper-level records and unique units; current Gariland has
one area and does not pretend to offer a second.

**OBSERVED IN FFT:** the unmodified event9 Gariland deployment state generated by
the existing boot door has seven occupied party records at0x80057f74, stride256:
Ramza, four Squires and two Chemists; script variable0x2c is2,000gil. Party
byte6's high nibble is zodiac. Source equipment is adapted in head/body/accessory/
weapon/shield order using source indices0/1/2/3/6. Inventory is at0x800596e0.
The local source-state SHA256 is
`b1d78e072a39093200c28926c495f83ec52019578d158f5ca63b217e4298d6f6`.

**OBSERVED IN FFT:** squad256's record is `8610c600050c002305160001`:
capacity5, center(5,12), oriented legal rows00000/00011/11111/10000/00000.
The initial deployment contains only Ramza at grid cell8, world(4,11). Source
left increments grid column; right decrements it. The same projection/facing
conversion is applied to the native renderer, controls and tactical placements.

**OBSERVED IN FFT:** the victory routines separate initial enemy levels times100
from live present-unit money modifiers times100 and their trophy item fields.
Nonexistent/off-field holders do not contribute trophies/modifiers. The original
also computes a conditional level term which these payout routines do not credit;
it is not added to native funds. Gariland ENTD184 records include Mythril Knife,
Potion and Phoenix Down. Overflow at99 owned including equipped copies credits
the item's quarter price; the gil cap is99,999,999. Prices are read from the main
executable Item table, not an incorrectly offset BATTLE overlay.

**OBSERVED IN FFT:** event8 message0 supplies Academy narration;1..14 are its
dialogue. Event10 messages0..5 precede Gariland and message6 supplies its aftermath
(also present in events11/12). Orbonne's abduction focus corresponds to source
Ovelia at world(4,3). Existing source portraits and map/actor banks are reused.

**TARGET IMPLEMENTATION:** FQF1 stores these source-derived fields/text separately
from portable rules. FSV3 extends the existing save with funds, flow checkpoints
and explicit selected placements. Native starting recruits are fixed to this
observed roster; matching the source random recruitment generator is not claimed.
Serif chapter/location/result strips adapt the reviewed typeface for160x100;
the world texture and source item icons are reused. Enemy aftermath visibility
is separate from tactical/reward state; neutral bystanders face the focus while
KO allies retain their pose. Scripted actors retain adapted choreography, rather
than claiming the PS1's complete camera/scene movements. Source fields, rules,
runtime checks and pending visual/package gates are documented in campaign/FLOW.md.

## Management/font exploration — 2026-10-18

**OBSERVED IN FFT:** `EVENT/FONT.BIN` contains 2,200 glyphs, each 35 bytes,
10x14 pixels in packed 2bpp. All 77,000 bytes match WORLD's font bitmap symbol.
Proportional widths are at WORLD offset 0x733e0. Formation context/Item/Ability hierarchy and
the job wheel were inspected in the local community source study; original
deployment/status was also captured through the existing boot door. Five unit
snapshots and listed JP costs are read from the original state/disc.

**OBSERVED IN FFT (dialogue/title follow-up):** a static scan of2230 referenced
TEST.EVT entries yields3573 pages without decoder errors. The largest portrait
page measured contains103 glyphs/spaces (event425/text10, tutorial explanation).
This excludes a universal bound on substituted names and other WORLD windows.
Portrait boxes force three source lines; width is measured from text. Source
MAPTITLE strips and ATTACK's palette match the original title-load/show routines:
bitmap reveal, shaded leading edge, hold and erase. The five player records unlock
Squire/Chemist only (`c0 00 00`); Knight/Archer both require Squire level2.

**TARGET IMPLEMENTATION:** separate portable menus, three adapted rosters,
small/comfort AMS glyphs, original actors/portraits, original equipment icons and
sample dialogue. Dialogue now returns to5-row ink so the103-character source page
fits at once. Both window dimensions follow the used text; the portrait sits above
it. The FFA dark gradient and grey-to-white frontier remain, without any text/window
shadow. The CATALOG density control selects10x16 actors in4x3 or5x3 cells. Dense
12/15-slot visual fixtures repeat the five original records and are labelled as tests.
The dialogue speaker name retains an extra pixel of top padding; its badge
now fits the actual name width plus8 pixels and loses the extra bottom row.
A configurable White dialogue option inverts body/name/gradient/bevel exactly;
future text goes light-grey to dark-grey to black, with no added assets or RAM.
The black default and session-local preference share the existing theme flag. Change Job offers
animated cards and a reversible Squire/Chemist confirmation in the current menu
session; original raw stats and legal retained equipment provide its derived values.
No campaign/battle saves change. Orbonne/Gariland lettering uses the actual narrow
source artwork with a feathered reveal rather than a substitute font. Header numbers
share the labels' baselines. Equipment icons retain the1280-pixel/512-palette-byte
PS1 VRAM proof. The200-JP Learn balance remains illustrative; full job progression,
inventory and story playback remain future work. See `MANAGEMENT.md`.

`roms/ps1/Final Fantasy Tactics (USA)/` (SCUS-94221, one MODE2/2352 track, 2,464 files, local).
Labels: **OBSERVED IN FFT** (read or measured), **LIKELY INTERPRETATION**, **TARGET
IMPLEMENTATION** (what the TI does, and why).

## Decisions (grilling, 2026-10-09)

- Goal: a rendering and terrain proof of concept of the Gariland battle map: four camera
  orientations (F1 / F5), occlusion that changes with the view, three units, one moved with
  FFT's Move / Jump rules. No combat, menus, AI.
- **Map**: the real Gariland heights, extracted from the local disc by `tools/extract.py` into
  `map.h` (never committed); the art was ours at first (`tools/art.py`). After seeing the
  map's own textures rendered in 4 greys next to our drawing (`x/real_vs_ours.png`), the user
  chose the game's textures (2026-10-09): the scenery is now the mesh drawn on the PC
  (`fftv0..3`, never committed); units, cursor and rotation frames stay ours (the frames' shades are
  measured on the views).
- **Scale**: 24 x 12 tiles, one height unit = 6 px, units ~13 x 20 plus the outline. The user
  asked for FFT Advance's tile (32 x 16) reduced by 20-30 %; three candidates (26, 24 and
  22 px) were rendered on the real map with units (PNG mock-ups) and 24 x 12 chosen. The map
  (301 x 169 px) is larger than the screen: the camera follows the cursor.
- **Rotation animated** (polygon frames between the views), not instantaneous.
- **4 greys**, Titanium first, the program kept under the TI-89's 24 KB.

## Battle decisions (grilling, 2026-10-09, milestones 13-22)

- Fidelity: stats, turn order, damage and hit chance as the game's, each number checked on the
  running original. The AI as FFT's if it can be made fast enough; if an enemy turn costs more
  than ~1 s on the TI, the numbers and a simplified version go to the user first.
- Oracle: the original at Gariland's first turn, reached by poking RAM or a patched local copy
  of the disc (the first battle loading Gariland's), else key scripts through the prologue.
- Random numbers: ours; probabilities and mean damage checked over many trials.
- Units: the disc's (ENTD 0x184, below): five enemies, Delita as an AI guest; the player's
  Ramza and four recruits drawn as the game draws them (two Squires, two Chemists), names
  from the game's lists. Random fields (Brave, Faith, zodiac, equipment, secondary skills)
  drawn each battle by the game's rules (a fixed seed in tests).
- Brave, Faith and zodiac active, shown in Status.
- Deployment: fixed tiles in the deployment area first, FFT's placement screen later.
- Dialogue: the disc's English text, optional (a setting); a box with a portrait (size picked
  on a sheet of 24 and 32 px candidates).
- EXP, JP and level ups earned during the battle, lost at the end; animations faster than the
  game's but not hurried, a key to skip.
- The current demo (Agrias, the thief) is replaced by the battle; its rotation, occlusion and
  reach tests stay with the battle's units.
- End: a results screen, then the battle again with new draws. [ESC] saves and quits, the next
  run resumes. Keys: [2nd]/[ENTER] confirm, [ESC]/[shift] cancel, F1/F5 the camera, [F2] status
  and turn order. Titanium, and the TI-89 through `-pack`.

### Abilities (grilling, 2026-10-11, milestone 18)

- Act opens a list as FFT's: Attack, then the unit's skill set, then its abilities (an item with
  none left greyed); [shift] goes back a level.
- Only what each unit knows, drawn at each battle by FFT's rule like the stats (a fixed seed in
  the tests); Accumulate and Yell cannot be learned at level 1 (300 and 200 JP for at most 199).
- Statuses only as the battle makes them (Dead, Critical, Defending); Heal, Antidote, Eye Drop
  and Echo Grass usable and missing when there is nothing to remove, as in FFT. Defending's
  effect in the formulas now, the Defend command with the AI (milestone 20); Equip Change not
  kept (no effect in battle).
- The bag as FFT's: the new game's items, used up; the enemies' do not run out.
- Done when each ability's preview and outcome equal the original's on one case, with
  distributions for the random ones (Dash, Throw Stone). Enemies and Delita still wait (the AI
  comes at milestone 20). On screen: the user's FFT pose, the number (or the status removed)
  rising, the ability's name at the top right.

### KO and the end (grilling, 2026-10-11, milestone 19)

- A KO unit's death counter shown above it (3 to 0, the panels' 3 x 5 digits); the guest's is not
  (FFT: Delita never turns).
- A crystal gives back all HP and MP (FFT's ability learning dropped: a Squire would learn
  little and it needs a choice screen); a chest one of the dead unit's items, shown by its name
  only (no inventory); both from FFT's own sheet (`BATTLE/OTHER.SPR`).
- EXP rising above the user after the target's number (`EXP+12`, `LvUp` for a level up), JP
  only in Status; FFT's rules as they are (the enemies earn too, JP shared, job levels).
- The results: `Victory!` or `Game Over`, a line per unit of the player's side (level before >
  after, EXP and JP earned), [ENTER] the battle again with new draws. Saving with [ESC] at
  milestone 29 (renumbered 2026-10-13).
- Done when a few rewards equal the original's (an attack, a kill, a level up), the crystal or
  chest odds and the counter's length as the code reads, a battle won and one lost by keys.

### Enemy AI (grilling, 2026-10-11, milestone 20)

- FFT's scoring without its simulation: FFT plays each candidate in its real engine (random
  rolls at their midpoint) and scores the whole field; we compute the same score's change on
  the units the action touches (mean damage, no critical, the KO weight).
- All of FFT's rules kept: the 8 best actions ranked and drawn on their hit chance, act then
  back off to the safest tile, a Critical unit retreats, a mostly-support unit heals and revives,
  the enemies' 1-in-7 push, Delita seeking a healer or a crystal below 45/128 HP, Accumulate and
  Yell on a coin flip, the Wait facing.
- The search as FFT's (Attack from every adjacent tile, the other abilities once per target
  from the safest tile), spread over frames: at most ~150k cycles of thinking per frame, a turn
  decided in under 0.5 s (15 frames); the numbers to the user first if the worst case is over.
- An AI turn shown as a player's without the menus: the cursor on the unit, its move range
  briefly, the walk, the ability's name at the top right, the target marked briefly, the act,
  the facing.
- Defend and the AI's item range measured on the original, then as FFT's: Defend drawn as FFT
  draws a support ability, a command for the player's units that have it.
- Checked on the original: `oracle.py --ai` logs its AI's turns (unit, tiles, ability, target,
  facing) over the 40 turns already replayed and over built situations replayed with other
  draws; our first choice must be FFT's most frequent one, our moves the same tile or one as
  safe.
- No AI for the player's side as a feature; a test scenario where the AI plays both sides to the
  end (tests, xcheck, cycles).
- Measured then: the oracle's enemy Chemist has Defend (support 0x1DF); FFT's AI throws items at
  most the unit's Move away (its exact targeting: 4) and the enemies use the items of their
  level or less (at level 1 only Potion).

## Decisions after playing milestone 21 (grilling, 2026-10-13, milestones 22-29)

From the user's play in TiEmu; the milestones themselves are in `ROADMAP.md`.
- OBSERVED (our build, headless): the "can't attack" and "farthest water tile" reports were the
  cursor on a canal tile drawn on the same pixels as the street tile 2 units higher in front
  (21-27 such spots a view); the reach, the attack and the projection were right.
- OBSERVED IN FFT: a walked tile ~0.33 s, the blow ~0.7 s after the order, an attack with its
  EXP ~3.2 s (menu back at frame ~192 of 60 Hz); ours were 2-3x faster. The weapon is its own
  sprite (`BATTLE/WEP.SPR`, 85,504 bytes, placed by `WEP1/WEP2.SHP/.SEQ`); each unit `.SPR`
  holds a ~48x32 portrait (rotated, raw rows); generic units have names (unit record +0x12C:
  Goodman, Thompson, Barton, Miria, Edgar; Greg, Doreth, Sorvez, Hampsten), drawn anew each
  game; the active-unit marker and the UI digits are in `EVENT/FRAME.BIN`; height shown as a
  large "2h" top right, "depth 1" on water; all 38 canal tiles height 0, depth 1; four
  45-degree yaws only.
- OBSERVED IN FFTA (`x/ffta_*.png`, the snowball tutorial): a see-through band bottom left with
  a bust rising above it, name, HP/MP/JP as numbers; the command menu bottom right under a
  "MENU" tab, a ball cursor; the target's band mirrored bottom right; Status full screen.
- TARGET: the original's durations at 30 fps, FFT's pauses before an AI action, the AI's camera
  turned only when the actor or the target is more than half hidden; move tiles all at once,
  style chosen on a sheet (and a test without depth); the hit flash on the sprite's inside, a
  small knockback, no sparkles; the sword and dagger from the disc during the blow; FFTA's
  layout in 4 greys (light grey bands, not dithered: text must read), FFT's portraits, names
  and marker, the turn order on APPS; water sinking as measured; eight orientations only after
  a one-view experiment.

## Tools

- `psxiso.py ... sources/fft_ps1/disc` lists the files; `--file MAP/MAP022.GNS` (and the
  `MAP022.*` resources) extracts the map. Nothing else was needed: the terrain is plain data
  in a documented format, so the game was not run headless (Gariland is the second battle,
  behind the Orbonne prologue; reaching it with key scripts would have cost far more than
  reading the file).
- Format references: the FFT modding community, `adamrt/fft_toolkit` (`src/map_record.c`,
  `src/terrain.c`, read on GitHub, not copied): map list (`MAP022` = Magic City Gariland), GNS
  records, the terrain block. Every field used was checked on this map (below).
- **The battle oracle** (milestone 13): `tools/oracle.py DISC sources/fft_ps1/gariland_t1.state
  trace.txt` runs the game headless (psxrun's core) from a new game to Ramza's first turn at
  Gariland in 30 s and saves the core state (local, never committed); deterministic (two runs
  give the same battle, RNG included). Gariland is reached without playing Orbonne: the script
  variables `CURRENT_EVENT` (word 0x27) = 6 and `NEXT_SCENARIO` (0x64) = 1, written from frame
  2000 until the game moves on, make the scenario loader start event 7 (the academy scene
  where the recruits join), then event 9 is Gariland. Pad: CIRCLE confirms, CROSS cancels
  (USA). Deployment by pad (UP / LEFT then CIRCLE places the selected unit, R1 selects the
  next one) and by writing the deployment grid for the last two units.
- **Code reading aid**: `adamrt/fft_decomp` (cloned in `sources/fft_decomp`, local), a
  decompilation with named globals and functions: the scenario table and loader
  (`attack_load_scenario_conditionals`), the script variables, the battle unit struct
  (`include/fft/unit.h`), the deployment screen. Read to find addresses, never copied.
- **The battle's units** (milestone 14): `tools/battle.py DISC battle.h` reads the ENTD
  events, jobs, items and generation tables (`make battle.h`); `--check STATE TRACE [oracle.h]`
  compares the oracle's eleven units with that model and writes their raw stats for the C
  tests (`make oracle.h`, which runs the oracle first if its state is missing).
- **The battle's turns** (milestone 15): `tools/oracle.py --turns DISC STATE 40 turns.txt` goes
  on from the first-turn state: Ramza waits (menu DOWN DOWN CIRCLE, facing CIRCLE), then the
  player's five units lose their player-controlled flag (team byte +0x05 bit 0x08, poked every
  frame: `BATTLE_TEAM_FLAG_PLAYER_CONTROLLED`) and FFT's AI plays both sides; a turn is logged
  when `has_turn` (+0x186) rises on `g_battle_turn_unit_id` (0x8018f520), a repeat of the same
  unit with no other CT grown skipped (the AI's trial writes flicker it). 40 turns in 20 s,
  four KOs among them. `battle.py --check ... oracle.h turns.txt` turns them into the fixture:
  each turn's CT bonus is its unit's CT at the next turn less the ticks between (counted on a
  living unit that did not act).
- **The AI's turns** (milestone 20): `tools/oracle.py --ai DISC STATE 40 12 ai.txt` goes on from
  the first-turn state: the move range FFT shows Ramza and the recruit after him (tile byte 5
  bit 0x20 of `g_battle_map_tile_data` 0x8018f8cc, the menu's Move opened, a fresh load each),
  then the AI's first 40 turns as `--turns` plays them, then 8 built situations (the actor given
  CT 100, the others 0; HP, Critical or Dead poked: positions cannot be poked, the game keeps
  them elsewhere too) each replayed 12 times with Ramza's Wait delayed 7 frames more each time
  (other draws). Per turn: each unit's tile, facing, HP, CT, death counter, statuses, the
  learned bytes of its job's row, its support, and the AI's tile priorities
  (`g_battle_ai_data_base` 0x8019f3c4 + 0x5f4), at its turn's start and the next one's; what the
  actor did from its record (+0x187 moved, +0x188 acted, +0x170 last ability, +0x17a / +0x17e the
  target's tile; Defend: acted, no ability, Defending after). 2 min. A probe of the AI workspace
  frame by frame while a unit thinks (its grids, candidate tiles, ranked actions: `battle.h`'s
  `battle_ai_data_t` offsets) settled the Potion case below.
- **Attacks** (milestone 17): `tools/oracle.py --attack DISC STATE 40 400 attacks.txt` goes on from
  the first-turn state: Ramza's menu DOWN, CIRCLE (Act), CIRCLE (Attack), CIRCLE (closes the
  help box), LEFT (the recruit south of him), CIRCLE: the preview; then Execute and OK (two
  CIRCLEs). Units cannot be moved by poking x / y (the game finds a target through its own
  map of the tiles), so the cases poke the two units' stats instead (PA +0x36, Speed +0x38,
  Brave +0x24, right hand +0x1d, sign: birthday +0x09 high nibble, sex +0x06 0x80 / 0x40,
  facing +0x49 low nibble). 40 previews and 400 attacks in 9 min; `battle.py --check ...
  attacks.txt` writes them into oracle.h.
- **Abilities** (milestone 18): `tools/oracle.py --skills DISC STATE 300 skills.txt` gives Ramza
  every Gariland ability by poking (primary skill set +0x12 = 0x19, secondary +0x13 = 0x06 Item,
  learned bits +0x99 (row 0) FC 00 00 and (row 1) FF FC 00, Throw Item: support bits +0x91 |=
  0x08); FFT then shows Guts and Item in the Act menu. Each case: Act, Guts or Item, the entry,
  CIRCLE (the help box), LEFT (the recruit), CIRCLE: the preview (hit % 0x8014d058, amount
  0x8014d056), then Execute and OK; the target's HP and statuses read before and after, Ramza's
  HP and the bag (`g_main_item_quantities` 0x800596e0). A status must be poked in both the
  current set (+0x58) and `inflicted_status` (+0x1bb, what a removal checks), or a cure misses.
  13 cases and 300 executed Dash and Throw Stone each in ~10 min.
- **Rewards** (milestone 19): `tools/oracle.py --rewards DISC STATE rewards.txt` plays six actions
  of Ramza (as `--skills`, every ability given) on the recruit south of him: Attack, a kill (its
  HP poked to 1), a level up (his EXP +0x21 poked to 95), Potion, Heal missing, Dash; every
  unit's EXP (+0x21), level (+0x22), JP and total JP of career slots 0 and 1 (+0xdc, +0x104,
  u16 per job) and job levels (+0xd2) before and after, Ramza's raw stats (+0x72) and HP. 7 s.

## Behaviour and data

### The map files

- OBSERVED IN FFT: `MAP022.GNS` is a list of 20-byte records (type at +4: 0x1701 texture,
  0x2E01 primary mesh, 0x3001 alternative mesh, 0x3101 end; sector at +8, length at +12) per
  time of day and weather; the primary mesh is `MAP022.9` (35,984 bytes).
- OBSERVED: the mesh's u32 at 0x68 points to the terrain: x count (10), z count (15), then
  2 levels x 15 x 10 tiles of 8 bytes: byte 0 & 0x3F surface, byte 2 height, byte 3 slope height
  (& 0x1F) and depth (>> 5), byte 4 slope type, byte 5 ceiling depth and the transient marks
  (0x20: a move destination shown), byte 6 flags (bit 0 blocked, bit 1 untargetable, bit 7
  can't stop: `map_tile_t` in fft_decomp), byte 7 auto camera. Level 1 is all zero on Gariland.
- OBSERVED: heights 0 (canals) to 10 (a chimney); streets and squares at 2, houses 6-8 with
  sloped roofs; surfaces grass, waterway, wooden floor, stone floor, roof, tree, box, chimney,
  bridge. Correction (milestone 20): bit 6 (set on the canals) was read as "can't walk" until
  the move ranges were measured on the original; it has no established meaning. No tile is
  blocked at Gariland: the canals (depth 1) are waded, the chimneys stood on, the trees (bit 1,
  untargetable) crossed but not stopped on.
- OBSERVED: the slope byte is four 2-bit edges, from the top bits N S W E (0 low, 1 half,
  2 high); N is +z and E is +x: every incline's high side meets the neighbour at h + slope
  height (the 25 sloped tiles checked once against their neighbours). A corner is high when an
  adjacent edge is 2, or, with no edge at 2 (convex), when both adjacent edges are 1.
- LIKELY INTERPRETATION: the standing height of a sloped tile is its middle, h + slope / 2
  (FFT shows heights like 7.5 on stairs).
- TARGET: `map.h` holds per tile the four corner heights, top and side materials, walkable
  (not can't-walk, not tree or chimney), standing height in half units.

### Camera and drawing

- OBSERVED (known play, not measured here): FFT turns the battlefield between four diagonal
  directions with a smooth rotation of about half a second, and has two elevation angles;
  units and the cursor are never hidden by a dedicated roof-fading system: the player turns
  the camera to see behind buildings.
- OBSERVED: the primary mesh (pointer at 0x40): counts of textured triangles, textured
  quads, untextured triangles and quads (Gariland: 24, 361, 18, 51), their vertices (s16 x,
  y, z; x 0-280, z 0-420, y 0 to -120: a tile is 28, a height unit 12, y up is negative), the
  textured ones' normals, then per textured triangle 10 bytes (u, v, palette, -, u, v, page,
  -, u, v) and per quad 12 (a fourth u, v). Texture MAP022.8: 256 x 1024, 4 bits, the page
  adds 256 to v; 16 palettes of 16 BGR555 colours at the u32 0x44, colour 0 transparent. The
  untextured polygons are the map's black skirt. Quads are ABCD in the PS1's order (ABC,
  BDC). The mesh's heights per tile match the terrain grid (mean difference ~1 unit, roofs).
- LIKELY INTERPRETATION: a 3D renderer (textured polygons, ordering table) with the map as a
  mesh; the terrain grid only drives rules and the cursor.
- TARGET: no 3D at play time. Each view is a 2:1 isometric picture of the textured mesh,
  drawn on the PC with the depth of each pixel (the tile it belongs to) and read in place;
  the rules turn the terrain grid by renumbering tiles and corners; the rotation between two
  views is three flat-shaded polygon frames from the grid; occlusion is FFT's: things behind
  a building are hidden, turning reveals them (cover masks from the views' depth, README).

### Units

- OBSERVED (game data, known): a squire has Move 4, Jump 3; units cannot stop on another
  unit; enemies block the path.
- TARGET: BFS over walkable tiles, a step allowed when the standing heights differ by at most
  Jump (both ways), allies passable but not a destination; walking 4 frames per tile with a hop
  on height changes. Units were our drawings at first; since milestone 9 they are FFT's own
  sprites (below).
- OBSERVED (`BATTLE/*.SPR`, `TYPE1.SHP`, `TYPE1.SEQ`; formats from FFTPatcher's
  ShishiSpriteEditor and TacticsTemplateG, read on GitHub, the numbers checked on this disc): a
  sprite file is 16 palettes then a 256-wide 4-bit sheet (288 raw rows, then a compressed
  part); a frame is up to 8 tiles of the sheet placed from the unit's anchor (near the feet);
  frames 9-13 face the camera looking down-left, 14-18 face away looking up-left, 3 tiles each
  (body and two arms). The battle idle marches in place (frames 11 10 9 10 11 12 13 12, 6 8 10
  8 ticks), the walk uses the same frames faster (2 4 6 4). Ramza (RAMUZA, chapter 1), Delita
  (DILY), Agrias (AGURI) and the thief (THIEF_M) all use TYPE1. The other two directions are
  the same frames mirrored (exmateria-gambit-tactics' reading of BATTLE.BIN, not verified
  here); the poses are ~14-23 x 37 pixels.
- OBSERVED (a capture of Gariland sent by the user): a standing unit (hair to feet) is ~235 px
  where a canal one tile wide spans ~130 px of height, i.e. ~0.9 of a tile's width; the faces
  show two eyes drawn in the outline's colour (colour 1) inside the skin.
- OBSERVED: the canal tiles have surface type 0x0E (MAP022.9); FFT animates its water.
  TARGET: the views stay still; glints slide on the visible water pixels (README).
- TARGET: the same 10 frames per unit scaled 0.6 to 16 x 26 (0.5, the first choice, made a
  unit 0.75 of our 24-px tile), each eye placed on its own (1 x 2 black at the centre of its
  group of colour-1 pixels in the face, a pixel apart) on a face one grey lighter, FFT's idle and walk timings at ~30 frames per second, the mirroring at draw
  time (a 256-byte table), a world facing per unit, turned with the camera.

### The battle (milestone 13, the oracle)

- OBSERVED (disc, `EVENT/ATTACK.OUT` at 0x801cf938: 480 records of 0x18 bytes): the scenario
  table. Events 1-2 Orbonne's chapel (map 62), 3-6 the Orbonne battle (map 56, ENTD 0x183),
  7-8 the Military Academy (map 24, ENTD 0x188), **9-12 Gariland** (map 22, ENTD 0x184, squad
  0x100, Ramza mandatory). Event 9 runs the battle's condition script 3.
- OBSERVED (disc, `BATTLE/ENTD4.ENT` entry 4, event 0x184 "Gariland Fight"): six units.
  Delita (sprite 04, his own Squire job 04, level 1, Sagittarius, guest: team byte 0x84, AI
  controlled) at (8, 12); five enemies (team 0x90), level 1, Brave / Faith / zodiac /
  names random (0xFE): Squire M with a Broad Sword at (6, 5), Squire M with a Dagger at
  (1, 1), Squire M with a Dagger at (5, 4), Chemist M (Brave fixed 38) at (7, 4), Squire F
  with a Dagger at (3, 3). War trophies: Mythril Knife, Phoenix Down, Potion. Squire skills
  (set 05): Accumulate, Dash, Throw Stone, Heal; Chemist: Item (Potion, Phoenix Down...);
  Delita's set 1C adds Wish, Ramza's 19 adds Yell. The male Chemist's sprite (ITEM_M.SPR)
  uses `TYPE2.SHP` / `TYPE2.SEQ`, not TYPE1.
- OBSERVED (run): a new game creates Ramza only; the academy scene adds six recruits (two
  male and two female Squires, a male and a female Chemist, level 1, Brave and Faith 48-70)
  and Delita in party slot 16. Gariland's deployment area (5 x 5 grid, row by row, 1 =
  valid): `00000 / 00011 / 11111 / 10000 / 00000`, unit limit 5, Ramza mandatory.
- OBSERVED (run, RAM `0x801908cc`, 21 units of 0x1C0 bytes, offsets from fft_decomp and all
  checked against the screen and the party data): job +3, team +5, level +0x22, Brave
  +0x24, Faith +0x26, HP / max +0x28 / +0x2A, MP / max +0x2C / +0x2E, PA +0x36, MA +0x37,
  Speed +0x38, CT +0x39, Move +0x3A, Jump +0x3B, x +0x47, y +0x48. Enemies are units 0-5
  (Delita 0), the player's 16-20. At level 1: Squires HP 34-44, MP 10-11, PA 3-4, MA 3-4,
  Speed 6, Move 4; Chemists HP 35-38, Move 3; Ramza HP 52, MP 16, PA 5, MA 5, Move 5; Delita
  HP 53, PA 6.
- OBSERVED (run, `sources/fft_ps1/gariland_t1.txt`): every unit starts at CT 0 and gains its
  Speed per clock tick, several ticks per frame; all have Speed 6, so all reach 102 on the
  same tick and act in unit order (Delita, the five enemies, then Ramza: ties go to the lower
  unit index). An acting unit's CT drops by 100 when its turn starts and gets 20 back for each
  of Move and Act it did not use: Delita moved only (102 -> 22), enemy 1 moved and acted
  (-> 2). LIKELY INTERPRETATION: FFT's known rule (CT - 100, - 80, - 60).
- OBSERVED: the AI writes trial positions and HP into the real unit records while it thinks
  (a trace shows units jumping between tiles for a frame): read the records between turns.
- TARGET: milestone 14 rebuilds these units and stats from the disc and compares them with
  this table; milestone 15 the CT clock against this trace.

### The battle's units (milestone 14)

- OBSERVED (fft_decomp, read: `main_unit_init_for_battle`, `main_unit_calculate_entd_data`,
  `main_unit_generate_base_raw_stats`, `main_unit_calculate_actual_stats`,
  `main_unit_set_equipment_attributes`, `main_unit_calculate_random_equipment`): a unit has five
  24-bit raw stats (HP, MP, SP, PA, MA; battle unit +0x72, 3 bytes each) and its job's growth
  and multiplier per stat (+0x81). A generated unit draws raw = base << 14 + rand() * variance
  / 2 (rand() 0..32767) from one of four rows (`0x8005e90c` base and starting equipment,
  `0x8005e93c` variance): generic male (base HP 30, MP 14, SP 6, PA 5, MA 4, variance 2 1 0 0
  0), generic female (28 15 6 4 5), Ramza (30 15 6 5 5), monster. Each level above 1 adds
  raw / (growth + level - 1). A stat is raw * multiplier / 100 >> 14 (at least 1; HP, MP <=
  999, Speed <= 50, PA, MA <= 99), then the equipment adds the helmet's and armour's HP / MP
  (`0x80063ed8`) and each item's attributes (`0x800642c4`: PA, MA, SP, Move, Jump; Jump <= 7);
  Move and Jump start from the job (`0x800610b8`, 48 bytes per job). ENTD value 254 means
  random: Brave and Faith 45 + rand() * 30 / 32768, birthday 1..365 (zodiac from the day limits
  at `0x800661e8`), an item one of those of the highest required level not above the unit's
  that its job can equip (not rare). At level 1 for Gariland: Leather Hat (+8 HP) and Clothes
  (+5 HP) on every unit whose ENTD says random, Delita's weapon Dagger or Broad Sword.
- OBSERVED (run, `tools/battle.py --check`): the eleven units at Ramza's first turn read from
  the oracle's RAM: their HP, MP, Speed, PA, MA, Move and Jump are exactly this model applied
  to their raw stats, jobs and equipment; their raw stats lie in their type's range, their
  equipment among the candidates, random Brave / Faith in 45-74, fixed values as the ENTD
  says, start tiles as the trace's first line. Ramza was generated by the new game (type
  Ramza: Brave and Faith 70, Leather Hat, Clothes, Battle Boots: Move 5, Broad Sword); the
  recruits are the Military Academy's ENTD 0x188 units (generated there, then kept: the
  deployed ones are its slots 3, 7, 4 and 2).
- OBSERVED: FFT's x / y are our x / z (every start tile is a walkable tile at the expected
  height; one enemy Squire starts on a roof, h 8); facing 0 -y, 1 -x, 2 +y, 3 +x
  (`battle_move_direction_e`). Both Chemists' sprites (ITEM_M, ITEM_W) use `TYPE2.SHP`, whose
  frames 9-18 are the same poses as TYPE1's.
- TARGET (`tools/battle.py` -> `battle.h`, `battle_units()` / `unit_stats()` in `fft.c`): the
  same model and numbers, drawn with our RNG (`rt_rand() >> 1` for rand()) at each battle
  (scenario 0 seeds from the clock; test scenarios use their number). The units in FFT's
  order (Delita, the five enemies, Ramza, the four recruits: CT ties follow it); the player's
  units at the oracle's deployment tiles facing -y (decision Q11: a fixed placement); Delita a
  guest (passable, not selectable). Ramza's birthday, chosen by the player in FFT, is drawn.
  A random item with two candidates is one of the two (FFT: one of n). Stats checked in C on
  the oracle's raw values (`oracle.h`, `make test`).

### Turn order (milestone 15)

- OBSERVED (fft_decomp, read: `battle_action_run_between_turn_control`, `battle_action_end_turn`,
  `battle_action_calculate_at_list`, `battle_action_sort_at_list`,
  `battle_status_build_upkeep_action`): the clock is a state machine run between turns. ADD_CT:
  every unit whose CT is not frozen (Stop, Sleep, crystal...) gets CT += Speed (halved under
  Slow, x1.5 under Haste), at most 254. PICK_UNIT: the unit with the highest CT above 99, a tie
  to the lowest index (strict comparison in index order); its CT becomes CT % 100; no unit:
  charged abilities tick, statuses expire, then ADD_CT again. After a turn the machine resumes
  at PICK_UNIT, so several units at 100+ act in turn before the next tick. End of turn: +20 if
  the unit did not move, +20 if it did not act (Don't Move / Don't Act count as used), then at
  most 60. A dead unit is picked like the others (its CT ticks): each pick lowers its death
  counter (+0x07, 3 in battle); picked at 0 it becomes a crystal or a chest (rand() & 0x10, a
  wider chance for the player's units) and its clock stops. Every pick draws one rand().
- OBSERVED (fft_decomp): the AT list the HUD shows: the active unit first, then for each living
  unit its next turns, its clock run alone from its CT (no end bonus assumed), keyed tick << 8
  | (100 - CT % 100), inserted in unit order before the first larger key (stable), 40 entries.
- OBSERVED (run, `oracle.py --turns`, 46 turns from the battle's start, the player's side played
  by the AI): every turn's unit and every unit's CT at each pick equal the rule above fed with
  the original's bonuses (0, 20 or 40 per turn) and KOs; Speed 6 everywhere, so the order is
  index order with the bonuses deciding who reaches 100 a tick earlier (a unit that waited,
  42 + 60 = 102, goes before one that moved and acted, 2 + 60 = 62). A KO unit's CT keeps
  growing; the first enemy KO turned into a crystal or chest at its fourth pick, its CT frozen.
- TARGET (`ct_next`, `ct_end`, `turn_order` in `fft.c`): the same rule (Slow, Haste, Stop and
  charged abilities when abilities need them, milestone 18); the death counter kept (a crystal
  or chest at milestone 19); our RNG is not drawn at a pick (decision Q3: no RNG fidelity). The
  AT list of the next ORDER_N (6) turns. Until the menus (milestone 16): enemies and the guest
  wait (+40), a player's unit moves (+20, the turn ends) or waits with [diamond].

### The turn menu (milestone 16)

- OBSERVED (run, `oracle.py --turns`): Ramza's turn opens a menu whose third entry is Wait
  (DOWN DOWN CIRCLE from the first, Move, then Act); Wait asks for the facing, confirmed with
  CIRCLE, and the turn ends with the +40 bonus.
- TARGET (`menu_keys`, `make_window`, `make_panel`, `draw_arrows` in `fft.c`): Move / Act /
  Wait / Status at the top right, one line each in AMS's small font; Move once (greyed after),
  Act greyed until milestone 17; Wait shows four arrows on the neighbouring tiles' sides, the
  facing in black, turned by the arrows as the cursor moves. Differences from FFT, for the TI:
  a move can be undone ([shift] in the menu) until the unit acts, which FFT does not allow;
  Status shows only what the battle uses (level, HP, MP, Move, Jump, Speed, PA, MA: Brave,
  Faith and the zodiac stay hidden numbers, decision of milestone 13; no EXP or JP until
  milestone 19); the units' panels follow Metal Gear Solid: Portable Ops (the user's
  reference) instead of FFT's bottom windows: two thin columns at the bottom corners, read
  sideways, with the name, HP and MP as numbers and gauges, so the map keeps its height.

### Attack (milestone 17)

- OBSERVED (fft_decomp, read: `battle_action_run_pre_formula_setup`, `battle_formula_weapon_damage`,
  `battle_formula_calculate_weapon_damage`, `..._physical_evade`, `..._facing_evade`,
  `..._final_hit_percent`, `..._zodiac_compatibility`, `..._critical_hit`, `..._knockback`,
  `battle_move_check_knockback_destination`, `battle_move_apply_knockback`,
  `battle_target_calculate_strike_lunge_range`, `battle_target_calculate_aoe_vertical_tolerance`):
  Attack runs the weapon's formula; the battle's weapons (Dagger 0x01: knife, WP 3; Broad
  Sword 0x13: sword, WP 4; none: item 0) all use formula 1 and strike (flag 0x80). XA = PA
  (sword, bare hands) or (PA + Speed) / 2 (knife), then the zodiac (below), then a critical
  hit (rand() * 100 / 32768 < 4) adds rand() * XA / 32768 to XA and calls the knockback;
  damage XA * YA, YA = WP (bare hands: PA * Brave / 100, at least 1), at most 999. Each step
  is a 16-bit multiply then a truncated division. Zodiac: the signs' distance indexes
  BATTLE.BIN 0x8018f600 (0 0 0 1 2 0 3 0 2 1 0 0: 1 bad, 2 good, 3 opposite: worst for the
  same sex, best else); p = XA * 25: good +p / 100, best +p / 50, bad -p / 100, worst -p / 50,
  XA at least 1. Hit: evades are the target's class evade (job byte 0x19: Ramza's Squire 10,
  the other Squires and Chemists 5), shield, accessory, weapon (only with Weapon Guard); the
  attack's dominant axis against the target's facing: facing the attacker is the front (all
  evades), the back drops class and shields, a side the class evade; accessory evade always
  counts; each source rolled rand() * 100 / 32768 >= evade. The preview draws the same rolls
  (it advances the RNG) and shows no critical (its random value is 0x4000). Range: the four
  neighbouring tiles whose height (2 h + half, half units) is within [h - 6, h + 5] of the
  attacker's. Knockback (a critical only, not when it kills): one tile away along the
  dominant axis (a tie: y), onto a tile in the map, free, not higher (facing edges compared),
  without water or lava; a fall of more than Jump levels costs (levels - Jump) * max HP / 10.
  At 0 HP the unit is Dead, its death counter 3. HP <= max / 5: Critical (the kneeling pose).
- OBSERVED (`BATTLE/TYPE1.SEQ`, animation records from 0x406, (frame, ticks) pairs and 0xFF
  opcodes): hit 19 / 20 (animations 66-69), the swing 11, 27 (wind-up), 31 (blow), 27 facing
  the camera and 29, 33 facing away (52-57), kneeling 21 / 22, lying 23 / 24 (102-105). The
  weapon is a separate sprite in FFT (WEP*.SPR), not in these frames.
- OBSERVED (run, `oracle.py --attack`: Ramza's first turn, Act, Attack, the recruit south of him
  (a target can be an ally), the preview read at 0x8014d056 / 0x8014d058): the game's own case
  previews 95 % and 12 (Broad Sword, PA 5; the target faces him, opposite signs, both male:
  worst); 40 cases with the attacker's PA, Speed, Brave, weapon, sign and sex and the target's
  sign, sex and facing poked: every preview equals the model. 400 attacks executed in the
  game's case (each after other cursor moves, which draw rand()): 19 misses (4.75 %), 8
  criticals seen (damage above 12, 2.0 %; with XA 3 a critical adds 0 a third of the time), no
  push (the tile behind the target is higher), 11.55 HP per attack.
- TARGET (`attack_range`, `attack_side`, `hit_chance`, `attack_damage`, `attack`, `attack_step`
  in `fft.c`): the same rules and numbers with our RNG (`make test`: the 40 previews equal;
  4000 of our attacks: misses 4.2 %, criticals seen 2.9 %, 11.68 HP per attack). Our draws use
  a 32-bit xorshift: the runtime's wyhash16 (16-bit state) tied two draws in a row, every
  critical then adding at least 1 (criticals seen 3.8 %). Differences, for the TI: Act goes straight
  to the weapon's targets (FFT's submenu of Attack and the skills comes with them, milestone 18);
  only a tile with a living unit can be confirmed (FFT lets an empty tile be attacked); the
  knockback compares standing heights, and a fall's damage is added to the number shown; the
  preview is a small window at the top right (Hit 95% / Dmg 12) instead of FFT's two portraits;
  the poses are FFT's, scaled 0.6, without the weapon (milestone 21); the attack takes 34
  frames (~1.1 s: wind-up 6, blow 4, the target hit 8, the number rising 8 px), the number in
  black with a white outline (FFT: white). EXP and JP: milestone 19.

### Abilities (milestone 18)

- OBSERVED (fft_decomp, read: `main_unit_calculate_abilities`, `main_unit_learn_job_abilities`,
  `main_util_roll_pass_fail`; the JP table 0x80066182 read in the oracle's RAM: 0, 100, 200,
  350...): a generated unit's JP per job are the JP its job level needs plus rand() * 100 /
  32768 (level 0: 100 + that), so 100..199 at level 1 for every Gariland unit; then each
  ability of the job's skill set, in the set's order (`g_main_ability_human_skillset_abilities`
  0x80064a94: 16 actions, then reactions, supports, movements), is learned if rand() * 100 /
  32768 < its learn rate and the JP left pay its cost. Ramza, made by the new game, knows Wish
  only (`learned_abilities[0] = 4`). Only the primary set's abilities can be used (no unit has a
  secondary set at Gariland). Learnable with 199 JP: Squire's Dash (80 JP, 90 %), Throw Stone
  (90, 80 %), Heal (150, 70 %); Chemist's Potion (30, 100 %), Antidote (70, 65 %), Eye Drop (80,
  35 %), Echo Grass (120, 35 %), Phoenix Down (90, 60 %); Delita's Wish (0, 100 %).
- OBSERVED (run, RAM at Ramza's first turn): the eleven units' known abilities (+0x99, a row of
  3 bytes per job) are all outcomes of that rule: Delita Dash and Wish, the enemy Squires Dash
  (two of them Throw Stone too), the enemy Chemist Potion and Antidote, the recruits Dash, the
  Chemists Potion with Antidote and Eye Drop or Eye Drop only. The party's bag
  (`main_item_init_new_game_inventory`): Potion 5, Hi-Potion 2, Antidote 1, Eye Drop 2, Echo
  Grass 1, Phoenix Down 2 and one of each other item; an enemy's item use is not counted
  (`battle_action_decrement_player_item_quantity`: the player's team only), and an item is
  used up when the action starts, whether it lands or not (`battle_action_prepare_attack`).
- OBSERVED (fft_decomp, read: the ability range records 0x8005fbf0, `battle_target_set_ability_panels`,
  `battle_target_set_panels_for_action`, `battle_target_apply_vertical_tolerance`, the formulas
  `damage_random_y_times_pa`, `apply_damage_and_knockback`, `3c_..._heal_target_two_fifths`,
  `heal_z_times_ten`, `heal_random_one_to_z_add_status`, `apply_status_to_action`,
  `battle_status_modify_inflictions`, `calculate_final_hit_percent`,
  `apply_evasion_changes_due_to_statuses`): an ability's range spreads `range` steps over the
  map whatever the heights, then a vertical tolerance (where flagged) keeps the tiles within 2 x
  tolerance half units of the user's height; items reach 1 tile, 4 with Throw Item (every
  Chemist has it innately), heights aside. Dash: 1 step, not the user, within 1 level; damage
  (rand() * 4 / 32768 + 1) * XA, XA = PA with the zodiac (no critical), never evaded; at
  execution only, a knockback (Attack's) when rand() * target's Brave / 32768 < rand() *
  user's Brave / 32768. Throw Stone: 4 steps, not the user, any height, (rand() * 2 / 32768 + 1)
  * XA, evaded as Attack (flags 4 bit 0x02), the same knockback. Heal: 1 step or the user,
  within 2 levels, removes Darkness, Silence, Poison; Antidote Poison, Eye Drop Darkness, Echo
  Grass Silence; a removal keeps only the statuses the target has (`inflicted_status`), none:
  a miss. Wish: 1 step, not the user, within 3 levels; the user loses max HP / 5, the target
  heals twice that. Potion: 30 HP (Z 3 x 10). Phoenix Down: needs Dead, revives with rand() *
  20 / 32768 + 1 HP. Defending halves the base hit (100 -> 50) of evadable actions: the chance
  (50 - evade) * 100 / 50, the roll rand() * 50 / 32768 >= evade. No area, charge time or MP
  cost at Gariland.
- OBSERVED (run, `oracle.py --skills`, Ramza given every ability on the recruit south of him,
  the attack's game case: PA 5, the worst signs, XA 3): previews Dash 100 % and 9 (FFT's preview
  draws its own roll: the amount executed differs), Throw Stone 95 % and 6, Wish 20, Potion 30,
  Phoenix Down 11 on a KO target (2 executed), Heal / Antidote / Echo Grass 100 % on a target
  with the status (removed), 0 % without (a miss; Eye Drop and Phoenix Down used up anyway),
  Throw Stone and Attack 90 % on a Defending target, Defending kept after the hit; Wish's cost to Ramza (52 -> 42) lands ~5 s after the heal.
  300 Dash executed: 1, 2, 3, 4 times XA 25.0, 25.7, 24.0, 25.3 %; 300 Throw Stone: 5.7 %
  misses, once and twice XA 46.3 and 48.0 %; no push (the tile behind the target is higher).
- TARGET (`known`, `can_use`, `skill_range`, `skill_hit`, `skill_amount`, `skill`, the menus
  `acts_keys`, `make_window` in `fft.c`; `battle.py` reads the sets, abilities and statuses):
  the same rules and numbers with our RNG (`make test`: the 13 cases; 4000 Dash of ours 24.7,
  25.3, 24.9, 25.0 %, 4000 Throw Stone 5.1 % misses, 47.3 and 47.6 %; 3000 battles' draws within
  0.003 of the rule's exact odds; Dash on the flat pushed 534 of 1000 times, Brave 70 against
  60). Differences, for the TI: the preview
  shows the range of a random amount (Dmg 3-12, HP +1-20) instead of FFT's one roll; Wish shows
  only the target's heal (the user's HP drops in its panel); a cure shows the first status
  removed; the abilities use the attack's swing pose (FFT has throw and item poses, milestone
  21); a KO unit's tile can be chosen only with Phoenix Down; the Defending status ends at the
  unit's next turn (INTERPRETATION: FFT's own Defend lasts until then).

### KO and the end (milestone 19)

- OBSERVED (fft_decomp, read: `battle_unit_update_attacker_earned_experience`,
  `battle_unit_apply_action_rewards`, `main_unit_check_level_up`, `main_unit_apply_level_growth`,
  `battle_unit_store_jp_and_calculate_unlocked_jobs`, `main_job_calculate_level`): an action
  whose outcome did something earns its user 10 EXP, a kill 20 / the target's KO count (counted
  first), plus the target's level minus the user's, at least 1, at most 99 (Gained EXP Up
  doubles); several targets: the highest award; a miss nothing; reactions nothing. Everybody
  earns, enemies and the guest too, but a Dead, Crystal, Petrify or Treasure user. At 100 EXP
  (level < 99): EXP 0 (the rest lost), level + 1, each raw stat += raw / (growth + the old
  level) (growth 0 counts as 1), the stats made again, HP and MP only clamped to the new
  maxima. JP: 8 + 2 x the job level + the old level / 4 (none without EXP), Gained JP Up x 1.5;
  every other unit whose initial team is the user's gets a quarter, in the user's job slot;
  9999 at most. Job levels: JP totals 100, 200, 350, 550, 800, 1150, 1550, 2100
  (`g_main_job_level_jp_requirements` 0x80066184, read on the disc).
- OBSERVED (run, `oracle.py --rewards`): Attack 10 EXP and 10 JP for Ramza (job level 1 at 173
  total JP: 8 + 2 + 0), +2 JP in slot 0 for Delita and each living recruit (the Chemists too:
  their Squire slot), none for the enemies; a kill 20 EXP and no share for the KO target; from
  95 EXP a level up to 2 with EXP 0, raw HP 518253 -> 561440 (+ raw / 12: Squire growth 11 +
  level 1), max HP 52 -> 55, HP kept at 52; Potion 10 EXP; Heal missing nothing; Dash 10.
  Initial EXP: an ENTD unit's EXP is rand() * 100 / 32768 when its ENTD says random
  (`main_unit_calculate_entd_data`; Delita 78, the enemies 11-98); the recruits (made from the
  academy scene's ENTD: INTERPRETATION) 4-91; Ramza 0 (`main_unit_generate_out_of_battle`).
  Every unit's total JP in its job 100-199 at the start (Ramza 173).
- OBSERVED (fft_decomp, read: `main_status_set_ct`, `battle_status_build_upkeep_action`,
  `battle_unit_generate_crystal_or_treasure`, `battle_unit_generate_treasure`,
  `battle_unit_learn_from_crystal`): the death counter is set to 3 when Dead is set or removed
  (0xFF for an IMMORTAL unit: Delita's ENTD flags 0x84); each dead upkeep (a pick of the unit)
  lowers it 3 -> 0, the next one turns the unit (an Undead one may revive): a crystal when
  rand() & 0x10 (an enemy, 1 in 2) or & 0x110 (a player-controlled unit, 3 in 4), else a
  treasure; it stays on the map as a unit. A unit moving onto it: a chest gives one of the dead
  unit's non-empty equipment slots at random (none: one of the 13 consumables 0xF0-0xFD but
  Elixir); a crystal offers the abilities the mover can learn (each by its learn rate) or
  restores HP and MP, only the restore when nothing is learnable.
- OBSERVED (fft_decomp, read: `battle_action_check_battle_outcome`,
  `battle_menu_dispatch_system_function`): defeat (checked first) when a unit with importance
  3 (Ramza, team 0x0B) is a crystal, a chest or gone, or when every blue unit present, the guest
  included, is in the KO set (Crystal, Dead, Petrify, Invite, Blood Suck, Treasure); victory when
  every other unit is: a plain KO counts. Checked after each action's rewards, at Wait and
  between turns. Gariland's own condition script was not read (the generic path is "defeat all
  enemies"). Its war trophies (ENTD): Mythril Knife, Phoenix Down, Potion.
- TARGET (`rewards`, `job_level`, `outcome`, `pick_up`, `end_battle`, `draw_end`, `ct_next` in
  `fft.c`; the crystal and the chest from `BATTLE/OTHER.SPR`, `units.py`): the same rules and
  numbers with our RNG (`make test`: the six actions equal, crystals 0.509 of an enemy's and 0.748
  of a player's over 4000 each; the counter 3, 2, 1, 0 then the object; a battle won and one
  lost by keys). Differences, for the TI: a crystal only restores HP and MP; a chest's item is
  shown, not kept; JP shared to a unit of another job are dropped (we keep each unit's own job
  only); a single target, so no highest-award rule; the results have no war trophies; the outcome
  is checked after each action and at each turn's start (which covers Wait). The crystal is one
  of FFT's eight turning images, white with a black line, floating; the chest is drawn at 0.55
  (24 px wide) with its greys picked per material (by luminance it was a mush).

### Moving (milestone 20)

- OBSERVED IN FFT (oracle: the move ranges shown to Ramza, Move 5, and to the recruit after him,
  Move 4, Jump 3 both): the canal tiles (surface 0x0E, depth 1) are in them, a chimney too; the
  occupied tiles are not; a tree tile is never a destination.
- OBSERVED (fft_decomp, read: `battle_move_set_tile_flags_for_pathfinding`,
  `battle_move_calculate_spread`, `battle_move_update_candidate`, `battle_move_check_horizontal_
  jump`, `battle_move_init_source_geometry` / `_destination_geometry`): a tile is enterable unless
  blocked (untargetable ones too for a walking unit); a step's remaining range loses the terrain
  cost of the tile it spreads from (`g_main_terrain_movement_cost_tables[0]`, 0x8005ea50: 2 for
  surface 0x0E-0x11, 255 for 0x12, 0x19, 0x1C, 0x3F, 1 for the others at Gariland); its height
  is the exit side's (+ 2 x depth out of water) against the entry side's, at most 2 x Jump; a
  unit spreads by jumps too, over up to Jump / 2 tiles (1 here), to a tile no lower, the tiles
  jumped over below the unit's body; enemies' tiles are not enterable, allies' are.
- LIKELY INTERPRETATION (fitted to the two ranges and the AI's turns, where the code read above
  did not match): a step from water to water costs 2, any other step 1; a step into water from
  land cannot use the last point; a jump costs 2 (3 from water) and climbs at most Jump half
  units (3: a 1.5-level rise), not 2 x Jump. With these the two ranges are exact (0 tiles off of
  150 each) and the AI's turns end on the original's tile 125 times in 136 (73 with the old
  rule: canals, trees and chimneys blocked).
- TARGET (`edges`, `spread_start`, `spread_go`, `compute_reach`, `make_path`): those rules for the
  player's units and the AI; the AI's spread from its focus uses the code's cost (the tile it
  spreads from: from the focus outwards, so a step out of water costs 2), which matched more of
  its moves. A jump's path is one step of two tiles (the walk slides over the gap). Units in
  water stand on its bottom; milestone 27 adds depth-1 sprite immersion below.

### Enemy AI (milestone 20)

- OBSERVED (fft_decomp, read: `battle_ai_build_command`, `select_initial_action`,
  `init_target_consideration`, `find_nearest_target`, `build_movement_scenarios`,
  `select_destination`, `fill_foe_proximity_grid`, `simulate_action_and_score`,
  `classify_ability_effect`, `evaluate_ability_outcome`, `evaluate_movement_origins`,
  `take_next_movement_origin`, `insert_ranked_action`, `select_ranked_action`,
  `choose_wait_facing`, `select_retreat_action`; a sub-agent's report, every point re-read):
  FFT plays each candidate (ability, target tile, origin tile) in its battle engine with the
  random rolls at their middle (`get_random_0_7fff` returns 0x4000: no critical, Dash's mean)
  and scores the field: each unit HP x 128 / max HP plus a signed weight per status (Dead -192,
  Critical -32, Poison -26, Darkness -64 x the share of its abilities it blocks...) plus 357 for
  its kept equipment, the actor's side + and the other -. The best candidate of each ability
  enters a ranking of 8 if it beats the score of moving to the fallback tile (by score, then hit
  chance, then the origin's danger); the ranking is drawn in order, entry i taken when rand() %
  100 < its hit chance, else the first. Tiles: danger = sum over the enemies acting before the
  unit's next turn of max(0, 10 - steps, heights aside); steps from the focus by the unit's moves
  (125 where the spread does not reach); the focus is the nearest enemy (Manhattan, the lower
  index on a tie), for a Critical unit with support left (two of its side alive, one not
  Critical) or a guest under 45/128 HP the nearest unit that can give HP (Potion, Wish) or a
  crystal. Within reach (Move + the shortest range on an enemy, items counting half on an ally,
  at least the steps from the focus) the fallback is the safest tile one step nearer (3 steps
  with a ranged ability against an enemy), else the safest tile from which it strikes next turn
  (that reach halved 1 time in 7 for the enemies); acting from its own tile it acts, then runs the
  decision again with no ability: the safest tile (hit and back off). Attack is tried from every
  tile (its facing changes the hit chance), the others once per target from the safest tile
  reaching it (a target used up only once really tried); an ability that can reach the user is
  also tried on itself from its own tile then the fallback tile (ties: from its own tile first).
  Throw Stone and items thrown 2 tiles or more need a clear straight line (from 24 above the
  feet to 24 above the target's, every 2 units: inside a tile's column up to its height x 12, or
  a standing unit's 16 x 16 x 36 box, blocks it). Enemies use the items of their level or less
  (Potion only, level 1; Phoenix Down is level 13), items at the unit's Move at most. With none
  ranked it goes to the fallback tile, Defending if Defend is its support. At Wait it faces the
  nearest enemy acting before its next turn, by quadrant, unless the neighbour that way is off
  the map, blocked or 3 higher.
- OBSERVED (oracle `--ai`): of the original's 136 turns (40 of the battle, 96 in 8 built
  situations), the same action and target as our first choice 129 times, in our ranking 134,
  the same tile at the end 125, the same facing 128; in each built situation FFT's most
  frequent choice is our first (a Squire next to a recruit attacks then backs off; Critical:
  Throw Stone from its tile then away; out of reach: closer; the Chemist: Potion on the hurt
  ally from the tile its throw reaches, then away; a recruit at 4 HP: Throw Stone 9 times in 12,
  Dash 3; the Chemist with nobody in reach: moves and Defends; Delita under 45/128: still
  attacks). FFT's support abilities: drawn among those the unit's jobs' JP bought
  (`main_unit_calculate_rsm`): at 199 JP at most only Equip Change (Chemist, 0 JP, always),
  Equip Axe or Defend (Squire, 170 or 50 JP); the enemy Chemist of the oracle has Defend (0x1DF),
  the others Equip Change (0x1E0), Ramza none.
- OBSERVED (probe): FFT's tile priorities are never reset between turns (`score_reachable_tiles`
  writes only the tiles it scores); with the unit's own tile left unscored they can make it rank
  first among the origins. LIKELY INTERPRETATION: the case is rare; emulated (the field's
  absolute score, the grid kept across turns, FFT's own grid loaded in the tests) it changed none
  of the 136 turns, so it was left out.
- LIKELY INTERPRETATION: FFT's mostly-support policy (a unit with 77/128 of its usable
  abilities on allies heals and revives first) cannot occur at Gariland (Attack is always
  usable, an enemy Chemist only Potion), nor statuses but KO, Critical and Defending.
- TARGET (`ai_setup`, `ai_setup2`, `ai_ability`, `ai_ability_go`, `ai_score`, `ai_clear`,
  `ai_choose`, `ai_facing`, `ai_think`, `ai_next`, `ai_step`): the same rules computed on what an
  action changes (the units hit, the user for Wish, a crystal's or a chest's pickup), the same
  ranking and draw with our RNG; Defend drawn as FFT draws a support (Defend learned with the
  Squire job's JP after the reactions and supports listed before it, then one of the two
  supports), a command of the Act list for the player's units that have it. The turn shown as a
  player's without the menus. Cost on the TI: the picture stands still once two frames in a
  row have the same checksum (the animation clock stops while thinking), the frame is then not
  drawn and the thinking runs a step a frame within ~270k cycles: 5 frames a turn on average,
  6 at most, after the camera's pan (~13 frames, as before).

### Presentation (milestone 21)

- Decisions (grilling): every frame of the game within 360k (the start, the rotation and a
  picked tile's path allowed as stalls); C before asm (asked first); FFT's throw pose only (no
  weapon sprite: 1-2 px at 0.6; no item pose); the hit as FFT draws it, measured; the camera:
  while walking it follows the walker past a margin, while aiming or acting it stays when both
  units are on screen, else centres between them; the markers drawn from each tile's real
  corners; the HUD as it is unless the user flags something.
- OBSERVED (`BATTLE/TYPE1.SEQ` animations 58 / 59: frames 11, 27, then 28 held facing the camera
  and 16, 29, 30 facing away; run headless, `oracle.py`'s skills state, Ramza's Throw Stone on
  the recruit, a picture every 3 frames): the thrower raises the arm behind the head then holds
  it forward over the head: the same wind-up as the swing (27 / 29), then 28 / 30 instead of the
  blow 31 / 33.
- OBSERVED (same run, the target every 2 frames): at the impact a white burst about twice the
  unit's size covers the target for ~4 frames (60 Hz), then breaks into white sparkles and a
  blue glow flying apart for ~12 frames; the number rises over it; the target's sprite itself
  keeps its colours (the hit pose).
- TARGET: the throw pose for Throw Stone and the items (`unit_pose`, `units.py` frames 28 and
  30); the burst as a white blob outlined black for 2 frames and 8 outlined sparkles flying
  apart for 6 (`art.py` `spark`, `draw_spark`), for damage that hits: white alone vanishes on the
  light floors, FFT's glow cannot be drawn in 4 greys.
- OBSERVED (`map.h`): 25 of the 150 tiles are sloped: ramps of 1 or 2 units (two adjacent
  corners up) in the four directions, and two tiles with one corner up.
- TARGET: the cursor and the rings drawn on the tile's four corners at their heights (`art.py`
  `slope_marker`, 13 shapes, picked by the corners' lifts in the view, `marker`).

### Pace (milestone 23)

- OBSERVED (run headless from `gariland_t1.state`, Ramza's first turn; each unit's movement
  record `g_battle_unit_misc_data` 0x800b7308, 16 x 0x440 bytes, found by its backlink +0x134
  to the unit's stats: `real` +0x18 x / height / depth in 1/4096 world units, `step_phase`
  +0x7f; read in fft_decomp): Move, a tile picked, the unit walks at once (6 frames after
  [O]) and FFT asks "Are you sure you want to move here?" once it stands there. A flat tile
  is two phases of 7 frames at 60 Hz (to the edge, to the centre: 8190 units a frame, a tile
  28 x 4096): 14 frames, 0.23 s; a step up 2 units: the edge, a jump phase rising 10 frames
  and falling 2, the centre: ~21 frames. (Our first estimate, 0.33 s, counted the cursor's
  glide.)
- OBSERVED (Ramza's Attack on the recruit, Broad Sword; pictures every 2-6 frames): Execute
  at 0; the panels close (8); the wind-up from ~10, the blade from ~26; the slash and the hit
  at ~31 (the HP lowered in RAM at +10, ahead of the picture); the target's hit pose ~48
  frames; the number rises ~12 frames and stays to ~+84; "10 EXP." +92-+128, "10 JP" +134-+164;
  the menu back at ~+188 (3.1 s).
- OBSERVED (the first AI turns after Ramza waits, a Chemist's Attack): the turn's unit at 0,
  the camera pans to it (~30 frames), its panel ~36, the move range then the walk; after the
  walk the camera turned (~16 frames) where the actor was behind a house; its panel again
  (~36), the cursor glides to the target (~12) and its panel (~12), the preview with the hit
  chance ~30, the blow ~30 later; a level up message ~40; the next turn ~200 after the hit.
  No ability was used in these turns: the name box was not measured (~1 s kept from the
  grilling).
- TARGET (30 fps, half the frames): a tile 7 frames, a climb, drop or jump 10 (a hop table);
  the attack: standing 5, wind-up to 13, blow to 16, the hit at 16 (hit pose 24, the number
  24), the EXP at +29, the JP at +51 (18 frames each), the menu at +78 (+28 when nothing is
  earned); the AI: the cursor gliding a tile every 2 frames to its tile and its target, the
  preview 15 frames, an ability's name 30 more. The panels' slides are not reproduced (our
  panels are drawn when they change).
- TARGET (the AI's camera): FFT's rule is not read; ours turns only when the actor (on the tile
  it acts from) or its target is more than half hidden, to the view hiding the least of the
  worse of them, and the player's view comes back on the player's turn. Hidden rows from the
  map's corner heights (the top faces' upper edges at four columns of the sprite): as the
  view's depth says half hidden or not on 558 of 560 tiles and views (`make test`).

## Blows (milestone 25)

- OBSERVED IN FFT (disc plus `tools/weapons.py --check DISC STATE`): the level strike uses
  unit SEQ 130/131, body wind-up 86/135, blades with body 87/136, 89/138, 90/139. Weapon
  scripts 2/3 in WEP1 (TYPE1 units) and WEP2 (Chemists) select their own shapes. WEP SHP
  has 32 weapon-type offsets at +4, 512 frame offsets at +0x44, data at +0x846; its header
  rotation indexes `BATTLE.BIN`'s table at 0x80094508. This is a screen rotation, applied by
  `battle_gfx_construct_polygon_data_for_units`. WEP.SPR's first palette block and sheet at
  +0x200 supply the blade; `g_battle_gfx_item_graphic_data` at 0x800943e4 supplies its palette
  and row. Dagger type 1, Broad Sword type 3. On the original's back-facing level swings,
  frames 34/35/36 and 74/75/76 match our body frames, weapon animation, rotation and part
  position/size/UV exactly. The fixture updates the initialized display weapon as well as
  battle stats; a stats-only poke leaves the original's graphics stale.
- TARGET: existing 0.6 scale and original palettes, a blade thickened horizontally and
  vertically to at least two pixels, white outline. Rotation and sampling are offline;
  separate archived `fftw` (40,704 bytes), no RAM sprite copies except the covered blade.
  Armed hands/body use the original level-strike frames; low/high swings remain adapted to
  the level swing. Blade drawn behind the body (the ordering table prepends the original's
  weapon layer), hidden during wind-up, visible at 13-18, put away at 19.
- TARGET (user's explicit choice): the earlier burst/sparkles removed. Successful damage
  flashes only the sprite's fourth-plane interior: light grey 2 frames, white 8, light 2,
  then normal; black contour and dark shadow stay. Rendered recoil away from the attacker:
  horizontal 2 px/vertical 1 for 6 frames, then 1/1 for 4, then 0. The logical tile, HP,
  hit/miss, abilities and critical gameplay knockback are unchanged.
- CHECKED: 91 interior pixels light/white, 32 black contour pixels unchanged, misses/heals/
  Defend unflashed; `make test`; 25 default xcheck scenarios, 87 additional PC/TI screens at
  blow phases and replay ends (`make check-blows`). Every measured ordinary frame <= 360k:
  blow doors 307,218 max, 14,000-frame AI run 356,180, defeat replay 359,070. Initial/view
  loads, rotations and all-at-once reach creation are the allowed stalls. Optional HUD,
  cursor mask and ring removal no longer force themselves into a crowded camera pan.
  Review sheet: `make review-blows` -> `x/m25_blows.png` (local).

## Numbers

| | FFT | TI | how |
|---|---|---|---|
| map | 10 x 15 tiles, h 0-10 | same | `extract.py` (MAP022.9 terrain) |
| tile / height unit | 28 / 12 world units | 24 x 12 px / 6 px | ratio kept (6.3 px) |
| unit height | ~0.9 tile width (capture) | ~22 px of 24 (scale 0.6) | capture, `units.py` |
| Move / Jump | Squire 4 / 3, Chemist 3 / 3, Ramza 5 / 3 (Battle Boots) | same | job and item data, oracle RAM |
| HP at level 1 | Squires 34-44, Chemists 35-38, Ramza 52, Delita 53 | same model, drawn | `battle.py --check`, `test_draw` |
| rotation | ~0.5 s, smooth | 3 frames, 0.54 s | ti-cycles |
| CT | +Speed per tick, act at 100+, -100, +20 per unused Move / Act, max 60 | same | decomp, 46 turns of `oracle.py --turns` |
| Attack | XA (PA, knife (PA + SP) / 2) x WP, zodiac, critical 4 %, hit 100 - class evade from the front | same | decomp, 40 previews and 400 attacks of `oracle.py --attack` |
| abilities | learned: JP 100..199 spent in order on passing learn rolls; Dash 1..4 x XA, Throw Stone 1..2 x XA, Wish 2 x max / 5, Potion 30, Phoenix Down 1..20 | same | decomp, 13 cases and 600 trials of `oracle.py --skills` |
| rewards | 10 EXP (kill 20 / KOs) + level difference, 100 a level; JP 8 + 2 x job level + level / 4, a quarter shared; death counter 3, then crystal (enemy 1/2, player 3/4) or chest | same (the share to the same job only) | decomp, six actions of `oracle.py --rewards` |
| moving | canals waded (water to water 2), jumps over one tile, trees crossed | same, the two measured ranges exact | 2 ranges of `oracle.py --ai`, decomp |
| AI | scores the field (HP x 128 / max, KO -192, Critical -32), ranks 8, draws on hit chance; danger 10 - steps; hit and back off | same, 129 of 136 turns the same action, 125 the same tile | decomp, 136 turns of `oracle.py --ai` |
| a hit | a white burst ~4 frames, sparkles ~12 (60 Hz) | interior flash 2 light / 8 white / 2 light, small visual recoil, no sparkles | user choice, milestone 25 pixel checks |
| pace (60 Hz / ours 30) | a tile 14, a jump ~21; Execute to hit 31, menu +188; AI glide ~12, preview ~30 | 7, 10; 16, +78 after the hit; 2 a tile, 15 | movement records and pictures, § Pace |

## FFTA-style interface (milestone 26, 2026-10-13)

- OBSERVED IN FFTA: the local original captures `x/ffta_*.png` show bottom portrait bands,
  bottom-right MENU and ball cursor, target band mirrored, full-screen Status with a second
  equipment page. These are layout references; the numbers and rules remain FFT's.
- OBSERVED IN FFT: portrait raw pixels are `.SPR +0x8200..0x9200`, a 256x32 texture.
  The original quad maps UV `(128,0),(128,31),(80,0),(80,31)` to the portrait's four corners:
  the 48x32 patch is rotated into a 32x48 bust. Portrait palette 8 matches player CLUTs in
  the oracle's VRAM; enemy recolour palette 11 is adapted to the same four-greys portrait.
  `battle_gfx_load_spritesheet_into_vram_slot` and
  `battle_menu_build_unit_portrait_poly` establish the texture and UVs.
- OBSERVED IN FFT: the floating selection pointer uses FRAME.BIN UV `(64,128)..(80,152)`,
  CLUT `(64,0x1E2)`. Its palette is `BATTLE.BIN` at loaded address `0x80094B64`. Texture
  pixels start at FRAME.BIN `0x1000`. The original points at its cursor; the target uses
  this shape over the active unit, as requested, keeping native tile targeting rings.
- OBSERVED IN FFT: `EVENT/SMALL.OUT` loaded at `0x801D7000` binds name sections through
  offsets at `+0x70` and string base `+0xEC`; the names start at `+0xED`. Entries
  `0x100..0x1FE` are 255 male names; `0x200..0x2FE` are 255 female names. Name records use
  digits `0..9`, capitals `0x0A..0x23`, lowercase `0x24..0x3D`; `FA` is a space,
  `D9 C1` an apostrophe, `D9 B6` a period, `FE/FF` terminate a record. Names are at most
  11 characters. `main_unit_store_character_names` excludes used names of the same gender.
- TARGET IMPLEMENTATION: `tools/interface.py` creates archived `ffti` (40,876 bytes),
  generated `interface.h`, six mirrored 19x29 busts with white outlines, a 16x14 pointer,
  two fixed-width name tables and AMS small glyphs pre-shifted offline. Portraits use
  nearest-neighbour scaling, four greys by the existing palette-luminance thresholds.
  Generic names are unique by gender, drawn once per battle by an independent 16-bit
  generator seeded from that battle; existing combat/stat RNG streams stay unchanged.
- TARGET IMPLEMENTATION: 80-pixel horizontal bands, light grey with a dark rule, portrait
  rising above the band, name / HP / MP / Lv-CT. Menus replace one another bottom right
  under a tab, ball cursor; a valid target gets its mirrored band and the hit/amount
  preview above it. Motion and action animations hide bands and menu. Height `2h` / `0h`
  and canal `depth 1` appear top right. Status pauses simulation: stats on page 1, equipment
  and learned skills on page 2; LEFT / RIGHT pages, ESC / shift returns.
- CHECKED: `interface.py --check DISC gariland_t1.state` matches all six portrait sheets
  against the original's VRAM (9,216 source portrait pixels), pointer texture/palette and
  all nine generic oracle names. Native tests cover seeded name stability/uniqueness,
  HP/CT updates, page reversal with unchanged units, depth label and hidden motion bands.
  40 xcheck scenarios, 116 additional UI partial-build/page/input screens and 87 existing
  blow/replay screens agree between PC and TI. Fifteen review states are in local
  `x/m26_interface.png`. No new hardware path; no emulator run for this milestone.
- CHECKED: all ordinary measured frames <=360,000 cycles: UI doors 313,278, the 14,000-frame
  AI replay 357,270, defeat replay 359,840. Profiling found a just-completed background cache
  was permitting forced HUD work in the expensive frame that built it: `bg_drawn` records
  whether the frame actually used the cheap cache. Budget also adjusts when scene edits
  invalidate a previously usable copy. Aligned long copies and cached preshifted text keep
  the native UI small; Status builds a part per frame, peaking at 173,440 cycles.

### Character HUD revision (2026-10-14)

- TARGET (user reference): transparent character details, with a cutout portrait on the
  left, name to its right, HP and MP stacked with small current/maximum values above
  their gauges, and only a small level under the portrait. No zodiac badge or CT field.
  Both active and target panels use this layout; Status keeps its full statistical pages.
- IMPLEMENTATION: the original portrait's palette-zero mask already removes its background;
  the previous grey band hid that transparency. Reuse the same six outlined 19x29 assets,
  with no new bank or extraction. Five cached parts compose an 80x36 panel, including an
  opacity plane for the portrait, outlined glyphs and 56-pixel gauges. HP is black, MP dark
  grey, empty gauge interiors light grey. Values align to the right above each gauge.
  CT no longer invalidates the panel cache. Motion and blows still hide the details.
- CHECKED: existing native tests, 52 scenario PC/TI screens, 116 UI phase/input screens and
  87 blow/replay screens pass. Every ordinary measured frame remains <=360k: UI doors
  317,458, 14,000-frame AI replay 356,014, defeat replay 357,308. The generated panel text
  and HUD loops have no software multiply/divide or floating-point helpers. Program 62,075
  bytes; allocations 52,320 and 55,556 bytes, both below 64 KB. Actual framebuffer review
  covers all six portraits, empty HP/MP, 500/999 HP, 999/999 MP and level 99, in local
  `x/character_hud_details.png`; active/target scenes are in `x/character_hud.png`.
  No hardware path changes or emulator run.

### Compact attack HUD revision (2026-10-14)

- TARGET (second user reference): level over the portrait, shorter/thinner gauges, two
  opposing character panels during an attack, with the portrait defining the HUD's height.
- IMPLEMENTATION: complete 80x29 panels, level over the lower portrait corner, gauges
  34x2 with a one-pixel white border. Left portrait mirrors inward, right portrait sits
  at the outer right edge and looks inward. Name/forecast, HP and MP fit in the portrait's
  height, with no panel background. Palette-zero portrait cutouts and archived data stay
  unchanged. The valid target's cached inline damage/heal/status and chance replace the
  actor's name, with an outlined central chevron; the old TARGET window is removed.
  The forecast calls the existing combat formulas, including skill amount ranges, and
  caches its target/ability together with the actor's stats. Target name remains visible.
- CHECKED: native tests, 52 PC/TI scenario screens, 116 UI partial/input screens and 87
  blow/replay screens pass. Every ordinary measured frame stays <=360k: UI doors 317,566,
  14,000-frame AI replay 356,116, defeat replay 357,410. Native HUD/text loops have no
  software multiply/divide or floating-point helpers. Program 62,109 bytes; allocations
  52,320 and 54,300 bytes, saving 1,256 RAM bytes from the preceding panel layout.
  Actual framebuffer review: `x/compact_attack_hud.png` (menu and attack),
  `x/compact_hud_details.png` (six portraits, empty bars, level 99 and three-digit values).
  Potion aiming also shows the inline positive prediction. No hardware path changes.

### Gauge alignment refinement

- TARGET / IMPLEMENTATION: align the gauge's visible left edge with the text anchor,
  round its four outer corners through the opacity mask, and align the values' right
  outline with the gauge's right outline. The two-row fill stays thin; width 40 leaves
  a gap between the ten-pixel `MP` label and the 28-pixel `999/999` string.
- CHECKED: native tests, 52 scenario comparisons and 203 UI/blow/replay PC/TI screens
  pass; ordinary peak remains 357,410 cycles. Program 62,171 bytes, allocations unchanged.
  Actual PC framebuffer review includes three-digit HP/MP on both panels at once;
  rounded outer corner pixels remain transparent. Local review: `x/rounded_hud.png`
  and `x/rounded_hud_999.png`.

### Target attachment and active-marker revision

- TARGET / IMPLEMENTATION: enemy gauges and values now end against the portrait at
  panel column 61; the target name aligns to the same edge. Both character names remain,
  while the centered prediction uses a separate seven-row outlined cache above the
  comparison arrow. Cache matching suppresses a stale prediction when target/ability
  changes. All 510 source names fit: the longest measured string is 43 pixels, leaving
  the target name's first pixel inside its panel. Three-digit HP/MP still fit both sides.
- TARGET / IMPLEMENTATION: a native 16x19 active pointer replaces the unreadable reduced
  source marker. `tools/art.py` draws a ring, wide crossbar and shaded pointed blade,
  dilates the silhouette for a white outline, and emits three native word-row planes in
  `art.h`. The original archived pointer stays as unused reference data, preserving the
  existing bank layout. Its full body/marker top margin is 39 pixels. Facing selection
  holds the camera when the actor fits, avoiding unnecessary repeated full-view copies.
- CHECKED: tests, 52 PC/TI scenario screens, 116 UI partial/input screens and 87 blow/replay
  screens pass. The AI finishes the same 80-turn native battle in 10,086 frames. Every
  ordinary measured frame stays <=360k: the 14,000-frame AI replay peaks at 350,712, defeat
  replay at 357,400. A 369,558-cycle facing frame during the initial camera-margin change
  was mandatory work (budget already negative): profiling isolated the moving view copy,
  eleven actors and the facing arrows. Holding the fitting camera removes that overrun;
  no extra stall exemption or larger budget. Generated outlined-text/HUD loops have no
  software arithmetic helpers. Program 62,733 bytes; allocations 52,320 and 54,930 bytes.
  Review: `x/target_hud_marker.png`, `x/target_hud_999.png`, `x/active_marker.png`.

### FFT command hand and Status inspection refinement

- OBSERVED IN FFT (ordinary pad input, `gariland_t1.state`, no RAM writes):
  `DOWN:24 DOWN:24 DOWN:24 CIRCLE:120` opens Status; `SELECT:60` activates its
  field cursor. Four `DOWN:30` presses followed by `CIRCLE:60` open the Move help
  panel. The original uses a white glove outside the command frame and beside the
  selected Status field. Local original captures: `x/ps1_status_overview.png`,
  `x/ps1_status_select.png`, `x/ps1_status_down.png`, `x/ps1_status_help.png`.
- OBSERVED (read in `fft_decomp`): `bunit_menu_update_node_cursor` handles directional
  selection and confirmation/cancel for field explanations, including equipment and
  ability nodes 18..27. `bunit_menu_equipment_and_ability_panel_thread` draws five
  equipment slots and ability/skill-set entries. Actual gear swapping is a separate
  Change Equipment action (`battle_menu_run_equip_thread`), outside native Gariland's
  current action set. The pad capture above confirms field help, not a gear change.
- TARGET / IMPLEMENTATION: a native padded 16x9 outlined white glove replaces the
  internal ball, extending outside light-grey framed menus with black title tabs.
  Status's second page selects each of its five equipment slots and learned abilities,
  including learned Defend. Confirmation opens existing item bonuses or ability
  range/height, JP, effect, stock and evasion; ESC returns to the list. Browsing never
  changes equipment, HP, learned skills or inventory. Cursor movement reuses the cache;
  detail opening/closing keeps the nine-part per-frame build.
- TARGET / IMPLEMENTATION: active pointer reduced from 16x19 to padded 16x13, retaining
  the ring/crossbar/blade and white silhouette outline. Camera framing stays at 39
  pixels, now including six pixels of headroom. Reducing that margin with the sprite
  caused a 363,120-cycle defeat frame; keeping established framing removes the overrun.
  The terrain HUD draws height alone; removing the depth line saves 600 cache bytes.
- CHECKED: unit tests (slot/ability navigation, detail/back, empty entries and unchanged
  unit data), 58 PC/TI scenario screens, 177 partial/input UI screens and 87 blow/replay
  screens pass. Ordinary peaks: UI 295,764, Status 202,426, 14,000-frame AI battle
  349,634, defeat 356,322 cycles; no budget exemption added. Program 64,937 bytes;
  allocations 52,320 and 54,332 bytes. Review: `x/m26_interface.png` and
  `x/status_details.png`. No hardware path changed or TiEmu interaction.

### Animated glove and narrower pointer refinement

- TARGET / IMPLEMENTATION (user's marked-up native screenshot): active marker outline
  width 10 -> 8 pixels, keeping its 13-row height and existing camera framing. Glove
  outline width 15 -> 18 pixels, padded to 32 for the existing ExtGraph sprite primitive.
  An eight-byte lookup gives a three-pixel sideways motion in 32 native frames; the
  maximum-right tip leaves a one-pixel gap before menu and Status text. This cadence is
  a native visual adaptation, not an original FFT timing measurement.
- TARGET / IMPLEMENTATION: remove the horizontal white highlight and draw only the
  title tab above the frame. Its cached first row masks the three upper rows, preserving
  scenery in the rest of that strip without another cache or background redraw.
- CHECKED: tests, 58 PC/TI scenarios, 177 interface screens and 87 blow/replay screens
  pass. Peaks: UI 295,790, AI battle 349,660, defeat 356,348 cycles; all <=360,000.
  Program 65,133 bytes; RAM allocations unchanged. `make review-interface` now writes
  `x/hand_menu.gif` from eight real framebuffer captures over one full animation cycle.

### Shorter, taller glove and round active ring

- TARGET / IMPLEMENTATION (user correction): glove width 18 -> 15 pixels, height 9 -> 12,
  increasing its palm vertically instead of lengthening its index finger. Restore the
  16-wide sprite primitive; reposition it so the existing three-pixel sway still leaves
  one pixel before the text and the full glove fits even the bottom menu row.
- TARGET / IMPLEMENTATION: upper marker ring uses a six-by-six round contour and rounded
  four-by-four opening. Outlined width stays eight pixels; padded height becomes 15.
  Established camera framing stays unchanged, with four pixels of headroom.
- CHECKED: tests, 58 PC/TI scenarios, 177 interface screens and 87 blow/replay screens
  pass. Peaks: UI 296,338, AI battle 349,660, defeat 356,348 cycles. Program 65,109 bytes;
  allocations unchanged. Updated actual-framebuffer animation: `x/hand_menu.gif`.

### FFA title-screen glove, smaller ring and removed terrain readout

- OBSERVED (visual references, no local FFA ROM): Nintendo's Collection of Mana capture
  at https://www.nintendo.com/us/store/products/collection-of-mana-switch/ shows the
  original Game Boy title menu at five times its native pixels, viewport x=240..1039.
  Its glove occupies native x=40..55, y=88..99: short index, rounded thumb and light-grey
  shading along the lower palm. The independent title capture at
  https://ocremix.org/game/115/final-fantasy-adventure-gb/ confirms the contour. Downloads
  remain local in `sources/art/ffadventure/`; neither screenshots nor a ROM are committed.
- TARGET / IMPLEMENTATION: hand pattern transcribed from that reference, with one-pixel
  white outline and padding, 18x14 visible pixels. The detached title-screen ground shadow
  is omitted. A 32-wide ExtGraph sprite preserves its native shape and existing sideways
  animation. Command windows gain four bottom rows and Status entries move up three
  pixels so even the last selected glove stays clear of the screen edge/footer.
- TARGET / IMPLEMENTATION: ring diameter 6 -> 4 pixels, with a two-pixel opening; the
  complete marker returns to 13 padded rows. The terrain readout, builder and cache are
  deleted. Height/depth still drive terrain, movement, water, occlusion and ability reach.
- CHECKED: tests include bare scenery across the entire former readout on dry/wet tiles;
  58 PC/TI scenarios, 177 interface screens and 87 blow/replay screens pass. Peaks:
  UI 293,956, AI battle 347,840, defeat 354,528 cycles. Program 64,525 bytes; allocations
  52,320 + 53,730 bytes. Review: `x/adventure_menu.gif` and `x/m26_interface.png`.

### Canal immersion (milestone 27)

- OBSERVED IN FFT (`make check-water-original`, `tools/water.py`, the reproducible
  `gariland_t1.state`): all 38 canal records have surface 0x0E, floor height 0, depth 1.
  Humans remain at that floor (`misc.screen.vy = 0`), but use the submerged SHP set.
  TYPE1/TYPE2.SHP's first word points to its second 256-offset table; each table's frame
  data starts 0x402 bytes after it. Across six sheets and ten idle/walk poses each, the
  head stays at the same source row and the lower opaque extent loses 6..13 rows:
  Ramza 8..12, Delita 6..11, male Squire 7..13, female Squire 8..11, male Chemist 7..10,
  female Chemist 6..8. The submerged bottom is 4..8 source rows above the anchor.
- OBSERVED IN FFT (live GPU packets): 5,472 complete submitted UV-part sets match the
  disc's normal/submerged pose sets. Each of the 38 actual terrain records is copied
  beneath a visible test unit, paired with depth cleared, for all six sheets; each pair
  uses a fresh state, healthy idle, 24 settling frames and a 26-video-frame probe window.
  This is a controlled display experiment, not 38 walked paths. Off-camera units are
  culled; the shared display scratch can be overwritten by another unit; submitted GPU
  packets lag the current animation record. The checker reads submitted CLUT/page packets
  and matches the whole pose set, requiring a nonempty result for every pair.
- OBSERVED IN FFT (ordinary pad-driven movement, no coordinate/terrain pokes): from
  Ramza's first menu, `CIRCLE:60 CIRCLE:60 UP:30 CIRCLE:100` moves (4,11) -> (5,11).
  His height falls -24 -> 0; the submitted submerged pose first appears after the floor
  landing (video frame 185 of this sequence). Local evidence: `x/m27_original.txt`,
  `x/m27_original_walk.txt`, `x/m27_original_water.png`, `x/m27_original_poses.png`.
- OBSERVED (read in fft_decomp): `battle_gfx_select_unit_shp_frame` switches an ordinary
  human to its submerged set when the surface is at least 12 world units above its feet;
  a raised hop uses the normal set. `battle_gfx_calculate_screen_z_from_input_coords`
  keeps ordinary humans on the floor; Float, Walk on Water, crystals and treasure have
  their own exceptions. Gariland has no deeper canal case to reproduce here.
- TARGET (`unit_in_water`, `draw_unit_water`): fixed-height native cropping at the projected
  depth-1 surface, six pixels above the anchor: draw 15 of the 26 stored rows, hiding the
  lower legs and shadow, with a light-grey edge following the existing contour/cover rule.
  Wet flashes and blades stop at the same surface. Wading stays cropped; hopping into or
  out of water uses the full pose until grounded. Crystals/chests retain their full images.
  The original's separate submerged art and variable limb shapes are adapted to a fixed
  cut; no new sprite bank or per-pixel operation is added. Terrain classification is cached
  with each unit's cover; the dry shader keeps its existing fast path (22 extra RAM bytes).
- TARGET (performance): the active-unit pointer identifies an AI actor while it thinks;
  the tile cursor returns when the plan is shown. This avoids combining its drawing with
  a new turn's clock/order update, uncached camera copy and eleven unit covers. CT order
  uses `ti-table recip --n 51` products instead of repeated divides (numerator <=149,
  table exact through 815); 300 predictions at Speed 1..50 equal full clock advances.
  `compute_reach` skips removed units before marking occupied cells: the new traversal
  fixtures exposed an out-of-range write that the TI binary caught as code corruption.
- CHECKED: unit tests, 52 xcheck doors, 168 water phase screens, 116 UI screens and 87
  blow/replay screens pass. Every ordinary measured frame remains <=360,000 cycles:
  water doors 254,232, AI battle 355,466, defeat replay 356,764, UI doors 316,910.
  Review: `x/m27_water.png` (twelve actual native scenes). Program 60,451 bytes, game
  buffers 104,880 bytes; archived banks unchanged. No new hardware path or emulator run.


## Shoulder inspection exploration (2026-10-15)

- OBSERVED (local MAP022 decoder): 479 unique original mesh vertices, 866 triangles and
  75 supporting planes. All 300 nonvertical faces have upward normals after converting
  PS1 y to height. Original winding supports plane-based backface rejection.
- TARGET: three optional cameras share this mesh and the existing unit/terrain records.
  Inspection pauses tactical updates and RNG while retaining original idle selection.
  Direct Ramza entry skips the opening AI actions rather than simulating their results.
- CHECKED: 936 TI framebuffer checksums equal portable C along the same 312-frame input
  sequence in each camera; tactical-state/RNG conservation, full orbit, bounded lateral
  motion, cache rebuild equality and return to Move pass. Standard FFT tests and the
  existing 60 cross-check doors pass. Timing methodology, limitations and results are
  in PERSPECTIVE.md and PERSPECTIVE_BENCHMARK.csv.
- CHECKED: a conservative block-depth rejection experiment preserved those 936 images
  but reduced measured performance, so it was discarded. The fixed shoulder camera is
  currently the practical choice; continuous original-mesh orbit needs geometry reduction.


## Close fixed shoulder refinement (2026-10-15)

- OBSERVED: the old fixed geometry cache cost 6,706 cycles per warmed frame; sprites
  cost about 944,466. A different floor renderer would not address that dominant cost.
- TARGET: native-resolution polygon background, world-depth masks baked per actor,
  byte-aligned cached idle poses; two temporary RAM backgrounds for opposite directions.
  The viewpoint moves closer (56 -> 40 world units) and F5 selects the opposite angle
  without changing Ramza's tactical facing. Version-3 mesh data adds 451 creases/ledges
  and directional material shades, retaining legacy face shades for the old experiment.
- CHECKED: 384 old/new TI source frames equal portable C, 24 cached/rebuilt street/roof
  images across four facings, preserved tactical/RNG state and a 110-frame return-to-Move
  replay. Old three-mode tests/replays pass. The new 320-frame demo estimates 23.54 FPS
  including cold frames, and 32 FPS warmed (limiter), at 12 MHz with 9% grayscale reserve.
  First forward/reverse frames take 1.43/1.14 s. These are datasheet-emulator estimates.
  The 24 KB pose pool peaks at 23,730 bytes, with no reset; actor masks use 1,582 bytes.
  No extra heap allocation. Details: FIXED_SHOULDER.md and FIXED_BENCHMARK.csv.

### Fixed perspective water / zoom / placement audit

- OBSERVED IN FFT (fresh original run, `make check-placement`): all eleven initial unit
  positions match the disc/deployment definitions and the actual quick-start exploration
  state, with every original CT zero. X is unchanged and PS1 Y is world Z. By the original's
  first Ramza turn all five enemies and Delita have moved; the quick-start deliberately
  skips those turns and retains their initial positions. Evidence is local in
  `x/placement/placements.csv`, `report.json`, `trace.txt` and the original save states.
- OBSERVED (MAP022 mesh/terrain): the 76 visible water triangles are at height 12, above
  the submerged height-zero standing floor. Water must be selected by terrain type/depth
  and surface height, not merely by low polygon height.
- TARGET / CHECKED: MODE toggles close/wide distance 40/80 while F5 retains opposite-side
  inspection; four-phase white canal glints are precomputed from final water visibility
  and drawn behind actors. No additional heap allocation. 512 PC/TI demo frames agree;
  four warmed views reach the estimated 32 FPS limiter, with cold zoom reconstruction
  1.3–1.9 s. Full measurements and sprite reuse discussion: `PERSPECTIVE_WATER.md`.

### Elevated wide inspection and camera cutaway

- TARGET / CHECKED: wide inspection keeps distance 80 but raises eye height 28 -> 76
  world units above Ramza's feet and pitch 5.625 -> 33.75 degrees down. Close framing is
  retained. MODE changes both zoom/pitch, F5 retains them for the opposite direction.
- TARGET / CHECKED: offline version-4 mesh data adds six architectural components;
  five samples through a short camera corridor suppress a nearby obstructing component's
  faces and edges together. Close mode extends 56 units forward beyond Ramza. ALPHA
  restores solid buildings. This is a presentation bounding-box heuristic, not a game
  visibility/collision rule. Real terrain and unit positions/heights remain unchanged.
- CHECKED: 768 source frames match PC/TI, including all four solid/cutaway cameras;
  pitch/toggle/conservation/cache/water/Move tests and sanitizers pass. Four warmed views
  meet the estimated 32 FPS limiter at 12 MHz with 9% grayscale reserve. The 512-frame
  demo including cold construction averages 18.83 FPS. No additional heap allocation or
  new assembly. Before/after GIFs and detailed limits: `PERSPECTIVE_CAMERA.md`.
- CHECKED (normal-build preparation): the initial normal TiEmu launch showed insufficient
  RAM despite passing the headless runner. Omitting fixed-view-inactive isometric rotation
  scratch saves 2,748 bytes of Work; the two game allocations are now 110,358 bytes. The
  standard and legacy three-mode builds retain their original work layouts. The reduced
  normal build then launched successfully on the same clean private Titanium profile.

### Camera-near removal and dotted architectural edges

- TARGET / CHECKED: component state 2 removes geometry at the eye with an eight-unit
  horizontal bounding-box margin; further corridor hits (state 1, margin 20) omit fills
  and retain two-on/two-off black architectural edges. Unaffected components stay solid.
  ALPHA retains the full-solid override. This refines presentation, never game LOS.
- CHECKED: synthetic inside/near/outside/above/distant cases and an actual edge-rendering
  fixture pass; dotted edges respect foreground opaque depth and write no depth. Existing
  tactical/cache/water tests and 768 PC/TI frames pass, as do both sanitizer suites.
  Warmed views still meet the estimated 32 FPS limiter; the 512-frame demo including cold
  construction averages 18.65 FPS. No new asset or heap allocation. Details and GIFs:
  `PERSPECTIVE_DASHED.md`.

### Conservative visible-pixel cutaway (PC review, TI pending)

- TARGET: replace broad component bounds with winning pixel ownership and reciprocal
  depth in small protected screen regions. Keep the close/front left house and the
  wide/back right structure filled; dot the wide/front foreground structure. No entire
  component disappears solely because the camera is nearby. ALPHA (PC: V) cycles
  Auto / Solid / Dotted.
- CHECKED ON PC: real Gariland component regressions, synthetic foreground/background
  samples, localized near clipping and depth-tested dotted edges pass, as do the existing
  fixed-camera/cache/water/state tests and both sanitizer suites. Across 384 captured
  frames, Auto equals Solid outside the HUD in close/front and wide/back, and differs
  in wide/front. Reviewed GIFs and reproduction: `PERSPECTIVE_REVIEW.md`.
- TARGET / UNMEASURED ON TI: temporary 16 KB ownership reuses the pose-pool scratch;
  Auto may require a second cold geometry pass. No added heap allocation or asset change.
  At the user's request this iteration stays PC-only until visual approval and an explicit
  request for calculator validation. Existing TI binary, FPS and size results are historical.

### Original sprite source sampling (PC visual review)

- OBSERVED: perspective already scales the isometric 16x26 bank from FFT's source SPR/SHP
  frames. That bank was reduced to 0.6 scale with LCD-specific outline/face changes before
  perspective enlargement. Direct original-frame sampling retains more close-view detail.
- TARGET / CHECKED ON PC: an offline source-sprite compositor uses actual C-exported poses,
  feet anchors, actor ordering and scenery depth. Compare the current sprites, direct four
  greys and original palette colours across four views. One-screen-pixel white outlines
  remain; the reduced bank's synthetic shadows are omitted. Details: `PERSPECTIVE_SPRITES.md`.
- LIMIT: this is a visual experiment, not a new calculator rendering path. Source art and
  captures stay local; production code/banks are unchanged. No TI build or validation, and
  10 FPS GIF playback is not a measured execution speed.

### Scenery textures in perspective (PC visual review)

- OBSERVED: the user's requested sprites were the ground/building/roof artwork, not the
  characters. The TI isometric banks contain composed views; MAP022's original atlas,
  UVs and palettes are their reusable source. All 866 source triangles match the indexed
  perspective bank, and the review uses the original isometric four-grey thresholds.
- TARGET / CHECKED ON PC: a diagnostic exports exact winning face IDs and edge pixels.
  Offline ray/triangle UV mapping textures those surfaces; the existing C cache then draws
  the unchanged TI-style actors and animations over them with the same depth masks.
  128 diagnostic flat frames match the previous review; 256 textured replay frames cover
  nearest sampling and light filtering. Sky/cutaway gaps and black edges are preserved.
- LIMIT: source transparency retains the flat underlying surface (41/32 filtered pixels
  in forward close/wide views). This is a PC-only visual prototype. The diagnostic is
  rejected for 68000 builds; no TI build or testing and no new performance claims.
  Visuals, reproduction and limitations: `PERSPECTIVE_TEXTURES.md`.

### Low-cost texture contrast (PC review)

- TARGET / CHECKED ON PC: retain filtered source luminance and compare the existing
  four-grey conversion with middle-grey intervals narrowed to 65% and 35%. More source
  highlights become white and shadows black without changing geometry, actor sprites,
  cutaway, edge coverage or the C renderer. The baseline reconstructs byte for byte;
  all cached backgrounds retain the same 6,000 bytes. Two new profiles cover 256 replay
  frames. More extreme texture pixels and identical camera transforms are checked.
- INTERPRETATION: offline conversion adds no runtime contrast pass when one chosen variant
  replaces the existing data. This is independent of the still-unimplemented TI texture
  mapping path; there are no new TI measurements or tests. The 65% variant preserves more
  roof detail than 35%. GIFs and reproduction: `PERSPECTIVE_TEXTURES.md`.

### Grey canal, real low-poly entry/exit, sprite benchmark preparation

- TARGET / CHECKED ON PC: exclude original surface-water materials from texture/contrast
  conversion, restoring the exact old light-grey canal and its C glint animation. All
  128 camera/pose water regions match the old flat-water renders, including sprite occlusion.
- TARGET / CHECKED ON PC: interactive fixed inspection enters/exits through 17 flat-shaded
  half-resolution geometry steps, interpolating camera/projection via an integer smoothstep
  table. Battle updates are frozen and actor poses continue. Existing scratch is reused;
  no heap allocation. A 136-frame real-key replay and stepwise tests preserve units, turn,
  mode, AI and RNG. Fixed-camera, cutaway and legacy perspective PC tests and sanitizers pass.
- PREPARED ON PC: eight matched with/without-sprite camera cases and 1024 checksum frames,
  using the reviewed contrast-plus cached backgrounds. Cold geometry and masks remain
  common; only billboard draws differ. This is cached replay, not a TI texture mapper.
- LIMIT: animation GIFs run at an illustrative cadence. No new TI build or FPS measurement;
  headless timing authorization has been requested under the user's PC-only restriction.
  Reproduction and limits: `PERSPECTIVE_MOTION.md`.

### Intermediate canal, remembered/direct wide entry, smooth zoom

- TARGET / CHECKED ON PC: retain the original water motif but clamp it to light/dark grey;
  white comes from the existing C animated glints. Dry scenery retains contrast-plus.
  Relative to the flat grey version, all image changes stay within the canal, across
  four cameras and 32 poses. This replaces the exact flat-water restoration above.
- TARGET / CHECKED ON PC: DIAMOND opens the remembered zoom; MODE + DIAMOND selects wide
  directly. Entry and exit animate in either mode. MODE animates close/wide distance,
  eye height and pitch without changing view direction. All transitions have 17 low-poly
  steps. Tests cover direct/remembered entry, four facings/both sides, intermediate zoom
  pictures and exact final cache reconstruction. A 544-frame C replay verifies all paths
  and tactical/RNG conservation; fixed/legacy tests and sanitizers pass.
- CORRECTED SCOPE: the requested FPS comparison is textured versus flat scenery with
  characters kept in both. Eight such cases and 1024 PC checksum frames are prepared.
  Warm replay uses the same background copy and actor pipeline; actual texture mapping
  is still offline. No new TI FPS measurement or real-time texture-mapping claim.
  Controls, visual evidence and reproducible targets: `PERSPECTIVE_MOTION.md`.

### Authorized TiEmu FPS comparison: flat/textured scenery, actors in both

- CHECKED IN TIEMU (Titanium HW3, AMS 3.10, configured 360000 cycles/30 ms): close/front
  and wide/back display 32.0 FPS in both variants; close/back displays 28.4 flat / 28.3
  textured; wide/front 28.5 in both. Raw animated render capacity is respectively
  51.5, 36.4, 40.9 and 52.2 FPS, identical to printed precision. Grayscale active;
  32 warm-up frames, 512 raw iterations, 128 displayed frames per case, 256 Hz timer.
- CHECKED IN TIEMU: first-frame construction/cache loading is about 1.7–4.4 s. This
  includes geometry/cutaway, masks and initial poses; the texture map itself is prepared
  on PC. Camera transitions and actual runtime texture mapping are not measured here.
- CHECKED WITH TI-CYCLES: 1024 PC/TI source frames match. Corresponding warmed means
  match exactly: 202906 / 273424 / 247228 / 200579 cycles. Loading a prepared texture
  background adds 36268 cold cycles. A CPU-only capped estimate omits actual display-sync
  waits and therefore cannot replace the displayed TiEmu cadence.
- LIMIT: emulation results, not hardware FPS. Reproduction, binaries, timers, screenshot,
  hashes and CSVs: `SCENERY_FPS.md`. Private emulator/display stopped; user session untouched.
## Playable wide perspective (2026-10-16)

- **OBSERVED (native C / headless 68000):** `FFT_PLAY` routes wide-perspective input through
  the existing tactical update. Move/undo, Attack, Wait/facing/turn change, enemy planning
  and Potion consumption/healing pass 555 differential state/RNG updates. Both RNG states
  matter: FFT's `rng32` is independent of the runtime's `rt_seed`.
- **OBSERVED (TI binary):** 810 consecutive source framebuffer hashes match PC for two
  key replays. Neither sprite motion nor menus rebuild the cached scenery/depth image.
  Mean walking costs are 464k cycles near Ramza's start and 341k farther away; Attack costs
  300k. Move's first range-overlay frame still peaks at 3.24M cycles. These are datasheet
  cycle counts, not TiEmu or physical-device FPS. Full report: `WIDE_PLAY.md`.
- **OBSERVED:** the textured bank represents four specific camera samples. Re-entry at a
  different anchor/facing or a changed cutaway style must use the actual flat geometry.
  Dynamic actor masks need the current direction's depth image, so a cached opposite
  background alone is insufficient when actors move.
- **INTERPRETATION:** a fixed cached perspective supports real tactical actions with modest
  dynamic work. Comfortable whole-battle play still needs visibility/framing decisions and
  an incremental reachable-tile overlay; merely reducing floor resolution does not solve
  those interaction limits.

## Target-centered attack camera (2026-10-16)

- **USER DECISION:** remove free tactical play from wide perspective. Keep normal isometric
  selection/movement; use an automatic perspective shot only after a valid attack confirmation.
- **OBSERVED (PC):** melee and Throw Stone flow through target transition, native attack
  poses/blade or flight cue, hit flash, damage/HP, EXP/JP, then the complete isometric UI.
  Twelve shot tests conserve the original combat's tactical state and both RNGs; entry
  and return freeze action timing, commands are ignored, damage is applied once.
- **OBSERVED (headless TI diagnostic):** 560 frames match PC; 222 native perspective frames
  also match the complete PC game. The diagnostic excludes the full isometric UI. Rebuilding
  geometric transitions cost about 16.7M cycles per frame; the rectangle target transition
  reduces this to one 20.8M/27.4M-cycle preparation under a title card, then 305k/271k mean
  action frames. No new TiEmu or hardware run.
- **LIMIT:** the complete TI game exceeds one variable (73,108 bytes). The complete PC flow
  is ready for review; the headless TI binary measures the real camera/logic, not a complete
  playable calculator package. Initial preparation and full ISO restoration remain material
  timing concerns. See `ATTACK_CAMERA.md` for reproduction, GIF timing and fixture placement.

## Attack-camera visual revision (2026-10-16, latest user steering)

- **USER DECISION:** restore low-poly geometric camera movement; remove the white attack
  title/rectangle transition. Use a close view for melee, wide for ranged, with the original
  ground/wall/roof artwork and the approved contrast treatment.
- **OBSERVED (current PC):** 560 source frames cover two valid confirmations, 17-frame camera
  entries/returns, 188 textured action frames, 12/6 HP loss and the complete isometric menus
  restored. Close distance 48 / eye 40 / pitch 5; ranged fixture distance 96 / eye 84 / pitch 6.
- **IMPLEMENTATION:** two exact target-camera texture records are generated offline from
  winning C face IDs and MAP022 UVs; 12,028 bytes, no depth/rule change. Unmatched shots remain
  flat. This is a review asset bank, not general real-time texture mapping.
- **LIMIT:** earlier TI timings and the 73,108-byte package blocker belong to older revisions;
  the current textured/close geometric flow is PC-reviewed only. Existing measured geometric
  travelling costs remain unresolved. No new TiEmu run. See `ATTACK_CAMERA.md`.


## Reward pace, direct attack zoom and textured turns (2026-10-16)

- **USER DECISION:** double EXP/JP presentation speed, favor direct zoom over attack-camera
  orbiting, and review original scenery textures during F1/F5 rotations.
- **OBSERVED (PC / headless TI):** rewarded actions last 70 rather than 94 updates; hit timing,
  reward values, damage and RNG are unchanged. ISO reward labels last 9 rather than 18 updates; cinematic slots last 11/14 rather than
  22/27 updates.
  All existing tests and twelve shot tests pass. 560 diagnostic TI framebuffer hashes match
  PC, including 208 complete-game camera frames. No TiEmu or hardware run.
- **FIX:** the view/world transforms require ISO yaw `56 - 16*rot`, not `56 + 16*rot`.
  The old sign reversed orientations 1/3. A mesh/ISO axis alignment regression covers all
  four views. Attack framing now preserves yaw unless separation needs adjustment; the
  correction is capped at eight 64th-turn units (45 degrees).
- **OBSERVED (PC visual feasibility):** 160 real F1/F5 input/state frames, including 24
  textured intermediate mesh views. Four clockwise turns and four reverse turns restore
  the starting orientation; units, RNG and tactical mode remain unchanged. Original UVs,
  grey canal textures and outlined TI actor pixels survive. No rotation-image bank is
  incorporated into the game. See `ROTATION_TEXTURES.md` for limitations.
- **LIMIT:** headless attack-camera geometry still costs roughly 15M cycles per travelling
  sample. These measurements do not establish smooth calculator playback. Offline F1/F5
  texturing demonstrates appearance only; runtime affine mapping and its TI cost remain
  separate work. Current diagnostic numbers are in `ATTACK_CAMERA.md`.


## Short cinematic attacks (2026-10-16)

- **USER DECISION:** remove about three seconds from the approximately five-second PC
  cinematic preview by shortening transitions and damage presentation.
- **IMPLEMENTATION:** entry/return sample the same eased geometric path at 0/4/8/12/16
  rather than all 17 indices. Cinematic action runs two consecutive original combat
  updates per displayed frame and stops on the first exit from `M_ACT`. No logical
  impact/reward/KO update is skipped; ordinary ISO and manual inspection timing is intact.
- **OBSERVED (PC):** twelve differential shot tests conserve complete tactical state and
  both RNGs, with a single HP change, locked input and automatic camera restoration.
  Rewarded shots use 5+35+5 = 45 display frames rather than 17+70+17 = 104: 2.25 versus
  5.2 seconds at the 20 FPS GIF cadence, saving 2.95 seconds. Damage/Miss lasts twelve
  displayed frames rather than twenty-four. ASan/UBSan pass.
- **OBSERVED (headless TI):** 560 diagnostic framebuffer hashes match PC, including all
  90 current camera frames from the complete PC game. Total cinematic work falls from
  571.3M to 191.1M cycles in melee and 569.1M to 183.0M ranged (66.6%/67.9% less). The
  diagnostic omits the ISO UI and is 45,625 bytes. This is not a full TI release.
- **LIMIT:** this count describes the PC preview cadence. Fewer geometry rebuilds reduce
  TI work, but do not establish a smooth 2.25-second hardware animation. No TiEmu run.


## Textured motion and gentler transitions (2026-10-16)

- **USER DECISION:** slow camera motion slightly, retain the faster attack/damage, and use
  the original ground/wall/roof art throughout all transitions.
- **IMPLEMENTATION (PC reference):** seven eased cinematic samples per direction rather
  than five, giving 7+35+7 = 49 display frames / 2.45 seconds at the review cadence. A
  general C filtered-UV sampler now handles geometric entry/exit, both close/wide zoom
  directions, settled perspective and F1/F5 turns in the full PC game. It uses the original
  138,532-byte atlas/UV/luminance bank, not a bank of full-screen camera images.
- **OBSERVED (PC):** 754 real-key frames cover both attacks and both entry presets with
  three zoom changes, exit and F5/F1. Original unit records, rules and both RNGs survive.
  Twelve differential shot tests pass with textures; eight additional quarter-turns cover
  all four orientations and return to the start. Native melee pixels match the preceding
  filtered endpoint; ranged differs at twelve quantized luminance pixels. ASan/UBSan pass.
- **OBSERVED (headless TI, separate portable build):** 560 framebuffer matches, including
  98 cinematic frames. Current diagnostic is 45,687 bytes; total cinematic geometry/actor
  work is 248.9M cycles melee / 245.0M ranged. This excludes the general PC texture sampler.
- **LIMIT:** the new reference sampler uses doubles and filtered per-pixel sampling on PC;
  a compile-time guard rejects TI inclusion. Its appearance is ready for visual review,
  but a fast fixed-point TI texture mapper and a complete TI package remain outstanding.
  No TiEmu run. Reproduction and scope: `ATTACK_CAMERA.md`.


## Half-second cinematic motion and ISO height contours (2026-10-16)

- **USER DECISION:** use approximately 0.5-second cinematic transitions; try the perspective
  contrast treatment on ISO scenery, and mark height boundaries black on light / white
  on dark areas for easier reading.
- **IMPLEMENTATION:** ten eased cinematic samples per direction; the 35-frame accelerated
  action stays intact. Preview duration is 10+35+10 = 55 frames / 2.75 seconds at 20 FPS.
- **OBSERVED (offline art):** four original colour images reproduced byte-for-byte before
  conversion. Original ISO canal tones retained. A 302-edge geometric candidate set selects
  top/slope-to-wall boundaries, excludes coplanar triangulation seams, and is visibility
  tested against the original supersampled mesh depth. Each orientation has 2,328–2,908
  one-pixel strokes, using black/white according to underlying tone.
- **OBSERVED (full PC game):** 432 frames compare original, contrast, and contrast+contours
  under identical inputs, actor state, RNG and camera logs. All four original occlusion
  planes are checked on every settled frame. `[8]` cycles the three visual variants; the
  contour variant is the preview default. Character sprites and tactical rules are unchanged.
- **IMPLEMENTATION:** offline colour variants load only when changing the ISO view; no edge
  detection/drawing each frame. A production asset build can substitute the selected colour
  planes before packing and retain the original view count, raw size and all depth data.
  The 139,528-byte two-style comparison bank is PC-only and leaves original files intact.
- **LIMIT:** visual trial, no TiEmu run, no newly claimed TI style performance. See
  `ISOMETRIC_STYLE.md` and `ATTACK_CAMERA.md` for reproduction and scope.

- **VALIDATION:** the ordinary game suite, twelve texture-enabled cinematic attacks and
  the 560-frame portable PC/TI diagnostic pass. ASan/UBSan additionally exercise the ISO
  contour replay. The host PNG writer now avoids a zero-length `fwrite(NULL,...)` for IEND;
  PNG bytes remain unchanged. This small host-only fix does not touch the TI backend.


## Selected ISO style, slower turns and attacker rewards (2026-10-16)

- **USER DECISION:** select contrast + contours; double F1/F5 rotation duration; place
  cinematic EXP/JP beside the attacking unit; quantify contour storage and CPU cost.
- **IMPLEMENTATION:** PC textured F1/F5 uses six intermediate angles rather than three.
  The camera/state loop advances through those frames; this is not a GIF timing trick.
  Original portable/TI turn timing is unchanged. Cinematic EXP/JP anchors to `st.atk`'s
  projected rectangle with viewport clipping; damage numbers remain associated with target.
- **OBSERVED (PC):** twelve shot tests preserve combat state and both RNGs, including
  reward-notice independence from target motion and dependence on attacker position. Both
  inspection replays assert six F5 and six F1 moving frames. Current PC GIFs come from the
  actual game. The portable TI diagnostic still matches 560 PC framebuffers.
- **OBSERVED (ZX0):** four original views total 38,869 compressed bytes; contrast 39,108;
  contrast+contours 41,505. Contours alone add 2,397 bytes; combined style adds 2,636 versus
  original. Raw sizes and all occlusion data are unchanged; all eight new streams round-trip.
- **OBSERVED (headless 68000):** all twelve style/view decoder sizes and checksums match.
  Mean original/contrast/contour load costs are 2,187,966.5 / 2,168,873 / 2,189,089.5 cycles.
  At 12 MHz, contours add 1.685 ms per view load over contrast, 0.094 ms over original.
- **INTERPRETATION:** baking selected contours into the four existing colour images needs
  no additional production render-buffer RAM and no contour computation each frame. The
  larger PC multi-style comparison bank is separate review scaffolding. No TiEmu/hardware
  timing claim; full data and reproduction in `ISOMETRIC_STYLE.md`.
# Native camera audit (2026-10-16)

**OBSERVED — actual TiEmu Titanium timer, scenario 65/66:** F1/F5 hold/decode/switch
0.386/0.398 s; ISO to close/wide 28.441/28.570 s; both zoom directions 18.796 s;
wide return 27.257 s; full melee/ranged cinematic 36.292/36.808 s. Prepared fixed
views animate at 28.4/32.0 FPS after warmup, but first preparation costs 1.582/1.605 s.
The shared case driver passes 468 PC/TI framebuffer checks and all completion/HP checks.

**IMPLEMENTATION GAP:** the approved moving textures are PC-only. The native path uses
flat 80x50 travelling geometry and exact offline-textured endpoints; native F1/F5 has
no intermediate rotation. The full PC texture sampler has not been ported to TI.

**RECOMMENDATION:** retain ISO contrast/contours and cheap view switches; keep the current
smooth perspective travel and automatic cinematics off for a production TI build.
Fixed-view animation FPS does not establish usable camera latency. The audit leaves the
experimental defaults unchanged. Details/evidence: `CAMERA_AUDIT.md`, `x/camera-audit/`.

## Auto-battle controls — 2026-10-18

**OBSERVED IN FFT (manual/source study, not a new running-game oracle):** the PS1
manual lists Manual, Fight for Life, Protect Allies, Save Fading Life and Run Like a
Rabbit. The local `battle_menu_store_auto_battle_main_target_id.c` stores behavior
and target per unit; `battle_ai_init_target_consideration.c` keeps player-unit orders
separate from team affiliation. Source URL and adaptation limits: `AUTO_BATTLE.md`.

**TARGET ADAPTATION:** this unit / all allies delegate to our existing general AI;
Manual or Shift/ESC restores player control. Existing Move/Act, CT, inventory,
damage and reward rules apply. No specialized-order fidelity is claimed.

## Opening / Orbonne (2026-10-18)

**VERIFIED, PC / HEADLESS 68000 (initial tone correction)** — Map 56's former camera-0 percentile thresholds
bleached 71.1% of the initial camera's lit scenery to white. Per-view 1/50/96
percentiles after mild offline filtering reduce this to 4.0%. Black cutaway
sides and sky do not calibrate the thresholds. All four depth/mask layers,
terrain/collision and actor banks remain unchanged. Ten replay states and
ordinary average cycle counts match the previous build; packed view storage
changes by only +3 bytes. Details: `opening/README.md` and `opening/BENCHMARK.md`.

**VERIFIED, PC / HEADLESS 68000 (presentation follow-up)** — Gariland's same
`ledges`/`outline` pass is baked into the four Orbonne views. Depths/masks remain
byte-identical. The former render-order pose quota permanently starved some
visible actors. Walking/action priority plus a round-robin idle queue, shared
decoded records and actual miss/copy budget charges retain the one-decode limit.
All 11 idle actors and 648 exact displayed walking poses pass the PC controls;
the native replay retains PC pixel/state agreement. Cold target panels take
5–12 frames instead of 45, and their opacity mask now uses the trimmed portrait
index. The actor bank is unchanged. Archive growth is 4,421 bytes; RAM +2 bytes.
Neither `FFT_CONFIG` nor `FFT_EXPLORATION` is compiled, so there is no idle
perspective prefetch to remove from this build.

**OBSERVED / ADAPTATION** — Map-56 source animation row 0 uses mode 4, palette
row 5, three frames, duration 15; the original active runtime row agrees.
Surface 0x11 has 20 depth-four water cells, missed by the earlier 0x0E-only
check. The original pathfinder rejects depth >=4 for sinking human units
(`battle_move_set_tile_flags_for_pathfinding.c`). These cells are now blocked.
The native visual adaptation uses sparse four-phase glints on visible palette-5
pixels, not the original palette sequence. Even-x/2,y coordinates occupy 1,948
archived bytes and are read in place. No depth-one immersion is required here.

**OBSERVED** — `tools/opening_oracle.py --story` cold-boots the US disc without
writing EVENT/NEXT. Event 2 uses chapel map 62; events 3–6 use map 56 / ENTD
0x183. Event 5 is the first combat. Only the player-control flag is cleared
there to let the original AI finish for observation. Active text-window message
IDs/actor placements establish 37 distinct dialogue beats through event 6.
Generated reference states, frames and text remain local.

**OBSERVED** — Orbonne's Ramza is battle slot 0, level 9, character identity 2.
Agrias is slot 1/job 0x34, Gafgarion slot 2/job 0x17. Their resolved SPR IDs
are 2, 52, 23; the latter reads H61.SPR, not the other Gafgarion sheet. Eleven
unit records' HP/MP/SP/PA/MA/Move/Jump agree with the common stat model.

**OBSERVED** — Raw stat fields at unit +0x72 have already grown to the source
unit's level. Gariland's level-1 fixture did not expose this. The asset tool
inverts each `v += v/(growth+level-1)` step exactly (asserted per step) before
feeding our base-stat/level model. Applying growth again to source level-9
Ramza overestimated his HP (197 rather than the observed 154).

**OBSERVED** — Both maps contain 140 tiles, so second-layer terrain addressing
must retain the original fixed 256-tile stride. Map 56 has no depth-one canals;
its depth-four water is described above. All four
mesh projections end at y=190 with the existing native origin. Removing rows
192–211 preserves the bottom of the mesh and saves 6,400 buffer bytes. Tall
upper decoration is still outside the fixed origin; this is not a renderer of
all map-56 tower heights.

**TARGET / ADAPTATION** — `opening/` shares `fft.c` instead of forking its battle
logic. The chapel uses one offline source-mesh cutaway and six interpolated
actors, with the common interior-eroded silhouette rule. The abduction uses
compact native staging. Bow/Charge presentation, extra sword effects/statuses
and AI decisions remain adaptations (opening README), not measured claims of
full original fidelity. The Academy/campaign handoff remains future work.

**VERIFIED, HEADLESS 68000** — The full adapted encounter reaches victory without
forcing HP or outcome, then the original ending dialogue sequence. Ten PC/TI
pixel/state checkpoints agree. `opening/BENCHMARK.md` separates actual native
CPU costs, decode/turn stalls, and illustrative PC GIF playback. The preceding
softened-map packed build was cold-launched in Titanium TiEmu and handed back
at Ramza's turn; these later corrections have not replaced that live session.


### OBSERVED / TARGET: opening review, 2026-10-18

The source camera matrix at Orbonne has screen X (+x,-z) and screen Y
(-x,-z,+PS1-y). Our old equivalent-front projection reflected X. This was a
camera-handedness defect, despite the earlier correct tile-coordinate audit.
The active opening/campaign projections, masks, marker slopes, facings and
turns are corrected together. Historical research fixtures remain controls.

OBSERVED: the US Charge table at 0x80061044 is (4,1),(5,2),(6,3),(8,4),
(10,5),(14,7),(20,10),(35,20): CT ticks, then primary-stat bonus. Potion's
item ability 0x170 has Y=3; Hi-Potion 0x171 has Y=7. Source functions and
native targeting/clock choices are documented in `opening/BUGFIXES.md`.

OBSERVED: chapel actors use event frames >=210 from shared event artwork,
not TYPE1/TYPE2 indices. `tools/opening_scene_oracle.py` records original
VRAM parts/pages and actor facings without event or victory writes, through
Academy event 8 (map 24). Source KO bodies are still observable after Orbonne;
the native story omits death-counter overlays. TARGET: a fixed shot, source
poses and text, a short native punch/collapse/Chocobo substitute for movie
choreography, then a separately loaded Academy/Gariland chapter. The rear
shot draws actors in the foreground because its static battle mesh lacks
original event visibility changes; tactical terrain occlusion is unchanged.


## Title/QA follow-up,2026-10-23

**OBSERVED IN THE NATIVE ADAPTATION:** a private Titanium AMS3.10 reproduces the plain-title campaign group stopping with Missing data or memory after Orbonne chapel. The matching packed-child candidate reaches Agrias dialogue with the same normal New Game/Data2 inputs; a second Continue/Data2 invocation resumes Orbonne. The title writes both slot and selector through AMS after teardown. This is a bounded startup/save check, not a complete campaign or hardware FPS measurement.

**TARGET IMPLEMENTATION:** title A reuses existing logo/map with a brighter centre; B uses the existing serif face on black, title then rule then68k, any-key skip consumed before enabling choices. Shared bare choices/file selection; no extra actor sprites. Chapter/results reuse one left-to-right strip reveal. Source-order result drawing is gated until aftermath completes; Academy pan clamps to target; shortcut99 retains deployment/Gariland dialogue/map. Scenery1px/2px and title A/B remain visual decisions. Campaign close/wide perspective and B06/B07 ghosts remain open. Details: `campaign/QA_TITLE_FOLLOWUP.md`.


## Canonical native battle cells,2026-10-24

**TARGET / NATIVE-CHECKED ADAPTATION:** units, cursor, paths, movement undo,
Charge binding/interruption, knockback/falls and AI hypothetical placement carry
compact node identity through actual battle logic.128 checks on an asymmetric
synthetic two-floor graph agree PC/sanitizers/68000. This is not an OBSERVED PS1
upper-floor movement, line-of-sight or bridge-cover claim. Existing169 reference
pictures remain identical; source floor/render and full256-node battle-count
migration remain open. FMB2 validates seven code images and explicit ABI;68
malformed/valid-bank checks pass. See `campaign/CORE_CELL_STATE.md` for limits,
111320-byte requested battle heap formula and465566-byte private package.


## Full256-node native tactical envelope,2026-10-24

**TARGET / NATIVE-CHECKED ADAPTATION:** actual battle state/search/AI/selection
accept256 compact nodes and13 actors; node255 is valid across floors. Counts,
AI endpoints, range completion and counting-sort offsets use16 bits.3181
PC/sanitized/68000 checks include the full envelope, frozen AI order, Charge,
failed cache growth and ownership.169 existing images remain identical.
Topology uses a separate48-byte-per-node cache; the pathological AI setup zone
is about9.25× faster with merge sorting and unchanged ties. This is not an
OBSERVED source upper-floor traversal/LoS/cover claim. Full scene and combined
memory gates remain open. See `campaign/CORE_FULL_GRID.md` for exact layouts,
116090-byte current Gariland heap formula and466067-byte private package.


## Dual Archer Charge,2026-10-26

**OBSERVED (isolated unchanged MIPS):** `tools/charge_hands_oracle.py` executes
eight Charge menu initializations/used-hand selections and sixty scalar cases.
The tested Two Swords pair gives two strikes; Charge power enters each hand's
base XA before target statuses/zodiac. Critical threshold is4%, then amplitude
adds floor(roll*XA/32768). A zero-amplitude critical still calls recoil. Source
RNG helper values are supplied by the fixture; recoil is isolated/counted, and
the fixture multiplies original XA/YA. This is not an observed full delayed HP/
target/terrain/animation trace. Function/disc/input hashes are in
`campaign/x/core-charge-hands/oracle/source.json`.

**TARGET / NATIVE-CHECKED ADAPTATION:** all eight powers use independent hands,
both delayed weapon IDs, fixed-tile requery after recoil/KO, one root reward and
post-primary Counter/Auto-Potion.586 PC/sanitizer/68000 checks pass. Shared
forecast/AI retains its potential-hit approximation; source range/projectile/
layered target completeness remains open. ABI18/twelve images; no State/Work/
Unit/FSV3 growth. Battle randomness is rng32, distinct from runtime rt_seed; the
new fixture checks pure queries and the legitimate existing AI chooser draw.
See `campaign/CORE_CHARGE_HANDS.md` for storage/timing/source limits.


## Due-wave order and target identity,2026-10-26

**OBSERVED (isolated unchanged MIPS):** `tools/deferred_oracle.py` runs six
between-turn sequences and four attacker-coordinate cases. Ready turns precede
the next decrement; simultaneous charges return in slot order before expiry/
next CT, Stop holds its countdown and a later cancelled record is skipped.
CT255/Quick can interrupt the due wave. Expiry/upkeep callbacks are isolated;
cancellation/Quick mutations are injected between calls. Original target-kind5
uses stored coordinates while kind6 reads the selected unit's current location.
This is not original full HP/MP/area/interruption/target-validity certification.
Provenance and function hashes: `campaign/x/core-deferred/oracle/source.json`.

**TARGET / NATIVE-CHECKED ADAPTATION:** iterative due Charge dispatch replaces
recursive turn entry for empty/invalid records, and busy roots protect all
pending/request/RNG state. Thirteen roots or empty records share one clock phase;
late caster invalidation, Counter ownership and terminal priority are covered.
ABI18/types unchanged. Magic/MP/AoE, Quick255 priority and all-permanent-freeze
clock liveness remain open; unit-following targeting is not enabled yet. See
`campaign/CORE_DEFERRED.md`.


## Casting MP and preparation policy,2026-10-27

**OBSERVED (unchanged isolated MIPS):** `tools/casting_oracle.py` records33 MP
and12 preparation cases, plus retail Holy arming at999 MP then a separate
resolution payment to943. Six CT/action-used are set without movement-used or
MP subtraction on arm. Default-menu non-Mime execution uses range MP cost;
Half MP rounds down. Preview checks affordability/records cost without debiting.
Silence/Frog/Don't Act reject before payment. Short Charge rounds odd half up;
Non-Charge then zeroes delay; persevering actions ignore both. External status
handlers are isolated, dry/unmounted; no elapsed CT/effect/target trace is
certified. No source job among160 has innate Half MP. Provenance/hashes:
`campaign/x/core-casting/oracle/source.json`.

**TARGET / NATIVE-CHECKED ADAPTATION:** pure resident cost callback shared by
player/forecast/AI; actual COMMAND pays once before effect start, with deferred
Charge arming returning before payment. Whole-State invalid-order purity differs
from original resolution-entry bookkeeping; preview scratch may change while
world resources/RNG do not. A private Dash cost7/Silence flag is a synthetic
carrier, not retail behavior. No new Holy/Fire producer/formula/area/caster is
enabled. Cold range construction moves unchanged to COMMAND2; ABI19/twelve
images. See `campaign/CORE_CASTING.md` for evidence and packing/remaining limits.


## Incantation lifecycle,2026-10-27

**OBSERVED (unchanged isolated MIPS):** `tools/incantation_oracle.py` checks four
action-initializer payment/binding sequences (empty tile, living/KO/absent unit)
and twelve Charge-only recoil predicates. The initializer pays56 MP before
strike initialization and coordinate binding; strike initialization and external
status handlers are isolated. No full target-list/effect/refund/presentation
trace is certified. Recoil cancellation applies to Charge skillset8, not White
Magic10; movement/equipment mutations do not make the predicate reject ordinary
casts. Hashes/provenance: `campaign/x/core-incantation/oracle/source.json`.

**TARGET / NATIVE-CHECKED ADAPTATION:** eight-byte pending records distinguish
fixed tiles and live actor slots, including node255 and layer changes. Actual
CT/menus, source support timing, caster displacement/weapon survival, late MP/
status rejection,13 due casts and Auto-Potion/root settlement pass. Empty/KO/
absent centres pay and skip effects/rewards; full empty-spell presentation is
still future. Synthetic carriers certify the lifecycle, not retail Holy/Fire
formulas/traits/areas. Native exact-cost effects now use a post-payment evaluator,
fixing the preceding resource slice's hidden second affordability check. ABI20/
ModCtx344; gameplay/save types unchanged. See `campaign/CORE_INCANTATION.md`.


## Holy scalar / formula08 (2026-10-27)

**OBSERVED:** `tools/magic_oracle.py` runs the unchanged original Holy damage
pipeline on104 ordinary-human profiles, with explicit normal-weather and zero-
element-trait conditions. It records per-step modifier rounding, zodiac/Faith/
Innocent behavior and actual MEV shield/Defend bypass. Holy flags4=0x40 lacks
magic-evasion bit0x02; Fire flags4=0x42 differs. Preview consumes no RNG; execution
calls the generic proc roll once with(100,19). This is not a full target-list/
HP-commit/display/cap or monster/element/evasion certificate. Provenance:
`campaign/x/core-magic/oracle/source.json`.

**TARGET, implemented:** pure resident formula08 callback, shared native AI/
prediction and a private normalized Holy through vulnerable cast/payment/impact/
Sleep wake/Auto-Potion/terminal settlement.1352 PC/sanitizer/68000 checks pass
(4DCFDD40); production Holy/Fire and every AF_CAST producer remain disabled.
Canonical Shell/Frog/Chicken/Faith/Innocent scalar reads do not enable their
other effects or producers. ABI21/ModCtx348; battle/save allocations unchanged.
Private group493049 (+503), input64919/headroom17. Cold banner/cache setup uses
PARTY7/8; frequent end-turn CT stays resident. Details: campaign/CORE_MAGIC.md.

**INTERPRETATION:** single-target Holy isolates non-evadeable magical damage
without conflating Fire's target policy. Element/target eligibility, magic
evasion, areas and reviewed casting/empty-centre presentation are the next
separate gates. No general playable-magic or full-core claim follows.


## Elemental trait producers and stat images (2026-10-27)

**OBSERVED:**178 executions of original job-copy/equipment loops match four job
masks plus zero Strengthen, then item-mask OR. All160 internal job bases,
13 distinct nonzero item masks and overlapping/duplicate equipment are covered;
these are20 generic playable careers plus other templates, not160 careers.
Actual-stat arithmetic is isolated, ordinary-human flags are used, and arbitrary
fixture gear does not prove legal equipment or monster policy. Provenance:
campaign/x/core-affinity/oracle/source.json. Source formula reading distinguishes
absorption/healing from nullification; full typed effect traces remain next.

**TARGET, implemented:** FCJ9/FEA1 stores416 references to32 shared masks in582
bytes and rejects old/malformed data. A65-byte cache outside Unit is rebuilt by
existing battle/party/stat producers; native State adds66 bytes with alignment,
Unit/Work/FSV3 stay unchanged.1881 PC/sanitizer/native assertions C0B66CFB pass.
Native stats are local in PARTY/ACTION/RESPONSE; no evicting callback remains.
Resident pointer entry uses PARTY9/transient input. ABI22/ModCtx348/signature
61226, largest6060, private495758 (+2709), input64859/headroom77. No new asm.
Details: campaign/CORE_AFFINITY.md.

**INTERPRETATION:** This is a prerequisite, not active elemental damage or
absorption. A typed outcome must survive forecast/AI/impact/response packets;
source narrowing/caps need proof before widening the former magic bounds.
Production Holy/Fire stay disabled. Cached data and unchanged pictures do not
certify monster equipment, status traits, frame budget or combined residency.


## Elemental formula08 outcomes and signed HP (2026-10-27)

**OBSERVED:**77 profiles execute unchanged original formula08/finalizer and the
HP/MP commit prefix. Strengthen precedes support/status/zodiac; signed halfword
narrowing precedes Half/Weak/Faith, and only positive quantities cap at999.
Nullification wins over absorption and skips the proc draw; absorbed results
use the healing category. Source signed HP subtraction/addition can reverse
actual HP direction at overflow boundaries. Reaction/MP split are isolated and
commit stops before stat/status/KO/EXP: no full spell trace is claimed. Other
than real Holy50/element2/no-evasion, modified power/element carriers are synthetic.
Provenance: campaign/x/core-elements/oracle/source.json.

**TARGET, implemented:** pure cached classification/scalar, signed HP transform
shared by forecast/AI/impact, prepared effect_kind through response packets,
paid nullification without effect RNG/reward, normal absorption without damage-
only wake/Auto-Potion.1525 PC/sanitizer/native checks92063E01 pass. Both category
bytes occupy former padding: State2190/ActionPacket22/Unit112/Work45516/FSV3
unchanged. ABI23/ModCtx352/signature61487, zone6096; FORMULA10 owns cold format.
Private496819 (+1061), input64795/headroom141; all28 data banks unchanged.
The old magic1352 fixture now expects positive999 finalization (B5895FED), while
its104 raw scalar profiles retain source values. Details: campaign/CORE_ELEMENTS.md.

**INTERPRETATION:** Event category and signed HP direction are distinct. Cache
or formula support does not imply magic evasion, full target eligibility,
weather/Oil/Float/Undead/monster policy, areas or reviewed spell presentation.
Retail Holy/Fire stay disabled; no bulk-content or full-core certificate follows.


## Magical evasion and ordinary formula08 final validity (2026-10-27)

**OBSERVED:**238 original-MIPS profiles in
`campaign/x/core-magic-evasion/oracle/source.json` execute magical evasion,
formula08, presence predicate and finalizer with source equipment secondary
records. Evasion channels are accessory/right shield/left shield/class0;
even zero channels roll, in order. Holy bypasses, Fire does not. Defend/Abandon
halve base100, Transparent/no-evasion statuses zero channels. Percentages multiply
before final division. Abandon's water exception is canonical byte2&0x46.

**OBSERVED:** Crystal/Treasure/Jump are absent from the target-presence query;
Dead/Petrify/Wall are present and ordinarily rejected after formula/proc draws.
The external pass/fail helper is deterministic with exact source scaling; normal
weather/unmounted humans, graphical target sorting isolated. No complete original
HP/status/reaction/reward/area/presentation trace is claimed.

**TARGET, implemented:** resident pure percentage/target callbacks shared by
FORMULA/AI/COMMAND; source sequential draws only at execution. Native late due
invalidation, one payment, no-effect misses and AI rejection pass7510 PC/
sanitizer/68000 assertions (995C7993). FCJA stores six-bit MEV plus shield tag in
Item.pad, preserving HAND_DUAL; source maximum50, all256 items checked, raw
catalogue41086 unchanged. Old FCJ9/ABI23 reject. ABI24/ModCtx360/signature61752,
zone6146. Cold RESULTS1 row labels cost45716 below the60k task budget; callbacks
cannot evict their executing image. Private498763 (+1944), input64899/headroom37;
Unit112/State2190/Work45516/FSV3/five allocations unchanged. Retail spells remain
unavailable; next weather/Oil/Float and area/presentation work. Jump deferred.


## Formula08 weather, Oil ordering and Float (2026-10-27)

**OBSERVED:**254 original-MIPS profiles execute effective weather, formula08,
status filtering, finalizer and signed HP-prefix, with supplied script weather/
proc helpers and isolated reaction/MP-split paths. Calendar weather is clamped
0..4; indoor2 disables, snow1 adds3 for raw>=2. Weather modifies the signed HP
halfword sequentially (storm: fire3/4 then lightning5/4; snowstorm: ice5/4), before
Half/Weak/Faith and positive999 caps. Source function/disc hashes and inputs are
in campaign/x/core-weather/oracle/source.json.

**OBSERVED:**Oil doubles XA after the HP product, so formula08 damage is unchanged.
It prepares protected mutable Oil removal; innate Oil is retained. Float nullifies
earth; a source null packet can retain a removal mask, but commit returns on miss.
Preview attack type136 is a bitfield (HP damage plus status). The original live
status/KO/reaction/EXP tail is not newly certified by the HP-prefix oracle.

**TARGET, implemented:**weather byte/shared query/AI/command math, protected latched
Oil removal bit, pure forecasts, current due-climate and immutable prepared damage,
actual Auto-Potion packet transport.6493 PC/sanitizer/68000 checks8685AD66, four
forecast cache hashes and targeted prior closure. Native State2192(+2), Unit112/
Work45516/packet22/FSV3 stable; five battle allocations, first5330. ABI25/ModCtx364/
signature62019/zone6172. FORMULA11 owns unchanged forecast-cache drawing with pure
resident width callback. Private499042 (+279), input64735/headroom201;28 non-code
banks unchanged. Full cold forecast reaches92032 cycles in the fixture, beyond
its old60k HUD estimate; complete-frame cold-work accounting stays open in C07/L4.
Live climate/calendar/GNS, bow/movement weather and Oil/Float infliction/movement
producers remain gated. Next source areas/per-target effects/reactions/rewards
and reviewed casting presentation. Jump remains low priority.


## Ordinary radial area footprints (2026-10-27)

**OBSERVED:**79 original-MIPS green-panel/list profiles and42 vertical boundaries
cover Manhattan/whole-map, nearest-layer/tie-lower/highest-only, hidden upper
surfaces, raw water/half-height, u8 reference/signed16 candidates/-255 blocked
sentinel, unit-follow centres and panel team-filter overlap quirks. Whole-map255
bypasses vertical; KO/Petrify/Wall are present, Crystal/Treasure/Jump absent.
Follow-centre coordinates are still read for an absent target. Raw ability flags
occupy offsets3/4/5/6. Source lists ascend slots; ordinary strike resets nearest
sorting and prepares every formula before HP commits. Inputs/hashes/source lists:
campaign/x/core-targets/oracle/source.json. These are synthetic actors, with two
real terrain maps, not complete encounter/reaction/reward traces.

**TARGET, implemented:**pure formula08 radial mask cast_targets(), stable native
IDs, explicit invalid/unsupported, compact tile255, no resource/RNG/State changes,
one-floor upper-read fast path.1193 PC/sanitizer/68000 checksFC02B091; cold query
max40642, warm max28578 cycles. New TARGETS1564-byte image, ABI26/ModCtx364/
signature62276/largest6172; State/save/Work unchanged. Private500606(+1564), raw
bank62070, input64749/headroom187,28 non-code banks unchanged. Broader source
slot mapping, batch formula preparation/one-cost commit/reactions/rewards and
reviewed presentation remain next; live spells and Charm/mount/special shapes
remain gated. No full frame/stack/residency closure; Jump deferred.


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

## User-selected AI pause (2026-10-28)

**TARGET, implemented:** CORE_AI_PACING.md includes thinking in a1.5s elapsed
256Hz minimum, preserving the frozen picture. It no longer adds48 frames after
planning; a plan taking longer than the deadline adds no further full pause.
The user explicitly accepts the measured roughly1s setup inside this pause;
do not treat that quantum alone as a30fps production blocker.171 PC/sanitizer/
68000 checks58CBDE4E cover injected elapsed time/wrap/replan/cancel/frozen planes.
No original-game timing claim is added. All fourteen images/ABI32/layouts and
five heap requests remain unchanged; resident static+2 bytes, input64467 (+20),
headroom469. Private509418(-24 compressed), only ffbcore changes. Total worst
plan latency and remaining RAM/stack/residency/interactive-frame gates stay open.

## Private area selection UI candidate (2026-10-28)

**STATIC original evidence:** targeting_range accepts marked panels, then
select_tile enters ability_preview with marked targets. action_confirm chooses
occupied/empty and UNIT_OR_TILE/TILE_ONLY variants. confirm_action stores a fixed
panel(type5) or stable unit ID(type6/7), with tile fallback for absent occupants.
set_panels_for_action reads secondary flags_2 NO_FOLLOW_TARGET0x10: existing raw
range byte4. These are bounded code readings, not new original UI run traces.

**OBSERVED, native integration2026-10-28:** user approved the UI candidate.
CORE_AREA_UI.md now records actual shared range/list/Unit-Panel ordering.499
PC/sanitizer/native checks67EED097, eight HUD hashes and four full tactical replay
frames/states agree. This is synthetic supported spell activation, not an original
encounter replay. AreaUi228 reuses idle ai_dist256; Work/State/save/five allocations
remain fixed. New AREA_UI image4096, ABI33/ModCtx372/15 images/zone6264. Resident
loads TARGETS/COMMAND only after UI returns. The unchanged AI camera is in
AI_SETUP3/4;1321 PC/sanitizer/native checks4CC4400E agree. Private515564(+6146),
resident64719/headroom217,28 data banks unchanged. Cold synthetic13-target HUD
375252 cycles leaves interactive timing open; no full-frame/residency certificate.
Approval and immutable prototype artifacts are recorded locally. Next per-target/
empty impact presentation; retail/full core gates remain open and Jump deferred.


## Area-impact visual candidate (2026-10-28)

**STATIC original evidence:** battle_effect_store_targets_and_display_types
stores each victim ID/result animation; no victims and animate_on_miss_flag==0
produce a tile-coordinate effect target. battle_effect_run_keyframe_actions_all_targets
runs display/pose events across the target list with a caster-only exception.
battle_action_build_targets_post_action_message queues messages per victim.
These three bounded source readings are SHA-pinned in the local review report;
no new original spell animation/timing trace is claimed.

**OBSERVED in the current TI model:** the scalar draw_number can cache Miss when
act_t first reaches ACT_HIT but the owned batch is still READY. Commit occurs on
the next update, leaving that cached scalar glyph. Only st.tgt receives the old
pose/flash/label even though every batch victim has its HP applied. The zero-set
case can consequently display a false Miss above the caster.

**TARGET, private visual candidate only:** preview_area_impact.c uses six hooks
in a generated engine copy, retaining existing update/batch/response/reward code.
Per-victim execution labels, harmful-hit flashes/poses, a covered sloped centre
pulse and an Oil- message share the current sprites/font/number primitives.
Wait for APPLIED instead of a speculative scalar glyph. Root effects are suppressed
during secondary reactions; ordinary EXP/JP and terminal settlement remain.
Before/after use one common fixture camera. This is a proposed TI presentation,
not original spell art or a native implementation. Eight PC/sanitizer scenarios
and exact metrics are in campaign/CORE_AREA_IMPACT_REVIEW.md / its local report.
No active catalogue/module/package/image date or live TiEmu change. Native scratch,
resident/camera/layer/frame budgets follow visual approval. Full core remains
open; Jump is deferred.


## Approved area-impact integration (2026-10-28)

**OBSERVED, headless PC/68000:** the approved per-victim/empty-centre presentation
now runs in the shared engine.1012 actual-renderer checks12D9C238/64 individual
full screensD7A684DC agree;416 candidate captures match with the same scenario/
cache history.154 bounds checksC56EC8B9 cover13 prepared glyphs, both floors,
node255, signed quantities, Oil, followed Crystal/Jump/vanished centres and public
Work-owner preparation. The number-backend picture9DE20073 is not a full13-actor
scene certificate. No new original source oracle or complete spell timeline.

Cache412 aliases idle ai_gain512. Stage one glyph per wind-up update; prepare
thirteen plus Oil before ACT_HIT16. Shared resolution-centre identity differs
from new-command eligibility: Crystal/Jump coordinates still anchor an already
armed area. The root painter waits for APPLIED and yields to secondary reactions;
existing costs/HP/RNG/EXP/JP/outcome ordering is preserved. NUMBERS2416 holds the
unchanged scalar raster and new cache/blits through existing pure callbacks.
ABI34/ModCtx372/sig64400,16 images/zone6264; raw banks41072/38606. Resident input
64473/headroom463. No authored asm, new bitmap bank or live TiEmu operation.

**Measurement boundary:** native glyph-step peak20178, cold13-number backend
96350, full-render cold-orientation peak2528106. These are different tasks, not
a universal360K pass. Cached interactive/HUD/stack/combined residency remain
open. The roughly1s AI setup remains accepted inside its elapsed frozen pause.

**Portable-test finding:** planes_sum reads native32-bit words; its value differs
on a little-endian PC even with identical frame bytes. The new tests hash visible
plane bytes in a fixed order. Native fixture files include the existing DAT
trailer; keep the portable fixture constructor outside the limited native checker.

Next: Quick priority/clock liveness, remaining status/reaction/action and broader
AI/campaign/tooling families. Full core/content production is not complete. Jump
refinement remains deferred. Reviewed artifacts and user playtest/save state stay
intact; see campaign/CORE_AREA_IMPACT.md and its local verification report.

## Shared Math Skill core (2026-11-02)

OBSERVED: unmodified retail Calculator targeting compares CT/Level/EXP/whole
walking height against Prime/5/4/3, excludes zero and odd half-heights, forces
caster CT100 and marks panels before listing occupants. Source flags3 bit0x40
selects45 learned White/Black/Time/Yin-Yang spells; Math bypasses Reflect and
Counter guards but retains magical evasion. Function/terrain hashes,96 masks,
12 evasion profiles and explicit external helper boundaries are recorded in
campaign/x/core-math/oracle/source.json.

TARGET: CORE_MATH.md implements the shared owned path, pure forecasts and real
staged menus with first-frame cached rows. FCJD/ABI57/ModCtx368 retains the same
gameplay/save layouts and allocations. Nonroster JP-level acquisition and the
bounded stationary Math origin/utility AI are explicit native adaptations.
Sixteen cached numeric masks reuse32 bytes of existing idle scratch; the same
native trial measures15.15M to4.84M cycles, without changing its results. This
is not a full original action/menu/AI or whole-frame production certificate.
Jump12 stays user-deferred; remaining R/S/M and broader release gates stay open.


## PA/MA/Speed Save reactions (2026-11-03)

**OBSERVED IN FFT:** tools/stat_save_oracle.py runs unmodified retail finalizer,
post-damage dispatch, reaction preparation, original stat clamps and three
self-target lists.36 stages/18 effects show successful HP-damage-only triggering,
including zero damage and Sleep wake, blocked KO/secondary contexts, PA/MA+1
capped99 and Speed+1 capped50. External unrelated bookkeeping and status-specific
graphics handlers are isolated; this is not a full original UI/AI/animation trace.

**TARGET IMPLEMENTATION:** source IDs422..424 equip through existing FSV3 storage
and share bounded response staging, self effects, once-only impact and primary
packet restoration. No stock/MP/reaction reward or live CT/Charge rollback.
The existing native single-value stat policy is retained. Pure resident reaction
eligibility and unused callback removal give ABI58/ModCtx360; AREA_UI owns impact
numbers, NUMBERS retains forecasts. Generic coverage stays189/201, Jump deferred.
Evidence and narrow production margins are in campaign/CORE_STAT_SAVE.md.


## HP Restore / MP Restore / Critical Quick (2026-11-03)

**OBSERVED IN FFT:** tools/critical_reactions_oracle.py executes unchanged retail
post-impact dispatch,87 staging profiles,24 effects through final commit and
three self-target lists. Critical23 is derived after HP, honoring immutable
traits; successful zero damage can react, while Brave0 draws nothing. HP/MP
missing quantities freeze before impact, Undead retains positive HP healing,
Wall makes the prepared result miss and Quick writes CT255. Source CT is+0x39;
reaction bits at+0x8c are0x40/0x20/0x10. Bookkeeping/graphics external boundaries
are isolated; these are bounded function trials, not a whole UI/AI replay.

**TARGET IMPLEMENTATION:** equipped IDs431..433 share the existing response owner,
resources, scalar labels and complete root/Quick settlement. No cost, reaction
reward, new gameplay/save field or mutable Critical cache. The pure predicate
uses target_query253. ABI59/360 moves ordinary attack construction to RESPONSE4
and excludes an unused reward wrapper to recover room. Other R/S/M, original AI
and full-frame/residency limits remain explicit in campaign/CORE_CRITICAL_REACTIONS.md.


## Lancer Jump (2026-11-03)

**OBSERVED IN FFT:** unmodified retail preparation phase2 canonicalizes Jump to
394, with max(1,floor(50/Speed)) delay. Fourteen timing profiles and sixteen
formula100 profiles cover PA*WP, polearm PA*3/2, unarmed Brave, target physical
statuses/Defense Up and zodiac. Original source tables contain five horizontal
and seven vertical learning upgrades. Hashes are in the local oracle JSON.

**TARGET IMPLEMENTATION:** the user authorized simplified terrain rules. Keep
same-layer targeting, a fixed compact target cell, a protected airborne actor
and its reserved origin. Reuse deferred/action/reward ownership and approved
sprites/UI. Source layer preference, ceilings, PS1 jump animation and full future
CT/AI are separate. See campaign/CORE_LANCER_JUMP.md for verification and limits.

## Self-status reactions (2026-11-03)

CORE_STATUS_REACTIONS.md / ABI61 retains ModCtx360 and existing storage.
**OBSERVED:** Caution426 and Dragon Spirit427 use the pre-impact physical flag, even for a
miss/heal/no-op. Regenerator428 uses post-HP-damage staging; successful zero
is eligible. Exclude formula7; pre-physical reactions also exclude Math.
At Brave0 the first two still consume a chance draw, Regenerator does not.
**TARGET:** Freeze Defend/Reraise/Regen requests, then re-filter late immunity at commit.
A filtered no-op keeps source hit1 with packed request0; do not mutate pending
casts for it. Wall misses; Regen cancels Poison and refreshes36 ticks. Reuse the
status owner and preserve Charging/Performing. Source oracle60 stages/33 effects,
1159 logic+117 UI assertions/18 pictures compare PC/sanitizers/68000. No FCJD,
gameplay/save allocation or standard campaign group changes; full R/S/M and
production frame/stack/residency gates remain open. See the family note and
x/core-status-reactions/verification.json for final packing/regression evidence.

## Brave/Faith/Transparent reactions (2026-11-03)

**OBSERVED IN FFT:** Sunken State425 and Brave Up429 use pre-impact physical
eligibility, including misses/no-ops; Faith Up430 reads nonzero effective MP
cost independently of hit/physical flags. Formula7/reaction incapacity still
exclude them. All three consume a failed chance draw at Brave0. Original self
effects add3 Brave/Faith (cap100), or Transparent19 with ordinary status filters.
Transparent clears all four evades (four execution draws), clears on HP-damage
type even zero and on a later acted turn; a fresh infliction resets turn-removal
arming. Jump clears it when accepted. Source evidence:69 stages/37 effects,
three self-target lists and evade/removal fixtures.

**TARGET IMPLEMENTATION:** CORE_BRAVE_FAITH_REACTIONS.md uses existing status,
response and stat owners; the unsaved allegiance byte's spare bit6 carries
Transparent arming, and Invite preserves it. Keep the reviewed outlined sprite;
Hidden reuses the impact-label cache. Manual targeting/occupancy/collateral are
legal; hostile scalar/nearest AI and area/Math utility skip the hidden actor.
FORMULA14 prepares typed self packets after RESPONSE returns; RESPONSE6/7 owns
consecutive strikes, ORDER16 owns turn entry. ABI62/ModCtx360, zone8390,
banks62374/63840, actual packed input64927/64936:9 bytes remain. No gameplay/save
layout/heap/FCJD growth.1372 logic+98 UI assertions/14 pictures match
PC/sanitizers/68000. Broader R/S/M/permanent Brave/Faith carry-over and full-frame,
stack/residency/release gates remain separate. Preserve live TiEmu, the standard
campaign group and protected file bytes AND dates.

## Counter Tackle/Flood and Absorb Used MP (2026-11-03)

**OBSERVED IN FFT:** Counter Tackle436 tests raw flags4 bit0x10; Counter
Flood437 tests bit0x80, which includes ordinary Attack and Geomancy. Either may
stage on a miss/heal/no-op. Math/formula7 and ordinary reaction restrictions
still apply. Absorb438 reads the effective incoming MP byte; its self effect
adds that byte, caps MP, ignores Undead and misses under Wall. Original
preparation skips the acting slot. Source evidence:60 stages, nine effects and
four constructors. Constructor calls are Dash147/validation1 and the current
terrain's Elemental/validation0; isolated preview/MP/strike-init hooks are
recorded, not a complete original targeting/animation replay.

**TARGET IMPLEMENTATION:** CORE_COUNTER_REACTIONS.md freezes the MP byte in the
existing pending slot (0x8000|cost), after CoreAction validates ID438. No save
field grows. Tackle shares Dash math/RNG, with bounded adjacent/height reach;
special/stacked actor targeting remains open. Flood reuses Work.cast only after
the root commits, freezes all radius1 victims before HP and uses its own area
painting. Actor/ability ownership and phase checks reject stale FX/second commits;
secondary effects grant no EXP/JP and cannot stage nested reactions. ABI63 keeps
ModCtx360 and18 images: RESULTS3/4 own selection/strike queries, TARGETS11 Flood,
ORDER16 existing post-clock resets. Zone8452, banks63962/63918 and actual packed
input64867/64936 leave only38/82 bank bytes and69 input bytes.813 logic+34 UI
assertions/12 pictures match PC/sanitizers/68000; three UI transport partitions
retain all checks and pictures. Full-frame/stack/residency/release gates remain
open. Preserve protected saves/groups, including dates; do not touch live TiEmu.

## Movement delivery block A (2026-11-03)

**OBSERVED IN FFT:**128 retail movement configurations,16 additive Move/Jump
bonus boundaries and24 Teleport trials are recorded by movement_oracle.py.
Teleport rolls floor(rand*100/32768) against (distance-Move)*10, even inside the
guaranteed range. Equipped Float is promoted to innate Float17 by source status
initialization, protecting from Earth and ordinary removal. Six64-byte terrain
rows distinguish weather-dependent0, forbidden255 and water/lava classes.

**TARGET IMPLEMENTATION:** CORE_MOVEMENT.md closes blockA's15 generic traversal
capabilities. One encoded byte/unit and archived innate flags share battle and
management bonus math. Retain the previous ordinary-water adapter; source dry
slow costs and severe weather now have real consumers. Cold MOVEMENT18 returns
before AI_SETUP/TARGETS load. Native SCREENS shares Teleport's no-undo rule.
Fly/Teleport are bounded direct destinations on the current layer, with legal
landing/depth/lava/occupancy checks and existing art/target heights. ABI64/360
keeps one zone8640 and uses three matching FMB3/FMD2 shards6/6/7, with mandatory
whole-set validation before execution; no decompression/new ASM/new heap block.
Native Unit112/State2224/Work45582; FSV3/Save3138 unchanged. FCJE44538 adds864
archive bytes. Actual non-BENCH TTPack input64175 leaves761 bytes; do not quote
instrumented63227 as production.9957 family assertions and32 pictures agree
PC/sanitizers/68000. Cold reach can exceed a frame; full-frame/stack/residency/
release gates remain open. Continue with blockB17, then C12/D6 (35 IDs remain).
Preserve live TiEmu, standard groups and protected save/proof bytes AND dates.

## Automatic Mime integration (2026-11-04)

OBSERVED: hash-verified retail `battle_action_set_current_attacker_data` records
original team/facing/weapons and current unit/fixed target offsets; Mime job93
returns without replacing it. `battle_action_set_mimic_ability` filters the same
source team and PREVENT_REACTION, rotates the offset and previews the copied
action with borrowed weapons. CANNOT_MIMIC is secondary byte5 bit16 and applies
to default/Math/monster menus. Items/Throw/Jump/Charge use their menu-specific
rules. The source between-turn controller drains Mime slots after responses and
rewards; imitators retain primary-action reactions and rewards. Controlled
MIPS fixtures and isolation details: campaign/x/core-mime/oracle/source.json.

INTERPRETATION: a second primary root in the existing native context is enough;
recursive secondary ownership would incorrectly suppress ordinary reactions.
Borrow only weapon/menu state, never another actor's derived stats or CT.

TARGET: CORE_MIME.md documents the bounded implementation. Reuse current action
presentation; skip invalid/empty scalar centres, retain the existing same-layer
Jump adaptation, and copy only one immediate performance pulse. These are not
claims of complete original Mime animations or all weapon/AI fidelity.

## Ranged Hamedo integration (2026-11-04)

OBSERVED: hash-verified retail outer Hamedo draws Brave before PREVENT_REACTION
and counter-ability target/preview checks, and imposes no melee-only weapon gate.
36 controlled profiles plus four incoming/target/MP early gates execute the
original instructions; geometry/preview/list/MP callees are explicitly isolated.
See campaign/x/core-hamedo/oracle/source.json and CORE_HAMEDO.md.

INTERPRETATION: native ranged returns can reuse the existing pure trajectory
query, prepared before the effect image is entered. Rejected return geometry
must retain the already-consumed Brave draw without charging the original twice.

TARGET: reuse idle AI trajectory scratch, original pre-action ownership and the
ordinary shot renderer. Shared native three-tile extended range, straight-ray
obstruction and scalar target ownership are bounded policies; full original
weapon geometry, overlapping-panel selection and broader weapon fidelity remain
separate. No new image, save format or hardware path is required.

## Source weapon panel reach (2026-11-05)

OBSERVED: the unchanged, hash-verified retail weapon selector and strike/lunge/
arc/direct marking routines produce54080 panel outputs across160 weapon/hand/
Frog/height profiles. The direct spread/close-range helpers run unchanged; only
final target-selection filtering is isolated. The original attack-panel caller
excludes the origin.64 actual effective-height cases cover water, Float, movement
traits and byte wrapping. See tools/weapon_reach_oracle.py and its source manifest.

INTERPRETATION: the flags already stored in FCJH identify four distinct shapes;
item reach alone cannot distinguish a lance from an arc, or a gun from a bow.
Arc height penalties use signed truncation toward zero, including uphill cases.

TARGET: one pure shared rule supplies Attack, Charge, AI, Counter/Hamedo and
Mime. Manual masks prepare the origin once with existing scratch storage.
ORDER owns the unchanged shared CT/pending loop without per-tick image swaps.
This is panel reach, with existing straight obstruction/presentation; full arc
trajectories, lunge collisions, overlapping-panel victim lists and broader
weapon formulas/procs remain open. See campaign/CORE_WEAPON_REACH.md.

## Physical weapon bases and owned preparation (2026-11-05)

OBSERVED: unchanged retail base-XA, Charge, physical modifier, critical and
ordinary gun routines pass288 controlled scalar profiles. The actual random
helper returns16384 in preview without calling libc rand. Variable PA draws
precede the critical chance/amplitude. Ordinary gun formula03 starts at WP*WP
and omits ordinary physical evasion; outer guards remain before that handler.
Twenty-six further clean, compatible weapons pass original Two Swords hand
initialization/selection. Evidence: tools/weapon_formula_oracle.py.

INTERPRETATION: the existing source item-type byte distinguishes all physical
families without another catalogue field. Scalar/critical correctness does not
certify elements, healing/absorption, magical guns or the conditional-status/
spell-proc dispatcher and its subsequent RNG draw.

TARGET: a33-byte family map and a pure resident weapon_amount callback share
physical arithmetic. Each executing RESPONSE/COMBAT owner prepares its own
hit/base/critical sequence through weapon_roll.inc, without loading another
image. The facing-axis refactor retains1024 previous classifications. FCJH
changes only27 low hand bits, including the unarmed marker;26 real compatible
weapons gain native Two Swords. No renderer/art or gameplay/save layout changes.
See campaign/CORE_WEAPON_FORMULAS.md and its private verification manifest.
