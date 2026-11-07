# Area impact: private visual candidate

Historical review, approved2026-10-28 with “ok and continue”. Current integration:
[CORE_AREA_IMPACT.md](CORE_AREA_IMPACT.md). At the review stage the active checkpoint was CORE_AREA_UI:
ABI33,15 images, private package515564 bytes, resident64719/headroom217. No active
engine, module, catalogue, asset, save or calculator package changes here.

Run `make -C games/fft/campaign area-impact-review`, or
`tools/pyenv/bin/python games/fft/campaign/review_area_impact.py` from the root.
Artifacts: x/core-area-impact-review/area-impact-review.png,
area-impact-targets.gif and area-impact-empty-status.gif. They are before/after
pairs at the same160x100 scale, camera, tactical positions and update frames.
Captures take every second30fps model frame; GIF delays66ms are a PC review pace,
not a hardware performance measurement. Only the final three composites are
published; the individual raw frames remain temporary.

## Proposed presentation

- Keep one common spell preparation/impact. Use the existing wind pose rather
  than the generic throw pose; no new sprite or spell bitmap bank.
- Flash and animate each successfully damaged victim. Show each victim's own
  outlined HP amount, absorbed healing with a plus sign, or Miss. The labels
  come from the prepared execution packet, never from the earlier forecast.
- Pulse the actual selected ground tile, with its slope and existing cover
  mask. Empty centres keep this anchor; zero victims produce no invented damage
  label or hit animation on the caster.
- Oil removal has a small Oil- line below that victim's amount. This is the
  only supported extra status feedback in this candidate, not a complete status
  effect/message system.
- Stop the root effect when a secondary reaction is running. Existing response,
  reward and terminal drains keep their rules: Auto-Potion is not a second root
  hit; EXP/JP settle once; victory waits for settlement.

The generic pulse and message placement are TI adaptations for visual review.
They do not claim original Fire art, original per-spell timings or simultaneous
effects for every original ability. The common fixture camera also applies to
the baseline; this comparison does not certify the production area camera.

## Source and observed problem

STATIC original readings, pinned by SHA256 in verification.json:

1. battle_effect_store_targets_and_display_types builds unit-ID/result-animation
   blocks for victims. With no targets and animate_on_miss_flag==0 it builds a
   tile-coordinate target instead. The empty effect anchor is not a fake caster.
2. battle_effect_run_keyframe_actions_all_targets dispatches display and pose
   events to the target list, with an explicit caster-only exception.
3. battle_action_build_targets_post_action_message queues results per victim.

These are bounded code readings, not a new original presentation trace.
Existing compound preparation/commit/CT/response evidence is reused.

The current scalar renderer loses all but its single target and can cache Miss
on the frame act_t first reaches ACT_HIT while the batch is still READY. The
next update commits HP, but that old scalar glyph remains cached. The candidate
waits for APPLIED and uses each retained CastVictim. This changes presentation
only; it does not reroll or reapply damage.

## Verification and integration boundary

preview_area_impact.c includes a generated private fft.c copy with six explicit
render hooks. It uses the real batch prepare/commit and attack_step reaction/
reward drain. Eight synthetic fixtures cover two victims around an empty centre,
guaranteed magic evasion, allies, absorption, zero victims, KO/Oil, final victory
and allied Auto-Potion. Source Fire16 raw data is retained; only its normalized
producer is enabled in a temporary catalogue. This is effect-stage injection,
not another CT/command audit or retail activation.

PC and ASan/UBSan compare state/RNG across baseline and candidate, check rendering
leaves State/live CastBatch/RNG unchanged, and compare all captured frame bytes.
11800 scoped checks agree at6EE77E42;832 captured frames match between PC and
ASan/UBSan. Protected file hashes/dates and the three source hashes are in
x/core-area-impact-review/verification.json. No native build/benchmark or
live TiEmu action occurs before visual approval. Existing gallery image dates
and both standard/previous private packages are preserved.

The prototype owns glyph-only caches; it is not the native allocation design.
After approval, prove reuse/lifetime of idle AI scratch, stage glyph preparation
during wind-up, integrate callback/image dispatch without executing-image eviction,
budget the tight resident input, retain correct camera/upper-layer anchors and
measure actual cold/interactive frames. Test only that affected closure.

Per-spell art, broader status/Reflect/special-action families, full interactive
frame/stack/residency and remaining core/content-production gates stay open.
Jump refinement remains deferred. Do not report the core complete or unlock
retail casts from this prototype. The visual approval gate is explicit in
.claude/skills/ti89-c-dev/SKILL.md: “Visual changes are validated by the user”.
