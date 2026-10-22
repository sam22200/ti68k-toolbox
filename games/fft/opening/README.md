Current review corrections, source evidence and limitations: [BUGFIXES.md](BUGFIXES.md).
For the complete opening -> Academy -> Gariland package, see [campaign](../campaign/README.md).
The storage/timing figures below describe the initial slice; current measurements
are in `x/check/checked.json` and the latest section of `BENCHMARK.md`.

# FFT opening: Orbonne

Playable native content variant of the existing Gariland engine, on the
`feat/fft-opening` branch. `opening.c` includes the common `fft.c`; movement,
CT, menus, attacks, AI, sprites, terrain coverage and rewards are shared.

The current slice is the chapel conversation, the confrontation outside,
Orbonne's first battle, then Ovelia's abduction and Ramza recognizing Delita.
The campaign supervisor in `../campaign/` continues through the Academy and into
Gariland, releasing each chapter before loading the next.
This is an adaptation, not a PlayStation executable translation.

## Build and play

From the repository root, with the local US disc and installed toolchain:

```sh
export PATH="$PWD/tools/bin:$PATH"
make -C games/fft/opening reference   # once: original cold boot + AI observation
make -C games/fft/opening assets      # offline conversion; ZX0 can take a few minutes
make -C games/fft/opening check       # PC tests + actual 68000 framebuffer/state checks
make -C games/fft/campaign bundle     # the battle program (every chapter) + one TI transfer group
make -C games/fft/opening review      # PC framebuffer GIFs, not a TI speed demonstration
cd games/fft/opening/data
../fftopen_pc
```

The reference tool uses ordinary controller input until the fight, then clears
only the player-control bit so the original AI can finish it. No event, damage
or victory forcing is used. Its states, dialogue trace and screenshots are in
`../x/opening/reference/`. Without `--story` it stops at Ramza's first turn and
uses no RAM writes.

Orbonne is a chapter of the campaign's battle program (`../campaign/ENGINE.md` M4): send
`../campaign/x/fftcampaign.89g` and run `fftbat(100)` (or `fftstory()` for the whole chain).
The packed launcher avoids the original TI-89 AMS 2.xx 24,576-byte executable-entry
limit with GCC4TI's packed launcher. The plain `make ti` executable is intended
for the Titanium; `make cycles` is the non-I/O profiling binary.
The packed launcher and all eight archived banks now cold-launch in Titanium
TiEmu (2026-10-18), displaying the softened battlefield at Ramza's manual turn.
This verifies entry and initial grayscale display, not a complete interactive
playthrough or an original TI-89 HW2 launch.

Doors: argument/scenario 0 = whole opening; 1 = immediate manual Ramza turn;
2 = full AI battle; 3 = conclusion. The PC accepts `--scenario N`.

- ENTER / 2nd: finish the typewriter, then advance; ordinary tactical confirm.
- SHIFT: skip the current scripted section; ordinary tactical back/Move undo.
- CATALOG during dialogue: invert the fitted dialogue window (black/white).
- Arrows: ordinary tactical navigation; F1/F5: existing animated low-poly turns.
- Ramza's Auto-battle menu can delegate his turn to the shared AI.

## Storage and RAM decisions

`data/storage.json` records actual generated sizes. Local retail art/text, binary
banks and screenshots are ignored by git. Required assets are generated from
the user's disc; no retail assets are embedded in the tracked source.

- Only one chapel camera is stored: a source-mesh cutaway of the camera-side
  walls, 160x100 with two coverage-depth layers. No movie or per-dialogue screen.
  The unpacked view reuses the combat buffer.
- Ovelia and Simon retain only two idle poses per facing. Delita retains walking
  poses, without unused attack/KO/armed poses. All logical mirrors reference
  the same canonical data; identical poses share one compressed record.
- 139,568 bytes of logical actor frames become a **39,171-byte** indexed ZX0 bank.
  The Ovelia/Simon canonical compressed streams total **1,071 bytes** (shared
  directory entries are separate). An actor holds one 208-byte decoded pose.
- Eight battle portraits replace eleven portraits; the unused 510-name dictionary
  is removed. Body/weapon data are separated: only 3,024 bytes of shared blades
  are needed. Jobs/items/unit templates and raw stat seeds live in archive.
- One ordinary pose-cache miss is admitted per render. Walking/action poses have
  priority, then visible idle actors receive turns in a round-robin order. Idle
  poses may lag the source animation clock; they cannot be permanently starved.
  Identical poses already decoded by another actor are copied without another
  decode. New actors are prewarmed; object/scene slots decode immediately.
- Orbonne has no traversable depth-one canals. Its source surface 0x11 includes
  20 depth-four water cells, now blocked for the human party. Water artwork uses
  animated palette 5 independently of those collision flags. Four-phase glints
  use 1,948 archived bytes (even-x/2,y pairs) read in place, without RAM point
  arrays. Status/dialogue pixels reuse the idle
  portrait-panel storage. The original mesh bottom fits row 191 in every view;
  removing 20 empty rows saves 6,400 buffer bytes.
