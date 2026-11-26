# Shared core and replaceable chapter packages

## Current ABI99 contract (2026-11-25)

The user requested the freeze after the demo was repaired on a rebuilt core.
Current reviewed groups: `x/core-abi99-1125/packages/fft-core.89g`706459 and
`chapter-01.89g`295209 transport bytes; key
`785d5a96d378b836b1bb79c847ebf040`. `packages/core-contract.json` freezes ABI99,
29 matching images, State2278/Unit116/Work37790/ModCtx400/FSV9 Save4684,
zone9142 and companion4132 (cold reader3546). Actor42 remains; depth is18520.

What moved from the2026-11-24 checkpoint
(`x/core-abi99-release/packages/`, key `10965478d3a2c348c22d2dea8e6ac51d`,
core701169 + pilot295145, zone9118, depth18488), and why:

- the shared archive tail took the status pictograms and the creature art,
  `CONTOUR_BYTES`6892 ->11668. **A chapter group carries its own `ffcu`, so a
  group built before this one cannot be read by this core**: `archive_art_tail`
  refuses the shorter tail, `contour_gfx` stays null and the scene player loads
  nothing. Rebuild chapters with `make packages`, never mix the two.
- `FFT_CORE_DEPTH_MAX`18488 ->18520, the32 bytes North Wall of Bethla needs.
- a crystal now teaches the dead unit's abilities (`loot.inc`).

ABI99, the image count, ModCtx and the save format are untouched, so this is a
content move, not an interface move. The earlier group is kept whole because
the installed Titanium RAM figures quoted across these documents were measured
on it, and `MOD_ZONE` has since grown24 bytes.
Actual TTPack input64725 leaves211 bytes. Chapters stay data-only and match
this exact key; saves and configuration remain outside both groups.

Run `make core-contract-check` with its new default groups. Pilot checks also
use these groups; new builds go to `x/build-packages` and chapter-only builds
reuse the promoted core by default. Override `PACKAGE_CORE` for another
explicitly reviewed release. Preserve old `x/packages`/`x/chapter1-ready`
proofs. See [INSTALLED_RAM_ABI99.md](INSTALLED_RAM_ABI99.md) for the cold
fragmentation fix, source/package evidence and promotion scope.

Actual private Titanium free/max: Orbonne2420/2414, Gariland1420/1414 and
Zeklaus13 diagnostic436/430 bytes, including logging storage. These are
settled snapshots;16KiB and all-action/future-map minima are not certified.
The user deferred difficult RAM recovery. Keep cache capacities and full C2
geometry. The pilot hardware lifecycle/Save/Continue, wide state/rotations/exit,
PC/sanitizer ordinary fights, selected native CPU gates and package checks pass.
The user's live emulator and all historical groups remain untouched.

The following sections retain historical packages and measurements.


## Historical ABI97 contract (2026-11-23)

`packages/core-contract.json` records the exact core group SHA/key, every
shared variable, native layouts and public interface hashes. Run
`make core-contract-check PILOT_PACKAGE_DIR=x/chapter1-ready/packages`. A
source/layout drift fails; explicitly rebuild, review and freeze a new core
before distributing changed interfaces. Chapters remain data-only, matched
by core key, resource-pooled and independent of saves/configuration. The
packager rejects executable members in chapter specifications.

Current groups: `x/chapter1-ready/packages/fft-core.89g`700091 and
`chapter-01.89g`295145 transport bytes; core key
`ac1de6c7a4d71264d1bee76df06f489d`. ABI97 has29 images; graphics27 (4128) and
cold Save reader28 (3542) share one preallocated4128-byte block, reclaiming
3136 RAM bytes. Replacing either owner invalidates the cached body entry.
Zone9114, actor cache42 and depth18488 remain. Actual TTPack resident input
64833 leaves103 bytes. FSV9/4684 is unchanged. PC route and selected native
owner/save checks are in `x/chapter1-ready/`; installed evidence is in HANDOVER.
This interface freeze does not certify unripped content or16KiB free RAM.

Earlier sections retain historical packages and layout measurements.


