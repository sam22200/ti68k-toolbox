# Yoshi's Island reference notes

Keep findings labeled OBSERVED / INTERPRETATION / TARGET. The local ROM is the
behavioral authority; the US community disassembly is only a reading aid.

## Identity and reference door — OBSERVED

- Local European ROM: 2,097,152 bytes, no 512-byte copier header; LoROM header
  at `7FC0`, title `YOSHI'S ISLAND`, mapping/type `20/15`, region `02`.
- SHA-256 `91a4dc481c54b620cb3bccaffe5fa3f69db955ae600309414d18bb59307cba90`.
  Header checksum and full-ROM byte sum both `E10B`.
- Snes9x 1.63, pinned revision `fae2fea08f74180759ef540ee94259213f503480`;
  locally patched PPU exports, core binary SHA-256
  `bb1f2c64ff6584c4a02280c06fc7ae96f76e1c6e31a78c892fd869e730765d8e`.
  The generated `start.json` is authoritative if a rebuild changes this hash.
- Measured frame rate 50.006978908188586; callback video 256×224. Default
  Super FX clock 100%, compatibility timing, CPU overclock disabled.
- `tools/reference.py`: 9900 scripted cold-boot frames reach the untouched
  opening tutorial, mode `13`, stage `0B`, X135/Y1888. A single cartridge-RAM
  poke `70008C:2=1000` invokes the original tutorial exit on the next run.
  `keys/ref_enter.txt` plus 620 frames selects and settles level 1-1, mode
  `13`, stage `00`, X119/Y1904, velocity zero. Boot scripts and their hashes
  are recorded in `start.json`; no manual save or SRAM is required.

## Candidate memory confirmed on PAL — OBSERVED

| Address | Size | Meaning / evidence |
| --- | --- | --- |
| `70008A` | 2 | X fraction/intermediate, changes on directional motion |
| `70008C` | 2 | Player X; input timeline and an idle live X160 poke |
| `70008E` | 2 | Y fraction/intermediate, full signed integration residue |
| `700090` | 2 | Player Y; jump timeline |
| `7000AA` | s2 | Vertical velocity; tested against complete C timelines |
| `7000B4` | s2 | Horizontal velocity; direction/brake/reversal timelines |
| `7000A8` | s2 | Horizontal motion velocity; previous VX on the controlled flat floor |
| `7000B6` | 2 | Ground angle; zero in controlled probes, varies on the first hump |
| `7000C0` | 2 | Jump state; 0 ground, 6 ascent, 7/8 descent in these probes |
| `7000D2` | 2 | Flutter state; low phases 0/1/2/4/6/8 measured, high byte zero in these cases |
| `7001D0` | 2 | Skid countdown; refreshed to 4 against the current velocity sign |
| `7001D4` | 2 | Shared phase/fall/landing countdown |
| `7001E6` | 2 | Flutter cooldown countdown; starts at 15 in the PAL probes |
| `7E0030` | 2 | Logic counter; increments each frame in stationary probes |
| `7E0118` | 1 | Game mode: local normal gameplay is `13` |
| `7E021A` | 1 | Stage index: tutorial `0B`, level 1-1 `00` |

Cartridge RAM is essential: these player variables are not in WRAM. Snes9x
exports 32 KiB SRAM for this Super FX ROM even though its ordinary header
SRAM-size byte is zero. Exported words use little-endian order.

The US disassembly labels normal gameplay mode `0F`; our PAL ROM uses `13`.
Do not copy its mode numbers into PAL boot logic. Its variable names led to
the address candidates, which the local experiments then confirmed.

## First movement and jump observations — OBSERVED

`make measure` writes raw before-input sample -1 and post-frame samples 0..89
for idle, right, left, brake, reverse, tap, 35-frame hold, long flutter hold
and release/repress. No terrain or actor is replaced in these timelines.

- Direction from rest adds/subtracts 20 velocity units per logic step.
  After 30 RIGHT frames, VX600; neutral braking subtracts 40, reverse input
  initially subtracts 70. RIGHT reaches a 920/940/960 speed cycle; do not
  silently substitute a guessed hard cap. Horizontal positions include slopes.
- Jump input is SNES B. At natural flat X119/Y1904, first sampled jump velocity
  is -1328, with no vertical displacement until the following frame.
- Held B adds 50 to VY each subsequent frame; release adds 200. Ordinary
  falling speed is limited to 1280. The one-frame tap rises 14 pixels; the
  35-frame hold rises 66 pixels, returning to Y1904 on frame 48.
- The fraction has eight effective fractional bits, but the stored 16-bit
  word is the entire signed intermediate. Integrate `(old_sub & 255) + vy`,
  store it, and add its signed high byte to Y. Landing stores fraction `FFFF`.
  This model matches every Y/fraction/VY sample on the PC and compiled TI.
- Holding beyond frame 35 enters flutter: phase 4 at frame 35 (VY344), then
  phases 6/8/4 in two-frame segments. Later VY decreases by 16 per frame.
  At frame 89, Y1833/VY57, phase0. The controlled model below now reproduces
  this full timeline, including release/repress and rearming.

## Controlled movement milestone — OBSERVED

`make movement` saves twenty 240-frame cases in local `movement.json`, with
the reference identity, events, all raw samples and this explicit intervention:
**write only X=119 at `70008C` before every original frame**. It preserves the
X fraction, velocities, Y, camera, original collision and actors. Post-frame
X still reveals motion integration; no speed or floor result is injected.
All samples remain in normal mode `13`, angle0, with one logic-counter tick
per displayed frame. These are isolated flat-ground/air tests, not traversal.

