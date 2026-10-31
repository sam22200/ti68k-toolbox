# Working rules (for any agent: Claude Code, Codex, ...)

The rules the user gave across sessions that are not about the platform itself. `CLAUDE.md`
(`AGENTS.md` is a link to it) holds the technical rules; this file holds the collaboration ones.
Both apply to every agent working in this repository.

## Talking and writing

- **Answer the user in French** (full accents); write every project file (code, comments, docs,
  commit messages) in English.
- Be concise: say what was done, what was measured, what is left; no recap of the question.
- Ask before writing any 68000 asm (`CLAUDE.md`), and before anything hard to undo or public.

## Roadmap and order of work

- **FFT UI/input work:** read `games/fft/GAME_DESIGN.md` first. It records the
  user's mandatory first-frame feedback, shared menus/file slots, physical keys,
  legal deployment, management animation/JP and1px-only scenery decisions.

- The roadmap is `TI68K_Game_Development_Toolbox.pdf` (repository root): 1 the Portable Game
  Runtime (C, SDL and TI backends), 2 the Open Flappy Bird port (`games/flappy`), 3 TI-Basic to C
  (`games/ffa`), 4 keypad "touch" gestures, 5 Bubble Ghost (Game Boy ROM reverse engineering),
  6 Prince of Persia.
- **FFT Talk Skill (2026-10-30):** CORE_TALK_SKILL.md adds seven Mediator
  speeches (115 records, 82 casts, 112/201 entries). Signed Brave/Faith loss in
  the record Y; AB_FAITH; AB_QUICK writes the prepared CT. Oracle tables in the
  68000 checks: use u8 when every field fits, emit only compared rows.
- **FFT Punch Art and protection families (2026-10-30):** CORE_PROTECT.md
  adds Reraise/Golem (State.golem[2], MOD_SIG changes); CORE_PUNCH_ART.md adds
  Stigma Magic/Chakra/Revive as immediate area casts and Wave/Repeating Fist
  as single-target strikes (108 records, 75 casts, 105/201 entries). Resident
  room: cold per-chapter setup belongs in an image (chapter_grid -> PARTY10;
  malloc/static buffers stay resident); `FFT_PRODUCTION_CORE` drops exported
  test-only entries from fftbat (non-static functions are always linked).
  Measure in scratch; object-section sizes mislead (inlining, unused code). Measure resident growth per change: an inline status read
  (core_status_current) costs ~50 bytes per copy, and a shared helper can cost
  more than it saves; drop source branches that are natively unreachable and
  document them. Oracle tables count against the 68000 check program limit:
  emit only compared rows/columns.
- **FFT control family (2026-10-30):** CORE_CONTROL_STATUS.md adds Frog/Blind
  Rage/Confusion Song/Foxbird together (101 records, 70 casts, 98/201 entries).
  STATUS_NO_CONTROL (status_flags 64) hands Berserk/Confusion/Chicken units to
  the AI; AB_BRAVE is Foxbird's prepared Brave loss. Resident room: the module
  zone is the largest image, so shrinking the AI image pays 1:1; code only
  called from one image belongs in that image (opening_after now in BEATS).
  ABI 45. Next: Reraise and Golem; Float waits for L1 movement with Jump.
- **FFT decisive family (2026-10-30):** CORE_TERMINAL.md adds Death/Quick/
  Zombie/Petrify together (56 original profiles, 12 upkeep traces, 1702
  PC/sanitizer/native assertions). 97 records, 66 casts, 94/201 generic entries.
  Death keeps its nominal max-HP damage separate from the Dead infliction;
  Quick has no status bit (explicit CT255 request); Petrify statues keep tile/
  HP/CT; Zombie reuses the Undead bit. KO-counter and end-turn CT policies live
  in FORMULA14/15, called only from resident clock code after images return.
  ABI 44/sig1438, zone 8228; private 539377, resident 64929/headroom 7: relocate
  before any resident growth. Next grouped Reraise/Frog/Float/Berserk/Confusion/
  Brave-Faith control; core incomplete, Jump deferred.
- **FFT resource-restoration family (2026-10-30):** CORE_RESTORATION.md adds
  Hi-Potion/X-Potion/Ether/Hi-Ether/Elixir together; all seven resource producers
  share source preparation/commit, all 14 ordinary Item records are enabled.
  93 records, 62 casts, 90/201 generic entries; 111 entries are not 111 effects.
  Paired effect_mp aliases scalar self cost only under explicit kinds; preserve
  response bytes and never charge that target MP payload to caster HP. Original
  Undead Elixir retains MP healing despite its HP-only damage flag (80 formula
  profiles/38 actual store traces). Resource no-change grants no EXP/JP; action
  start invalidates scalar forecast cache. SCREENS owns unchanged menu/view input,
  returns before resident loading callbacks; AI pause clock remains resident.
  2481 PC/sanitizer/native assertions (1683/AB2D4E2B +437/F9DBABDF +361/D10B5DD9).
  ABI 43/376/sig1181, 17 images split8+9, static zone8172 (+966, not heap), raw
  47710/53218. Private537806 (+5982), resident64829/headroom107; 27 banks identical.
  Unit112/State2192/Work45576/FSV3 and five allocations unchanged. Scoped final
  caller replays, layouts/12 old frames/pilot saves pass. Three compact legacy
  cleansing-item RNG adapters remain separate. Next grouped control/barrier/job
  families, then R/S/M/campaign/formats/tools/full budgets; core is incomplete,
  Jump deferred. Preserve live TiEmu/standard group/saves and proof bytes AND dates.
- **FFT cleansing family (2026-10-29):** CORE_CLEANSE.md adds Esuna/Dispel
  and Maiden's Kiss/Soft/Holy Water/Remedy as one cancellation family.88 records,
  62 casts (47 spells/15 summons),85/201 generic entries;116 remaining entries
  are not116 effects. Pack the removable subset in source bit order into the
  existing u16 (compiler bound16), rather than storing a live set ID or HP.
  Innate remains protected; immunity does not block cancellation; clear only
  corresponding timers, preserve HP/cell on a statue, refresh clocks/controls.
  New item0x38 packets consume no formula RNG and use FORMULA's shared commit/
  delayed reward drain; legacy compact pilot item adapters remain unchanged.
  1711 PC/sanitizer/native checks (1296/92582E7A +232/6A8077BD +183/4421B5B6),
  78 original profiles. NUMBERS owns cold forecast text/cache with a local pure
  evaluator and unchanged Wait arrows; never call loading skill callbacks from
  an executing image.32 arrow/six new cache controls remain byte-identical.
  Buff assertions split only for native test transport:5032/DB4D36CB plus
  189/671BE44C recombine to5221/3C724297, capacity461/E318A9A7 unchanged.
  ABI42/376/sig924,17 images/split9+8/static zone7206 (+86, not heap), raw
  50016/44228. Private531824 (+4415), resident64885/headroom51,27 data banks
  unchanged. No larger Unit/State/Work/FSV3 or sixth core allocation. Next HP/MP
  restoration items and remaining grouped control/barrier/job/RSM/campaign/
  format/tool/full-budget gates. Full core remains incomplete; Jump deferred.
  Preserve live TiEmu, standard group, user saves and old proof bytes AND dates.
  See private verification for final scoped closure and preservation hashes.
