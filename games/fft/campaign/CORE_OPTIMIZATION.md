# Final core/campaign optimization pass

2026-11-07, after the shared-resource delivery. Combat coverage remains closed
at201 actions/78 RSM. This pass changes the shared actor cache and scene heap
reservation; it does not change artwork, rules, archive formats or saves.

## Changes

`actors.inc` checks a slot's remembered cache entry before scanning48 keys.
The entry's current key must still match: another actor can have evicted it.
The ordinary lookup, LRU ages, animation budget, mirror handling and ownership
updates remain the fallback. Existing zero-initialized Work and entry writes
keep every remembered index within the cache. No extra cache or per-unit field
is introduced. This implementation is shared by battles and scripted scenes.

The scene player validates all steps before reserving its widest picture's
six planes. It allocates once per set, rather than reserving the engine's
320-pixel maximum for every set. Narrower sets release excess memory; sets
containing only titles/cards reserve no picture. Failed allocations leave a
missing-set result and support retry. PC reinitialization releases the previous
Work, picture and actor allocations instead of abandoning them.

| Scene picture buffer | Before | After | Bytes saved |
| --- | ---: | ---: | ---: |
| Orbonne,224 pixels wide |24000|16800|7200|
| Academy / Gariland title,160 pixels wide |24000|12000|12000|

Native scene Work remains7264 bytes and its48-frame cache10176. All four
depth/mask planes remain present. Tracking the picture allocation adds one
static16-bit size; the table above measures the picture buffer itself.

## Measurements and bounds

Identical13-actor replays under `ti-cycles`:

| Scenario | Ordinary average before/after | Ordinary peak before/after |
| --- | ---: | ---: |
| Board |223615 /218905|336750 /332836|
| Rotation replay, ordinary frames |255742 /248713|336750 /332836|
| Last actor Status |137800 /137737|330010 /328080|
| Last actor equipment |143887 /143843|353782 /351228|

The board and rotation replay averages improve2.1% and2.7%; this is not a
universal frame-rate claim. Cold loads and turns remain separately slower.
A deliberately late-entry1000-hit cache probe drops from2636022 to500022
cycles; that isolated probe is not a whole frame.

Fresh core393176 + chapter213249 =606425 transport bytes,42 more than the
preceding delivery. Installed payloads/size words total605249. The small code
increase buys CPU/RAM savings. Actual production TTPack input64911/64936 leaves
25 bytes; execution zone8690, ABI80/348/signature0A92,27 images, Unit112,
State2224, battle Work37460, FSV4 and five battle allocations remain unchanged.
Core content changes its package key: use the matching generated chapter
manifest. Chapter content payloads and all27 executable images are unchanged.

## Verification

`make -C games/fft/campaign test` includes the58-check cache fixture, scene
grow/shrink/no-picture/allocation-failure cases and existing campaign tests.
The cache fixture agrees on PC, ASan/UBSan and68000, including stale entries,
cross-bank frames, budget deferral and a new actor with no decode budget.
Scene lifetime tests also pass ASan/UBSan with leak detection enabled.
The old Status unit fixture compared three contiguous planes after chapter
panels had changed to three ten-byte slots per row. Its failure reproduces
with the pre-task actor code. The fixture now compares every displayed byte
through `PAN_CACHE`, retaining all three planes without reading adjacent caches.

The shared-resource suite passes237 host/33 C checks and nine identical scene
replays on PC/sanitizers/68000. Four13-actor renderer checks retain
0546/ECF7/A87E/AA26 and ordinary peaks below360k. Native memory probes confirm
the buffer sizes above. Actual installed pilot flows retain rules, pixels,
FSV4 saves, repeated reward receipts and full-stock conversion.

Private evidence and matching install groups: `x/final-opt-pass/`, especially
`verification.json` and `release/INSTALL.md`. Historical groups/proofs/saves
and the live emulator are not build outputs for this task. Full combat-family
certification is historical; only affected integration was rerun.

Further broad factoring or runtime compression is not justified by this pass:
archive identities/art are already pooled, CPU lookup tables serve measured
paths, and adding overlay crossings to save small helpers can cost frames.
Keep new optimizations evidence-driven and preserve the25-byte resident gate.

## Rules for subsequent changes

1. Reuse the shared actor implementation. Keep the remembered-entry fast path,
   validate its current frame key, and retain the full search on eviction.
   Preserve48 entries, mirror keys, LRU ages and the decode budget. A new actor
   must never inherit another actor's appearance when the budget is exhausted.
2. Keep allocation out of scene frames and step transitions. Validate the whole
   set first and reserve its widest picture once, including all six planes.
   Titles/cards alone require no picture buffer. On set replacement, release
   excess capacity; on failure, expose a missing set and permit retry. Release
   all previous scene allocations on PC initialization and on program exit.
3. Access cached panels through `PAN_CACHE` and compare the displayed span of
   every plane row. A packed or aliased buffer is not a contiguous plane array.
   Reproduce an unexpected fixture failure against the preceding code before
   changing game behavior or weakening assertions.
4. Measure hot queries and complete affected frames separately. A fast cache
   probe does not certify frame rate; distinguish ordinary frames from cold
   loads/rotations. Require unchanged scene/render pixels and state/save rules.
5. Count core plus the installed chapter, actual production TTPack input and
   peak RAM. A few extra archive bytes may buy CPU/RAM savings, but never exceed
   the executable limit or silently shrink animation caches. Any core content
   change requires matching chapter metadata even when ABI/layouts stay fixed.
6. Run the affected PC checks, sanitizers and native replays; use private output
   and save directories. Preserve historical evidence and the live emulator.
   Reuse source-hashed unchanged evidence instead of reopening combat coverage.
