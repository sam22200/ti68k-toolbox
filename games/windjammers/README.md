# Windjammers: native Beach training prototype

M3c is playable on the Portable Game Runtime: H. Mita on the left against
a simple native B. Yoo practice partner, a court filling the entire 160x100 LCD,
ordinary straight/diagonal throws, swept rebounds, neutral contact decisions,
3/5-point goals and service returned to the loser. Ordinary captures now
include measured recoil/lockout, possession-dependent return strength and
automatic release after 64 original logic steps, with animated ROM actors in four greys and
white outlines. Centered numbered point bands, the outlined net and completely
white sand use native artwork, with no ground shadow. Disc and HUD
remain native artwork. Solo training is a
provisional choice, not a claim to reproduce the cartridge's CPU opponent.

Arrows move. **A/2nd** throws; hold Up or Down while pressing A to aim
diagonally. **B/Shift** throws a lob (Shift/X on PC), also with Up/Down aim.
**Enter** restarts, **Escape** quits. On PC, A is Space/Z/Ctrl.
Launch `./windjam_pc`; the calculator program is `windjam()`.

Without the disc, **direction + A/2nd** dashes, including diagonals. In TiEmu,
press an arrow together with **AltGr**. Release A after the dash; holding it
extends the original dash animation while movement keeps slowing down.

Without the disc, tap A while stationary just before its arrival to prepare
the reception. The measured short window lifts the disc; the player charges
automatically underneath, with a wider visible bar, then recatches it. An A cue
appears ahead of aligned incoming shots; release the arrows before tapping A
to prepare a lift rather than a dash. A filled star marks complete charge.
Release A, wait for recapture, then press A again for the special. Tap A at or
just after an ordinary catch for a powerful immediate return, also with
Up/Down aim. After charging and recatching, A releases Mita's wave or Yoo's
wall-first accelerating special; B releases the special lob, which accelerates
along the ground after a missed reception. A normal missed lob awards two
points. Receive and return a ground special on the same logic step to send its
kind back; the next step can already be too late near the rear boundary.
Buttons and movement during the measured lift/charge interval are ignored,
as in the original.

While holding the disc, quickly roll Up -> Up+Right -> Right+A for one curve,
or Down -> Down+Right -> Right+A for the other. On the right side, use Left
in those motions. The ROM accepts recent adjacent 45-degree directions;
each of the last two direction segments must last at most four logic steps
(about 68 ms). Low normalized history power needs a full three-direction
quarter-circle; high power can accept a shorter arc. Neutral is encoded like
Up in the original, so some short gestures also succeed at low power.
Hold both arrows for diagonals and press A/2nd with the final direction.
B keeps its lob behavior. A wall rebound ends the curvature.

The compact two-digit scores sit at the top center, with the countdown between
them in white on black. Each digit is six pixels wide and ten high, retaining
two-pixel strokes. The native clock starts at 30 seconds, advances at the
logic rate (including goal pauses) and stops at zero; this training prototype
continues playing afterward, pending original round-ending rules in M3.
The court reaches all four screen edges; there are no separate title/control
bands. Both eight-pixel side bands show black 3s on white outer zones and a
white 5 on the dark central zone. The band geometry is symmetric about Y=49.5,
the exact center of the 100-pixel LCD; all labels stay upright on both sides.
The net has a black outline, a pale two-pixel core and darker mesh joins,
with no ground shadow. This is a cosmetic display adaptation;
scoring still uses the measured original Y=120..167 five-point interval.
The floor is completely white, with no sand fragments or marks.
Preserve the numbered bands, outlined net without a shadow, plain white floor
and current score/countdown layout when adding later artwork; these are the
user's current visual choices.
Offline integer lookup tables fit original court coordinates to the LCD;
ROM actors retain their half-scale size and are kept within the screen at
the boundaries. This is a display adaptation: physics and collision bounds
keep their original units. The disc draws above the scoreboard to remain
visible during a top-edge rally.
The disc's black ring is eight pixels across, with a white outline. Airborne
height uses a quarter-scale projection to keep the disc near its ground path.
A fixed bracketed target marks the lob destination throughout flight; a small
grey moving shadow distinguishes the current ground position from that target.
These action cues do not add decorative marks to the plain floor.
Powerful ground shots (speed word at least256) leave fading grey afterimages.
Charged Mita/Yoo shots, special lobs and their ground rebounds use a stronger
energy afterimage with animated perpendicular sparks. Four recent projected
positions follow the actual flight, including curves, wall turns and height.
The trail draws behind players, landing markers and the score overlay, then
disappears on a catch or goal; ordinary slow shots keep the white floor clear.

