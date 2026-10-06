# Sonic 1: reference notes

## Decisions

2026-10-05: confirmed faithful feel with a C engine, five original MD screens,
ROM graphics with high contrast, direct-play start and incremental work.
Q4/Q5 subsequently confirmed: half-scale for all geometry and distances;
movement and terrain before rings, enemies or damage (README).
Q6/Q7 confirmed: rigid bridge, full-height following camera, agreed controls,
fall reset and an endpoint on safe ground near five original screen widths.
All design prerequisites of the first traversal milestone are settled.

## Tools and provenance

- ROM: `roms/md/Sonic_1.md`, SHA256
  `46160baa06362c711c9f1a5017cb7371026444936c8af5e93a78996cf32ff2a6`.
- OBSERVED: 524,288 bytes, serial `GM 00001009-00`, region `JUE`, stored and
  computed checksum `0x264A`; header RAM range FF0000..FFFFFF.
- OBSERVED: reset SSP FFFE00, reset PC 206, VBlank vector B10, HBlank 1126.
- INTERPRETATION: REV00, matching the header values in the community
  disassembly; any code addresses still need a ROM-byte check before reuse.
- Original emulator: Genesis Plus GX `58c341487e5bfcf979ea68413c7987633adb0c56`,
  local `sources/md_core/`, libretro build, reported NTSC rate
  59.92274340431231 frames/s and 64 KiB work RAM.
- Disassembly reading aid: Sonic Retro `s1disasm`, AS branch,
  `f74712a26c7568544457f5e28d33f7933b2e3660`, local
  `sources/sonic1_md/disasm/`. Use Revision=0, FixBugs=0 when comparing.
- Our runner: `tools/md/mdrun.py`; see `tools/md/README.md` for build and API.
  CPU-order dumps swap host RAM byte pairs on the normal x86 core build.
- `keys/ref_boot.txt`: START held at frames 600-601, run 1,000 frames from
  reset. `make reference` regenerates `sources/sonic1_md/start.state` and the
  metadata with ROM/core hashes. It is not a hand-made save state.

## Addresses used in initial checks

These names come from the disassembly and have been checked against the
running ROM for the listed actions. Addresses are CPU addresses, not host
RAM offsets. Numeric units will be confirmed separately on controlled ground.

| Address | Size | Meaning | Check |
| --- | --- | --- | --- |
| FFF600 | byte | Game mode | Boot: 00, 04, 8C, then 0C in normal play |
| FFD000 | byte | Player object ID | 01 in GHZ1 |
| FFD008 | word | Player X | 80 at spawn, increases with RIGHT, poke 120 survives idle update |
| FFD00C | word | Player Y | 944 at spawn, decreases during the jump |
| FFD010 | signed word | Horizontal velocity | Changes under movement; slightly negative on the standing jump |
| FFD012 | signed word | Vertical velocity | Negative ascending, increases each air frame |
| FFD014 | signed word | Ground speed/inertia | Changes with ground movement |
| FFD022 | byte | Status | Bit 1 set during the jump |

## Experiments

### Boot / injection door

Command: `make reference` (Makefile contains the exact runner arguments).
OBSERVED: normal level mode 0C by sample 840; at frame 999 Sonic is at
(80, 944), stationary, without the level title card. Captured frame is 320x224.
TARGET: TI scenario 0 will start directly in this portion, without boot art.

### Tool integration check

Command: `make -C ../../tools/md check` from this directory.
OBSERVED: reproducible boot reaches GHZ1; RIGHT moves Sonic; C launches a
jump; an X-position poke affects the next frame; replaying the same 20-frame
RIGHT sequence from one saved state matches all 64 KiB of RAM and the video
pixels frame by frame. Check passed, 2026-10-05.

### Initial motion and jump traces

Commands: the `reference` target runs `ref_move.txt` (60 frames) and
`ref_jump.txt` (70 frames) from the same state and writes CPU fields per frame.
OBSERVED: at the spawn, RIGHT frame 0 produces ground speed 12, then 20,
28, 36; Y changes from 944 to 943 during the release section. A standing
held jump has horizontal velocity -163 and initial sampled vertical velocity
-1651; the next air-frame velocity is -1595 (difference 56).
INTERPRETATION: the spawn is slightly sloped, so these values are unsuitable
as a flat-ground acceleration/jump impulse specification. The negative
horizontal jump component is consistent with a slope-normal launch.
TARGET: use controlled original ground and short, labeled experiments before
choosing engine constants; keep original units distinct from target scale.

