# Shared core and replaceable chapter packages

2026-11-07. This is the active distribution checkpoint after
[combat closure](CORE_FINAL_COMBAT.md) and [factorization](CORE_FACTORIZATION.md).
The combat inventory stays complete: 201 actions and 78 R/S/M consumers.
The change concerns loading, distribution and content ownership. Current
shipping normalization and ABI80 supersede the original ABI79 loader below;
see [shared resources and authoring rules](CORE_RESOURCES.md).

## What is installed

| Package | Ownership | Current transport bytes |
| --- | --- | ---: |
| `fft-core.89g` | Supervisor, packed engine/scene/menu programs, executable overlays, shared catalogue, identity pool, management/title/interface/weapon resources | 393,134 |
| `chapter-01.89g` | Active chapter manifest, encounter descriptors, maps/views/depth, scripts/dialogue, world-map progression, actor frames and pose tables | 213,249 |
| Existing saves/configuration | User-owned; present in neither package | Unchanged |

Both groups have 23 variables. Their installed variable payloads plus size words
total 605,207 bytes. Transport bytes include PC transfer headers; neither
measurement includes all AMS VAT/archive metadata, fragmentation or other
installed programs. The builder defaults to a **planning** budget of 2,700,000
bytes with a separate 131,072-byte reserve. This is not a measurement of free
Flash or proof that every future chapter fits. Runtime RAM is a separate limit.

The first pack is explicitly an **incomplete Chapter I pilot**: Orbonne, Academy
and Gariland. There is no newly authored Chapter II. The continuation-only pack
in the tests is a QA fixture, not shipping campaign content.

`packages/core.json` owns shared variable names; `packages/chapter-01.json`
owns chapter data. The builder rejects overlaps, executable code in a chapter,
save/configuration members, missing banks, conflicting folders and encounters
outside the compiled envelope. `ffca`/`ffcb` belong to the active chapter: its
encounters/scenes pool frames once, and the next pack may replace them. FCP2
portraits and generic names remain shared, with no per-chapter identity copies.

## Build and use

Build the core and current authored pack with existing generated assets:

```sh
rtk proxy make -C games/fft/campaign packages
```

Outputs under `campaign/x/packages/` are `fft-core.89g`, `chapter-01.89g`,
`packages.json`, `resources.json`, `INSTALL.md` and explicit core/chapter variable lists.
`source.89g` is a build intermediate, not an installation group. The old
`make bundle` still provides an optional combined group with both manifests;
`make packages` does not overwrite the historical standard combined group.

After generating a chapter's data, build its pack against an existing core
**without compiling the engine**:

```sh
rtk proxy make -C games/fft/campaign chapter-package \
  PACKAGE_CORE=x/packages/fft-core.89g \
  PACKAGE_OUTPUT=x/new-chapter \
  PACKAGE_CHAPTERS=packages/chapter-01.json
```

Replace the example specification with the authored chapter's JSON. This target
reads only declared `.89y` data members from `data/` and `../opening/data/`;
it invokes no compiler. Custom asset folders can use the Python entry directly:

```sh
rtk proxy python3 games/fft/tools/chapter_packages.py \
  --reuse-core games/fft/campaign/x/packages/fft-core.89g \
  --data-dir /path/to/built/chapter/data \
  --chapter /path/to/chapter.json --out /path/to/output
```

The supplied core is checked against its own metadata and copied byte-for-byte.
An input cannot silently replace a core member. New shared resources or changed
engine capabilities require a revised core and regenerated matching manifests;
ordinary chapter maps/scripts do not require another engine executable.

Install the core once and exactly one chapter group in `main`, preserving their
archive attributes. Run `fftstory()`. To switch: leave the game for HOME, remove
only the names in the old chapter's generated variable list, install the next
chapter group, then Continue. Do not remove core members or `ffsave`, `ffsave1`,
`ffsave2`, `ffsave3`, `ffslot`, or user configuration. Installation and deletion
are explicit operations outside gameplay; the launcher never deletes a pack.

## Load and resume contract

`ffcore` (FKR1) describes the exact shared build. `ffpack` (FKP1) describes the
single active chapter, its prerequisite/completion story flags, previous/next
pack IDs, encounter routing, ordered scene/deployment/battle steps and data
dependencies. Both formats use byte-addressed big-endian words, checked lengths
and a Fletcher checksum. Each dependency declares its exact AMS payload length
and Fletcher checksum. A 128-bit key derived from SHA-256 binds the chapter to
the actual core variable contents and envelope.