## Build and run

From this directory:

```sh
make test
make pc
./windjam_pc
./windjam_pc --headless --scenario 4 --frames 160 --shot x/prototype.png
make ti                 # windjam.89z + wjart.89y, Titanium program and art data
../../tools/bin/ti-run windjam.89z wjart.89y
make validate           # native PC/TI state hashes and screen checks
make art-check          # decoded ROM actor pixels against original RGB scenes
make court-check        # decoded ROM panels/net/shadow against original RGB
make profile            # datasheet cycles, every frame in twenty-four timelines
make advanced-reference # repeat the 62 no-write lob/special/charge trials
make curve-reference    # repeat 180 no-write gesture/curve/return trials
make dash-reference     # repeat 48 no-write dash trials
make showcase           # x/Windjammers-TI89-showcase.gif, actual LCD with named captions
```

Local prerequisites: the project's GCC4TI/runtime, the prepared Windjammers
cartridge and supplied BIOS, and the pinned FBNeo core. Missing original
reports trigger `make -C ../../tools/neogeo gameplay` / `rules`; see
`../../tools/neogeo/README.md` to prepare/build the instrument first.
`tools/prepare.py` binds program identity and reference settings, generates
local angle data/fixtures and measures/replays Mita's throws.
`tools/measure_actions.py` records/replays complete ordinary actions;
`tools/measure_holds.py` measures delayed manual returns and automatic rallies;
`tools/measure_timing.py` records contact-timing sweeps, lift/charge/recapture
and complete capture-interrupting returns on both sides;
`tools/measure_advanced.py` repeats lobs, charged flights, counters and ignored
charge inputs. `tools/measure_curves.py` sweeps directional gestures and replays
curved flights, wall transitions, catches and manual returns.
`tools/advanced_tables.py` extracts original quantized vectors
and flight-distance buckets. `tools/pack_art.py` copies the validated actor
rows into `wjart.bin` (host words) and `wjart.be.bin` (TI words); `wjart.89y`
is required alongside the program, and can remain archived. Sprite words are
read in place through the existing runtime file API, without decompression.
`tools/extract_art.py` decodes C-ROM pairs and original pose descriptors.
`tools/layout.py` generates the native full-screen projection and score glyphs.
`tools/extract_court.py` validates captured VRAM C-ROM scenery against the
original RGB image as an independent reference, then generates native numbered
point bands and the outlined net on plain white sand, without using the ROM
shadow. Five opaque 32x100 strips contain 4000 logical plane
bytes; the compiler can share identical empty arrays.
All generated ROM data, state files and results stay ignored. A built native executable
does not run FBNeo or need the cartridge/BIOS at runtime.

`--save x/game.state` / `--load x/game.state`, F2/F3 and key scripts are the
runtime's PC facilities. The saved native state includes scheduler phase,
draw parity, clock fraction and previous logic pads; state files are
same-platform ABI files. Direction history and dynamic recoil are saved and
hashed explicitly; recreate native saves made before M3c or the flight-trail
update. The trail history also lives in saved state and explicit field hashes;
drawing the same frame twice never advances it.
`keys/play.txt` is a deterministic control sequence, not a guaranteed win.

## Measured behavior and explicit adaptations

Positions/velocities retain the original signed 16.16 units. Walking speeds,
fraction-preserving clamps, neutral rectangles/arcs and court/point bounds
are recorded in the [original notes](../../.claude/skills/ti-port-neogeo/reference/windjammers.md).

New no-write Mita trials begin after the original straight Yoo shot has been
caught and settled (80 frames after the standard serve door). Mita stands at
X=28.5625, Y=138. A/Up+A/Down+A on local frames4/5 release on 16/16/12;
his throw action ends on32. Speeds are `00049800` straight and `00033F78`
per diagonal component, versus Yoo's `00057000 / 0003D830`. Six shots use
the observed formation offsets (20 pixels straight, 18.828125 horizontally
and 2.828125 vertically for diagonal shots). Exact original frames, including
fractions and wall contacts, are checked through the complete first capture
and settled hold, with both players' positions, velocities and actions.

