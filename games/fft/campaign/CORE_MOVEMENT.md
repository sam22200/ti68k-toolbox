# Equipped movement and terrain: delivery block A

2026-11-03. The complete first delivery block adds fifteen generic-job movement
capabilities: Move+1/+2/+3, Jump+1/+2/+3, Ignore Height, Any Weather, Any Ground,
Move in Water, Walk on Water, Move on Lava, equipped Float, Fly and Teleport.
Block B (equipment/progression) follows; the next work is not another isolated
reaction candidate. The remaining generic R/S/M count is35:12 reactions,
18 supports and five movements (four movement benefits, Move-Find Item).

## Source evidence and bounded native model

`tools/movement_oracle.py` executes unchanged retail MIPS for128 movement
configurations (all15 IDs plus normal controls, Float/Frog/Chicken and weather),
16 Move/Jump bonus boundaries and24 Teleport trials. Original function/disc
hashes and full cost rows are recorded in `x/core-movement/oracle/source.json`.
Weather returns and RNG are controlled; this is not an original full path/AI,
actor-column, mount or animation replay. Existing source terrain and ordinary
movement/Float fixtures remain the reference for unchanged geometry.

Movement selection is loaded before deriving saved-party statistics. One byte
per unit encodes the source ID486..509 as1..24, with zero for none. Three
archived source bytes per job retain innate movement; equipped and innate bits
are ORed. Move/Jump bonuses are additive across distinct bits, with source
stat caps252/7. Battle stats and management previews use the same bonus helper.
The five benefit/discovery IDs remain outside the completed producers.

The fifteen capabilities share navigation and its existing storage. Terrain
costs come from six64-byte original rows; source0 means weather-dependent,
255 stays forbidden, Any Weather fixes weather cost1, and Any Ground flattens
nonzero traversable costs. Severe weather uses the existing effective weather
state. Dry slow surfaces are profiled once at map binding. The ordinary pilot's
previous water surcharge/spare-point adapter is retained where no special
policy, dry slow terrain or severe weather requires the new table path.
Original normal dry-to-water differences remain explicit, as in CORE_FLOAT.md.

Custom costs use the existing eight-bucket search storage and shared destination
cost tags; narrow/wide results are compared on their common bounded-cost domain.
Player reach/path and native AI use the same prepared policy. A default human
with unchanged geometry retains the previous fast path without a new image
load per actor. Capability, job, status, weather and Jump changes invalidate
special preparation. Starting another query resets search ownership.

Ignore Height raises traversal clearance to31 while retaining legal landings,
slopes, body/ceiling tests and the existing one-gap graph. The three water/lava
policies share source costs and landing-depth/lava restrictions. Equipped Float
also supplies innate Float17 at status initialization: Earth immunity applies
and ordinary Dispel cannot remove it. This is distinct from the spell's mutable
Float and its pre-existing height/immersion adaptation. Reviewed actor art and
floor-based targeting/painting are retained; no hover art or new height model.

Fly uses a bounded direct Manhattan range within Move; Teleport enumerates
valid destinations across the current source layer. Both use existing path and
walking presentation, skip intermediate occupancy/obstacles and retain legal
landing/tree/occupied/deep-water/lava filtering. Full original layered routing,
stepping stones, mount/actor columns and flight height animation remain open.
Teleport consumes exactly one source-style random draw after a legal target:
`floor(rand15*100/32768) >= (distance-Move)*10`. Preview/path rejection is RNG
free. Failure leaves the actor in place and still spends Move; both success and
failure disable Move undo to prevent rerolls. Independently armed actions and
ordinary turn settlement retain their existing ownership.

## Code placement and archive contract

ABI64 retains ModCtx360 and the single logical executable zone; there are now
19 images. MOVEMENT18 owns cold graph construction and terrain preparation.
Resident wrappers prepare before entering AI_SETUP or TARGETS; an executing
image never loads another image through a resident callback. Native SCREENS
and portable input share Teleport's undo rule.

FMB3/FMD2 retain their existing record layout and zero-context reconstruction.
Three matching shards, ffbm/ffbn/ffbo, replace the nearly full pair, split6/6/7.
All three headers, contiguous IDs, bounds, signatures and common export token
must validate before execution. Old ABI63 exports, missing/corrupt third shards
and mixed exports reject. No decompression, new ASM or additional heap block is
introduced. The catalogue is FCJE: six cost rows and160 innate bitsets add864
archived bytes; existing ability records/order and201/201 generic actions stay.
The save remains FSV3/Save3138/SaveUnit176. Exact compiler layouts, archive sizes,
actual TTPack input and five allocation formulas are recorded in verification.

## Verification and limits

`make core-movement-check` compares original rule rows, real production movement,
saved-party load/stat refresh, map binding, terrain/weather costs, both search
queues, occupancy/incapacity, high destinations, barrier crossing, native AI,
Teleport RNG/commit/undo and equipped Float persistence/Earth protection.
PC, ASan/UBSan and actual68000 images must agree. Real Gariland reach/arrival
screens use the existing renderer in all four camera orientations; no new art,
projection or menu style is introduced.

Scoped legacy navigation/AI/action/status/area regressions, actual three-bank
bindings and prior-export rejection, private Gariland/Orbonne saves/pixels,
management/common-catalogue checks and protected-file evidence are in
`x/core-movement/verification.json`. Cycle zones distinguish cold preparation
from warm queries; these are CPU samples, not full-frame30fps certification.
Cold special reach can exceed one frame. Whole-frame/cold rendering, stack,
combined residency and release gates remain open. The user's live TiEmu and
standard campaign group are not changed.

Final family evidence:9556 source-rule assertions (9E393AA0),288 runtime
assertions (D31EBB7C),113 UI assertions (58EFA3C4) and32 pictures agree on PC,
sanitizers and actual68000 images:9957 assertions total. Real-map cold reach
peaks at3243404 cycles and warm reach at316986 in these fixtures; geometry and
archive loads account for the cold path. The UI assertion does not certify a
whole rendered frame at30fps.

Native layouts are Unit112/State2224/Work45582, SaveUnit176/Save3138;
State grows14 bytes and Work6. The five allocation formulas remain, including
5362 bytes for battle State+Save and61742 bytes for scene+Work. The logical
image zone is8640; the three archives are43440/44018/44938 bytes, leaving
20560/19982/19062 below64000. FCJE is44538 bytes. Actual non-BENCH TTPack input
is64175/64936 (761 bytes left), confirmed both by the raw production variable
and the packed TP header; packed data is39327 bytes. The instrumented raw
carrier differs and must not supply the production headroom number.