### Controlled flat-ground measurements

Command: `make reference measure` from this directory. `tools/measure.py`
reloads the reference, injects x=192, y=930, zero velocities/inertia and status
2 (airborne), then runs 60 idle frames. It asserts settled x=192, y=940,
angle/status/speeds zero before every action is derived from that saved state.
All per-frame data and input transitions are in local `physics.json`.

OBSERVED: positions are original 16.16 words; speeds are original signed 8.8
words. Keep these units for simulation and halve distances only for the target
view, rather than doubling constants to simulate a slower logic clock.

| Rule | Observed original result | Reproduction |
| --- | --- | --- |
| Ground acceleration on flat | +12 per frame; speed 240 after 20 RIGHT frames | `accelerate_release`, frames 0..19 |
| Release friction on flat | -12 per frame; 240 -> 228 -> 216 | Same action, frames 19..21 |
| Opposite-direction braking | 240 -> 112 -> -128; next LEFT frame -140 | `reverse`, frames 19..22 |
| Standing jump launch | vy=-1664, radius 19 -> 14, Y changes by 5 for the ball shape | `jump_held`, frame 0 |
| Gravity | vy=-1664 -> -1608, increment +56 | `jump_held`, frames 0..1 |
| Held jump | 61 airborne samples; apex center Y845.1875 | `jump_held`, 70 frames |
| Tap jump | vy=-968 at frame 1 after release; 38 airborne samples; apex Y906.4375 | `jump_tap`, release at frame 1 |
| Rolling entry and friction | Down at speed240 produces 228; subsequent frames lose 6 each | `roll`, frames 19..22 |

OBSERVED: the launch frame changes posture and velocity but does not yet move
the ball upward; the next frame moves by the old velocity, then adds gravity.
INTERPRETATION: update order matters to short/long jumps; do not infer it solely
from constants. Ground collision also leaves fractional Y bits, so pixel
positions are not a sufficient reference for exact arithmetic.
TARGET: tests for these rules in original units, two logic steps per drawn
frame if the TI bench supports them. Exact whole-ROM state is not the goal.

### Terrain and first-slice route

OBSERVED IN ORIGINAL: after removing enemy/ring slots fully, holding RIGHT
traverses the first band without a jump or spring. The bridge is used around
X1108..1258 (standing-on-object status bit 3); Y changes through its flexion.
At X1520..1538, center Y876 and angle0; around X1598 angle8 begins the next
descent. Flat terrain at the endpoint is distinct from the spawn's slope.
TARGET IMPLEMENTATION: stop marker X1536 (4.8 original screen widths), retain
render data through X1664, and reset fallen players below Y1200 to the start.

SOURCE OBSERVATION: object11 is a 12-log bridge, centers X1096..1272 at Y904
at rest. The static terrain is missing at X1088..1279. A rigid top at Y896
is the agreed first-milestone approximation. Object1C
subtype3 at X1072 and1296 is decorative bridge stump scenery (`col_none`),
not a solid platform. Object41 at X820,Y808 is a vertical yellow spring on
the upper route; it is not needed by the verified lower traversal. No loop
in the first chunks of the selected band.

SOURCE OBSERVATION: the layout is at FFA400, decompressed chunks at FF0000,
512 bytes per 256x256 chunk, with 256 words selecting 16x16 blocks. Descriptor
bits0..10 select the block, bits11/12 flip X/Y, bit13 enables top solidity and
bit14 sides/bottom. For the local REV00 ROM: GHZ collision index at64A00,
angles at62900, normal 16-column signed heightmaps at62A00.
TARGET IMPLEMENTATION: extract this small band's terrain from the local ROM/reference
RAM, keep generated data local, and implement only the rules this band needs.

### Collision marker correction and first engine

OBSERVED: flat heightmap angles can contain bit0=1. Treating that as a one-unit
slope makes a standing jump move horizontally and introduces vertical speed
on flat ground. The original `Sonic_FindSmaller` treats bit0 as an angle snap
marker; the extractor normalizes cardinal markers. The recorded flat actions
then match the C velocities/flags/X for every sampled frame. Y is within one
original pixel; source subpixel collision residue is deliberately not chased.

TARGET IMPLEMENTATION: byte grid of collision profile IDs (128x128), 23
profiles of 64 bytes with solid row masks, signed column heights, angle and
solidity flags. Diagnostic half-scale terrain is a 52x64 TileMap, 27 tiles.
The complete data blob is 22,944 bytes, in both host and TI byte orders.
`tools/extract.py` regenerates it from the verified ROM and original start
state; it does not depend on a disassembly checkout.

