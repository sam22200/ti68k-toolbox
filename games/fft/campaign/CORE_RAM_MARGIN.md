# Battle RAM: bounded grayscale target (2026-11-12)

Historical first RAM lot. The subsequent [archive-colour delivery](CORE_REACH_ARCHIVE.md)
saves another8638 production bytes (Work29282, input57275, ABI87). Its real
installed supervisor diagnostic leaves14058 bytes free; the16KiB gate remains
open. The direct-caller measurements below retain their original context.

The first RAM follow-up is delivered; the16KiB readiness gate remains open.
The old11.4KB free figure predates the companion and program growth. Do not
reuse it as the current installed battle margin.

## Change and measured gain

`native_battle.c` opts into `RT_DRAW_ROWS=100`. Physical plane geometry remains
240x128 with30-byte rows; the fixed-address hidden target holds only100 rows
per plane. Other programs retain128 rows and their existing GrayDBuf behavior.
The compact color path requires `RT_GRAY_COPY_PRESENT`, does not initialize or
retrieve GrayDBuf planes, and copies3000 bytes per plane after the same switch
wait. GrayOn still owns its full visible planes and the interrupt driver.

Sprite clipping advances source rows at the top and shortens the bottom;
partially visible text preserves its last LCD rows. Clear/fill honor the
configured bound. Battle rotation clears and result copies touch100 rows.
The host background-cache diagnostic also honors the bounded allocation.

| Production quantity |128-row control|100-row target|
|---|---:|---:|
| Hidden allocation request |7688|6000|
| Actual malloc block, from AMS heap |7690|6002|
| TTPack input, from packed header |57735|57731|
| Headroom against64936 |7201|7205|

Net simultaneous production storage saving: **1692 bytes** (1688 hidden +4
resident). ABI86, zone8698, Unit112, State2224, Work37464, FSV4/3202,48 actor
frames and all game allocation envelopes remain. Compared with the preceding
Sweegy build, only TURN1 and RESULTS7 image bytes change; logical image sizes,
three shard lengths63782/61710/57470 and the generated header remain identical.
A later installed release needs a rebuilt core and matching chapter metadata.
Existing groups and installed states were not replaced.

`tools/headroom.sh` now includes the two-byte length word consumed by TTPack.
Older57733/7203 reports counted only the variable payload: their true packed
input/headroom was57735/7201. The new payload is57729, packed input57731.
The4-byte improvement is independent of this reporting correction.

## Current accounting and hardware boundary

`probe_core_layout.c` and `tools/core_budget.py` now count the real fixed5120-byte
navigation envelope,8698-byte heap module zone and3072-byte companion. The old
report described obsolete per-map NAV_* allocations and omitted both code
blocks. The seven game requests total106596 bytes on either pilot map or the
maximum map. Add the6000-byte hidden request, GrayOn storage, allocator overhead,
program/supervisor/stub residency, stacks and AMS activity for actual use.
Navigation allocates once; there is no old-plus-new growth transient.

One private Titanium AMS3.10/HW3Patch session ran instrumented packed128-row and
100-row controls through an AMS caller, using private archived banks and no
core/pack manifests (legacy fixture routing). RAM/log reads show:

| Instrumented checkpoint |HeapAvail|HeapMax|
|---|---:|---:|
| Before physical control |189418|65520|
| Physical battle ready |7934|6918|
| Before compact control |189278|65520|
| Compact battle ready |9606|9600|
| After compact exit |189278|65520|

The ready values differ by1672 bytes because the instrumented compact resident
is16 bytes larger, unlike the4-byte production reduction. The caller's initial
140-byte before/after difference is recorded, not silently normalized away.
This includes a530-byte log variable and a diagnostic caller/resident; it is
**not** a fresh installed supervisor-flow peak/stack certification. The compact
exit restores its pre-call free heap. A keyboard rotation reached rot1, grayscale
rendered correctly, and one screenshot was reviewed/published. The private
emulator, image/profile and X server were isolated and stopped afterwards.

## Validation and remaining work

`make battle-ram-check` runs reference128-row, compact100-row and ASan/UBSan
builds on PC. Guarded allocations check sprites8/16/32 (masked/opaque), top,
bottom and horizontal clipping, both fonts, fills, real battle rendering,
rotations, Move/cancel, Status/equipment, defeat results and settings. The100-row
test allocations catch writes and sanitizer reads beyond their boundaries.
Existing renderer signed-shift conventions use the same `-fno-sanitize=shift`
exception as other battle renderer tests; other UBSan failures halt the run.

Hashes: primitive D062D327, Gariland CA08B690, Orbonne50686EAC. All three PC
variants agree. `check_battle_planes.py --native` adds only two selected68000
cases: primitive and Gariland; both agree. Runtime PC self-tests remain green.
One13-actor last-unit equipment replay retains AA26 on PC/68000, with ordinary
update+render peak355370 cycles (previous351228), average145865. These frame
zones exclude presentation copies, as in the preceding measurements.

Evidence: `x/battle-ram-margin/`, especially `planes-results.json`,
`production-sizes.json`, `after-layout.json`, `capacity/results.json`, hardware
logs/heaps and `verification.json`. No combat-family sweep was repeated.

Next: recover further RAM, starting with ring-backup row spans while preserving
their capacity and avoiding cold re-decode stalls. Their width-mask/rotation
scratch aliases also constrain the gain. About2.5KB from that proposal alone
will not close the current gate. Then measure the actual installed supervisor
flow and peak contiguous heap, before claiming16KiB on every accepted map.