- Native `State` is 1,012 bytes; `Work` is 35,520 bytes. Both scene allocations
  total **96,960 bytes**, plus 11,528 bytes for the grayscale runtime's saved LCD
  and double buffer. The program's writable statics already count in its native
  executable. Allow extra stack/AMS heap margin; the earlier packed build launched
  in Titanium TiEmu, but this correction has not replaced the user's live session.

No new assembly was written. ZX0 uses the existing verified decompressor.
The compiler-generated assembly was inspected for layout/alignment and arithmetic.
Actor coordinate fields and script record starts are word-aligned for the 68000.

## Battlefield tones

Orbonne uses a softer offline conversion than Gariland. The previous thresholds
were taken from camera 0's shaded facade and reused in every orientation: the
opposite courtyard and tower lost most details to pure white. Each battle view
now chooses thresholds from its own lit scenery (percentiles 1/50/96), excluding
the sky and black solid cutaway sides. A half-strength 0.55-pixel Gaussian pass
reduces texture speckles before four-grey quantization. Most scenery stays in
the two middle greys; actors and UI keep their existing contrast.

The same geometric ledge detector and visibility-tested black/white edge pass
as Gariland now mark real height changes. They modify scenery colours and flat
rotation shades, preserving heights, coverage depths/masks and the actor silhouette
shader. Filtering, threshold selection and contour painting are offline. The
separate deep-water collision correction is described above. The chapel and
Gariland retain their existing conversion.

The original map ping-pongs palette 5 through three palette frames, duration 15
PS1 ticks. The native adaptation keeps water in the middle greys and adds four
moving white-glint phases only on visible pixels using that palette; it does
not reproduce the exact PS1 palette sequence. No grass/wall or hidden-water
pixels receive glints.

There is **no perspective prefetch** in this variant: neither `FFT_CONFIG` nor
`FFT_EXPLORATION` is enabled. Removing the optional inspection cache elsewhere
cannot accelerate this build. The portrait slowdown came from reserving decode
cost for every visible actor despite the one-decode quota. Charge only actual
misses/copies, and use the correct trimmed portrait index for both pixels and
opacity mask. The cold browsing control falls from 45 to 5–12 frames.

## Behavior and limits

Eleven source unit records are checked against the common stat formula. Jobs,
equipment, levels, Brave/Faith, signs, start tiles and hero names come from the
original first battle. Only Ramza is controlled manually; the other allies are
AI guests. Original scene dialogue order and chapel actor positions come from
the unmodified event flow. Actors interpolate between observed dialogue poses.

The native opening adds bow reach, directional shield evasion (each evade rolled in turn,
as the decompiled rule; these rules are now the engine's for every chapter: `../rules.inc`), Accumulate,
stat breaks, Stasis Sword's cross-shaped damage and Night Sword's HP drain.
It shows the shared movement/blow/hit/KO/reward animations and compact projectile
and sword effects. Guest/enemy AI deliberately favors attacks and damage/healing
abilities over stat breaks. Archer Charge now waits on the CT clock and targets a fixed tile. Ballistic
collision, secondary Stasis status effects and exact original AI decisions
remain bounded adaptations. Special-skill
reach, splash and presentation are bounded native adaptations, not an oracle
validated reimplementation of every skill formula.

The scene staging and abduction are compact adaptations using original actors;
a compact punch/collapse and sprite Chocobo departure replace the movie.
There is no FMV, music or exact PS1 camera choreography.
The battle retains its terrain heights. Upper decorative towers beyond the fixed
native scene origin are cropped; this is not a full-height renderer of map 56.
Perspective inspection and management menus remain separate existing prototypes.

## Verification and performance

`check.py` compares exact framebuffer checksums and explicit endian-independent
combat summaries between PC and the **actual 68000 binary**, through intro,
manual turn, rotations, complete AI battle and conclusion. The script prints
stage-specific average/peak cycles and records `x/check/checked.json`.
`test_opening.c` also checks source HP, guest ownership, Move/undo, HP drain,
complete combat and the entire scripted flow. Existing Gariland tests pass.
`visual_checks.c` checks all 11 actors' idle changes, 648 exact displayed walk
poses (Gafgarion and every enemy, all four views), panel masks/latency and
four water phases. `orbonne-animation.gif` plays the ordinary AI without the
fast-forward key; it is still a PC presentation, not a TI timing capture.

Cycles are MC68000 datasheet execution costs under `ti-cycles`; ROM-call costs
are estimates. Grayscale interrupts, frame waits and packed startup are excluded.
The renderer still has scene-decode and animated-rotation stalls; these are
reported separately, never presented as ordinary 30-fps frames. The full replay
also exposes occasional combat and dialogue frames beyond 360k cycles. Consult
`BENCHMARK.md` for the measured run, rather than interpreting PC GIF playback
speed as calculator performance.
