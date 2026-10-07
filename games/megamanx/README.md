# Mega Man X: opening Highway section

Portable C for the TI-89/Titanium and PC, using the local USA SNES ROM as a
measured reference. X runs, fires, holds/releases two charge tiers, jumps,
slides down walls and wall-jumps. The selected enemy is the first spiked roller; other source enemies are
outside this prototype. It moves, takes hits,
brakes after two small hits, explodes and deals contact damage; X recoils,
blinks, loses HP and can retry. Both charged tiers pass through armor/body
and finish the roller over consecutive updates.
Original scenery, X poses and projectile/enemy animations use four greys
and white outlines. Initial equipment only: no dash or armor upgrades.

The working scope follows the user's final “small part” clause: X128 to
X1008, including the first gap at 800..831 and the raised road. This is 880
original playable pixels, drawn at half scale. The prompt also mentioned
the complete introduction; the optional scope/control questions received no
answer, so these choices remain disclosed assumptions. Bosses, Zero/Vile,
sound and later stages are outside this opening section. Decisions and
measured rules: [ROADMAP](ROADMAP.md), [RE_NOTES](RE_NOTES.md).

| TI key | PC runtime key | Action |
| --- | --- | --- |
| Arrows | Arrows | Run / steer in the air |
| 2nd | Ctrl, Space or Z | Jump; press again against a wall to wall-jump |
| Shift | Shift or X | Fire; hold to charge, release to launch |
| Enter | Enter | Restart the section |
| Esc | Esc | Quit |

In Docker TiEmu, AltGr maps to 2nd. The existing emulator keyboard belongs to
the user; this project has not restarted it or claimed hardware validation.

```sh
cd games/megamanx
make test               # regenerates missing local references/art from the ROM
make pc                 # ./mmx_pc: SDL window, or --headless
make ti                 # mmx.89z, mmxmap.89y and mmxart.89y
make original-ti        # compare actual 68000 motion/projectiles with SNES RAM
make culling-reference  # 744 restored-flight boundary probes on the original
make check              # complete PC/TI states, LCDs, per-frame cycles and ROM pixels
make preview            # x/mmx-highway-smooth.gif
make package            # x/mmx-ti.zip: all three calculator files and hashes
./mmx_pc --headless --keys keys/traverse.txt --frames 340 --shot x/finish.png
```

`tools/snes` must have its pinned core built; the local ROM name/hash is
checked by `tools/reference.py`. `make reference`, `make measure` and
`make combat-reference` force fresh original captures when desired. `make
art` rebuilds changed art dependencies and both TI banks. Python uses the
project's numpy/Pillow environment. Generated references, pixels and third
party reading aids stay local; a checkout needs its own ROM to regenerate.

For a clean Titanium load when the emulator is available:

```sh
../../tools/bin/ti-run mmx.89z mmxmap.89y mmxart.89y
```

On a calculator, archive both data variables and run `mmx()`. The NOSTUB
program remains below the TI-89's 24576-byte program limit. The three required
files are built locally; ROM pixels are not committed.

| Scenario (`mmx(N)` / `--scenario N`) | State |
| --- | --- |
| 0 | Normal opening |
| 1 | Opening with enemy interaction disabled |
| 2 | First right-wall slide/kick probe |
| 3 | Approach to the road gap |
| 4 | Raised right road |
| 5 | Roller and small-shot combat |
| 6 | Charged shot ready opposite the roller |
| 7 | Contact recoil/recovery |
| 8 | Three-projectile, roller and explosion density |
| 9 | Death/retry |
| 10–15 | Normal/medium/large shot at the left, then right removal edge |
| 16 | Original natural roller state; stop/fire or jump/turn/fire |
| 17 | Same natural state with large charge ready |
| 18 | Natural medium-charge release state |
| 19–20 | Medium/large charge at the first wall |
| 21–22 | Reused normal-shot fraction probes |
| 23–24 | Large/medium release at the wall-kick launch transition |
| 25 | Last unsupported RUN update at the gap |
| 26–41 | First-wall X/fraction probes |
| 43–44 | Medium/large release at the last unsupported RUN update |

Add 100 for isolated reference variants. PC state files use the PC build's
own ABI; PC/TI comparisons export 71 explicit 16-bit fields rather than copy
raw structs across machines.

The checked run matches 1230 original movement/recoil states and 72 projectile
states on PC and the actual 68000 binary. Another 860 source-active shot
samples cover running, jump, direction reversal and wall firing.
4759 complete native PC/TI states and
LCD frames match; the measured update+render peak is 185570 cycles against
a 360000-cycle budget. Rendering is 512/17 Hz, two logic updates each frame;
the nominal logic rate is about 0.23% faster than this NTSC ROM. See local
`x/checks.json` and `x/original-ti.json`. Original hero pixels are independently
checked over 313 frames (158073 visible opaque pixels), including firing,
wall actions and hurt. Another 151 projectile/enemy/effect frames cover
64999 pixels, including the broken roller. Maximum RGB error is 2.

