# First Yoshi's Island experiment

Read `games/yoshi/README.md`, `ROADMAP.md` and `RE_NOTES.md` before resuming.
The experiment uses the local European 2 MiB ROM, not the US 1.0 image used
by the community disassembly. The user requested the first five 256-pixel
widths of 1-1 for this experiment; that scope is not a default for other ports.
Half scale is provisional. The opening tutorial is separate. The verified
terrain endpoint is player X1264/Y1882, whole body inside the 1280-pixel band.

`make -C tools/snes core check` validates the pinned headless core and ROM.
`make -C games/yoshi measure` regenerates a cold-boot door and nine timelines.
The door discloses a tutorial-only X1000 poke, then uses the original loader
to settle 1-1 at X119/Y1904. Ordinary-jump traces drive both PC tests and real
TI binary state checks (`test`, `xcheck`, `tihash`). `make movement` records
twenty 240-frame flat-ground/air cases, explicitly anchoring only X=119 before
each original frame. The portable C movement probe matches these mechanics;
default gameplay now traverses the real terrain. Tongue, Shy Guy capture and
egg inventory are the first interaction milestone. ROM graphics were then
advanced at the user's request. Damage, crying bubble, rescue and the
ten-second countdown now work; aiming/throwing follows.

`make -C games/yoshi survey` records five original view samples through X1281,
with WRAM/SRAM/PPU captures and an explicit sprite-state suppression protocol.
This isolates streaming terrain and is not a winning interaction replay;
restore actors/platforms when verifying playable collision rules.

`make terrain-reference` confirms 27 stationary drops plus one-way ascent/
landing and ceiling impact: 404 original states match PC/TI. `make terrain-check`
compares a 600-frame native traversal and profiles each actual TI frame.
It finishes on update590; the earlier diagnostic build peaked at193552 cycles.
Current graphics need `yjprobe.89z`, the rebuilt `yjterr.89y`, and `yjart.89y`;
all extracted data remain ignored.

Important verified discoveries on this local revision:

- Normal gameplay mode at `7E0118` is `13`, not the US map's `0F`.
- Player state is cartridge RAM: X `70008C`, Y `700090`, VX `7000B4`, VY
  `7000AA`; fractions `70008A/8E`, jump state `7000C0`, flutter `7000D2`.
  WRAM-only traces miss these. The core exports 32 KiB cartridge workspace.
- Effective 8.8 integration keeps the whole signed intermediate in the
  fraction word; only its low byte is used next frame. Landing fraction is
  `FFFF`. The C ordinary-jump model matches 180 original frames exactly.
- Natural stationary spawn is a safe jump probe. X160 is already on a slope;
  teleporting X288 with the old camera triggers automatic retreat, so such
  pokes do not produce controlled flat movement measurements.
- X integrates the previous VX; Y integrates the newly updated VY. Flutter
  begins with old VY>352, reduces VY by `16 + max(vy-272,0)/8`, and ends at
  old VY<=-384 or release. Ordinary flutter's direction target is 480 rather
  than 960; both limits oscillate instead of hard clamping. Phase/timers and
  rearming are tested in C. See the game notes for the exact ordering.
- Keep the diagnostic's 51.2 Hz clock and measured probe cost distinct from
  final PAL timing and game costs; slope speed/impulse and other actors remain pending.
- Full collision Map16 lives in WRAM at `7F8000`: screen IDs come from SRAM
  `700CAA + ((y>>8)<<4) + (x>>8)`, masked63; each screen is256 LE words,
  row-major. The visible cache at `70409E` is a separate scrolling buffer.
- PAL page properties at ROM offset `53B12` give solid/one-way/slope flags.
  Ground slope samples at `53D0E + shape*128 + local_x*8 + 2` are angle and
  signed height bytes; slope ground adds1 to height. Flat ground is tile top.
  Player Y is its top: feet are (X+3/+8/+13,Y+32). Highest valid foot surface
  determines the stationary landing. These decoded heights match all27 drops.
- Ceiling contact pushes Y down1 per step, stops negative VY and starts head
  timer8/cooldown20. Copying a guessed immediate snap gives incorrect traces.

`make actors-reference` records 14 PAL interaction/flat-actor timelines plus
an explicitly instrumented actor census. `make actors-check` compares all2090
interaction states on PC/TI against the original and the90 Shy Guy motion
states. Scenarios50/51 isolate horizontal/vertical tongue (51 faces left);
52 parks X464/Y1888 before a genuinely loaded Shy Guy at X496/Y1904;53 is the
dense scrolling door. Native800-frame replay finishes update692 with two eggs.
Historical diagnostic route peak198162/dense peak202432 cycles, hardware grayscale
excluded. The earlier `YTR2`
bakes the static diagnostic terrain into byte planes; the56448-byte bank is
copied through Alundra's pure-C window method to avoid cold TileMap spikes.
The historical 2970 visible screens equaled the retained TileMap/rectangle oracle. Rebuild/send
`yjterr.89y` along with the binary; the obsolete `YTR1` bank is rejected.
Five Shy Guys are restored in the native band. Damage and Baby Mario now
work as described below; thrown eggs and other actor types remain pending.

`make art` extracts ROM Map16 scenery and controlled OAM poses. 876 cells
match the streamed original PPU. The preceding `YTR3` held the 56448-byte ROM
scene; `YAR2` holds 58 directional sprite frames in8586 bytes. Both banks are
read in place. Sprites use precomputed white outlines, cropped 8/16-pixel
blocks and the existing ExtGraph blits. Background parallax is flattened;
BG3 decoration is omitted; coins are static artwork until collection exists.
This historical graphics build carried Baby Mario and deferred damage at
the user's request. All2970 visible frames matched an independent pixel
renderer; dense80 PC/TI screenshots agree at every offset. Current native
mean137056/peak204252, dense peak208934 cycles, hardware grayscale excluded.
Send all three files and reject older `YTR1`/`YTR2` terrain data.

