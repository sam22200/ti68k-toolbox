# Chapter I archive planning checkpoint (2026-11-24)

The promoted ABI99 core is701169 transport bytes,700273 stored payload/size
bytes. The matching pilot is295145 transport bytes; key
`10965478d3a2c348c22d2dea8e6ac51d`. Stored bytes exclude transfer headers; the
131072-byte Flash/VAT reserve remains. This is a plan, not a full chapter build.

| Current planning component | Bytes |
|---|---:|
| Dated13-map view estimates with112% calibration |421016|
| R4/R5 raw colours under declared per-map row/width budgets |822016|
| Other dated upper estimates |608000|
| Projected chapter |1851032|
| Allowance:2700000 -131072 -700273 |1868655|
| Planning margin |17623|

`packages/chapter1-plan.json` retains the cached source census and declares
per-map authoring ceilings of `min(202, source_content_rows +8)`. The eight
extra rows are an authoring allowance, not a measured final artwork bound.
Zeklaus explicitly uses352px/188 rows, adding6016 colour bytes over its
old320px estimate. Other maps retain their declared320px budgets. A352x202
colour allocation is invalid; the projection now rejects it. No scenery is re-ripped
or silently cropped by this audit. Old compressed colours remain in the
estimate, so their raw replacements are deliberately counted twice.

**Two depth adaptations remain required:** Sweegy/map74 has an estimated
19304 bytes (816 above18488); Fovoham/map72 has19176 (688 above). Zeklaus/map76
now preserves the complete336px mesh on its approved352px canvas; its old
width-crop requirement is resolved. Keep the measured18488-byte depth
capacity and42-frame actor cache. Resolve and visually review these depth adaptations
during content authoring; a flag or matching PC/TI checksum cannot certify
handedness or artwork. The tool deliberately exits1 while these gates remain.

```sh
tools/pyenv/bin/python games/fft/tools/chapter1_budget.py \
  --packages games/fft/campaign/x/core-abi99-release/packages/packages.json \
  --out games/fft/campaign/x/chapter1-archive-current/projection.json
```

The actual groups, SHA/key and per-map rows are checked offline. Full chapter
banks, installed Flash overhead/fragmentation and RAM are separate gates.
The installed core/pilot and diagnostic wide-map checks are documented in
INSTALLED_RAM_ABI99.md;16KiB free is not claimed. The current projection is
`x/wide-ram/archive-projection.json`; exit1 still reports the two depth gates.

## Historical checkpoint


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
