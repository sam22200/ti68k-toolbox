# Sonic 1: a small Mega Drive-to-TI experiment

Status: traversal plus rings, enemies and damage run on PC and TI. Branch:
`feat/md-sonic1-first-level`. Milestone 5 adds ROM scenery, animated Sonic,
badniks, rings and missiles at half scale in four greys. Main actors have a
one-pixel white outline; backgrounds use white/light grey for readability.

The target is the beginning of Green Hill Act 1, about five original Mega
Drive view widths, starting directly in play. This is the only current slice;
title screens, logos, later acts and the rest of the game are outside it.

## Confirmed port decisions (2026-10-05)

- Faithful feel and rules, with our own C engine on the Portable Game Runtime.
  The shared 68000 makes original calculations easier to study. Selective
  native routine reuse may be investigated if demonstrably simpler, but adding
  assembly still requires explicit agreement. Whole-game 1:1 is not the default.
- The extent is five **Mega Drive** screens, not five TI screens. The observed
  reference view is 320 pixels wide: stop at x=1536, on safe ground before the
  next descent (4.8 view widths). Render data extends to x=1664 for camera margin.
- Use the ROM's graphics, converted for the TI, prioritizing high contrast.
  Main sprites need a white outline. Generated art and map data remain local.
- Engine and checks first, final graphics last; small milestones rather than
  the whole slice in one implementation.
- Half scale for geometry, sprites and all distances: one target pixel is two
  original pixels. The original 320-pixel view width remains visible on the TI.
- First playable milestone: movement and terrain only. Rings, enemies and
  damage are separate later milestones in the same five-screen portion.

- Q6/Q7 confirmed: rigid bridge, arrows to run, Down to roll, [2nd] to jump
  (held/tap height), ENTER to restart, ESC to quit. The camera uses all 100 rows;
  no permanent HUD. Falling below the slice resets to the start. The endpoint
  freezes play and displays an ENTER restart prompt. Alpha (`K_D`, V on PC)
  toggles a small state overlay for diagnostics.
- The spring is not required by the verified lower route and is omitted here.
- Interaction milestone: 14 rings and the three enemies placed in this slice
  (Motobug, Buzz Bomber, Chopper). Jump/roll destroys badniks; missiles remain
  dangerous while rolling. Damage spills rings, locks controls during recoil,
  then grants two seconds of invulnerability on landing. A hit without rings
  restarts the attempt immediately, using the existing fall/reset convention.
  The ring count appears for three seconds after a pickup/hit; the default
  view still has no permanent HUD.

## Build, play and check

From the repository root:

```sh
make -C tools/md core                     # local pinned original emulator
make -C games/sonic test pc ti             # generates terrain, art and reference data
cd games/sonic
./sonic_pc                               # arrows; Space/Z/Ctrl jump; Enter restart; Esc quit
./sonic_pc --headless --keys keys/play.txt --frames 250 --shot x/play.png
make xcheck                              # 13 scenarios, PC/TI screen checksums
make tihash                              # six scripts, 260 state hashes per script
make rendercheck                         # optimized particles = individual sprite draws
make profile                             # actual per-frame 68000 costs, x/cycles.json
```

Send **all three** files: `sonic.89z`, `sonterr.89y` and `sonart.89y`, then call
`sonic()` on the calculator. Both data variables can be archived and are read
in place. The code is about 16 KiB; the terrain/object file is 23,142 bytes and
the art file is 54,280 bytes. Re-send code and art together: the SNA1 bank now
includes the pre-shifted ring rows. Missing data displays a message.

Simulation uses original positions (integer plus 8 fractional bits), signed
8.8 speeds, and two ~60 Hz logic steps per ~30.1 fps drawing. No new assembly:
the only inline multiply is the project's already verified `muls16` primitive.
Generated collision profiles and diagnostic TileMap live in `sonterr.bin`
(PC order) / `sonterr.be.bin` (TI order), never committed.
Object placements are six-byte records read directly from that same file;
small fixed arrays hold per-attempt collection, actors and particles. No
runtime allocation, division or 32-bit multiplication is needed for them.

`tools/art.py` uses original RAM/VDP snapshots for GHZ tiles, maps and enemy
patterns; Sonic's mapping/DPLC tables select his uncompressed ROM art. Its
SNA1 bank holds 232 deduplicated 16x16 tiles, 148 sprites with mirrored poses,
and pre-shifted ring rows. Resizing, contrast conversion and outline dilation
happen offline. One opaque TileMap combines scenery and a static background:
the background has no parallax. Walk/run/roll/hurt/duck poses use a small
animation selector; original animation timing and slope pose selection are
not reproduced. The complete extracted atlas remains available locally at
`x/atlas.png`; the four-scene art review is `x/art-review.png`.

### Injection doors

PC `--scenario N`, TI `sonic(N)`:

| N | State |
| --- | --- |
| 0 | Normal GHZ1 start |
| 1 | Measured flat ground, x192 |
| 2 | Before bridge, x1024 |
| 3 | On rigid bridge, x1184 |
| 4 | Endpoint approach, x1456 |
| 5 | Airborne above an upper ledge |
| 6 | Below bridge: fall/reset check |
| 7 | Flat ground with state overlay |
| 8 | First ring group, airborne at x324 |
| 9 | Descending attack near Motobug |
| 10 | Motobug contact with ten rings |
| 11 | Buzz Bomber approach with ten rings |
| 12 | Thirty-two spilled rings: rendering stress test |

