# Resume checkpoint: Mega Man X opening section

The requested short introduction section is playable on PC and built for TI.
The optional grilling questions received no answer. Disclosed assumptions:
final short-section clause, X128→1008, half scale, arrows/2nd/Shift, initial
equipment. Preserve them unless the user steers scope or controls. Neither
this experiment nor the skill prescribes a fixed screen count.

Completed: running, short/held jumps, wall slide/kick, normal/medium/large
shots, charge/release, roller motion/hits/braking/explosion, contact HP/recoil,
blink, death/retry, outlined ROM animations and original Highway scenery.
The keyed winning replay defeats the roller and crosses the real road gap.
Milestones 14–17 pass. Wall firing: charged firing through grip, kick windup and launch,
including gun direction independent of movement and deferred charged birth.

## Current acceptance

- Unit tests and all original PC/TI oracles pass: 1230 movement/recoil states,
  72 stationary projectile states, 860 coupled projectile observations and 434 RUN-departure hero/charge/birth states,
  1040 natural roller observations, 1320 damage/recovery states with 188
  projectile samples, 245 capacity updates with 278 projectile samples.
- Wall firing: 21 cases / 840 hero states / 531 projectile samples, 91
  adjacent/repeated muzzle probes, 24 controlled normal-fraction samples.
- Long routes: 1238 hero/roller states and 116 source-active shots match
  spawn-to-endpoint movement, charge, roller defeat, gap jump and wall recovery.
  Another 280 source ledge/clamp/input samples pass on PC and compiled TI.
- Native PC/TI: 4759 complete 71-word states and LCDs agree. Peak compiled
  frame 185570 cycles against 360000; average 130048. Compiler output has
  no 32-bit multiply/divide or floating arithmetic helpers.
- Original pixels: 313 hero frames / 158073 visible pixels and 151 other
  actor/effect frames / 64999 pixels, maximum RGB error 2. Covered pixels
  are independently verified against the covering OBJ in source video.
- Art: 222 mirrored poses / 42216-byte bank; terrain/image 17424 bytes.
  Program 16297 bytes, below the TI-89 24576-byte program limit.
- Smooth preview: fifteen clips, 1300 consecutive rendered updates, 43.16s,
  512/17 Hz, no artificial holds. ZIP CRC and all three file bytes match.

```sh
cd games/megamanx
make test pc ti
make original-ti
make check
make preview package
```

Reports: `x/checks.json`, `x/original-ti.json`, `x/combined-ti.json`,
`x/enemy-ti.json`, `x/damage-ti.json`, `x/capacity-ti.json`, `x/wall-ti.json`,
`x/art-checks.json`, `x/preview.json`, `x/traversal-ti.json`. Deliverables:
`x/mmx-highway-smooth.gif`, `x/mmx-ti.zip` (three calculator files, controls,
manifest with byte lengths and SHA-256). Source captures and states stay in
ignored `sources/megamanx_snes/`; generated headers/art remain local.

## Preserve these verified distinctions

Wall shot direction follows the gun pose, not VX. Intended horizontal X
before wall clamping determines the muzzle. The falling contact birth uses
current Y-6; the earlier previous-Y hypothesis is disproved. Shooting does
not reset grip age. At the forced launch transition, normal press starts
charge without emitting; charged release consumes charge immediately and
allocates next update. Existing projectiles keep updating.

Normal slot XS survives reuse and affects movement. Charged XS is unused,
fresh/reused native slots use canonical 0/64; large formation VX is residue
until age 6 sets traveling direction. Two repeated wall probes (88/90) have
explicit XS differences: source culls/reuses slot 0 (192), wider native view
keeps it and allocates slot 1 (0). Do not relax other normal fractions.
All three source wall doors have no active shots; an earlier claim that the
medium door kept a pellet was incorrect. Inactive slot history remains.