FSV9/ABI96 update (2026-11-23): playable Propositions and separate poach/Fur
stock require all current frontends plus28 matching images. Save4684 appends
missions402/stock256/identity2 after the old4024-byte prefix; FSV2..8 migrate.
Core owns ffpr4079/ffpe437/ffpt16148; chapter owns ffsv8 service-event bindings.
Packaging validates their formats, ownership and bindings before producing
matching keys. The pilot leaves later chapter services locked.

Prepared private groups: `x/town-complete/packages/fft-core.89g`699535 transport
bytes and `chapter-01.89g`295145; key522698b3edc1e50102e6ef96b08662bf.
The intermediate `package-source.89g` must not be installed. All28 image
signatures2288 and shard CRC agree. Zone9114, pinned companion7264 (now also
composes fitted cached choice windows without overlay churn), cache42/depth18488.
Fixed requests +2966 against FSV8; save scratch adds256 transient stack bytes.
Battle TTPack input64827 leaves109 bytes (packed header and decompressed size).
No cache/capacity reduction. Current installed free/peak/contiguous RAM is
unmeasured;16KiB remains unpassed. Old successful boots below are historical.
No groups/saves were installed or replaced in the live emulator.

Bar/FSV8 update (2026-11-22): [BAR_SERVICE.md](BAR_SERVICE.md) adds shared core
ffbr/FBR1 and chapter ffru/FRU1; packaging validates ownership and formats.
Save3992 ->4024 appends64 Brave Story profile nibbles; FSV2..7 migrate without
moving old fields. ABI95 stays, MOD_SIG changes automatically with Save size.
All frontends, battle and28 images must be rebuilt together for the next release,
with matching core/chapter keys. Existing installation groups are preserved.
The companion reader is5140 bytes, so its reserve5048 ->5140 (+92), with no
cache reduction. The16KiB free/peak target remains unpassed; the earlier successful
battle launch below is historical evidence, not FSV8 installed-RAM certification.

ABI94 update (2026-11-22): the battle's Status screen became the shared unit sheet
([UNIT_SHEET.md](../UNIT_SHEET.md)) -- the campaign's plate, the sheet's tab bar and the
source's slot pictograms -- so `MOD_ABI` is 94 and core and chapter must match. The battle
program grows **63,269 -> 64,503 bytes** (measured with the same `ti-cc` line; the dead
`UC_STATUS` card layout came out in exchange). Core 643,751 + chapter 291,789 transport
bytes, core key 5e10e934de49acac0b01059ef8fffee5.

**Measured on the Titanium, 2026-11-22**: the groups install and `fftstory()` runs -- title,
New Game, the Orbonne opening with its portraits and its skip prompt. **The battle then
refuses to start: "need 99 KB, free 94 KB."** That is the installed-RAM peak this file has
listed as open since 2026-11-14, not a new fault: at HEAD the program is 63,269 bytes, which
leaves about 95 KB free against the same 99 KB `BATTLE_RAM_BYTES`, so it was already about
5 KB short. The screen itself is checked on the PC (`status-frames-test`, 360 frames) and by
the campaign's 62 PC/68000 replays; **a battle has not been seen running on hardware with
this core.**

**The battle runs on the Titanium, 2026-11-22.** `fftstory()`, New Game, the Orbonne opening
and the Academy bridge skipped, and Gariland's battle comes up: the map, Ramza's card
(HP 154/154, MP 25/25) and the turn menu. The Status screen's three tabs work on the machine,
item icons and all, and [2nd] shows FFT's own description. **This is the first battle seen
running on hardware with this core**; the installed-RAM peak this file listed as open since
2026-11-14 is closed.

What it took. `ti-mem heap` at the refusal named every block, which made the budget legible:

| bytes | what it is |
| --- | --- |
| 64,948 | the battle program itself, unpacked and locked in RAM |
| 53,952 | `sbuf`: the scene planes plus `Work` |
| 20,122 | AMS's own |
| 18,490 | `vbuf`, the depth envelope |
| 16,384 | AMS's own |
| 10,178 | the actor cache |
| 8,934 | the module execution zone |

