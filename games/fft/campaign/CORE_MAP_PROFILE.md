# Per-map layer specialization

Subsequent source recoil and focused-validation checkpoint: [CORE_KNOCKBACK.md](CORE_KNOCKBACK.md).
The figures and whole-suite results below belong to this preceding milestone.

2026-10-25, workstream4. The user accepted [CORE_JUMPS.md](CORE_JUMPS.md), then
requested disabling unnecessary upper-layer rules on single-floor maps. The
Decision marker is cleared. This specialization preserves that accepted image
baseline; source upper paths/raster cover and the overall production gate remain
open. No live TiEmu launch/input or standard bundle replacement.

## Retail reference census

`tools/map_layers.py` reads the local retail US disc. Its denominator is the
union of maps referenced by the480 ATTACK scenario rows and the active random
encounter routes. The random table is read through the original WLDCORE pointer
cell0x80096A40, pointing at0x80096A64:19 locations, three24-byte route rows each,
four progress tiers. Rows with a nonzero encounter chance AND mask contribute.
Source interpretation: `wldcore_map_roll_random_encounter.c`, read-only pinned
decompilation. Table, overlay, ATTACK and terrain hashes are in `maps.json`.

There are105 event-referenced maps,19 random-route maps (44 active routes), and
109 distinct maps in their union. No known test or unreferenced map enters the
denominator. All214 terrain variants of those109 maps are checked conservatively;
a map qualifies only if every variant does. This is static reference evidence,
including scenes, not a played proof of every conditional event/variant. It is
not a list restricted to battles offering a deployment screen (Orbonne would
be incorrectly dropped by that filter).

| Property across all variants | Maps | Share of109 |
|---|---:|---:|
|No upper tile where a unit can stop (blocked OR untargetable)|77|70.6%|
|No unblocked upper movement geometry|66|60.6%|
|No positive-height upper projectile span|64|58.7%|

Eleven maps have no upper stop cell but still have unblocked upper movement
records: MAP002,004,011,017,034,039,052,055,062,069,096. Untargetable is not the
same as absent geometry. Separately, MAP056,064,082 have no upper movement
records but retain upper projectile solids. The flags must remain independent.
For example, Orbonne skips upper movement queries but keeps its upper ray
collision records; Gariland skips both. MAP029/053 retain the complete path.

Per variant,140/214 skip upper movement,135/214 skip upper rays, and140/214
also omit the ordinary ceiling filter under the existing lower-bound proof.
The result is not a75% assumption baked into code.

## Runtime contract

The existing reserved Work byte becomes `surface_flags`; no new field or
allocation is added. `surface_profile` computes three independent properties
once when geometry is bound, outside the frame/AI query loops:

- `SURFACE_MOVE_UPPER`: at least one upper record is not blocked. Ceiling and
 intermediate-jump kernels otherwise run one iteration instead of two.
- `SURFACE_HIT_UPPER`: at least one upper base height is positive. The existing
 AI ray sampler queries strictly positive heights; a zero-height upper span
 cannot intersect it. The general full kernel still supports other heights.
- `SURFACE_ROOM`: a potentially active upper record or a lower edge/water/body6
 bound above127 requires the ceiling filter. Otherwise that whole filter is
 bypassed, as at the preceding source-surface checkpoint.

`SURFACE_GRID_UPPER` separately records actual compact upper-node presence when
binding a tactical grid; `edges` only enumerates both destination floors when
that bit is present. A lower-only custom grid now avoids phantom upper lookups.
Custom grids with upper nodes retain the conservative endpoint ceiling path.
Grid rebinding clears old source flags; source rebinding validates first and
invalidates the edge cache. Archive geometry is immutable in this path; future
terrain mutation must reclassify/invalidate, not retain stale flags.

The required lower-floor slope/height/jump tests remain. Disabling an absent
upper layer does not mean accepting jumps through a lower wall or slope.
`surface_{ceiling,hit,jump}_layers` share the existing full behavioral kernels;
full two-layer wrappers remain the oracle controls. No map IDs or encounter
exceptions appear in the runtime. FGS1/SGE1/FCD3 data sizes and FSV3 saves stay
unchanged. Matching FMB2 modules use ABI5.

## Verification and preliminary costs

