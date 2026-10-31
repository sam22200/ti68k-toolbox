# Shared portrait/stat presentation

2026-10-23. `../unit_card.h`, `../unit_card.inc` and `../portrait32.inc` serve
management, deployment, battle HUD and battle Status. This is one portable
presentation implementation, compiled into each screen's owning program; no
additional loadable overlay or graphics bank.

`UnitCard` is a transient snapshot: portrait-bank pointer, name/job strings,
current/maximum HP/MP, level, EXP, Brave and Faith. Management adapts `SaveUnit`
and `Stats`; battle adapts live `Unit` records. Rule/stat calculation stays with
the existing rules. Sprite/equipment banks and save formats are unchanged.

Layouts: `UC_FORMATION`, `UC_MANAGEMENT` (page header), `UC_BATTLE_LEFT`,
`UC_BATTLE_RIGHT`, `UC_STATUS`. Parts select portrait/name/HP/MP/level/job/EXP/
Brave/Faith. Common code owns portrait decoding, numeric/ratio formatting,
alignment and full/flat/rounded bars; font adapters select plain menu text or
the existing outlined battle writer. `CARD_MANAGEMENT_LAYOUTS` keeps unused
battle layouts out of management and vice versa. No allocation, new persistent
buffer or runtime scaling. HP/MP<=999 and width<=60 keep bar math unsigned16-bit.

Callers own caching: battle retains its five incremental parts, two completed
panel buffers, portrait opacity stamping and completion handoff. Status retains
its line-by-line cache and portrait/status union; its overlay invokes the core
renderer through `ModCtx.unit_card_battle`. This avoids an additional overlay
switch while constructing HUD parts. Deployment retains its selected-stat cache.

`MOD_SIG` now includes `sizeof(ModCtx)` as well as state/work sizes, because the
callback changes the context ABI. Rebuild `ffbm` and core together. Old overlay
banks must not be mixed with these programs. The live FCJ4 group is untouched.

Validation: campaign unit tests;29 campaign and12 opening battle +6 scene native
replays; all preserved reference PNGs byte-identical, no reference refresh.
Five updated deployment replays agree PC/68000. All74 Gariland +14 Orbonne flow
assertions, saved bytes, repeat payout and trophy overflow checks also agree.
Instrumented flow checks are split by chapter to stay below the per-variable
limit without dropping assertions; these are not full-package size results.
Status/cache and deployment pass ASan/UBSan, excluding legacy shifts/leak accounting.
The root FFT unit suite and opening unit/regression/presentation suites also pass,
including648 exact walking poses and the existing portrait-mask checks.

## Placement candidate

The full5x5 floor is an outline-only grid at depth0. Legal slabs retain four-pixel
sides; inactive cells have no filled face or side. The original bottom-left arrow was superseded by the screen-space south
indicator beneath the grid (later refined to the user's source-photo wedge); see [CHOICE_MENU.md](CHOICE_MENU.md).
It is separate from the tactical unit-facing convention (+x down-right,
-x up-left, +z up-right, -z down-left).

Top rims are completed after side faces and before actors. Thin dark outer
keylines keep white left/right tips visible against the white background.
Light-gray floor lines remain visible without restoring rejected filled inactive
tiles. Bounded integer C only; no handwritten assembly.

Review-4 was superseded by the source-photo wedge/animated shared choice menu in
`x/qa/flow/review-6/new/deployment-slabs-review.png`, now marked as a gallery
decision. Old images and other pending decisions remain.
Visual approval precedes full packaging, changed-presentation benchmarks or a
hardware launch.
