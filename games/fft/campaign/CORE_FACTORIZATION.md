# Shared identity data and compiled-overlay factorization

2026-11-07. Combat coverage stays complete at201 generic actions and78 R/S/M
consumers. This change removes repeated archive records and compiled helpers;
it does not add a combat family, mounts, content or a new renderer.

## Measured package size

| Measurement | Bytes |
| --- | ---: |
| Previous composition reported to the user |620,995|
| Same pre-change sources, fully rebuilt |622,372|
| Fully rebuilt optimized package |611,132|
| Controlled net saving |11,240|

The controlled saving is1.81%, including indexes, new access/validation code
and compressed program differences. The new package is596.81KiB across44
variables. The earlier composition reused several older packed menu programs;
its difference from the new package is9,863 bytes. Do not describe the raw
record savings as the net package saving or mix those two baselines.

## One copy of portraits and generic names

FCP2 stores46 unique32x29 portraits, each with unchanged light/dark/transparency
planes, the existing ten speaker aliases, three tiny portrait-ID directories
(Gariland6, Orbonne8, management41), and one510-entry generic-name directory.
The old61 portrait uses still resolve byte-for-byte to their reviewed picture;
repeated names retain their order, spelling and gender-specific index.

All consumers return pointers into the archived bank. No portrait/name decode,
expanded bank, per-unit buffer or new battle allocation is introduced. Dialogue
speakers retain their portraits and aliases. Management's FMG3 keeps its job,
item, icon, actor and font sections, replacing its portrait records with IDs and
moving generic names to FCP2. FCD4 retains the52-word encounter header; its
portrait field now points to a directory in FCP2. Old formats are rejected.

Identity banks fall from59,753 to50,161 raw bytes:9,592 saved after directories.
The original9,700-byte duplicate pool needs108 bytes of extra metadata/indexing.
The pool generator reads the original source data, and the independent reference
is pinned to SHA2565a67bc8d4612e5a4a79154e0bf7129aa0fb98bc4b368ca42d6b72b0dac59c663.
Generators rebind chapter offsets when aliases move and retain idempotent writes.

## Compiled helpers

The common applicability query now runs through the existing target_query
callback. Its criterion is16-bit; bit15 selects an ability applicability query,
while255/254/253/252 retain validity/reaction/critical/Jump semantics. Invalid
ability IDs return false. Local source helpers remain for the portable build;
native images use the callback without loading another image or consuming RNG.

The module builder first compiles to discover GCC's arithmetic libcalls, then
compiles again with exactly the required C forwarding wrappers. It never edits
instructions or adds authored assembly. Undefined symbols still reject an
export. Make marks verified exports complete even when their bytes are unchanged;
otherwise a newer resident-only input or unchanged generated contour header can
make every test rebuild all26 overlays. This changes generated build timestamps,
not protected historical artifacts. The two-pass mask has focused parser checks and actual native coverage.

Three raw module banks shrink from56,638/62,736/55,730 to55,616/61,352/55,200:
2,936 bytes saved. ModCtx remains348 bytes, Unit112, State2224, Work37460 and
Save3202/FSV4. ABI78 signature0890 rejects previous ABI77 shards. One execution
zone remains in the resident program and shrinks from8,880 to8,690 bytes.
The actual TTPack input is64,913/64,936, leaving23 bytes. A shared-helper change
must check both this limit and frame time; further indiscriminate factoring is
not part of this delivery.

## Validation

The identity carrier passes589 checks on PC, ASan/UBSan and actual68000,
including61 complete portrait/mask records,510 names and12 malformed-pool cases.
Fifteen management input/state/save replays agree on both CPUs; all fifteen
pictures are byte-identical to the pre-change build. Twenty deployment/scene
replays agree PC/TI. Gariland81/AE9CFD31 and Orbonne14/2CD70391 preserve rules,
pixels, exact settled FSV4 bytes, repeated receipt and full-stock conversion.

Thirteen-actor board/rotation/Status/equipment keep checksums0546/ECF7/A87E/AA26.
Ordinary peaks are336,762/336,762/330,010/353,782 cycles. Planner general/area
peaks are294,582/342,512, below360k. Total planner work grows slightly (about
0.2% general and0.5% area in this fixture); these bounds do not claim a universal
speedup. Cold loads and rotations retain their separately measured latency.
Runtime heap remains112,226 bytes, largest block53,620; all four render carriers
return their allocations to zero. The48-frame actor cache remains unchanged.

All100 suites pass:99 from the frozen-production full run and the Geomancy
transport recheck. That checker initially exceeded its TI variable by38 bytes;
two contiguous reference halves retain all6,403 math assertions and recombine
the same229FDD40 digest. The full8,927 checks still recombine4BF14699. No game
code changed for this split. The actual packed13-actor battle starts on an
isolated Titanium with archived FCP2/FMG3 banks; the startup screen was inspected.
Repeated-exit heap was not inspected in this run. The private emulator/X server
have stopped. Evidence is in `x/core-factorization/verification.json`. Previous proof/save
bytes and both timestamps, the standard groups and the user's live emulator
remain protected. Private deployable output is `x/core-factorization/after.89g`.
