# The Minish Cap: native Minish Woods

M5 is playable on the Portable Game Runtime: walking through original terrain,
cardinal/diagonal movement, partial metatile collisions, corner slides, slope
speed changes and a scrolling camera, with original scenery and animated Link
in four greys. Scope: the 720x320 original opening, drawn at **70% scale**
(504x224 displayed pixels) through the TI's 160x100 screen. Forty-four extracted poses have a white
outline; tree canopies cover Link where the original foreground does. Ordinary
sword swings now cut the 53 bush cells, changing their scenery and collision;
walking through newly cleared paths works. Forty additional poses show Link
with the sword, outlined together. The two opening Octoroks now wander, spit,
take sword hits and cause contact/projectile damage. Three hearts, short recoil,
the original knockback poses and damage flash, and a native game-over/retry
complete the encounter.
ENTER restores bushes, enemies and health. M5 adds Link's original roll
(B: Shift/X on the PC, Shift on the TI), the original leaf burst when a bush or
grass is cut, a lighter earth texture that stays readable where bushes were, and
the original fading death puff for Octoroks. Other objects, water/pit actions and
room transitions remain later work. [TiEmu playthrough](captures/tiemu/final.gif) (`keys/final.txt` from `minish()`)
(one continuous game on the Titanium), [showcase](captures/showcase.gif)
(`tools/preview.py --showcase`), [combat replay](captures/combat.gif),
[sword replay](captures/sword.gif) and [opening replay](captures/opening.gif).

Scenery and Link are about 30% smaller than the original, with about 43% more
source terrain visible along each axis. Up to M5 a second build drew the same
game at 1:1; it was retired after M5 (too zoomed in on the 160x100 LCD), so
only the 70% game is built and checked now. Figures quoted below for "100%"
are historical. Doors 64..95 inspect all horizontal camera shift phases;
96..99 inspect clipping at the slice corners. These are rendering probes, not
additional walkable entrances.

The source-space movement/collision model is shared. Graphics and silhouette
reduction run offline; the one-pixel white outline is added after shrinking
Link. The displayed world is 504x224 pixels, with 44 poses in 32x32 canvases.
The view reads packed scenery planes and restores masked canopy pixels over
the actor; a coordinate table handles the 7/10 mapping without runtime
division. Data payloads total 175202 bytes, read in place: 9984 terrain, 43008
scene/foreground planes, 18338 walking actor/table bytes, 54964 action bytes
and 48908 enemy bytes, including sixteen offline shifts for each enemy pose.
The action bank also holds deduplicated pre-shifted bush XOR patches; the
render loop needs no mutable world bitmap or additional heap buffer.

The game passes all 6310 original walking steps, 400 animation steps and
96 actor-depth steps, plus **1312000 independent oracle pixel
checks**, covering every walking pose, both canopy depths, all camera shift
phases and all four edges. M3 adds **672000 sword/changed-scenery oracle pixels
per build**, 68 original action trials and **1947 exact source steps**, covering
all 53 bush cells, holding/releasing, rapid restarts, facing changes, recovery
movement and successive cuts inside dense beds. Each original trial replays
twice with eight memory/video digests. Attack extraction matches 16819200
original RGB pixels; ground replacement additionally checks 131 exposed pixels.

M3 baseline native checks: **4639 PC/TI field hashes and 216 screens** at 100%,
**4675 hashes and 252 screens** at 70%. This includes all 40 attack poses in
two fully cleared areas and 32 camera offsets with every bush cut. Maximum
complete instrumented frames are **326918 / 331056 cycles**, respectively,
below 360000. Compiled programs are 12579 / 13245 bytes. State save/load retains
both collision and changed scenery; reset restores all bushes. No hardware
or shared runtime changes were needed, so checks remained headless.

M4 adds twenty original Octorok poses, **10137600 checked source RGB pixels**,
twenty twice-replayed combat trials and **608 updates checked for targeted
movement, contact/recoil, sword kills and ordinary rock flight**. Native
uncompressed actor/canopy expectations add **960000 oracle pixels per scale**.
The source contact boundary includes twelve pixels and excludes thirteen;
both bodies have half-extents (6,6), offset (0,-3). Contact and ordinary rocks
cost two health units (a quarter-heart); health starts at 24, with thirty source
updates of invulnerability and seven moving recoil updates at 2.5 pixels each.
The Smith sword kills a two-HP Octorok in one hit. Rocks move at 2.5 pixels per
source update. The native demo defeats both enemies with 22/24 health remaining.
Save/load also checks an active four-rock scene with every bush already cut.