- First decrement phase timer, cooldown and skid timer when nonzero.
- X integrates the previous VX. Its fraction is the full signed sum of the
  old low byte and VX, and its integer advances by the signed high byte.
  Y integrates the newly updated VY. The horizontal controller runs afterward.
- Held direction accelerates by 20 toward target ±960 normally, ±480 during
  active flutter (phase>=2). At/beyond the directional target it brakes toward
  zero by 40, without a hard cap: right runs 920/940/960; left from rest is
  symmetric. Reversal refreshes skid4 while moving against the input, then
  continues acceleration70 for the remaining nonzero skid countdown. Thus
  reversal can produce a different terminal cycle (e.g. -970/-930/-950).
- Neutral on the ground brakes40 to zero. Neutral in air preserves velocity
  strictly inside the current limit; at/beyond it, brake40. This also handles
  releasing direction when the flutter limit becomes smaller.
- Jump press sets VY=-1328 and jump state6 after movement/collision, including
  a press on the landing frame. Holding adds50, releasing adds200, fall cap1280.
  Stationary and running flat-ground jumps share these vertical rules.
- Flutter phase1 waits for **old VY>352** while B is held, then enters phase2.
  Active flutter subtracts `16 + (max(old_vy-272,0) >> 3)` instead of gravity.
  A zero phase timer reloads2 and advances phase by2, wrapping >8 to4. A timer
  already active from falling can leave phase2 visible for several frames.
- Release or **old VY<=-384** cancels flutter, stores phase0, jump6, cooldown15,
  then applies ordinary gravity. The subsequent nonnegative-VY jump-state
  update uses the *previous* jump state; copying the apparent new state here
  gives incorrect cancellation traces.
- Phase0 rearms only on a fresh B edge with decremented cooldown<8; phase1
  waits until cooldown zero. Holding through automatic flutter end cannot
  rearm it. The fast/late/too-early rearm cases cover these distinctions.
- Nonnegative ordinary motion with phase<2 advances jump6→7→8 when the shared
  countdown reaches zero, reloading8. Landing sets Y1904, fractionFFFF, VY0,
  jump0 and countdown5; its flutter phase is retained until the next ground
  update, which sets phase1.

`mixed_controls` was added after the initial model to check combined reversals,
short/held jumps and landing input. All twenty cases match every stored field.
Transformation modes, flutter high-byte flags, slopes, ceiling/wall collisions
and actor effects were not measured by these movement tests. Collision and the
first interaction implementation are described in the later milestones below.

## Experimental pitfalls — OBSERVED / INTERPRETATION

- OBSERVED: poking X160 from the natural spawn snaps idle Y1904 to Y1898:
  that location lies on a slope, so it is unsuitable for a flat-floor model.
- OBSERVED: poking X288 without moving the camera causes automatic X retreat
  in four-pixel steps until roughly X251. INTERPRETATION: the original camera/
  streaming bounds constrain a teleported player. The final measurement
  protocol preserves the natural spawn/camera rather than fitting those
  transients as physics.
- OBSERVED: an initial input-only traversal stopped on the flower tutorial
  and then on the Baby Mario recovery message. A held button need not produce
  the fresh edge required to dismiss a message. Those probes do not establish
  an unassisted five-screen winning route.

## Flat probe milestone — OBSERVED (retained regression)

The earlier flat-ground milestone implements the controlled movement above.
Scenarios1/2/3/10/11 retain its diagnostic floor; scenario10 anchors X119.
Scenario0 now uses the actual terrain described below. Bounds remain X16..1264.

- PC tests: 4800 complete original states (twenty cases ×240 frames), plus the
  earlier 180 ordinary-jump Y/fraction/VY samples, restart/quit and both bounds.
- Actual compiled TI (`tihash`): all 4800 complete original states agree on
  X/Y, fractions, VX/VY, flutter phase, phase timer, cooldown, jump and skid,
  with grounded derived from original jump0. Explicit packed fields avoid
  host/TI struct-layout comparisons.
- Prior flat build: `xcheck` with `keys/movement.txt` gave identical PC/TI screen
  checksums for its five scenarios at frame81 and frame240. One PNG was reviewed:
  diagnostic actor and floor are visible; ROM art is intentionally still absent.
- 720 prefix-run differences under datasheet-counted `ti-cycles` cover every
  frame of `flutter_hold`, `air_reverse` and `mixed_controls`. Update+render
  Current flat regression mean **89,877**, peak **91,504**
  (mixed_controls frame220: update1842/render89662).
  This excludes hardware grayscale/timing and is not the future game's cost.
- Generated `-Os` game assembly uses 16-bit operations and shifts; no multiply,
  divide or 32-bit arithmetic/float helpers. No new assembly or TiEmu run.

The provisional probe clock remains 51.2 steps/s, 2.4% faster than this PAL ROM.
Matching discrete state timelines does not establish identical wall-clock speed.

## Five-screen terrain survey — OBSERVED

`make survey` starts at the original 1-1 door and scripts RIGHT/held B, with
fresh A presses for reading messages. Before **every** frame it writes zero
to the 24 sprite-state words at `700F00 + 4*slot`. This deliberately suppresses
those actors; it does not establish an unassisted playable route. Removed
sprite platforms/objects must be restored when validating collision semantics.

| Capture | Post-frame index | Player X/Y | Camera X/Y |
| --- | --- | --- | --- |
| 0 | 0 | 119 / 1904 | 0 / 1756 |
| 1 | 111 | 331 / 1866 | 257 / 1756 |
| 2 | 208 | 589 / 1854 | 513 / 1740 |
| 3 | 284 | 843 / 1816 | 768 / 1671 |
| 4 | 373 | 1098 / 1774 | 1024 / 1677 |

