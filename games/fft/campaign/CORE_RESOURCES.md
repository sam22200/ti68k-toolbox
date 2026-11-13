# Shared campaign resources and shipping deduplication

FKO1 update (2026-11-13): [monster Dead art](MONSTER_KO.md) is optional active-
chapter `ffko`, shared by all encounters. Its direct job/pair/frame indexes pool
equal records, job aliases and mirrors. The reviewed bank is2876 bytes/11
physical poses. Packaging validates this family, bounds reserved cache keys
and retains the whole-bank duplicate audit; no core/pilot copy is added.

FCD5 update (2026-11-13): [traps/source-map identity](TRAPS.md). The descriptor
adds one word; shipping normalization retains the existing exact sharing pass
and all live encounter payloads. FCD4/ABI80 measurements below are historical.

2026-11-07. This extends [core/chapter packaging](CORE_PACKAGES.md). Combat
coverage and FSV4 are unchanged. Apply these rules to every arena, dialogue,
introduction, interlude and menu, including different encounters in one chapter.

R4 update (2026-11-12): [archive reach colours](CORE_REACH_ARCHIVE.md) stores
one raw colour payload per pooled physical view, with compressed depth. It does
not add a resource family or retain a duplicate compressed colour bank. FRF1
aliases and the mandatory exact sharing audit still apply; core spec/manifest
keys require matching R4 core/chapter builds.

## Ownership rules for future content

1. Classify a resource by reuse, not by its first consumer. Rules/catalogues,
   identities, interface artwork and engine lookup tables belong to the core.
   Chapter-specific characters, scenery and story belong to the active chapter.
   Do not move the entire cast of future chapters into the permanent core.
2. Pool chapter resources across **all** battles and scenes before serializing
   them. A character has one physical frame bank; per-encounter pose tables
   contain IDs/mirror bits. A backdrop, title, beat block or arena view reused
   elsewhere has one physical payload. Different unit definitions, objectives
   and dialogue remain encounter/scene data; reuse their shared art directly.
3. Refer to shared resources with bank names, IDs and even byte offsets. Never
   copy them into an arena bank merely to simplify its loader. Resolve references
   on entry and keep direct archive pointers during frames. Reacquire pointers
   after a child or a save/archive operation. No new per-frame lookup, extra
   decompression or expanded whole-chapter RAM pool is allowed by this policy.
4. Deduplicate exact bytes offline. Preserve order, spelling, actor placement,
   source handedness, timing and reviewed pixels. Speaker portraits remain
   mandatory. Do not merge merely similar artwork or discard depth/masks.
5. Measure **core + one installed chapter**, including indexes/reference code;
   moving bytes between groups is not a saving. Check actual TTPack input,
   the static execution zone, heap and affected native frame costs. Performance
   takes priority over a few saved bytes.
6. Extraction/oracle controls may retain their original layout for reproducible
   offline conversion. They are not installation artifacts. Normalize at the
   packaging boundary and test the resulting installed bytes, not just raw
   `data/` files. Never distribute `source.89g`.
7. Every new resource family must join the sharing pass/audit and gain a bounded
   reuse fixture. Do not silently add another independently generated copy.
   An unsupported duplicate bank fails packaging with its owner names.

## Implemented shipping pass

`tools/chapter_resources.py` runs automatically from `chapter_packages.build`
for both fresh builds and content-only builds against an installed core:

- The identical6,858-byte engine-art/lookup tail formerly appended to every
  first map view lives once in `ffcu`. All maps borrow that core tail. Existing
  per-frame tables, masks and access offsets are byte-identical. The validated
  per-view-tail fallback exists only for old extraction/test controls.
- Encounter interface banks retain terrain records, fixed statistics and their
  indexed unique names. Converted UnitDefs and water-glint coordinates already
  live in FCD4; obsolete local ability/skill-set tables, old UnitDefs and the
  extra Orbonne water copy are dropped. FCD4 offsets are rebased; its active
  unit, geometry, water and reward data stay byte-identical.
