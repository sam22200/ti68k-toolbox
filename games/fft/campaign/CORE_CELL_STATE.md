# Canonical battle cells and module boundaries

Subsequent checkpoint: [CORE_FULL_GRID.md](CORE_FULL_GRID.md) implements the
actual13-actor/256-node tactical count migration, separate topology ownership
and faster AI origin ordering. Earlier sizes and remaining count work below
are historical; source floor/render and combined residency gates remain open.

2026-10-24, workstream4, following [CORE_GEOMETRY.md](CORE_GEOMETRY.md).
The actual battle now carries compact cell identity through unit placement,
cursor, paths, movement undo, Charge, knockback and AI. Two cells with the same
x/z on different floors no longer alias in these paths. This is a tactical
state milestone, not a complete upper-floor renderer or a256-node playable map.

## Representation and active rules

The chapter Unit uses its former alignment byte for `cell`; x/z remain cached
projection coordinates. `unit_place` owns both values. `cursor_place` likewise
keeps cursor coordinates and cell together. Do not assign a new field position
by writing x/z directly. Removed actors retain their last cell, with presence
encoded separately; a gone object must not be picked up a second time.

The16-entry path stores node IDs rather than pairs of coordinates. Height,
water, animation endpoints and movement undo use those exact nodes. Pending
Charge already stores byte targets; its origin/cancellation and hit lookup now
use cell identity too. Moving a target between floors leaves a tile-targeted
Charge on its original floor. Moving the caster between floors counts as a
move even when x/z are unchanged. This preserves existing Charge behavior;
it does not add unit-targeted magic or certify new source rules.

Melee/skill targeting and hypothetical AI positions use exact cells. AI cursor
glides resolve their intermediate coordinates onto a floor and finish on the
planned node, including a change of floor with identical x/z. Knockback retains
the floor when possible and otherwise resolves an available lower landing;
fall damage uses the selected node height. These synthetic floor connectors and
landing policies remain TARGET adaptations pending original bridge traces.

Feet-cache keys include cell identity and camera orientation. Standing height,
path endpoints and tactical markers read the selected node. Scenery/depth
ownership and source-correct upper-floor occlusion are still separate gates.
`battle_bind_grid` validates a tactical grid and its coordinate planes before
changing live bindings; a future content frontend must supply matching view,
depth and geometry banks. The fixture does not render mismatched scenery.

The topology cache allows16 outgoing links (four directions, adjacent/leap,
two destination floors). A leap with no intermediate node on its source floor
uses a separate kind bit; its via payload is ignored. No tile ID is reserved
as a missing-node sentinel. Native movement costs retain the same table and
stable queue order for existing links.

## AI code and executable bank

AI preparation/facing and candidate evaluation now occupy separate overlays.
The resident dispatcher retains the same staged frame schedule. A module never
replaces the executing overlay: it returns to resident code before another load.
The formula evaluator remains local to AI evaluation where required.

FMB2 adds an explicit module count and contains seven images. The resident
loader validates the entire directory before execution: count, aligned offsets,
nonoverlapping spans, image sizes, entry/context bounds, FMD1 signatures and ABI.
Only the optional six-byte AMS OTH tail may follow the images. `MOD_ABI=2` also
invalidates old same-sized layouts. Rebuild the core and every module together;
FMB1 is rejected. This is a module-bank contract, not a frozen map/scene v1.

## Verification and measured cost

- **128 actual battle checks**, PC, ASan/UBSan and68000, hashEEB37732:
  independent same-x/z occupation and reach; cell-preserving cursor/path/undo;
  an upper leap with no intermediate node; Charge stays on its floor when a
  target moves and cancels when its caster changes floors; replacement target;
  AI hypothetical placement and cursor; upper knockback/fall; feet height/key.
  This asymmetric nine-node fixture permutes IDs and uses real battle actors
  and formulas, but supplies synthetic tactical geometry, not PS1 floor proof.
  Legacy signed-shift checks are disabled only for the battle sanitizer run;
  the new bank validator runs with all undefined-behavior checks enabled.
- **68 module-bank checks**, PC, full ASan/UBSan and68000, hash0000011C:
  raw/OTH banks, every truncated length, missing tiny single-module headers,
  zero/wrong count, odd/overlapping offsets, wrong signatures, oversize images,
  invalid entry and context bounds.
- The shared256-node kernel retains13887 checks/hashD1F77B11. All91186 frozen
  movement comparisons and1837 boundary checks pass on actual maps and the
 13-actor descriptor. Root/campaign/opening PC suites and16 audit tests pass.
-37 campaign and24 opening/scene/control native replays pass. **169 existing
  PNGs are byte-identical**, including the new13-actor render output rather
  than reusing the previous checkpoint's images. Nine fixed11-capacity control
  pictures also match. No visual reference was updated.
