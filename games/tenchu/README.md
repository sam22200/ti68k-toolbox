# Tenchu training / contextual perspective laboratory

Five portable C rendering paths, genuine GCC4TI TI binaries, four synthetic common
scenes, and a local extraction of Tenchu: Stealth Assassins USA's training geometry.
This is an engine/traversal prototype. Original combat, mission completion, characters,
textures, faithful grappling motion and a complete training walkthrough remain milestones.

## Build and run

From this directory, **build serially**: GCC4TI uses shared intermediate object filenames.
GNU Make's grouped asset targets require Make 4.3 or newer.

```sh
make test pc ti cycles
./tenchu_pc --scenario 20          # synthetic Tenchu courtyard, filled 3D, 80x50
./tenchu_pc --headless --scenario 20 --frames 48 --keys keys/sweep.txt
make xcheck
```

The normal calculator binary is `tenchu.89z`. Transfer `tnspr.89y` with it and run
`tenchu(20)`. No kernel, FAT SDK, heap-generated code or hardware patch is required.
Its executable data is 24,501 bytes, below stock AMS 2.xx's 24,576-byte limit. The
transfer file includes 90 additional bytes. That leaves only 75 executable bytes
of margin: use a single-backend build for production, or GCC4TI's verified packing.

The local commercial disc is required only for the original training bank:

```sh
make extract                     # reads roms/ps1/Tenchu - Stealth Assassins (USA)/
./tenchu_pc --scenario 1044       # original training, filled geometry, 80x50
./tenchu_pc --scenario 1028       # lossy heightfield adapter
./tenchu_pc --scenario 1092       # hidden-line inspection
```

Transfer `tenchu.89z`, `tnmap.89y`, `tnspr.89y`; archive the data banks, then run
`tenchu(1044)`. All ROM-derived files are ignored and regenerated locally. The
29,638-byte bank contains geometry and collision data, not room backgrounds.

Standalone Phase 1 omits the polygon workspace and other renderers:

```sh
make ti pc cycles NAME=teray CFLAGS_GAME='-DRT_MONO -DRT_FRAME_TICKS=20 -DTE_ONLY_RENDERER=0'
# teray(4), or teray(1028) with the local training bank
```

`TE_ONLY_RENDERER=1,2,3,4` similarly specialize filled, portal, floor and hidden-line
builds. Choose a different NAME for each build. Rebuild PC executables after changing
flags; Make does not track flag changes. All five implementations share world,
camera, projection, collision, aiming, sprites and buffering code.

## Controls

| Key | Perspective prototype |
|---|---|
| arrows | forward/backward, yaw left/right |
| Shift + up/down | pitch; UI limit +/-33.75 degrees |
| 2nd | traverse to the selected ledge; MGS fixture: guard-box ray test, result in `state.hit` |
| Alpha | native training jump; map view: rotate the map |
| Diamond | 2x scope zoom |
| MODE | perspective / diagnostic map; preserves position and viewing direction |
| F1 | cycle the four synthetic scenes |
| F5 | 160x100 / 80x50 / 80x100 |
| APPS, or digits 1..5 | select renderer |
| digit 6 | presentation variant |
| digit 7 | original training injection door |
| ENTER | synthetic door open/closed |
| ESC | exit; release all heap workspaces |

PC mappings are the runtime's: Space/Ctrl = 2nd, Shift = Shift, C = Diamond,
V = Alpha, M = MODE, A = APPS. `--save`/`--load` use the runtime's POD state.
Scene changes require the matching injection door; cross-scene state-file loads
are not yet reconciled with the world bank. Use same-scene saves for this prototype.

All builds use a fixed simulation ceiling of 12.8 updates/s (20 ticks at 256 Hz).
CPU-limited camera motion can run slower. First-person is the initial traversal
test presentation; the diagnostic map is not a finished top-down/isometric game.
The FFT fixture has an active unit's eye viewpoint, four diagnostic map rotations
and a reversible view toggle, not an integration into `games/fft`'s tactical rules.

## Injection door

`scenario = scene + (resolution << 2) + (renderer << 4)`:

- scenes 0..3 = Tenchu B, FFT A, MGS C, Vagrant D;
- resolution 0..2 = native, half width/height, half width only;
- renderers 0..4 follow the five requested phases;
- add 128 for a yaw/pitch pose, 256 for an elevated/downward pose;
- add 512 for ten actors instead of five;
- add 1024 for the original training bank (there are no native NPCs yet);
- add 2048 for the isolated sprite fixture, where all ten actors are visible;
- add `(sprite_method << 12)`: 0 baked, 1 runtime NN, 2 bounded LRU;
- add `(style << 14)` for styles 0..3. Style 4 is selected interactively.

Phase 4 style 0 = full horizontal sampling, 1 = two-pixel sampling, 2 = flat floors.
Phase 5 styles: hidden line, full wireframe, black silhouettes, sparse dithering,
mixed near fills/distant transparent lines. The mixed variant is functional but
has not received a separate timing CSV.

## Benchmarks and screenshots

