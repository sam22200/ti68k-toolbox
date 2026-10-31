# Pure compound formula08 forecasts

Follow-up: [CORE_AREA_AI.md](CORE_AREA_AI.md) now connects cached immediate area
planning to actual AI. The query contract below is unchanged.

2026-10-28. Follow-up to [CORE_HEADROOM.md](CORE_HEADROOM.md).
`cast_forecast(&packet)` now predicts the current supported area footprint and
per-victim accuracy, signed quantity and typed outcome without changing State,
MP, battle RNG, HP, statuses, rewards, pending casts or the live Work packet.
This is an effect-query API. The area AI chooser, binding/HUD UI and reviewed
multi-target/empty-spell presentation remain gated; retail spells stay disabled.

## Contract and ownership

The caller owns a60-byte CastBatch and supplies actor, ability, reference and
follow. Follow0 selects a compact cell (255 is valid); follow1 uses a stable
actor slot. Successful output has phase CAST_BATCH_FORECAST, membership mask
and thirteen four-byte records. hit is a0..100 percentage in this phase, rather
than the execution boolean used by READY/APPLIED. All unused/blocked/absent
records are cleared to zero amount/hit and AB_NULL. Empty footprints succeed.

The query accepts EMPTY or an existing FORECAST result for refresh. READY,
APPLIED, invalid references/actors/abilities, unsupported families and missing
raw geometry reject before any output write. The dedicated W->cast owner is
always rejected as output, including when empty. Existing prepare/commit reject
FORECAST packets; a display probability can never accidentally become an impact.
Caller storage is trusted internal data, not a serialized/untrusted format.

Membership uses the source-checked radial selector and its one-floor fast path.
Final effect eligibility uses the existing absent-versus-blocked policy; typed
null/absorption, source magic evasion, Faith/support/status/element/weather
arithmetic, positive999 cap, signed narrowing and protected Oil removal share
existing rules. magic_quantity.inc centralizes the cap in old single-target
forecasts, execution preparation and the new area query. No execute/proc draw is
performed. An Oil bit predicts removable Oil; it never mutates the target.

This is a current effect-stage query, not initial command eligibility. It may
be used while a paid action owns a root or with insufficient remaining MP.
The UI/AI must separately enforce command cost/status/range/binding rules before
offering or executing an order. A followed centre uses its current coordinates;
the query does not simulate future CT, movement, reactions or expected AI value.
No hypothetical actor move is silently performed by the evaluator.

## Native boundary

TARGETS3 consumes the existing transient CastBatch pointer. The resident wrapper
preloads the image before assigning that pointer; no executing overlay may call
this resident loader or evict itself. Future native AI integration must return
or stage its work before crossing that boundary. No ModCtx callback, image,
State/Work field, save field or heap request is added. ABI31 protects the new
operation and FORECAST phase contract; ModCtx stays364, signature63621, zone6204.

## Source evidence and focused checks

`tools/area_forecast_oracle.py` runs the original presence/formula08/finalizer
instructions in preview mode2 for four groups of thirteen slots (52 inputs).
It reuses the compound fixture inputs after checking disc/kernel hashes rather
than regenerating the unchanged geography/trait inventory. Cases cover evadeable
and sure magic, mixed statuses/absence, null/absorption/resistance, removable and
protected Oil, Faith and signed power255 stress. Normal weather and ordinary
unmounted human scope remain explicit. Existing environment/footprint evidence
is reused; this is not a full original AoE UI/AI/dispatcher trace.

The original still calls its evasion RNG helper in preview mode, though failure
does not set an execute miss. The oracle observes/stubs those calls to pass;
the port deliberately omits them, preserving its established pure-preview rule.
No execute proc draw is observed in preview. Source/function/input hashes and
observed calls are recorded in x/core-area-forecast/oracle/source.json; the
FBF1 fixture is1518 bytes and remains outside production archives.

`make core-area-forecast-check` passes359 PC/ASan/UBSan/actual68000 checks,
hash7E9BBEF7. It checks cold/warm/refresh equality, live paid root and Work packet
protection, failed query purity, explicit phase guards, insufficient-MP effect
inspection, empty cleanup, moved follow versus fixed tile, four rotations and
256 compact nodes across two layers, including cell255. Synthetic topology is
explicitly rebound to the real chapter before its scenario reset.

Cold query average81566/max110258 cycles; warm57812/max86504. These are complete
backend query zones including the wrapper, not full HUD/frame timings. A warm
worst case exceeds the old60K optional-task heuristic. Do not call it repeatedly
per AI candidate without bounded scheduling/cache reuse. C07/L4 full-frame,
stack/fragmentation and combined residency remain open.

## Storage and affected closure

Native State2192, Unit112, Work45576, ActionPacket22 and FSV3 Save3138 are
unchanged; fresh host/native probes retain all layout/allocation fields and the
same five heap requests. The caller still supplies60 transient bytes. These are
layout/formula results, not peak stack, fragmentation or combined residency.

Private group504604 bytes, +391 from504213; only executable payloads change:
ffbcore+15, ffbm+6, ffbn+370. Resident TTPack input64295 leaves641 bytes (-2 from
the prior margin); the forecast wrapper remains subject to dead-code removal
until a live producer references it. Budget its caller/storage before activation.
Raw FMB3 shards34396/32002; thirteen images, TARGETS3430 (+378), COMMAND5558 (-8),
AI_SETUP4976 (+6), maximum6204 unchanged. All28 non-code banks/FCJA bytes remain
identical. Signature/enum/helper changes can alter compiler output even when an
image retains its length; do not infer binary equality from sizes.

Shared-quantity closure reuses the old original scalar/trait/environment/geometry
inputs. Batch1202 (94558E80), owned-area353+5 (11D6FCB4/00000230) and magic/actual
AI1352 (B5895FED) keep their PC/sanitizer/native outcomes. Area command/preparation
now costs130582 cycles (was127872), maximum due launch88018 (was85376), impact
41372 unchanged. The larger cold TARGETS copy is included; this is not a speedup.

Pilot Gariland81/Orbonne14 flow checks, actual native save equality, twelve old
action/reaction frame hashes and eleven pilot image bytes/dates pass. Native
binding/retry/parser tests reject the previous ABI30 as well as malformed banks.
Proof and artifact/source hashes: x/core-area-forecast/verification.json. The
standard user group and live TiEmu remain untouched; old gallery dates are kept.

## Follow-up

Next: source-backed area AI candidate generation, friendly-fire/absorption scores,
empty-centre choices, stable ties and bounded native scheduling/cache ownership.
Then connect reviewed targeting and per-victim/empty-spell presentation. Keep
other unsupported status/reaction families, retail casts and content-production
gates closed until their fixtures/contracts pass. Jump remains deferred.