- Native cancellation288 cases, rotation144 cases/80 frames/81840 independent
  corner checks,48 original Charge trials and95 full-PC flow assertions pass.
  Native flow phases verify tactical pixels, save ownership after Work release,
  receipt idempotence and overflow payout. Pending save storage is still the
  tail of State's longer-lived allocation, never the released Work cache.

Actual compiler layouts and package, compared with the geometry milestone:

| Item | Previous | Current | Change |
|---|---:|---:|---:|
| Unit |80|80|0|
| State |1494|1480|-14|
| State plus deferred Save allocation |4632|4618|-14|
| Work |44434|48046|+3612|
| Largest scene/Work heap block |60594|64206|+3612|
| Sum of four requested battle heap blocks |107722|111320|+3598|
| Largest executable overlay |8868|7404|-1464|
| Private campaign group |464820|465566|+746|

The four requested blocks are4618,32320,64206 and10176 bytes. This is an
allocation formula, not measured heap high-water, stack/fragmentation or combined
supervisor/code/module/grayscale residency. The enlarged topology cache accounts
for3600 bytes; the floor/coordinate bindings add12. The64206-byte block leaves
little per-block headroom: the full256-node integration needs another memory
layout, not just larger array constants.

Seven raw images are5004,3452,7404,5732,6072,1040,4208 bytes. FMB2 totals33002
raw bytes; its OTH payload is33008. The packed battle payload is40224 bytes.
AI splitting reduces the largest reserved code image while retaining the prior
staging and existing pictures. It does not claim faster complete battles.

On current movement fixtures, compact limit4 searches average47827/41429 cycles
(Orbonne/Gariland), peaks79538/82314. Ordinary13-actor board/turn peaks are
266564/329474 cycles, Status/equipment239414/262622. Cold, turn and existing
full-fight/story outliers remain separate and can exceed360k; no universal30fps
or hardware/LCD timing claim. All48 native decode repetitions retain their
previous cycle counts. Compiler output for `node_at`, `unit_place`,
`cursor_place`, `edges` and `spread_go` has no32-bit multiply/divide or float
helpers; fixed topology rows use shifts. Cold edge construction still uses
16-bit direction multiplies (`compiler-review.json`).

The private candidate is `x/core-floors/fftcampaign.89g`,42 variables,
464476 payload bytes plus1090 transport bytes. SHA256:
`e77d7a177309b057fa20e7d450753cfd8695df627219ba249d99834e5e6ed26b`.
It adds746 bytes to the previous private milestone and is479 bytes smaller than
the unchanged466045-byte standard group. `package.json`, `native-budget.json`,
`source.json`, `image-regression.json` and the fixture logs are under
`x/core-floors/`. The existing ffca64000-byte planning alert remains unchanged.

## Scope still open

- The actual battle still has150-node arrays and byte counts. The separate
  compact kernel supports256 IDs, including255, with16-bit endpoints. Do not
  raise `MAP_NMAX` and infer full256-node battle support.
- Full-node loops, AI pending counts, target validity, long paths and maximum
  allocation layout need their own migration and boundary fixtures.
- Source upper-floor traversal, special movement, height/solid-span line of
  sight, cover masks and archived per-map topology need original traces and
  asymmetric rendered fixtures. No new map or floor artwork is shipped here.
- Save persistence is still FSV3/3138 bytes. Deployment descriptors currently
  provide lower-floor positions; this is not a new layered save/data format.
- B06/B07 native LCD ghost reports, action/status families, persistent guests,
  formats/editors and the production gate in [CORE_AUDIT.md](CORE_AUDIT.md) remain
  open. The user-owned live TiEmu and standard bundles are untouched.

## Reproduce

```sh
rtk proxy make -C games/fft/campaign core-cell-check
rtk proxy make -C games/fft/campaign core-grid-check
rtk proxy python3 games/fft/campaign/check_core_boundaries.py \
  --out games/fft/campaign/x/core-floors/boundaries
rtk proxy python3 games/fft/campaign/check_capacity_render.py \
  --out games/fft/campaign/x/core-floors/render
rtk proxy python3 games/fft/campaign/check_flow.py --native \
  --out games/fft/campaign/x/core-floors/flow
rtk proxy make -C games/fft/campaign bundle \
  BUNDLE_OUTPUT=x/core-floors/fftcampaign.89g \
  BUNDLE_BUDGET=x/core-floors/package.json
rtk proxy python3 games/fft/tools/core_budget.py --native \
  --group games/fft/campaign/x/core-floors/fftcampaign.89g \
  --out games/fft/campaign/x/core-floors/native-budget.json
```
