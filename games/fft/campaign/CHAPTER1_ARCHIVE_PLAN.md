# Chapter I archive planning checkpoint (2026-11-12)

The user requested a bounded search for the remaining roughly2KiB of RAM,
then progression if recovery was complicated. No simple saving was selected:
scene/depth, actor cache, navigation and background caches have active or
measured roles. The spare2682 bytes in the current companion block are a
reserved3072-byte rules envelope, not unused scratch to remove silently.
Further RAM refactoring is **deferred**, not a claim that16KiB was reached.
The installed diagnostic remains14058 free/max13042; production15171 is only
an estimate. Preserve these measurements and the current capacities.

## Reproducible host-only projection

`packages/chapter1-plan.json` records the dated2026-11-10 estimates for13
physical maps,20 battles and20 scenes, plus the cached census hash. The existing
readiness table supplies historical R2 view estimates; the cached census supplies
four cropped-depth figures per map. No disc extraction or109-map census is rerun.

Run against matching actual package groups:

```sh
make -C games/fft/campaign chapter1-archive-budget \
  PACKAGE_OUTPUT=x/archive-reach-colours/release
```

For future builds, point `PACKAGE_OUTPUT` at the fresh package directory. This
command only reads files and writes `x/chapter1-archive-plan/projection.json`;
it does not compile, run the emulator or access saves. It checks group hashes
against their manifest and validates the core with the existing R4 package/key
validator. A stale manifest/core cannot supply a misleading current size.

| Planning component | Bytes |
|---|---:|
| Dated13-map R2 view estimates |375907|
| Those views with112% scenery calibration |421016|
| Add all R4 raw colours:13*4*202*40*2 |840320|
| Other dated upper estimates |608000|
| **Projected chapter** |**1869336**|
| Allowance:2700000 -131072 reserve -405033 current core |2163895|
| **Planning margin** |**294559**|

The raw-colour addition deliberately retains all old compressed bytes, without
credit for their replacement. It is conservative for that format conversion;
it is **not an absolute bound on unfinished content**. The other content
estimates still need validation. The actual pilot chapter is291731 bytes,
not the full Chapter I pack. All13 cached cropped-depth sets fit20480; Zeklaus
still needs the documented width crop336 ->320. Per-map crop/art review is
separate. Production uses1px scenery without optional width masks.

Host checks cover the actual package projection, allowance overflow, depth
overflow and duplicate-map rejection. No battle code, layouts, ABI, save,
allocation or performance path changes. Native evidence from the preceding
archive-reach delivery remains applicable; no new TI checks were needed.
Evidence: `x/chapter1-archive-plan/verification.json`.

Next: automate the installed chapter route, preparing scripts and checking
logic on the PC first, with one targeted emulator milestone for real AMS/link
behavior. Full chapter assembly/resource audit, real Flash overhead/fragmentation,
RAM peak/contiguous/reentry certification and interface freeze remain distinct
unfinished work. The16KiB target can be revisited when a low-risk saving appears.
