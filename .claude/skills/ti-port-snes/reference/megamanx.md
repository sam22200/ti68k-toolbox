# Mega Man X USA: second verified SNES experiment

Use for a Mega Man X study or a comparable platformer. These offsets and
timelines belong to this local revision, not to arbitrary SNES games.
The short opening section and half scale are this project's working choices,
following the user's final short-section clause; they are not skill defaults.
Full findings and executable probes: `games/megamanx/RE_NOTES.md` and `tools/`.

ROM: `roms/snes/Mega Man X (USA).sfc`,1572864 bytes, no copier header,
SHA256 `3e1209f473bff8cd4bcbf71d071e7f8df17a2d564e9a5c4c427ee8198cebb615`.
Pinned Snes9x `fae2fea08f74180759ef540ee94259213f503480` reports 60.098812 Hz.
`keys/ref_boot.txt` cold-boots2200 frames to Highway0, X128/Y367,
camera0/256 without RAM injections.100 restored WRAM/PPU/video frames agree.
This cartridge has no exported SRAM; an absent SRAM region is valid.

## Physical measurements

Player X/Y7E0BAD/7E0BB0; fractions7E0BAC/7E0BAF; signed8.8 speeds
7E0BC2/7E0BC4; states 7E0BAA/7E0BAB; HP7E0BCF. Camera7E1E4D/7E1E50.
Original B maps to target2nd, original Y to target Shift. Movement cases clear
enemy/projectile primary states only; contact cases first run naturally.

Run376/256 pixels/tick; jump1363/256; gravity64/256 subtracted before
integration, positive VY upwards. Ending the jump hold resets falling speed.
Ground rest is floor−17; preserve fractions and transition-frame velocities.
Wall kick pauses before launch, forces seven away steps, then resumes input.
Contact recoil is−138/256 horizontally and 512/256 initially upwards. The
native contact immunity uses the observed92-update repeat interval.

Source logic and drawn-frame cadence are different questions. Unmodified
running through a streaming/actor activation point includes occasional lag;
do not turn it into a universal movement speed correction. Source equipment
also matters: initial X can wall-jump and charge, but has no dash or armor.

Charge release thresholds31 and 101 were checked on both adjacent sides.
The pellet adds64 before integration even after its stored speed saturates
at 1536; cruise movement is 6.25 pixels, not6. Medium charge formation lasts
nine updates before travel; large formation lasts six, then uses 2048/256.
Check complete projectile timelines, not just initial speed or a screenshot.

## Coupled-action probes

Standalone motion and stationary weapons do not establish combined behavior.
Fourteen original run/jump/wall/leftward/stop cases add 656 source-active projectile samples
on PC and the compiled TI engine. Clear enemies without clearing the player
shots; preserve the player's native state transitions and camera timing.

Charge formation follows horizontal motion while retaining launch Y and side,
even during a jump or direction reversal. Medium follows X through age9;
large follows through age5, sets flight VX without moving at age6, then flies.
The muzzle is pose dependent: standing +16/−3, running +27/−4, rising +25/−8;
early right-wall +19/−9 changes to a leftward −18/−2 after the turn. Compare
both the trajectory and the action state at release, including simultaneous
jump/release and release after the jump is established.

The native engine flattens running normal-shot bob within one source pixel
in Y; all coupled X and other sampled Y are exact. Unused running charged
fractions are excluded because wider-camera slot reuse differs. Compare all
source-active trajectories and disclose extra target shots after source
viewport removal. Keep the stationary exact oracle separately.

Leftward cases exposed two assumptions that rightward tests could not:
the sampled large formation retains positive VX1536 while aimed left, switching to −2048
at age6; retain launch side separately for motion and mirrored art. Source
shots remain active below X0 (unsigned X wraps). 744 restored-flight
probes change only projectile X, in both directions and all three kinds.
Normal removal uses the integrated coordinate within [−32,288). Charged
removal reacts one frame later to that coordinate; its first-frame cached
visibility is unchanged by the poke. A previous-position native model with
32-pixel grace matches natural charged flight, and normal culling follows
integration. Native drawing/removal uses signed coordinates and extends the
right bound to 352 for the wider camera. Do not infer an exact bound or
removal phase from a sparse natural trajectory alone.


## ROM layout and render anchors

Level pointers come from 868D24/868D93/868E02/868E71/868EE0. For stage 0,
layout928140→32×4 scene IDs (RLE), scenes92E180→8×8 block words,
blocks96C428→four map IDs, maps998000→four PPU attributes, collision95E1E1
→a byte per map. Layout and collision map IDs were compared with the
original7EE800 and 7E2000 caches before using the ROM hierarchy.

The shared `tools/snes/ppu.py` decodes planar tiles/OAM and Mode1 background
maps. Mode1 includes three palette/flip-aware layers; support for another
background mode must be added deliberately. Source scroll values still come
from measured game memory, not raw register-mirror guesses.