The run reaches X1281/Y1837 at frame461, camera1207/1733, stage00/mode13,
Baby Mario state8000. The five views and complete streaming-memory snapshots
are local under `sources/yoshi_snes/survey/`, together with generated keys and
the complete timeline/provenance in `survey.json`. Camera thresholds256/512
are crossed by one pixel; this is five view samples, not a stitched map.
At this survey stage, the safe target endpoint and hidden collision profiles
were still unverified; the following terrain milestone resolves those points.

A preliminary attempt to use the candidate invulnerability timer `7001D6`
with value1000 froze movement; that experiment was discarded. Do not treat
an address label as proof that an arbitrary injected value has the intended
effect. The final survey uses the explicit sprite-state suppression above.

## Decoded collision geometry — OBSERVED

`tools/terrain.py` uses the cold-boot 1-1 RAM map, not pixels from the video.
The screen table is cartridge RAM `700CAA + ((y>>8)<<4) + (x>>8)`; mask its
ID with63. A screen holds256 little-endian Map16 words in WRAM:
`7F8000 + (id<<9) + ((y&240)<<1) + ((x&240)>>3)`.
The original's on-screen cache at `70409E` is distinct from this full map.

Local PAL ROM page metadata begins at file offset `53B12` (CPU `0A:BB12`),
three bytes per Map16 page: collision flags, special type, slope kind. The
headerless ROM hash and initial table signature are checked before extraction.
Flags1/2/4 are one-way ground, solid and slope; non-colliding decorative and
collectible pages remain empty in the collision bank. Only the selected band
X0..1279/Y1536..2047 is extracted; map storage uses padded128-cell rows.

The slope sample table begins at file offset `53D0E` (CPU `0A:BD0E`). Ground
sample address is `base + (shape<<7) + ((x&15)<<3) + 2`; byte0 is angle, byte1
signed height. Its ground coordinate is tileY + signedHeight +1. Solid and
one-way flat ground use tileY directly. Two slope kinds extend across rows;
keeping signed heights and searching adjacent rows handles these correctly.

Player X/Y is the top-left coordinate, not feet. Normal feet contacts are
(X+3,Y+32), (X+8,Y+32), (X+13,Y+32); the highest valid surface determines
stationary landing. Side probes are X+1/+15 with Y+9/+23; head contacts are
X+6/+10 with Y+4. These offsets came from the US reading aid and are checked
on this PAL ROM through actual contact timelines, rather than assumed geometry.

`terrain-reference` recreates surveyed camera doors with the original terrain
survey protocol, then injects zero-speed drops40 pixels above the selected
surface, jump6/fractions0. It anchors X and suppresses the24 actor words before
every frame. All 27 drops land on update10; their complete Y/fraction/VY/jump/
angle/phase/cooldown/head timer timelines match PC and actual TI code. They
include flat ground, shallow/45-degree slopes, platforms and the endpoint.

Two additional controlled rises confirm collision direction and ceiling rules:

- X744/Y1708, initial VY-1328, held B35: passes upward through the one-way
  surface1696, reaches Y1641, then lands at Y1664 on frame39. No ceiling contact.
- X1008/Y1672 with the same rise: hits the isolated solid tile at (1008,1648)
  on frame2, retaining fraction64412, VY0, Y1658, head timer8 and cooldown20.
  Head contact pushes Y down one pixel per frame until1660, rather than snapping
  immediately to that coordinate. Gravity stays zero while the head timer is
  nonzero. It finally lands on the lower ground at Y1840 on frame66.

Total independent original collision checks: **404 states** (27×11 +40 +67).
Original actors are suppressed for these probes; this does not validate enemy
contacts, moving sprite platforms, tongue, eggs or Baby Mario recovery.

## Native five-screen traversal — OBSERVED / TARGET

OBSERVED: `yjterr.bin` / `yjterr.be.bin` / `yjterr.89y` are generated locally.
The 7296-byte bank contains 4096 padded collision cells, 11 profiles, a padded
48×24 visual map and 26 diagnostic tiles. Four original 16×16 cells combine into
one 16×16 target tile offline. The runtime's existing ExtGraph TileMap handles
scrolling; archived data are read in place and include the allowed OTH trailer.
Host/TI tiles use native/big-endian words; metadata remains big-endian on both.
The bank and generated original fixtures are ignored, not distributable assets.

OBSERVED: normal `yjprobe()` starts directly at X119/Y1904 on actual terrain.
The scripted route in `keys/terrain.txt` reaches X1264/Y1882 on frame590 and
freezes until ENTER; the whole 16-pixel body remains inside X0..1279. The same
original endpoint drop confirms Y1882 and slope angle238. The camera follows
both axes within native X0..480/Y0..156. Falling beyond2048 restarts at spawn.
The actor is an outlined8×16 black diagnostic box; ROM graphics are pending.

OBSERVED: `terrain-check` compares all 600 native replay states PC=TI, including
camera, collision angle, head timer and finish. Eight collision/scenario screens
match at frame11; the replay matches at frame301 (checksum2E3B) and frame600
(checksum1A54). One TI-memory PNG of the scrolling route was reviewed.
The full 600-frame prefix profile has mean **83,280**, peak **193,552** cycles
(frame204: update5712/render187840), including cold TileMap/cache rebuilds.
This is update+render, excluding hardware grayscale/timing. It passes the
provisional 210k-cycle budget at 51.2 Hz; interactions/art need their own budget.
`-Os` game and terrain assembly contain no multiply/divide instructions or
32-bit arithmetic/float helpers. No new assembly or hardware-driver change.