Current complete PC/TI checks cover **7521 hashes / 346 screens** at 100% and
**7557 hashes / 382 screens** at 70%, including game over, retry, all enemy
poses, fully/partially clipped actors and intermediate scrolling screens.
Instrumented update/render peaks are **359248 / 348320 cycles**, below 360000.
Program payloads are 20063/20759 bytes; the five archived banks total
84956/175202 bytes, respectively.
Static HUD rows, pre-shifted rocks and enemy art, and cropped canopy restoration
keep these cases within budget. No shared runtime or hardware path changed.

M5 checks **330 original roll updates** (direction lock, per-update speed and
terrain collision) and **2352000 roll/effect oracle pixels** per scale for 49
extracted roll, leaf and death poses. Up to four leaf bursts run at once; the
oldest is recycled. Doors `(430)` roll, `(432)` kill an Octorok, `(440..455)`
four bursts at once over cleared beds, with both Octoroks dying from `(448)`,
`(512+)` every effect pose and `(700..891)` clipped effect poses. Complete PC/TI
checks: **10002 hashes / 715 screens** at 100% and **10038 hashes / 751
screens** at 70%; peaks **358736 / 344834 cycles**. The 70% view copy reads
one `long` per destination word and shifts it once (128k instead of 183k
cycles for both planes), which brought the dense effect scenes under budget.

Taking damage follows the original: Link plays its knockback animation (ten
source poses, one per facing and recoil update, doors `(408..417)`) while he is
pushed back, then stays visible during the thirty invulnerable updates. The
source pulses Link's palette in four 4-update phases; on four greys the
native body goes one step darker, black, black, then ordinary (doors `(418)`,
`(419)`), always inside its white outline. The knockback poses and the
projectile's sixteen pre-shifts live in `mifight`/`mizfight`, which keeps the
program payloads at 22873 / 23609 bytes; the six archived banks total
153636 / 246534 bytes. Ran in TiEmu (Titanium) from the six archived banks:
[TiEmu replay](captures/tiemu/showcase.gif) (`keys/tiemu.txt` from door
`(432)`: three contacts, a sword kill and four rolls).

Current state (70% only, static "MINISH WOODS" label removed from the HUD):
complete PC/TI checks cover **10920 hashes / 752 screens** (including the
882-frame `keys/final.txt` playthrough), peak
**343346 cycles** per instrumented frame; `minish.89z` is 23559 bytes and the
six archived banks total 246534 bytes.

The enemy study door adds only the original `TABIDACHI` flag before the original
room loader, separately from `woods.state`; this is controlled preparation,
not natural story progress. Native AI uses a fixed local random seed and a
smaller roaming area around the opening. At the end of a walk, an Octorok
can face nearby Link before its measured spit animation, making shots visible
without requiring its random walking direction to match. The round projectile
has a six-pixel body with a one-pixel white halo in both views; its measured
movement and hitbox remain unchanged. Hearts have a small grey highlight and
only a one-pixel white silhouette outline, with scenery visible between them.
Tile clipping, eight-way recoil,
death blinking/removal, rock glyphs and retry are native adaptations. Rocks
disappear on walls, sword contact, player contact or expiry; the original
falling/bouncing deflection, item drops and story persistence are deferred.
The ordinary flight and damage parameters above are measured separately.

Start: (248,88) in the main western alley. The earlier reference probe at
(32,88) is an isolated clearing, retained as scenario 1. A replay crosses
the main alley to the third view, near the exploration flag (692,136).
This native endpoint marker is not an original quest object.

```sh
make -C tools/gba core
make -C games/minish reference  # cold original door; generates woods.state
make -C games/minish test      # PC mechanics against original frame fixtures
make -C games/minish pc ti     # minish_pc, minish.89z and six data banks
make -C games/minish preview   # headless opening GIF (--sword/--combat/--showcase via tools/preview.py)
make -C games/minish cycles tihash  # headless TI costs, state/screen equality
cd games/minish
./minish_pc                    # arrows; SPACE/Z/Ctrl sword; ENTER resets; ESC exits
./minish_pc --scenario 140     # beside the central bush bed
./minish_pc --headless --keys keys/opening.txt --frames 400 --shot captures/opening.png
```

