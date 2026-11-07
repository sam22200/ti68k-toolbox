# Orbonne review corrections

The review cases are the user's local `fft_bugs/` captures. Retail captures,
dialogue and generated banks remain local. This is a bounded native adaptation.

## Combat

- Predictions show one amount and hit chance, never a minimum–maximum range.
- Potion is decoded from ability `0x170`: `Y=3`, therefore 30 HP. The previous
  hand-authored `Y=10` was wrong. Hi-Potion (`0x171`) is 70 HP, not 100.
- Archer Charge uses the US executable's table at `0x80061044`: 4, 5, 6, 8,
  10, 14, 20, 35 clock ticks for +1, +2, +3, +4, +5, +7, +10, +20. Orbonne
  exposes only its learned +1/+2 abilities. These are clock ticks, not a fixed
  number of unit turns. Confirming an action does not advance the clock while
  the player is choosing. Other ready units may act before resolution.
- A pending action stores its target **tile**, origin, weapon, ability and
  remaining ticks. The archer stays present/targetable. An empty tile loses
  the shot; a replacement occupant receives it. Death, displacement or weapon
  change cancels preparation. Resolution gives no extra normal turn or CT bonus.
- The power bonus is added to the weapon's primary stat, rather than its WP.
  Physical XA against a charging target receives the source 3/2 modifier.
  The AI discounts a Charge when its target can act before impact; this is a
  compact heuristic, not a claim of exact original AI equivalence.
- `DEFER_CHARGING`, `DEFER_AIRBORNE`, `DEFER_PERFORMING` are distinct identifiers.
  Only Archer charging is implemented/exposed. Jump's absence/untargetability,
  unit-following magic and periodic songs/dances are future mechanics, not
  aliases for Archer Charge.

Primary implementation references in the local decompilation, also available at
[fft_decomp](https://github.com/adamrt/fft_decomp):
`battle_action_prepare_attack.c`, `battle_action_run_between_turn_control.c`,
`battle_formula_apply_target_xa_affecting_statuses.c`,
`battle_action_run_pre_formula_setup.c`. The disc's own values are authoritative;
the three-tick +1 table sometimes quoted online is not this US executable.

## Camera and scenery

The earlier coordinate-only placement audit proved unit tiles, not camera
handedness. The original camera matrix observed at Orbonne has screen X
`+2896*x -2892*z` and screen Y `-1292*x -1294*z +3661*y` (PS1 Y points down).
Our equivalent front orientation had the opposite screen X. `FFT_SOURCE_CAMERA`
corrects the projection, sprite mirroring, slope markers, cursor/facing inputs,
moving rotations and offline depth/masks together. World tiles, heights and
tactical rules are not transposed. The opening and campaign Gariland enable it;
historical standalone research controls keep their old banks/projection.

Gariland's reviewed colour/mask planes are mirrored offline; depth **values**
are reversed by byte position, never bit-reversed. Orbonne's source renderer and
the ledge pass use the corrected projection directly. The source wood (terrain
20) and stone (21) swatches from Gariland are mapped onto Orbonne's corresponding
horizontal surfaces offline. There is no second runtime texture atlas. Edges,
water, source geometry and depth tests remain separate. Non-walkable void cells
no longer draw a selection diamond underneath the scenery.

## Story

`opening_scene_oracle.py` observes the original with controller confirmations,
without scene/HP/victory writes. It records display-unit positions, facings,
poses and messages through the chapel, abduction and Academy. One camera and a
64-pixel panorama replace PS1 camera choreography in the chapel. Only moving
actors walk. Stationary ones use source standing, prayer or kneeling poses.
The incoming female Knight appears with the alarm. Actors are ordered by depth;
Ovelia no longer overwrites Agrias merely because of record iteration order.

Frames >=210 use shared event artwork, not ordinary TYPE1/TYPE2 frame indices.
The oracle captures the actual display parts, texture page and palette from
VRAM. Only needed composed poses enter the same canonical/mirrored ZX0 dictionary;
the ordinary actor banks are searched for identical bytes first. The location
title reuses the management prototype's MAPTITLE converter and sweeping white
reveal, without a second font.

Chapel/outside and story/battle changes fade through black. Exterior dialogue
uses stationary actors; the conclusion uses its own source staging, not live
battle movement or death-counter overlays. Text confirmation never restarts
the drag animation. The native punch/collapse and Chocobo departure are compact
sprite adaptations; they do not reproduce the original movie. Source KO bodies
are observable after the fight; their battle countdowns are omitted in scenes.
Dialogue restores a white border and fitted speaker badge. Management actors
are static at both roster sizes and in job previews.
This reproduces only part of the reviewed management dialogue presentation:
opening/Academy do not yet draw the speaker portraits. The current opening bank
keeps battle portraits only; scripted speakers have name badges without images.

The end returns a continuation flag. It never silently restarts the prologue.
`../campaign/` supplies the Academy and Gariland handoff, loading programs one at
a time so old scene buffers and archive pointers are released first.

## Reproduction

Run `make -C games/fft/opening check`, then `make -C games/fft/campaign test pc`
and `make -C games/fft/campaign bundle`. Never overlap GCC4TI builds. The
opening's `regressions.c` checks delayed/empty-tile/interrupted attacks and
dialogue coordinate stability; existing walking/portrait/water checks remain.
`check.py` compares PC and actual 68000 pixels, pending-action state and final
handoff state, with datasheet timings. PC review GIF cadence is not TI FPS.


The rear shot uses the orientation exposing the action and draws its staged
actors above the static battle mesh. That mesh does not contain the original
event's foreground visibility changes; it must not turn the punch and princess
into opaque-terrain silhouettes. This adaptation is confined to the scripted
shot. Battle/chapel terrain coverage and depth-sorted actor order remain intact.

### Orbonne initial tactical facing (2026-10-19)

The asset importer used the render actor's interpolating angle at scene+0x70.
In the observed event-05 snapshot Ramza still had3072 (+x) there, while his
tactical unit+0x49 was0 (-z), confirmed again at the original first Ramza turn.
Every imported unit now uses its tactical facing. Only Ramza differed in that
snapshot. No unit position, shared camera transform or sprite mirror rule is
patched. The regression checks world face3 and all four projected directions.