TARGET / limitation: the ground-following controller uses a 16-pixel support
window and the previously measured flat movement. It does not yet reproduce
the original's slope speed projection or slope jump impulse; its camera is
our own. The native replay is not claimed frame-exact to an original traversal.
Original geometry and stationary collision timelines are the verified scope.

## Grilling and project scope — TARGET

Level1-1 and five original256-pixel view widths belong to the user's request
for this project; neither porting skill prescribes them. Half scale, camera,
the standard TI input mapping and the provisional51.2Hz clock had previously
been treated as implementation assumptions without grilling. They are now
explicitly provisional. Questions about commands, Baby Mario damage/recovery
versus immediate restart, and original egg aiming/rebounds versus a simplified
throw are pending. Their answers do not block isolated tongue/ingestion work.

## Interaction milestone4a — OBSERVED

`actors-reference` regenerates the original cases and a separate actor census.
The census scripts RIGHT, B for80/100 frames, Y every20 and A every10. Before
each frame it suppresses flower ID00AD and restores Baby Mario state8000.
These interventions bypass introductions/damage and establish placements
only, not an unassisted original winning route. The census finds12 placements
inside the band. Five are Shy Guy ID001E: (496,1904), (560,1904), (688,1840),
(848,1824), (1152,1808). Other IDs00AD/0181/00FA/0066/00BA still need their
own behavioral study; do not infer types or collision from their video alone.
Generated placement arrays remain local in ignored `generated/actors_map.h`.

The controlled capture door naturally loads the first Shy Guys with170 input
frames, suppressing only the flower introduction. It then injects the player
once at X464/Y1888, zero velocity/fractions (Y fractionFFFF), grounded, facing
right, flutter1. The real first Shy Guy remains in its original loaded slot23,
at X496/Y1904 with idle timer20. No per-frame anchoring or actor suppression
is used in the samples. Empty tongue cases use the untouched spawn; the left
case only pokes facing2. All sampled frames retain attached Baby Mario8000.

| Address | Size | Confirmed meaning |
| --- | --- | --- |
| `7000C4` | 2 | Facing:0 right,2 left |
| `700150` | 2 | Mouth phase:1/2 horizontal extension/retraction,3/4 upward |
| `700152/154` | s2 | Tongue relative X/Y, in original pixels |
| `700168` | 2 | Captured slot offset+1 (93 for slot23) |
| `700162` | 2 | Holding slot offset+1 |
| `7001E0` | 2 | Tongue/spit/ingestion phase timer |
| `7001EE` | 2 | Automatic-swallow countdown; starts1200 on capture |
| `701DF6` | 2 | Egg-inventory byte offset; divide by2 for actual eggs |
| `700F00 + 4*slot` | 2 | Actor state:16 ordinary,8 captured |
| `7010E2/1182 + 4*slot` | 2 | Actor top-left X/Y |
| `7010E1 + 4*slot` | 1 | Actor X fraction |
| `701220 + 4*slot` | s2 | Actor horizontal speed |

Tongue is SNES Y, swallowing is Down. Extension advances8 pixels per update.
Held extension reaches56, then pauses on a3/2/1 countdown before retracting8
per update. After early release, horizontal extension reaches at least32;
upward extension reaches at least40. Holding cannot retrigger a completed
tongue; a fresh edge is required. Capturing the controlled Shy Guy occurs on
frame1, switches directly to retraction without the endpoint pause, and starts
the1200 countdown. The actor remains in the mouth after the tongue closes.

Down starts mouth47 immediately, then phases51/55/59/63/67/71/75, with durations
3/3/7/3/3/2/2/3. On phase67 one egg is added; the holding slot clears at the
end of phase75. The stale automatic-swallow countdown continues to decrement
even after swallowing. A fresh Y with an actor in the mouth starts spit phase1
and timer10; release occurs eight updates later with timer2 still stored.
The controlled horizontal release has VX640. Native follow-on spit flight and
contacts are not claimed original-exact by these mouth-state comparisons.

Fourteen cases contain2090 eight-field action-state samples, including the
1240-frame automatic-swallow case. They match the original on PC and actual
compiled TI after explicit normalization of slot93→1 and egg offset/2.
The flat Shy Guy case additionally matches90 X/Y/fraction/VX/state samples:
20 idle updates, acceleration5, then speed85/90 oscillation. Its X integrates
the newly updated speed, unlike Yoshi's X. Copying the player's update order
made the initial actor fraction incorrect; the independent trace caught it.

## Native interactions and rendering — OBSERVED / TARGET

OBSERVED: the native band now has the five Shy Guys, horizontal/upward tongue,
capture, manual/automatic swallow, mouth/spit release and a six-egg reserve.
`keys/actors.txt` eats the first two actors and reaches X1264/Y1882 with two
eggs on update692. All800 native states match PC/TI, including every actor
field, camera and frozen finish. Scenario53 additionally checks80 dense-scene
states, covering all16 horizontal bit offsets. All 80 dense-scene frame checksums
also match between the PC and the compiled TI binary. Reset clears all interactions.
The capacity test injects an already full reserve; it is a target-rule test,
not an additional original six-egg reference trace.

TARGET / limitations: ground support samples the actor's center, with our
64-pixel patrol boundary and camera activation/pause margin. Original slope
AI and full activation logic are not implemented. The native tongue collision
uses the measured actor centers with a conservative overlap heuristic and
static solid blocking, rather than the complete original collision dispatcher.
Spit flight/contact are simplified. Enemy contact currently passes through
the player: damage, Baby Mario loss/recovery/countdown/failure and thrown eggs
remain milestones4b/4c. Other required actors follow in4d. All art is diagnostic.

