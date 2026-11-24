# Shared campaign resources and shipping deduplication

Measured follow-up (2026-11-24): [the full pack report](CAMPAIGN_PACK_REPORT.md)
supersedes the unmeasured part proposal below. Complete sprites remain the
combat default: fixed-row two/three parts enlarge the native30 corpus and
increase native preparation13..15%. No generic-body/head replacement has a
visual verdict or counted saving. Actual scene-pose pruning remains authoring.
The user allows optional wardrobes only below50KB total. Ramza+Delita measure
45808 body bytes /47232 with portrait/reference allowances in the extended
control; budget50000 and recheck final required poses. Other costumes remain
canonical; all six outfit candidates exceed the cap. No installed bank changes.

Accepted pack/scene policy (user2026-11-24): retain eight main packs (I whole,
II split after Goug, III after Orbonne, IV after Bethla and after Igros).
Use one orientation per scripted scene by default; extra directions are rare
source-driven exceptions. Do not equate one direction with one crop per room:
retain needed camera travel and reuse physical backdrops across scenes. The
previous8..20KB/location scene allowance remains conservative pending the
actual per-scene camera inventory; do not automatically halve it.

Actor-parts candidate: the earlier estimate assumed complete human sprites,
not separate head/torso/feet. The user now proposes sharing secondary bodies
while retaining unique recruits and major story actors. Evaluate head + body
including feet first; split legs separately only if actual pooled bytes and
native preparation cost improve. Keep a complete unique appearance for every
recruit and selected important actors (explicit examples include Ramza, Delita,
Alma, Agrias, Gafgarion, Orlandu and Meliadoul). Candidate additional major
actors include Ovelia, Wiegraf, Dycedarg, Zalbag and Elmdor. This is not a final
exhaustive importance list or blanket approval to replace other named bodies.
Named scene-only actors can keep their own silhouettes with only the poses
actually referenced by scenes; they do not need a full combat animation bank.
Retain source gestures, walking, kneeling, injuries and deaths when scripted.

If adopted, physical parts must remain pooled in the shipped archive: baking
every composed character back into a full shipping bank loses the sharing.
Compose only at a bounded pose-cache miss/preparation, then draw one complete
cached208-byte human pose through the existing fast renderer. Keep validated
appearance+pose keys, eviction/reentry behavior,42 entries and preparation
budget. Account for extra decode/composition work in complete native frames;
existing CPU evidence does not certify a new preparation path. Preserve one
coherent silhouette/white outline, internal hit mask and cover at part seams;
separately outlined pieces cannot simply overpaint one another. No new cache,
save identity or per-frame three-part draw is implied by this proposal.
Do not claim a byte or CPU saving before a bounded part-bank prototype proves
it. Current approved sprites/runtime and conservative full-body estimates stay.

Named-human simplification (user2026-11-24): one selected human body per named
person across costumes and source guest/enemy/chapter identities. This overrides
the older instruction below to retain costume variants. Identity, job, stats,
portrait/name and recruitment are preserved; major nonhuman forms are budgeted
separately. Chapter-confined NPC bodies do not accumulate permanently. Keep
recruited bodies even when they have no later scripted dialogue. No installed
art/bank has been rebuilt by this sizing decision.

Service ownership update (2026-11-23): the reusable96-mission catalogue,
universal reward tables and all mission descriptions/reward names belong once
to core: ffpr/FPR14079, ffpe/FPE1437, ffpt/FPT116148 bytes. Prose/page offsets
are pooled offline and borrowed from archive; reuse the real management sheet
and town page, not a second renderer or page buffer. Chapter ffsv/FSV1 is8
bytes of explicit source-event/native-flag bindings, missing=255/locked.
The shared Fur room/portrait/shop/item UI is reused; its256 stock counters
belong in FSV9 Save, not an extra artwork bank. Matching private ABI96 groups
are in `x/town-complete/packages/`; old installed groups are unchanged.

Bar update (2026-11-22): core ffbr/FBR1 owns the approved Bar room/portrait once;
chapter ffru/FRU1 owns pooled rumor titles/prose/effects/page offsets. Both join
packaging validation and the existing duplicate audit, with a shared-alias reuse
fixture in `check_bar_service.py`. Direct archive reads reuse the town page,
without an article buffer. FSV8/core keys require a matching release rebuild;
previous installed groups remain intact. See [BAR_SERVICE.md](BAR_SERVICE.md).

