# Windjammers native evidence

OBSERVED facts and the reference instrument's identities are maintained in
`../../.claude/skills/ti-port-neogeo/reference/windjammers.md`. Measurements
come from the running local cartridge with the pinned FBNeo core; program
inspection explains them but does not replace execution checks.

## OBSERVED: M1 inputs

- Neutral walking/clamps and Yoo's three ordinary launches: ignored
  `sources/windjammers_neogeo/gameplay/report.json` and its CSV files.
- Swept wall rounding, neutral descriptor overlaps/arcs and goal boundaries:
  ignored `sources/windjammers_neogeo/rules/report.json` and its CSV files.
- Full original catch knockback and throw pose selection are outside the M1
  equality interval. The native prototype must not report those as exact.

## TARGET: native adaptation

- A whole playable Beach court filling the LCD, with a compact central HUD.
- Rational 2959/50 Hz logic sampled by 8/9 runtime ticks at 256 Hz; integer
  accumulator, with original input edges applied only once per logic step.
- A simple following/aiming training partner, not the original CPU opponent.
- Goal pause returns service directly to the loser, without original ballboy
  animation. Rear deflection and moving/action-pose contacts remain adapted.
- Scores accumulate until Enter resets; match/round/time-out rules await M3.
- Native training points are normalized binary counters, capped at 99;
  the original's BCD storage and separate arcade totals are not copied.
- ROM actor poses with native animation playback and offline white outlines;
  native centered numbered bands and outlined net without shadow on a plain
  white floor; disc and HUD remain training
  artwork. ROM banks stay local/ignored.

## OBSERVED: completed native M1 checks

`tools/prepare.py` independently predicts the original neutral collision
angle, rather than consuming it. Program `0191CE` selects quadrants, doubles
the center-coordinate differences, scales small vectors and quantizes them
into the 64x64 table at `01927E`. The native result matches all 168 original
contact words, including zero/non-overlap, catch and rear deflection.
Only neutral descriptor selection is included in the native model.

Three additional Mita ordinary throws start after the original Yoo straight
throw/catch has settled. All 480 recorded original frames replay identically.
Mita's straight/diagonal magnitudes are `49800 / 33F78`, release timing is
16/16/12 and throw lockout ends at 32 (local input on 4/5). The native engine
matches these trajectories through first catch at 62/88/84. Formation offsets
are 20 pixels straight and 18.828125/2.828125 diagonally, on both players.

The engine checks 1472 original walking frames, 437 six-shot launch/flight
frames, 168 contact words, 14 fractional ordinary-wall cases and 16 original
goal probes. Original equality excludes the explicitly adapted post-catch,
rear-flight, pause/serve and practice AI behavior. Source identities and
field intervals are retained by the fixture generator.

PC/TI: 1540 per-field hash samples, seven final screen checksums, all equal.
The SDL state-file continuation is identical across an odd-parity save.
30,000 practice steps give 394 catches with valid bounds/states; independent
missed-shot tests cover goals on both sides and persistent points/loser serve.
The measured peak frame is 168550 datasheet cycles. The physics assembly has
no multiply/divide/float instructions or arithmetic-library helpers.
Results and compiler output are local under `generated/` and `x/`.

## OBSERVED: M2a ordinary capture equality

`tools/measure_actions.py` prepares two normal-input doors, records both
players' positions/velocities/actions/poses/timers and disc fields, then
fully replays eight 180-step trials. Six manual throws feed native fixtures
through step109 (660 steps total). No RAM injection is used in these trials.
The two no-input automatic releases have different speeds and remain
measurements for M2b: Yoo 96 (VX=-196608), Mita 100 (VX=204800).

`01B6FA..01B730` copies disc speed/velocity into a defender when the speed
word is at least 160. For the six selected manual shots, Yoo 174 causes Mita
recoil; Mita 147 causes a stationary Yoo capture. `01CABE..01CB38` reconstructs
components before halving the speed word on alternate steps. Native tables
use the independently inspected original trig factors, not captured
velocities as a frame script: axial 2048, diagonal 1448, speed 174>>level.
All six action fixtures match both actors' X/Y/VX/VY/action and disc X/Y/state.