- **FFT percentage/resource-transfer family (2026-10-29):** CORE_PERCENT.md
  enables Demi/Demi2/Lich/Spell Absorb/Life Drain together. 82 normalized records,
  60 casts (45 spells/15 summons),79/201 generic entries;122 remaining entries
  are not122 effects. Source09/0f/10 percent amount is ceil(maximum*Y/100),
  independent of Faith; Faith/support/zodiac affect accuracy. Drains freeze a
  separate caster return in the existing slot, including nominal over-drain;
  MP effects do not touch HP/Sleep/Auto-Potion. Geometric masks exclude caster,
  effect masks include return. No new State/Work/FSV3 or heap allocation.
  2396 PC/sanitizer/native assertions (1710/82DE1E51 +476/90D1C72C
  +210/A45DC0D6),57 original paired-result profiles. Adapted AI values MP below
  HP and values denial only against units with learned casts. Shared scalar
  helpers retain original element/weather/heal/status fixtures. Unchanged
  catalogue validator moves to SAVE5 with null Work at bootstrap; ABI41/376/
  sig667,17 images/zone7120. IMPORTANT accounting correction: mod_zone is static
  storage inside resident code, never an additional image heap allocation;
  older checkpoint wording about image heap growth was inaccurate. Reuse
  unchanged bitmap/geometry/Jump evidence. Next compatible status/control/
  barrier and job families, then other full production gates; Jump deferred.
  Full core/content gate remains open. Preserve live TiEmu, standard playtest,
  user saves and old gallery/group bytes AND dates. Private527409 (+1392),
  resident64845/headroom91, raw43760/46002.27 other data banks identical. See
  CORE_PERCENT private verification for closure and preservation hashes.
- **FFT buff/status family (2026-10-29):** CORE_BUFFS.md enables 22 together,
  including Protect/Shell/Wall, timed controls, Faith/Reflect/Carbunkle/Silf.
 77 normalized records/55 casts,74/201 careers;127 unavailable table entries
  are not 127 effects. Source 10/11 all-or-nothing single-byte addition sets
  only; AB_STATUS packs byte/mask, never HP. Pure magic_heal query 0 handles
  this shape, FORMULA commits with shared bitmap/timer rules; no nested loads.
 5682 PC/sanitizer/native checks (5221/3C724297 +461/E318A9A7),67 original
  formula profiles/10 Protect cases; source clamp precedes evasion.13 distinct
  same-team test actors are a synthetic capacity fixture, not a retail party.
  Skip whole empty status bytes in the shared filter/apply loops; source row
  order and all 4843/262BF732 bitmap checks (177 profiles) stay equal.13-victim
  impact 441270→296906 cycles, preparation 289686; full frame/stack/RAM stays open.
  Unchanged cached compositor moves to RESULTS,28 seeded parts equal; preserve
  plane pointers and reviewed menu/frame/cursor style. ABI 40/376/sig 410,
 17 images/zone 7020 (+370 static resident zone bytes); Unit/State/Work/FSV3 and
  five core allocations unchanged. Private 526017(+2753), resident 64915/h 21;
 27 other data banks unchanged. Eight bad starts include real prior 39 export.
  Reuse cached original/geometry/Jump evidence unless its code changes; native
  replay of unchanged healing/Reflect carriers against final banks is valid.
  Next five compatible percentage/resource-transfer entries as a family; keep
  Jump deferred. The full core/content gate is not complete. Preserve standard
  playtest, user saves, live TiEmu and old gallery/group bytes AND timestamps.
- **FFT healing batch (2026-10-29):** CORE_HEALING.md enables8 together:
  Cure1–4/Raise1–2/Moogle/Fairy, for33 casts/55 normalized actions and52/201
  generic entries (149 unavailable table entries, not149 effects). Source0c/0d
  share CT/area/Reflect/typed effects/AI; cures ignore Shell/MDUp, Raise uses
  Faith on accuracy and ceil percentage HP independent of Faith. Undead/Dead/
  innate Dead/crystals/cure/CT/counter/one MP debit have source/native fixtures.
 2110 PC/sanitizer/native checks11072857,75 original profiles; old25 retain
 2736/77DAC288. Unchanged banners move to NUMBERS,32 exact plane comparisons.
  ABI39/376/sig153,17 images/zone6650; same State/Work/FSV3/five allocations.
  Private523264(+2925), resident64927/headroom9;27 data banks/assets unchanged.
  Binding162 includes new pure magic_heal pointer,8 bad starts include actual
  prior38 export;20 layouts/12 old frames/native pilot saves pass. Next compatible
  buff/status producer batch includes any required resident relocation; no
  standalone content-free prerequisite turn. Full core gates remain open,
  Jump deferred. Preserve live TiEmu/standard group/saves/gallery bytes AND dates.
- **FFT Reflect batch (2026-10-29):** CORE_REFLECT.md enables all25 ordinary
  offensive casts (15 spells/10 summons),44/201 generic action-table entries,
 157 unavailable entries (not157 unique effects). One source-derived family,
  original caster stats/fixed origin, one MP payment, single endpoint/no second
  AoE/no recursive bounce, existing bounded response queue. RESPONSE owns setup
  with pure resident math/lookup callbacks; no nested loader/new gameplay field/
  heap. Public status38 uses timer14/cure/immunity; Reflect spell/Carbunkle are
  still gated formula11 producers.2736 PC/sanitizer/native checks77DAC288 cover
  all25, with18 unchanged-MIPS endpoints/4 guards. Scoped callers/loader/layouts/
 12 old frames/native pilot saves pass. ABI38/372/sig65428,17 images/zone6398,
  group520339(+1512), resident64887/headroom49;27 data banks unchanged. Unused
  Reflect declarations explain TURN/BEATS drift in isolated controls. Future
  resident growth needs measured relocation. Next8 healing/resurrection entries
  as one compatible family batch; broader core gates remain open, Jump deferred.
  Preserve standard group/live TiEmu/saves/gallery bytes AND dates.
- **FFT family-batch direction/checkpoint (2026-10-29):** user explicitly
  requires effects to be implemented in batches, not per named ability or tiny
  gated slice. CORE_OFFENSIVE_MAGIC.md enables14 actual retail formula08 casts:
  Fire4/Bolt4/Ice4/Meteor and10 offensive summons. One offline classifier, no
  runtime dispatcher/ABI/RAM change; only ffcj changes, private518827 (+0).
  1290 PC/sanitizer/native checks08E75784 cover all14 production records, real
  learned lists, bindings, CT/MP/Silence/HalfMP/impacts/team filters/actual AI.
  Generic coverage33/201,168 unavailable table entries (not168 distinct effects);
  all normalized records36,14 casts. Reflectable11 remain a shared prerequisite
  batch; healing/status/other families, R/S/M and full core gates remain open.
  Reuse approved generic area presentation; no PS1 summon cinematics added.
  See grouped backlog/coverage.json before the next batch. Preserve standard
  group/live TiEmu/user saves/gallery bytes AND dates; keep Jump deferred.
- **FFT incapacity checkpoint (2026-10-29):** CORE_INCAPACITY.md adds ordinary
  elimination using original KO mask60c5000000 plus native HP/presence. Statues
  keep HP/cell/death counter and stop sustaining a camp; Stop/Sleep/Jump differ.
  Source counts original camps; future Charm/effective teams need separate data.
  Public Petrify/cures/art/persistence and source importance3 objectives remain
  gated; existing mandatory-Ramza permanent-loss rule stays.193 PC/sanitizer/
  native checks55D44C55 cover30 original outcomes/4 finalizers, ordinary effects/
  splash, queued Counter/AI, HP/occupancy/State/RNG, node255/count256 and terminal
  injection doors. game_scenario stops at end before test setup can overwrite
  M_END. Binding fixture restores real dimensions after abstract1x1 context.
  ORDER1 owns pure outcome, ORDER0 AT; no nested loader. ABI37/372/sig65171,
 17 images/split8+9/zone6316(+52); same core layouts/FSV3/five allocations. Raw
  shards40664/39940, private518827(+245), resident64765/headroom171;28 data banks
  stable. TURN+6/BEATS/RESULTS/AREA_AI drift reproduced with unused declarations.
  Scoped actions/reactions/scheduler/tempo/magic-integration159/owned CT/banks/
 12 frames/pilot saves pass. Outcome15922/5172, wake318536 are backend-only;
  full frame/stack/residency stays open. Reuse unchanged scalar/evasion/geometry/
  Jump evidence. Full core/content gate remains open; Jump deferred. Preserve
  live TiEmu/standard group/saves/gallery bytes AND dates; signal completion only
  after actual production gates pass. Next remaining status/response/targeting/
  AI, real producers/objective metadata, campaign, formats/tools and budgets.
