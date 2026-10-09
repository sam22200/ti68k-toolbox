# Minish Woods reference notes

## Identity

OBSERVED: local ROM `Legend of Zelda, The - The Minish Cap (USA).gba`,
16 MiB, header game code `BZME`, SHA-256
`bedc74df62755f705398273de8ed3bc59be610cf55760d0b9aa277f1f5035e73`,
SHA-1 `b4bd50e4131b027c334547b4524e2dbbd4227130`.
This matches the USA revision documented by
[zeldaret/tmc](https://github.com/zeldaret/tmc), used only as a reading aid.
Local checkout: ignored `sources/minish_tmc`, revision
`6fb6dfb4a7efbe24d0fd1dda5097af6131faacde`.

OBSERVED: pinned mGBA 0.10.5 reference core, HLE BIOS, frameskip 0 and no
idle-loop removal. `sources/minish_gba/reference.json` records the binary
hash, frame rate, options, exact injection writes and output hashes.

## Direct access, without playing the adventure

OBSERVED: after 360 neutral title frames, initialize a minimal study save
at `02002A40`, set its saved destination to area 0/room 0 with layer 1,
default spawn, facing 4 and local position (32,88). Select task 2/state 0/
substate 0 at `03001000`. The original game task loads the forest itself;
after 180 frames the live task/state/substate are (2,2,2), and the scene is
visible with working directional input. No story playthrough, ROM patch,
downloaded save or manual state is used.

OBSERVED: the original title retains a white exit fade. An injection that
only selects gameplay loads the correct map but leaves the palette white.
Preparing the normal file-select black exit in `gFadeControl` before switching
tasks fixes the display. The complete recipe is in `tools/reference.py`;
its metadata also records the save reset and every poke.

OBSERVED: local position (176,104) lies in solid terrain and cannot move;
this is not evidence of a broken keypad. Position (32,88) is walkable.
Twenty held RIGHT updates move X from 32 to 57 while Y stays 88: 1.25 source
pixels/update in this trial. A complete movement/collision model has not yet
been measured.

OBSERVED: two cold boots reproduce the same forest memory/video hashes.
Three 120-frame restored input sequences (one after closing and reopening the
core) match every sampled EWRAM, IWRAM, save, VRAM, palette, OAM, I/O and RGB
hash. Original enemies and managers remain active. The study save has three
hearts, no equipment and no claim of natural quest completion.

## Offline maps and types

OBSERVED: the USA room header at ROM offset `11C488` specifies origin
(2976,2160), size 1008×1008, tileset 0. This is one scrolling room, not a
sequence of fixed 240×160 rooms. "First screens" needs a route/endpoint.

OBSERVED: nine type-0x10 BIOS LZ77 streams at documented USA offsets decode
to the forest's three graphics banks, two metatile banks, two type banks
and two map layers without running the game. Their offsets, lengths and
hashes are generated in `reference.json`. Each map contains 63×63 metatiles
of 16×16 pixels. The original expands ROM rows to a 64-entry RAM stride.

OBSERVED: the two decoded maps, two metatile banks and two type banks match
the original loaded RAM byte for byte (56816 bytes total). Maps are compared
with `mapDataOriginal`; current tiles can change during gameplay. The
graphics-bank bytes are extracted, but animated tile replacement and
original RGB extraction fidelity still require separate checks.

OBSERVED: live lower map at `02025EB0`, upper map at `0200B650`;
map data +4, collision bytes +2004, original map +3004, tile types +5004,
four subtile descriptors +7004. Collision and surface semantics are separate:
an apparently empty collision byte does not alone establish safe walking.

TARGET: reconstruct the opening route plus camera margin on
our native runtime engine. Preserve measured rules, state units and timing;
make scale/camera and action adaptations explicit. Further source story
flags/equipment must be chosen and validated when the requested actions need
them, rather than treating this empty study save as a normal forest arrival.

## M1: native walking and TI checks

OBSERVED: (32,88) is a small western pocket with no walking-only connection
to the main alley. M1 starts at (248,88), with bends through (264,136),
(376,232), (424,156), (600,140) and (692,136). The original reaches
(691.75,136.125) in 555 source frames, always in normal player action 1.

OBSERVED: cardinal speed is 320/256 pixels/update; diagonal speed is 226/256
per axis. FixedMul truncates toward zero; an unrounded sine product is wrong.
All sampled walking fractions fit exact Q8.8. Ground-to-ground slopes reduce
speed by 80/256 to 240/256 cardinal or 169/256 diagonal. Slope layer 3 points
at the same lower collision array as layer 1 (ROM pointer table 08000248).

OBSERVED: normal player collision uses ROM lookup 080082DC and forty 16-row
masks through pointer table 0800823C. Eight probes around hitbox offset
(0,-3) produce bits: right low/high 4000/2000, left low/high 0400/0200,
bottom right/left 0040/0020, top right/left 0004/0002. Cardinal partial-corner
contacts can redirect motion along a free side at 1 pixel/update. Collision
IDs 10..13 hex rotate requested direction by 45 degrees. Post-move central
probes back out penetrating partial cells while retaining the coordinate
fraction. The native engine implements these measured rules in portable C.

OBSERVED: ROM offset 0B3E80 maps tile type to collision; 0B37A0 maps it to
action. Combining these with offline map/type streams matches 900 collision
and 900 action cells loaded by the original in the 720x320 opening. The
padded native bank contains IDs, masks, shape indices, diagnostic tiles and
action IDs; host-order and TI big-endian payloads both contain 9984 bytes.

OBSERVED: 73 twice-replayed original trials cover eight directions, neutral
input, opposing directions, contacts at six placements and the full route.
All eight memory/video digests match on every restored frame. 6310 retained
normal-walking steps match native X/Y fractions, direction and collision
bits exactly. Metadata records excluded tails at slice boundaries, original
doors and automatic non-walking actions; equivalence after those transitions
is not claimed. Original actors and managers remain active during trials.

OBSERVED: ten native cases compare 3560 explicit PC/TI state hashes and ten
final visible-plane checksums. All instrumented full-frame samples are below
360000 cycles; maximum is 246762, including hash/marker overhead and cold/
cached TileMap rendering. Normal route averages about 12274 update + 104245
render cycles. Compiled game C has no multiply, divide or floating-point
helper instructions. Runtime and ExtGraph primitives remain unchanged.

INTERPRETATION: M1 establishes opening traversal. Other actors, surfaces,
transitions and story systems remain separate. Graphics are a collision
diagnostic and our own outlined actor; ROM scene art is M2.

TARGET / ADAPTATION: 1:1 geometry in a tighter 160x100 camera; two source
steps per 30.118Hz draw make motion about 0.85% faster than the measured
59.7275Hz GBA clock. Input is sampled at draw rate. The native script reaches
(688,133.625), within five pixels of the original endpoint. Boundary clamps,
static doors and the endpoint flag are native slice choices.

## M2: original scenery, actor poses and canopy depth

OBSERVED: the study uses mode-0 4-bpp text backgrounds and ordinary OBJ
sprites. Packed low-nibble-first tiles, palette banks, descriptor flips,
screen blocks, OBJ dimensions/mapping and priority order decode offline.
BG3 has measured alpha weights EVA=0, EVB=16, so the layer is visually
transparent over its second target. Unsupported affine, window, 8-bpp and
other blend cases are rejected. The decoder matches every RGB pixel across
439 frames: 16857600 pixels, without tolerances or mismatch exclusions.

OBSERVED: the pinned core exports RGB565. Pillow's `BGR;16` expansion uses
integer rounding; a simple five-bit channel shift produces unequal RGB.
Match the original runner conversion before testing extraction fidelity.

OBSERVED: completed OAM/video depicts the previous logical player pose,
VRAM allocation and coordinates, with the previous room camera. Anchoring
against post-frame fields makes the same walking pose shift within its
canvas. Hardware BG scroll offsets include an extra eight-pixel vertical
cache margin. Previous entity and room fields give a stable (16,32) anchor
inside a 32x40 canvas, with a border reserved for the white outline.

OBSERVED: bottom BG2 and foreground BG1 compose from ROM metatiles/map
descriptors and live source VRAM. At the chosen initial frame, animated tile
replacement changes 40 bytes in graphics bank 0 and 136 in bank 1; bank 2
matches its decoded ROM bytes. Composing the ROM maps against live graphics
matches 604432 exposed background pixels at moving route checkpoints.
Scenery cannot be certified from the unmodified ROM graphics alone.

OBSERVED: Link's OBJ parts occupy tiles starting at 352 and palette bank 6
in this study. Stable pose keys yield four idle and forty walking poses.
Each cardinal cycle has ten poses, each lasting three source updates.
Four controlled trials settle for 80 frames, then reset only X/Y to
(320,184) before each of 100 held-input updates, followed by five release
frames. Two runs agree on every EWRAM/IWRAM/save/VRAM/palette/OAM/I/O/RGB
digest across 420 frames. Extracted exposed actor pixels match 124133 RGB
pixels; the native selector matches 400 displayed source poses and settled
idle in all four directions. The study save has no cap or equipment.

OBSERVED: at walkable (246,160), Link's ordinary OBJ parts have priority 2;
BG1 hides 76 opaque actor pixels. Native pixel fixtures check 51 contrasting
foreground/actor samples in each depth mode, for 102 assertions. Canopies
are restored only in masked metatiles intersecting the actor canvas.

OBSERVED: action 38 at (424,200)/(424,216) changes OBJ priority to 1;
action 52 at (104,248)/(120,248) leaves it at 2, although both classes slow
walking. Reading the separate entity spritePriority field does not establish
OAM priority: the actual value is spriteRendering bits6..7 at 03001179.
Eight controlled up/down trials replay twice with all eight digests across
96 frames, and the native displayed foreground flag matches every update.

OBSERVED: 227 opaque scenery tiles fit the existing byte-indexed TileMap.
Seventy-three masked foreground tiles and 44 preoutlined 32x40 actors live
in archived banks. Payloads: terrain 9984, scenery 25632, actors 21120 bytes;
all are read in place. The normal TI program is 9793 bytes. C game assembly
has no multiply/divide instructions or floating/32-bit arithmetic helpers;
runtime and ExtGraph primitives are unchanged.

OBSERVED: M2 preserves all 6310 walking fixture steps. Fifty-six native
cases compare 3606 complete field hashes and final visible-plane checksums,
including all 44 actor poses, both canopy depths, moving/blocked traversal
and reset. Maximum sampled full-frame cost is 296152 cycles, including
hash/marker overhead and cold/cache rendering. The entrance replay averages
13366 update + 122925 render cycles, below the 360000-cycle budget. Hardware
paths were unchanged; validation stayed headless.

TARGET / ADAPTATION: native geometry stays at provisional 1:1 scale in the
160x100 camera. Reduce scenery and actor palettes independently for contrast
and dilate actor masks by one pixel offline. Scenery retains the initial
animated-tile/palette phase; only Link animates. Idle blinking and pushing
poses are deferred; blocked native movement uses idle. Displayed animation
keeps the measured one-update lag, while position/camera use the current
native step. The ordinary actor draws behind the foreground; slope traversal
uses the source's foreground priority change. No enemies, sword actions,
destructibles, story interactions or new room transitions are claimed.

## Additional 70% view

UPDATE (after M5): the user retired the 1:1 build. The 70% view below is now
the only build, named `minish`; 1:1 figures elsewhere in these notes are
historical.

TARGET / ADAPTATION: the user requested an additional version zoomed out by
about 30%. `minishz` uses 7/10 visual scale while the source-space engine,
720x320 scope, movement clock and collision probes stay shared with `minish`.
The 160x100 LCD now covers approximately 228.6x142.9 source pixels. Camera
fields are display pixels in this build. The normal executable and art bank
checksums remain identical to the completed M2 build.

OBSERVED: nearest sampling of the full 70% world needs 411 distinct 16x16
tiles, beyond the existing runtime's byte-indexed TileMap. This variant
instead uses packed raster planes, with the measured Alundra C word blitter
and foreground restoration limited to the actor canvas. Reading the canopy
mask before color data and skipping transparent bytes lowers the peak from
345876 to 306898 cycles. No ASM or shared runtime changes were needed.

TARGET / ADAPTATION: verified source RGB/foreground masks and unoutlined
actor RGBA remain in ignored `fixtures/source_art.npz`. World RGB and canopy
coverage share the same nearest-sampling lattice at 504x224 pixels. Link's
32x40 source canvas shrinks to 22x28, is padded into 32x32 at anchor (16,24),
then receives a new one-pixel outline. Movement-to-display lookup entries
are floor(7*x/10), preserving exact Q8.8 source mechanics and quantizing only
rendered coordinates. Pixel reduction, masks and the table are prepared offline.

OBSERVED: `mizscene` contains two 512-bit-stride image planes and one
foreground mask, 43008 bytes total. `mizactor` contains the 44 actor poses
and a 721-entry u16 coordinate table, 18338 bytes. With the shared 9984-byte
terrain bank, total payload is 71330 bytes, read directly from archive.

OBSERVED: all 6310 walking, 400 displayed-animation and 96 actor-depth
fixture steps still pass. A separate unpacked-array target oracle checks
1312000 scene/actor pixels over 82 placements: all 44 poses, both depth
orders, all 16 horizontal shifts and clipping at all four slice corners.
Ninety-two complete native PC/TI cases match 3642 field hashes and final
screens. Maximum complete instrumented frame is 306898 cycles; the entrance
route averages 13910 update + 256352 render cycles. The renderer has no
multiply, divide or floating-point helpers. All validation remains headless.

## M3: ordinary sword and bush traversal

OBSERVED: equipping the Smith sword requires only byte writes 1 at save
`02002AF4` (A equipment) and 4 at `02002B32` (two-bit item ownership).
The cold forest study state, story flags and skills remain unchanged. Trials
settle 80 neutral frames at (320,184), then inject initial XY and cardinal
facing. `fixtures/actions.json` records these writes, state/ROM identities,
inputs and a digest-stream fingerprint for every twice-replayed trial.

OBSERVED: ordinary sword motion lasts 15 source updates. Its ten poses follow
indices 0,1,2,3,4,4,5,5,6,6,7,8,8,9,9. Player XY is locked until update 15,
when held movement resumes. Holding A produces one swing; a fresh press
restarts it, including during an active swing, and can change facing. Face
selection retains compatible cardinal facing for diagonals. Player-state
attack status is at `03003F84`; the item pointer is at `03003FAC`.

OBSERVED: tile cutting uses point samples on updates 1,4,10, separate from
enemy hitboxes. ROM signed-pair table at offset `129072` and animation frame
flags select these source offsets: north (+12,-15),(0,-22),(-12,-15);
east (+12,-15),(+15,-3),(+12,+7); south (-12,+7),(0,+13),(+12,+7);
west (-12,-15),(-15,-3),(-12,+7). West mirrors player sprite X. A tile
interaction additionally depends on the original tile-type/action lookup;
tile type 991 is not an ordinary sword-cuttable bush in this slice.

OBSERVED: the opening contains 53 cuttable cells: original metatiles 49/type63
and 19/type78. Cutting replaces them with 48/type28 and 18/type39. Live
collision changes from 29 to 0, and action from 20 to 10. The live lower map
changes immediately; the completed BG image shows the replacement one update
later. At the central probe, update 4 already has the changed map but only
39 of 154 exposed pixels match ground; update 5 matches all 136 exposed pixels.
Two settled replacement probes independently match 131 exposed RGB pixels.

OBSERVED: 68 restored input trials replay identically through all eight
memory/video digests over 1947 frames. Every source XY, active pose and complete
53-cell cut bitset matches native steps. All 53 cells are cut in these original
trials, including six interior cells reached by cutting neighbours, walking
through the cleared terrain and swinging again. No action, collision or map
bytes are injected after initial placement/equipment.

OBSERVED: Link parts use palette6/allocation352, while sword parts use
palette1/tiles368..399. Palette-aware attribution separates overlapping tile
ranges. Merged raw RGBA poses use a 64x56 canvas at anchor (32,36); ten per
direction yield 40 stable poses. Full compositor checks match 16819200 source
RGB pixels, including transient original effects. The native art intentionally
keeps Link and sword only. Outline dilation happens after merging and after
resizing for 70%; cropped silhouette bounds drive canopy restoration.

TARGET / ADAPTATION: shared C logic retains exact source updates, point
samples and immediate collision changes. Displayed poses and bush changes
keep the one-update video lag; position/camera retain M2's current-coordinate
convention. Two updates per native draw preserve the existing clock adaptation.
Bush cut flags and a compact cut list are part of portable state; reset
restores all bushes, and PC save/load restores changed scenery and collision.
Hearts/rupees, particles, sound, enemy damage, charging and spin attacks remain
outside M3. Bareheaded Link reflects the minimal study save.

OBSERVED: changed scenery is drawn before the actor as archived XOR deltas
against the immutable scene. Original canopy colors remain in a bush patch
where a foreground overlaps. At 70%, world sampling uses the same lattice as
the scene, patches discard unchanged rows, identical patches share data and
sixteen pre-shifted variants permit two word-aligned long XORs per row.
Clipped cases call existing ExtGraph routines. Restricting canopy restoration
to actual silhouettes reduces work further. No new ASM, runtime API, mutable
world bitmap or action heap allocation was added. Game/render C assembly has
no division, floating or 32-bit arithmetic helpers.

OBSERVED: independent unpacked source-derived arrays check 672000 additional
sword/changed-scene pixels per build, with zero differences. The earlier
6310 walking, 400 animation and 96 depth steps still pass; 70% retains its
1312000 prior oracle pixels. Native checks cover 216/252 screens and
4639/4675 complete PC/TI field hashes at 100%/70%, including all 40 attack
poses in two fully cleared areas and 32 dense camera offsets. Peak complete
frames, with hash/marker overhead, are 326918/331056 cycles, below 360000.
The early masked-patch 70% dense replay reached 391118 cycles; XOR patches,
pre-shifts and tighter canopy bounds bring it inside budget. All checks
remain headless because the runtime's hardware paths are unchanged.

OBSERVED: M3 TI programs are 12579/13245 bytes. Four archived payloads
total 75872/126294 bytes; new action banks `miact`/`mizact` are 19136/54964
bytes and remain under AMS variable limits. The normal TileMap and separate
70% raster view are retained. Doors 140/143 start beside bush beds; 100..139
inspect attack poses; 141/142 and 144..255 stress fully cleared scenery.

## M4: two opening Octoroks, ordinary combat and native retry

OBSERVED: the original loader gates room property 2 (enemy placements) on
global flag `TABIDACHI` (0x15). The original walking/sword study save lacks
this flag. `tools/combat.py` sets bit5 at save+0x25e before the original room
loader, plus the two existing Smith-sword inventory/equipment bytes. Two cold
boots reproduce all eight memory/video digests. `combat.state` is separate
from `woods.state`; neither asserts natural adventure progress.

OBSERVED: the ROM's placement list at 080F4F30 contains Octoroks at (328,56)
and (280,152); the remaining enemies lie outside the 720x320 native opening.
The sampled entity pool begins at 030015A0, with 0x88-byte entries; the second
opening Octorok occupies 03001958 in this reproducible door. Enemy and ordinary
Link bodies use offset (0,-3), half-extents (6,6); contact at separation12 is
accepted and separation13 excluded. Octorok health is2, ordinary walking
speed96/256 pixels per source update.

OBSERVED: ROM 080CA170..17F confirms walking durations30/60/60/90, the
ordinary spit threshold1 (outcomes1..3 out of4), nut offsets (0,-3)/(4,0)/
(0,2)/(-4,0), and bound-turn modifiers+4/-4. Source pauses are
24+(Random()&0x38). The walking animation has two poses of16 updates per
facing. Shooting poses last4/16/4/4 updates, with ordinary rock emission
at the transition after the first20 updates. Actual natural and controlled
shoot traces are recorded; the latter set only the initial animation pointer,
direction, action, timer and placement before taking their input-only samples.

OBSERVED: ordinary contact and rocks subtract2 from the24-unit, three-heart
study health. Contact starts30 updates of invulnerability and recoil duration8;
the player decrements before moving, resulting in seven movement updates at
speed0x280 (2.5 pixels cardinally, diagonal components452/256). A Smith-sword
hit kills the2-HP Octorok, followed by12 updates of enemy recoil at speed0x180.
Source sword boxes are sampled per action update and facing from the auxiliary
player-item pool at030011E8, separately from the three bush-point samples.

OBSERVED: a rock has half-extents(2,2), offset(0,0), Z=-3, speed0x280 and an
initial48-update ordinary-flight timer. The original changes state on terrain
impact, sword deflection or timer expiry, with slower falling/bouncing phases.
At player contact the source rock persists for one logical collision update
before deletion. Native contact removes it immediately; flight fixtures stop
comparing that rock once the player loses health. Health/invulnerability/
recoil remain compared through that impact and the subsequent updates.

OBSERVED: twenty twice-replayed original trials cover608 targeted updates:
four cardinal walking trials, eight contact/boundary trials, four sword hits
and four shooting trials. Each restored update checks all eight original
memory/video digests. The native fixtures compare enemy XY for walking,
complete player XY/health/invulnerability/recoil for contact, kill timing for
swords, and ordinary projectile XY/timer plus player damage/recoil for shots.
These are selected mechanic comparisons, not whole-enemy or whole-room equality.

OBSERVED: twenty source Octorok poses are captured from four standing/walking
cycles and four shooting sequences. Complete RGB composition matches10137600
original pixels. OBJ attribution uses the previous entity/camera, its palette
and the64-tile allocation range (shooting tiles exceed the walking32-tile
range). Both scales add960000 independent unpacked enemy/hero/canopy oracle
pixels, covering all poses at three actor depths. White halos are generated
after scaling; the native rock glyph deliberately uses an eight-pixel sprite
(six-pixel round body and one-pixel white halo) for visibility in both views.

TARGET / ADAPTATION: two source updates per draw remain shared by both views.
Local16-bit RNG and a smaller roaming rectangle choose native pauses/turns/
spits; the original global RNG and other room actors are not reproduced.
At walk expiry, a target within128 horizontal/96 vertical source pixels
selects the shooting facing, with the existing3/4 chance. This native aiming
change allows shots when the preceding walk faced away from Link; source spit
timing, mouth offsets, ordinary speed and damage remain measured parameters.
Native tile blocking is a conservative body probe. Player/death recoil uses
eight directions, preserving measured cardinal/diagonal speeds; enemy contact
push,32-way death angles, drops, source death effects and story persistence
are deferred. Native death has12 recoil updates then20 blinking updates.
Rocks disappear on wall/sword/player impact or48-update expiry. Falling,
bouncing and reflected returns are deferred. Quarter-heart HUD and an ENTER retry
are native presentation; the knockback poses and flash are below.
Hearts keep their quarter fills, a two-pixel grey highlight and a silhouette
mask dilated by exactly one pixel; scenery remains outside that mask. HUD and
projectile glyphs are generated independently of the ROM-derived combat bank.

OBSERVED: the native300-draw combat route defeats both Octoroks with22/24
health remaining. Unit checks cover failure/retry and save/load of all cuts,
both actors and four active rocks. Door420 starts a real walk-to-spit AI
transition while facing away from Link; a unit check follows release, flight,
damage and removal. Complete PC/TI checks pass7521 hashes/
346 screens at100%,7557 hashes/382 screens at70%, including last-heart failure,
all enemy poses,48 clipped/offscreen actor/rock doors, and intermediate route
screens around the clipping regression. Maximum instrumented complete frames
are359248/348320 datasheet cycles, under360000. Both variants retain their
programs and five archived banks; payloads total84956/175202 bytes.

OBSERVED: a mixed signed/unsigned canopy bound (`x+w`, signed x/unsigned w)
behaved differently under the TI16-bit and PC32-bit integer promotions when
an actor was completely left of the LCD. It restored a full-width strip on
the TI and raised a route frame from its proper cost to365688 cycles. Explicit
signed rectangle sums, bounds rejection before unsigned indexing, and new
intermediate/edge screen comparisons fix both rendering and the cycle spike.
Precomposed static HUD rows, pre-shifted ordinary rocks, pre-shifted zoom enemy
poses, and pointer/clip-mask reuse in canopy restoration keep dense encounters
inside budget. No new assembly or hardware/runtime path was introduced.

OBSERVED: after an ordinary contact the player plays animation 24+facing during
the seven recoil updates, independent of the knockback direction: frame82
(up) for all seven, frames79/80/81 (right, left mirrored) and76/77/78 (down)
for three, three and one updates; the last recoil update returns to idle.
From the contact, Link's OBJ attributes use palette15 for the thirty
invulnerable updates and he is never hidden. Palette15 changes every four
updates (first phase three updates): a saturated red/orange cycle whose
luminance runs medium, darker, darkest, lightest. The knockback tiles are
decoded with Link's ordinary palette6 (two identical replays per facing).

ADAPTATION: the four-grey flash maps the phases to one step darker, black body,
black body, ordinary; white outline pixels stay white. Phase timing is taken
from the contact, as observed; whether the source cycle is global was not
tested.
