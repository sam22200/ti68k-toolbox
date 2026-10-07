# Yoshi's Island: coins, tongue and eggs

Playable half-scale traversal of the first five original256-pixel view widths
of level1-1, as requested for this experiment. Start X119/Y1904, endpoint
X1264/Y1882. Original collision terrain, five Shy Guys, tongue/capture/swallow/
spit, 21 animated collectable coins, idle animation, six following eggs and
aimed egg throws work. ROM scenery and animated actors use four greys
and white outlines. The reusable SNES skill does not prescribe this scope.
See [ROADMAP.md](ROADMAP.md) and [RE_NOTES.md](RE_NOTES.md).

Enemy side contact now knocks Yoshi back, cancels his tongue and detaches Baby
Mario on the next update. Yoshi blinks during invulnerability. The baby cries
in an animated outlined bubble; retrieve him by touching him or with the
horizontal/upward tongue. The return takes32 updates and pauses the countdown.
A stomp defeats a Shy Guy without losing the baby.

The reserve starts at **ten real seconds**. Rescue preserves the remainder,
then gradually recharges it to ten. Another hit while the baby is detached
preserves the remaining time. At zero, gameplay stops; ENTER retries.
Crying is visual, without sound. The explicit request establishes damage,
bubble and ten seconds. Recharge rather than a fresh ten seconds on each hit
is the stated working assumption from the unanswered grilling question;
controls and exact PAL movement scheduling also remain provisional.

## Build and play

```sh
make -C tools/snes core check
make -C games/yoshi measure movement terrain-reference actors-reference damage-reference
make -C games/yoshi art test pc ti
make -C games/yoshi state-check damage-check
make -C games/yoshi items-reference aim-reference items-check items-preview
make -C games/yoshi preview damage-preview
```

PC: run `./yjprobe_pc` from this directory with `yjterr.bin` and `yjart.bin`.
Titanium: send **all three rebuilt files** `yjprobe.89z`, `yjterr.89y` and
`yjart.89y`, then run `yjprobe()`. The formats are `YTR4` and `YAR4`; older
banks are rejected. ROMs, third-party code, extracted banks, reference captures,
states and generated fixtures remain ignored and local.

Arrows move; **2nd** jumps/flutters; **Shift** uses the tongue, with **Up** for
vertical extension. **Down** swallows; Shift with an enemy in the mouth spits.
Keep an enemy in the mouth, turn if desired, then Shift to spit or Down to
make an egg. Captured enemies ride the retracting tongue into the mouth.
**Diamond** starts an oscillating reticle; press Diamond again to throw.
**Alpha** freezes its angle and direction; press Alpha again to resume the
sweep, corresponding to original L/R. The outlined15px cursor is drawn last.
Down or Shift cancels aim without spending an egg. Shots defeat Shy Guys,
collect coins and reflect from terrain, disappearing at their third bounce.
The egg train follows Yoshi's spatial path, including jumps, and keeps its
spacing while he stands still. Resting eggs bob; Yoshi looks around and blinks.
ENTER restarts and ESC quits. Baby rescue uses these same controls.

`yjprobe(54)` starts beside an enemy to test damage. `yjprobe(55)` starts with
the baby already floating and ten seconds to retrieve him. These are the PC
`--scenario 54`/`--scenario 55` doors too (separate CLI option and number).
The headless previews are [damage-preview.gif](x/damage-preview.gif) (crying,
10-to-zero, defeat) and [art-preview.gif](x/art-preview.gif) (whole route,
two losses and two rescues). No TiEmu input is used to make them.
The new [items-preview.gif](x/items-preview.gif) shows coins, idle/followers,
visible tongue capture, spit/conversion, a complete ground jump/stomp,
lock/unlock, an egg flying into a live enemy and a floor rebound. Brief actions
show every update at50fps, with no artificial pauses. The refreshed
[smooth GIF](x/items-preview-smooth.gif) avoids cached copies of the earlier
slow preview. `yjprobe(60)` tests a coin,
`yjprobe(61)` starts with six eggs, `yjprobe(62)` starts at a capturable Shy Guy,
and `yjprobe(63)` demonstrates a stomp. PC uses `--scenario N`.

## Measured behavior and adaptations

`damage-reference` naturally collides with the real first Shy Guy after one
player-pose injection and a disclosed tutorial flag. It records700 contact
frames and two150-frame rescue traces, each with one airborne player-pose
injection. No repeated anchoring/suppression occurs during these timelines.
The local PAL hit has VX−640/VY−1162 and160 invulnerability updates. Baby
starts with VX−435/VY−1162 one update later. Twenty-four complete launch states
match original position, fractions, speed and phase on both PC and TI.
Touch and tongue both start a return; riding resumes after32 updates. PAL
recharges a counter tenth every12 updates.

The original PAL109-tenths reserve reaches zero after436 reference frames,
about8.72 seconds. Our **2560 clock-tick reserve** follows the user's ten-second
request and the runtime's256Hz clock. Recharge adds26 ticks every12 updates.
The launch/deceleration phases reuse observed arithmetic; later floating,
LCD-edge reflection, contact hitboxes, stomp bounce, return homing and failure
panel are a native model. The original capture-by-minions cutscene is omitted.
No complete original damage-physics or wall-clock fidelity claim is made.