- **FFT Haste/Slow checkpoint (2026-10-28):** CORE_HASTE_SLOW.md implements
  public tempo states, actual CT and finite/innate AT segments. Slow precedes
  Haste, Dead ignores both; base Speed stays stored. Real Slow1 gains0/M_CLOCK,
  AT minimum1/Quick55; Stop/Sleep wait consumes predicted tempo duration. Expiry
  precedes gain; Charge countdown is unscaled.748 PC/sanitizer/native checks
  30A798F0 cover45 original gains,4 charge phases/33 AT lists and lifecycle13.
  ORDER0 replaces FORMULA6:1018 bytes/no nested loader/new reciprocal data.
  ABI36/372/sig64914,17 images/split8+9/zone6264; directory+4, same core layouts/
  FSV3/five allocations. FORMULA5752(-436); unused-declaration controls reproduce
  RESULTS/AREA_AI same-size drift. Private518582(+650), resident64803/headroom133,
  shards40644/39636;28 data banks unchanged. AT39086/29724, CT max8778, wake266028
  are backend costs, not full-frame/stack/residency proof. Scoped scheduler/due/
  public status58/owned CT/banks/12 frames/pilot saves pass; use --integration-only
  to reuse bitmap evidence (unsupported control now Reraise). Next other status/
  incapacitation/response/AI/campaign/formats/tools/budgets. Retail spells/full
  core still gated; Jump deferred. Preserve live TiEmu/standard group/saves/
  gallery bytes AND dates; announce actual full-core completion only after gates.
- **FFT scheduler checkpoint (2026-10-28):** CORE_SCHEDULER.md closes Quick255
  priority/CT0 and all-frozen liveness. CT_DUE254/CT_WAIT255 never enter st.act;
  retain charge phase through Quick, settle responses/rewards first. M_CLOCK
  preserves last actor without turn credits, supports rotation/shared Quit;
  dialogs pause timers. Idle8 ticks, waking16, ordinary128 preserve normal pace.
  Original phase9 accepts Quick; source NONE is0xff00 after1201 phases, not0.
  AT clamps Speed0 to1 but retains source Quick255 remainder55.263 PC/sanitizer/
  native checks3056BFD2/two nonempty HUD hashes pass; max wake256892 backend
  cycles is not a full-frame/stack certificate. ABI35/372/sig64657,16 images/
  zone6264, layouts/FSV3/five allocations unchanged. FORMULA+14, AI_SETUP-6,
  same-size TURN drift reproduced with isolated declaration controls. Private
  517932(+195), resident64681/headroom255, shards41080/38606;28 data banks
  unchanged. Scoped deferred/control/owned CT/AI pause/banks/12 frames/pilot saves
  pass. Next broader status/reaction/targeting/AI and campaign/formats/tools/
  budgets. No retail Quick/casts or full-core completion; Jump deferred. Preserve
  standard group/live TiEmu/saves/gallery bytes AND dates. Tell the user when the
  actual full production gates close, not when a scheduler slice passes.
- **FFT approved area impacts (2026-10-28):** user validated the three impact
  artifacts with “ok and continue”. CORE_AREA_IMPACT.md is now shared PC/native
  implementation.412-byte glyph cache aliases idle ai_gain512; one victim per
  wind-up update, then shared Oil. Keep live packet/State/RNG untouched while
  painting, wait for APPLIED, show no fake caster Miss on an empty footprint,
  and suppress root effects during responses. Geometry and presentation share
  area_resolution_cell: followed Crystal/Jump coordinates survive; vanished
  off-field centres use MAP_N, not tile255. Initialize both ordered and public
  preparation into W->cast. ABI34/ModCtx372/sig64400,16 images/split8+8/zone6264;
  NUMBERS2416 also owns unchanged scalar number drawing, with no new callback
  or nested image loading. Resident64473/headroom463; raw banks41072/38606.
  416 accepted candidate captures match;1012 PC/sanitizer/native checks12D9C238,
 64 individual full screensD7A684DC,154 boundary checksC56EC8B9/9DE20073 pass.
  Per-glyph max20178 cycles;13-number cold backend96350; full-render cold view
  max2528106 is not a360K frame pass. UI cold375K and full stack/residency remain
  open. Next Quick/clock liveness and remaining shared families, not more Jump.
  Approval artifacts are immutable/read-only, badges cleared. Preserve standard
  group/live TiEmu/user saves/gallery bytes AND dates. Retail casts remain gated;
  the full core is incomplete. Announce completion only after production gates pass.
- **FFT approved area UI (2026-10-28):** user validated the historical PNG/GIF.
  CORE_AREA_UI.md is now shared PC/native integration, not a pending candidate.
  Unit/Panel selects centre binding; the target list inspects all victims and
  never excludes any. Raw range byte4 bit0x10 disables follow. AreaUi228 aliases
  idle ai_dist256; five allocations/Work45576/State2192/Unit112/FSV3 stay fixed.
  ABI33/ModCtx372/sig64143,15 images/split8+7/zone6264. AREA_UI cannot load another
  image; resident controller handles QUERY/SEND/BACK and pure area_paint never
  loads code. AI camera kernel moved unchanged to AI_SETUP3/4, pure world_of.
  499 UI checks67EED097/eight HUD hashes/four full replay screens,1321 camera
  checks4CC4400E and affected ABI/cast/AI/pause/save regressions pass. Private
  515564(+6146), resident64719/headroom217,28 non-code banks unchanged. Cold
 13-unit whole-map HUD375252 cycles is not a360K/full-frame pass; interactive
  budget/stack/residency remain open. Next reviewed victim/empty-centre impact;
  retail/full core gates stay closed, Jump deferred. Approval record pins old
  artifacts; review script is read-only after acceptance, badges cleared.
  Preserve standard group/live TiEmu/gallery bytes AND dates. Tell the user when
  all production gates actually close; do not equate this milestone with core done.
- **FFT AI-pause decision/checkpoint (2026-10-28):** user accepts the measured
  roughly1s AI setup inside the requested1–2s freeze; do not prioritize splitting
  it solely to meet30fps. CORE_AI_PACING.md counts thinking/camera settling in a
  1.5s elapsed256Hz minimum, with no second full hold if slower. Resident timestamp
  2 bytes; ai_t0 arms,255 skips post-action replan.171 PC/sanitizer/native checks
  58CBDE4E cover deadline/wrap/fast calls/restarts/cancel/frozen planes. Fourteen
  images/two shards/ABI32/layouts/five heap requests unchanged. Private509418(-24),
  resident64467/headroom469 (+20 input). Next reviewed area targeting/per-target/
  empty presentation; total worst-plan latency, stack/residency and other core
  gates stay open. A peak quantum is not a total plan measurement. Keep player
  first-frame input rules, Jump deferral and live playtest/gallery preservation.
- **FFT cached area-AI checkpoint (2026-10-28):** `CORE_AREA_AI.md` adds actual
  immediate hostile formula08 area choice via ai_think/AI_AIM; cache84 bytes aliases
  idle ai_dist256 only after setup/focus finishes. Restore actor coordinates on
  every return; weather restarts; forecast can evict AREA_AI and resume safely.
  Require both CAST+AREA: AREA-only0x9b weapon splash keeps legacy path. Late cast
  rejection replans without spending action.14 images/ABI32/ModCtx364/sig63878,
  zone6218(+14)/directory56(+4); State2192/Work45576/Unit112/FSV3/five allocations
  unchanged.676 PC/sanitizer/native checksD04607FA; source12 bounded HP/accounting
  profiles/5 ranks/3 empty panels, not full original future CT/status/Reflect/MP
  chooser. Cache count4 formula calls/one origin; whole-map skips equal centres.
  New area max319968 cycles; general stage0 setup/reach peaks12059828 (~1.005s
  at12MHz). Historical360K failure is accepted inside the frozen pause under
  the follow-up user decision above; next reviewed area UI/impact.
  Private509442(+4838), shards34464/36646, input64447/headroom489,28 data banks
  unchanged. Scoped forecast/CT/magic/actions/bindings/pilot saves/12 frames/layout
  pass; no broad geometry/Jump census. Core/C07-L4 remain open; Jump deferred.
  Preserve standard playtest466045 bytes/live TiEmu/gallery bytes AND dates.