`01DA2C..01DB8C` uses character limits and the holding flag, not pose extents,
for court nudges. A holder at integer X<=26 is moved to X31 (five pixels
inward) on the left; the other boundary nudges four pixels inward. Fractions
are kept. This explains Mita's apparent alternation near X27/X31 during recoil.
The earlier working hypothesis of pose-dependent recoil bounds was rejected.

Mita's first 15 recoil steps have action1000, the sixteenth still integrates
the last velocity but changes to1004; the next step zeros velocity. Yoo's
first 12 stationary steps have1000 and the thirteenth becomes1004. Inputs
during capture and non-neutral defenders remain outside this equality claim.

## OBSERVED: M2a ROM actor decoding

`tools/extract_art.py` follows pinned FBNeo's paired C-ROM planar decoder
and the program's pose table32020 / descriptor renderer012DEE..013086.
Source012F54..012F9A replaces descriptor palettes with entity byte+3 when
nonzero: measured Mita 0x2A and Yoo 0x27. This override matters: Yoo's descriptors
use 0x26, but that is not the selected player's visible palette.

`tools/validate_art.py` checks six throws/captures and sixteen no-write walks.
It resolves VRAM strips, original flips/palettes and their opaque drawing
depth before comparing RGB; it excludes only later opaque sprite pixels.
All 1,329,378 visible actor pixels match in 1256 scenes, with 135 observed
pose/flip pairs and no excluded actor action samples. Body descriptors in
RAM precede the image presenting them by one reference step. Same-step RAM
against RGB is wrong at pose changes and must not be patched with offsets.

## TARGET: M2a native graphics and acceptance

Four-grey banks max-pool alpha at half scale, average opaque colors, and
dilate masks by one native pixel. Animated actors retain original anchors;
the court/HUD and deliberately enlarged readable disc remain native. The
cosmetic clock uses original sequence intervals; playback/reset is native,
and does not drive physics or claim full original turning behavior.

Nine native scenario doors pass 1980 PC/TI field hashes and final screens.
Both capture doors, actual SDL state files, input/reset, goals and the
30,000-step training run pass. Current compiled cycle costs and binary size
are in README; no hardware path changed and TiEmu was not launched.

## TARGET: full-LCD display revision

The user requested that the court fill the whole display, with compact two-digit
scores at the top and the timer between them. Original X=16..304 and Y=72..200 now project to the
160x100 LCD using offline integer tables. The half-scale ROM sprites remain
outlined; final draw positions are clamped within the LCD, without altering
actor or disc physics. The top-center score overlay uses six-pixel-wide,
ten-pixel-high native glyphs with two-pixel strokes; the timer is inverted
white-on-black between the scores. The disc renders above it. Title and
bottom help bands are removed. Numbered 3/5/3 bands are centered on each LCD
side; a native outlined net has no shadow. Scoring retains
the independently measured five-point interval [120,168).
The native clock starts at 30 seconds, adding 50 to a subsecond phase each
logic step and consuming a second at 2959. It ticks during goals and saturates
at zero; training continues afterward, pending original round endings in M3.
Its fraction and seconds are included in state files and PC/TI field hashes.
Training scores now saturate at 99, so the HUD always has two digits.
Five opaque background strips reset both planes without a separate clear.
Unit tests, nine PC/TI screen/replay doors plus 1000 frames through clock zero,
and per-frame profiling are rerun
for this revision; results and current program size are in README.

## OBSERVED: selected ROM Beach scenery

`tools/extract_court.py` decodes the original first-service VRAM strips with
the existing C-ROM decoder: base banks192..211, foreground goal banks262..267,
net pole bank228. Chained positions, tile animation bits, flips, palette and
opaque priority match 41,344 RGB pixels in the original scene (Y=40..175).
Before grey conversion, later opaque strip ownership independently selects
3464 goal pixels, 360 pole pixels and 228 selected shadow pixels for equality.
No mismatched pixels are used to decide visibility.

