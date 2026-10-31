# Cached current-world area AI

User follow-up: [CORE_AI_PACING.md](CORE_AI_PACING.md) accepts the frozen setup
peak inside the requested pause and fixes additive waiting. The30fps failure
below remains historical measured evidence, not the next optimization priority.

2026-10-28. Follow-up to [CORE_AREA_FORECAST.md](CORE_AREA_FORECAST.md). The actual
AI ability loop now stages ordinary hostile formula08 area planning, ranks empty
or occupied centres and sends the chosen fixed cell through cast_order. Retail
casts and unreviewed area UI/presentation remain disabled. This is the current
immediate-HP AI subset, not a complete original future-turn/status/MP-utility AI.

## Data, scoring and candidates

Read original AI flags from existing CJ_PRIMARY records; no catalogue extension
or map/ability-ID exception is introduced. Support is explicitly limited to the
ordinary hostile HP magical family: primary flags byte0=0x82, ordinary silence/
Faith/follow/reflect flags, magical trajectory class2 and usable/enemies-only
policy. Random-use, direct/arc, stat, MP, status-proc and other families reject.
Use the shared cast-family validator, resource/control policy and range helper.

Cache the thirteen current per-victim quantities/probabilities and HP-worth
changes once per ability. Recompute the actor's own entry when changing origin,
including location-dependent magic evasion. Footprints still use the source-backed
selector, including both layers/blocked surfaces/team filtering; no formula is
repeated for every centre. Absorption uses the shared signed HP transform. Native
immediate worth retains the existing HP/Critical/KO/Poison/Darkness model.

Original hit accounting uses an arithmetic mean across counted target/strike
pairs, not chance-weighted HP scores. Hostile HP attacks subtract one point for
an affected ally, even when absorbed. A candidate needs at least one counted
useful target. Rank by signed priority, then mean hit percentage, then origin
proximity; equal keys keep the earlier centre. Scan centres by source layer/row/
column rather than compact node number, so reversing compact IDs preserves ties.
Initial candidate filtering includes empty panels near useful targets and all
walkable panels when useful self-healing can move with the caster.

Origins reuse the existing AI order. A first non-adverse origin consumes its
centre; centres with adverse self-hits remain available for safer origins. Do not
claim general original deferred CT/follow groups, Reflect, Charm/monster policy,
all canonical status utilities, global MP utility, reactions or source event
simulation. Those remain explicit L2/L3 contracts before broader content enablement.

## Ownership and native execution

Work aliases an84-byte AreaAi cache with its idle256-byte ai_dist array, only
after focus setup/search finishes. Stages4/5/6 initialize effects, filter centres
and evaluate origin/centre pairs. A new thinking run overwrites that scratch via
the existing setup; weather invalidation restarts it. Unit positions are restored
before every return, so rendering never observes a simulated move. Failed or
resumed scoring does not spend MP or consume battle RNG; the existing final
ranked-action chooser retains its legitimate random draws.

A new AREA_AI image (ID13) keeps the existing AI/scalar image from absorbing the
whole area selector. AI returns before the resident loads AREA_AI; AREA_AI uses
local footprint/forecast/rank helpers and pure resident callbacks, never another
image loader. It returns before the resident advances to the next ability. One
execution zone remains; no per-candidate image ping-pong or heap allocation.

AI_AIM now submits a fixed cell for area incantations, including empty cells.
Rejection returns to thinking without spending the action. The route requires
both AF_CAST and AF_AREA: the existing0x9b weapon splash has AREA alone and must
keep its old preparation/execution path. No visible spell producer is enabled.

## Source and validation

`tools/area_ai_oracle.py` executes original score-state, hit-accounting, rank and
empty-panel scan instructions:12 profiles of13 supplied target states, five rank
comparisons and three empty source panels on two layers. It isolates action
preparation/effect/reaction/range/coordinate/VSync calls; source/kernel/disc hashes
and those supplied boundaries are in x/core-area-ai/oracle/source.json. Geometry,
formula and preview evidence from earlier milestones is reused. This is not a
full original battle AI decision trace. No geography/Jump census is repeated.

The focused fixture exercises real ai_think and AI_AIM: empty two-enemy optimum,
friendly fire, allied/enemy absorption, nulls, resource/unsupported-policy
rejection, weather restart, compact-ID reversal, legacy weapon splash, a paid
empty-centre cast and thirteen actors/256 nodes on two layers. A whole-map footprint keeps its first legal centre at each origin instead of
rescanning equivalent panels. A mid-plan forecast can evict AREA_AI and resume
from the retained cache; late MP rejection returns to planning. Every quantum
restores unit bytes; area search preserves RNG. A separate callback-count trial
measures four quantity calls for the one-origin search (three initial live
victims plus the moved actor), rather than one full forecast per candidate.

## Performance gate remains open

The new area stage is bounded to64 scanned positions and at most six footprints
per quantum. Initial candidate filtering and cached effects have separate stages.
Native timings below are backend quanta, not full rendered-frame certification.
The stress fixture also measures the general planner separately: its existing
setup/reach phase exceeds360K by a large margin. Do not hide that peak by quoting
only area timings or mark C07/L4 complete. Next priority is to bound that general
setup work before broader production or a hardware latency claim.

Detailed checks, cycles, layout/package numbers and preserved artifacts are
recorded in x/core-area-ai/verification.json. Keep live TiEmu, the user's standard
group and existing gallery bytes/dates unchanged. Jump refinement remains deferred.

## Final checkpoint budget

676 PC/ASan/UBSan/native assertions agree at D04607FA, including the whole-map
stress fixture; max64 planner steps in this fixture set. The cache-count trial
uses four formula calls at one origin. Native area-stage quanta:125 calls,
average103868, maximum319968 cycles. General planner:96 calls, average513927,
maximum12059828 in stage0, about1.005 seconds at12MHz before display/interrupt
costs. The general360K gate fails explicitly. These are ti-cycles datasheet
measurements, not TiEmu timing or a hardware latency certificate.

Affected closure: pure forecasts359/7E9BBEF7; owned area CT353/11D6FCB4 plus5
Orbonne retries/00000230; magic/actual AI1352/B5895FED; primary actions295/EDCEBFAE;
bank parser130/00000047 and138 binding checks/1713233A across28 cold/warm loads,
three stale retries and seven startup failures. Both pilot flows/saves, twelve
old frame hashes and fresh twenty-field PC/native layouts agree. Reuse unchanged
geometry/catalogue/source formula evidence; no broad map/Jump sweep.

ABI32, ModCtx364, signature63878, fourteen images. The largest image grows14 to
6218 bytes; the cached archive pointer directory grows4 to56 bytes. The new
AREA_AI image is4608 bytes; raw FMB3 shards34464/36646. State2192, Unit112,
Work45576, Save3138/FSV3 and the same five heap requests stay unchanged. The
84-byte cache borrows existing idle scratch; directory/zone static growth is
18 bytes, distinct from the unchanged heap formulas.

Private group509442 bytes (+4838 versus the area-forecast checkpoint), still43
variables and28 byte-identical non-code banks. Resident TTPack input64447 leaves
489 bytes below64936; budget further resident growth before activation. The
standard466045-byte playtest group and all pre-existing gallery bytes and dates
are preserved. Full core/content production remains closed: general setup latency,
reviewed area UI/presentation, broader action/status/campaign families, formats/
editors and combined CPU/RAM/stack/residency gates remain outstanding.