ROM extraction verifies876 Map16 cells against streamed PPU views. The art
bank has174 directional frames, including four idle poses, eight ingestion
poses, four throwing poses and four coin phases, with matching solo Yoshi poses.
`YAR4` is45620 bytes, read in place; the bubble uses one32-pixel block and
tears one existing masked blit. The counter panel/digits are baked together.
`YTR4` is60544 bytes, including collision data,218 scene tiles retained for
study and two tightly packed640×256 image planes. The C window copier
selects its shift once per plane and copies ten overlapping aligned pairs
per row, avoiding repeated row calls and TileMap cache rebuild spikes.

Coins are removed before foreground/background composition, so collection
reveals the correct backdrop. Eight-pixel coin/egg sprites have all sixteen
sub-word shifts baked offline for masked C row writes on TI; clipped sprites
keep the ExtGraph path. The 5568-byte TI frame cache is allocated at startup
and freed through GCC4TI's standard `atexit` callback, keeping the plain
program below the TI-89 AMS 24 KiB limit.

Background parallax is flattened and BG3 decoration omitted. Actor patrol, ground following, activation, spit
flight, camera, pose selection and animation cadence are native adaptations.
Slope speed projection/impulses and other actor types remain pending.
Egg following uses a64-point spatial history; aim uses a64-direction integer
lookup table, an oscillating upper/front arc and8.8 straight flight. Rebounds,
projectile hitboxes, six-shot cap,180-update lifetime and idle pose timing are
native adaptations; original aiming/flight is studied, not translated exactly.
The user confirmed original sweeping aim, lock/unlock and rebounds. The TI
mapping is Diamond for ready/throw and Alpha for lock/unlock. The original
PAL first floor rebound and lock/resume are recorded by `aim-reference`;
exact multi-contact trajectories, post-bounce gravity and color progression
remain adaptations rather than a full flight-physics translation.
Movement updates still use51.2Hz versus
50.007Hz PAL. The countdown uses clock ticks and is independent of that rate.

## Checks and scenario doors

`make test` covers the4800 original movement states,404 terrain states and
2090 original action states, plus damage, exact ten seconds across clock wrap,
body/tongue rescue, recharge, repeated hits, stomp, defeat/reset, raw PC
snapshot reload without charging time since the save, and visible
pixels against an independent renderer. The native input replay in
`keys/actors.txt` finishes at update1192 with two eggs, two hits and two rescues;
all subsequent state stays frozen. The original terrain-only route now uses
scenario59, keeping its separate physics test reproducible.

`make state-check` compares4800 movement and404 collision states against the
original on compiled TI code. `make damage-check` compares24 original baby
states,2098 complete native PC/TI states and108 LCD checksums, covering all16
horizontal offsets of the dense scene. It profiles every frame of the
1300-update route,80-update dense door and540-update expiry door.
The provisional budget remains210000 cycles per update/render pair;
peaks are166658 on the route,202398 in the dense door and160640 at expiry.
See `x/damage_*_cycles.json` for full reports. Hardware grayscale is excluded.
The runtime's `RT_CYCLES` clock now advances like the PC clock; normal TI
interrupt timing is unchanged. Runtime unit tests and screen cross-check pass.
No new assembly was written. Hardware verification remains a later milestone.

`make items-check` compares every item field (including trail, followers,
projectiles, coin bitset and score pixels) via explicit133-word packets,
avoiding raw PC/TI structure padding. Current results are in `RE_NOTES.md`
and `x/items_*_cycles.json`. It checks state and LCD equality, then profiles
every update/render pair of the route, dense actors/followers/baby, throws and
six-shot stress scenes. Units also check distinct idle/coin images, duplicate
collection, restart, aim cancellation/empty/full pool, both throw directions,
lock edge versus hold, sweep resumption, direction-preserving locked throws,
projectile collection, stomp, hold/turn/spit without creating an egg, and
deliberate conversion. Earlier ROM fixtures pass.

Scenario0 is normal gameplay;1/2/3/10/11 isolate flat movement,12/13 one-way/
ceiling interactions,14/15 late-band terrain,20..46 stationary drops.50/51
isolate tongues,52 capture,53 dense actors. Damage is disabled in these earlier
probe doors.54 tests a natural side hit,55 detached baby,56 touch/return,
57 expiry,58 five actors+six eggs+baby+HUD with live damage,59 isolated
terrain traversal. ENTER always returns to normal gameplay.
Doors60..66 cover coins, idle/followers/aim, capture/spit, stomp, dense actors
plus six followers and bubble, upper terrain throws and six simultaneous shots.
Doors67..70 expose a longer visible capture, ground jump/stomp, distant egg
target and one-egg floor rebound. Scripts assert actual airborne ascent,
no baby loss, visible flight before impact and upward velocity after contact.
The expanded check has3225 complete PC/TI states and1031 equal LCD checksums.