The mesh shadow is baked into the base tiles using palette40's sand shades,
not a separate sprite. The narrow original image strip X=144..153,Y=56..183
contains its motif; darkest indices1..4 are kept during native conversion.
These RGB comparisons validate original reference art. Native panel placement
is deliberately redesigned; the measured scoring interval is unchanged.

## TARGET: high-contrast ROM court adaptation

The user's latest request restores numbered point zones and removes the net's
shadow, retaining the completely white floor and current score/countdown.
Both eight-pixel bands use black 3s on white outer fields and a white 5 on
the dark center field[31,69), centered at LCD Y=49.5. Digits remain upright
on both sides; band geometry mirrors horizontally and vertically. The native
net occupies X=78..81 with a black outline, a pale two-pixel core and darker
mesh joins. No original ground-shadow pixels or sand marks are rendered.
This is a visual adaptation; the measured scoring and collision model stay
unchanged. Preserve these current choices when extending the game.
Five opaque 32x100 two-plane sprites cost 4000 bytes and use the existing
runtime blitter; there is no runtime decoding or new hardware path.

## OBSERVED: M2b1 possession and ordinary return strength

`tools/measure_holds.py` uses the normal-input M2a doors, warms two genuine
reference steps and samples fields/fingerprints before and after each trial.
There are no RAM writes. All 29 trials replay twice with 7140 exact full
RAM/VRAM/palette/video/status fingerprints. Manual A/Up+A/Down+A trials use
ages selected by delays 0/4/12/24 for both characters, plus delay40 for Yoo.
Two no-input controls each run 600 original steps through repeated captures
and automatic returns. Native equality covers 4058 selected steps: manual
release, complete flight and first settled catch, plus all automatic controls.

`01CC8C..01CCB8` increments possession age (player word+3A), saturating at64
and automatically starting a return there. Manual A is tested before that
increment. `01CCBA..01CCF8` normalizes a human return using word+52 and byte+4D:
`power=(power+hold)>>1`, `hold-=power>>3`, `hold+=bonus`,
`bonus=max(bonus-6,0)`. `01CCFC..01CD48` subtracts the losing player's BCD
round-score deficit; `01CD48..01CD5A` clamps the result to0..64.
The opening-serve Yoo door has history4/bonus8; post-catch Mita has34/2.
This history persists through captures, explaining why the next automatic
return is not always the same speed. E.g. Mita's normalized auto age60 gives
speed100; Yoo's next normalized age62 gives102; age64 gives96.

`028D0E..028D50` selects speed: normalized age>=64 gives96; otherwise the
character table at `028D50+(character<<3)+((age>>2)<<1)` supplies a word.
Characters are Mita20 and Yoo12. Axial components multiply that word by2048;
diagonals use1448. These products, quantized collision substeps, reflections
and alternate-step recoil tables are generated offline (102 profiles). The
six original manual indices remain compatible with existing diagnostic doors.
Captures now select stationary/recoil behavior from speed>=160, rather than
from which character threw. Selected measured incoming speeds remain below192.

Equality includes both actors' X/Y/VX/VY/action, history and bonus; possession
age is compared during throw/catch/hold, and disc X/Y/state plus flight VX/VY.
The original word+3A also changes before a nonholder catches the disc; those
idle/walk uses are outside the possession model. Neutral walking fixtures
therefore call the isolated `wj_walk` integrator, rather than launching an
unrelated automatic rally while sampling only one moving actor.

## TARGET: M2b1 bounds and training history

Native reset/service history is explicit: initial reference-door values for
practice, fresh history4/bonus8 after the adapted loser service. Native binary
training points replace original BCD round scores in the deficit calculation;
the zero/equal-score original trials do not validate that adaptation. Reset,
clamps, held-button edges and loser-service history have native tests.

Early input during capture, original charge/special paths (including the
below-minimum age speed288 branch), speed>=192 catch animation timing,
nonholder anticipation, moving/action-pose contacts and rear deflection remain
unvalidated M2b/M3 work. Unsupported charge/fast paths currently use ordinary
throw/catch playback in the training engine; the speed table alone is not a
claim to reproduce those original actions. Approved court/HUD art is unchanged.
Native save files now include power/bonus; recreate older same-platform saves.

