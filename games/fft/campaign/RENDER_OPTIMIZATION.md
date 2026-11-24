# Human rendering and bounded HUD work

Latest follow-up (2026-11-24): [INSTALLED_RAM_ABI99.md](INSTALLED_RAM_ABI99.md)
records installed RAM and promotion of key10965478d3a2c348c22d2dea8e6ac51d.
Cold bootstrap now waits for the first descriptor before allocating scene/Work;
only RESOURCES26 changes among the images. Native13-unit CPU numbers below
remain exact. The original2eea5b... candidate described below is preserved as
CPU evidence, not the current installation group. The promoted groups are
`x/core-abi99-release/packages/`; actual free/max is2420/2414 Orbonne,
1420/1414 Gariland and436/430 Zeklaus13.16KiB remains unpassed.


2026-11-23. This implements the user's priorities1 and2 after the Zeklaus
profile. The shared ABI99 candidate passes the selected ordinary360000-cycle
gates at both12 and13 units. Approved C2 scenery, terrain, rules and durable
saves are retained. This is not installed-RAM or complete Chapter I certification.

## Measured result

All numbers are MC68000 datasheet cycles under `ti-cycles`, production100-row
drawing, with the same diagnostic placements and input scripts. Loading and
the six animated-turn frames remain separately classified; no budget threshold
or cold-frame classification was relaxed.

| Case | Units | Before mean / peak | Final mean / peak |
|---|---:|---:|---:|
| Zeklaus board |12|349628 /459576|282702 /331842|
| Zeklaus rotations, ordinary frames |12|346978 /432130|290152 /327594|
| Zeklaus board |13|357218 /453064|289724 /339962|
| Zeklaus rotations, ordinary frames |13|349239 /459644|307987 /356832|
| Gariland board |13|247963 /372140|218014 /281086|
| Gariland rotations, ordinary frames |13|not remeasured before|243444 /325234|

The tightest selected peak has3168 cycles of margin. This is a bounded replay
result, not a guarantee for every possible formation or real hardware FPS.
Cold Zeklaus loads/turns still reach about2.17M/4.31M cycles.

## Rendering ownership and cache lifetime

Humans skip optional monster-bank dispatch. `unit_blit.inc` borrows each human's
otherwise unused256-byte monster-cover scratch (`Cover.bc`), retaining the
same Work37790 bytes,42 decoded actor entries and18488-byte depth allocation.
It stores up to25 composed rows: shifted light/dark longs and an unshifted
opaque mask,10 bytes/row. A transparent endpoint lets a26-row source fit;
full-height poses, side clipping, flashes, water and loot use the ordinary
compositor. No actor cache entry is sacrificed or new heap buffer allocated.

`bvalid==3` owns human scratch;1/2 retain the monster cover meanings. Pose keys
are the decoded cache's actual keys, including mirrors and deferred animation.
Only the sub-word x phase affects stored shifts; vertical clipping happens at
draw time. Regenerating scenery cover invalidates human rows. A monster clears
the human owner tag before reading the same scratch. The odd `Cover.bc` offset
requires one alignment byte; compile-time checks protect its layout/capacity.
The native address-error experiment remains an intermediate failure, not a
passing proof. Warm blits process two rows per loop iteration.

During one unit-render pass, `render_poses` records human poses already resolved
by `actor_animate`. Only calls with decode budget may mark a slot: an attack
target that deferred a pose must be queried again. Reuse validates the remembered
entry's current key and updates its age. The mask is cleared before preparation
and after the final unit blit, and at chapter initialization. It is two resident
bytes, not a Work/State/Save field. Existing eviction and new-actor rules remain.

The rare water and raw monster-KO shaders stay out of line. Oversized human
poses, flashes and objects share one fallback compositor/ExtGraph call.

## HUD scheduling

The common Menu/Act camera case computes the same cursor-centred target in
resident code. This avoids loading BEATS on every update between RESULTS menu
chunks. Other camera modes retain the existing shared camera calculation.