Commands: `make test`, `make xcheck`, `make tihash` from this directory.
OBSERVED: all unit/reference tests pass, including the complete traversal;
eight PC/TI scene checksums match; 240 per-frame state hashes match for each
of walk, jump and roll scripts. The state hash enumerates fields, so host
padding and byte order do not affect the comparison.

Command: `../../tools/bin/ti-cycles --file sonterr.89y --keys keys/walk.txt
--frames 180 sonicc.89z`. OBSERVED: update average20,602, render98,436 cycles;
first-frame run (`--frames 1`) costs14,836 +174,368. These exclude grayscale
ISR and frame waiting; compare with the project's ~360k available game budget.
The traversal average includes reaching the endpoint. Code `sonic.89z`8,319
bytes; data `sonterr.89y`23,040 bytes. Compiler assembly was checked: no
`__mulsi3`, divide/modulo32 helpers or floating-point helpers.

Titanium hardware-path check: clean `ti-emu restart sonic.89z sonterr.89y`,
then `sonic()` after the group transfer, RIGHT held1.2s and 2ND held0.25s,
one `ti-shot x/titanium.png --lcd`, ESC held0.4s. OBSERVED: program/data load,
grayscale terrain and outlined actor display, RIGHT advances the player and
camera (actor moves from its starting screen X40 to camera-follow X80).
The capture is after the short jump input, not a timed airborne measurement.
ESC was sent after the capture; headless tests verify its normal exit result
and remain the evidence for jump/terrain rules. No new hardware implementation
was added.

## Interaction milestone (2026-10-05)

TARGET: same bounded slice, diagnostic presentation, rings and only its three
placed badnik types. No additional level, spring/monitor mechanic, score,
lives, sound or original death sequence. Damage without rings uses the
prototype's immediate attempt reset; no permanent HUD is added.

OBSERVED IN ROM BYTES: GHZ1's complete six-byte object layout matches the
local disassembly's `objpos/ghz1.bin` at ROM offset 6B096 (1,290 bytes). X is
big-endian; Y's low12 bits are position, with flip flags above; object ID bit7
marks remembered state. `tools/extract.py` reads directly from the verified
ROM, not the checkout, and expands ring groups with the source's subtype
spacing rules. The portion before X1536 contains:

| Object | Original placement | Count / role |
| --- | --- | --- |
| Ring25 | X324,Y864, horizontal spacing24 | 3 |
| Ring25 | X820,Y556, vertical spacing24 | 6, upper route |
| Ring25 | X1132,Y800, horizontal spacing24 | 5, above bridge |
| Motobug40 | X832,Y940 | Ground patrol |
| Buzz Bomber22 | X1056,Y816 | Flying patrol and diagonal missile |
| Chopper2B | X1184,Y1120 | Periodic jump from below bridge |

Command: `make generated/objects_ref.h`. `tools/measure_objects.py` clears
all 96 complete 64-byte object slots, places Sonic at X192,Y940 on verified
flat ground, and injects one actor. Long observation runs give Sonic 255 flash
frames so missiles cannot kill the observer and freeze original object updates.
Local `sources/sonic1_md/objects.json` contains provenance and every sample;
`generated/objects_ref.h` is consumed by PC tests only. The script also probes
ring pickup, stationary contact with ten rings and descending ball attack.

OBSERVED: ring count at FFFE20 increments once, on the second sampled frame
after ring initialization. Collision extents in the source are ring/missile
6x6, Motobug20x16, Buzz24x12 and Chopper12x16; Sonic's object reaction uses
width 8, height equal to posture radius minus 3. Attack animation is roll/ball.
TARGET: immediate pickup during the target collision pass (no artificial
object-initialization delay); collected rings stay collected until reset.

OBSERVED: Motobug falls with gravity 56, settles to Y945 on this flat test at
frame 7, switches to vx=-256 at frame 8, then drives one original pixel/step.
Buzz starts vx=-1024, moves four pixels at frame 1, stops when X distance<96,
waits 30 steps and spawns a missile at frame 31. Its first missile movement is
frame 62: vx=-512, vy=512, from X260,Y892; spawn-to-movement delay 31 steps.
Missile direction/offset follow the parent (X±24, Y+28); a destroyed parent
cancels the charging missile. Buzz flight and turn waits are 128 and 60 steps.
Chopper starts vy=-1792, adds 24 after each position update and resets that
velocity once below its original Y. These are original units, halved only
by rendering.