## TARGET: user-required advanced actions

The user explicitly requires lobs, rebounds, precisely timed receptions that
lift the disc and allow charging a special, curved throws entered with
directional arcs, and powerful immediate catch/rethrows. These requirements
are durable port scope; see ROADMAP's M2b2/M3a/M3b acceptance criteria.

M2b1 validates ordinary settled possession and automatic releases only. Its
speed table/fallback does not implement immediate-return timing, lifting,
charge/specials or gesture-driven curves. Ordinary wall rebounds are checked;
additional rebound behavior in the requested action chains still needs ROM
measurements. Exact inputs, timing windows and airborne/curve rules remain
unmeasured in M2b1 and must not be presented as observed facts for that slice.

## OBSERVED: M2b2 stationary reception timing and immediate returns

`tools/measure_timing.py` creates each incoming door from genuine serve inputs,
18 steps before its ordinary capture. No game RAM is written. All 78 trials
replay twice with 10920 identical full fingerprints; 6704 native steps compare
actor positions/velocities/actions, word+3A, power/bonus, charge and charged
flag, disc position/state/flight velocity, height and vertical velocity.
`generated/timing.json` states each equality interval; eight original block
outcomes are tested separately after a compared pre-contact prefix.

Stationary nonholder A starts action0C00. `01C8CC..01C8E2` uses character
tables selected at01C908 to compose word+3A: the incremented low byte is the
ready age, the high byte is the timing-window flag. Mita's flag is set at
ages2/3/4, Yoo's at1/2/3. Preparation ends at step13. The contact was sampled
after the previous flight update and is consumed before player integration;
the existing ready flag determines lift14 versus failed block10.

In these doors, Mita presses14/15/16 lift on18;13 and17 block. Yoo presses
14/15/16 lift on17,17 lifts on18;13 and18 are outside that preparation window
(18 is an ordinary capture/instant throw). Yoo's ready descriptor has X
anchor+2 (mirrored to-2 on the right), half-width18 instead of neutral14,
half-height15, centerY0. Mita's ready and neutral boxes share half-width13,
half-height15 and centerY-4. The disc adds eight pixels to each half-size.
Do not turn these door-relative press times into a universal reaction window.

A during ordinary capture interrupts recoil before hold increment; at contact
the first return uses normalized age0 for Mita and2 for Yoo, both speed288.
The following steps normalize through speed256 and then slower buckets.
`01B272..01B2B6` injects button edges on A/B release only while player+20 bit5
indicates possession. Holding A through arrival does not repeatedly throw;
releasing it later while holding does. Native A edges follow that rule.
Capture interruption retains the reception-facing animation: all measured
immediate straight/diagonal returns release after12 steps, while settled
downward returns use8. Direction at trigger sets the trajectory; animation
facing is a separate field. New diagonal trials cross the catch-to-hold
boundary rather than assuming all downward animations are interchangeable.

Both players' incoming speeds288/256 are now checked through full capture
and settled hold, including quantized recoil decaying every two steps.
Their stationary capture lengths are18 (Mita) and22 (Yoo), versus16/20 for
the measured speed>=160 slower band and9/13 below160. Recoil tables now retain
ten levels including the final zero, generated offline.

## OBSERVED: lift height, complete charge and recapture

Lift disc14 copies only the player's integer X/Y, retaining incoming disc
fractions (`01BAD4/01BADA` word writes). Planar velocity is zero. Height starts
at0, vertical velocity at0x34000; integration adds velocity then subtracts
0x1500 gravity per step. The next player update changes ready0C00 to charge4.
Charge word+4E rises automatically underneath the disc: threshold43 for Mita,
40 for Yoo; completion sets player+21 bit4 on charge step45/42 and resets the
counter. That flag remains set after descending contact and held recapture.
Descending integer height<3 samples contact; the next step consumes it,
zeroes height/velocity and begins held action1004 with age1.