Pre/post snapshot alignment is essential. The first newly created pellet
exists at RAM X144 but has no OAM yet; the next sample has RAM X148 while
its OAM remains centered144. Hero jumps likewise use pre-update screen
positions. Save an anchor before `retro_run`, extract PPU afterward, and
compare opaque pixels against the original video. Using only post-frame RAM
creates shifted poses and fake duplicates. Correcting anchors reduced this
bank from 226 to 194 poses and the pellet to one canonical sprite.85 original
hero frames /40255 pixels match within2 RGB units (RGB565 conversion).

Charge-start and traveling sprite spans are kept distinct. Cycling every
captured pose after launch would replay the muzzle expansion in mid-air.
Deduplicate assets by semantic pose plus correct anchor, rather than by an
arbitrary sequence of unmatched memory/video snapshots.

## Original video checks and OBJ ownership

The expanded art check validates 313 hero frames /158073 visible pixels,
including firing, wall slide/kick/fire and hurt. Another 151 projectile,
roller, broken-body and explosion frames cover 64999 pixels. Maximum RGB error remains 2.
The original 85 movement frames /40255 pixels remain a separate report group.

Gun flashes, wall dust and charge sparks use other OBJ palettes and can
cover X. Resolve the covering OAM index before comparing an isolated hero
with video. Every excluded pixel must match its covering OBJ in the source
video; do not mask unexplained errors just because a pose overlaps another
actor. Extraction fidelity does not imply exact native animation cadence
or SNES layer priority after reduction to the TI display.

## Enemy damage: state changes beyond HP

Seven natural roller timelines add1040 observations checked on PC and the
compiled68000: enemy position/fraction/VX/HP/phase/fuse, explicit removal and
player HP. First normal hit reduces2→1 (bit7 is a transient flag). The second
changes phase2→4 without reducing HP:43 moving updates add5 to VX, then a
three-update fuse destroys the roller. An extra hit shortens the fuse to one
update. Both charged tiers persist and hit armor/body on consecutive updates.
The initial20-update immunity interpretation is removed. Unchanged HP alone
does not prove that a shot was absorbed; capture the whole actor/substate.

Velocity pokes−379,−180,0,+180 leave fuse timing unchanged. Do not infer a
speed threshold just because one natural timeline always ends at VX−169.
The native model uses the measured finite43-update sequence.

Object+20 points to bank86 hitbox bytes: signed offsets and unsigned half
sizes. Player86A552=(0,−1,6,14), armor86CA2B=(0,0,12,13), body86CA35=
(0,9,13,11). Controlled axis probes validate facing inversion and shot
ranges. Large poses alternate BEA4/BEA8 every six updates after formation.
Two256-byte age profiles plus14 four-byte descriptors replace runtime
modulo/ROM decoding. A constant normal box avoids an unnecessary table read.

Stop+fire also retains the prior RUN muzzle for one update;167 new samples
bring coupled comparisons to656. A medium natural release has a disclosed
one-source-pixel flattened running-muzzle Y difference; all its tested enemy
outcomes are exact. Native blink/effect cadence remains an adaptation.
Nine broken-roller poses and mirrors grow the art bank to212 frames/39768
bytes. Another42 source frames validate18773 body pixels and340 covered
pixels independently against video.

## Contact while charging and recovery input

Twelve natural cases add1320 hero states and188 projectile observations on
PC and the compiled68000: positions, fractions, velocities, state/HP, charge
tiers, all projectile births (including suppressed presses) and source-active
trajectories. Charge survives contact and continues during recoil. Recoil
relative update30 returns idle but still blocks firing; update31 permits it.
A press on update30 is consumed, with no deferred pellet. Release while
recoiling clears charge without firing. Holding jump through recoil does not
launch on recovery; release/repress does. Held movement resumes normally.

Raw charge countdown takes an extra decrement at the intermediate transition.
Use measured tiers and release outcomes, not an assumed one-to-one mapping
from the source countdown to a native held-update counter.300 pose samples
retain86A552 for run/jump/wall/fire; damage cases keep the same rectangle.

Formation VX1536 was slot residue in the first captures. Starting charge
during recoil leaves a fresh slot and large formation VX0. Twelve independent
inactive-slot probes vary VX−1536/0/123/1536 and XS0/64/192: both are retained
through age5; age6 selects traveling VX from launch side. The native engine
keeps large formation VX, while unused charged XS is canonical0 for fresh
slots or64 for reused slots because target culling changes pellet carry.
The stationary72-sample exact oracle remains intact; new charge trajectories
exclude unused XS explicitly and check normal-shot XS exactly.

## Projectile capacity and failed releases

Eight player-object slots do not mean eight allowed buster shots. The
starting buster admits three active projectiles. Rapid taps contribute an
exact25-update/69-projectile allocation prefix before source viewport
removal/reuse changes later availability. Three taps followed by medium or
large charge retain complete75/145-update timelines (109/100 projectiles).
All245 updates' charge tiers/births and278 projectile samples pass PC/TI;
stationary charged fractions here are exact. Later rapid-fire availability
is explicitly a viewport adaptation rather than a weakened allocation test.

