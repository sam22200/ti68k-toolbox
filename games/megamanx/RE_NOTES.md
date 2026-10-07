# Mega Man X (USA): measured reference

## Source and current scope

**OBSERVED:** local `roms/snes/Mega Man X (USA).sfc`,1572864 bytes with no
512-byte copier header. SHA256
`3e1209f473bff8cd4bcbf71d071e7f8df17a2d564e9a5c4c427ee8198cebb615`.
LoROM title `MEGAMAN X`, version byte0. The existing pinned Snes9x core reports
60.098811862348406 Hz and 256×224 video. Default timing/options remain unchanged.

**TARGET:** follow the prompt's final short-section clause, without an answer
to the optional clarification: X128 spawn through X1008, crossing the first
gap and its raised right road. Half-scale rendering keeps original-coordinate
physics; arrows run,2nd jumps/wall-jumps,Shift fires/charges. These are disclosed
working choices. No dash/armor/boss/Zero sequence, later level, sound or menu.

**Reading aids (not evidence for this ROM):**
[MegaED X](https://github.com/rbrummett/megaedx_v1.3) exposes the level layout,
scene/block/map hierarchy and collision tables. Its source lives locally under
ignored `sources/megaedx/`.
[Mega Man X recompilation](https://github.com/mstan/MegaManXSNESRecomp) provides
candidate player/enemy structures, locally under `sources/megamanx_recomp/`.
Their offsets and mechanics must be confirmed against this version. Neither
project is the calculator engine; no generated CPU emulation is proposed.

## Deterministic door and memory

**OBSERVED:** `tools/reference.py` boots2200 frames with `keys/ref_boot.txt`
to Highway, stage 0, X128/Y367 and camera0/256. No RAM injection is used for
that door. Two100-frame runs after restoring its state agree on all WRAM,
VRAM, CGRAM, OAM, register mirrors and RGB video hashes. Source identity,
core/options/key hashes and post-frame sampling are in local `start.json`.
This cartridge exposes no SRAM; do not make an unavailable SRAM snapshot a
mandatory part of every SNES game's integration check.

**OBSERVED on this ROM:** player X/Y at 7E0BAD/7E0BB0, fractional bytes
7E0BAC/7E0BAF, signed8.8 VX/VY at 7E0BC2/7E0BC4, state/substate
7E0BAA/7E0BAB, HP7E0BCF, facing flags7E0BB9. Charge fields are
7E0BFF/7E0C00/7E0C03. Camera X/Y7E1E4D/7E1E50, stage 7E1F7A.
HP confirmation includes the natural16→14 contact event. Player shots occupy
eight64-byte slots at 7E1228..7E1427;16 enemy slots at 7E0E68..7E1227.
Object X/Y are +5/+8, X fraction +4, VX +1A, kind +A. Enemy HP +27 has
bit7 set transiently on impact, not128 extra life.

**Isolation disclosed:** movement/wall probes clear the active byte of
enemy/projectile pools before each original frame; no player/camera/timing
anchoring. Damage and enemy-shot timelines initially use unmodified play.
`hurt_reference.py` then anchors only the roller near X to maintain contact;
repeated damage occurs92 updates apart. A guessed invulnerability address
7E0BCE failed to prevent damage and is not an accepted timer address.

## Movement and contact

**OBSERVED:** idle0, start2, run 4, rise6, fall8, land10, hurt14, kick16,
slide18. Running is 376/256 original pixels per logic tick. Start waits one
frame, moves four ticks at 256/256, transitions without movement, then runs.
Release stops immediately; reversing a run changes direction immediately.
Idle jump initializes VY1363 without moving on the first frame; a running
jump integrates immediately. Positive VY points upwards, subtract64 gravity
before movement. Ending the hold or passing the apex resets VY to 0 on the
fall transition. Fractional integration and preserved landing fractions matter.

**OBSERVED:** the first pit spans X800..831. Its right wall begins832, with
road surface352 versus 384 before the pit. X's resting center is floor−17.
Right-wall grip clamps X824; a slide pauses eight updates, then descends
at 512/256 pixels per tick. Wall kick has an initialization pause and four
windup ticks, launches at−376/1363, forces seven away movement updates,
then resumes normal air steering; held/short kicks differ after that lock.
The eight-pixel outer extent clamps before the inner seven-pixel sensor
reports side contact. Controlled first-contact probes below resolve the
earlier tentative current-position interpretation.

**OBSERVED:** first natural contact at reference frame 286 is X543/Y367
against kind15(hex), roller at 561/364. HP16→14. The next update initializes
VX−138,VY512; gravity64 and a short recoil follow. At frame 317 state returns
idle while retaining velocity until the next idle update. Full recoil
fractions are checked against 90 original samples.

**TARGET:**92-update player contact immunity, native blink and death/retry.
Activate the roller at X480, spawn 624/368, then initialize364. Original
streaming causes occasional lag, so natural frame243 activation is not a
universal native frame number. Its regular phase sets VX−384 each update.
Enemy damage and braking now follow the natural timelines below. The initial
20-update enemy-immunity rule was a mistaken reading of unchanged HP after
the second hit; it is removed. Player immunity remains separately measured.

## Buster

**OBSERVED:** Y press emits kind0 immediately;31 held updates permit kind1
on release,101 permit kind3 with the starting equipment. Threshold cases
30/31 and 100/101 are measured explicitly. Pellet initial VX1024, accelerated
by64 before integration. Stored speed caps at 1536, but the integrated step
still includes the extra64: cruise is 6.25 pixels/update. Medium follows the horizontal
muzzle through age9 then travels at 1536/256. Large follows the horizontal muzzle through age5,
sets VX2048 at age6 without moving, then travels8 pixels/update. The stationary reused
charged slot retains fraction64 after the first pellet is gone in this
stationary case; it is not a universal projectile initialization value.

**TARGET:** at most three simultaneous shots; target-visible-width culling
replaces the original256-pixel viewport boundary. Charge aura is native
white/black geometry; firing/movement/projectile art comes from the ROM.
Do not call the initial-equipment kind3 shot an armored buster upgrade.

## Coupled firing: run, jump, reversal and wall slide

**OBSERVED:** `combined_reference.py` samples fourteen cases from original
start/wall states. It clears enemy primary states only, preserves player
projectiles, and does not anchor the player/camera/timing. 656 source-active
projectile observations cover taps, both running charge tiers, reversal during
large-shot formation, immediate/late airborne release, and wall taps.

Running large release uses X+27/Y−4. Its formation follows X through ages
1..5; age6 retains the previous X while setting VX2048, then travels eight
pixels/update. Medium follows X through age9. Both keep launch Y, even while
X rises; changing direction during formation preserves the launch side.
A precharged jump released immediately keeps X+16/Y−3. A later rising
release/tap uses X+25/Y−8. The right-wall tap before the turn uses X+19/Y−9;
a late tap uses X−18/Y−2 and travels left despite RIGHT still being held.

**TARGET:** semantic muzzle families reproduce those launch rules without
looking up full source animation metadata per tick. Normal running-shot bob
is flattened to Y−4: at most one original pixel of Y error in the oracle.
All coupled X and other sampled Y values are exact. Unused fractional carry
for running charged slots is excluded: original viewport removal leaves a
different fraction than the wider native camera. Integer charged travel does
not use that fraction. Original-active trajectories are all compared; extra
native shots may survive source removal. Preserve the exact stationary oracle.
Native firing art skips standing poses in the air and selects outward-facing
wall poses after the turn. The clear/retry message has a white backing panel.

## Leftward shots and screen-edge removal

**OBSERVED:** three additional original cases cover left taps and both charged
release tiers. They add 104 projectile samples, including wrapped X values
below zero; total coupled observations are now 656. Left normal and medium
formation store negative VX. These large-formation cases retain positive VX1536 even
when launching left, then changes to −2048 at age6. Its launch side cannot
be inferred from its stored formation velocity.

**OBSERVED:** `culling_reference.py` adds 744 controlled one-coordinate
probes, both directions and all three kinds. Each restores a natural flight
state and changes only projectile X before advancing two frames; player,
camera and timing are not anchored. Normal removal follows the integrated
coordinate, with inclusive −32 and exclusive 288 bounds relative to the
stationary 256-pixel camera. Charged first-frame removal does not react to
the new coordinate; its second-frame active flag follows the first integrated
coordinate with the same bounds. Every probe asserts these rules.

**INTERPRETATION:** charged removal consumes visibility calculated during
the preceding update. No RAM address for that cached flag is claimed. Using
the preceding integer position reproduces natural charged trajectories.
The earlier 24-pixel normal pre-update margin fit the coarse natural samples
but was not uniquely established; these probes replace it with the measured
32-pixel post-integration rule.

**TARGET:** signed coordinates for drawing/removal, normal post-integration
and charged pre-integration bounds, both with 32-pixel grace. Extend the
right boundary from source 288 to native 352 for the wider logical viewport.
Remove the premature absolute-world check. Preserve launch side separately
from large formation VX for flight and art. All 104 added natural projectile
observations still pass on PC and the compiled 68000. Six scenario doors
exercise last-active normal/charged coordinates on both native edges.

## Terrain and art

**OBSERVED:** stage 0 LoROM pointers from the tables868D24/868D93/868E02/
868E71/868EE0 resolve to layout928140, scenes92E180, blocks96C428,
maps998000 and collision95E1E1. Layout RLE expands32×4 scene IDs and
matches WRAM7EE800. Scenes are8×8 block words; each block contains four
16×16 map IDs; each map contains four8×8 PPU attributes. Map IDs also match
the original7E2000 collision cache. Collision byte low six bits select
profiles; this extracted band uses 0,34,35,3B(hex).

**OBSERVED:** `tools/art.py` reproduces the first streamed256×256 BG1
tile region exactly, including flips and palettes. The shared
`tools/snes/ppu.py` handles Mode1 backgrounds and planar OBJ.85 hero frames
cover idle, run, jump and running jump:40255 opaque pixels match original
video, maximum RGB565 conversion error 2. The expanded action check adds
firing, wall slide/kick/fire and hurt: 313 hero frames /158073 pixels in
total, plus 151 projectile/enemy/effect frames /64999 pixels.

**OBSERVED:** original OAM is built using pre-update object positions.
For a standing pellet, first post-frame RAM X144 has no displayed pellet;
next RAM X148 has OAM tile left140, centered144. Anchoring that sprite at
post-update148 shifts the asset four pixels. Save the object's screen
anchor before `retro_run`, extract afterward. Applying this to jumps,
shots and rollers removes duplicate offset variants:194 mirrored semantic
poses before adding the broken roller,212 afterward (39768-byte art bank),
then222 /42216 bytes with the dedicated wall-kick firing spans.
Charge formation and flight use distinct spans.

**TARGET:**2×2 half-scale reduction, four foreground/actor greys, one-pixel
white actor outlines, pale two-grey BG2. Parallax is flattened/repeated
offline; original draw priorities and animation timings are simplified.
Static foreground vehicles are scenery here, not added enemy behaviors.
Scenery/collision bank 17424 bytes; actor rows have native PC and big-endian
TI variants. ROM, generated pixels, states and fixtures remain ignored.

## Acceptance

`make test`:1230 original movement/recoil states,72 projectile states,
charge edges, damage/death/retry, spaced-pellet defeat and winning traversal.
`make original-ti`: those 1230 and 72 original states checked directly on
the compiled 68000 code, plus 860 coupled projectile observations with the
explicit adaptations above,1040 natural enemy observations and1320 natural
contact/recovery states (including188 projectile samples).
Capacity adds245 updates /278 projectile samples; wall firing adds840 hero
states /531 projectile samples,91 muzzle probes and24 controlled fraction samples.
`make check`:4759 complete 71-word PC/TI states
and 4759 LCD sums, including charged fire, contact, wall kicks and maximum
three-shot/roller/explosion density. Per-frame datasheet peak184810 cycles,
budget360000 at 512/17 renderedHz; two logic ticks per render (60.2353 Hz,
about 0.23% faster than this NTSC ROM). No runtime float or32-bit arithmetic
helpers are needed in game hot paths. No TiEmu/hardware run is claimed.

`make preview`:1205 consecutive rendered updates,40.01s GIF, centisecond
delays distributed to the real target rate, no chapter holds or missing
updates. See local `x/checks.json`, `x/original-ti.json`, `x/art-checks.json`
`x/combined-ti.json` and `x/preview.json` for machine-readable results.

**OBSERVED:** gun flashes, wall dust and charge sparks can cover hero pixels
with palettes 2 or 0. Comparing an isolated palette-1 hero directly with
those pixels produces apparent extraction failures. `check_art.py` resolves
OBJ ownership in OAM index order before comparing visible hero pixels. Every
excluded pixel must match the actual covering OBJ in the independent video;
unexplained mismatches are still failures. Maximum RGB565 conversion error
remains 2 across all tested actions. This check validates extraction, not
bit-exact native pose timing or original SNES layer priority on the TI LCD.

## Verified roller reaction, charged penetration and source hitboxes

**OBSERVED:** `enemy_reference.py` restores the natural roller at reference
frame260, X600/Y364/VX−384/HP2, and player X505/Y367/RUN/XS16. It captures
no-fire, one tap, two taps spaced12 or32 updates, jump/turn/shoot from behind,
and natural medium/large releases. No RAM injections are used in these seven
cases. 1040 observations match PC and compiled68000 enemy X/Y/fraction/VX,
masked HP, phase, fuse, explicit death and player HP. Extra native actors
following source viewport removal are the disclosed camera adaptation.

A first normal hit changes HP2→129 transiently, then1; motion stays−384.
The second normal hit changes the phase byte at object+1 from2 to4, while
HP remains1. This is a damage reaction, not an absorbed invulnerable hit.
The following43 moving updates add5 to VX each time; then movement stops,
a three-update fuse starts, and the slot is removed. An additional hit in
phase4 sets HP0 and a one-update fuse. Both charged kinds remain active:
first they switch the armored roller to phase4/HP1, then kill its body on
the next overlapping update. Normal-hit cleanup is deferred one update.

**OBSERVED:** controlled velocity changes in a natural braking state do not
change the fuse-start frame. VX−379,−180,0 and180 all reach the fuse after
43 updates from the captured state. Thus a speed threshold is not the
accepted rule; the finite animation/death sequence determines that lifetime.
`collision_reference.py` records these probes separately from natural play.

**OBSERVED:** object+20 points into bank86 to four-byte hitbox descriptors:
signed X/Y offsets, unsigned half-width/height. The player rectangle at
86A552 is (0,−1,6,14); armored roller86CA2B=(0,0,12,13), braking body86CA35
=(0,9,13,11). Controlled axial probes validate the descriptor interpretation,
facing inversion and combined ranges for all three shot kinds/directions.
For example, a normal shot versus armor has radii16/17; a traveling medium
has radii34/25 with a horizontal offset. Large-shot pointers alternate
86BEA4/86BEA8 with a six-update cycle, so its rectangle changes with pose.
Contact uses radii18/27 for armor and19/25 for the lower braking body.

**TARGET:** 14 four-byte descriptors, two256-byte charged age profiles, a
constant normal box; no division or generic collision decoding in the loop.
A native43-update brake counter reproduces the measured finite sequence.
The four-update enemy flash and24-update explosion playback are visibility
adaptations; no enemy invulnerability is imposed. Source broken-roller art
adds nine poses and their mirrors:212 frames,39768-byte bank. Their42 original
video frames validate18773 visible pixels within2 RGB units; ownership checks
also verify340 pixels covered by the hero. Including prior actor checks:
151 projectile/enemy/effect frames,64999 pixels;292 hero frames,146886 pixels.

**OBSERVED:** stop+fire retains the preceding RUN muzzle X+27/Y−4 for one
update even though the player state becomes idle. The new isolated normal,
medium and large stop cases add167 projectile observations; total656 on PC
and the compiled TI. The next idle tap returns to X+16/Y−3. Medium natural
release has a one-source-pixel running bob difference in our flattened muzzle;
all its checked enemy outcomes remain exact. Existing movement and stationary
projectile oracles are unchanged.

## Verified charge through contact and recovery input

**OBSERVED:** `damage_reference.py` records twelve natural timelines from
the existing first-hit state or the preceding roller approach. No RAM writes
are used in these cases. 1320 hero samples (X/Y, fractions, VX/VY, state/HP)
and188 source-active projectile samples match portable C and the compiled
68000. Charge tiers and every projectile birth are checked, including no-shot
updates: absence is an outcome, not just an omitted trajectory comparison.

Holding Y during recoil builds charge normally. A charge started before
contact also survives the hit. The recoil lasts through source relative
update29; update30 returns idle, while firing is still suppressed. A new Y
edge on update30 is consumed without emitting a pellet; update31 permits
normal firing or a31-update intermediate-charge release. Releasing a charged
hold while still recoiling clears the charge without creating a projectile.
Raw charge countdowns take an extra decrement at the intermediate-tier
transition, so comparisons use measured tiers/releases rather than treating
7E0BFF as our held-update counter.

Holding jump through recoil does not launch on recovery. Release/repress
afterwards does; held movement restarts with the normal startup delay. The
natural airborne-contact case also checks recoil before landing. These
checks cover controllable movement/firing during the remaining immunity.

**OBSERVED:** the large projectile's formation VX is free-slot residue,
not always1536. Without an earlier pellet (charge started during recoil),
it is zero. Twelve independent inactive-slot probes vary VX−1536/0/123/1536
and XS0/64/192: formation preserves both through age5, then selects VX2048
from the launch side at age6. Earlier leftward captures retaining positive
VX1536 were one reuse history, not a universal initialization rule.

Player-pose profiles add300 original updates over running jump, wall kick
and airborne fire. All retain86A552=(0,−1,6,14), also observed throughout
the twelve damage cases. The native constant player rectangle therefore
matches these studied actions; this is not a claim about other equipment.

**TARGET:** retain charge through a nonfatal hit, suppress shot emission
using the preceding HURT state, consume input edges normally, and retain
large-shot slot VX until launch. Charged travel ignores XS: fresh slots
use0, reused native slots use canonical64 because target viewport removal
changes the old pellet's unused carry. Existing72 stationary samples still
match exactly; the188 new trajectories compare normal XS exactly and omit
unused charged XS explicitly. Native state export includes that chosen value.

## Verified projectile capacity and charge re-press

**OBSERVED:** `capacity_reference.py` measures rapid one-update taps and
three normal taps followed by31/101-update holds. The original permits
at most three active player projectiles despite its eight object slots.
The short rapid-allocation prefix ends before the first source viewport
removal/reuse at update25; all25 updates and69 projectile samples are checked.
The medium and large cases retain their complete75/145-update timelines:
109/100 source-active projectiles, including exact stationary fractions.
All245 updates' charge tiers and projectile births, plus278 trajectories,
pass portable C and the actual68000. Extra native projectile lifetime after
source removal remains the documented viewport adaptation.

**OBSERVED, controlled:** keep existing projectile current/previous X at200
before each update, without changing hero/input/charge/motion. Three pellets
remain allocated through the31- or101-update release. Both charged releases
are refused and charge clears; no fourth actor or deferred launch appears.
These anchored probes are separate from the natural-input trajectories.

**TARGET:** the native three-slot pool is now confirmed by the studied
original starting buster. A full-slot release consumes charge without
allocating another shot. The dense isolated scenario108 confirms that rule
on PC and the compiled TI. Later rapid-fire availability follows native
viewport removal rather than pretending the two view widths are equal.

## Verified wall muzzle phases, deferred birth and reused fractions

**OBSERVED:** `wall_charge_reference.py` records 21 forty-update cases:
normal/medium/large emission at falling contact, grip, late slide, kick input,
windup, launch transition and established launch. Source doors use 480 natural
RIGHT updates with Y held for the final 0/31/101; only enemies are cleared.
All three doors have no active player projectiles. Their inactive slots retain
source history; no player, camera or timer writes prepare these timelines.
840 hero states and 531 source-active projectile samples pass portable C and
the compiled 68000, including charge presence/tier and suppressed births.

The 91 adjacent/repeated normal-fire probes establish the pose sequence,
including an earlier shot at update 3 and wall jump at update 10. Selected
birth offsets relative to the post-update hero are:

| Source relative update / pose | X offset | Y offset | Direction |
| --- | --- | --- | --- |
| 0, falling into grip | +26 | -6 | Toward right wall |
| 1..5, initial grip | +19 | -9 | Toward right wall |
| 6, turn begins | -17 | -9 | Away |
| 7..12, turned grip/slide | -14 | -5 | Away |
| 13..18, late slide | -18 | -2 | Away |
| 19..20, descending | -19 | -2 | Away |
| 10, jump input edge | -15 | -5 | Away |
| 11..14, kick windup | +15 | -5 | Toward wall |
| 15, forced launch transition | No immediate shot | | |
| 16..23, launched kick | +18 | -9 | Toward wall |

These are observed post-update offsets, not a universal position table.
Fifteen independent controlled probes vary grip VX 0/128/256/376/512 and
hero fraction 0/64/192. Hero final X stays clamped at 824, while projectile
X is 824 + ((XS + VX) >> 8) + 18. The source muzzle uses intended horizontal
movement before wall clamping. The falling transition's Y is current Y-6;
the earlier previous-Y interpretation was a coincidence. Local ROM table
86BE39, indexed by projectile+3C, confirms raw offsets (25,-6), (18,-9),
(15,-5), (19,-2), (15,-5), (18,-9) for the studied pose indices. Table
inspection is a reading aid; trajectories and independent probes establish
behavior. Shooting does not restart the underlying grip pose progression.

At launch transition 15 a normal press emits no pellet, but starts charge.
A charged release clears charge on update 15 and emits on update 16. Existing
projectiles still update. This distinguishes a deferred actor birth from
skipping the complete weapon handler. During the forced-away kick, movement
is leftward while the gun and newly emitted shot point toward the right wall.

**OBSERVED, controlled:** three inactive normal-slot fraction probes (0/64/192)
produce 24 exact trajectory samples. A new normal shot retains XS, and that
fraction participates in subsequent motion. Charged XS remains unused and
canonicalized as previously disclosed. In two repeated muzzle probes (88/90),
source culling frees slot 0 with XS192, but the wider native view retains that
pellet and uses fresh slot 1 with XS0. Birth X/Y/VX remain exact; only this
proved allocation-history difference is excluded from normal XS equality.
`x/wall-ti.json` identifies both cases explicitly. Other normal fractions
remain exact. The original stationary 72-sample oracle still passes.

**TARGET:** keep wall/air pose age and deferred release in explicit state;
use within-step intended X for the muzzle, and preserve normal slot fraction.
Dedicated windup, push and launched-firing ROM spans avoid replaying windup
while airborne. They add 21 source-video frames / 11387 visible hero pixels;
313 hero frames / 158073 pixels now pass, maximum RGB error 2. The art bank
contains 222 mirrored poses / 42216 bytes. Nine native replay cases add 500
complete states and LCDs: total 4759 explicit 71-word states/screens, peak
184810 cycles. Fourteen smooth preview clips contain 1205 consecutive rendered
updates / 40.01 seconds. The TI program is 15957 bytes. No hardware run is
claimed; the user's existing TiEmu session remains untouched.

## Whole-path verification and boundary input order

**OBSERVED:** `traversal_reference.py` restores the normal source spawn and
records two paths through X1008/Y335/HP16: charge before activation, defeat
the naturally spawned roller, then either jump the gap or run off and recover
with a wall kick. Clear only non-roller enemy active bytes; preserve hero,
roller, player shots, camera, collision and timing. The source takes604/636
displayed frames,1238 effective logic updates combined. Byte7E0B9B advances
once per observed update, wrapping naturally. At source frame226 it does
not advance and all independently sampled hero/charge/projectile/roller
fields remain unchanged. No input edge occurs there. Effective input scripts
remove that single lag update per route, preserving all original transitions.

1238 hero/roller states, charge tiers and every shot birth/suppression pass
PC and compiled68000.116 source-active projectile samples pass with a
disclosed one-source-pixel running-muzzle Y difference for the large release;
normal XS is exact and unused charged XS remains canonicalized. Native
gameplay keeps its own stable cadence, without the source streaming lag.

**OBSERVED, control:** the same first path with no writes spawns an additional
kind41(decimal) at source frame309, X577. It hits X at392, X698, HP16→13,
changing later jump timing. The short port models the selected roller only;
this actor and later kind15(decimal) actors are omitted. Do not describe the
isolated successful path as a complete unmodified original-stage replay.

**OBSERVED:** a natural last-ledge door is X807/Y367/XS160/VX376/RUN with
support already lost. Pressing jump immediately still launches; waiting one
update enters FALL at unchanged X807, then a new jump no longer launches.
Two24-update input timelines verify this priority. Seven-pixel support probes
leave RUN for one unsupported update; FALL initialization does not integrate
horizontal motion. This fixes the earlier premature fall.

Sixteen independent pre-contact X/XS probes use a naturally prepared falling
state atY383/VX376/VY−704. Vary X821..824 and XS0/64/128/200 only. Contact
flag is set iff intended X+7 reaches832, while the outer X+8 clamps to824.
For example, X823/XS0 clamps to824 with no side flag; X823/XS200 also clamps
to824 but has a side flag. Using clamped position, prior position or one
width for both tests incorrectly starts the wall slide. Existing lower-wall
and isolated kick oracles still pass. The48 ledge and16 clamp samples pass
PC/TI, together with all prior mechanics.

**TARGET:** delayed fall, jump priority and intended-X inner contact sensor.
Scenario25 restores the last unsupported RUN door;26–41 restore the16
first-wall inputs. An end-to-end native wall-recovery replay and edge cases
add516 complete states/LCDs. Total4759 explicit71-word native states/screens
agree; peak184810 cycles and average129699 against360000. Program15957 bytes,
art42216, map17424. Rebuilt40.01-second smooth GIF and three-file ZIP match
the current build. Source evidence: ignored `traverse/reference.json`;
compiled comparisons: `x/traversal-ti.json`.

## Direction release together with jump and support loss

**OBSERVED:** four additional24-update last-ledge input cases cover stopping,
stop+jump, reversing, and reverse+jump. A120-update natural run→stationary
jump case starts at the normal spawn, runs40 updates, then presses jump
while releasing direction. All216 added states pass PC/TI (280 edge samples
including the previous48 ledge/16 clamp probes). A fresh jump has priority
over support loss; support loss has priority over stopping. On the FALL
initialization update, position/velocity/facing remain unchanged; the next
update applies new steering. On RUN→RISE, zero direction immediately sets
VX0 and integrates VY1299, while retaining facing.

**TARGET:** reorder jump/fall/stop checks. This fixes a suspended hero after
stopping on the first unsupported RUN update, and a lost stationary jump
when releasing the run direction. Outside wall poses, facing is checked
against source flags. Wall-bank orientation deliberately stays canonical
to the wall side; source outward pose/gun direction is derived from wallage
and separately verified by muzzle and ROM art oracles. Do not equate that
native bank-orientation field with the source facing byte.

230 added native replay frames bring complete PC/TI states and LCDs to4759,
peak184810 cycles, average129699. Program15957 bytes. The rebuilt smooth GIF
adds a stationary jump after running:14 clips,1205 updates,40.01s.

## RUN-departure cannon origin and inactive slot history

**OBSERVED:** seven additional original cases cover normal/medium/large
run+jump emission, normal/stopped firing on support loss, and both charged
releases on support loss. Seven cases add204 source-active projectile samples,
bringing coupled observations to860. The compiled68000 also checks434 exact
hero motion/HP states, charge presence/tier and every shot birth/suppression.
Existing movement, stationary, combat, capacity and wall oracles still pass.

Leaving RUN for RISE or initial FALL keeps muzzle X+27 on that update. Normal
run+jump probes at10/16/20/28/35/40 give Y offsets -5/-4/-5/-4/-6/-4. Normal
last-ledge fall/jump gives -6. At run+jump40, both charged tiers give -4.
World-position-triggered holds fromX758/X500 prepare medium/large ledge doors
without player/camera/timer writes; only enemy active bytes are cleared.
At support loss, medium uses Y-6 while large uses Y-4. After the transition,
ordinary airborne muzzle profiles apply. Motion state alone does not define
the immediate cannon pose.

**TARGET:** preserve transitional X+27; normal/medium Y-5 flattens the observed
bob within one original pixel, while large Y-4 is exact in these cases. Only
new boundary cases permit charged Y tolerance1; the earlier exact charged
oracles retain their original bounds. Compare all source-active shots and
normal XS exactly; unused charged XS stays explicitly canonicalized.

The natural normal ledge door retains inactive XS128/VX2048 from its earlier
large shot. Scenario25 now copies this history because a newly allocated
normal shot uses XS. Charged ledge scenarios43/44 use measured cached VX1536
and held RIGHT/Y input history. An unused field during charged travel can
become meaningful for another kind after reuse; scenario setup must account
for that history. No active projectile is invented in these doors.

410 added native replay states/LCDs bring current acceptance to4759 complete
71-word states/screens, peak184810 and average129699 cycles (budget360000).
Program15957 bytes. The final smooth preview has14 clips,1205 updates,40.01s,
including run+jump large-charge release. Source: ignored combined/reference.json;
compiled report: x/combined-ti.json. All art/pixel checks remain passing.

## Charge presentation follow-up

TARGET: replace the blinking square with masked energy sparks which converge
on the cannon. Medium charge uses two particles, full charge four, over sixteen
precomputed distance steps. The cannon core and armor shade cycling distinguish
the tiers; swapping/selecting plane pointers preserves the original silhouette
and white mask outline. This is native LCD art, not an extracted source effect.

OBSERVED (compiled native target): all 4759 complete 71-word states and LCD
frames match PC/TI. Peak update/render is 185570 datasheet cycles, average
130048, budget 360000. Program 16297 bytes. Fifteen preview clips contain
1300 consecutive rendered frames over 43.16 seconds at 512/17 Hz.
