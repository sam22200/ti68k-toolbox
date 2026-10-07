# Mega Man X: introduction-stage slice

The user requested a second SNES experiment to populate `ti-port-snes`, using
the local USA Mega Man X ROM. Required mechanics: running, shooting, charging
and releasing, jumping, wall jumping, enemy contact damage and animations.
The prompt mentioned the entire introduction and then requested only a small
part. The optional clarification received no answer; the working scope follows the final clause:
a short opening band with real terrain and a useful wall-jump passage. The
endpoint selected from the terrain survey is X1008: cross the first road gap
at X800..831 and reach the raised road. Extracted band: X0..1023,Y256..511.
This is 880 original playable pixels after the X128 spawn, not the full stage.
The optional clarification received no answer; this scope remains a disclosed
working assumption rather than a confirmed answer.

Proposed controls: arrows run,2nd jumps/wall-jumps,Shift shoots, hold/release
Shift charges/releases. The optional mapping question received no answer. Half-scale
graphics follow the existing SNES experiment; the world and physics keep
original pixels. Intro X has the original starting equipment; no dash/armor,
boss/Vile/Zero scene, later stages, menus or sound is implied by this slice.
Changes to scope from the user's answers supersede these working assumptions.

| Milestone | Deliverable and acceptance | Status |
| --- | --- | --- |
| 0 | Reproducible cold boot into original Highway, source identity, RAM/PPU/video snapshots and deterministic replay | Passed: 100 replay frames |
| 1 | Measured running, short/held jumps, wall slide/kick, shots/charge and natural contact damage; confirmed RAM fields | Passed: 1230 motion/recoil and 72 projectile samples |
| 2 | Portable movement on the extracted collision band, per-mechanic PC tests and end-to-end traversal | Passed: real gap/raised-road traversal |
| 3 | Shots, charge tiers and enemy interaction, HP/recoil/immunity/death/retry, scenarios and deterministic tests | Passed: two small hits start delayed death; charged shots pierce armor/body |
| 4 | Original ROM scenery and animations with white outlines and readable charge/HP; PC/TI explicit states/screens and per-frame cycle budget | Passed: 4759 full 71-word states and LCDs; peak 185570 cycles |
| 5 | Smooth headless action preview, rebuilt TI artifacts and verified reusable notes in the SNES skill | Passed: 1300 consecutive LCD updates; 43.16s GIF |
| 6 | Titanium hardware run once after headless checks; respect the user's ownership of TiEmu keys | Deferred: current keyboard belongs to user; no hardware run claimed |
| 7 | Measure and verify buster muzzle/formation during run, jump and wall slide; preserve the standalone passing mechanics | Passed: 656 source-active projectile samples on PC and TI; 365 added complete PC/TI frames |
| 8 | Verify firing, wall, hurt, projectile, roller and explosion extraction against original video; distinguish OBJ occlusion from extraction errors | Passed: 292 hero frames / 146886 pixels, plus 109 projectile/enemy/effect frames / 46226 pixels; maximum RGB error 2 |
| 9 | Verify left normal/medium/large shots, formation direction and signed offscreen lifetime; rebuild native checks and preview | Passed: 104 added original PC/TI samples; 195 added complete native frames |
| 10 | Probe exact projectile removal bounds/phase instead of inferring them from sparse natural samples; test both native edges | Passed: 744 original controlled probes; 36 complete native PC/TI states and LCDs |
| 11 | Measure roller response near a stationary hero and the stop+fire transition; keep requested section and existing passing oracles | Passed: 1040 natural enemy observations and 167 added stop/fire projectile samples on PC/TI; broken-roller ROM poses and source hitboxes |
| 12 | Measure charge/firing through contact, recovery input and player-pose rectangles; preserve prior reference oracles | Passed: 1320 natural hero states and 188 projectile samples on PC/TI; 300 pose-box samples, 12 slot-reuse probes, 350 added complete native frames |
| 13 | Measure projectile capacity, repeated press/charge and full-slot release suppression without changing the selected section | Passed: 245 updates and 278 projectile samples on PC/TI; 190 added complete native states/LCDs |
| 14 | Compare normal/medium/large firing through wall grip, kick windup and forced-away launch, including correct muzzle/direction | Passed: 21 timelines / 840 hero states / 531 projectile samples / 91 muzzle probes; 24 controlled normal-fraction samples; 500 added complete native states/LCDs |

| 15 | Compare the whole chosen path from spawn to endpoint, including charge, natural roller defeat, gap traversal and wall recovery; independently probe support/contact transitions | Passed: 1238 hero/roller states, 116 projectile samples and 64 ledge/clamp samples on PC/TI; 516 added complete native states/LCDs |

| 16 | Verify release/stop, stationary jump and reversal at the last unsupported RUN update, including facing; preserve full traversal and prior oracles | Passed: 216 added source input states (280 edge samples total), including a full stationary jump after running; 230 added complete native states/LCDs |

| 17 | Measure cannon origin on the exact update leaving RUN for jump/fall; retain prior isolated/coupled oracles | Passed: 204 added source projectile samples and 434 hero/charge/birth states on TI; 410 added complete native states/LCDs; final run+jump charge clip |

Use two original logic steps per approximately 30 Hz TI rendering frame, subject
to a measured budget and input-edge checks. Keep ROMs, extracted assets and
third-party reading aids local. Do not claim original physics from appearance;
separate OBSERVED rules from native TARGET adaptations in `RE_NOTES.md`.

The user requested autonomous work throughout the available session. Credit
usage is not exposed to this agent; keep checkpoints and resumable commands.

Art follow-up: corrected pre-update OAM anchors, separated charge formation
from flight poses, and slowed idle pose playback. 85 original hero frames
(40255 opaque pixels) match original video within two RGB units. This reduced
the local art bank from 226 to 194 poses without dropping semantic actions.

| 18 | Replace the charge box with converging sparks and readable armor pulses; rebuild the smooth preview | Passed: 4759 PC/TI states and LCDs, peak 185570 cycles; fifteen clips / 1300 rendered updates / 43.16s |