The message was printing truncated kilobytes, which hid how close it was; printed in sixteens
it read **need 6080, free 6012 -- 1,088 bytes short**. Four reservations were the worst case
rather than what the battle takes, and all four are now sized to what is loaded:

- **the map envelope** was `malloc(MAP_NMAX * 20)` = 5,120 at cold boot. `nav_reserve(MAP_N,
  2)` already takes the loaded map's nodes and grows if a later map needs more, so the
  reservation is gone: **2,120 bytes** for Gariland's 150 nodes of 256.
- **the depth envelope** `FFT_CORE_DEPTH_MAX` 20,480 -> 18,488, the biggest view that exists:
  **1,992 bytes**. Confirmed in the heap: `vbuf` 20,482 -> 18,490.
- **the companion block** 5,376 -> 5,048, its image being 5,046: **328 bytes**.
- **the actor cache** 48 -> 42 frames: **1,272 bytes**. This one is a trade, not a cleanup:
  `README.md` has Gariland's AI battle at ~9,073 frames with 48 and ~10,445 with 32, so six
  frames cost roughly 5 % of a battle's length. It was the last block of that size and the
  1,088 needed a margin; `test_actor_cache.c` now reads `AC_N` instead of 48, and at 48 it
  still produces its old digest `00000002` over 63 checks, so the trial itself did not move.

Together **5,712 bytes**, against 1,088 needed; the rest is the margin the earlier attempts
lacked. Wiring the Secondary command in the same session put about 950 bytes back into the
program, which is resident, and the battle still starts.

Two things learned the hard way, for the next person: `fftbat(200)` is Gariland's **door 0**,
a story start, not its battle -- `fftbat(261)` is the battle, and from a cold HOME it wants a
save the story makes, so `fftstory()` is the way in. And `HeapAvail()` read at the refusal is
measured after the cleanup has freed everything, so it flatters the figure; the useful
numbers are the heap listing and the needed total.

FMA1 update (2026-11-14): shared core ffma13082 supplies approved monster
Formation/Status. `formation_art_format: FMA1` is required and validated by
packaging. Only ffmgcore/ffpccore and manifests change; all program stubs,
battle/shards and chapter content except ffpack remain exact. ABI92/FSV7 and
battle allocations are unchanged. Matching groups441437+291789=733226, key
37f0ec2538ccb78ed0b0f20ef00cdedd: `x/monster-formation/release/`. Adult battle
deployment/art remains next; this is not a16KiB installed-RAM pass.

FSV7 update (2026-11-14): [calendar/eggs](BREEDING.md) require matching ABI92/
ModCtx356/FA04 programs and images. Core spec declares FSV7/FWB1/FEG2 and owns
universal ffwb285/ffeg640 once. All six packed children and supervisor are
rebuilt for Save3992; all28 images share the new signature. Zone8702, companion
1612 inside its unchanged3072 reserve, battle input61491/headroom3445. Current
transport427121+291789=718910, key 656593affdef2a427bec0030cb75fefd; use matching
`x/eggs/release/` groups. Chapter content is byte-identical except ffpack's key.
Save slots remain user-owned; FSV2..6 migrate on read.16KiB/installed peak stays open.

FCX1 scene update (2026-11-13): [story choices](STORY_CHOICES.md) require
`story_choice_format: FCX1` and matching core/chapter keys. Only `ffscore` and
manifests change; existing battle code/28 images and all chapter data remain
byte-identical. Current core420024 + chapter291789 =711813 transport bytes,
key668a8d4b8281e62a6087d23389480f73. Scene input26283; ABI91/FSV6 and battle
input61381/headroom3555/zone8842 remain. Choices update the session, with explicit
slots still owned by the title's Save/Continue. Old core reuse is rejected.