OBSERVED: contact with ten rings immediately enters routine 4, clears the
ring counter, spawns ten ring-loss objects, gives vx=-512/vy=-1024 and
flashtime 120. Hurt motion adds 48 gravity each logic step and ignores steering.
The controlled original lands at frame 44 and restores flashtime 120 there;
normal control then decrements it. Ring reaction refuses collection while
flashtime>=90. A descending ball attack in the original destroys the badnik
and reverses the post-gravity vy (sample -824); upward attacks instead add 256
to vy. The source caps spilled rings at 32, uses gravity 24 and three-quarter
vertical rebound, and staggers floor probes across four frames. REV00 resets
the shared 255-step life timer of already scattered rings on another spill.

TARGET IMPLEMENTATION: own portable C actors in fixed arrays, camera-band
activation, 32 ring-loss slots, four missile slots. Recoil is ±512/-1024;
controls return after landing, followed by 120 logic steps of flashing. Ball
attacks destroy badniks but do not cancel active damaging projectiles. Lost
ring directions use the generated sine table; scattered rings expire or can
be reclaimed after the source's delay. The SNC2 blob appends 17 six-byte
placement records, totaling 23,046 payload bytes (TI file 23,142). TI `rt_file`
reports 23,052 because AMS includes the six-byte OTH trailer: bounded payload
validation must allow it. The code file is 14,497 bytes.

Commands: `make test xcheck tihash ti profile`. OBSERVED: all tests pass;
three controlled actor traces match positions, fractions and velocities for
40/110/160 logic steps. The keyed playable traversal finishes at drawn frame
245 with two kills, one ring and zero resets; RIGHT alone without attacks dies.
Thirteen scene checksums agree PC/TI. Six 260-frame state replays agree for
walk, jump, roll, full play, damage and 32-ring stress, including every mutable
actor/particle field. Old terrain regressions remain alongside the new tests.

OBSERVED WITH `ti-cycles`: per-frame prefix differences in `x/cycles.json`
give update+render average/peak 168,835/287,022 for 250 play frames;
190,628/301,730 for 90 ten-ring damage frames;
210,787/345,196 for 90 stress frames with 32 rings. All include the cold first
TileMap build and stay below the ~360k game budget. Masked8x8 sprites replace
repeated per-ring rectangles; particles completely covered by the actor drawn
last are skipped. This visibility optimization leaves representative screen
checksums unchanged (4B3A, 6E32, E1AA). Compiled `sonic.c`/`objects.c` hot paths
contain no 32-bit multiply/divide or float helpers. No new assembly or hardware
path was added; the user's TiEmu session was not touched for this milestone.

## Milestone 5: ROM presentation and bounded rendering costs

OBSERVED: the pinned upstream libretro frontend does not export VDP memory.
`tools/md/expose_vdp.py` adds read-only exports in the ignored local checkout:
VRAM via memory ID3, core CRAM via 0x10000, registers via 0x10001. `MD.vdp()`
normalizes VRAM word byte order. Boot/input/state replay still passes, and the
tool integration check also validates the exports. Nothing is emulated on TI.

OBSERVED: GHZ1's RAM chunk layout at FFA400 and block map at FFB000 select
VDP patterns using 16x16 block attributes; chunk descriptions begin FF0000.
Palette RAM FFFB00 is 0BGR with three-bit channels at bits1/5/9. Sonic's live
mapping pointer is 0x211E2, the DPLC table is 0x217FE and uncompressed art is
0x21AFE in the pinned REV00 ROM. Five-byte mapping pieces use column-major
tiles; each DPLC word encodes a tile count minus one and a 12-bit source index.
The original renderer's normal intensity expands three-bit CRAM channels to
even four-bit levels before RGB565 quantization. The extraction check compares
496 visible Sonic pixels against the original video, within one RGB565 decode
unit. Foreground-priority grass covering the shoes is excluded from that check.

TARGET IMPLEMENTATION: offline two-by-two sampling, four greys, one-pixel
white dilation around Sonic and the three badnik types. Rings/logs retain dark
small shapes without an exterior outline. The background uses white/light
grey; a repeated initial plane-B template is flattened behind the foreground
in one opaque TileMap. Static background placement is a deliberate adaptation:
no runtime parallax or MD priority compositor. Sonic selects walk/run/roll,
duck and hurt ROM poses from target speed/state; exact source animation timing
and slope orientations remain outside this presentation milestone.

