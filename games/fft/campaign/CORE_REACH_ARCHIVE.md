# Reach colours in the archive (2026-11-12)

This follows [bounded grayscale](CORE_RAM_MARGIN.md). The production battle now
restores reach rows directly from immutable archive colours. Native Work drops
8,182 bytes and resident input drops456: **8,638 bytes saved together**.
The16KiB free-RAM gate remains open. Evidence lives in
`x/archive-reach-colours/verification.json` and its adjacent logs.

## Storage and ownership

R4 retains the eight-byte ragged-view header: magic `R4`, big-endian u16 colour
length, optional width-mask length, unpacked ragged-depth length. It stores two
raw320-pixel colour planes (40 bytes/row), an optional raw one-plane width mask,
then the existing ZX0 depth stream. Depth, cover, scene dimensions, art and
coordinate convention are unchanged. Cold byte-wise validation checks lengths,
row capacity and the fixed20,480-byte depth envelope.

`view_split.py` converts existing R2/R3 losslessly and idempotently, preserving
the compressed depth and rebasing the contour tail. The eight current views
retain every colour/depth byte. Their local bank sizes grow80,043 bytes including
alignment; shipping still strips the common contour tail into `ffcu`.
There is one physical colour payload per shared view, without a second compressed
copy. Exact chapter-wide resource pooling remains mandatory.

The core spec declares `view_format: R4`, changing its manifest key. Build and
install matching core/chapter groups; a content-only build cannot reuse an old
core. Packaging resolves aliases before rejecting non-R4 views. FSV4/3202 saves
and configuration are outside both groups and unchanged. R2/R3 decoding survives
only in the explicitly selected historical PC control. Optional width-mask
handling is retained for isolated review fixtures; production remains1px.

## Runtime and budgets

`ring_into_scene` reads the original colours of `scene_rot` from the archive;
`scene_rows` copies any changed span back without a128-row pool. Rotation keeps
its required full-resolution scratch, removes the unused half-resolution buffer
in the native low-poly configuration, and stores clamped sort keys and buckets
as bytes. Legacy/configuration/texture experiments retain their half buffer.

The stable counting sort accepts256 nodes: cumulative positions wrap only after
the final slot. If a sole bucket has256 nodes, its histogram wraps to0 and its
start is correctly0; forward placements still visit0..255 exactly once. Other
bucket populations fit bytes. Do not consume the final bucket values as counts.

| Native production quantity | Before | Current |
|---|---:|---:|
| Work |37464|29282|
| Scene colours + Work allocation |53624|45442|
| Seven requested game blocks |106596|98414|
| True TTPack input, including length word |57731|57275|
| Input headroom below64936 |7205|7661|

ABI87/ModCtx352/signature61867 replaces ABI86. Zone8698, Unit112, State2224,
48 actor frames, depth20480, navigation5120 and companion3072 remain. There are
28 images in three banks63768/61648/57504 bytes; the companion image is390.
Production groups built privately in `x/archive-reach-colours/release/` occupy
405033 core +291731 chapter =696764 transport bytes (core stored bytes404445).
Core key: `df78af11aa63c652776de37962b10015`. Never distribute `source.89g`.
Historical packages, saves, captures and the user's emulator profile are preserved.

Archive cost is a RAM/performance trade. A deliberately conservative extension
of the old roughly1.03MB Chapter I estimate adds all raw colours for13 maps,
without crediting their former compressed bytes:13*4*202*40*2 =840320 bytes,
roughly1.87MB total. This is below the2,163,895-byte chapter planning allowance
(2700000 -131072 reserve -405033 core). It is **an estimate**, without optional
width masks, not an assembled/full-chapter Flash certification. No109-map census
or source artwork re-extraction was repeated.

## Validation and approved pixel correction

PC and ASan/UBSan come first. `make archive-reach-check` compares production
with the historical algorithm using a202-row pool, sufficient for the entire
scene. Each selected Orbonne/Gariland and synthetic-mask profile checks64 full
scene plus64 LCD hashes through dense/sparse/global reach, both styles and
width modes, cancel, forced reload and depth immutability. All384 per-phase
comparisons agree. The one selected68000 carrier matches all128 hashes of the
mask profile and5586 checks/hash74B2ABF3. Header boundaries and5 distributions
at1/255/256 sort nodes are included (5105 sort checks).

The old128-row control could overflow, then sample already-painted ring pixels.
A whole-run aggregate hash hid paired differences; independent per-phase hashes
exposed them. The real Gariland review changes exactly two visible pixels in a
dense global range, at x38..39/y88. The user approved the before/after image with
`cest bon`; `review/decision.json` records its scope and image hash. This is an
approved overflow correction, not a claim that every old marker pixel is retained.
The independent ring compositor passes168 exact full-scene checks with both
storage layouts. Base scenery is unchanged.

Other affected checks:

- A complete ordinary Sweegy fight agrees PC/sanitizers and the previous trace:
  14697 frames,43 actions,16 monster actions,129 moves,40 HP changes, victory,
  382 checks/hash453E560F and identical FSV4 save. This caught and fixed a loader
  loop reusing the returned terrain-section offset; the final reader keeps it.
- 256-node native/PC/sanitizer fixture:3185 checks/hashAD6C1C88. Byte buckets and
  node255 are exercised explicitly.
- All28 native image bindings pass356 checks; a real ABI86 export is rejected
  before execution. Fresh normalized packages pass42 host +819 C checks on
  PC/sanitizers (94924924). Cleanse rendering passes192 checks/8357FD26.
- 13-actor native rotation ordinary peak335008/average247699 cycles; equipment
  peak355298/average145828. Screen checksums ECF7/AA26 remain. Cold rotation peak
 4962432 is separate; these zones exclude presentation as in the baseline.
- One16160-byte Gariland colour transfer takes97244 datasheet cycles instead of
 914868 for ZX0, with identical output. This is a transfer microbenchmark, not
  whole-frame or loading latency. No full combat-family/native sweep was repeated.

## Installed Titanium control and remaining gate

**Verified in a private Titanium TiEmu session:** matching installed diagnostic
core/chapter manifests, archived banks and an FSV4 route fixture resume Gariland
through the real `fftstory(2)` supervisor. Keyboard, grayscale and quit/return
work. RAM/logs were the primary evidence; one progress screenshot was captured.
The private container/Xvfb were then stopped; the gallery remains available.

At `runtime ready`: **14058 free, largest block13042**. Diagnostic resident heap
is57860 bytes; its production counterpart is estimated57277, and the log
variable occupies530. Merely removing these diagnostic costs predicts15171
free, still below16384; that is an estimate, not a production measurement.
The older9606-byte result used a direct caller and a different installed set:
do not subtract those two readings to infer a like-for-like gain.

Supervisor free heap is185236 before the call and185096 after return (delta-140),
with largest block65520 restored. Seven game blocks are released; the remaining
140-byte AMS/runtime change is recorded, not silently normalized. No repeated
entry/leak or worst-flow peak certification is claimed by this one run.

Follow-up user decision (2026-11-12): a bounded audit selected no simple extra
roughly2KiB saving; defer further difficult RAM refactoring and proceed.
[Archive planning](CHAPTER1_ARCHIVE_PLAN.md) now provides a reproducible host-only
projection. The16KiB target and production installed peak/contiguous/reentry
certification remain open. Next is the installed chapter route, PC-first.
Preserve actor/cache/scene allocation gains; do not reopen a generic combat queue.