OBSERVED: `YTR2` keeps the collision cells/profiles/TileMap reference and adds
two canonical byte image planes baked offline from those same diagnostic tiles.
The image stride is96 bytes and height256; the two planes take49152 bytes,
and the whole bank56448, below the TI variable limit. The native bank's tiles
retain host/TI word order; image bytes remain canonical on both. New binaries
require the rebuilt `yjterr.89y`. No extra data variable or image RAM is used.

The renderer reuses Alundra's pure-C image-window method. A separate unrolled
row function reduces GCC4TI register pressure; byte-aligned offsets0/8 copy
words/bytes directly, avoiding shifts and odd-address word reads. Otherwise
it uses the shorter shift, at most7 bits. Actors, hero and reserve use masked
outlined blits. All2970 tested visible screens equal the original TileMap/
rectangle renderer pixel for pixel. Their physical off-LCD padding may differ;
comparisons cover the160×100 display. The smooth camera is retained.

`actors-check` profiles every compiled TI update/render frame: route mean
132726, peak198162 (frame364: update10330/render187832); dense peak202432
(frame25: update19434/render182998). Both fit the provisional210000 budget
at51.2Hz. Hardware grayscale/timing and future ROM art are excluded. Generated
game/terrain/actor assembly contains no multiply/divide instructions or float/
32-bit arithmetic helpers. No new handwritten assembly or hardware-driver code.
The clock still needs a measured PAL scheduling decision before real-time fidelity.

Next: damage/recovery and aiming/throwing, informed by the pending grilling
answers. Keep movement, terrain, original action traces and native replay checks.

## Reading aids