OBSERVED: the SNA1 art bank contains 232 target tiles, 148 sprites including
pre-mirrored variants, and 6,400 bytes of pre-shifted ring rows; payload 54,184
bytes, TI variable file 54,280 bytes. Terrain/objects remain in the unchanged
SNC2 bank. Archived variables are read in place. All extracted assets, atlas,
review captures and generated indices remain ignored. Review:
`x/art-review.png` combines start, rigid bridge, jump and 32-ring damage scenes.

TARGET OPTIMIZATION: cached ring sprite descriptors avoid repeated bank reads.
A dense spill fitting 32x32 is composed in native rows in its original drawing
order, then blitted once through ExtGraph. Pre-shifts are generated offline.
The four ordinary ring poses have dark plane equal to opacity, checked by the
asset validator. Sparse spills keep the last identical pixel-position draw:
keeping the first could change how intervening overlapping rings are painted.
The actor outline contains transparent holes, so the earlier diagnostic
rectangle's opaque-player occlusion shortcut is no longer applicable.

Commands: `make test rendercheck pc ti xcheck tihash profile`, plus
`make -C tools/md check`. OBSERVED: art checks and previous physics/interaction
tests pass; 13 scene checks and six 260-frame full-state replays agree PC/TI.
Across three 260-frame scripts, optimized planes equal the full individual
sprite renderer on every frame. No new assembly or hardware code was added.
The current TiEmu session was left to the user; presentation hardware validation
is milestone 6. Per-frame measurements and final file sizes are in README.md.

OBSERVED ON DEPLOYMENT: the user then requested a TiEmu launch. A clean
Titanium restart loaded `sonic.89z`, `sonterr.89y`, `sonart.89y` together and
ran `sonic()`. `x/titanium-art.png` confirms ROM scenery, outlined Sonic and
grayscale output. No gameplay inputs were sent before handing the running
build to the user; presentation is checked, controls remain unverified here.

OBSERVED ON REQUESTED PLAYBACK: a later user request authorized playing and
recording. A clean reload followed by `ti-play keys/tiemu_play.txt 30.1` and
`ti-gif x/sonic-titanium.gif 12 15` gives 180 frames of real Titanium gameplay:
movement, jumps, scrolling, ring pickup/loss and arrival on the bridge. The
wall-clock script takes damage and does not reproduce the headless win at
frame245. All held inputs were released at the demo's final event.
The first recording also exposed the old fixed GIF crop: at a resized
464x1037 window it filmed casing and a partial LCD. `ti-gif` now uses the
default skin's native LCD rectangle, scaled to the actual window dimensions.
Native rectangles were read from the Docker .skn headers (Titanium:
440x984 image, LCD63,94..379,294; TI-89:443x1006, LCD60,104..379,304).
The corrected capture is reviewed through sampled GIF frames; ROM-derived
captures remain local. Reusable lessons are in `ti-port-md` and `ti89-emulator`.

## Shared 68000: investigation, not authorization to add ASM

OBSERVED IN DISASSEMBLY: Sonic movement reads absolute-short RAM globals;
the jump routine calls collision, trigonometry and sound, and alters the
caller's stack on one path (`addq.l #4,sp`). Hardware routines depend on VDP,
interrupts and MD RAM addresses.
INTERPRETATION: shared instructions make studying arithmetic and possibly
isolating a pure routine easier, but do not make the whole engine callable
on AMS. Whole-game binary reuse would require extensive relocation and a
hardware/runtime redesign.
TARGET: portable C engine, exact small calculations when useful, original
traces as behavioral reference. Consider a native routine only with a measured
reason and the user's ASM agreement.

Primary reading aids:
[Sonic routines](https://github.com/sonicretro/s1disasm/blob/AS/_incObj/01%20Sonic.asm),
[variables](https://github.com/sonicretro/s1disasm/blob/AS/_Variables.asm),
[Genesis Plus GX API](https://github.com/libretro/Genesis-Plus-GX/blob/58c341487e5bfcf979ea68413c7987633adb0c56/libretro/libretro.c).

## Open questions

- Verify slope landings more closely before adding mechanics that depend on
  them: current air landing takes horizontal speed as ground speed.
- Exact animation timing and slope poses may be refined if an observed visual
  issue warrants it. Background parallax is omitted for the current budget.
- Bridge flexion and springs remain outside the agreed rigid-bridge scope.
