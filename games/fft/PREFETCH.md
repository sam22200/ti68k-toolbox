# Cooperative perspective preparation

The [storage cleanup](STORAGE_CLEANUP.md) now removes textured-turn/attack banks and
compresses fixed inspection colours into3,902 archived bytes. The20,284-byte idle
cache and its Work layout are unchanged. Current CPU-only fixture entry checks are
389 ms close /194 ms wide when prepared; the TiEmu measurements below remain tied
to their historical binaries and were not rerun for this revision.

The FFT_CONFIG exploration build prepares the selected forward close/wide geometry
while the player waits in Ramza's settled action menu or its CATALOG overlay. Sixteen
consecutive eligible input-free frames start the job. Input, walking/attacks, AI,
banners and rotation take priority. The complete tactical PC renderer additionally
uses only its spare frame budget; the configuration menu freezes tactics, CT and RNG.

The same transform, near clipping, triangle spans, reciprocal depth and visible mesh
edges run cooperatively. Spans yield between rows. The displayed image and actor
occlusion are unchanged:34 prepared/cold pictures, all four facings and both zooms and
scenery appearances match PC/native output exactly, including a moved/animated actor.

## Memory and cache lifetime

The final native job uses **20,284 heap bytes in four allocations**, largest13,600 bytes.
It stores only screen rows8..92: top8/bottom7 rows are always covered by the inspection
HUD. Packed20-byte colour/water rows avoid LCD stride padding; all useful pixels retain
native160x100 resolution and the original8-bit reciprocal depth.

Transformed vertices temporarily occupy the inactive Status buffer. Projection is
recomputed per visible face instead of keeping another3 KB array. ISO colour, unpacked
scene depths, cover masks and displayed planes remain intact. A partial job is cancelled
before Status/AT-list/turn depth overwrites that scratch. A matching inspection request
completes its job **before** clearing Status for FWork. A finished job needs no vertices
and survives ISO return. Current actor positions, masks and poses are rebuilt at entry.

The bounded native review omits unused full-tactical panel/window/status backing stores:
Work falls from43,546 to22,916 bytes (-20,630). Thus its new cache adds no net game-work
heap against the previous bounded review (-346 bytes before allocator headers). The
full tactical PC build retains its panels; its optional job adds about20.3 KB.
Allocation failure safely retains synchronous inspection. Full tactical TI packaging
is still a separate blocker; this is the compact interactive camera review.

Only Ramza's tile/facing, selected zoom/style and this run's immutable map geometry
key the cache. Solid architecture is supported. Opposite cameras and historical automatic/
dotted cutaway use ordinary rendering. Mismatches cannot consume stale geometry.
Native texturing coverage is unchanged: only matching offline-filtered fixture cameras
are textured. This does not establish general native perspective texture mapping.

**AMS lesson:** the initial33.5 KB allocation failed despite33,806 free bytes because
HeapMax was only10,418. Removing unused UI buffers and splitting allocations helped the
smaller timer program, but the larger interactive program still had only about21 KB.
The final20.3 KB layout handles that actual limit. Startup compaction runs before taking
archive/data pointers and locking the game's large allocations. Total free memory and
largest free block both matter; a host allocator does not validate either constraint.

## Reproduce and verify

`make camera-prefetch-check` rebuilds the configuration review first, then snapshots its
banks under x/prefetch/data. Ordered dependencies prevent asset regeneration during a
PC/native replay. Checks cover34 matching images, preserved ISO buffers/tactical state,
input yielding, camera/position/style mismatch rejection, allocation fallback, partial
completion and borrowed-Status invalidation. `camera-config-ti` compares488 consecutive
PC/native images, including420 idle-menu frames and both real rotation renderers.
The ordinary tactical/direct-camera suites and AddressSanitizer checks also pass.
`make camera-prefetch-filter-check` additionally checks the full PC filtered mapper,
including exact cold/prepared image equality and partial Status/turn interruption.

Full tactical native packaging remains unresolved. Run the bounded native fftcfg review
as described in CONFIGURATION.md; the current binary/source/bank evidence is under
x/config and x/prefetch. Physical TI measurements remain separate from TiEmu evidence.

## Datasheet-counted native CPU

These exclude grayscale, interrupts, presentation and waits. They are not desktop/GIF
FPS or TiEmu wall times. At the original facing with textured scenery and live actors:

| View | Cold CPU | Prepared CPU |
|---|---:|---:|
| Close | 1473 ms | 368 ms |
| Wide | 1536 ms | 165 ms |

Across16 cases: cold1239-1744 ms, prepared146-368 ms.
Maximum idle call:168,060 cycles (at12 MHz). Background preparation
takes248-375 idle calls in the96-credit control, including an input interruption;
the complete PC tactical menu's conservative policy takes848 frames on this fixture.
These counts are not a full-game TI frame-rate claim.

## TiEmu audit

The automatic prefemu() timer harness uses the same renderer, actual configuration page,
fixed-address plane copies, active grayscale and the usual32 FPS cap. Load it with every
archived x/prefetch/data bank on a clean Titanium boot. The256 Hz table reports cold,
prepared/reopened entry, total background preparation and worst displayed menu frame.
Always check the larger interactive fftcfg binary too: the first33.5 KB attempt passed
the timer but silently fell back to cold rendering in that larger executable.

Historical33.5 KB timer readings (before the final compact-cache layout): close1,847 ->
507 ms, wide1,882 ->214 ms; idle9.25/11.09 seconds, maximum menu frame35/39 ms. Those are
not evidence that the larger interactive program successfully allocated that old cache.
The old binary and metadata remain prefemu-before-compaction.89z and
tiemu-before-compaction.json. Current results and interactive validation are recorded
separately in x/prefetch/tiemu.json and interactive-capture.json.

## Final TiEmu results, 2026-10-17

Fresh private Titanium boot, explicitly12 MHz, grayscale/fixed-plane copies/32 FPS cap:

| Textured view | Cold | Prepared | Reopened | Complete idle preparation | Worst idle frame |
|---|---:|---:|---:|---:|---:|
| Close | 1,804 ms | 453 ms | 457 ms | 8,968 ms /287 frames | 42 ms |
| Wide | 1,871 ms | 207 ms | 199 ms | 10,750 ms /344 frames | 39 ms |

The larger64,695-byte interactive fftcfg program was separately cold-booted with all
archived banks, left idle for15 seconds, then entered through the real DIAMOND key.
Its actual LCD GIF reaches the perspective HUD at1.50 s of recording, after a deliberate
one-second source hold and the key tool's focus delay; the failed33.5 KB version reached
that header at3.20 s with the same driver. This confirms successful warm use in the
actual interactive executable; the table above supplies the independent timer values.
The GIF is unaccelerated20 Hz capture, not a measured20 FPS game claim.

Evidence: timer-results.png, timer-results-lcd.png, tiemu.json, timer-capture.json,
tiemu-profile.ini, interactive-ready-tiemu.gif and interactive-capture.json under
x/prefetch. Binary/source/asset hashes accompany the checks. All tests used a private
emulator; the user's desktop/profile were untouched. No physical TI was used.