FSV6/FGP1 update (2026-11-13): [persistent guests](GUESTS.md) require matching
ABI91/F8B5 core/chapter groups. `persistent_guests` generates one chapter-owned
`ffgu` during the mandatory shipping pass; repeat builds/core reuse retain it.
The pilot binds Delita4 in Gariland, leaving prologue actors encounter-local.
FSV2..5 migrate to3918-byte saves with four empty guest slots. Save slots remain
outside installation groups and change only through explicit Save. Current
transport416987+291789=708776 bytes; input61381/headroom3555, zone8842,
Work29340. Older measurements/formats below are historical.


Checkpoint update (2026-11-13): [CHAINS.md](CHAINS.md). Core declarations require
`checkpoint_version:1` for safe intermediate Save and FSV2/3/4/5 progress reads.
Battle step skip_flag must match descriptor WON; Save prompts need that durable
flag. Matching core/chapter keys reject the preceding supervisor. FKP1/FSV5/
ABI90 remain; only fftstory grows74 bytes (pilot414169+291735 transport bytes).

FGO1 update (2026-11-13): [named objectives](OBJECTIVES.md) add optional shared
chapter `ffgo` and an `objective_format: FGO1` core declaration. ABI90/F4D0,
Work29314 and new matching keys are required; FCD5/FSV5 and the chapter-only
build flow remain. The ordinary pilot ships no unused objective data.

Latest (2026-11-13): [monster KO support](MONSTER_KO.md) declares optional
chapter-owned `ffko`/FKO1, validated by packaging and the cold loader. The core
spec's `ko_actor_format` binds support into the matching key; old core reuse
is rejected. ABI89/FSV5 and all28 overlays remain. Pilot transport412690+291735,
key0a6f05fdec1f5dd1e3eaf1c97f717351. The human pilot has no unused monster bank;
the private with-KO inspection chapter is294653 bytes.

Previous (2026-11-13): [traps and current-raw saves](TRAPS.md), matching ABI89
core/chapters, FCD5 source-map identity and FSV5/Save3204. Old core reuse is
rejected through the declared encounter format and package key. Pilot transport
bytes410040+291735; this is not full Chapter I or Flash/RAM peak certification.

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
total 605,207 bytes. Transport bytes include PC transfer headers.

The builder now gates on the **measured** archivable ceiling, 2,555,826 bytes on
an empty Titanium, with the same separate 131,072-byte reserve. TI's "2.7 MB" is
`allExceptBaseCode` (2,686,976) and is not archivable; `experiments/archfree/`
reads the real figures through `EM_survey`. Each archived variable also costs a
measured 23-byte archive header (`installed_archive_bytes`), and variables sent
unarchived are charged to RAM instead (`unarchived_ram_bytes`), not to the
archive. Fragmentation needs no term: AMS packs archived blocks contiguously.
This is still not a measurement of the user's own free Flash, since other
installed programs come off the same ceiling, nor proof that every future
chapter fits. Runtime RAM is a separate limit.

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

