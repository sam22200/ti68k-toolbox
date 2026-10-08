# Windjammers: first local Neo Geo preparation

## Status and evidence

The local cartridge and supplied `roms/neogeo/neogeo.zip` pass chip audits,
frontend/core checks, original cold boot/input and deterministic replay.
A reproducible two-human Beach first service supplies a validated entity
field map and original walking/throw timelines (1882 replay frames).
The rules milestone checks wall sweeps, neutral contact boxes, point zones
and both losing players' next serve (1390 more replay frames).
These court/character choices are reference fixtures. Native TI controls,
opponent, scaling and rule adaptation remain implementation decisions;
the first native Beach training prototype is now checked separately below.

The user's `/roms/neogeo` resolves here to the repository-local
`roms/neogeo/wjammers/`; there is no host `/roms/neogeo` directory.

## OBSERVED: chip identity

`make -C tools/neogeo check` validates the following eleven local chips
against FBNeo's `wjammersRomDesc` at revision
`0282ae7e0dc647316043f2f6b2cd312d3164218a`. All sizes and CRCs match; the
total is 9,699,328 bytes. Source filenames end in `.bin`; the staged archive
uses the driver's canonical chip extensions.

| Source stem | Driver name | Bytes | CRC32 | Use |
|---|---|---:|---|---|
| `065-p1` | `065-p1.p1` | 1048576 | `6692c140` | 68000 program |
| `065-s1` | `065-s1.s1` | 131072 | `074b5723` | FIX graphics |
| `065-c1` | `065-c1.c1` | 1048576 | `c7650204` | Sprite graphics |
| `065-c2` | `065-c2.c2` | 1048576 | `d9f3e71d` | Sprite graphics |
| `065-c3` | `065-c3.c3` | 1048576 | `40986386` | Sprite graphics |
| `065-c4` | `065-c4.c4` | 1048576 | `715e15ff` | Sprite graphics |
| `065-m1` | `065-m1.m1` | 131072 | `52c23cfc` | Z80 program |
| `065-v1` | `065-v1.v1` | 1048576 | `ce8b3698` | Samples |
| `065-v2` | `065-v2.v2` | 1048576 | `659f9b96` | Samples |
| `065-v3` | `065-v3.v3` | 1048576 | `39f73061` | Samples |
| `065-v4` | `065-v4.v4` | 1048576 | `5dee7963` | Samples |

`make -C tools/neogeo prepare` regenerates a SHA-256 manifest and reference
inputs under ignored `sources/windjammers_neogeo/`. A canonical
`wjammers.zip` preserves every chip byte; its name selects the FBNeo driver.
No BIOS or game data is included in the committed tooling.

## OBSERVED: P1 byte order

Raw P1 SHA-256:
`cd2684eaa7fe3f457573c026099b70e9b30fcda6c2bf2e27683151b91b674767`.
Its bytes at offset `100` begin `EN-OEG\0O`. Swapping adjacent bytes yields
the CPU-order `NEO-GEO\0` signature. The exported 1 MiB big-endian image has
SHA-256 `53dc6e3d48729c74cef82e6b2dfde5af67af88ea445d820c509346677601c641`.

The normalized header identifier at `108` is `0065` (hexadecimal bytes,
not decimal 65). The first two big-endian vector longs are SP `0010F300`
and reset PC `00C00402`. That reset address is in the BIOS range; it is not
the cartridge gameplay entry. The export is useful for 68000 big-endian
Ghidra inspection at cartridge base `000000`, with the actual BIOS/mapping
handled separately. The normalization is reversible. Do not swap the files
inside the FBNeo ZIP, or apply it to graphics/audio chips.

## INTERPRETATION: reference hardware, not game measurements