- **FFT resident checkpoint (2026-10-28):** `CORE_HEADROOM.md`: pure FCD3/FGS1
  checks shared by PC and native SAVE4. Invalidate the old module binding before
  preload, then write descriptor/interface lengths into the existing input union;
  stale descriptor/Work pointers must not survive a same-chapter retry. ABI30/
  ModCtx364/signature63364/zone6204; resident64293/headroom643(-634), private504213
  (+74), SAVE3456, shards34390/31632. Nine images only change signature; TURN/FORMULA
  have same-size compiler changes, AI_SETUP shrinks6. Controlled declaration-only
  builds reproduce all three historical images; State/
  Work/FSV3/five allocations and28 non-code banks stable. Scope: parser130,
  bindings/retry130,7 native startup failures,16 loader/capacity cases, owned-area
  353+5, actual AI/control594, pilot flows/saves/12 frames/layout. Historical FMB2 rejection needs only
  one valid image: recombining all code now exceeds AMS transport limits. No
  authored asm or live TiEmu work. Next area forecast/AI/reviewed presentation;
  retail casts/broader core/C07-L4 stay gated. Tell the user explicitly when the
  full production gates are met; no unsupported date/progress percentage. Keep
  Jump deferred, playtest and old gallery bytes/dates unchanged.
- **FFT owned-area checkpoint (2026-10-27):** `CORE_AREA_CT.md` now routes supported
  formula08 area cast_order/real CT/automatic ACT_HIT through a dedicated60-byte
  Work packet.353 PC/sanitizer/native checks11D6FCB4 plus5 Orbonne retry checks
  00000230; source payment/binding/footprint/compound evidence reused, not a full
  original spell dispatcher trace. COMMAND returns3, resident calls TARGETS2 then
  preloads RESPONSE; RESPONSE11 commits at impact. No nested image eviction.
  Crystal/Jump can be absent victims but followed centres; vanished centres pay
  once with an empty root; missing raw geometry rejects before arming. Scenario/
  retry resets both State owner and Work phase (battle_units/fixed_units). Work
  45576 grows60, State2192/Unit112/packet22/FSV3 stable,5 allocations/totals+60.
  ABI29/ModCtx364/signature63107/zone6204; private504139(+533), shards33782/31632,
  input64927/headroom9,28 non-code banks unchanged. Scope tests to owned lifecycle,
  prior batch/incantation/due/actions/Charge, pilot flow/12 frames/layout/offsets.
  Software rt_file has16 cache slots: run separate processes per chapter as the
  supervisor does; do not expand runtime cache for a fixture. Next area forecast/
  AI and reviewed multi-target/empty presentation; retail AF_CAST remains disabled.
  Resident growth and C07/L4 frame/residency remain open; Jump deferred. Preserve
  live TiEmu, standard group and old gallery bytes/dates.
- **FFT archive-bank checkpoint (2026-10-27):** `CORE_BANKS.md` replaces active
  FMB2 with required FMB3 ffbm/ffbn33772/31234. All13 code images differ only by
  ABI28 signature62790; ModCtx364, execution zone6204. Validate matching export
  token/ranges/all spans before any code load; no single-bank fallback. Loader
  caches13 archive pointers (52 bytes replacing6) once, with no per-call shard/file
  lookup. Fixed70 binding fields share a364-byte const template;23 dynamic fields
  refresh on cold loads, warm input preserved.130 PC/sanitizer/native parser
  checks,112 native full-context checks and6 bad/missing/mixed startup cases pass.
  Batch1202/actions/Charge/pilot save-flow and12 old frames are the affected
  closure; no geography/scalar/Jump sweep. State/Work/FSV3/five allocations stable.
  Private503606(-27), input64845/headroom91;28 non-code banks unchanged. The new
  template adds a small measured cold-copy cost; no measured warm-speedup claim.
  New native inputs/Makefiles must ship BOTH shards. Legacy controls remain
  explicitly historical. Next durable batch/area CT/command/AI/impact/presentation;
  resident growth and C07/L4 frame/residency stay open, Jump deferred. Preserve
  standard group, live TiEmu and gallery bytes/dates.
- **FFT compound-cast checkpoint (2026-10-27):** `CORE_BATCH.md` adds effect-stage
  all-target preparation, one MP payment, once-only HP/status commit and existing
  response/reward drain.13 original compound groups/1202 PC/sanitizer/native checks
  94558E80; no full original cost/HP/reaction/reward/presentation dispatcher trace.
  Shared magic_prepare/magic_commit serve old single-target and new batch paths.
  Caller retains60 bytes through commit; State2192/Unit112/Work45516/FSV3 unchanged,
  no new heap. ABI27/ModCtx364/signature62533/13 images/largest6204. TARGETS1 prepares,
  RESPONSE10 commits; no image may evict itself. Private503633(+3027), raw bank64984,
  input64783/headroom153,28 non-code banks unchanged. Production wrapper may be
  dead-stripped until referenced: budget live ownership/code before activation.
  Next area CT/command/AI/explicit impact routing and reviewed presentation. Never
  run old attack_step between PREPARE and explicit COMMIT. Unsupported performance/
  weapon/item/drain families reject. C07/L4 full-frame/residency remains open;
  Jump deferred. Scope tests to batch plus affected magic/actions/responses/Charge/
  flow/12 frames; preserve playtest, live TiEmu and gallery bytes/dates.
- **FFT radial-target checkpoint (2026-10-27):** `CORE_TARGETS.md` verifies a pure
  formula08 footprint query:79 original footprints/42 columns,1193 PC/sanitizer/
  native checksFC02B091. One-floor maps skip upper reads; hidden upper surfaces
  still affect two-floor choices. Presence differs from final effect eligibility;
  follow centres can outlive their victim's presence. Team filters operate on
  panels, including overlap/mark-order quirks. ABI26/thirteen images/ModCtx364/
  signature62276/largest6172; State2192/Unit112/Work45516/packet22/FSV3 unchanged.
  Private500606(+1564), raw bank62070, input64749/headroom187;28 non-code banks
  unchanged. Query only: next source slot-order mapping and all formula results
  before HP commits, one payment, per-target reactions/rewards/presentation.
  Keep retail spells gated, C07/L4 cold-HUD/frame/residency open, Jump deferred.
  Scope closure to targets/ABI/layout plus weather/actions/Charge/flow/12 frames;
  preserve live TiEmu, standard group and gallery bytes/dates.
- **FFT formula08 environment checkpoint (2026-10-27):** `CORE_WEATHER.md`
  supersedes the weather/Oil/Float next-step notes below.254 original profiles/
  6493 PC/sanitizer/native assertions8685AD66 preserve signed sequential weather,
  PS1 Oil's no-double-damage bug/protected removal and Float/earth null. Queries
  are pure; due casts read live climate, then packet quantities/removals survive
  later queries/climate changes and actual Auto-Potion transport. Preview source
  attack-type136 is a bitfield, not a null. Oil/Float infliction/movement and live
  climate/calendar/GNS/bow-weather producers remain gated. Raw strength0..4 comes
  from the original calendar clamp; flags snow1/indoors2. State2192 adds2 bytes,
  first heap block5330, five requests; Unit112/Work45516/packet22/FSV3 stable.
  ABI25/ModCtx364/signature62019/zone6172 rejects24. FORMULA11 constructs forecast
  cache through local math/formatter and a pure resident width callback. Keep
  sizing resident (cold-sizing trial83k rejected); final header45822. Full cold
  forecast max92032 is not within the old60k heuristic: C07/L4 still needs full
  frame/cold-work accounting, stack and residency evidence. Private499042 (+279),
  input64735/headroom201; all28 non-code banks unchanged. Next area target sets,
  per-target effect/reward/reaction/presentation. Keep Jump deferred, live TiEmu/
  standard group and all existing image bytes/dates untouched. Scope tests to
  magic/packet/UI/ABI/flow closure; reuse unchanged catalogue/geometry evidence.