A new window first stages its native RESULTS image, then builds its frame and
rows on later HUD quanta. PC uses the same schedule. The existing immediate
Math feedback exception remains. Enemy/Guest outlined labels are built one per
quantum: `tag_ok`0 means empty,2 means first label ready,1 means complete.
Completion reporting waits for both labels. Warm human draws receive a
conservative5k estimate discount; a row-cache miss/fallback is charged16k
before optional work. The360k hard gate remains unchanged.

Pixels after completion are unchanged. HUD/cursor preparation can complete on
different frames; it is not valid to require every intermediate whole-screen
checksum to equal the slower schedule. The reference comparison checks every
gameplay state and checks pixels when both HUDs are ready outside the bounded
12-frame input/rotation transition window. The static morph control has no HUD
and compares its quiet pictures directly. Two Gariland cursor differences at
frames88/89 are transient work scheduling; final pictures match.

## Package and interface

The larger hot renderer required moving the existing cold FMB1 validator to
PARTY16. Its pointer/length request reuses the context union: ABI98 ->99,
ModCtx400 unchanged. No executing image loads another image. The resident
loads PARTY after RESOURCES returns, validates each present body bank and
keeps the same missing/invalid-bank policy. This removes duplicated resident
validation code rather than increasing RAM or dropping input checks.

Final candidate: `x/wide-opt/final/`, key
`2eea5b271e52aee7272fc26f65b6faa0`. Core701247 + pilot295145 transport bytes;
actual TTPack input64727/64936 leaves209 bytes. All29 images match; zone9118,
companion4132/cold reader3546, State2278/Unit116/Work37790/ModCtx400 and
FSV9 Save4684 remain. Signature9615. The approved ABI97 installed groups and
global frozen contract remain untouched. The local contract is a candidate
snapshot only. Builder output conservatively sets `performance_accepted:false`;
the separate verification report records the measured replay gates.

## Verification and continuation

- `make unit-blit-test actor-cache-test`:1362 scalar-oracle checks and57 cache
  checks. The human-row oracle also passes ASan/UBSan and real68000, including
  every x phase, top/bottom/side bounds, pose/cover/owner changes and fallback.
- `x/wide-opt/pc-final/`:24 baseline/current-sanitized replays,8640 identical
  gameplay states and4756 identical settled pictures; source and driver retained.
- `map-check/`:13403 PC/sanitizer source-cell, depth/color, input, movement,
  width transition, allocation-failure/retry and reentry assertions. The later
  render-only refinements reuse these unchanged geometry/allocation proofs.
- `final-render12/`, `final-render13/`, `gariland13/`: hard native CPU gates
  above, plus PC/native final picture and identity equality.
- `pilot/`: New Game, scenes, deployment, ordinary Orbonne/Gariland fights,
  results, towns, Formation, Save/Continue and defeat/retry pass PC/sanitizers.
  One selected native Gariland fight completes8852 frames with all29 matching
  images, no God/skip, and all buffers released. Battle settlement retains
  the exact4684-byte Gariland save hash
  `c37b0544ac51bdcfa583c23c22ab35208c396ee45f85d3095173a08270cc7d4f`.
- `package-check/`:42 host +1023 C metadata/supervisor cases, PC/sanitizers;
  `final/contract/` records native layouts and all29 matching image signatures.

After normalizing only the ABI signature,25 images remain byte-identical to
ABI98. AI0/TARGETS12/PLANNER24 also change compiler output after the context
declaration; PARTY16 gains the validator. The selected complete native fight
uses the rebuilt set. All three final durable pilot saves equal the prior PC
pilot's saves, not only each other across sanitizers.

All paths above are under `x/wide-opt/`. Earlier failed experiments remain
separate. No TiEmu run or global contract promotion. Next is the selected
installed-RAM milestone for this wider candidate, then remaining map depth
adaptations and source encounter/scene authoring. The difficult16KiB hunt
remains deferred; do not shrink the measured caches or claim it passed.