`ffcore` lists every core member except `fftstory` itself: the running supervisor's image
differs from its file (tigcclib's startup writes into it), so checking it always failed with
"Damaged/mixed file fftstory". The key still binds its exact bytes. After each child returns,
the supervisor deletes that child's RAM copy (`SymDelTwin`): AMS would otherwise keep ~1 KB
per program run until the supervisor exits, and the battle's ~183 KB of allocations then
failed ("Missing data or memory") after title → scene. A normal start (`fftstory()`) goes back
to the title when a step returns 0 (Game Over, quit battle), as in FFT; Continue resumes before
that step. Direct doors (`fftstory(2)`, `fftstory(99)`) still stop at HOME.

FSV6 (FSV2..5 migration), all three slots, original source IDs, party/inventory/progression and
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

## Native test loop (2026-11-08)

Install the core once into a named TiEmu state, then send only the chapter under test.
The core needs HW3Patch (`CHAPTER1_READINESS.md` § 4): build the core state on the `hw3`
state (once: `ti-emu restart tools/patches/hw3patch/hw3patch.89z`, `ti-key 'hw3patch()' ENTER`,
`ti-emu save hw3`):

```sh
TI_STATE=hw3 ti-emu restart x/packages/fft-core.89g   # returns once received (~26 s), HOME
ti-emu save fftcore                         # again whenever the core changes
ti-mem mkvar x/packages/tilog.89y           # optional: the supervisor's event log
TI_STATE=fftcore ti-emu restart x/packages/chapter-01.89g x/packages/tilog.89y
ti-key 'fftstory()' ENTER; ti-mem log       # story start/call/back lines with free RAM
```

`ti-mem heap` lists the AMS heap blocks (executable copies, battle buffers) when an
allocation fails. Measured on Titanium AMS 3.10 with the 1 KB log: 187,152 bytes free at
every child call through Orbonne, Academy, deployment and Gariland; the battle reserves
182,950 bytes (program 65,014, scene buffers 32,322 + 53,622, actor cache 10,178, State 5,428,
grayscale 15,378, its stub 1,008), leaving only a few KB. Since 2026-11-10 (ragged depth,
[CHAPTER1_READINESS.md](CHAPTER1_READINESS.md)): 187,554 free before `fftbat`, the battle
reserves 176,158 (program 64,934, depth 20,482, scene and Work 53,622, actor cache 10,178,
map envelope 5,122, State 5,428, grayscale 15,384, stub 1,008): ~11.4 KB stay free, on every
map. When the RAM is short the battle says "Not enough RAM / need N KB, free M KB" (free: AMS's
free RAM plus what the battle already holds). Verified natively: New Game →
Orbonne scene → battle won → Academy → deployment → Gariland; Gariland lost → title →
Continue → Gariland again. The Gariland victory path was not replayed natively here.

## Saves, debug aids and logs (2026-11-08)

**Saves as in FFT.** Every program reads and writes the session `ffsave` (party, flags, flow,
deployment, rewards). The player's slots `ffsave1..3` are written only by the title's Save:
the world map's Save, or a chapter step marked `"save_prompt": true` in its JSON (step byte 1
bit 0; the supervisor opens `ffttitle(3)` once the step completes, SHIFT/ESC declines). New
Game starts a fresh session without choosing a slot; Continue copies a slot into the session;
`ffslot` only remembers the last slot. A lost or quit battle returns to the title and the
slots are as last saved.

**PC / TI exchange (2026-11-23).** The same FSV payload can now be exported as
a TI variable or extracted back to a PC slot using `../tools/save_transfer.py`.
All slots/session and legacy FSV2..9 are preserved; existing destinations need
explicit replacement. Real PC→native atomic save→PC reload matches all4684
current bytes. This covers campaign checkpoints, not raw PC `.state` snapshots.
Commands and verification: [SAVE_TRANSFER.md](SAVE_TRANSFER.md).

**Skips and debug (fixed2026-11-13).** In standalone and battle-map scenes,
ESC asks "Skip scene?" with No selected. No, ESC or SHIFT resumes; Yes via
ENTER/2nd passes to the normal battle/aftermath handoff, without returning to
the title. Scene timers/fades pause during the shared confirmation.
See [scene/save delivery](SCENE_SAVE_FIXES.md). In battle, Settings (MODE/CATALOG) then [alpha] opens the hidden DEBUG page: God
mode (allied blows kill, damage on allies misses; single blows and area batches, not the
deferred weapon panel/line victims), Fast AI (AI turns as with [2nd] held, no 1.5 s hold) and
"Skip this fight?" (No/Yes: every enemy at 0 HP, the ordinary victory, rewards, aftermath and
session save follow). Flags live in `st.dbg` (State's former alignment byte) for the current
battle. The turn order ([APPS]) now takes UP/DOWN: the cursor and camera go to the selected
unit, other keys close it and restore the cursor. While an AI thinks, the units keep their
idle animation: one picture in four is drawn (every one during its final hold).

**Reading the game without screenshots.** `python3 games/fft/tools/fftmem.py battle | watch |
set god|fast on|off` reads/writes the live battle State in TiEmu's RAM (offsets from the TI
compiler itself, `tools/state_layout.c`): units, HP, turns, modes, KOs, outcome; no code in
fftbat. Scene, deployment, title, map, management and the runtime's save writes log to
`tilog` (`ti-mem log`), as does the supervisor (each call, result and free RAM).

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
