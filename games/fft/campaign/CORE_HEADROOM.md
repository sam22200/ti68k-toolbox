# Resident headroom before area forecasting

Current follow-up: [CORE_AREA_FORECAST.md](CORE_AREA_FORECAST.md) adds pure
per-target effect forecasts. The measurements below are the resident split checkpoint.

2026-10-28. Follow-up to [CORE_AREA_CT.md](CORE_AREA_CT.md). Existing FCD3/FGS1
chapter span validation now lives in the cold SAVE image. The portable loader
uses the identical pure helper in `../chapter_validate.h`. Accepted formats,
geometry rules, playable actions, visuals and save bytes are unchanged. This
does not certify the planned map/scene v1 format or finish the core.

## Ownership and retry

After acquiring the current descriptor/interface, resident chapter_load checks
the required archive pointers, invalidates mod_id and preloads SAVE. Only then
does it write descriptor/interface lengths into the existing four-byte input
union. SAVE4 returns a validated geometry offset or zero. The caller binds map
dimensions, actors and Work only after success. SAVE never loads another image.

Forcing a cold binding is required even on a same-chapter retry: a warm image
may still contain the previous descriptor/Work pointer. Transient request sizes
must be written after preload, because the template clears them on cold load.
No new callbacks, images, heap requests or map-ID exceptions are introduced.
ABI30 rejects29 despite unchanged ModCtx364, because SAVE4 is a new contract.

## Size and performance

|Measure|Before|After|
|---|---:|---:|
|Resident TTPack input|64927|64293|
|Headroom below64936|9|643|
|SAVE image|2842|3456|
|Largest execution image/zone|6204|6204|
|ffbm raw shard|33782|34390|
|ffbn raw shard|31632|31632|
|Private campaign group|504139|504213|

The recovered634 resident bytes do not mean a smaller archived package: moving
code changes compression, and the private group grows74 bytes. Nine images differ
only in the ABI signature63364. TURN/FORMULA retain their lengths but have compiler
instruction/branch changes; AI_SETUP becomes4970 bytes (-6). Controlled preprocessed
rebuilds removing only the new unused input-union view and restoring the old
signature reproduce all three historical images exactly. Their source logic is
unchanged. Actual AI/control, casting and picture checks are included in the
affected closure. All28 non-code banks are identical.
State2192, Unit112, Work45576, ActionPacket22 and FSV3 Save3138 remain unchanged,
as do all five allocation requests and their previously documented formulas.
The packed program input reduction is not a measured combined residency peak.

Native preload-only average33888/max40726 cycles cold, warm296; previous average
33419/max40726 and warm296. The larger SAVE copy increases the average cold cost.
Chapter loading now also executes this image before binding; no new copy or
validation is added to combat frames, AI thinking or casting. The area lifecycle
zones stay exactly127872 command/preparation,41372 impact and85376 maximum due
launch in the existing fixture. These are backend zones, not full-frame timing.

## Affected evidence

`make core-headroom-check` checks the existing FMB3 parser (130 PC/sanitizer/native
cases,00000047), actual context bindings plus three stale-context retries
(130 native checks,17ED9E8F), seven bad/missing/previous-ABI startup cases and
sixteen loader/capacity cases. Native/PC boundaries include Orbonne, Gariland,
13 actors, invalid counts/dimensions/lengths/version/geometry and allocation
failure. Existing capacity checks are reused; no geography or Jump census runs.

The historical FMB2 rejection fixture now packs one structurally valid current
image: combining every current image exceeds AMS's per-variable limit, which
is precisely why the production format uses two shards. Legacy magic rejection
does not require an oversized thirteen-image transport fixture.

Additional closure:353+5 owned-area PC/sanitizer/native lifecycle checks,
594 existing control/actual-AI PC/sanitizer/native checks15C96953,
Gariland81/Orbonne14 pilot flow checks with equal native saves, twelve existing
action/reaction frame hashes, eleven unchanged pilot images with original
timestamps, fresh native/host layout equality and unchanged standard group.
Detailed proof: `x/core-headroom/verification.json`. Live TiEmu is untouched.

## Next gate

Proceed with area forecast/AI and reviewed per-target/empty-spell presentation.
Retail area casts remain disabled. The643-byte margin is a bounded allowance,
not a durable solution for all future resident growth. Keep further cold code
inside images that fit the current zone; budget any zone increase explicitly.
Broader mechanics/campaign, formats/editors and C07/L4 full-frame/stack/combined
residency remain open. Jump refinement remains deferred.
