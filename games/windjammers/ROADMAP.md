# Windjammers native roadmap

## Required action scope confirmed by the user

The following actions are required for this port, not optional later polish:

- Lobs, including height, travel, landing and the defender's response.
- Original rebounds: retain checked ordinary wall bounces and measure the
  additional wall/contact/landing behavior used by the requested actions.
- A precisely timed reception that lifts the disc, followed by charging and
  releasing the character's special. Validate this complete chain.
- Curved throws entered with directional arcs/circular motions, in both
  directions and mirrored for the two sides of the court.
- Powerful immediate returns when reception and rethrow are timed within the
  original short window. These are distinct from M2b1's settled-hold throws.

Measure the original input sequences and frame windows before choosing native
controls or thresholds. Keep H. Mita/B. Yoo and the approved court/HUD while
adding these actions. Prioritize them over additional scenery or characters.

## M1: Beach mechanics and a playable training court

Completed: headless original/native fixtures, playable training court,
1540 PC/TI field-hash samples and seven final screens, saved-state replay,
30,000-step practice stress run, normal TI build and per-frame profiling.
Peak measured game frame: 168550 cycles, below the 360k budget.

Provisional target choices: H. Mita on the left, B. Yoo on the right,
one human against a simple native practice partner. The original two-human
reference remains independently controllable by the test API. These choices
can change when the user answers the mode question. Arrows move, A/2nd
throws, Up/Down+A aim diagonally, Enter restarts, Escape quits.

Use portable C on the existing runtime. Keep original 16.16 coordinates and
59.18 logic steps/second with an integer scheduler, drawing at 256/8.5 Hz.
Crop to the playable court, at half scale: original X=16..304 and Y=72..200
become 144x64 pixels. Use geometric outlined actors for engine review; art
extraction belongs to M2. No new assembly or runtime changes are needed.

Acceptance:

- Headless unit tests before SDL/TI: neutral movement for both characters,
  ordinary disc integration and exact fractional wall contacts; independent
  collision-angle calculation checked against original contact diagnostics.
- Original CSV comparisons from reproducible doors for all sixteen walks,
  the three measured Yoo launches through contact, and Beach goal boundary
  and point-zone diagnostics. Report which fields and intervals are checked.
- A complete native rally, goal, persistent 3/5-point scores and loser serve;
  practice AI and adapted possession/reset behavior explicitly labelled
  TARGET, not compared as if they were original animation/AI.
- Scenario doors, input scripts and PC state save/load. PC/TI per-field state
  hashes and screen checks, and actual compiled TI frame costs below 360k.
- Build the normal Titanium program; the existing runtime owns all hardware
  paths. TiEmu only if new hardware behavior needs verification.

M1 does not claim full original action fidelity: catch knockback, turning
animations, deflection trajectory, automatic-hold timing and ballboy delivery
are separate from the measured neutral contact decision. Training scores
continue until reset; original round/time-out endings are not implemented.

## M2a: ordinary captures and ROM actors

Completed: six ordinary throw/catch/settled-hold timelines compared through
110 original steps each, including both players' positions, velocities and
actions and the disc's position/state. Mita's incoming Yoo-shot recoil uses
original speed quantization, decay and holder boundary nudges; Yoo's captures
retain the measured stationary animation lockout. The practice partner waits
for capture completion before pressing A.

Original actor tiles/pose sequences are decoded offline, with the measured
palette overrides, flips and anchors. Four-grey conversion and white outlines
replace the geometric actors; playback is native. Original RGB validation
uses VRAM depth to exclude occlusion independently of pixel differences.
The disc/HUD remain native; selected ROM court scenery is now adapted below.

The requested display revision expands the court to the entire 160x100 LCD,
with compact two-digit scores and a 30-second countdown between them at the
top. Eight-pixel numbered 3/5/3 bands have geometry centered at LCD Y=49.5,
with upright digits on both sides. The native net has a black outline and
no ground shadow. Keep this design, the current score/countdown and the
completely white floor without sand marks, as requested by the user. Title and
control bands are removed. Native coordinate lookup tables stretch the court
view while retaining original physics units and half-scale ROM actors;
cosmetic boundary clamps keep the outlined sprites visible.
The native training clock ticks through goals and stops at zero; round/match
endings remain M3. Scores saturate at 99.

Acceptance passed: 660 complete-action fixture steps, deterministic original
action replay, original RGB scene comparisons, native unit/stress tests,
nine PC/TI replay doors including both capture types, saved-state continuation
and compiled TI frame profiling. See README for current counts/costs.

## M2b: remaining action fidelity and court artwork

### M2b1: possession timing and ordinary return strength (completed)