- **FFT magical-evasion checkpoint (2026-10-27):** `CORE_MAGIC_EVASION.md`
  supersedes the elemental-only next-step note below. Source238 profiles and7510
  PC/sanitizer/68000 assertions995C7993 cover magical percentages, source-order
  zero-channel RNG, late ordinary Dead/Petrify/Wall rejection, actual due payment/
  AI/miss behavior. Previews remain pure; nullification is after evasion, so only
  non-evadeable Holy null is RNG-free. FCJA replaces9 with six-bit item MEV and
  shield tag in the same byte as preserved HAND_DUAL; source max50, no record/
  cache/save growth. All256 source items and unrelated catalogue bytes checked.
  ABI24/ModCtx360/signature61752/zone6146 rejects23. Private498763 (+1944), input
  64899/headroom37; five allocations unchanged,27 non-code banks unchanged.
  RESULTS1 formats cold cached menu rows with local can_use and pure callbacks;
  no image may evict itself. Cold row45716<60k task budget; full-frame/stack/
  residency not certified. Keep retail spells/status/monster/mount producers
  gated. Next weather/Oil/Float/areas/presentation. Source Abandon water mask0x46
  differs from older gated reaction adapter0x0e; fix that adapter with its own
  producer fixture before enabling Float/Berserk/movement traits. Keep Jump
  deferred, standard group/live TiEmu and existing image bytes/dates untouched.
- **FFT elemental-outcome checkpoint (2026-10-27):** `CORE_ELEMENTS.md`
  connects cached Strengthen/Half/Weak and damage/healing/null outcomes across
  prediction/native AI/impact.77 original formula/finalizer/signed-HP-prefix
  profiles;1525 PC/sanitizer/native checks92063E01. Preserve signed narrowing
  before Half/Weak/Faith and positive999 caps; event type differs from actual
  HP direction on wrap. Null wins over absorb, pays once, takes no effect RNG/
  reward; normal absorbed healing does not wake Sleep or offer Auto-Potion.
  Prepared effect_kind survives trait/query changes and response packet copying.
  Injected healed-primary response tests transport only, not a natural reaction.
  State2190/ActionPacket22 use former padding; Unit112/Work45516/FSV3/five blocks
  unchanged. ABI23/ModCtx352/signature61487/zone6096 rejects22. Private496819
  (+1061), input64795/headroom141; all28 data banks unchanged. FORMULA10 formats
  forecasts from resident callers with local evaluation; never invoke that
  loader from an executing image. Cold46180 cycles fits the60k task envelope,
  not a full-frame certificate. Magic1352 hash becomes B5895FED for newly
  source-capped shared amounts;104 raw profiles retain values. Retail Holy/
  Fire stay disabled. Next magic evasion/full target eligibility, then weather/
  Oil/Float/areas/presentation; broader core/residency remains open. Keep Jump
  deferred, standard group/live TiEmu untouched and identical image dates intact.
- **FFT affinity-data checkpoint (2026-10-27):** `CORE_AFFINITY.md` adds
  FCJ9/FEA1:416 source job/item references,32 shared five-byte masks,582 bytes.
 178 original aggregation profiles and1881 PC/sanitizer/native checks C0B66CFB.
  Four job masks/zero Strengthen, then equipment OR; old FCJ8/bad references
  reject. Derived cache13x5 lives outside Unit112; native State2190 (+66),
  Work45516/FSV3/five heap requests unchanged (first block5328). Scratch Units
  must not overwrite live caches; clear removed/spare actors, replace rather
  than accumulate traits. Native stats local in PARTY/ACTION/RESPONSE; remove
  unit_stats callback so unsupported image callers fail linkage. Resident
  wrapper uses PARTY9 and the transient pointer/cast-input union; unsigned
  growth helper keeps stack ABI. ABI22/ModCtx348/signature61226/zone6060.
  Private495758 (+2709), input64859/headroom77;27 non-code banks unchanged.
  This is data/cache preparation only: elemental effects/absorption/healing/
  nullification, monster gear policy and retail spells remain gated. Next typed
  elemental outcomes through forecast/AI/impact/response, source narrowing/caps,
  then evasion/areas/presentation. Keep Jump deferred, standard group/live
  TiEmu untouched, and preserve identical image dates. CPU tradeoff is recorded;
  no full frame/residency claim follows from passing checks.
- **FFT magical-scalar checkpoint (2026-10-27):** `CORE_MAGIC.md` adds pure
  resident formula08 math and a private normalized Holy through actual cast/
  payment/impact/AI/reaction settlement.104 original-MIPS profiles,1352 PC/
  sanitizer/native checks4DCFDD40. Holy flags4=0x40 lacks magic-evasion bit;
  Fire0x42 differs. Keep the one generic execute proc draw; preview takes none.
  Physical vulnerability/critical/Brave knockback do not apply. Scalar bit reads
  for Shell/Frog/Chicken/Faith/Innocent do not enable producers or other effects.
  Elements/innate traits/monster zodiac/magic evasion/areas/presentation remain
  gated; production Holy/Fire and all AF_CAST records stay disabled. ABI21/
  ModCtx348/signature60771, zone5882; battle/save layouts/five allocations
  unchanged. Private493049 (+503), input64919/headroom17;28 data banks unchanged.
  Pure callbacks must not evict their executing image. Cold banner/cache setup
  moves to PARTY7/8; invalidate the tag before new Work/dimensions. Frequent
  end-turn CT stays resident. Next traits/target policy/evasion then areas and
  presentation. Broader core/residency gates remain open; Jump deferred.
  Keep standard group/live TiEmu untouched and preserve identical image dates.
- **FFT incantation checkpoint (2026-10-27):** `CORE_INCANTATION.md` adds
  actual tile/unit-bound vulnerable casts using the existing eight-byte pending
  records (states4/5, tile means node or actor slot; Charge alone reads origin/
  weapon/off_weapon). Arming preserves Move, spends Act and no MP/RNG. Stop
  pauses; KO/Sleep cancel; late Silence/Don't Act/MP failure reject. Caster
  movement/weapon changes survive. COMMAND3/4 own validation/arming/due launch;
  cast arguments live in ModCtx, with no State/formula mutation on rejection.
  Return before ACTION preload/AI continuation; resident pending polling never
  loads an image. Paid execution uses skill_effect_hit, not preview skill_hit:
  an exact-cost natural hit catches the preceding hidden post-payment MP check.
  Private carriers are not retail spells/areas/UI. ABI20/ModCtx344/zone5862,
  unchanged State2124/Unit112/Work45516/FSV3/five allocations. Private492546
  (+1481), input64859/headroom77;28 data banks unchanged. No slot recycling
  while bound identities remain live; future lifecycle producers need explicit
  identity policy. Next magical formulas/traits, areas and presentation. Keep
  Jump deferred and standard group/live TiEmu untouched.
- **FFT casting-resource checkpoint (2026-10-27):** `CORE_CASTING.md` adds pure
  resident skill_cost, shared MP eligibility and once-only execution debit. Arming
  returns before payment. Source33 MP/12 prepare cases and Holy arm999/pay943;
  native nonzero-cost tests use a synthetic Dash cost7/Silence carrier, never
  retail Dash or an enabled spell. Test real AI setup lists; ai_usable is absent
  from resident native builds. Preview may write request scratch; invalid orders
  must preserve whole State/RNG. Half MP floors; Short Charge rounds half up;
  Non-Charge wins unless persevering. Timing kernels are not live cast producers.
  ABI19/ModCtx336/twelve images/zone5860; native Unit/State/Work/FSV3/heap count
  unchanged. COMMAND2 owns the unchanged cold range body; pure callbacks must
  not evict their caller. Private491065 (+686), input64743/headroom193;28 data
  banks unchanged. Original inline version exceeded the packer; final sharing/
  relocation resolves it. Next actual tile/unit casting/formula/AoE; broader
  liveness/Quick/traits remain open. Keep Jump deferred and standard group/live
  TiEmu untouched.