The original post-charge release uses action1410 and character-specific disc
states44/42, observed in exploratory no-write probes. These special flights
are not part of M2b2's native equality intervals; M3a must measure and implement
their complete chains before claiming special release support.

## TARGET: M2b2 bounds, display and saved states

This paragraph records the M2b2 boundary; the M3a findings below supersede its
charged-release and lob exclusions. Missed failed-preparation block10 flight
remains adapted independently of the measured normal/special lob model.

Original block10 chooses an RNG destination (`027C62..027DAC`). The native
failed-preparation block has a separate adapted vertical/planar flight;
only its input/contact classification is compared. An airborne miss currently
falls back to an ordinary slow flight, pending M3a's landing/bounce model.
Stationary lift/charge/recapture is compared exactly; movement cancellation,
dash preparation and other action-pose contacts remain separate work.

The lift uses existing ROM hold art with a small preparation marker and a
native outlined charge bar. The raised disc has a ground marker and its
height scales by one half, with a top visibility clamp. Approved court/HUD
art is unchanged. A charged native return currently uses the ordinary throw
path and clears the charge; full character specials remain M3a.

Ready/charge saves on both sides replay state and plane checksums over120
draws; scheduler tests ensure A does not retrigger across multiple logic steps.
Scenario11/12 use the lift script;13/14 interrupt an ordinary catch. Right-side
doors route the runtime pad to Yoo. New air/timing fields are included in
per-field hashes and native saves; recreate saves from before M2b2. No new
hardware path or ASM is introduced.

## OBSERVED: M3a original airborne and special trajectories

`tools/measure_advanced.py` loads normal-input serve/incoming doors, warms two
real frames, then repeats each trial with independent controller edges and
whole RAM/VRAM/palette/RGB/status fingerprints. No original RAM writes are
used. `generated/advanced_capture.json` retains all inputs and fields; the
report declares each native comparison interval. 62 trials cover 18600 frames
per pass, repeated twice, and 8573 native comparison steps. Six late-B cases
observe automatic ordinary release before B can request a lob. Ten charge
controls press movement/A/B early, or movement after recapture: these inputs
do not cancel the selected stationary lift/charge chain.

The selected Beach is **arena1**, not arena0. Its normal-lob row is
`(integerY-64)/20`; the eight-by-eight direction map at28176 selects target
bands76/93/110/127/144/161/176 from the arena's table at28244. Normal lobs clamp
hold62 and aim X=296-2*hold from the left, X=23+2*hold from the right. The global
RNG adds a target-band jitter; the reference fixtures condition native jitter
on the original target instead of claiming to reproduce that RNG.

Formation01D62E copies the player's integer coordinates while retaining disc
fractions, adds sixteen horizontal pixels and an original four-pixel vector.
Normal B action140C retains hold/history, releases at12 steps (downward settled
facing8) and finishes at28. Disc mode22 uses integer distance quantized down
to eight-pixel buckets;2A3E6 provides planar/vertical birth speeds. Quantized
original angle and trig produce the components. Each step adds VX/VY/VZ,
subtracts2A00 gravity, then damps all three velocities by subtracting their
arithmetic right shift5. Air Y is clamped74..198 on this Beach, X20..300;
ground wall contacts remain swept. A low descending disc can be recaught.
Height<=-16 enters mode28, zeroes velocities, then a miss awards two points.

Charged A action1410 uses the ordinary history normalization followed by
an eight-unit power deduction. Mita releases at12 steps, Yoo16; both finish
at52. Mita's mode44 speed is ordinary hold speed+16. Its32-bit angle changes
by speed*1536, alternates direction at128-angle limits, retains fractional
angle at wall reflection and reconstructs vectors from the original signed
4096-scale trig. Source026B96..026CAA /0287D2..028872 and the no-write timelines
agree through flight, walls and reception. Yoo mode42 starts vertically
(Up/neutral toward the top, Down toward the bottom), then at the first wall
retains `speed-speed/2-speed/8` horizontally toward the opponent. Subsequent
steps add `VX>>5`, capped at9 pixels per logic step. Source026A6C..026B84.