The census produces972 unique (whole-map profile,column) pairs. The profile
suite checks every byte-valued movement height0..255, four directions, body4/6,
and positive ray samples against the full kernels.3,234,825 checks, hash
`12C1CCBB`, agree PC, full ASan/UBSan and actual68000. Explicit classifier
boundaries cover blocked-but-solid upper records, unblocked zero-height upper
records and the lower127 ceiling bound. The original9294/47264 query evidence
continues to define the full kernels; the new checks establish equivalence of
the reduced paths, not a second independent PS1 behavioral claim.

The156 actual jump reach/path checks,56 source bridge/ray checks,3181 full-grid
checks and1846 boundary checks pass.298 changed retail edges still match original
MIPS, preserving the accepted jump correction.37 campaign and24 opening/scene/
control PC/native replays pass. The complete flow PC fixture retains81+14
assertions; native variants cover all of them plus exact post-handoff save
bytes, repeated receipts and trophy overflow.

The instrumented late-flow checker exceeded one TI variable by100 bytes. Its
native setup/aftermath and result confirmations now run in companion fixtures;
the results fixture still executes the same prefix, while the aftermath fixture
checks every prefix assertion. No production behavior or assertion is removed.

The packed resident engine also reached TTPack's64,936-byte input limit. Result
pages now use an eighth existing-style overlay, separate from Status/config.
This reduces the resident overlay zone from7406 to6074 bytes (-1332), without a
new heap block or overlay-to-overlay call. Result rendering and menu behavior
remain byte-identical in the accepted picture/state tests. Loader count and ABI
checks require all eight rebuilt images; prior banks fail validation.

Final cold Gariland graph measurements (same source-filtered graph):

| Control | MC68000 cycles | CPU at12MHz |
|---|---:|---:|
|All ceiling/upper queries forced|3590180|299.18ms|
|Cached ceilings, upper queries forced|2088748|174.06ms|
|Cached ceilings and absent upper layers skipped|2079094|173.26ms|

The incremental upper-layer skip saves9654 cycles, about0.80ms (0.46%) in this
cold graph fixture. The earlier ceiling bypass accounts for most of the full
control's additional cost. Do not describe the whole speedup as newly gained
by this patch or infer a large FPS gain. These are datasheet MC68000 CPU
measurements, not TiEmu timings, warmed keyboard latency or LCD FPS.

Final private `x/core-single-floor/fftcampaign.89g`:469834 bytes,42 variables,
SHA256 `6d2422b6ade18dc56cb3a72d979126abba8ed50f1d970d1973cb16d581cb597a`.
Delta +734 bytes against the469100-byte source-surface checkpoint, including
both the subsequently accepted jump rule and this specialization/module split.
The eight raw module images are `[5096,3504,3656,5734,6074,1042,4414,4104]`;
the bank is33726 bytes before the AMS trailer. Work45514, Unit80, State1586,
Save3138 and the five battle allocation requests are unchanged: Gariland116094
bytes, maximum nodes/current202-row scene121182. The resident overlay zone
shrinks1332 bytes, but overall code residency also changes; no net peak-RAM
reduction is claimed. All48 decoder repetitions have zero cycle delta. The only
planning alert remains the existing large `ffca` variable. Combined maximum
residency, stack/heap high-water and future scene extrema remain unmeasured.
All169 pictures accepted after CORE_JUMPS remain byte-identical. Compiler review
finds no long/float helpers in the new geometry/profile/edge sections.

## Reproduce

```sh
make -C games/fft/campaign core-profile-check       # focused default
make -C games/fft/campaign core-profile-deep-check  # explicit full certificate
make -C games/fft/campaign core-jump-check
```

Use the existing host-only pinned Unicorn dependency for original queries.
`maps.json`, `results.json`, `battle/results.json`, `image-regression.json`,
private group and native/package reports live under `x/core-single-floor/`.
`before/` preserves the469100-byte package report and the169 images accepted
by the user after CORE_JUMPS. The package delta includes the accepted jump
implementation, which had not yet been bundled on its own.

Build native programs serially. Also serialize host save tests and campaign
replays sharing temporary files. Keep commercial data/oracle fixture banks local.
Continue source endpoint/long-jump/actor-span policies and upper-floor raster
cover before declaring source-correct layered gameplay or opening map production.

Subsequent validation policy: [CORE_TESTING.md](CORE_TESTING.md). The focused
default has2574 checks; the historical3.23M certificate is retained, not rerun
for every core edit.