Both characters' automatic releases and manual returns at different
possession ages are measured through subsequent capture/hold. The ROM's
hold-age/history/bonus normalization and speed tables now drive native
ordinary straight/diagonal profiles generated offline, with no runtime
division or multiplication. The approved court and HUD are preserved.

Acceptance: replay every original measurement twice, compare native position,
velocity/action and possession age during throw/catch/hold through each selected
release/flight/catch (idle anticipation counters remain separate);
add two 600-step full-rally no-write controls, scenario doors and native reset/
save-load tests; check all new PC/TI doors and compiled per-frame costs below
360k. Early A during capture, moving/action-pose contacts and rear-flight
deflections remain independent M2b slices unless measured here explicitly.

Acceptance passed: 4058 original possession/action steps from 29 twice-replayed
normal-input trials (7140 replay frames), including two 600-step automatic
rallies; all existing mechanics tests and 30000-step practice; eleven PC/TI
doors and 3420 state hashes with twelve equal final screens; state-file
continuation; normal Titanium build and 1320 individually profiled TI frames.
Peak measured cost: 200950 cycles under 360000. No new hardware path or ASM.

### M2b2: immediate reception, lift and powerful return timing (completed)

Use reproducible incoming-disc doors for both characters. Sweep button presses
before, on and after contact at individual original logic steps; distinguish
held buttons from fresh edges and input accepted during capture lockout.
Identify the windows and input sequences for ordinary catch, disc lift and
powerful immediate rethrow. Record the lift's height/velocity and the entry
conditions for charging a special, without inventing a timing tolerance.

Acceptance: normal-input reference controls and identical repeated replays;
positive and negative cases on both edges of each measured window; native
action/position/velocity and disc-state comparisons through complete immediate
returns and lift-to-charge entry. Add scenario doors, saved-state continuation,
PC/TI checks and frame profiling. Test input edges across multiple logic steps
per draw so the scheduler does not duplicate a timed action. Full special
charge/release trajectories continue in M3a.

Completed: 78 no-write input trials, each replayed twice with 10920 identical
original frame fingerprints; 6704 native equality steps and eight separate
block classifications. Both stationary preparation windows, ordinary catches,
capture-interrupting straight/diagonal returns, high-speed captures, lift
height/velocity, automatic charge completion and airborne recapture match the
selected original timelines. A release edge while holding also throws; a held
button does not repeatedly trigger. The original reception animation retains
its 12-step throw release, including downward returns; settled downward throws
use eight steps.

Native ready/charge state-file replays check both fields and planes on each
side, including a timed input spanning multiple logic steps per draw. Fifteen
PC/TI doors plus 1000 uninterrupted training frames pass 4300 hashes and
sixteen final screen checks. Normal Titanium build: 61615 bytes. Current
per-frame costs are in README. No new hardware or ASM.

Bounds: the original failed-preparation block flight selects RNG targets;
native block flight is adapted, with its classification checked separately.
Charging uses an existing ROM hold pose and a native visible bar. The fully
charged flag survives airborne recapture, but release currently uses an
ordinary return: character-specific specials, lobs, airborne misses/rebounds
and charge cancellation remain M3a. Directional arcs remain M3b.

### Remaining M2b work

Measure both characters' complete throw/hold/knockback/turn timelines and
rear-deflection trajectories, then compare native state through whole rallies.
Ordinary idle-defender captures are covered by M2a; next isolate moving
defenders, other action-pose boxes/facing and rear contacts. Stationary ready
boxes and early A during capture are covered by M2b2. Automatic no-input launches are
recorded in
`generated/actions.json`, with different measured speeds from manual throws;
M2b1 now covers ordinary manual and automatic possession with native equality
fixtures (29 trials, 7140 replay frames, 4058 native comparison steps).
Keep each added action bounded and tested before claiming full action fidelity.

Selected Beach panels/net/shadow already pass 41,344 original RGB scene pixels
and independent asset ownership checks. Decode remaining Beach/FIX assets only
within the requested visual scope, preserving the plain white floor and
readable native disc. Put larger ROM banks in local archived data files
before an unpatched TI-89 release; the current normal program targets Titanium.

## M3: selected match rules and actions

### M3a: lobs, airborne disc, rebounds and charged specials (completed, bounded)

Implement the requested lob and complete M2b2's lift -> charge -> special
release chain. Measure height, airborne/landing transitions, landing/contact
rebounds, charge duration/thresholds, cancellation and release behavior for
both selected characters. Trace the resulting special flight through walls,
defender interaction and scoring, including its return/counter behavior.
Do not count the existing ordinary wall reflection as completing these actions.

Acceptance: complete normal-input original action chains, boundary cases for
charge/release and height/contact, native replay fixtures, readable airborne
and charge cues on the existing white court, scenario/save-load and PC/TI
checks, and compiled costs within the frame budget. Generate trajectory tables
offline where appropriate; no runtime floating point or trigonometry.