```sh
python3 tools/benchmark.py --phase 1 --label final_phase1
# repeat phases 2,3,4,5 in order
python3 tools/benchmark.py --phase 2 --sprites-only --keys keys/static.txt --sprite-mode 0 --label sprites_precomputed
# modes 1,2: sprites_runtime, sprites_cache
python3 tools/benchmark.py --phase 2 --native --label native_poly
python3 tools/benchmark.py --phase 2 --native --keys keys/static.txt --label native_static
python3 tools/report.py           # comparison + contact sheet; requires Pillow
```

Every complete CSV run validates the TI binary's final framebuffer checksum against
the PC at the same camera, script and frame count. A truncated CPU run, mismatched
checksum or workspace overflow aborts the runner. `bench-output/<label>/` holds raw
logs, framebuffers and a manifest with the binary SHA-256. `BENCHMARK_final_phase*.csv`
are the comparison data; earlier baseline CSVs are historical development evidence
and are superseded (handedness/aspect corrections and different sprite sampling).

Timing is **Musashi MC68000 datasheet cycles**, with estimated AMS ROM costs, not
TiEmu time or physical hardware FPS. CSV FPS values are CPU ceilings at 12 and 10 MHz.
They exclude interrupts and the final LCD copy (~17,500 cycles from the project's
verified ExtGraph measurement). The fixed-rate application cannot exceed 12.8 FPS.
Stage timings include instrumentation; parent/child zones overlap and must not be
added. Zone 5 is internal expansion/copy into the runtime plane, not the LCD driver.
Worst render includes the first-frame cycle sample: the generic ti-cycles `max`
output deliberately leaves out call zero and otherwise hides cache warm-up cost.

For the final hardware-path check, `TI_CALC=89u` selects original HW2 / stock AMS
2.09. Example on an idle emulator: `TI_ARGS=1044 TI_CALC=89u ../../tools/bin/ti-run
tenchu.89z tnmap.89y tnspr.89y`. Restart from the clean profile for each build. Do not
send keys to another person's game. This milestone was checked on a private X server
and private Docker/profile copies; `local/emulator/hw2.png` is the one capture.
No physical calculator was available. Titanium and HW1 were not tested this milestone.

## Representation and limits

The synthetic world uses compact extruded boxes, gabled roofs, actors and ledges.
The native world streams a big-endian vertex/face bank from the archive. Coordinates
and arithmetic use 16-bit words, 16x16 products, Q8 rotations and a Q14 reciprocal
table. Tables come from `ti-table`; no runtime floats or long multiplies/divides
are needed in the hot renderer loops. The verified `muls16` idiom is the only inline
assembly added here; no new assembly routine was written.

Raycasting has six depth-tagged vertical spans per column. It supports elevation
transitions, doors, translucent-mask sprites and camera height, but its horizon
shift is not true pitch. Its world adapter flattens slopes and cannot show bridges,
overhangs or overlapping surfaces. Native wall thickening is explicitly lossy.
The separate collision model still supports walking below the synthetic bridge.

The polygon paths clip the near plane and screen, reject world-facing planes before
transforming, and sort faces/sprites by mean depth. They represent bridges and
slopes, but mean-depth painter sorting is approximate: intersecting geometry and
large coalesced faces can be ordered incorrectly. There is no full-screen Z buffer.
Portals use row-span intersections and a bounded three-room tree in the MGS fixture;
outdoor scenes and the native training bank are single cells. Cyclic/multiple-path
portal graphs and automatic native room generation are pending, not measured wins.
Phase 4 maps separate elevated horizontal planes in the synthetic fixtures. On the
native mesh it maps the base plane only; elevated faces remain flat shaded.

Sprite comparisons use the same eight quantized sizes and two placeholder poses
for pixel-identical A/B/C results. B is runtime nearest-neighbor scaling to those
sizes, not a separate continuous-size quality comparison. A's pixels are baked
offline; C generates on misses with four fixed 512-byte slots and wrap-safe ages.
Opacity remains independent of white ink, preserving the white outline. Atlas:
2,208 bytes; popcount table: 256; metadata/source: 137. Per pose: 1,104 baked bytes
versus 32 bytes of original ink/opacity; cache allocation stays 2,064 bytes.

Native collision is deliberately adapted: a 16-unit grid, two possible floor slots
(this training extraction uses one), two-unit heights and snapped X/Y wall intervals.
Thin walls, diagonal geometry and merged vertical intervals are approximate. Native
faces entirely beyond 512 yaw-depth units are culled by their bounds; straddling
faces are retained. The 100 geometric ledge candidates are reduced to the nearest
eight in range each update. These are not a reverse-engineered list of Tenchu's
allowed grapple targets. The 12-step grapple tween is not the original rope/body
trajectory and does not prove collision-free travel for the whole character.

The first prototype is monochrome, including its ordered dither. Four-tone rendering
and a new grayscale-driver test are deferred until the geometry meets its budget.
See [BENCHMARK.md](BENCHMARK.md), [RESEARCH.md](RESEARCH.md),
[RE_NOTES.md](RE_NOTES.md) and [ROADMAP.md](ROADMAP.md).