Charge now has converging masked sparks, a pulsing cannon core and armor shade
cycling. Full charge adds more particles and a faster pulse. The animation
is table-driven and preserves the actor outline; it is a native LCD adaptation.

The smooth GIF contains 1300 consecutive LCD updates over 43.16 seconds: running
and charged defeat, gap jump, wall jumps, two-pellet defeat, charge release
and damage/recovery, plus running charge, airborne fire, wall shooting,
stop/fire and jumping past the roller to shoot from behind.
The two charged wall clips show shooting away during slide and toward the
wall during a kick, while X moves in the opposite direction.
It is a native headless preview, with every rendered
update included and no artificial chapter delays.

Animation timing, camera framing, target-width projectile culling, charge
aura, contact blink and charge presentation are native adaptations.
The source velocities, fractional movement, short/held jump transitions,
wall-kick sequence and recoil timelines have explicit original tests.
The backdrop's parallax/priority is flattened offline. No new 68000 assembly
or CPU emulator runs on the calculator. Titanium hardware validation is the
remaining handover check.

Coupled shots match source X exactly; normal running-shot bob is flattened
within one original pixel in Y. Charged formation follows X but keeps its
launch Y and direction. Unused running charged fractions and source viewport
removal remain camera adaptations. See `x/combined-ti.json`.

The source roller's second small hit starts 43 moving brake updates, followed
by a three-update explosion fuse. This replaces the initial enemy-immunity
guess. A further hit during braking shortens the fuse to one update. Original
bank-86 hitbox descriptors and age profiles reproduce changing charged-shot
rectangles with two 256-byte lookup tables. `make original-ti` also checks
1040 natural enemy observations (position, fraction, velocity, HP, phase,
fuse, explicit death and player HP). The medium-release oracle has a one
original pixel running-muzzle Y adaptation; its enemy outcomes are exact.

Charging continues through contact and recoil. Release during recoil clears
the charge without firing; wait until recovery to launch. Held movement
resumes after recovery, while jumping needs a fresh press. Twelve natural
source cases add1320 exact contact/recovery states and188 projectile samples
on PC and the compiled TI, including suppressed shot births and charge tiers.
These reports are in `x/damage-ti.json`. Player jump/wall/fire pose profiles
keep the original standing hitbox over300 measured source updates. Twelve
independent inactive-slot probes confirm that large formation VX is residue,
not a constant; the launch side determines traveling VX. Charged fractions
are unused and canonicalized for the wider native viewport.

The original starting buster's three-projectile limit is confirmed. Charge
can start after rapid taps; release with three occupied slots clears charge
without firing. Three original cases add245 updates and278 checked projectile
samples (`x/capacity-ti.json`). The rapid-fire comparison ends before the
first source cull/reuse, since the wider native view changes later capacity.

Wall firing adds 21 original timelines: 840 hero states, 531 projectile
samples and 91 neighboring/repeated muzzle probes, checked on PC and compiled
TI. Normal fire at the kick-launch transition starts charging without a
pellet; a charged release consumes charge immediately and emits next update.
Wall pose age controls the gun independently of movement direction and
firing animation. Three controlled reuse histories add 24 normal-fraction
samples. Two repeated-shot probes have explicitly reported fractional carry
differences because the wider native view retains the preceding pellet.
The dedicated kick firing poses add 21 checked source frames; current banks
contain 222 mirrored poses / 42216 art bytes and 17424 terrain/image bytes.
The calculator program is 16297 bytes. See `x/wall-ti.json`.

Two complete source-aligned routes add 1238 hero/roller states and 116 shot
samples, including charge before the natural spawn, defeat, gap traversal and
wall-jump recovery. They clear only non-roller enemy active bytes. A natural
no-write control records the additional kind41 actor and its three-HP hit;
this actor is omitted in the prototype. Source frame226 is a verified skipped
logic update; input edges are aligned to effective updates for the comparison
without inserting that lag into native gameplay. Another 64 ledge/clamp
samples verify delayed falling, the last allowed jump and the seven-pixel
contact sensor inside the eight-pixel wall clamp. Source-active running
shots of every tier have at most one original pixel of flattened muzzle Y
bob. See `x/traversal-ti.json` and `keys/traverse-wall.txt`.

RUN→jump/fall firing adds seven cases / 204 projectile observations (860
coupled samples total) and 434 exact hero/charge/birth states on compiled TI.
The cannon briefly retains the RUN X offset on these transitions. Medium
and normal muzzle Y bob is flattened within one original pixel; the large
release uses its separately measured pose. The final GIF also shows charging
while standing, running, then releasing during jump. All earlier oracles pass.