Send `minish.89z`, `mindat.89y`, `mizscene.89y`, `mizactor.89y`, `mizact.89y`,
`mizfight.89y` and `mizfx.89y`, archive the six banks, and run `minish()`.
`minish(140)` starts beside bushes for a quick sword demonstration.
Arrows walk, 2nd swings the sword, Shift rolls, ENTER resets, ESC exits. Scenario doors: `(1)` isolated clearing,
`(2)` first tree contact, `(3)` second view, `(4)` eastern opening, `(5)`
endpoint, `(6)` partial tree corner, `(7)` canopy occlusion and `(8)` the same
pose drawn in front for inspection. Doors `(16)` through `(59)` freeze each
of the 44 poses; directional input resumes walking. PC equivalents are
`--scenario N`;
doors 100..139 inspect 40 sword poses, 140 starts beside central bushes and
143 beside the eastern bed. Doors 141/142 and 144..255 stress fully cleared
scenery, sword poses and camera phases. Directional or sword input resumes play.
Door 9 preserves the enemy-free walking study. Door 256 starts the combat demo,
257 starts in contact, 258 tests last-heart failure, and 260..271 stress both
enemies/four rocks on cleared beds. Doors 300..359 freeze twenty enemy poses
at three depths; 360..407 inspect actor/rock clipping. Door 420 checks an
ordinary walk-to-spit transition aimed at Link, flight and projectile damage;
`tools/preview.py --shots` records it without a window.
The PC accepts the same
numbers through `--scenario N`.
PC state files use runtime F2/F3 or `--save/--load` support. No new hardware
path was introduced; this milestone was checked without TiEmu.

M2 baseline validation: 73 original trials, **6310 exact source steps**, including a
555-frame complete route. Each trial replays twice with all eight memory/
video digests. Native checks compare **3606 explicit PC/TI field hashes and
56 final screens**, including all 44 poses, canopy depth, diagonal contacts,
reset and cold TileMap draws. Conservative measured peak, including
hash/marker overhead: **296152 cycles per TI frame**, below the 360000 budget.
The opening route averages about 13k update + 123k render cycles. Program:
9793 bytes. All banks are read in place, with 56736 total payload bytes:
9984 terrain, 25632 scenery and 21120 actor bytes. No new runtime or assembly
primitives were needed.

Art extraction matches **16857600 original RGB pixels** across 439 checked
frames, including 604432 exposed map pixels and 124133 actor pixels. Four
controlled actor trials replay twice with all eight memory/video digests
matching over 420 frames. Native animation matches 400 displayed source poses;
102 pixel checks verify both canopy occlusion and drawing in front. Another
eight twice-replayed terrain trials check 96 source actor-depth updates,
distinguishing slowing surfaces that change priority from those that do not. The scene
uses 227 opaque TileMap tiles and 73 masked foreground tiles. Each of four
walking directions has ten poses lasting three source updates each; four
static idle poses complete the bank.

One source step preserves exact Q8.8 positions. The TI draws at 30.118 Hz,
performing two source steps per draw. Against mGBA's measured 59.7275 Hz,
wall-clock motion is about 0.85% faster; input is sampled once per TI draw.
The per-step rule is preserved. The 30Hz replay reaches (688,133.625), within
five pixels of the source route endpoint.

Boundary clamps and frozen source door tiles are native slice adaptations.
Original fixtures stop explicitly before leaving the slice, door transitions
or automatic non-walking actions; excluded tails and first states are recorded.
Those systems are not claimed as implemented. Original actors/managers remain
active in reference trials; the native scene renders Link and the two opening
Octoroks, rather than all original room managers and actors.
Scenery uses a static palette/animated-tile phase. Idle blinking and original
push animations are deferred; blocked movement uses the native idle pose.
The minimal study save has no cap, so the extracted Link is bareheaded. M3
equips only the Smith sword through two explicit save-byte writes; no quest or
skill flags are added. A swing lasts 15 source updates with ten poses. Movement
is locked during the swing; a fresh press restarts it and can select a new facing.
Holding A produces one swing. Three measured point samples cut bushes; these
are distinct from enemy hitboxes. Collision updates immediately, while displayed
poses and bush changes preserve the original one-update video lag. Position and
camera use the current native step. Rupees, cut particles, sound, charging
and spin attacks remain later work.

Nine streams decode offline; 56816 map/metatile/type bytes match original
loaded RAM. M1 additionally reconstructs and checks 900 collision and 900
action cells from ROM tables. `make extract` only reads the exact local USA
ROM; `make measure` regenerates walking fixtures from the reference state.
`make art` regenerates checked scenery, actor banks and art fixtures. None of
these scripts needs the decompilation checkout. Reading aids remain under
ignored `sources/minish_tmc/`.
`tools/actions.py` regenerates the action trials and banks; `--pack-only`
rebuilds banks from verified local raw poses. `tools/action_oracle.py` prepares
independent unpacked expectations. `tools/preview.py --sword` exports a
native GIF without an SDL window or emulator.
`tools/combat.py --door --measure` regenerates the separate combat study door,
reference trials and both enemy banks. `tools/combat_oracle.py` supplies the
independent pixel expectations; `tools/preview.py --combat` captures the
native fight.

ROM-derived banks, states, snapshots and fixtures stay local and ignored.
M0 outputs: `sources/minish_gba/`; native fixtures and PC/TI captures: `fixtures/`
and `captures/`. See [ROADMAP](ROADMAP.md) and [measured notes](RE_NOTES.md).
