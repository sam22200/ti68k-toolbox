# Source-specific monster KO poses (2026-11-13)

The user approved the proposal: "parfait le visouelle et continue". The core
renders the six source-specific Dead families through optional chapter-owned
FKO1 art. The human pilot contains no unused monster bank. Full standing/action
art, other species/palettes and authored monster encounters remain content work.
ABI89, Work29310, State2224, FSV5/Save3204, seven blocks98444 and48 cache entries
remain. All28 executable overlays are byte-identical to the trap release.

## Observed in FFT

`tools/monster_ko_review.py` resolves six ordinary sheets from the retail
spritesheet extent/shape/sequence tables in BATTLE.BIN, not from guessed job
names. It executes the original SEQ unpacker and unit sequence player with
Unicorn on the PC. Only the downstream GPU part loader is isolated; its frame
and animation arguments are recorded. Four families give16 first-frame/wait
pairs, matching the strict direct-pair reader:

| Sheets in this study | SHP / SEQ | First idle, front/back | Dead, front/back |
|---|---|---|---|
| Behemoth, Bomb | MON / MON | 2 / 4 | 26 / 28 |
| Goblin, Skeleton | TYPE1 / TYPE1 | 11 / 16 | 23 / 24 |
| Squid | TYPE2 / TYPE3 | 11 / 16 | 23 / 24 |
| Chocobo | CYOKO / CYOKO | 9 / 14 | 37 / 38 |

The source Dead animation is0x1a, indexed as52/53 for front/back. Its first
frame has wait0. These fixed Dead poses are distinct from the falling/death
transition animation0x34 (104/105); this study does not replay that transition.

The Behemoth's tiny KO silhouette is intentional source artwork, not loss of
a large SHP part. Its ordinary frame has955 opaque source pixels in the front
sample; Dead has164, bounds x=-13..8/y=-10..1 relative to the feet. The rear
Dead sample has177 opaque pixels. Bomb leaves a small flame; Chocobo leaves
feathers. Goblin, Skeleton and Squid retain larger fallen silhouettes.
Human-only frame23/24 selection would leave Behemoth in another full-body pose.

The compositor retains every SHP part, tile order, flips and the original feet
anchor. Chocobo includes off-canvas transparent dummy parts; they must not
crash composition or justify silently dropping opaque parts. The parser accepts
offset0 as a valid frame even at a nonzero index, and rejects absent/truncated
records. Retail SPR extent sizes are sector-rounded, unlike ISO file sizes.

## Approved implementation

The comparison shows source colours and a proposed four-grey conversion at the
same0.6 scale and anchor, front/back beside each other. It uses no human face,
skin or eye correction. The proposed silhouette receives the required white
outline. Source-derived images and reports stay under `x/monster-ko/`.

The32x40 preview is cropped in **empty rows only**, keeping scale/feet anchor
and all silhouette pixels. The samples fit8..20 rows. Each physical record has
32 columns and four raw planes: light, dark, inverted mask and inner body.
Eleven physical poses cover six front/back pairs and their mirrors in2876
archive bytes. Equal records, complete mirrors and job aliases share IDs.
The current human/object banks had no suitable monster body to reuse.

Computing wide cover every frame initially exceeded the CPU budget. Each fixed
KO now prepares two shaded16x26/208-byte halves in the **existing cache**.
At most26 entries serve13 KO units, within its unchanged48-entry capacity.
Cover.cm holds two remembered indexes/height while r0's high bit marks KO mode.
Both current cache keys must match after eviction; job/pose, feet, depth,
camera and immersion invalidate the result. Normal cover is rebuilt on revival.
Fixed KO poses skip animated decoding; preparation misses retain the optional
work charge. No Work growth, allocation or new assembly is introduced.

Keys0xff00 upward are reserved for shaded halves. Packaging/cold loading reject
physical actor counts at or above32512 with FKO1, preventing mirror-key clashes.
Native mirroring uses the existing verified ExtGraph routine. Ordinary cover,
white interior/outline, immersion, clipping, markers and the KO-to-crystal/chest
dissolve remain. One active dissolve uses bounded stack scratch; waiting actors
keep cached KO poses. Low-poly turns retain their unoccluded actor presentation.
Perspective experiments remain outside campaign packages.

