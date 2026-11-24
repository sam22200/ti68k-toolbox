# Installed RAM and ABI99 promotion

2026-11-24. The user requested installed-RAM measurement and promotion. The
reviewed core checkpoint is now `x/core-abi99-release/packages/`, key
`10965478d3a2c348c22d2dea8e6ac51d`. `packages/core-contract.json` freezes these
exact groups and layouts. Earlier ABI97 and rejected candidate trees remain
unchanged. This promotes the core and existing pilot, not a complete Chapter I.

## Actual Titanium measurements

All measurements use the packed production entry and its matching29 images,
archived banks, HW3Patch and AMS3.10, in a private Docker TiEmu/Xvfb session.
Numbers come from `HeapAvail`/`HeapMax` and independently agree with the AMS
free-list decoded from captured RAM. The512-byte log ring occupies530 heap
bytes and remains included in the figures; no estimated logging rebate is used.

| Installed case | Free bytes | Largest allocation |
|---|---:|---:|
| Orbonne pilot |2420|2414|
| Gariland pilot, five deployed units plus source roster |1420|1414|
| C2 Zeklaus,13 borrowed diagnostic units |436|430|

These are settled battle snapshots, not a claim of a continuously sampled
minimum across every action or future encounter. The Zeklaus margin is very
small. **16KiB free remains unpassed and its difficult recovery stays deferred.**
Keep actor42, depth18488, full scenery/geometry and existing buffer lifetimes.
Other applications or unarchived variables can consume this margin.

The pilot completes confirmed scene skips, deployment, DEBUG battle settlement,
world Save, clean exit and Continue to the world. This hardware route checks
installation/lifecycle; ordinary fights remain covered by PC/sanitizers and
the preceding complete native Gariland replay. Zeklaus checks actual13-unit
state, both rotation directions and clean battle exit, using borrowed actors.
The supervisor regains the same185562 free bytes after each battle, with no
battle allocations retained. Eight installed payloads, including the packed
entry and all four module shards, equal the reviewed files and are archived.

## Fragmentation found and fixed before promotion

The previous candidate `2eea5b...` runs the pilot but fails the wide cold load:
58376 bytes are free, yet the largest allocation is53948, below the54334-byte
scene/Work request (plus allocator overhead). Bootstrap first allocated a
Gariland-sized scene, then pinned actors beside it. Freeing that scene could
not grow the resulting hole past the actor allocation. Host malloc and the
headless native runner did not reproduce AMS's locked heap placement.

Bootstrap now allocates State, depth and actor cache without a speculative
scene. RESOURCES10 allocates the first descriptor's exact scene/Work block.
The portable bootstrap follows the same lifetime. Shrink, failed growth,
retry and reentry remain supported; there is no maximum-sized scene retained
for narrower maps, extra buffer or cache reduction. Failure cleanup still
guards null pointers. ABI99/ModCtx400, State2278/Unit116/Work37790/FSV9 Save4684,
zone9118 and companion4132 remain unchanged.

Only RESOURCES26 changes among the29 module images. The resident cold setup
also changes; actual TTPack input is64725/64936, leaving211 bytes. The core is
701169 transport bytes, the existing pilot295145. New full CPU gates retain
the exact13-unit Zeklaus peaks339962/356832 and PC/native pictures905C/38B4.
Prior12-unit and Gariland CPU evidence is reused for unchanged hot paths.

## Evidence and next work

`x/wide-ram/verification.json` ties source hashes, package key, installed RAM,
CPU results and promotion together. Its `fixed-core/`, `fixed-map-check/`,
`fixed-render13/`, `fixed-pilot/`, `fixed-package-check/`, `pilot-fixed/ti/`
and `zeklaus13-fixed/ti/` retain the builds, logs, RAM dumps and drivers.
PC/sanitizers pass13403 map/allocation checks and all three ordinary pilot
flows with unchanged durable saves; package checks42 host/1023 C pass.
The preceding native8852-frame fight remains applicable to unchanged combat.
Earlier failures and the initial pilot run with an extra972-byte `heaplow`
test variable are retained separately; only private sessions deleted that
variable. No screenshot was needed. The user's live emulator PID662887 and
1267 protected-manifest checks (1256 distinct files) retain bytes and mtimes.
All private emulator/display processes are stopped.

`make core-contract-check` and pilot checks default to the promoted groups.
New package builds use `x/build-packages`; chapter-only packaging reuses the
promoted core by default. The old `x/packages` and `x/chapter1-ready` proofs
must not be overwritten. No commit or push is implied by this promotion.

Next: Sweegy/Fovoham depth adaptations, remaining source encounters/scenes and
field formations, then full chapter assembly. Chapter archive planning now
counts Zeklaus's352px rows and leaves17623 bytes of dated planning margin;
this is neither finished content nor installed Flash certification.