- **FFT deferred checkpoint (2026-10-26):** `CORE_DEFERRED.md` changes due
  Charge dispatch to iterative turn entry. Internal charge_resolve returns0
  for consumed/missing/rejected work and1 for presentation ownership; direct
  fixtures must not expect a recursive turn. Guard live roots before mutation.
  Source six clock sequences/four coordinate cases; native stress uses13 due
  slots, invalidation, Counter and terminal boundaries. Use Speed50/CT50 for
  turn_start stress: Speed100 exceeds the reciprocal table domain. ABI18/types/
  image bank/data unchanged; private490379 (+8), input64857/headroom79. Next
  magic must distinguish tile from stable-unit binding and source-check MP/
  interruption without inheriting Archer weapon/movement rules. Quick255 and
  all-permanent-freeze clock liveness remain open. Keep Jump deferred and live
  TiEmu/standard group untouched; prior layout/art evidence is reused.
- **FFT dual-Charge checkpoint (2026-10-26):** `CORE_CHARGE_HANDS.md` enables
  all eight Archer powers for supported two hands, source criticals, both delayed
  weapon IDs and shared potential preview/AI.586 targeted PC/sanitizer/native
  checks; source8 initializations/60 scalars, not full delayed targeting/terrain.
  Pending.off_weapon consumes padding: seed both actual hand IDs in manual
  fixtures; native State2124/Unit112/Work45516/FSV3/five allocations unchanged.
  ABI18/twelve images, ModCtx332, zone5814; FCJ8 and28 data banks unchanged.
  Private490371 (+1570), input64845/headroom91. Shared resident physical math
  avoids image eviction; unchanged pickup moves to PARTY6. More resident code
  still needs a module budget. Battle RNG is rng32, not rt_seed: scalar/preview
  queries preserve it; complete AI selection has a legitimate existing chooser
  draw even at100% hit. Earlier hands prose overstated its rt_seed assertion.
  Next: fixed-cell/unit-following magic, target sets/AoE and equal-tick deferred
  scheduling; other skills/procs/elements/status/reaction/residency gates remain
  open. Keep Jump deferred and standard group/live TiEmu untouched.
- **FFT hands checkpoint (2026-10-26):** `CORE_HANDS.md` enables ordinary
  Two Swords/dual Counter for plain melee/unarmed hands, shared potential-damage
  forecast/AI and capability-aware left equipment.196+16 targeted PC/sanitizer/
  native checks; existing strikes/reactions/context/actions/Charge and pilot flow
  retain hashes. FCJ8 reuses Item.pad and appends20 innate-job bytes; update the
  descriptor builder as well as readers. FSV3/Unit112/Work45516 unchanged;
  State2124 (+30), packet22, ModCtx328. ABI17/twelve images split COMMAND and
  RESPONSE. Rejections must precede resident request-field mutation. Return
  before AI continuation; reserved mod_call op255 preloads ACTION after COMMAND
  returns during wind-up, without invoking an entry. The cold-copy-at-impact
  version averaged42614 cycles; final targeted impact average15364/peak54446.
  Private488801 (+5387), input64821/headroom115, zone5750. Further resident
  growth still needs a module budget. Left-only Charge records/validates the
  promoted actual weapon; two-strike weapon skills/elements/procs stay gated.
  The existing potential-hit AI is not a full original probability simulation.
  Keep Jump deferred and live TiEmu/standard group untouched. Older notes are
  historical measurements, not a full-core certificate.
- **FFT consecutive-strike checkpoint (2026-10-26):** `CORE_STRIKES.md` adds the
  bounded one/two-strike ordinary-Attack engine entry. Both impacts precede one
  response drain; keep the highest EXP amount and award JP once.250 targeted
  PC/sanitizer/native checks include fresh targets and FORMULA/AI ranged return
  boundaries. Normal commands request one: do not enable Two Swords until
  independent hands/formulas/preview/AI are verified. ABI16/ten images,
  CoreAction34/State2094 (+2); Unit112/Work45516/FSV3 unchanged. Private483414
  (+836), input64923/headroom13, zone5830. That13-byte packer margin requires
  relocating further resident growth before adding it. Source32 initializers,
  five isolated phase branches and ten EXP pairs are not a full original
  double-strike/retargeting trace. Keep Jump deferred, standard group/live
  TiEmu untouched; old checkpoint numbers below are historical.
- **FFT reaction checkpoint (2026-10-26):** `CORE_REACTIONS.md` enables equipped
  Counter/Auto-Potion through ACTION, the tenth image (ABI15). Source staging,
  Brave/stock choices, packet suspension/restoration, shared-stock competition,
  secondary suppression and deferred root rewards pass267 PC/sanitizer/native
  checks. Counter uses the source reaction flag: Dash/Throw Stone lack it.
  Revalidate live status/range/stock at drain; ranged LoS runs only after ACTION
  returns, with at most one rejected trajectory per update. Reuse ACTION's local
  evaluator while resident to avoid ACTION/FORMULA copy ping-pong. Native
  State2092 (+46), unchanged Unit112/Work45516/FSV3/allocation count; private
  group482578 (+4008), packed input64605/headroom331, largest image5742.
  Fixed/generated encounter traits, other reaction families, source strike
  sequencing and complete timing/residency remain open. Keep Jump deferred and
  live TiEmu/standard group untouched. Older notes are historical measurements.
- **FFT action-context checkpoint (2026-10-26):** `CORE_CONTEXT.md` adds a32-byte
  transient root/impact/response context, with357 PC/sanitizer/native checks.
  New orders reject while a root is live, before costs/RNG/pending mutation;
  FORMULA consumes hit/miss once. Response primitives are bounded per actor,
  slot ordered and nonrecursive, but no live reaction effect/producer exists.
  Native State2046 (+32), Unit112/Work45516/FSV3 unchanged; ABI14/nine images,
  private478570 (+434), input64727/headroom209, zone5742. Cold command start is
  FORMULA-owned (41644 cycles;5970 prepared), after any previous image returns.
  Raw impact fixtures must seed a context; successive independent actions must
  close the prior committed one when skipping presentation. Next: reaction
  effect/presentation packet ownership/restoration and source strike ordering.
  Keep Jump deferred and live TiEmu/standard group untouched. Older notes below
  retain historical measurements, not a full-core certificate.
- **FFT Sleep checkpoint (2026-10-26):** `CORE_SLEEP.md` enables the tested
  API effect:60-tick CT freeze, Charge cancellation, source physical vulnerability
  before zodiac, protected HP-damage-type wake at impact.1070 PC/sanitizer/native
  assertions; no new producer/art, allocation or fields. ABI13/nine images,
  Unit112/State2014/Work45516/FSV3 unchanged, private478136 (+112), input64837/
  headroom99, zone5742. The old Charge fixture must seed canonical Charging
  alongside its pending record; pending presence is not status authority.
  Next: explicit action/reaction contexts. Broader effects, residency/timing and
  content gates remain open; Jump deferred, live TiEmu/standard group untouched.
  Earlier checkpoint figures below are historical.
- **FFT control checkpoint (2026-10-26):** latest L2 slice is `CORE_CONTROL.md`:
  Stop pauses CT/Charge; Don't Move/Don't Act share player/preview/AI eligibility,
  CT credit and due-time Charge rejection. Existing Charging targets now receive
  the source no-evasion rule.594 PC/sanitizer/68000 checks pass. ABI12/nine images,
  unchanged Unit112/State2014/Work45516/FSV3; private478024 (+1340), input64847/
  headroom89. Cold AT/glyph setup use existing images. Scene steps request
  OPEN_BEGIN and return before resident battle entry can load FORMULA; invalidate
  the image tag before rebinding new Work. No callback may evict its caller's
  executing image. Next: Sleep damage/wake and action/reaction contexts; no new
  spell/terrain producers or status presentation. Keep Jump deferred and the
  user's live TiEmu/standard group untouched. Older notes below are historical.
- **FFT lifetime checkpoint (2026-10-26):** latest L2 evidence is
  `CORE_STATUS_LIFETIME.md`:16 source counters,15 ticked IDs, sparse active-bit
  iteration, Poison/Regen end-turn upkeep and shared timed/command KO cleanup.
  4140 PC/sanitizer/native checks include13 actors and terminal upkeep. ABI11/
  nine images, Unit112/State2014 (+210 RAM), unchanged Work/FSV3/five allocations;
  private476684 (+1168), packed input64789/headroom147. Source FCJ7 banks are
  unchanged. Next: incapacitation/cast interruption and action/reaction contexts.
  Other status handlers, trait/terrain producers and upkeep presentation remain
  open; no full scheduler/frame-budget certificate. Keep Jump deferred and the
  user's live TiEmu/standard playtest group untouched.