For ordinary Yoo shots (speed word174), Mita receives the incoming velocity
and decays its speed word by halves every two steps: 174,87,43,21,10,5,2,1.
Each component is reconstructed from the original quantized sine/cosine;
halving the velocity directly gives incorrect fractions. All return/recoil
profiles are generated offline. On the left boundary, the holder is nudged
to integer X=31 while preserving the integrated fraction. The six fixtures
also cover diagonal recoil and its Y clamp. Mita's final recoil step changes
to action1004; Yoo's lower-speed incoming Mita shots cause a 12-step stationary
action1000, followed by hold on step13.

M2b1 compares 29 normal-input trials: both characters return straight/up/down
at several possession ages, then catch and settle; two no-input controls run
600 original steps each through repeated automatic rallies. Manual A uses the
current age before incrementing it; idle possession increments and starts a
throw at 64 (about 1.08 seconds, release follows its normal animation). Throw
strength also depends on a persistent average of previous return ages and a
diminishing opening bonus. The observed character speed tables, collision
substeps and recoil are generated offline as 102 profiles. Slow incoming shots
now correctly use stationary captures even when thrown by Yoo.

The M2b1 comparison checks 4058 original steps: actor position/velocity/action,
history/bonus and possession age during throw/catch/hold, plus disc position,
state and flight velocity. Idle anticipation uses of the original age field,
other nonholder actions and special flight remain outside that M2b1 claim.
M2b2 adds early capture input, high-speed captures, lift and charge below.
Native score-deficit normalization
uses training points; the original comparison trials have equal/zero scores.

M2b2 adds 78 no-write reference trials (10920 frames, each replayed twice),
with 6704 native equality steps and eight separately checked block outcomes.
Each incoming door reaches a normal capture after 18 logic steps. Mita's
stationary A presses at delays14..16 lift on18; Yoo's14..16 lift on17, and17
lifts on18 because his ready box is wider. Earlier/later active preparation
blocks instead; an expired preparation falls back to an ordinary catch.
These are specific measured door timings, not a universal tolerance.

Lift state14 preserves incoming fractions at the player's integer position,
starts vertical velocity `0x34000` and subtracts `0x1500` per logic step.
On the next step the player enters charge action4; Mita becomes fully charged
on charge step45, Yoo on42. Descending contact is sampled before the following
recapture step. Equality includes height, vertical velocity, charge count and
fully charged flag through recapture. Tap A during the ordinary capture to
interrupt recoil and start the return before incrementing hold age; selected
immediate returns have speed288 or256. A release while possessing also starts
a throw, matching the original edge injection. Interrupted catches keep the
12-step release animation, including downward throws; settled downward throws
release after eight steps. Subsequent full-speed captures and stationary holds
are compared, with the extended original recoil table.

The independent neutral angle model reproduces `0191CE`'s quadrant selection,
small-vector scaling and quantization using the local 64x64 angle table.
It predicts the angle/classification in all 168 running-ROM contact probes;
it does not consume the original's collision-angle field as an input.
Full action-pose selection is still separate work.

The source-rate scheduler adds 23672 or 26631 for an 8/9 tick frame and
subtracts 12800 per logic step: `2959/50 = 59.18` steps/second at a 256 Hz clock.
The display runs at 256/8.5 Hz. Inputs are sampled per draw and their edges
are consumed once by logic, including draws with three source steps.
Ordinary physics uses precomputed speed/substep profiles. Advanced flight
uses original quantized trig/distance tables and isolated signed/unsigned
16x16 products; generated 68000 code uses MULS.W/MULU.W, with no division,
32-bit products, floats or arithmetic-library helpers. Wall substep divisions
use a small reciprocal table and one word-product correction.