Charged B action141C retains possession age, deducts eight power units,
releases at12 steps in all measured directions and finishes at36. Mode38 uses
four-pixel distance buckets at2A68E, VZ=A8000 and gravityA800; planar velocity
is not damped. The special-lob Y target clamps84..188 for arena1. Very early
holds add eight pixels to its X reach. Height<=-16 enters mode40 with retained
negative height, zero VY/VZ and a horizontal speed of at least four pixels;
each step adds `VX>>4`. These original airborne miss/bounce timelines are
included through the pre-score boundary.

## OBSERVED: M3a strong reception and counter window

Special catches retain constant recoil reconstructed from the original speed
word/angle, including special lob speeds which differ from raw VX. The
ordinary decay tables are not reused. Holder rear-boundary nudges precede a
forced push (player1808, disc24) on the following update. Vertical-boundary
reaction changes recoil direction and takes precedence over that X flag.
An original projected landing marker can invite automatic charge before the
special lob descends, even though the flying disc is still far away.

With Mita's measured fast wave, fresh A on Yoo's catch returns the incoming
mode44 with Yoo's16-step release; A one step later releases an ordinary
straight shot with the reception's8-step animation. Yoo's fast wall special
can be countered on Mita's catch; the following step is already the forced
push and ignores A at the rear boundary. Original01CD74 checks the normalized
age against2 for both selected characters. Tests include those accepted and
rejected edges, the returned flight and the next contact/pre-score boundary.

## TARGET: M3a native boundaries and data bank

Native goal-entry/celebration positions and the90-step direct loser serve
remain adaptations; the original reference includes scoring, but native
physics comparisons stop before its score/celebration entry. Goal values and
the two-point ceiling/reset have independent native checks. Rear deflection,
missed block10 RNG flight and full action-pose geometry are still adapted.
Special-lob100C/1010 selection uses the native contact angle: tests compare
their shared strong reaction family and exact recoil physics, not that pose
subtype. The original hold word has independent idle-animation uses; compare
it during active possession/ready/charge actions, as in earlier milestones.

The existing139 outlined actor variants are copied without pixel changes to
wjart (host/TI word orders), read in place through rt_file. Metadata/bounds are
checked once at initialization. Normal Titanium build38949 bytes, wjart.89y
30008 bytes; ROM/BIOS/banks/captures remain ignored. Throw/charge art reuses
ordinary ROM animations and native cues pending the remaining action-art slice.
Eight airborne/special saved-state replays compare140 states and plane pairs
each. Twenty-three PC/TI doors plus1000 training frames pass6060 per-field
hashes and24 screens; the normal court/HUD stays approved. No new ASM or
hardware path. The signed/unsigned word-product helpers compile to MULS.W /
MULU.W; no division, long-product or float/library helper occurs in physics.


## OBSERVED: M3b directional history and curve recognition

`tools/measure_curves.py` checks 180 no-write trials twice with complete
RAM/video/status fingerprints: 43200 original frames per pass, 17764 native
equality steps. It reuses the two possession doors, warms two real frames,
resets frontend pads and restores the saved framebuffer on replay. Temporary
prepared states are process-specific. Generated reports retain every input,
field, comparison interval, core/ROM metadata and program/script identity.

Source01BC80 shifts a 64-byte history at100900/100940, storing a direction
from the pad table01BE94. The recognizer01D0CA reads only the newest nine
bytes. Neutral and Up both encode0. From newest to oldest it skips at most
four equal samples, requires a +/-32 adjacent direction, skips at most four
samples of that direction, then requires a differing third sample. If the
normalized power word+52 is below16, that third direction must be the next
adjacent direction of the same arc. At16 or above any differing third sample
suffices. Source01CCCE normalizes power before recognizing; a successful
curve then subtracts four power units at01CE8C..01CEA8. Source therefore
explains both the exact four-step segment window and neutral-assisted short
arcs. The 1..5-by-1..5 running-ROM sweeps verify both boundaries and directions.