- **FFT status-state checkpoint (2026-10-26):** `CORE_STATUS_STATE.md` is the
  latest L2 slice: canonical40-bit bitmap policy, innate/immune separation,
  protected existing cures/AI and Charge/Defend command-state consistency.
  FCJ7 adds650 archived policy bytes; ABI10/nine images, Unit96/State1804,
  +218 RAM bytes without another allocation, unchanged Work/FSV3. Private
  group475516 (+1877), packed input64673/headroom263.4842 status assertions
  pass PC/sanitizers/68000. Only implemented additions are accepted; new Poison
  stays rejected pending upkeep. Next: lifetime/upkeep, incapacitation and
  action/reaction contexts; equipment trait producers remain separate. Do not
  equate40-bit storage with40 playable effects. Keep Jump deferred. Earlier
  checkpoint package/layout numbers below are historical.
- **FFT status hook checkpoint (2026-10-26):** `CORE_STATUS_RULES.md` records
  Darkness in shared physical preview/rolls and source flag/menu-based Silence
  restrictions for player/formula/AI. FCJ6 appends224 archived dispatch bytes;
  old/missing tables reject. No new heap/State/Work/FSV3 fields; ABI9/nine images,
  private473639 bytes, packed input64575/headroom361. Full status policy and
  poison upkeep/reaction contexts remain open. Do not label the synthetic Wish
  silence-flag fixture as retail Wish behavior or claim magic is implemented.
- **FFT L2 primary-action checkpoint (2026-10-25):** `CORE_ACTIONS.md` separates
  impact commit from animation and shares existing target HP/status transitions
  with AI/forecasts. ABI8/nine images, unchanged Work/State/FSV3/heap; private
  group472465 (+464), packed input64177/headroom759. Six action pictures match
  captured old/current PC/native output. Impact copy adds CPU (peak33480 cycles
  in scoped test), not a speedup. Full statuses/reaction contexts remain next;
  Jump remains deferred. Previous checkpoint package/ABI numbers are historical.
- **FFT common topology checkpoint (2026-10-25):** `CORE_COMPACT.md` closes
  the already-changed storage work: ABI7/nine images,36/68 bytes per node;
  four cached first neighbors retain ordinary CPU and save1798/1678 pilot heap
  bytes. Direct per-query neighbor conversion was rejected for CPU cost. Native
  max two-floor layout grows to126304 bytes, not a peak-residency certificate.
  Current private group472001 bytes; cost6/32-link capacity is checked, while
  gameplay stays one-gap. Next priority is L2 actions/statuses, per user steering.
- **FFT priority correction (user2026-10-25):** Jump is a minor mechanic; stop
  spending disproportionate effort on it. Finish the already-changed common
  graph storage checks/package, then prioritize L2 shared actions/statuses.
  Retain long-span prototypes as deferred evidence; do not make exact Jump
  fidelity a prerequisite for all other core work. One-gap gameplay remains
  until later supported content actually requires more.
- **FFT current roadmap (2026-10-25):** read `games/fft/campaign/CORE_ROADMAP.md`
  first. L1 is advanced, not complete; L2 action/status/scheduler and L3 families/
  campaign remain substantial work, and L4 formats/editors/memory gates stay open.
  Dated checkpoint notes are history, not a full-core certificate. New targeted
  source propagation fixtures (`CORE_JUMP_COSTS.md`) prove a3-gap/cost3 destination
  can cost6; update link encoding/evaluation before hookup, not a storm/map-specific
  exception. Unit/weather producer and reverse AI cost policy are still separate.
- **FFT workstream4 (2026-10-24):** latest UI reviews are accepted. Audit original
  limits before map production; use `games/fft/campaign/CORE_AUDIT.md` and the
  twenty-case fixture registry. **Subsequent user-selected scope:**13 battle
  actors; omit the eleven-Monk encounter. Use256 compact8-bit tile IDs across
  both floors,16-bit counts and separate validity/coordinates. Read
  `games/fft/campaign/PORT_LIMITS.md`; original21-slot/512-source-cell research
  does not require a21-actor or512-node TI implementation. Keep16 Formation+4
  guest persistence separate from the thirteen-actor battle limit.
  The first implementation is recorded in `games/fft/campaign/CORE_CAPACITY.md`:
  descriptor-driven13-capacity actors pass native fixtures;256-node/two-floor
  identity/search passes `CORE_GEOMETRY.md`; canonical unit/cursor/path/Charge/AI
  cells and FMB2 modules pass `CORE_CELL_STATE.md`. The actual13-actor/256-node
  adapters and separate topology cache pass `CORE_FULL_GRID.md`. Source solid
  spans/ordinary endpoint headroom now pass `CORE_SURFACES.md` with ABI4, required
  FGS1/SGE1 geometry and9462+55 checks. Full source upper paths, raster bridge
  cover, special movement/player targeting and combined residency remain next.
  `CORE_JUMPS.md` is accepted2026-10-25 (47301+156 checks; four changed
  Orbonne auto-battle captures accepted; Decision cleared). The user requested
  single-floor specialization now passes `CORE_MAP_PROFILE.md`:3,234,825 native
  equivalence checks,169 unchanged accepted PNGs, ABI5/eight overlays, private
  group469834 bytes. Count referenced
  event/random maps and distinguish no upper stop cells, movement spans and
  projectile solids. Never disable necessary lower-floor slope/height checks.
  `CORE_STEPS.md` endpoint movement is accepted2026-10-25 (112 original cases,
  451 kernel +80 battle checks). Only Gariland rings change. Scratch reuse
  preserves accepted graphs/pixels with30.77%/28.18% less mean cold graph CPU
  over Jump1..4 and no extra allocation. Private group470407 bytes (+178).
  Ordinary Jump caps at7 before graph-key lookup. During `edges`, queue bytes
  and actor blockers are borrowed for edge/steep/water data: no search may run
  concurrently, and search initialization must reset these owners afterwards.
  `CORE_LONG_JUMPS.md`:22 original full-spread cases/278 checks verify a terrain-
  only prototype; shipping still has one-gap jumps. Next extend bounded links,
  queue cost support and every intermediate actor check before enabling it.
  The optional queue prerequisite now passes `CORE_SPAN_SEARCH.md` (27592
  graph +1219/1065 actual caller checks); existing1054-byte Search storage is
  reused. `FFT_NAV_SPANS` is test-only for bound builds: resident inclusion
  exceeds TTPack's64936-byte input ceiling. First solve code headroom, bounded
  links and all-intermediate actors; never evict an executing AI overlay to
  run another overlay. Ordinary private package470444 bytes (+37); no long
  jumps are active. ABI/layouts,28 data hashes and accepted images are intact.
  Current follow-up `CORE_RESIDENT.md` resolves the initial packer failure:
  ABI6/nine images split PARTY from SAVE; Status keys use SCREENS. Zone5734
  (down340), wide search now linked, Work/State/FSV3 unchanged. Private group
  471882 bytes (+1438); packed input64831 leaves105 bytes. Reports now distinguish
  TTPack headroom from archive size. The88-query/99-check actor-column prototype
  verifies both layers/frontier/body/standing+3 rules, but neither actor-record
  generation nor longer links is active yet. Keep old optional-only statements
  above as previous checkpoint history, not current packaging instructions.
  Serialize host save tests and campaign replays that share temporary data saves.
  Keep current evidence separate
  from historical baselines.
  Capacity, layered tiles, action/status contexts and native budgets precede
  batches of content. Keep the user's live TiEmu untouched unless asked to launch.