M3a adds 8573 native comparison steps from 62 normal-input trials replayed
twice (18600 reference frames per pass). Lobs include formation, possession
aim, height/velocity, descending recapture or a missed landing. Ground specials
include the full lift/charge/recatch/release chain, both wave directions,
Yoo's wall and acceleration, strong reception and accepted/rejected counter
edges. Special lobs include ascending/descending flight and missed landing
followed by ground acceleration. Ten controls confirm ignored charge inputs.
Normal lob targets condition native jitter on the recorded target; global
Neo Geo RNG equality is not claimed. Fixture intervals stop before native
goal-entry/celebration differences. Special-lob100C/1010 poses are compared as
one reaction family, with exact recoil; idle reuse of the hold counter is
excluded, as in earlier milestones.

M3b adds 17764 native comparison steps from 180 twice-replayed no-write trials
(43200 original frames per pass). Both arc directions and eight final
directions run from the two possession doors. Segment durations 1..5 test
the four-step acceptance boundary; low/high normalized power, incomplete or
neutral-interrupted motions and B fallback are covered. Exact comparisons
include actor action/position/velocity and possession/history fields, disc
position/state/velocity, 32-bit angle, speed and angular increment. Flights
continue through wall conversion and stationary capture/recoil. Twelve
controls add immediate or settled ordinary returns after curved reception.
Eight native saves resume either within the gesture or during flight and
compare states and planes through missed goals and loser service.

TARGET adaptations for this engine milestone:

- Rear hits reflect horizontally with a short collision grace interval;
  original deflection flight and moving/other action-pose contacts await M2b.
- Failed-preparation blocks have a native airborne trajectory; the original
  RNG-selected destination is not reproduced. Reference tests compare the
  entire pre-contact prefix and block classification separately.
- Lift/charge use an existing ROM hold pose and a native preparation marker/
  charge bar. Ground and raised disc cues remain readable on the white court.
  Advanced throw art reuses ordinary ROM poses pending the remaining action-art
  slice; actual lob/character-special trajectories are implemented.
- ROM pose sequences use a native cosmetic clock. Idle/walk/hold clocks reset
  on action/direction changes; throw/catch pose selection uses action age.
  This is not an exact original animation-state or turning model.
- The 90-step goal pause returns service directly, retaining scores; it omits
  ballboy delivery and the original pause timeline.
- Training scores run until reset, capped at 99. Original round/match/time-out,
  and sound are not implemented.
- Dash movement is measured; horizontal wall recovery and moving contact
  poses remain native adaptations. Existing walking art and short grey motion
  streaks represent the dash, pending extraction of its original poses.
- Native following/throw selection is practice AI, not measured original AI.
- The court model has simple top/bottom walls and open sides; original corner
  cells and raised-disc collisions await their own implementation/checks.

See [ROADMAP.md](ROADMAP.md) and [RE_NOTES.md](RE_NOTES.md) for M2b/M3.
Precisely timed immediate returns and lift/charge/recapture are completed in
M2b2. M3a completes the bounded lob/special/rebound slice. M3b adds the required
directional arcs/curved throws. The next match slice is round/time-out rules;
opponent behavior and remaining action-pose work remain on the roadmap.

M3c adds dash movement (48 no-write trials replayed twice, 816 position/velocity
steps), a larger disc, distinct fixed lob targets/moving shadows, and clearer
preparation/charge cues. The hint-to-lift-to-charge-to-special player sequence
passes for both characters, along with interrupted dash captures and replay.

## Validation

`make test` passes headlessly:

- 1472 original walking frames: both characters, all 8 directions,
  release/reversal and fractional court clamps.
- 437 original launch/flight frames and 660 complete action frames across six
  throws, including first capture, recoil/lockout and settled hold.
- 4058 possession/return steps across 29 trials, including two 600-step automatic
  rallies, delayed launches in all three directions and subsequent captures.
- 6704 timed-action steps across 78 trials, with complete immediate returns,
  lift/charge/descent/recapture and eight separate failed-preparation outcomes.
- 17764 curved-action steps across 180 twice-replayed trials: gestures,
  complete first flights/walls, captures and immediate/settled returns.
- 816 dash motion steps across 48 twice-replayed no-write trials: both
  characters, eight directions, three button durations and fractional clamps.
- The visible preparation hint leads through lift, full charge, recapture and
  a character special on both sides; native dash replay/recovery/capture passes.