### Validation and limits

`make test` passes: five recorded original flat-ground actions, the preserved
terrain-only traversal, bridge landing/pass-through, solid bank walls/underside,
falls/reset, held-jump behavior and deterministic scripted runs. Flat-action
velocities/flags and X agree with the original samples; Y allows one original
pixel for collision subpixel residue. This is a behavioral port, not full-state
1:1 emulation. Controlled original traces also match all three actors' positions,
fractions and velocities (40/110/160 logic steps). Tests cover one-time ring
collection, attack bounce, projectile charging/cancellation, recoil/control
lock, invulnerability, reclaim delay, particle limits/expiry and reset.
The normal RIGHT-only run now dies; `keys/play.txt` reaches the endpoint in
245 drawn frames, with two enemies destroyed, one ring and no resets.
`xcheck` matches 13 scenarios; `tihash` matches every mutable state field on
six 260-frame replays, including the complete playable traversal and damage.
Art checks compare 496 opaque Sonic pixels with the original RGB565 video;
they validate sprite bounds, native byte order, outlines and every pre-shifted
ring row. `rendercheck` compares all plane checksums across 780 frames against
unoptimized individual draws, including damage and the 32-ring stress case.

`make profile` uses differences of `ti-cycles` prefix totals, measuring every
drawn frame rather than inferring peaks from averages. Update + render:

| Replay | Frames | Average cycles | Maximum cycles |
| --- | --- | --- | --- |
| Full play | 250 | 186,420 | 323,092 |
| Damage, ten rings | 90 | 197,533 | 314,400 |
| Stress, 32 spilled rings | 90 | 223,562 | 353,666 |

All stay below the ~360k grayscale game budget, including cold TileMap creation.
Isolated rings use cached masked sprites. Dense spills combine pre-shifted rows
into one masked 32-pixel sprite; sparse spills omit earlier identical draws
when a later ring paints the same pixels. Both preserve the full drawing order
and image, including transparent holes in Sonic's outline.
Generated hot-path assembly contains no 32-bit multiply/divide or float helpers.
Earlier traversal milestone's Titanium TiEmu check: clean grouped load, diagnostic terrain/grayscale visible,
RIGHT moves the actor and camera; ESC sent to exit after the capture. One LCD capture
is retained locally in `x/titanium.png`.

This milestone changes no hardware paths. On the user's deployment request,
the three files were loaded through a clean Titanium TiEmu restart and
`sonic()` launched. One capture (`x/titanium-art.png`) confirms the ROM scenery,
outlined Sonic and grayscale presentation. On the subsequent request to play,
`ti-play keys/tiemu_play.txt 30.1` drove a 12-second real TiEmu recording at
15 fps, `x/sonic-titanium.gif`. It shows movement, jumping, scrolling, ring
pickup/loss and the bridge. The real-time replay takes damage and diverges from
the headless winning route; this capture does not establish a TiEmu win.
Held keys were released at the end, and the game remains running for the user.

The demo script adds an explicit final release: `ti-play` releases keys at its
last event, whereas the headless runtime keeps the final input snapshot. The
GIF capture now scales its LCD crop to the current default-skin window size;
the previous fixed rectangle cut off resized calculator windows.

The bridge is rigid; springs, monitors and sound remain absent. Ground/air rules are intentionally
limited to this band: no loop/wall-running engine, no later levels. Air
landing uses the horizontal velocity as new ground speed; slope landings are
an approximation to revisit against dedicated original traces when needed.
Enemy activation uses a bounded camera band, rather than reproducing the
original object loader in full. Spilled-ring launch directions use the existing
generated sine table; floor probes are staggered over four logic steps. Score,
lives and the original death sequence are outside this interaction milestone.

## Reference

Sonic the Hedgehog, Sega / Sonic Team, 1991, commercial Mega Drive REV00.
Local file: `roms/md/Sonic_1.md`, 524,288 bytes, checksum `264A`.
The local ROM run under Genesis Plus GX is the source of truth. The
[community disassembly](https://github.com/sonicretro/s1disasm/tree/AS) is a
reading aid: revision options and bug fixes must not silently replace the
local ROM's behavior. See [RE_NOTES.md](RE_NOTES.md) for provenance, commands,
observations and unanswered questions.

```sh
make -C tools/md core
make -C tools/md check
make -C games/sonic reference
make -C games/sonic measure
make -C games/sonic generated/objects_ref.h
```

`reference` regenerates the start state, image and movement / jump traces in
ignored `sources/sonic1_md/`; `measure` records controlled flat-ground actions
in `physics.json`. The runner reloads the original directly into GHZ1.
`tools/measure_objects.py` records local `objects.json` and a PC-test-only
header, probing complete object slots, pickup, damage and attack reactions.

## Next milestones

See [ROADMAP.md](ROADMAP.md). Each playable engine milestone has a
scenario injection door, PC unit/headless checks, then `ti-cycles` and PC/TI
state/screen checks. TiEmu comes last, for hardware-specific checks only.