Recent angles descending by32 backwards select mode6; ascending select8.
Their actions are1404/1408 on the left, swapped on the right. A requests the
recognizer; B retains its separate lob branch. Eight final directions, skipped
arcs, neutral gaps and higher-history delayed possession controls are checked.
The full curve aim map comes from01D444. Combined horizontal/vertical
inputs can select16/48 endpoints, while the earlier three-direction ordinary
fixtures used Up/Down alone (32/96). Native ordinary aim remains the prior
three-direction model; this slice checks the full input-to-aim map for curves.

## OBSERVED: M3b release, rotation, wall conversion and recoil

Curve animations have six poses at four logic steps each; throw action ends
at24 steps. The release event tables parallel those poses: first zero entry
selects a12- or16-step release depending on1404/1408 and held facing. Those
small delay tables are extracted offline through20A3E's pointers and checked
against the timelines; they are distinct from ordinary8/12-step releases.
Formation retains disc fractions, copies integer actor coordinates, adds16
horizontal pixels and the four-pixel vector, as for advanced throws.

Mode6/8 initialization027BA6..027C2A uses the normal hold-speed lookup and
character angular multiplier at027C2C: Mita560, Yoo480. The angular increment
is +/-speed*multiplier in original16.16 units. Each update025EA8 adds it to
the32-bit angle, derives the speed-scaled vector from the original4096-scale
trig and integrates position. This preserves negative-angle wrap and fractions.

A curved wall reflects the angle, adds +/-16 according to the arc sign, then
applies the side/facing limits, retaining the fractional angle. The angular
increment becomes0 and disc mode becomes4, with the new arbitrary straight
vector. The original contact normal+1D is128 at the upper wall and0 at the
lower wall; do not confuse it with a wall-position label. This distinction
matters after an angle crosses zero: the upper-wall limit caps the measured
Mita return at112, verified by the wrapped-angle trial. The complete curves,
conversion frames and subsequent straight flight match original snapshots.

Front reception uses ordinary capture, not a special's constant push. Dynamic
speed/angle reconstruct quantized recoil at each halving step; slow incoming
curves use stationary capture. Twelve A controls at one/two steps after catch
or settled hold continue through the returned straight flight. In the checked
fast Yoo curve reaching Mita from above, reception facing96 gives an8-step
immediate release; the opposite reception and settled straight return use12.
Those differences are preserved instead of reusing a universal catch delay.

## TARGET: M3b bounded comparisons, controls and saved states

Native geometry, outlined ROM bank, white sand, centered numeric point bands,
net without shadow and compact scores/countdown are preserved. Curve throwing
uses the existing ordinary outlined poses; exact curve pose art remains a
separate extraction slice. Rear deflections, other action-pose/moving contacts
and arcs during other special/preparation actions are not included in these
stationary possession/front-return fixtures. Goal-entry/celebration geometry
and direct native service remain adaptations; exact comparisons stop before
x exits the native16..303 court. B controls compare their complete gesture and
lob preparation prefix; normal lob RNG flight remains the M3a conditioned model.

Nine direction bytes per actor, held facing, dynamic disc vector/angle and
recoil parameters live in the native state and explicit PC/TI field hash.
Recreate pre-M3b native saves. Four curved scenario doors with per-draw scripts
exercise mirrored clockwise/counterclockwise motions, missed goals and service.
Eight real native state-file replays compare140 states/plane checksums each,
saving during the gesture or curved flight. All27 scenarios plus1000 training
frames pass6940 hashes and28 final screens. No runtime/hardware changes or ASM.

The normal Titanium program is40051 bytes; wjart.89y remains30008 bytes
with the same SHA-256 (1e125284f5ff2782...). Twenty-four compiled timelines
profile4840 frames, peak214812 cycles (59.7% of360000). Codegen still has only
word products, no division/long products/float/arithmetic helpers.

## OBSERVED: M3c dash input and motion

`tools/measure_dash.py` restores the original free-player serve/free doors,
warms two frames, then presses direction+A on local frame4. It records both
characters/eight directions with A held for1,2 or20 frames. All48 trials
replay with identical RAM/video/status fingerprints (1728 frames per pass).
Program, script and reference-core/BIOS identity are retained in `dash.json`.