- [Snes9x core](https://github.com/libretro/snes9x/tree/fae2fea08f74180759ef540ee94259213f503480),
  especially `libretro/libretro.cpp`, `ppu.h`, `memmap.cpp`.
- [US 1.0 disassembly](https://github.com/brunovalads/yoshisisland-disassembly):
  `disassembly/vars/sram_vars.asm`, `ram_vars.asm` and `bank01.asm`.
  Third-party code is kept locally in ignored `sources/yoshi_disasm/`.


## ROM graphics milestone — OBSERVED / TARGET

The user explicitly requested graphics next, postponing damage. The existing
movement, terrain and interaction model is retained. No additional grilling
answers are assumed for damage, egg aiming, controls or PAL scheduling.

OBSERVED: local PAL Map16 visual page offsets begin at ROM C32A4; four
little-endian tile words for each cell begin at C33F2 + pageOffset + lowByte*8.
The order is top-left, top-right, bottom-left, bottom-right. The offset table
prefix is 0000/0748/0DD0/0E70/0F38/0F58/0FF8/10C0. `tools/art.py` independently
expands all 80x32 cells using PPU VRAM/CGRAM, including tile/palette indices
and horizontal/vertical flips. 876 fully visible cells agree with the streamed
BG1 tilemap across the five original survey doors. Each door uses its own
Map16 workspace and VRAM: coins already collected by the original disappear,
and flowers can change tile graphics. Initial scenery is baked from view0.

OBSERVED: this level uses Mode1 with 8-pixel BG1 tiles, tile byte base E000,
and the 64x32 streamed BG1 map at D000. BG2 uses 16-pixel cells and a 32x64
map; its decoded tree/cloud pattern is 512x1024. At the first view, camera
BG1 Y1756/BG2 Y794 establishes the flattened offset -962. Native background
uses that alignment and repeats the original pattern at world speed, rather
than reproducing parallax or HDMA. Distant scenery is white/light grey;
foreground terrain retains all four greys. Decorative foreground BG3 is omitted
so it cannot hide the ground edge or small actors. Static coin artwork remains
visible but does not yet collect or animate; this is a graphic-only limitation.

OBSERVED: original OAM objects are decoded as planar 4bpp tiles, palette,
name-table selection, size and whole-object flips. Controlled idle, walking,
held-jump/flutter, horizontal/upward tongue, capture/swallow and loaded Shy Guy
doors supply 29 source frames. Both directions are generated offline, giving
58 logical frames including Yoshi/Baby Mario, Shy Guys and reserve eggs.
`art/poses.json`, original RGBA crops and review sheets remain under ignored
`sources/yoshi_snes/`. Half-scale BOX reduction and four-grey quantization are
followed by one-pixel mask dilation: the actor's white outline clears scenery
without additional runtime drawing. Original egg pixels replace the reserve
HUD's diagnostic rectangles. The tongue remains the previous outlined line.

TARGET: pose selection follows native movement/interaction state; cadence
uses the runtime frame clock, not a claim of exact original animation timing.
Baby Mario remains attached visually. Damage, detach/recovery/countdown and
egg throwing are deliberately deferred. The input and world-state traces are
unchanged; static sprite-bank pointers are outside the saved gameplay state.

OBSERVED: `YTR3` retains the earlier collision profiles and diagnostic tiles
but replaces the rendered image planes with ROM scenery. It is still 56448
bytes. Old `YTR2` data is rejected, so send the rebuilt `yjterr.89y`. `YAR2`
adds an 8586-byte sprite bank, with 58 frame records holding up to two masked
8/16-pixel blocks. Row words are host-native on the PC and big-endian on TI;
metadata and scenery image bytes are canonical. Both data banks are read in
place; no full image/sprite conversion buffer is allocated at runtime.

The first uncropped 32-pixel sprite build peaked at 217730 cycles in the dense
door. Cropping empty rows reduced it to210042. Split blocks reduced the bank
size, but an initial generic two-block loop was slower (210516). The final
32-byte TI frame descriptor, two unrolled calls and existing ExtGraph clipping
remove redundant iteration and bounding tests. Splitting/cropping is asserted
to preserve every source pixel and outline-mask bit offline. No handwritten
assembly or hardware driver changes were introduced.

OBSERVED: all 2970 visible frames match an independent bit/pixel renderer,
including scrolling, clipped blocks, tongue, reserve HUD and the win panel.
All80 dense PC/TI frame checksums agree across all 16 camera bit offsets.
Nine normal/terrain/tongue/actor doors agree at 60 frames; native actor-route
frames 250/800 agree with checksums C457/C6F8. The updated source extraction,
unit tests,4800 original movement states,404 original collision states,2090
interaction states,90 flat actor states,600 terrain-native states and800
actor-native states all pass on the PC and/or actual compiled TI as appropriate.

Current per-frame TI update/render costs with ROM art: native mean 137056,
peak 204252 at frame 499 (update11748/render192504); dense peak 208934 at frame 25
(update19434/render189500). Both are below the provisional 210000 budget.
These costs exclude hardware grayscale/timing, include current animations and
reserve use on the native route, and test five visible Shy Guys in the separate
dense door. The margin is small; future damage/throwing/HUD effects need a new
profile. The final art hot paths have no multiply/divide or float/32-bit
arithmetic helpers; initialization has one 16-bit row-length-validation multiply.

A compiled TI-memory PNG at native route frame 250 was reviewed, with checksum
C457 equal to the PC. Off-LCD physical padding is not part of the160x100 image.
Titanium hardware and exact PAL scheduling remain milestone6; the user's
running emulator has not been restarted or sent keys by this work.


## Contact damage and Baby Mario — OBSERVED / TARGET

**Authorization / TARGET:** after the ROM-art milestone, the user requested
contact damage, a crying baby in his bubble and a ten-second countdown.
Grilling asks original recharge versus a fresh fixed ten seconds per hit.
No answer arrived; the communicated working assumption is an initial
rechargeable ten-second reserve, body/tongue rescue, retained remaining time
on further hits, defeat at zero and ENTER restart. Crying is visual only.
This assumption is recorded separately from confirmed requirements.

**OBSERVED:** `tools/damage_reference.py` regenerates the local PAL contact
case and two rescue cases (700+150+150 frames). It uses the existing genuine
first-actor preparation, one contact pose X490/Y1888, initial invulnerability
zero and tutorial flag `7E0372|0080`. Touch/tongue probes restore frame80 and
inject one airborne player pose beside/in tongue range of the real baby.
No per-frame intervention occurs. Core/ROM/boot metadata is retained in the
ignored `sources/yoshi_snes/damage/reference.json`; original images and PPU
exports are local. The US disassembly remains a reading aid, not the tested
revision.

Confirmed addresses: invulnerability `7001D6`, riding-state bit15 `7001B2`,
baby slot0 X/Y `7010E2/701182`, fractions `7010E1/701181`, VX/VY
`701220/701222`, behavior phase `7019D6`. Stars/tenths `7E03B6`, drain fraction
`7E0392` and recharge fraction `7E0394` are in WRAM. Player VX/VY are
`7000B4/7000AA`; `700094/96` are not these speeds. The CPU614A cartridge-RAM
alias maps to `70014A`, not a naive `70614A`.

The natural hit sets player VX−640/VY−1162,160 invulnerability updates and
cancels the tongue. Riding remains set on the impact frame; next frame the
baby detaches at player X487/Y1885 with VX−435/VY−1162, zero fractions and
phase0. His first24 detached states match the native PC and compiled TI
position, fractions, velocity and phase exactly. Launch applies+80 to VY
before integration; phase10 begins after23 launch integrations. The original
player subsequently holds its position and its invulnerability count has a
32-unit discontinuity at reference frame17; these raw samples are retained.
The whole player-damage timeline is not claimed exact.

The original109-tenths reserve drains one tenth per four reference frames,
reaching zero on frame436 (about8.72 seconds at50.007Hz). Phase2 appears at
438; the following minion sequence is not our failure screen. Touch begins
return immediately and riding resumes after32 frames; the tongue test starts
return later and resumes riding at35. Both hold the stars during return.
Once riding, one counter tenth recharges every12 updates.

**TARGET:** reserve2560 ticks at256Hz, exactly ten wall-clock seconds from
detachment, rather than reproducing the PAL ten-unit duration. Clock subtraction
uses unsigned16-bit elapsed ticks and passes a wrap test. Recharge adds26 ticks
per12 updates, capped at2560. It pauses during the32-update return; another hit
while detached does not refill it. Expiry freezes gameplay and displays a
retry panel. Invulnerability uses160 updates when first losing the baby,
128 for a subsequent hit; input suppression during recoil is eight updates.
Body/actor hitboxes, stomp removal/bounce, bubble's later bounded float,
LCD-edge reflection and32-update homing are native adaptations. No minion
cutscene, lives screen or sound is added.

**OBSERVED art:** original Baby Mario tiles62/64 use object palette5;
bubble tiles9C/7E use palette4. Four controlled cry captures produce three
unique poses. Original Yoshi poses are also extracted without those baby
tiles so the rider disappears while detached. Four-grey white outlines and
tears remain offline or use existing masked primitives. `YAR2` now holds122
directional frames in17700 bytes, with a single32-pixel bubble block. Counter
panels for0..10, normal/alert, are static16-pixel sprites. Tears use one32×4
masked sprite instead of four filled rectangles.

**Measured rendering choices:** the scene deduplicates to233 real16×16 tiles.
A TileMap attempt was pixel-identical and averaged150718 cycles in the dense
scene, but cold/cache-rebuild frames peaked253604, above210000. The retained
C image window moves the shift decision outside the row loop, reads aligned
overlapping long pairs, uses a right shift already at offset7 to avoid
extra clear/swap instructions, and copies ten words per row. No handwritten
assembly. `YTR4` retains the233 tiles for study and packs the reference image
planes at80 bytes per row, total61504 bytes; old banks are rejected. Three
files still suffice. The counter bake and combined tear blit also remove hot
rectangle/glyph work.

**Checks:** full existing movement/terrain/action unit suites pass. Compiled
TI matches4800 original movement states and404 terrain states;2090 original
interaction states and90 actor motion states still match. The new checker
compares24 original baby-launch states,2098 complete native PC/TI states
(including every baby/timer field) and108 screen checksums, covering all16
scroll offsets in the five-actor/six-egg/bubble/HUD door. Native replay finishes
at1192 with two eggs, two hits and two rescues, then freezes. Scenario59
keeps the earlier terrain-only probe separate from damage-enabled gameplay.

Final compiled-TI update/render profiles, every frame measured:
- Route1300 updates: mean122032, peak166658 (frame361: update12036/render154622).
- Dense80 updates: mean168458, peak202398 (frame28: update22056/render180342).
- Expiry540 updates: mean125713, peak160640 (frame512: update872/render159768).

Reports: `x/damage_{route,dense,expiry}_cycles.json`. Budget210000 unchanged;
hardware grayscale is excluded. Generated game hot paths have no multiply,
divide, floating-point or32-bit helper calls. Art initialization alone uses
one16-bit multiply to validate row lengths. Runtime unit tests and screen
cross-check also pass. No hardware driver change or emulator input occurred.

**Runtime finding (headless verified):** `RT_CYCLES` had returned the static
interrupt tick counter even though no interrupts run in that build. Its
`rt_ticks()` now uses the same deterministic virtual clock as platform-sw,
`((u32)rt_frame * RT_FRAME_TICKS2)>>1`. Normal TI builds retain the hardware
clock. The complete countdown comparisons expose the previous frozen clock.

Raw PC state reload also passes: the remaining reserve is serialized, while
the live clock anchor stays outside the POD snapshot. Both an in-session F3
reload and a new-process load preserve the saved time instead of subtracting
the elapsed interval since capture. The tick field records the last observed
clock for diagnostics; it is not used as the restored timing anchor.

## Coins, resting poses and eggs (milestone4c)

**OBSERVED:** PAL Map16 cell6000 occurs21 times in the selected band. A single
player-pose contact injection at the first upper coin increments7E037B to1.
Positions and that observation are in local `items/reference.json`; the
labelled Map16 sheet and four unique coin PPU phases remain local. ROM/PPU
foreground comparison still matches876 cells before coin removal. After
removal218 unique scenery tiles remain, so YTR4 shrinks to60544 bytes.

**OBSERVED:** idle animation state7000F6 takes0/2/4. The controlled PAL door
changes state at97,138,141,264,267,308,311 and417. Four captured idle images,
eight ingestion poses and four egg-ready/throwing poses are unique. Capturing
the first Shy Guy with Y and pressing Down creates one egg. A at65 starts the
ready animation, reaching oscillating aim state6 at72; A at95 starts release.
Inventory size701DF6 decreases when readying the egg. The actual egg is actor
ID25 at offset92. From frame99 its8.8 speed is VX2032/VY−1016; later samples
show constant straight flight. Cursor angle7000EE changes by240 per update
during this measured oscillation. These addresses are confirmed on PAL, using
the US disassembly only as a reading aid. Input replay, complete samples and
source metadata are under ignored `sources/yoshi_snes/`.

**TARGET:** native rectangular collection, one bit per coin, body and egg
collection, and a two-digit score panel. Coins are removed from foreground
before composing the retained original backdrop, then drawn dynamically.
Original red-coin classification, lives and score tally are outside this
request. A64-point spatial path is sampled when Yoshi moves at least4 original
pixels on either axis; followers use positions6/12/18/24/30/36 samples behind,
with idle bobbing. A stationary character never writes duplicate path samples.
This is our trail model, not the original egg buffer translation.

**TARGET:** Diamond readies aim and a second press spends one egg at release;
Down/Shift cancels without consumption. This differs from PAL's earlier
inventory removal. The reticle oscillates over an upper/front arc quantized
to the generated64-direction2272-scaled sine table. Shots have constant8.8
velocity, terrain-axis reflection, a three-contact rebound limit,180-update
lifetime, off-band culling, actor hitboxes and a six-slot pool. The pool cannot
consume an egg when full; holding an enemy blocks throwing. Jump kills the
Maskass with the existing native stomp bounce. Shift keeps capture/spit and
Down deliberate conversion, retaining the original1200-update automatic
swallow timeout. Direction can change while holding; the captured sprite now
travels visibly on the retracting tongue. Idle timing uses the measured pose
doors on a native420-update cycle, reset by movement/jump/mouth/aim.

**Rendering:** YAR3 contains174 directional frames and15360 bytes of masked
small-sprite shifts (39476 total). Ten unrolled C rows replace repeated clipped
8px shifts when entirely inside the LCD. All sixteen aligned-word offsets are
prepared offline, with transparent padding; clipped cases use ExtGraph.
Coin X bins narrow both collection and rendering loops. Pointer loops replace
indexed follower updates/drawing. The frame cache moves from a program-file
zero array to5568 allocated bytes; a stack-convention `atexit` callback frees
it after the runtime restores the OS. No handwritten assembly or runtime
driver changes. The plain TI binary remains below24576 bytes.

**Checks:** units cover animation pixels, unique collection/restart, spatial
following after long idle and jumps, both aim directions, cancellation/empty
reserve/full pool, projectile coin collection and terrain bounce, stomp and
capture/turn/spit versus conversion. Existing4800 movement,404 terrain and
2090 original interaction states pass. Final explicit133-word packets compare
2580 complete native PC/TI states;325 LCD checksums agree, including every
frame of three80-frame scrolling scenes. Five additional540-frame checks
cover earlier damage/expiry/rescue doors. The route still finishes at1192
with two eggs and two rescues. Every compiled-TI update/render pair is measured:

| Scene | Updates | Mean cycles | Peak cycles |
| --- | ---: | ---: | ---: |
| Five actors, six followers, baby, coins/HUD | 80 | 177489 | 209572 (frame28) |
| Aim and throws in upper terrain | 160 | 128024 | 179138 (frame69) |
| Six simultaneous shots, consumed food actors absent | 80 | 138591 | 203716 (frame7) |
| Full native route | 1300 | 134007 | 188656 (frame964) |

Budget remains210000, excluding the hardware grayscale overhead. Earlier
rendering without preshifts/bins/pointer loops peaked226950; the final render
preserves the same visible pixels. New hot-path assembly has no32-bit
multiply/divide or floating-point helpers. Reports are
`x/items_{dense,projectiles,sixshots,route}_cycles.json`. The three rebuilt
TI files are local. No TiEmu keys/restart or hardware driver changes occurred;
hardware verification remains roadmap milestone6.

## Aiming visibility, lock/resume and action preview

**OBSERVED (local PAL):** `make aim-reference` captures/converts the real
first Shy Guy, then records three300-update probes in
`sources/yoshi_snes/items/aim.json`, with source metadata and exact inputs.
In the lock probe, A65 starts aim; L80 sets7000EA toFFFF and freezes7000EE
at2944 through frame95. R96 clears the flag and the sweep advances again.
This confirms a second shoulder press resumes aim on this ROM.
In the downward shot, A74 releases; frame78 has egg ID25/VX2032/VY1451.
Floor contact at81 changes ID25 to24 (green to yellow) and VY to−1450.
Later VY rises by1 per update. The diagonal probe separately confirms a
floor reflection at96, VY444 to−444. Only these observed contacts are claimed;
the native three-contact cap and exact multi-bounce path remain TARGET.

**TARGET:** keep Diamond ready/throw, add Alpha edge-triggered lock/resume.
Lock stores both angle and world direction; turning Yoshi does not rotate
the shot until unlocked. Holding Alpha cannot toggle repeatedly. Cancel,
launch and damage clear lock. The15×15 black/white cursor lies32 target pixels
from the launch center and draws after world actors and the baby. An extra
black center marks locked aim. Horizontal and upward tongues have2px black
ink and a1px white outline; capture/retraction timing is unchanged. Defeated
actors show an8-update outlined impact before disappearing.

**Rendering:** YAR4 adds6144 bytes of offline cursor shifts to YAR3's174
directional frames and small sprites:45620-byte payload. Each cursor pose has
sixteen aligned-word offsets. Fifteen unrolled masked C rows reuse the same
black ink for both planes; clipped cases retain ExtGraph. Initial looped
rendering exceeded the210000 budget at210482; the final aim-dense peak is
209802. No handwritten assembly or hardware driver changes. A compound
unsigned-long shift in the generated tongue bitmap needed an intermediate
u32 cast because LP64 host longs are64 bits; PC/TI now agree at every sampled
capture frame. The plain22937-byte program fits the TI-89 AMS24KiB limit.

**Checks:** existing ROM fixture suites pass;3225 complete native133-word
PC/TI states and1031 LCD checksums agree. The packet now includes aim lock,
stored direction and actor impact timers. Units test lock edge/hold, resume,
locked-world-direction throw, cancellation and floor/wall rebounds. Replay68
jumps from grounded state, rises above Y1850, then stomps at34 without baby
loss. Replay69 releases at55, travels visibly through frames56..63 and kills
the live Maskass at64. Door70 starts with one egg and proves upward velocity
after floor contact. The initial eight-caption GIF sampled every update of
brief actions in slow motion. At the user's request the refreshed preview
now captures all910 consecutive updates, including coins/idle/spit, at a
constant20ms per update (~50fps), without chapter pauses. It lasts18.2 seconds.
One shared palette keeps the LCD greys stable between frames. A new
`items-preview-smooth.gif` link avoids caching the earlier slow preview.

| Scene | Updates | Mean cycles | Peak cycles |
| --- | ---: | ---: | ---: |
| Five actors, six followers, baby, coins/HUD | 80 | 177827 | 209910 (frame28) |
| Same dense scene plus aim/lock | 80 | 179473 | 209802 (frame28) |
| Aim and throws in upper terrain | 160 | 128378 | 179476 (frame69) |
| Six simultaneous shots | 80 | 138929 | 204054 (frame7) |
| Full native route | 1300 | 134318 | 188994 (frame964) |

Budget remains210000 per update/render, excluding hardware grayscale.
Generated hot-path assembly has no32-bit multiply/divide or float helpers.
The three rebuilt files are `yjprobe.89z`, `yjterr.89y` (YTR4) and `yjart.89y`
(YAR4). The GIF and captures are PC headless; no TiEmu input was sent.