Completed: 62 normal-input trials replayed twice (18600 original frames per
pass), 8573 native fixture steps, both sides' normal lobs, timed charge/recatch
and A/B releases, Mita's wave, Yoo's vertical wall/accelerating horizontal
flight, special-lob ground acceleration and strong reception/return counters.
The measured movement/A/B presses during charging are ignored; these inputs do
not cancel the original selected lift/charge action. Normal lob landing awards
two points; native service resets all height/special state.

Eight advanced native scenario doors/save-file replays check state and planes,
including complete special-lob miss/bounce/goal/loser-service sequences and the
two-digit score ceiling. All 23 PC/TI scenarios pass 6060 explicit-field hashes
and 24 final screen checks. Sprite planes moved byte-for-byte to an archived
data bank, preserving the approved court/HUD. Frame costs are in README.

Bounds: normal-lob reference targets condition the native jitter; global Neo
Geo RNG is not ported. Goal entry/celebration geometry, rear deflection, idle
reuse of the hold-age word and the special-lob 100c/1010 pose selector remain
adapted. Special-lob reaction family, actor/disc physics and strong recoil are
checked. Throw/charge art uses the existing outlined ROM bank and native cues;
full special animation extraction belongs to the remaining action-art slice.

### M3b: directional arcs and curved throws (completed, bounded)

Measure the original directional sequence recognition, allowed sequence timing,
throw-button relationship and curve evolution. Cover clockwise and
counterclockwise arcs, both court sides and both characters; determine the
accepted arc lengths from the ROM rather than assuming a gesture size.
Check incomplete/late gestures, straight-throw fallback, curved wall rebounds,
capture/return and scoring. Preserve input history in native saved states.

Acceptance: reproducible original gesture traces and full curved-flight
comparisons, tests at gesture-window boundaries, native controls usable on the
TI keypad, scenario doors, PC/TI checks and per-frame profiling. Use measured
integer/LUT trajectories rather than an invented visual curve.

Completed: 180 no-write trials replayed twice (43200 original frames per
pass), with 17764 native equality steps. Both arc directions, all eight final
directions and both selected characters/sides are covered. The 1..5-step
segment sweeps check both timing boundaries; skipped and neutral-interrupted
motions, low/high history strength, A versus B, complete curved flights,
wall conversion to straight flight, recoil and twelve immediate/settled
return controls are included. Native input retains the nine samples actually
read by the ROM recognizer. Eight saved-state replays restore mid-gesture
and curved flight, then compare state/planes through scoring and service.

Twenty-seven PC/TI scenarios plus 1000 training frames pass 6940 hashes and
28 final screens. The approved art bank, full-screen court and HUD are
unchanged. Frame costs and build sizes are in README. No new hardware or ASM.

Bounds: first-flight comparisons stop at the native goal-entry boundary;
celebrations/direct service remain adapted. Rear deflection, moving/other
pose contacts, arcs during other special/capture preparations and exact
curve throw sprites remain separate. Existing outlined ROM throw art is
reused. Training mode still has no original round/time-out ending.

### Remaining match scope

Measure and implement the chosen round/time-out rules and opponent
behavior. Additional courts/characters follow selected scope. Menus, sound and
link play require their own requested scope.

### M3c: dash and action readability (completed, bounded)

Player review requested dash, a larger disc, readable lob destinations and
an understandable route to charged specials. Acceptance: original no-write
dash trials, native motion comparisons, direction+A controls, explicit charge
guidance, per-frame PC/TI equality and compiled TI budget checks. Export a
named showcase of the actual LCD covering all implemented action families.

Completed: 48 dash trials replayed twice (1728 original frames per pass),
816 native position/velocity comparisons, both characters/eight directions,
short/held inputs, fractional clamps, native dash replay and capture recovery.
The eight-pixel outlined disc, quarter-scale airborne height, fixed lob target
and distinct moving shadow improve visibility. Wider charge bars, full-charge
stars and the aligned-incoming A cue guide both successful special sequences.
The GIF uses the name Windjammers TI-89, actual native frames and captions.
All 29 scenarios plus the 1000-frame training replay pass 7380 field hashes
and 30 final screens. Runtime hardware paths and assembly are unchanged.

Bounds: horizontal-wall dash animation/recovery, moving contact poses and
dash art remain adapted; native art uses the existing ROM walk poses and
short motion streaks. Original round/match endings and CPU AI remain separate.

Follow-up visual polish adds fading powerful-shot trails and stronger charged
energy/spark trails, sampled from real projected flight positions. Unit/save
replay tests and six active-effect PC/TI screenshots pass alongside the7380
field hashes/30 final screens. The named showcase now includes those effects.