- Powerful/special trails follow actual previous positions, clear on capture
  and goals, and replay identical state/planes after saving. Slow curves have
  no false trail; repeated rendering leaves state and pixels identical.
- 168 original contact words including angles, 14 fractional ordinary-wall
  cases and 16 goal probes comparing award frame, exact position and recipient.
- Native goals on both sides, persistent points/loser service, input/reset,
  exact scheduler cadence and PC save/load continuation.
- The 30-second native clock, ticking through goals, stopping at zero,
  reset and two-digit score saturation.
- Native ready/charge saves replay state and screen checksums on both sides;
  timed input edges span multiple source steps per draw without retriggering.
- 30,000 native practice steps: 396 catches, valid states and court bounds.
  The two bots defend indefinitely in this trial; goal handling is exercised
  separately with controlled missed shots.

The additional 480 original Mita frames and 1440 complete action/automatic-hold
reference frames replay with every RAM/video/status
fingerprint identical. `generated/provenance.json` and
`generated/mita_reference.json` / `actions.json` retain settings, hashes and
original rows. `generated/holds.json` adds 7140 replay-checked original frames
with source fields, program/script hashes and all 29 possession trials.
`generated/timing_capture.json` / `timing.json` retain the complete 10920
replayed timing frames, input schedules, all sampled fields and each trial's
native equality interval; failed block flights are explicitly excluded.

`make art-check` compares 1,329,378 exact RGB actor pixels across 1256 scenes
(six action trials and sixteen walks), covering 135 original pose/flip pairs.
The original entity RAM describes the next scene; the following frame's
VRAM/video presents the earlier descriptors. Validation pairs those samples
explicitly and derives occlusion from later opaque VRAM sprite banks before
comparing colors. Both selected character palette overrides are measured:
Mita2A, Yoo27. All actors use decoded ROM tiles at half scale with a dilated
white outline; the larger native disc retains its readability.

`make court-check` verifies 41,344 original RGB scene pixels in the first-serve
snapshot. Independent sprite-depth ownership selects 3464 goal-panel pixels,
360 net-pole pixels and 228 retained shadow pixels for exact RGB comparison.
These comparisons validate original reference art; the displayed court is
deliberately native and has no ground shadow. The reference shadow is baked
into palette40's sand in the central tiles, not a separate
sprite. Goal banks262..267, pole bank228 and base banks192..211 are decoded
from the captured VRAM; the native crop/grey conversion is a display adaptation.
`generated/court.json` retains original tile descriptors, input hashes and
comparison counts; `generated/court.png` is the native background review.

`make validate` passes 7380 PC/TI per-field hash samples (29 scenarios x 220,
plus 1000 training frames through clock zero) and thirty final screen checks.
Six additional active-flight screenshots check both powerful returns and
both characters' charged shots/special lobs while the trail is fully visible.
SDL file continuation also
matches exactly: 301 uninterrupted frames = 101 saved + 200 restored.
`x/validation.json` / `scenario*.ti.log` retain the result. The normal
Titanium program is 42,235 bytes; `wjart.89y` is 30,008 bytes and contains the
same 139 outlined ROM pose/flip variants. The static court still uses 4000
logical plane bytes, with identical arrays shared. An unpatched TI-89 build
still needs program packing; this build targets Titanium. The shared runtime's hardware paths were
not changed; this milestone was checked without launching TiEmu.

Cycle results from `x/cycles.json` are measured on the actual compiled TI
binary with the MC68000 datasheet counter, excluding the frame wait and
grayscale driver. The game budget is 360k/frame.