Controlled probes hold only projectile current/previous X at200. Keeping
three pellets alive through31/101-update release refuses the charged shot
and clears charge. Dense native isolated scenario108 checks the same failed
release on PC and compiled TI; no fourth projectile or deferred shot appears.

## Verified wall muzzle phases, deferred birth and reused fractions

`wall_charge_reference.py` records 21 forty-update cases:
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

Controlled: three inactive normal-slot fraction probes (0/64/192)
produce 24 exact trajectory samples. A new normal shot retains XS, and that
fraction participates in subsequent motion. Charged XS remains unused and
canonicalized as previously disclosed. In two repeated muzzle probes (88/90),
source culling frees slot 0 with XS192, but the wider native view retains that
pellet and uses fresh slot 1 with XS0. Birth X/Y/VX remain exact; only this
proved allocation-history difference is excluded from normal XS equality.
`x/wall-ti.json` identifies both cases explicitly. Other normal fractions
remain exact. The original stationary 72-sample oracle still passes.

Target: keep wall/air pose age and deferred release in explicit state;
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

`traversal_reference.py` restores the normal source spawn and
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

Natural control: the same first path with no writes spawns an additional
kind41(decimal) at source frame309, X577. It hits X at392, X698, HP16→13,
changing later jump timing. The short port models the selected roller only;
this actor and later kind15(decimal) actors are omitted. Do not describe the
isolated successful path as a complete unmodified original-stage replay.

a natural last-ledge door is X807/Y367/XS160/VX376/RUN with
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

Target: delayed fall, jump priority and intended-X inner contact sensor.
Scenario25 restores the last unsupported RUN door;26–41 restore the16
first-wall inputs. An end-to-end native wall-recovery replay and edge cases
add516 complete states/LCDs. Total4759 explicit71-word native states/screens
agree; peak184810 cycles and average129699 against360000. Program15957 bytes,
art42216, map17424. Rebuilt40.01-second smooth GIF and three-file ZIP match
the current build. Source evidence: ignored `traverse/reference.json`;
compiled comparisons: `x/traversal-ti.json`.


## Direction release together with jump and support loss

four additional24-update last-ledge input cases cover stopping,
stop+jump, reversing, and reverse+jump. A120-update natural run→stationary
jump case starts at the normal spawn, runs40 updates, then presses jump
while releasing direction. All216 added states pass PC/TI (280 edge samples
including the previous48 ledge/16 clamp probes). A fresh jump has priority
over support loss; support loss has priority over stopping. On the FALL
initialization update, position/velocity/facing remain unchanged; the next
update applies new steering. On RUN→RISE, zero direction immediately sets
VX0 and integrates VY1299, while retaining facing.

Target: reorder jump/fall/stop checks. This fixes a suspended hero after
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

seven additional original cases cover normal/medium/large
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

Target: preserve transitional X+27; normal/medium Y-5 flattens the observed
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

## Native presentation and acceptance

Chosen band X0..1023,Y256..511; playable X128→1008 across gap800..831,
wall832 and raised road352. Half scale, two source ticks per render at
512/17 Hz (about 0.23% faster logic than the ROM). Static pale backdrop,
flattened parallax/priority, original foreground and outlined ROM actors.
Native camera/blink/charge aura and simplified pose timing
remain labeled adaptations. No full-stage, boss or sound claim.

`make original-ti` checks 1230 source movement/recoil and 72 projectile
states directly on the compiled 68000 engine, plus the 860 coupled samples
with the explicit adaptations above,1040 natural enemy observations and1320
contact/recovery states including188 projectile samples.
Capacity cases add245 updates and278 source-active projectile samples.
Wall cases add840 hero states,531 projectile samples,91 muzzle probes and24
controlled normal-fraction samples, with the two disclosed reuse differences.
Long paths add1238 hero/roller states,116 projectile samples and280 edge/input samples.
RUN-departure cases separately check434 hero/charge/birth states.
`make check` compares 4759
complete 71-word PC/TI states and LCDs, profiling every prefix difference
with datasheet timings; peak185570 cycles, budget360000. Native and
big-endian sprite-row banks are independently checked. Art bank 42216 bytes (222 mirrored poses),
map17424. `make preview` produces1300 consecutive rendered frames,43.16s,
without artificial holds. TiEmu has not been taken from the user; hardware
validation remains a handover check.

The charge presentation follow-up uses converging masked sprites and armor
plane-pointer shade cycling instead of a blinking square. Medium/full tiers
use two/four particles and distinct pulse rates. Original silhouette and mask
outline are retained; the effect remains a native adaptation. Rechecked peak
185570 cycles, average 130048; program 16297 bytes. The fifteen-clip preview
starts with both tiers and release; it contains every rendered update.