FMA1 update (2026-11-14): [actual monster Formation/Status](MONSTER_FORMATION.md)
uses universal core `ffma`,13082 raw bytes:16 chosen body/portrait pairs and255
pooled source names. Forty-eight job IDs alias the16 physical records. The bank
is validated by packaging and cold C binding, with native DAT trailers handled.
Management/deployment borrow it in place, with no allocation/cache growth.
Retained picture records are exact copies of the approved selected appearances;
future consumers must reference these compatible records instead of duplicating
them per encounter. Battle sprites and real monster placement remain next.

Human appearance decision (2026-11-14): the user extends the single-palette
rule to humans to limit campaign size. Select one high-contrast four-grey
palette per distinct human character/job sprite model; do not rip or ship
all of its palette-only color swaps. Reuse the selected sprite/portrait via
IDs across encounters and teams. Preserve genuinely different male/female
models, named characters, costumes and story forms, plus every unit's actual
identity, job, team, stats and dialogue. Known speakers still need their own
appropriate portraits. This is an extraction/ownership rule, not a claim that
existing installed banks were rebuilt or that storage savings were measured.

Monster appearance decision (2026-11-14): the user requests one high-contrast
visual per ordinary monster family, with job names distinguishing the three
types. `tools/monster_appearance.py` selects16 source appearances for48 actual
jobs. Share body/portrait references; do not duplicate three palette versions
or change gameplay job IDs, stats, abilities, breeding or saved identities.
This policy now drives shipping Formation/Status; battle art remains next.
Source shapes and source palettes remain available as offline controls.

Calendar/egg update (2026-11-14): universal core ffwb/FWB1 is285 rule bytes;
ffeg/FEG2 pools eight reviewed color/crack references into four physical16x26
sprites (640 bytes instead of1256). Management/deployment borrow the archive
pointer, never a per-chapter copy or new frame cache. Existing glyphs, glove,
human actors and portraits remain shared. New core spec keys bind these families
and FSV7 to the matching ABI92 release. All chapter content remains exact.

FCX1 scene update (2026-11-13): choice records and their preceding question
beats join the existing chapter-wide FSP1 pool; equal records across two different
scene sets share one physical span. Choice/flag-branch scalar destinations stay
in8-byte FSC rows; they are not resource offsets. The shipping parser validates
records, final question bounds and forward targets, and requires the FCX1-capable
core/key. No new font, portrait, image bank or whole-chapter RAM pool is introduced.
The production pilot adds no fictional recruitment data; see STORY_CHOICES.md.


FGP1 update (2026-11-13): [persistent guests](GUESTS.md) use one chapter-wide
`ffgu` directory of stable identity/encounter-slot references. The shipping pass
generates it from `persistent_guests`, checks an explicitly supplied bank against
the declaration, and includes it in the existing ownership/duplicate audit.
The pilot adds12 raw bytes for Delita, sharing all existing portraits/sprites.
No per-encounter actor copy or frame-time lookup; ABI91 matching keys are required.


FGO1 update (2026-11-13): [named objectives](OBJECTIVES.md) use optional active-
chapter `ffgo`, one route directory with exact shared title/name spans across
all encounters. Two equal routes add only16 directory bytes. Core result/default
strips remain once in ffcu; new whole named words are encounter content, never
copies of the UI bank. Direct archive reads add one cold Work pointer, no new
allocation/cache or frame-time lookup. Packaging validates declared routes and
requires the matching FGO1 core specification/key (ABI90); FCD5 remains.

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

Planning clarification (user2026-11-24): split Chapter IV and
retain only a few extra random terrains per pack, reusing active story fields
where possible. Old field destinations need explicit encounter-terrain routing;
this does not remove their travel nodes or silently discard their formations.
Budget scene-only locations separately and share existing battle imagery.
One/two selected scene compositions and a limit of ten common optional FX are
the current sizing proposal, not completed extraction or camera/CPU/RAM gates.
Audit temporary NPC/costume lifetimes before retaining them in later packs.
Generic-body substitutions for rare named characters are under consideration,
not blanket approved; retain required named portraits and gameplay identities.
Read the current HANDOVER and `x/pack-refinement/plan.json` for current scope;
the older `x/chapter-map-counts/revised.json` is superseded sizing evidence.
The current scene player already decompresses all six picture planes with
ZX0 at scene entry. The bounded six-room compression probe does not justify
adding a new codec for its roughly4% row-XOR gain. Event text4KiB-chunk ZX0
is a proposed cold-reader change, not an implemented format. The eight main
boundaries are now accepted planning; no new packs/reader have been released.

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
   mandatory. Apply the explicit single-palette decisions above; otherwise do
   not merge merely similar artwork. Preserve depth/masks in every case.
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