| Timeline | Frames | Average cycles | Peak cycles |
|---|---:|---:|---:|
| Human control script | 220 | 195341 | 210282 |
| Both practice bots | 220 | 195732 | 206040 |
| Goal/reset | 100 | 195950 | 207554 |
| Rear contact (adapted) | 60 | 193831 | 202806 |
| Mita capture/recoil | 40 | 193921 | 204762 |
| Yoo stationary capture | 40 | 192405 | 200094 |
| Yoo automatic rally | 320 | 192577 | 203548 |
| Mita automatic rally | 320 | 193010 | 204550 |
| Mita lift/charge/recapture | 220 | 200201 | 220478 |
| Yoo lift/charge/recapture | 220 | 199220 | 216254 |
| Mita immediate return | 220 | 193617 | 203848 |
| Yoo immediate return | 220 | 193491 | 211304 |
| Mita normal lob | 220 | 195788 | 221492 |
| Yoo normal lob | 220 | 201984 | 231442 |
| Mita charged wave | 220 | 197256 | 217702 |
| Yoo wall special | 220 | 197365 | 214578 |
| Mita special lob/catch | 220 | 197766 | 224642 |
| Yoo special lob/catch | 220 | 198705 | 226948 |
| Mita missed special lob/bounce | 220 | 198414 | 224070 |
| Yoo missed special lob/bounce | 220 | 198188 | 226928 |
| Mita clockwise curve | 220 | 194538 | 215184 |
| Mita counterclockwise curve | 220 | 193979 | 205242 |
| Yoo mirrored Up arc | 220 | 193339 | 213366 |
| Yoo mirrored Down arc | 220 | 193372 | 212672 |
| Mita dash | 100 | 194227 | 211806 |
| Yoo dash | 100 | 195627 | 211752 |

The 5040 measured frames have a largest draw/update total of 231442 cycles,
about 64.3% of the 360k game budget. `make codegen` checks the compiler output automatically;
its assembly/report stay in ignored `x/`.

## Scenario doors

| Scenario | State |
|---:|---|
| 0 | Mita serves; human left, practice partner right |
| 1 | Original reference serve geometry; two idle test-controlled pads |
| 2 | Ordinary diagonal Yoo flight before a rebound |
| 3 | Missed straight shot close to the left goal |
| 4 | Both native practice bots, continuous rally |
| 5 | Rear overlap at Mita; adapted deflection |
| 6 | P2 has 5 points, pause before loser Mita's service |
| 7 | Ordinary Yoo shot reaches Mita; recoil and holder boundary nudge |
| 8 | Ordinary Mita shot reaches Yoo; stationary catch and hold |
| 9 | Yoo reference possession door; no-input automatic rally |
| 10 | Mita post-catch possession/history door; no-input automatic rally |
| 11 | Original incoming Mita door; `keys/timing_lift.txt` prepares a lift |
| 12 | Original incoming Yoo door; human pad controls right; same lift script |
| 13 | Original incoming Mita door; `keys/timing_return.txt` interrupts capture |
| 14 | Original incoming Yoo door; human pad controls right; same return script |
| 15/16 | Mita/Yoo held-disc lob; `keys/lob.txt` |
| 17/18 | Mita/Yoo lift, charge and ground special; `keys/special.txt` |
| 19/20 | Mita/Yoo lift, charge and special lob; `keys/superlob.txt` |
| 21/22 | Same special lob, defender displaced for landing/bounce/goal/service |
| 23/24 | Mita clockwise/counterclockwise curves; matching `keys/curveN.txt` |
| 25/26 | Yoo mirrored curves; human right, matching `keys/curveN.txt` |
| 27/28 | Mita/Yoo without the disc; `keys/dash.txt` / `keys/dash_yoo.txt` |

On PC: `--scenario N`. On TI: `windjam(N)`. The C test API `wj_logic(p1,p2)`
controls both pads independently at original rate; `game_update` provides
the display-rate scheduler and human-left/practice-right controls, except
right-side timing scenarios12/14, even advanced scenarios16..22, and
curved scenarios25/26 and dash scenario28. Timing scripts sample once per draw; the
reference measurement tool probes individual original-rate logic steps.

`make showcase` exports actual runtime planes, never synthesized gameplay.
The 640x560 GIF shows the full 160x100 LCD at nearest-neighbor 4x, with
Windjammers TI-89 above it and mechanic/command captions below it. Its 1185
captured frames cover exchanges, both dash characters, immediate returns,
lobs, both charged ground specials, missed special-lob rebounds, both curve
directions, ordinary wall bounces and scoring/service. Original 8/9-tick
cadence is retained with short first-frame pauses for reading; the final loop
lasts 44.14 seconds. Captured modes/charge/input/checksums stay in ignored
`x/showcase/`, and the final GIF is `x/Windjammers-TI89-showcase.gif`.