Direction+A without possession selects action0800; stationary A selects0C00.
Initial axial/diagonal vectors are645120/456120 for Mita and673792/476392 for
Yoo, in16.16 units. Each later step subtracts the arithmetic component>>3,
including its asymmetric negative rounding. Court clamps preserve fractions.
A held through the early event delays animation progress while motion keeps
decaying. A one-frame tap produces11 Mita or13 Yoo integrated steps; holding
for two frames produces12/14. The complete original wall animation can differ
from that duration, notably Yoo's0808 horizontal-wall recovery.

Native comparisons cover816 original positions/velocities: the first16/18
timeline rows (four idle steps followed by the bounded dash interval) per
trial. Do not extend that equality claim to the original horizontal-wall
animation/recovery, moving collision boxes, or all20-held-frame tails.

## TARGET: M3c controls, readability and showcase

Direction+A now dashes; neutral A keeps the measured precision reception.
Native wall recovery expires while A remains held, using a short recovery
rather than reproducing all original0808 events. Dash captures clear active
dash movement; original moving-pose contact classification remains separate.
Existing outlined ROM walking poses and two transient grey streaks represent
the motion. Native dash replay compares states and planes on both sides.

The disc ring increases from six to eight pixels and retains a white outline.
Height projection changes from half to quarter scale, keeping ordinary lob
and lifted-disc arcs closer to their ground paths. A fixed bracketed target
uses the actual lob destination, while the small moving grey shadow uses
current ground coordinates. These cues preserve plain sand, symmetric numeric
point bands, the outlined net without its shadow and the compact central HUD.

A pre-contact hint predicts aligned flight four logic steps ahead; stationary
A preparation still uses the original short window. Both reference doors now
pass the player-visible hint -> tap -> lift -> full charge -> recapture -> A
special chain. Wider charge bars and a full-charge star make progress explicit.
This does not claim that an ordinary held disc charges by holding a button.

All29 native scenarios plus1000 training frames pass7380 PC/TI per-field hashes
and30 final screens. Twenty-six compiled timelines profile5040 frames with a
230746-cycle peak (64.1% of360000). The Titanium program is41461 bytes;
wjart.89y remains30008 bytes with identical outlined actor pixels. Hardware
paths and ASM are unchanged. Recreate native saves made before M3c.

`make showcase` captures1185 actual native LCD frames, exercises every existing
action family and adds the name Windjammers TI-89 and explanatory captions
outside the full-screen court. The loop lasts44.14 seconds; ROM assets, raw
frames, CSV states and the GIF stay local/ignored.

## TARGET: powerful and charged flight trails

Native effects distinguish speed-word>=256 ordinary/dynamic ground flight
from charged Mita/Yoo shots, super lobs and their ground acceleration.
The first uses two fading afterimages; the second adds a third fading sample,
an energy ring and two animated perpendicular sparks. Original ROM effect
sprites are not claimed. Gameplay integration, contacts and shot strengths
keep their existing measured models.

Four projected LCD positions are sampled once per draw in `game_update`,
after all source-rate logic steps, never in the renderer. Super-lob samples
use the same height projection as the head disc. Reset/catch/goal clear the
visible effect; category and owner changes start a new path. Trail samples
and metadata are explicit native save/hash fields; pre-effect native saves
must be recreated. Effects draw behind actors, targets and HUD, with the
white-outlined main disc on top. No floating point or runtime trigonometry.

Headless tests cover both powerful returns, both character specials, special
lob/bounce, slow-curve exclusion, real previous positions, capture/goal
clearing, five saved-state continuations and pure repeated renders. In
addition to7380 PC/TI hashes and30 final screens, six screenshots selected
during full trails have identical PC/TI planes. The updated named showcase
retains1185 actual frames and44.14 seconds; the short effects preview lives
in ignored `x/Windjammers-TI89-effects.gif`. Current compiled cycle costs and
42,235-byte Titanium program size are recorded in README; the art bank is
unchanged. Runtime hardware and assembly are unchanged.