Long-route comparison clears non-roller source enemies only. Natural no-write
control records kind41 spawning at frame309 and HP16→13 contact at392;
that actor and later kind15 actors remain outside this selected-roller port.
Byte7E0B9B advances once per observed logic update; source frame226 leaves
all measured hero/charge/projectile/roller fields unchanged. Normalize keys
to these updates for source comparisons; do not insert lag into gameplay.
RUN loses support using the seven-pixel probes, remains RUN one update,
then enters FALL without moving. A new jump wins on that last RUN update.
First wall clamp uses eight pixels; the seven-pixel contact probe uses
intended X, not prior/clamped X. Both independently sampled boundaries pass.

Keep other disclosed adaptations: one-pixel running muzzle
bob for every shot tier, wider native viewport removal/availability, flattened parallax/priority,
pose cadence, native blink/aura. Preserve all isolated exact oracles. Roller
second normal hit starts 43 moving brake updates then a three-update fuse;
extra body hit shortens it to one. Charge persists through recoil, while
firing remains blocked through its final update; jump needs release/repress.
Original hitbox descriptors use 14 four-byte entries and two 256-byte tables.

## Handover and remaining hardware check

Milestones 0–5 and 7–17 pass. Milestone 6 is deferred: the existing TiEmu
keyboard belongs to the user; no keys/restart or hardware validation was used.
A requested clean hardware run would load `mmx.89z mmxmap.89y mmxart.89y`
on Titanium to check grayscale/keyboard/interrupts. Do not seize that session.
No new 68000 ASM was added. Commit the SNES session through `ti-commit`;
preserve unrelated user modifications. `ti-port-snes` generic guidance and `reference/megamanx.md` contain
the verified findings; fixed endpoints/equipment remain project choices.

Credit usage is not exposed to this agent; no five-hour/account exhaustion
claim is made. See README/ROADMAP/RE_NOTES for scope and detailed evidence.

Milestone16 passed: release of direction at unsupported RUN enters FALL;
new jump with no direction integrates vertically immediately. Jump, support
loss and direction release are evaluated in that order. Preserving facing
on FALL initialization also matches the tested reversal. Four24-update edge
cases plus a120-update flat run/stop+jump add216 source states. All280 edge
samples pass PC/TI. Native wall facing denotes art-bank wall side; source gun
direction is derived from wallage and verified separately, not compared as
identical raw facing.4759 complete native states/LCDs, peak184810, program
15957 bytes, rebuilt14-clip40.01s/1205-update GIF and current ZIP.

Milestone17 passed: RUN→RISE/FALL emission retains X+27, not airborne X+25.
Normal/medium transitional muzzle Y is flattened to -5 (source -4..-6); large
release uses -4. Seven cases add204 source-active projectile samples and434
exact hero states, charge tiers and all births/suppressed births on compiled
TI. Total860 coupled shot samples; legacy charged tolerances are unchanged.
The last-ledge scenario copies inactive normal-slot XS128 from its earlier
large shot, so subsequent normal motion is exact; charged doors43/44 preserve
source cache VX1536. Native camera/charged fraction adaptations remain explicit.

All final headless gates pass:4759 complete71-word PC/TI states/LCDs, peak
184810, average129699; program15957 bytes;14-clip40.01s/1205-update smooth GIF
and verified ZIP. ROM art still313 hero frames/158073 pixels and151 other
actor/effect frames/64999 pixels, max RGB error2. No known required C feature
remains in the selected small section. Optional Titanium hardware handover
check remains deferred under the user's keyboard ownership; no UI run claimed.

Charge presentation follow-up passed: replace the blinking square with masked
energy sparks converging on the cannon and armor shade cycling. Full charge
has four particles and a faster pulse; medium has two. Tables and plane-pointer
selection avoid per-pixel work, division and new assembly. The added preview
clip shows both tiers and release. All 4759 complete PC/TI states/LCDs pass;
peak 185570 cycles, average 130048, program 16297 bytes. Rebuilt fifteen-clip
GIF: 1300 consecutive updates over 43.16s. The package CRC and hashes pass.
