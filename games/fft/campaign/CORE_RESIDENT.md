# Resident code capacity and actor-column evidence

2026-10-25, workstream4. After [CORE_SPAN_SEARCH.md](CORE_SPAN_SEARCH.md).
**Status: the wider-cost search now fits in the standard battle engine. Longer
links and actor-record generation are still not connected to playable movement.**

## Capacity change

The largest code image determines the permanently reserved resident module
zone. SAVE previously combined save writing/rewards with encounter construction
and party loading. These responsibilities now use separate images:

|Module|Previous image|Current image|
|---|---:|---:|
|SAVE: save writing/rewards|6074 bytes combined|2726|
|PARTY: encounter construction/party loading|inside SAVE|4064|
|SCREENS: Status/configuration|3656|4098|
|Largest image/resident zone|6074|5734 (BEATS)|

`status_input.inc` holds the unchanged Status keys; the PC includes it directly,
the calculator runs it inside the existing SCREENS image. The next Status draw
uses the same image, without a new per-frame image alternation. The ordinary
movement algorithm stays resident and unchanged. Stat calculation also stays
resident: level-ups do not load a save image. Initialization callers route to
PARTY; SAVE handles writes/rewards. Neither path evicts an executing AI overlay.

There are now9 FMB2 images, ABI6. Context/state/work/save fields are unchanged;
count/signature checks reject old banks. Rebuild the program and module bank
together. The module generator checks every image for unresolved symbols and
relocations. Reserving a smaller zone saves340 resident bytes; it does not imply
smaller total archive storage, since another image adds code/context overhead.

`FFT_NAV_SPANS` is now enabled for chapter builds. The costs1..5 routine is
linked and callable by the tested spread API. The game builder still generates
ordinary/one-gap links and requests the old fast search. Do not claim that
long jumps become playable merely because their queue now fits.

## Measured package and limits

Private `x/core-resident/fftcampaign.89g`:471882 bytes,42 variables, +1438 versus
470444. SHA256:
`4e01e2f39cd210eadedc90d12c49799e08127cf0915d1929c7bfe575d16ac504`.
All28 non-code variable hashes match the preceding private package. The standard
playtest group and live TiEmu remain untouched.

TTPack input is64831 bytes, leaving105 below its64936-byte input ceiling.
The separate unpacked TI variable payload is64829 bytes; the packed input also
contains its length word. The program builds, but this is a tight margin; further
resident growth must be budgeted before enabling link/body integration.
`core_budget.py` now reports TTPack input size/headroom separately from compressed
archive bytes and flags headroom below512. A known100-byte host packer trial
verifies the little-endian length/TP header. Three focused package-parser tests
cover payload/transport distinction, malformed groups and the new length field.
No map census or compressed asset-stream benchmark was rerun for this report.

Fresh native layout: Search1054, Work45514, Unit80, State1586, FSV3 Save3138.
No extra heap block or workspace. Gariland's five requests remain116094 bytes;
256 nodes/current202-row scene121182. These are layout formulas, not measured
full supervisor/code/grayscale residency, stack or fragmentation peaks.

## Focused integration evidence

- Isolated PC Status alias-restoration and save/KO/party tests pass. They use
private temporary save directories.
- Actual PC/68000 campaign flow passes81 Gariland and14 Orbonne checks, including
setup, before/after battle staging, results, settled save bytes, repeated receipt
and stock overflow. A matching new bank exercises initialization in PARTY and
writes/rewards in SAVE. ABI6 bank validation checks all9 image headers.
- Status input/render replay agrees in90 executed frames: checksum FE3C and
state hash224902255. The reviewed Status and range pictures remain unchanged.
- Actual fast/wide Work-bound searches retain the previous1219/1065 checks and
hashes AF1961E0/435FD706 on Gariland/Orbonne. Budgeted calls and queue/scratch
ownership transitions retain identical costs/predecessors. The search kernel
itself did not change, so the27592-case check certificate was not rerun.
- Only the two affected range views and the Status replay were checked. There
was no full font/management/profile/campaign sweep, and no TiEmu input.

Artifacts: `x/core-resident/logs/`, `flow/`, `layout.json`, `package.json` and
`verification.json`. The9 image sizes and raw/packed input sizes are recorded.

## Actor-column rule, source checked before runtime integration

`../tools/jump_actor_oracle.py` executes unchanged original intermediate-column
MIPS at0x8017622C (684 bytes, SHA256 prefix551a8a31fcf7897c). It supplies explicit
frontier flags, target-panel record indices and two7-byte movement records.
The record producer is NOT executed, and these are not full played jumps.

22 named cases in four facing directions give88 original queries. The pure
`../jump_actor_geometry.h` prototype agrees on PC, full ASan/UBSan and actual
68000:99 checks, hash F00000C2, including no-actor compatibility with the existing
terrain helper. This header is not included by the shipping battle engine.

Observed rules:

- Both lower and upper actor records participate. Normal allied bodies are
ignored; other-team bodies may block by standing/top spans. Frontier4 indicates
an occupant; frontier8 changes whether its body record participates.
- A blocked frontier with an occupant rejects immediately, including allied or
mountable records. A blocked unoccupied terrain record is skipped.
- Endpoints use strict interior comparisons and a containing-span test. The
standing-height+3 check can block even when a short actor's top only touches the
mover's feet. Different mover body sizes change the result.
- Terrain and body spans contribute to one minimum-low calculation. A synthetic
byte-contract pair isolates this reduction: testing terrain and actors as two
independent final Boolean checks is not equivalent. That pair is explicitly not
a played actor placement.

```sh
make -C games/fft/campaign core-jump-actors-check
```

Next: choose compact bounded storage for up to32 links, construct actor records
from actual unit/status/movement policy, verify wet long-span costs, and combine
those with the queue before enabling jumps. Single-floor terrain classification
must not erase a possible actor record on another layer. Raster/animation,
action/status, maximum combined residency and production formats remain open.
