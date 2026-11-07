# Approved area impacts in the shared engine

2026-10-28. User approval: “ok and continue”. The accepted presentation is now
shared PC/68000 code. It covers the ordinary supported formula08 batch family;
this does not enable retail magic or complete the full-game core.

## Visible behavior

Every victim gets its own prepared result: outlined HP quantity, absorbed healing
with a plus sign, or Miss. Successful harmful impacts share the existing flash/
hit pose; the caster keeps the existing wind pose during preparation. Oil removal
has the accepted Oil- line. No extra sprite, font or spell bitmap bank.

The pulse uses the selected tile's real slope and cover mask, including a centre
without an occupant. Zero victims do not display a fake Miss on the caster.
The number renderer waits for APPLIED; reaching ACT_HIT with a READY packet can
no longer cache a speculative scalar Miss.

The root effect is suppressed during secondary reactions. Auto-Potion/Counter
use the unchanged scalar presentation. EXP/JP and terminal checks retain the
existing response/reward drain; painting never rerolls, pays or applies damage.

These are approved TI effects, not original Fire artwork or a per-spell PS1
timeline. Wider status/Reflect/special-action families remain separate work.

## Storage and dispatch

- AreaImpact is412 bytes and aliases the existing512-byte ai_gain table. That
  table is consumed only while evaluating plans; the action starts after that
  stage. Replanning rebuilds it. Live CastBatch and the inspection cache remain
  separate. No added allocation or State/Unit/save capacity.
- Store thirteen seven-row glyphs, one shared Oil glyph, widths/readiness and
  a16-bit centre. Reconstruct the existing white-outline masks when blitting.
  Tile255 remains valid; MAP_N denotes an absent resolution centre.
- Prepare at most one victim glyph per wind-up update, then Oil when needed.
  Thirteen victims plus Oil fit before ACT_HIT16. Numbers are not generated
  synchronously during impact or every render.
- Both owned cast_order dispatch and public preparation into W->cast initialize
  this cache. Private caller packets cannot accidentally claim its presentation.
- Resolution-centre binding shares area_resolution_cell with the footprint query.
  Followed Crystal/Jump coordinates remain valid; a vanished off-field centre
  yields MAP_N. This is separate from command eligibility for a new order.
- The camera uses the frozen resolution tile, not the last victim visited by
  commit. Existing view_of/scene_x/scene_y and height/layer identities are reused;
  no unit relocation or new projection convention.
- ABI34/ModCtx372/signature64400, sixteen images. New NUMBERS image2416 owns
  unchanged scalar number drawing plus area preparation/blits. Pure callbacks
  already exist; no new callback or executing-image eviction. Image IDs0..14
  remain stable. Largest image6264 and split8/8; raw banks41072/38606.

Resident TTPack input64473 leaves463 bytes, versus64719/217 before this change.
Private group517737 bytes (+2173);28 non-code banks are identical.
Packing/group comparison and fresh host/native layouts are recorded in
x/core-area-impact. The five core allocations remain the layout baseline;
archive bytes do not certify combined peak RAM/stack.

## Scoped verification

`make -C games/fft/campaign core-area-impact-check` checks the actual renderer
and the13-victim/tile boundary. Evidence: x/core-area-impact/verification.json.

The accepted PC candidate is reconstructed using frozen source dependencies;
416 candidate captures match actual staged rendering byte for byte with the
same scenario/cache history. Its immutable historical record remains
CORE_AREA_IMPACT_REVIEW.md / x/core-area-impact-review/approval.json.

The full-render check covers eight fixtures: empty/occupied centre, real magical
evasion, friendly fire, absorption, no victims, KO/Oil, final victory and allied
Auto-Potion. A compact portable fixture bank keeps the native checker below the
TI variable limit without changing game allocations or replacing the renderer.
That test bank is not packaged. Compare64 sampled full framebuffers individually,
State/live-packet/RNG purity and rule outcomes across PC/sanitizers/68000.
There are1012 assertions12D9C238, with64 individually equal screens whose
aggregate isD7A684DC. The boundary suite passes154 assertionsC56EC8B9 and its
number-backend hash9DE20073.

The separate boundary checker covers13 cached rows, both floors/node255, signed
HP quantities, full wind-up readiness, Oil and Crystal/Jump/vanished centres,
including public preparation into the Work owner. Its picture is a number-backend
check, not a full13-actor tactical framebuffer certificate.

Shared ABI/scalar rendering/ownership require scoped loader bindings, earlier
action/reaction frame hashes, owned CT/batch, targeting UI, magic/area AI, elapsed
pause, fresh layouts and pilot victory/save flows. Exact metrics are in the local
report. No geography/Jump census, new original full-game trace or live TiEmu run.

Timing remains a gate: cold orientation decompression and complete frame/stack/
residency budgets are not certified by glyph or average-frame measurements.
Keep interactive frames separate from the accepted frozen AI thinking allowance.
Very wide footprints also need a production camera/visibility stress fixture.
Measured peaks:20178 cycles per staged glyph,96350 for the cold13-number
backend,107786 for an update in the actual eight-case run, and2528106 for its
cold full render including changed-orientation decompression. These different
zones do not constitute a universal360K full-frame pass.

## Next core work

Continue shared scheduler/status edge cases, including Quick priority and clock
liveness when units cannot advance, then the remaining action/reaction families.
Broader AI policies, campaign/progression, validated formats/editors and full
RAM/stack/CPU closure still precede mass map production. Jump remains deferred.
Tell the user when those full gates actually close; this milestone is not that
completion signal. Preserve standard playtest, user saves, live TiEmu and old
gallery bytes/dates.