- Per game: engine first, then game design, graphics last.
- **Targeted validation (user decision2026-10-25):** do not rerun a full census,
  exhaustive input sweep or every module after a local edit. Select the changed
  mechanic plus its direct callers/ABI consumers; PC assertions first, native
  equality for changed portable arithmetic/layout/callbacks, relevant renderer
  checks only. Expand after a failure or when changing shared formats/layout/
  scheduler/rendering. Keep exhaustive audits explicit (`--deep`), record their
  input/source hashes and reuse unchanged evidence. Counts are not a progress
  metric. FFT impact mapping: `games/fft/campaign/CORE_TESTING.md`.
- Tests without UI first (`make test`, PC headless runs, `ti-cycles`, `make xcheck`), the emulator
  last of all (`CLAUDE.md`, development flow). Titanium only; the TI-89 HW2 only for a release.
- Every game with state has an injection door (`game_scenario(n)`, `--scenario N`, `name(N)`).
- **The user validates every visual change** before it is kept or built on. This covers
  rendering, art, palette, asset pipelines, and compression or format experiments that change
  the picture. Show before/after or A/B variants side by side, at the same scale and view,
  as image files with absolute paths. Wait for the verdict before optimising, integrating
  or benchmarking further. A pixel-difference metric is no substitute. Exempt: output proven
  identical to the reference (same checksums/planes). Details: `ti89-c-dev/SKILL.md`.
- **Image gallery decisions:** before asking for visual review, run
  `tools/bin/ti-view --decision /absolute/path/to/comparison.png` for every image
  the user needs to decide on. These previews stay first with a **Décision** badge,
  including in the GIF-only listing. After a verdict or a superseding preview,
  remove the old marker with `ti-view --clear-decision IMAGE...` (this only clears
  the gallery marker, never deletes an image or constitutes approval). Markers
  are ignored local sidecars; do not use image timestamps to simulate priority.

## The emulator (TiEmu)

- **FFT decision2026-10-24:** the user has selected1px scenery edges. Retire the
  2px banks/active toggle; older A/B instructions below are historical. Shared
  glyph-font review is accepted. New hardware QA follows `games/fft/GAME_DESIGN.md`.

- **FFT perspective decision (2026-10-23):** close/wide inspection stays in
  `experiments/fft-perspective/`, outside the campaign. Its renderer is shared,
  but supporting each map adds archived geometry/visual data as well as RAM.
  Preserve the research controls; do not plan campaign integration. Ordinary
  isometric F1/F5 low-poly rotations remain active. Keep both1px/2px Gariland
  scenery edges in the temporary in-game test build until the user tests them.

- FFT perspective: the user explicitly authorized TiEmu FPS tests after visual review.
  Compare flat and textured scenery with actors in both; separate cached replay from
  offline texture preparation. Headless TI checks and an isolated TiEmu run are authorized.
  For camera changes, report complete transition/attack durations and cold preparation,
  as well as warmed animation FPS. List native TI feature gaps explicitly: PC textured
  GIFs do not validate TI travelling textures or usable camera latency. The native audit
  and feature matrix are in `games/fft/CAMERA_AUDIT.md`.
  The user subsequently rejected animated perspective travelling and automatic attack
  shots: fixed close/wide inspection now uses direct cuts by default; motion requires
  explicit `FFT_CAMERA_MOTION` research builds. For future rendering work, native cold
  and consecutive moving frames must be measured before promising animation speed.
  GIF export cadence is never measured FPS. See `games/fft/CAMERA_CUTS.md` for the
  PC/native implementation mismatch, cache-invalidation failure and prevention rules.
  F1/F5 isometric rotations must remain animated: the no-transition decision applies
  only to close/wide perspective entry, zoom and return. Use the native low-poly turn
  fallback when moving textures are unavailable/too expensive; do not substitute a
  hold/switch. Current implementation and measurements: `games/fft/ROTATION_LOWPOLY.md`.
  An opt-in native affine-textured turn mapper now exists (`FFT_TURN_TEXTURE`), initially
  measured at 3.312-3.535 s per turn in Titanium TiEmu. Its surface/visibility follow-up
  retains 80x50 textures and three angles, measured at 2.281-2.480 s; see
  `games/fft/SURFACE_TURNS.md`. It maps runtime UVs, not cached angle
  pictures. It does not restore perspective travelling or make the PC filtered mapper
  portable. Keep low-poly as default pending the user's review; details and same-mesh
  texture-off control: `games/fft/NATIVE_TEXTURED_TURNS.md`.
  The user has now selected **80x25** as the preferred textured moving-view resolution;
  use it for future textured previews. Preserve the other variants as controls. Their
  requested low-poly 160x100 comparison is genuinely native resolution, not an enlarged
  80x50 render; see `games/fft/TURN_COMPARISON.md`. Do not infer an original-TI-89
  full-game packaging result from these bounded review executables.
  **Rotation capture correction:** the original comparison GIF presents low-poly
  steps as 2,1,3 despite correct angular math and PC/TI memory checksums. Native
  numbered-frame checks reproduce it; extra swap waits do not cure it. The local
  fixed-address plane-copy presentation control restores order. See the direction
  audit in `games/fft/TURN_COMPARISON.md`; verify actual captured steps, and do not
  silently reorder footage or reuse old timings for a changed presenter.
  The CATALOG configuration build now exposes three unchecked defaults: low-poly
  rotation, wide perspective and textured scenery. Checked values are80x25 textured
  rotation, close perspective and filled untextured mesh (the user's "filled wireframe"). CATALOG is handled before
  its legacy APPS alias; APPS retains the turn list. See `games/fft/CONFIGURATION.md`.
  Its selected forward inspection view now has an optional20,284-byte cooperative
  geometry cache prepared during menu idle time. Completed geometry survives ISO return;
  actors/masks remain live. Distinguish background preparation, prepared entry and fully
  warmed animation. Native CPU/latency evidence and limits: `games/fft/PREFETCH.md`.
  **Current storage decision (2026-10-17):** textured rotations are retired from
  the active configuration/native package, including their atlases, UV and surface
  banks. F1/F5 keep native160x100 low-poly animation. CATALOG now exposes only the
  two perspective options. Disabled attack fixture banks are also retired. Earlier
  textured-turn scripts/captures are historical controls, not shipping assets.
  Current formats, savings and measured costs: `games/fft/STORAGE_CLEANUP.md`.

- **Every TI run is a clean restart**: `ti-run prog.89z [data.89y]` (or `ti-emu restart
  file...` to send without running). Never quit a running game to HOME to resend a build,
  never type paths into the file chooser (`ti-send` is a last resort).
- **Never send keys (`ti-key`, xdotool) while the user is playing** in the emulator: once a build
  is handed over, the keyboard is theirs. Stop, restart or relaunch only when asked.
- One screenshot at most per check; prefer printed numbers or `ti-cycles` screens from memory.
- A stuck modifier or garbled typing (`fl=#bc` instead of `ffal()`): `ti-emu stop`, then
  `ti-run` again.

## Git

- Repository `github.com/sam22200/ti68k-toolbox`, committed as `coz.samuel@gmail.com`
  (`git config user.email`, set locally; the global address is a work one).
- Procedure: `.claude/skills/ti-commit/SKILL.md` (a branch per task, Conventional Commits, one
  concern per commit, explicit paths only, docs delta check, merge into `main` once the tests
  pass). Never `git add -A`/`.`/`-u`, never force-push, never commit an ignored file.
- The user's own files at the root (`*.csv`, `*.gif`, `roadmap_ti.txt`, editor swap files) are
  not ours: never stage them.

## For agents other than Claude Code

- The skills are plain Markdown: `.claude/skills/<name>/SKILL.md`, listed in `CLAUDE.md`
  § Skills and knowledge. Read the matching one before its kind of task (porting an SDL game,
  remaking a TI-Basic game, art, emulator, commit); their `reference/` files are the knowledge base.
- `/ti-commit` is a Claude Code slash command: follow `ti-commit/SKILL.md` by hand. Its
  `Co-Authored-By` trailer is the one your own environment requires, if any.
- Claude Code also kept a private memory outside the repository; everything durable from it is
  in this file and in `CLAUDE.md`.
- Game state to resume from: `games/<game>/PROGRESS.md` (FFA: `games/ffa/PROGRESS.md`, the CT
  test: `games/ffa_ct/PROGRESS.md`).
