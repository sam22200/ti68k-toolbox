# Native ballistic weapon interactions

Current panel/preparation ownership: [CORE_PANEL_VICTIMS.md](CORE_PANEL_VICTIMS.md).
The checkpoint below is historical.

This extends the interactions delivery after CORE_MELEE_COLLISION.md. Generic
coverage remains201 actions and78 R/S/M consumers; no family is reopened.

## Observed source and native model

The retail arc dispatcher first tries the lower launch slope, then the higher
one. A clear path wins over an intercepted path. If both fail, a blocker whose
original camp differs from the shooter's current camp takes priority, lower
arc first. The final friendly-unit/terrain tie has its own asymmetric rule.
arc_oracle.py runs these branches unchanged on128 controlled trace outcomes,
including Charm-style current/original camp differences. Eleven unchanged
source peak queries confirm the gravity/range scale. Traces and geometry
providers are mocked in this dispatcher oracle; it is not a complete PS1 shot.

The TI uses gravity168 in original world units, a36-unit human body (24 when
Frog/Chicken,12 for a KO obstacle), a source at two-thirds body height and a
target at half body height. An unreachable half-height aim retries the feet.
The redundant source three-quarter-height test is omitted: it cannot pass if
the lower half-height test failed. A negative discriminant rejects the shot.
The original execution fallback to a45-degree unsolved shot is not reproduced.

For horizontal offsets dx,dz and upward endpoint rise h, the two parabolic
coefficients are168-h +/- floor(sqrt(168^2-336h-dx^2-dz^2)).64 fixed horizontal
steps replace PS1 trigonometric/impulse stepping and its fixed-point rounding.
This is an explicit native adaptation: thin solid intersections, nonhuman
sprite heights, Float/cooccupant/mounted endpoint details and exact PS1
trajectory rounding are not certified. Existing source column top/underside
and strict unit-box boundaries are reused for both terrain layers.

## Integration

PLANNER24 resolves ballistic weapons at the shared preparation point, before
evasion, critical and proc draws. The actual intercepted actor becomes the
damage recipient; the selected panel remains fixed for later hands, Charge
and Mime. A fully blocked shot consumes no formula RNG. Hamedo uses the same
pure ballistic preflight; response cancellation retains the suspended packet.
No executing image loads another image. Existing shot/impact presentation is
retained; no renderer, portrait or art format changes.

AI uses the existing per-origin13-bit geometry masks for both arc and direct
weapons. One eligible actor/target pair is traced per update;64 rejected pairs
bound a sparse scan. The existing cursor means origin*16+target for projectiles
and keeps its old origin meaning for short weapons. Ordinary Attack and Charge
consume these masks before ranking; the old redundant straight-ray veto is
removed for weapon actions. Direct weapons reuse their unchanged trace. Cache
preparation preserves RNG and restores the temporary actor position. Candidate ranking conservatively rejects a shot whose recipient
differs from the intended target. Scalar/drain/proc utility stays in WEAPON22
op6; geometry and actual preparation stay in PLANNER0/1.

## Validation and limits

Independent integer controls cover336 offset/rise/slope combinations and
interior/end samples, separately labelled from the128 retail branch controls.
All1533 assertions match PC, ASan/UBSan and actual68000, digestAEF99E38.
All11 affected suites pass (the focused suite plus ten regressions). The old
Hamedo body-block control now selects a direct weapon; a bow correctly clears
that body on its high arc. Its345 checks and18 render checks retain their
previous digests. Initial failed logs remain in x/core-arcs/closure.log.
Gariland81/AE9CFD31 and Orbonne14/2CD70391 retain actual native pixels, exact
settled saves, no double payment and trophy-overflow agreement.
Integration covers a wall cleared by the upper arc, both arcs blocked before
RNG, an intercepted lower arc under a ceiling, a body cleared by the upper
arc, Hamedo and a stationary planner. PC/sanitizer/68000 reports, capacity,
code generation and preservation evidence are under x/core-arcs/.

The interaction delivery still needs full overlapping-panel victims and
recipient-aware previews, broader cooccupant/innate/Reflect utility fidelity.
ABI74/ModCtx348 and25 images remain. AI0 is8592 bytes, PLANNER24 is8408;
WEAPON22 still sets the8604-byte zone. Shards56364/61580/58740 add1714
archived bytes. Unit112/State2224/Work46632/Save3202/FSV4, five heap blocks
and the actual resident bytes are unchanged. Raw64891/TTPack64893 leaves43
bytes; packed data39799. ArcFlight adds120 automatic bytes while resolving a
shot; it is not a new persistent cache. Generated arc_trace advances by
additions, with three16x16 setup products and no loop division or32-bit helper.
The source square root and setup use bounded16-bit products. No authored ASM.

Whole-frame, worst stack, peak residency and hardware release checks remain
the following stabilization delivery. Standard groups, saves and the live
TiEmu session must be preserved, including protected proof bytes and dates.
