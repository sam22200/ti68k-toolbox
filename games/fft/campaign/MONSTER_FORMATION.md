# Integrated monster Formation and Status (2026-11-14)

The approved16 family appearances now render through the actual management and
deployment programs. All48 jobs retain their real names and identities. FMA1
stores one32x26 body and32x29 portrait per family, plus255 source monster names.
The pictures match the accepted [review](MONSTER_FORMATION_REVIEW.md) exactly;
there is no new palette vote. Human palette-only variants remain excluded by
the user's [resource policy](CORE_RESOURCES.md).

**The combat/deployment gate remains open.** The deployment renderer can draw
these bodies, but real placement and the battle party loader still reject
monsters. A private injected placement picture does not make them playable.
Standing/action sprites, facing, cover/water/flash/cache ownership and complete
selected-family KO art must reach the battle renderer before removing that veto.
Do not let gfx255 fall through the human16-column actor lookup or replace a
monster with human artwork. Then finish actual deploy/Save/Continue/battle/result
flows before starting Bar/Fur Shop or campaign extraction.

## Runtime behavior

Formation shows whole silhouettes, portraits, individual names and species/job
labels. Status hides human Job Lv/Total JP. Eggs retain their approved card and
default-No dismissal; they cannot open fighter Status. F1/F5 browses adults in
Status and skips them in human equipment/ability/learn/job pages. This prevents
a human page from reaching a monster through a lateral browse despite the
command-menu veto. Human equipment navigation remains available. Browsing,
rendering and denied actions change neither Save nor the session file.

All data pointers borrow archived bytes. No new allocation or cache is added;
the existing page cache and portrait renderer are reused. Missing/invalid FMA1
leaves human/egg fixtures usable and withholds adult artwork. Normal shipping
requires the matching core manifest and its validated ffma member.

## FMA1 archive format

The universal core owns one `ffma`,13082 raw bytes. Header16: magic4; physical
count16, width32, height26, reserved0; four BE words: name count255, frame
offset574, name-directory offset64, string-pool offset11134. The following48
bytes map jobs94..141 to16 family records. Each660-byte record contains body
light/dark/inverted-mask planes104 bytes each, then portrait planes116 bytes
each. The255 BE offsets address pooled, terminated ASCII names of1..15 bytes.
Only final zero alignment padding is allowed. Cold validators check aliases,
offsets, names and transparent-plane invariants; the C reader handles the native
six-byte DAT trailer. Frame/name reads are direct after validation.

`tools/monster_formation_bank.py` losslessly converts the approved local FMR1
bank; all16 retained picture records are byte-identical. No new extraction,
palette sweep, decompression or per-frame resource search is introduced. The
new core capability is `formation_art_format: FMA1`; packaging validates its
payload before producing the group. Management/deployment share this archive.
Future battle art must reference compatible shared pictures/portraits instead
of copying them into each encounter.

## Evidence and packages

```sh
rtk proxy make -C games/fft/campaign campaign-monster-formation-check
# Only the two archive/word-order/render checks that need the native CPU:
rtk proxy tools/pyenv/bin/python games/fft/campaign/check_monster_formation.py --native
rtk proxy tools/pyenv/bin/python games/fft/campaign/build_monster_formation_release.py
```

PC and ASan/UBSan agree on ten accepted pictures and two real input flows,
including malformed archive boundaries,48 family references, distinct job
labels, full16-party browsing, Status/egg exclusions and human-page guards.
Two selected native frames retain21E4/86E9 and55 reference/render checks each;
their unchanged drawing/archive path is reused after the PC-only input-guard
follow-up. The egg menu/dismissal/atomic-save retry regression passes on PC and
sanitizers with the actual installed data. Packages pass42 host/867 C checks
on PC/sanitizers. No battle-family/native sweep or TiEmu ran.

The old `test_manage.c` fixture expects3 units but its preserved FSV4 input has5.
Its failure reproduces on the preceding commit; neither its assertion nor the
old fixture was changed. Current human equipment/list navigation is exercised
by the new actual-input flow. Details live in `x/monster-formation/regressions/`.

ABI92/ModCtx356/FA04, Unit112/State2224/Work29340, FSV7/Save3992, all28 battle
images, zone8702 and the3072 companion reserve remain. The battle program and
shards are byte-identical to the egg release; its native CPU/layout evidence is
reused. Only compressed ffmgcore/ffpccore, ffma and matching manifests change.
Core441437 + chapter291789 =733226 transport bytes, key
`37f0ec2538ccb78ed0b0f20ef00cdedd`, under `x/monster-formation/release/`.
ffmgcore grows621 payload bytes, ffpccore571; stubs and all chapter content
except ffpack remain exact. These are archive/transport figures, not an
installed16KiB/peak pass. Preserve16 party, four guests,13 actors and48 cache
entries; difficult extra2KiB recovery remains deferred.

Historical pictures, proofs and saves retain their bytes/dates. Current proof:
`x/monster-formation/verification.json`; current actual-page collage:
`x/monster-formation/review.png`.