The supervisor verifies the complete installed core/pack before invoking a
child or honoring the explicit new-game reset. Missing files, altered/mixed
banks, wrong core and unavailable prerequisite chapters stop with a short
message. Later packs open Continue directly, so New Game cannot overwrite a
slot while the first chapter is absent. Every archive/save pointer is acquired
again after a child returns; save/archive relocation cannot leave a stale
manifest pointer. Verification allocates no bulk buffer and runs only at
supervisor entry, never in a game frame. On the current fixture, that check
costs 26,307,772 datasheet cycles, about 2.19 seconds at 12 MHz; AMS ROM-call
costs remain estimates, not wall-clock hardware timing.

FSV4, all three slots, original source IDs, party/inventory/progression and
legacy migration stay unchanged. Step predicates read persisted story flags;
the existing Academy/deployment `flow` byte retains its pilot-specific meaning.
This does not introduce a new campaign scripting VM or generic mid-scene save.
Future scene/objective content must supply its own supported script data and
story flags; it is not certified merely by fitting in a package.

Encounter bands 1..254 retain the public door format `band*100 + door`.
The pack maps a band to its archived bank prefix, rather than the resident
assuming the fixed `"og"` table. Prefixes are lowercase letters or digits;
`c` is reserved because `ffca` is the actor-frame bank. Every registered
encounter needs `ff?v0..3`, `ff?a`, `ff?i`, `ff?d`; scripted starts additionally
need their `ff?st` data. Scene sets currently use `ffs0..ffs9`. Identical encounter
banks may be single-hop FRF1 references to another declared bank. All referenced
assets must be listed in the pack; generated scene pools are added automatically.
Unregistered encounters fail to load.
The old `og` fallback exists only for historical headless fixtures with neither
manifest; an installed core without `ffpack` never takes that fallback.

The fixed envelope remains 13 actors, 256 tactical nodes, 18 cells per axis,
two layers and 202 scenery rows. Larger content must be adapted or the core
revised with fresh RAM/cycle measurements. Package splitting does not expand
those runtime capacities.

## Native integration and verification

Current ABI80 adds a dedicated RESOURCES26 cold image; load10/route11 replace
NATURE10/11, still returning before PARTY12. ModCtx and execution zone remain.
See CORE_RESOURCES for current figures and verified sharing cases. The following
paragraphs record the original ABI79 packaging checkpoint.

ABI79/ModCtx348/signature0991 adds NATURE10 (registered encounter load) and
NATURE11 (resolved prefix for scripted starts). NATURE returns before PARTY12
executes; the input union holding `chapter_ugfx` is not reused as a second
simultaneous output. Unit112, State2224, Work37460, Save3202/FSV4, the five
battle allocations and 48 cached actor frames stay unchanged. The execution
zone stays 8,690 bytes; shards are 55,616/61,352/56,298. Production TTPack input
is 64,893/64,936: 43 bytes remain. Existing byte/frame budgets still apply.

Focused checks are:

```sh
rtk proxy make -C games/fft/campaign core-packages-check
rtk proxy python3 games/fft/campaign/check_chapter_packages.py \
  --input games/fft/campaign/x/packages/source.89g
rtk proxy make -C games/fft/campaign packages-routing-check
```

The package carrier covers malformed/truncated metadata, missing or mixed
files, core-key rejection, every persisted pilot resume boundary, cancellation,
world-map/menu returns and save-preserving chapter replacement. The real-engine
carrier loads Orbonne/Gariland, remaps Gariland from band2 to band3, removes all
Orbonne data in a continuation-only fixture, and rejects missing routes/packs.
PC, ASan/UBSan and actual 68000 results agree. The existing native bank/context
checks reject previous ABI78 exports; pilot flow checks retain their original
pixel/rule hashes and exact FSV4 VAT saves, repeated receipts and trophy overflow.
Twenty-five executable images remain byte-identical after normalizing only
their ABI signature; only NATURE's cold loader changes. This is targeted
integration validation, not a rerun of every unchanged combat family.

Private evidence and the reviewable delivery are under
`x/chapter-packages/`, with groups in `release/`. Historical groups, proof/save
bytes and both timestamps, unrelated work and the live TiEmu remain protected.
No new emulator/hardware execution is claimed for this packaging-only change.