- The unused756-byte Orbonne title strip is removed when FQF2's accepted shared
  glyph renderer supplies that title. Explicit legacy-serif controls keep it.
- Byte-identical views, pose tables, encounter interfaces/descriptors, battle
  story banks and scene sets become single-hop **FRF1** references. A second
  encounter can therefore reuse its arena while supplying its own units/script.
- Scene payloads are interned across the chapter: pictures, titles and complete
  beat/card/overlay blocks. The serializer chooses inline FSC1 when cheaper,
  otherwise FSC2 plus an FSP1 pool (`ffsp`, or `ffsp0..9` when split). Pools never
  exceed65,512 raw bytes. Repacking already optimized data is deterministic and
  idempotent; a smaller continuation pack can inline unused pools again.
- `resources.json` lists source/shipping sizes, reference targets and scene
  pools. The audit rejects identical unreferenced shipping banks of32 bytes or
  more within the installed core+chapter. This is an exact-bank/declared-scene
  audit, not a claim that every arbitrary substring of every format is unique.
  Small padding, distinct geometry representations and intentional lookup
  tables are not blindly compressed at runtime.

Actor frames, mirror equivalence, portraits, generic names and common text/UI
were already shared; they remain shared. This pass does not remove dialogue,
animations, terrain layers, camera orientations or visible detail.

## Formats and loading

FRF1 is16 bytes: magic4, canonical variable name8 (NUL padded), big-endian
offset2 and span2. Offsets are even; spans are nonzero and bounded. Targets must
belong to the same installed chapter or its declared core. Chained/cyclic,
missing, malformed and out-of-range references are rejected. `resource_file.h`
returns a borrowed pointer and a raw size, consistently excluding the AMS
six-byte `dat` trailer on both CPUs. Only cold consumers opt into this format.

FSC2 retains FSC1's pose-bank name6 and step count2, appends a resource-bank
name8, then uses the same8-byte steps. Blob offsets are relative to the shared
resource bank, while step offsets remain in the scene. The player caches both
bases at entry. FSP1 is magic4, blob count2, `(offset2,length2)` directory entries,
then even-aligned exact payloads. Its directory allows deterministic rebuilding;
the runtime renders through direct pointers and allocates no pool buffer.

ABI80/ModCtx348/signature0A92 has27 executable images. RESOURCES26 owns cold
encounter load10 and route query11; it returns before PARTY12 executes. This
keeps reference parsing out of hot images and the existing8,690-byte execution
zone. Unit112, State2224, Work37460, Save3202, five battle allocations and the
48-frame actor cache remain. The image pointer array adds4 static bytes;
the scene player caches two additional pointers. Hardware paths are untouched.

An ABI79 core cannot run these packages. Rebuild the core once; the builder
rejects an older core instead of changing a supplied core during a content-only
build. Subsequent chapter builds preserve the matching core byte-for-byte.

## Reproduce and verify

```sh
rtk proxy make -C games/fft/campaign packages PACKAGE_OUTPUT=x/chapter-resources/release
rtk proxy make -C games/fft/campaign core-resources-check PACKAGE_OUTPUT=x/chapter-resources/check
rtk proxy make -C games/fft/campaign packages-routing-check PACKAGE_OUTPUT=x/chapter-resources/routes
```

`check_chapter_resources.py` checks semantic bytes, idempotence, shared-arena
aliases, different dialogue on one background, bounded multi-bank pooling,
duplicate rejection, malformed references on PC/sanitizers/68000 and identical
intro/chapel/Academy/transition pixels. `check_flow.py --data-dir PATH --native`
checks the actual installed normalized banks, including FSV4/rewards/overflow.
Keep all outputs and temporary saves private; preserve historical proof/save
bytes and both dates. No emulator run is required for this archive-only change.

Task evidence and the installation groups are in `x/chapter-dedup/`; the pilot
remains incomplete Chapter I content. Measurements are recorded in its
`verification.json`, with source hashes rather than a new all-family claim.