The next interaction milestone covers aiming/throwing. Keep the
static terrain checks and measured flat mechanics; the current ground-following
controller and camera are our own, not exact original slope physics or scrolling.
Preserve Baby Mario and activation conditions when adding damage and recovery.

`make damage-reference` records the natural first-Shy-Guy hit and body/tongue
rescue, with disclosed one-time pose/tutorial injections. Confirmed PAL:
hit player VX−640/VY−1162 and invulnerability160; baby releases next frame at
VX−435/VY−1162. Baby slot0 position `7010E2/701182`, fractions `7010E1/701181`,
speeds `701220/701222`, phase `7019D6`; riding bit15 `7001B2`, invulnerability
`7001D6`. Counter `7E03B6` starts109 tenths and drains every4 reference frames,
reaches zero at436 (8.72s). Recharge fraction `7E0394` advances a tenth per12
updates. Both rescues pause the counter and return the rider after32 updates.
The original full player recoil/invulnerability timeline is not exact in C.

User request: crying baby in bubble and ten seconds. Pending the recharge
grilling answer, the stated assumption is an initial rechargeable ten-second
reserve, repeat hits preserving it, body/tongue rescue and ENTER after defeat.
The target uses2560 real clock ticks, not the PAL109-tenths duration. Later
bubble float/reflection, stomp, return homing and failure panel are native
adaptations; crying is visual, no audio/minion cutscene.

Current banks: `YTR4`61504 bytes (233 real scene tiles plus tightly packed
image planes) and `YAR2`17700 bytes (122 directional frames, including Yoshi
alone and baby/bubble). Counter is one baked16px panel; tears one32px masked
sprite. Retain the C window: TileMap cold-cache spikes exceeded the frame
budget. The optimized C loop dispatches once per plane and reads aligned
overlapping long pairs, with no new assembly. Rebuild all three TI files.

`make damage-check`:24 original launch states match PC/TI/ROM,2098 complete
native states match PC/TI,108 LCD checksums agree. Route ends at1192, two eggs,
two hits/rescues. Per-frame peaks:166658 route,202398 dense five actors/six eggs/
bubble/HUD,160640 expiry; budget210000 unchanged, hardware grayscale excluded.
Earlier mechanics still pass4800 movement,404 terrain and2090 interaction
states. `RT_CYCLES` now has the PC virtual tick clock; the hardware clock is
unchanged. Scenarios54..58 cover hit, bubble, rescue, expiry and density;
59 isolates the earlier terrain route. `damage-preview` produces the ten-second
countdown GIF; `preview` produces the entire damage-enabled route.

## Coins and completed Yoshi interactions

Milestone4c implements the user's explicit combined request:21 animated
collectable Map16 coins, resting poses, visible captured actors on the tongue,
world egg following, aimed throws, stomp kills and holding/turning/spitting
instead of converting. Pending the aiming answer, Diamond aims then throws,
Shift captures/spits, Down converts or cancels aim. These controls remain a
stated assumption. Egg flight/rebounds/path following are native adaptations;
PAL observations and inputs are documented in the project's `RE_NOTES.md`.

Current banks supersede the figures above: YTR4 is60544 bytes with218 retained
tiles, YAR3 is39476 bytes with174 directional frames and offline masked
small-sprite shifts. The heap frame cache keeps the plain program below the
TI-89 AMS24KiB limit. All three files must be rebuilt/sent together.
`make items-check` verifies2580 complete PC/TI states and325 LCD checksums.
Per-frame peaks are209572 for five actors/six followers/baby/coins/HUD,
203716 for six simultaneous shots,179138 for aiming/throws and188656 for the
native route. Units and original4800 movement/404 terrain/2090 action fixtures
pass. New doors60..66 and `items-preview` expose the mechanics without UI.
The original5-screen choice remains this project's scope, not a skill default.

## Readable aim and tongue follow-up

The user confirmed original sweeping aim, L/R lock/resume and egg rebounds.
The TI mapping is Diamond ready/throw and Alpha toggle lock/resume. Lock
preserves the angle and world direction until unlocked; cancellation/hit/throw
clears it. Aim now uses an outlined15px cursor drawn after actors/baby, with
a black center while locked. Tongue ink is2px with a1px white outline.

`make aim-reference` records three300-frame local-PAL probes. L80 freezes
7000EE at2944 and sets7000EA=FFFF; R96 clears the flag and resumes the sweep.
A downward shot first rebounds at81, ID25→24, VY1451→−1450, then VY increases
by1/update. Only observed contacts are claimed; the native three-contact cap,
post-contact trajectory and color progression are not exact PAL translations.

Current banks supersede prior art sizes: YTR4 remains60544 bytes; YAR4 is
45620 bytes, adding two offline-shifted cursor poses. Send all three rebuilt
files. The plain program is22937 bytes, below24576. No new assembly.
`items-check` now has3225 explicit133-word PC/TI states and1031 equal LCD
checksums, including all updates of aim sweep/lock, visible capture, ground
jump/stomp, distant enemy egg hit and one-egg floor rebound. Final peaks:
dense209910, aim-dense209802, six shots204054, upper throws179476,
route188994 cycles;210000 budget unchanged, hardware grayscale excluded.
The refreshed headless eight-clip GIF captures every update at50fps without
chapter holds (910 updates,18.2 seconds); the user rejected the preceding
slow preview. Replay68 stomps at34 without baby loss;69 launches at55 and hits64.
Doors67..70 expose these actions. Hardware verification remains milestone6;
no TiEmu keyboard/restart was used.
