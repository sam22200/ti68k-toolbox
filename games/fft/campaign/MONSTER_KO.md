# Source-specific monster KO poses (2026-11-13)

Source study and a visual proposal are ready. **Not integrated or approved.**
The shipping pilot still contains human actor banks; Sweegy's rules/save replay
does not certify monster artwork. No core build, ABI, cache, save or package changed.

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

## Target implementation / pending decision

The comparison shows source colours and a proposed four-grey conversion at the
same0.6 scale and anchor, front/back beside each other. It uses no human face,
skin or eye correction. The proposed silhouette receives the required white
outline. Source-derived images and reports stay under `x/monster-ko/`.

The preview uses a **32x40 study envelope**, not the current16x26/208-byte
runtime frame. In particular, the standing Behemoth cannot be cropped to the
human width without losing its body. Approval of the poses is not approval of
a changed runtime cache or a new memory/CPU claim. After review, fit the actual
chapter actor path without enlarging every cached human frame, retaining
occlusion, shared physical storage and mirror reuse. Search/reuse existing
chapter artwork first; the current shared banks are pilot humans and objects,
with no suitable Behemoth/Bomb/Chocobo body to reuse.

Do not replace these small KO poses with scaled human corpses. Keep their
original size relationship and the battle's existing KO counter/target feedback.
Other palettes/species, animation transitions, scene actors and complete
chapter monster banks remain content work; six source samples are not a census.

## Reproduce

```sh
rtk proxy tools/pyenv/bin/python -m unittest discover -s games/fft/tools -p test_monster_ko_review.py -v
rtk proxy tools/pyenv/bin/python games/fft/tools/monster_ko_review.py
```

The four focused tests cover asymmetric part order/flips, transparent off-canvas
parts versus forbidden opaque clipping, SHP bounds and strict SEQ rejection.
`x/monster-ko/results.json` records the source-file/function hashes and pose
bounds; `source-and-four-greys.png` is marked for a gallery decision. No shipping
bank is written. No TI test is needed for this host-only study.

## Independent next work

`incapacity.inc::battle_outcome` currently implements ordinary elimination
and mandatory Ramza permanent-loss/allied incapacitation defeat. Authored
target/protect objectives need validated encounter-local identities, explicit
KO versus permanent-loss semantics, simultaneous-result precedence, and an
exactly-once victory/save handoff. Do not infer them from current enemy count
or a guest's team flag. Objective text/target emphasis requires a separate
concrete visual review. The existing FCD5/ABI89 packages remain the baseline.