Do not replace these small KO poses with scaled human corpses. Keep their
original size relationship and the battle's existing KO counter/target feedback.
Other palettes/species, animation transitions, scene actors and complete
chapter monster banks remain content work; six source samples are not a census.
Standing Behemoth art still needs its full width in that future content path;
it has not been cropped to the human16-column frame.

## Format and packages

FKO1: magic4, pair/frame counts2+2,160 job pair indexes2 each (65535 absent),
four keys per pair2 each (front/back/mirrored front/back), frame_count+1
offsets2 each, then even-aligned records. Key bit15 is mirror. A record has
height1, anchor_x1 (=16), anchor_y1, reserved1 (=0), then four height*4-byte
planes. Height1..26, anchor_y0..28 and height-anchor_y<=12 bound40-row dissolve
scratch. Validators reject bad IDs/offsets/extents/flags and coloured/inner
bits outside the opaque mask.

Optional `ffko` is active-chapter art, validated cold and borrowed in place.
Every arena reuses it. `ko_actor_format: FKO1` binds core support into the
matching package key; old core reuse is rejected even though ABI89 remains.
Only fftbat/ffbcore change; all other programs/overlays/pilot data are reused.

Private normalized pilot: core412690 + chapter291735 =704425 transport bytes,
key `0a6f05fdec1f5dd1e3eaf1c97f717351`, under `x/monster-ko/release/`.
`release-with-ko/` adds the bank once (chapter294653); it is an inspection
fixture, not authored monster encounters. Never distribute source.89g.
Actual input61211/headroom3725 grows **3590 program-RAM bytes** versus traps.
Heap/cache capacities do not grow, but old free-RAM estimates are not current.
The16KiB/installed peak gate remains open; difficult RAM recovery stays deferred.

## Reproduce

```sh
rtk proxy tools/pyenv/bin/python -m unittest discover -s games/fft/tools -p 'test_monster_ko_*.py' -v
rtk proxy tools/pyenv/bin/python games/fft/tools/monster_ko_review.py
rtk proxy make -C games/fft/campaign campaign-monster-ko-check
# Optional selected68000 renderer carrier, not a whole native sweep:
rtk proxy tools/pyenv/bin/python games/fft/campaign/check_monster_ko.py --native
```

Seven Python tests cover source bounds/composition and pool/mirror/job aliases,
format/package rejection. PC/ASan/UBSan and one actual68000 renderer carrier
pass131 checks/59645262 and32 equal screens;24 poses/mirrors match the already
approved image pixel-for-pixel. Eviction of either half is checked. Normalized
packages pass42 host+819 C/94924924 on PC/sanitizers. PC battle/aftermath/save
flows pass; their stale FSV4 test input failed identically at baseline7bf703c
and is now FSV5. Migration itself remains in the existing save/trap suites.

Selected13-actor native ordinary-frame peaks, with cold preparation separate:

| Case | Checksum | Peak cycles |
|---|---|---:|
| Human rotation replay, optional KO bank installed | ECF7 |344308|
| Human equipment last actor | AA26 |357644|
|13 fixed monster KO poses |6753|318768|
| One dissolve plus12 waiting KO actors |41CF|262086|
| First KO then revival among13 actors |0EF6|296802|

First scene/cache preparations remain slower (about1.6..5.0M cycles in these
fixtures), not ordinary frame-rate claims. Evidence and controls are under
`x/monster-ko/`; final renderer runs are `verified-integration/`,
`verified-capacity/` and `morph-transition/`. No TiEmu or103-suite sweep ran.

## Independent next work

`incapacity.inc::battle_outcome` currently implements ordinary elimination
and mandatory Ramza permanent-loss/allied incapacitation defeat. Authored
target/protect objectives need validated encounter-local identities, explicit
KO versus permanent-loss semantics, simultaneous-result precedence, and an
exactly-once victory/save handoff. Do not infer them from current enemy count
or a guest's team flag. Objective text/target emphasis requires a separate
concrete visual review. The existing FCD5/ABI89 packages remain the baseline.