The pinned [FBNeo driver definition](https://github.com/finalburnneo/FBNeo/blob/0282ae7e0dc647316043f2f6b2cd312d3164218a/src/burn/drv/neogeo/d_neogeo.cpp)
identifies `wjammers` as a two-player MVS title with nominal output 304×224.
Check actual callback geometry and crop settings before scaling. The
[reference core's memory/video implementation](https://github.com/finalburnneo/FBNeo/blob/0282ae7e0dc647316043f2f6b2cd312d3164218a/src/burn/drv/neogeo/neo_run.cpp)
maps ordinary work RAM at `100000..10FFFF` with mirrors and maintains
graphics RAM and two palette banks. These facts do not identify player,
disc or score variables. Normalize exported memory only after validating
the pinned host core's byte/word representation with live pokes.

For graphics decoding inspect
[Neo Geo ROM loading and sprite decoding](https://github.com/finalburnneo/FBNeo/blob/0282ae7e0dc647316043f2f6b2cd312d3164218a/src/burn/drv/neogeo/neogeo.cpp),
[FIX decoding](https://github.com/finalburnneo/FBNeo/blob/0282ae7e0dc647316043f2f6b2cd312d3164218a/src/burn/drv/neogeo/neo_text.cpp)
and [sprite-chain drawing](https://github.com/finalburnneo/FBNeo/blob/0282ae7e0dc647316043f2f6b2cd312d3164218a/src/burn/drv/neogeo/neo_sprite.cpp).
Pair order, plane arrangement and tile orientation must be reproduced,
then checked against video. There is no validated art extractor yet.

## OBSERVED: completed reference-instrument milestone

`tools/neogeo/neogeorun.py` uses FBNeo/libretro revision
`a49cfac4b97cc62d0196c1d0cde8f5b14fde662c`, with its Neo Geo subset,
speedhacks and fastmath disabled. The current compiled core SHA-256 is
`6957555bbae881770c92c519cafea0bcf1580249b83ff80de852f0831a6db66d`
(includes the initial-RTC export added during the gameplay milestone).
The driver-only audit revision above remains the chip-identity source.

The supplied BIOS ZIP SHA-256 is
`36a47cf50a585cafc812e19ef7646ad1556a5de90440d9dcb06f373926cd68ea`.
Actual selected BIOS: `sp-s3.sp1`, CRC32 `91b64be3`, SHA-256
`d45b04e849e2b76ba72fd45feba45cd5bb16831cd0a809943f9ddf80e2d05c6d`.
The default game DIP selects MVS Asia/Europe ver.6 (1 slot), not AES or
UniBIOS. No prior NVRAM/card data is imported. Core-reported timing is
59.18 Hz, geometry 304×224. These are reference settings, not TI choices.

`make -C tools/neogeo reference` passes after 2400 no-input cold-boot frames,
visually in Windjammers' intro. The earlier 720-frame point is still the
Neo Geo splash, so the integration now waits for the game itself. The
120-frame continuation and state-restored replay match every work-RAM,
VRAM, palette, NVRAM/card, Z80 RAM, RGB and exported-status sample.
Byte, unaligned word and long RAM writes agree with the emulated CPU's bus
reads; restoring the state removes all diagnostic writes.

Both ports' live descriptors confirm A/B/C/D = RetroPad B/A/Y/X, with
coin/select ID2 and start ID3. Live probes check P1 right/A, P2 left/B,
P1 coin and P2 start independently. The runner now handles libretro's
`SET_INPUT_DESCRIPTORS` command11; descriptors arrive during the first
run after controller refresh, not necessarily at construction.

The ignored `sources/windjammers_neogeo/reference.json` records all chip/core/
BIOS hashes, effective options, 34 input descriptors and per-frame hashes.
CLI save/load, PNG and video/status exports were exercised separately.
No diagnostic RAM pokes are used during the normal cold boot or replay.

## OBSERVED: first service, walking and disc reference

`make -C tools/neogeo gameplay` runs
`tools/neogeo/keys/windjammers_serve.txt` from fresh cold boot. At post-frame
4350 (4351 calls to `retro_run`), two humans, H. Mita/Japan on the left and
B. Yoo/Korea on the right, stand at the first service on Beach. The right
player owns the disc; the clock word is BCD `0027` (the initial round
starts at 30, but three seconds have elapsed at this door). These character/court choices
isolate reference measurements, not a selected TI adaptation. BIOS, mode,
DIPs, persistent-data initialization and rate remain as recorded above.
The title ignored simultaneous P1/P2 Start in the trial; the successful
script uses P1 Start followed by the explicit `1P vs 2P` mode choice.

Two independent cold boots must match every exported fingerprint at the
door. They run in separate processes: reinitializing a real Neo Geo driver
in the pinned FBNeo library caused an invalid-pointer abort after unload.
The runner now rejects reinitialization with a fresh-process instruction;
save/load remains the supported way to repeat probes in one process.
With separate processes but the original host-clock initialization, two RAM
bytes (`1000EE`, `10FDD8`) and two raw NVRAM bytes (`005D`, `040F`) differed
in the sampled boots; all other exported fingerprints matched. Inspection
of `BurnGetLocalTime` / `uPD4990AInit` confirms the host local calendar source.
The runner now sets the initial emulated RTC to 2000-01-01 Saturday 00:00:00
before the first CPU frame, through a separate explicitly labelled export.
Counters/registers and the RTC update path remain intact. It advances on
normal emulated ticks; metadata records the configuration. No RAM differences
are masked in the cold-boot or replay comparison.
A 160-frame uninterrupted sequence equals its state-restored replay.
Sixteen 92-frame movement trials, three 80-frame throw trials and three
diagnostic trials also match every RAM/VRAM/palette/NVRAM/card/Z80/RGB/status
sample on replay: 1882 checked frames total, excluding preparation/boot.
The libretro core can duplicate the first restored video callback. The check
explicitly restores the frontend's saved last-presented surface for that
duplicate; RAM/core state and subsequent frames remain independently checked.

The local outputs are `sources/windjammers_neogeo/gameplay/`: serve state,
PNG, snapshots, per-trial CSVs and a provenance/hash/field-map report. Inputs
and source checks are committed; all ROM-derived outputs remain ignored.

### Field map validated against the running ROM

All addresses use CPU-order big-endian work RAM. Each entity's `x/y` long
contains an integer high word and an unsigned 16-bit fractional low word.
Velocities are signed 16.16 longs. Positions are logical game coordinates;
they are not directly framebuffer pixel offsets.

| Entity | Base | X | Y | VX | VY | Render X/Y words |
|---|---|---|---|---|---|---|
| P1 / Mita | `100800` | `100806` | `10080A` | `100828` | `10082C` | `100814 / 100816` |
| P2 / Yoo | `100880` | `100886` | `10088A` | `1008A8` | `1008AC` | `100894 / 100896` |
| Disc | `100A00` | `100A06` | `100A0A` | `100A28` | `100A2C` | `100A14 / 100A16` |

Player action words at `100824/1008A4` distinguish the observed neutral `0000`,
walking `0400`, possession `1004` and ordinary-throw animation `1400` cases;
these values are observations, not a complete action enumeration. Disc word
`100A22` is `0002` during the sampled possession, `0004` in ordinary flight.
At the door, players/disc are `(40,138)`, `(280,138)`, `(280,138)`.

Controlled directional differences isolate the position/velocity longs.
Independent diagnostic trials put P1 at `(80,110)`, P2 at `(230,110)` and
confirm their render-coordinate copies and visible relocation. A separate
in-flight disc injection starts at `(210,110)` with velocity `(-2,+1)`;
four original frames advance exactly to `(208,111)..(202,114)`.
Diagnostic pokes validate fields; ordinary trajectories contain no pokes.

### Walking, release, reversal and bounds

All eight directions, both characters, are checked over 80 held frames,
four released frames and eight reverse frames. No acceleration is visible
in these neutral walking trials: velocity takes its measured value on the
first pressed frame, becomes zero on release and reverses immediately.

| Character | Axial speed | Diagonal component on each axis |
|---|---:|---:|
| H. Mita | `00028000` = 2.5 px/frame | `0001C480` = 115840/65536 px/frame |
| B. Yoo | `00024000` = 2.25 px/frame | `00019740` = 104256/65536 px/frame |

Every sampled frame matches addition of the input-selected velocity followed
by clamping **only the integer coordinate word**. P1 X range is `27..141`,
P2 X `180..292`; both Y ranges are `76..188` on Beach. The updated fractional
word survives the clamp; the outward velocity remains nonzero while held.
For example, P1 held right at the middle barrier alternates `141.5,141.0`,
rather than becoming a constant fixed-point 141.0. These measurements apply
to these characters, neutral walking and this court only.

P1 is measured from the serve door. P2 is measured after an ordinary A throw
and 40 frames, once its observed action becomes neutral. This preparation
is made with normal inputs, not a forced action flag or suppressed AI.

### Ordinary disc trajectories

All times below are **local post-frame indices** after restoring the serve
door. P2 presses the listed keys on frames 4 and 5, releases on frame 6;
P1 is idle. Button A starts the throw action at frame 4. Direction affects
the sampled animation/release timing; it is not one universal delay.

| Input | First flight frame | Initial signed VX/VY | Wall rebound frames |
|---|---:|---|---|
| A | 16 | `-00057000 / 0` (−5.4375 / 0) | none before catch |
| Up + A | 16 | `-0003D830 / -0003D830` | 32, 66 |
| Down + A | 12 | `-0003D830 / +0003D830` | 27, 61 |

Both diagonal components have magnitude 251952/65536. P2's action becomes
neutral at local frame 32 in all three sampled cases. The straight shot is
caught automatically by idle P1 at frame 53; the up/down shots enter sampled
possession at frames 78/72. Catch extents are not inferred from these examples.
Between release and catch, all non-contact frames match `position += velocity`.
Wall-contact frames invert the vertical direction and use **partial movement
on both axes**. Up+A frame 32, for example, changes X by −151104 instead of
−251952. Its original CSV records the exact contact timeline; a full-step
update followed by sign reversal would fail that trace. The exact solver is now checked by the rules experiment below.
Lob/special actions remain outside these ordinary A-shot experiments.

## OBSERVED: Beach walls, contacts, points and next serve

`make -C tools/neogeo rules` rebuilds the same original first-service door
from cold boot and checks `check_rules.py` against the running cartridge.
It prepares left/right/up flight states with normal inputs, compares every
RAM/VRAM/palette/NVRAM/card/Z80/RGB/status frame on replay, and exports CSVs,
states and a provenance report to ignored `sources/windjammers_neogeo/rules/`.
Diagnostic writes are recorded explicitly as address/value/size triples;
they are absent from the two original diagonal trajectories and the two
missed-shot/next-serve sequences. No TI port or art conversion is claimed.

The host wall oracle was reconstructed using offline inspection of the
normalized program and verified against original frames. Capstone was used
for inspection only; it is not needed to run the committed checks. Avoid
linear disassembly through embedded tables; the old GCC4TI objdump also
misdecoded some word-sized branches in this image.

### Wall sweep and rounding

Program `025E3C..025E4C` integrates flight, then calls `02AB42..02AC10`.
For the tested velocities, away from side/corner geometry:

1. With both speed magnitudes below one pixel, test the fully integrated
   position directly. Otherwise select a substep count N.
2. Start with `D6=VY`, `D7=abs(VX)`. If D7 is below one pixel, replace D6
   with its absolute value. Compare D6 and D7 **unsigned**, select the
   greater one's signed integer high word, take its absolute value and add
   one. A negative VY can therefore select a different count from positive
   VY of the same magnitude: N=5 up, N=4 down in these diagonal trials.
3. Quantize each component as
   `step = trunc_zero(signed16(velocity >> 5) / N) << 5`.
   Test successive substeps from the pre-integration position, stopping at
   the first wall contact. No residual movement is applied after contact.
4. The top wall contacts integer Y below 72 and sets it to 72; the bottom
   contacts integer Y at least 200 and sets it to 199. Preserve the contact
   position's fractional word. X also retains only the completed substeps.
5. Without contact, keep the caller's full `old_position + velocity`,
   rather than the quantized substep sum. Thus ordinary flight has no
   quantization drift. Reflection reconstructs velocity from angle/speed
   (`0275EE / 027644 / 0277B2`); arbitrary injected velocities do not
   necessarily reflect by just negating their Y component.

The four normal rebounds at Up+A frames 32/66 and Down+A frames 27/61
match exact X/Y fractions and reflected velocities. Another 56 diagnostic
one-frame trials cover both walls, fractional edges, axial-dominant,
vertical-dominant and subpixel velocities. Their checks concern contact
position, not a guessed velocity reconstruction for injected angles.
The relevant map dispatch is `02AC12`, cell lookup `02B170`, half-cell
walls `02AE36 / 02AE52`; this oracle excludes corners and raised discs.
For a TI engine, generate quantized steps per measured angle/speed offline;
do not copy the original division into a hot loop.

### Pose boxes and catch versus deflection

The original resolves a collision descriptor through program table
`078000` and pose offsets at `078004 + 2*pose`. The first rectangle's
center offsets are signed words +8/+10 and half-extents words +12/+14.
The disc's ordinary sampled box has zero offsets and half-extents (8,8).
Mita's neutral sampled boxes have center offset (0,-4), half-extents
(13,15); Yoo's have (0,0), (14,15). Neutral overlap therefore includes:

| Defender | Disc integer center relative to player |
|---|---|
| Mita | X -21..21, Y -27..19 |
| Yoo | X -22..22, Y -23..23 |

Axis comparisons are inclusive and use integer position words; fractions
on either entity do not enlarge the box. The 168 two-frame diagnostics
check edges, corners, interior and separate fractional placements on both
players. They resolve the pose **selected by the original**, rather than
claiming a complete animation-selection model.

Overlap can become a catch or a rear deflection. Descriptor bytes +0/+1
bound an angular arc, mirrored by entity flag bit4. In `01AA08..01AA38`,
catch classification is `((angle-start)&255) <= ((end-start)&255)`; a flipped
pose replaces endpoints with `(-end)&255, (-start)&255`. The checks validate
that predicate with the **original's collision angle**, not an independent
atan2 model. Pose/turning and the collision-center angle both matter; do
not replace it with a rectangle-only or universal left/right catch rule.

Contact words `10081A / 10089A / 100A1A` contain `04xx` for catches and
`08xx` for deflections, with angle in the low byte. Collision detection
marks the current post-frame while the disc remains flight state `0004`;
the following frame consumes the contact and enters possession `0002`
or deflection flight `000C`. The observed catch knockback action is `1000`,
then holding `1004`; those do not describe the whole action machine.

### Goals, BCD points and return to serve

Low ground-flight side entry uses half-cell dispatch `02B0E6 / 02B0FC`:
integer X below 16 on the left, at least 304 on the right, sets disc flag
bit2 at `100A20`, and goal flag bit3 at `100B81`. Eight one-frame diagnostic
crossings check the exact integer/fraction thresholds on both sides.
These are logical coordinates; framebuffer geometry is still 304×224.
Raised shots, corners and other arenas require their own checks.

The scorer `0120C8..0120E8` compares integer Y with a selected [low,high)
interval. For Beach, `012684` selects from table `0126E2` using left/right
zone bytes `1000A3 / 1000A4`, **not the court ID**. Both start at index1,
which means **Y=120..167 awards 5 points, outside awards 3**. The table also
contains index0 (136..151), index2 (104..179) and index3 (88..199); their
selection behavior is not inferred from the fresh-serve experiment.
Sixteen goal injections check both recipients, low/high boundaries with
fractions and outer zones. This prevented an incorrect initial reading of
the table's first entry as the default Beach zone.

Round points are **BCD bytes** `100873` (P1) and `1008F3` (P2), accumulated
by ABCD at `01224C / 01215A`. The separate arcade totals are BCD longs
`100066 / 10006A`: one 5-point goal adds displayed 500, raw `00050000`.
Do not use the high-score mirror at `1000E0` as the match score.
The configured winning threshold byte `1000FF` is BCD `12`; complete
round/match ending and score overflow are not checked yet.

In the original missed-shot control, P1 holds Up for local frames 0..19;
P2 presses A on 4/5, then both release. The normal shot has Y=138.

| Local post-frame | Observed transition |
|---:|---|
| 60 | Disc X=15.3125, goal flags and P2 round points=5 |
| 62 | Disc enters goal animation `0012` |
| 154 | Disc reset state `0000` |
| 210 | Ballboy delivery state `0014` |
| 238 | P1 holds the next serve, action `1004`, disc state `0002` |

The losing player receives service and points persist through reset.
The other no-write missed-return sequence independently awards P1 five
points and returns service to losing P2. The round clock continues during
this sampled goal pause. From the prepared right-flight door, the other
sequence awards points at frame41 and P2 receives service at frame220.
Its BCD seconds word is `10008C`, subsecond
counter `10008E`; this experiment does not check time-out or overtime.

## OBSERVED: first native mechanics prototype (M1 historical baseline)

`games/windjammers/` implements a portable C Beach training court on the
runtime, at half scale. Read its [README](../../../../games/windjammers/README.md),
[RE_NOTES](../../../../games/windjammers/RE_NOTES.md) and
[ROADMAP](../../../../games/windjammers/ROADMAP.md) before extending it.
Normal TI build: `windjam.89z` (12603 bytes); PC: `windjam_pc`.
The provisional mode is human Mita versus a simple native Yoo practice
partner. Controls, court crop and training rule choices are explicit TARGETs.

The generator also measures and fully replays three Mita throws (480
additional original frames), with magnitudes `49800` straight and `33F78`
diagonally. Release/throw-neutral timing is the same 16/16/12 and 32 pattern
in this fixture; the first catches are at 62/88/84. No diagnostic writes are
used to prepare this original hold state or throw those shots.

The native neutral angle model independently selects quadrants, scales and
quantizes doubled integer collision-center differences using ROM table
`01927E`; it reproduces all 168 original contact words. It does not take the
original collision-angle field as an input. Full action/turn/animation
selection is not covered by this equality claim.

Native tests compare 1472 movement and 437 launch/flight frames, 168 contact
words, 14 ordinary fractional wall placements and 16 goal probes. Equality
intervals stop before adapted catch knockback and separate original pause/
serve animations. Native practice and reset are tested on their own.
Seven scenarios match 1540 PC/TI per-field hash samples and final screens;
SDL state continuation and 30,000 native practice steps also pass. The peak
measured draw/update frame is 168550 datasheet cycles, under 360k. No new
hardware paths were introduced; TiEmu was not launched for this slice.

The ROM-derived angle table, fixtures, original states and reports stay in
ignored `games/windjammers/generated/`. Program disassembly and native
screens/cycle logs stay in ignored `x/`; only our source/tools/docs are kept.

## OBSERVED: native M2a captures and ROM actors

The current native program is `windjam.89z` (49,299 bytes, Titanium), with
139 local outlined ROM pose/flip variants. The six manual action fixtures
compare 660 steps through capture/recoil/stationary lockout and settled hold:
both players' X/Y/VX/VY/action and disc X/Y/state. They come from eight fully
replayed 180-step normal-input trials in `generated/actions.json`. The two
no-input auto releases are measured separately: Yoo speed 96, Mita 100, versus
174/147 for the selected manual shots. They are not implemented natively.

Catch source `01B6FA..01B730` copies incoming velocity when speed>=160;
`01CABE..01CB38` reconstructs the velocity from the current angle/speed
before halving speed on alternate steps. Mita receives Yoo's174 and uses
speed174,87,43,21,10,5,2,1, with axial/diagonal trig factors2048/1448.
Yoo receives Mita's 147 and stays stationary. Mita integrates 16 recoil steps
(the final one becomes hold); Yoo has 12 catch steps and hold on step 13.
`01DA2C..01DB8C` uses character limits plus the holding flag: holders crossing
the left integer limit26 move to31, preserving fractions. These are character
and state limits; the earlier pose-extent explanation was a working hypothesis.

The C-chip pairs are interleaved and decoded using the pinned core's planar
convention. Pose table32020 and renderer012DEE..013086 provide columns, tiles,
flips, offsets and palettes. Entity byte+3 overrides descriptor palettes:
Mita 0x2A, Yoo 0x27. Walk direction selects facing; cosmetic sequence clocks are
native and do not drive the collision model.

`make art-check` checks 1,329,378 visible original RGB actor pixels across 1256
scenes/22 trials, including every walking direction and ordinary actions.
It observes 135 pose/flip pairs. Visibility is resolved from opaque later
VRAM banks before RGB comparisons; no actor action samples are excluded in
these trials. The entity RAM-to-VRAM/video pipeline has a one-step offset;
same-step pose comparisons fail at transitions even though tile decoding is
correct. See the native README/notes for generated reports and exact scope.

Native unit/stress tests, 1980 PC/TI per-field hashes over nine doors, final
screen checks and actual SDL state-file continuation pass. The six measured
compiled timelines stay below 360k cycles per draw; current costs are in the
game README. The hardware backend did not change.

The requested native display now fills the entire 160x100 LCD, with compact
two-digit scores and a 30-second countdown between them at the top. Original
numbered 3/5/3 bands have centered geometry at LCD Y=49.5 and upright digits
on both sides. The native net has a black outline, pale core and darker mesh
joins, without a ground shadow. The user's latest request retains completely
white sand without marks and the current score/countdown layout: preserve
these constraints when adding later artwork.
The native training clock ticks through goals and stops at zero; round endings
remain M3. Scores saturate at 99. A 1000-frame PC/TI replay additionally crosses
clock zero (2980 field-hash samples and ten final screens in total).
The court uses offline coordinate lookup tables;
ROM actors remain half scale, with cosmetic boundary clamps for visibility.
Physics/collision units are unchanged. See the game README for current layout
and per-frame measurements.

`make court-check` validates 41,344 original RGB first-service scene pixels.
Decoded VRAM depth independently selects 3464 panel pixels, 360 net pixels
and 228 retained shadow pixels for exact comparison. Base banks192..211 use
palette40 sand shades for the baked shadow; pole bank228 and goal banks262..267
provide original reference geometry. The current native numbered bands and
outlined net do not render these original panels or shadow; the reference RGB
check does not claim equality for the native redesign.
The native white-floor/grey adaptation
is precomposed into five opaque 32x100 strips, 4000 plane bytes. This selected
static scenery is not a claim of full Beach/FIX or raster decoding.

## TARGET: remaining action fidelity and court art (M2b)

Next, measure moving/action-pose contacts, turning, early throws during
capture, automatic holds and rear deflections, then compare native state
through whole rallies. Ordinary idle-defender capture timelines are covered
by M2a; full original animation state is not.
Resolve corner/raised collisions and any dash/lob/charge/special actions
needed by the chosen scope. Decode remaining Beach/FIX art only within the
requested plain white-floor style, validating it before grey conversion.
Round/time-out ending and original
CPU behavior require their own probes. Additional characters/arenas remain
separate measurements; ROM-derived assets stay local.

## OBSERVED: native M2b1 possession and ordinary return strength

M2b1 implements ordinary automatic releases and delayed manual returns while
preserving the approved full-screen design. See the native RE_NOTES/README
for the original addresses, field semantics and bounded equality claim.
`measure_holds.py` replays 29 no-write trials twice (7140 exact fingerprints),
including two 600-step automatic rallies. Native tests compare 4058 selected
steps through manual release/flight/settled capture and complete automatic
controls, including velocity, action and possession history/bonus.

Player+3A saturates64; automatic returns trigger there, while manual A tests
before incrementing age. Human history word+52 becomes `(history+age)>>1`;
normalized age subtracts `history>>3`, adds byte+4D's opening bonus and clamps
0..64 after score-deficit normalization. The bonus decreases by6 per return.
ROM028D50 character tables select ordinary speed by normalized age buckets;
age64 has fixed speed96. 102 native velocity/substep/recoil profiles are
precomputed offline. Captures test incoming speed>=160, so slow automatic Yoo
returns correctly give stationary Mita catches.

At M2b1, early capture input, speed>=192 catch timing, original charge/special paths,
nonholder anticipation, moving/action-pose contacts and rear flight remain
separate. Training score/serve/AI adaptations remain explicit. New power/bonus
fields invalidate older native state files. Hardware code is unchanged.

## OBSERVED: native M2b2 timed lift/charge and immediate returns

`games/windjammers/tools/measure_timing.py` uses incoming doors created by
normal serve inputs. 78 no-write trials replay twice with10920 identical
RAM/VRAM/palette/video/status fingerprints. Native fixtures compare6704 steps
through stationary ready timing, lift/complete charge/recapture and immediate
straight/diagonal returns with full-speed captures; eight failed-preparation
block classifications follow separately compared pre-contact prefixes.
The game RE_NOTES records original addresses, windows and coverage bounds.

Ready word+3A combines a low-byte age and high-byte timing flag. Mita's flag
is active at ages2..4; Yoo's at1..3. Yoo's ready contact box is wider and has
a mirrored X anchor, so preparation changes contact time. Compare the prior
ready flag when consuming contact; do not invent a shared human-input window.
A during an ordinary catch starts a return before recoil/hold integration.
The original also injects A/B release edges only while possessing (01B272),
so a long held A can release a normal catch when the button is let go.
Capture interruption retains a12-step release animation even for downward
throws; settled downward throws release at8. Fast288/256 catches are checked.

Lift preserves incoming fractions at the player's integer X/Y. Vertical
velocity starts0x34000, gravity0x1500. Automatic charge completes on step45
for Mita,42 for Yoo; its flag survives descending recapture. Native ready/
charge saves and scheduler edges have tests on both sides. Fifteen PC/TI doors
plus1000 training frames pass4300 field hashes and16 final screens; hardware
code is unchanged. Current frame costs and program size are in game README.

At M2b2, native charge art uses an existing hold pose with a visible bar; failed block
flight and airborne misses are explicit adaptations. Charged release currently
falls back to an ordinary throw. Full special flights/lobs/rebounds continue
in M3a, directional arcs/curves in M3b. Moving/other action-pose contacts and
rear deflection still need their own comparisons. Recreate pre-M2b2 saves.

## OBSERVED: native M3a lobs, specials and return counters

`games/windjammers/tools/measure_advanced.py` adds62 no-write trials, twice
replayed with18600 identical reference frames per pass, and8573 native fixture
steps. Read game RE_NOTES for original addresses, arena-specific target/birth
tables, height/damping/gravity, wave and wall-accelerating specials, special
lob landing/ground acceleration and strong recoil/counter edges. Ten controls
confirm that early movement/A/B do not cancel this measured charge action.
The selected Beach is arena1: do not substitute arena0's airborne Y bounds or
LOB row/target offsets. Neo Geo B is libretro bit8, not bit1.

Reference controls include complete scoring outcomes. Native comparison
intervals stop before goal-entry/celebration differences; normal-lob target
jitter is conditioned on the recorded target, not the original machine-wide
RNG. Idle hold-counter reuse and the special-lob100C/1010 pose selector stay
outside equality, while the strong-reaction family and exact recoil are checked.
Native complete missed-lob rebound/goal/service, score ceiling and eight
airborne/special state-file continuations have separate tests. Twenty-three
PC/TI doors pass6060 field hashes and24 screens. Approved court/HUD art stays
unchanged;139 actor variants are now read in place from wjart.89y (30008 bytes),
with a38949-byte Titanium program. Current cycle costs are in game README.
At M3a, saves gained trajectory fields and directional arcs continued in M3b
(see below). Additional original special poses remain the action-art slice. No new ASM or
hardware path is introduced.


## OBSERVED: native M3b directional arcs and curved throws

`games/windjammers/tools/measure_curves.py` repeats180 no-write input trials
(43200 identical original frames per pass) and exports17764 native equality
steps. Both arc directions, eight final directions, 1..5-step segment sweeps,
neutral/partial motions, normalized power below/above16 and B's separate lob
branch are covered. Twelve controls continue from a curved front reception
through immediate/settled returns. Read game RE_NOTES for the exact intervals,
source pointers, curve aim/release tables, speed-scaled32-bit rotation and
wall-to-straight transition. No global RNG or goal-celebration claim is added.

01D0CA reads nine bytes of the64-byte direction history. Neutral and Up both
encode0; each of the two newest direction segments may last at most four
logic steps. Low normalized power requires three consecutive adjacent
45-degree directions; high power requires only an adjacent recent transition
plus a differing older sample. Curve animations release at12/16 and end at24,
separately from ordinary throws. Top-wall contact normal128 and bottom0 are
critical after angle wrap. Native dynamic flight retains arbitrary angles
through straight rebounds and reconstructs ordinary decaying recoil.

The approved LCD design and139-pose archived bank are preserved. Eight native
saves resume gestures or curved flight, checking state and planes through
native goals/service. Twenty-seven PC/TI doors and a long replay through clock
zero pass6940 field hashes and28 screens. Current build sizes and frame costs
are in the game README. Recreate pre-M3b native saves. Curve pose extraction,
moving/other action-pose contacts and rear deflection remain separate;
round/time-out rules are the next match slice. No new ASM or hardware path.

## OBSERVED / TARGET: native M3c dash and action readability

The local measurement tool `games/windjammers/tools/measure_dash.py` records
48 no-write trials and repeats all1728 original frames per pass. Both free
characters execute direction+A in eight directions with three button hold
lengths. Native fixtures compare816 positions/velocities, including signed
component decay and fraction-preserving clamps. Single-frame taps integrate
11 Mita/13 Yoo steps; longer A holds delay action progress while motion keeps
decaying. Horizontal-wall recovery and moving-pose contact fidelity remain
separate, explicitly adapted in the native engine. Read its RE_NOTES for the
exact comparison interval before extending that claim.

The eight-pixel outlined disc, quarter-scale height, fixed destination target
and distinct moving lob shadow improve visibility without adding scenery.
An aligned-incoming A cue, wider charge bar and full-charge star clarify
stationary precision reception and automatic charging. Tests follow that cue
through actual lift, complete charge, recapture and special release on both
sides. Direction+A dashes; stationary A prepares the original short lift
window. Holding a button with an ordinary held disc does not charge it.

Twenty-nine native scenarios plus the clock-zero replay pass7380 PC/TI hashes
and30 screens. Compiled profiling covers5040 frames, peak230746 cycles under
360000. Normal Titanium build41461 bytes; unchanged wjart bank30008 bytes.
Native saves made before M3c must be recreated. `make showcase` produces the
named Windjammers TI-89 GIF from1185 native frames, covering all implemented
action families with command captions outside the full LCD. Its44.14-second
loop preserves game cadence; all captures and ROM-derived data remain ignored.

Native visual polish adds fading afterimages for speed-word>=256 ground
shots and stronger energy/spark trails for charged shots, special lobs and
their ground acceleration. It samples four real projected positions once
per draw, outside rendering. Effect history is saved and hashed explicitly;
pre-trail saves must be recreated. Tests check actual path samples, clearing
on catches/goals, pure repeated renders and five saved continuations. Six
active-trail PC/TI screenshots match alongside7380 field hashes and30 final
screens. Current build42235 bytes, 5040 profiled frames, peak231442 cycles
(64.3% of360000). The named showcase includes the trails. These are native
effects, not a claim to reproduce the cartridge's effect artwork.
