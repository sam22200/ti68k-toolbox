# Playable Bar and Chapter I rumor rules

2026-11-22. The accepted Bar A is now a real `worldmap.c` service, with
source room0, owner117/D, the left shared menu, grey reader and campaign3x2
continuation cue. Enter the town, choose **Bar**, then **Rumor**. Arrows choose
a topic; ENTER/2nd opens it. LEFT/RIGHT changes pages; ENTER/2nd advances and
returns to the list after the last page. SHIFT/ESC unwinds one level, retaining
the topic cursor when closing a reader. There is no numeric page counter.
No Propositions entry is added in Chapter I.

## Chapter I transitions (follow-up2026-11-22)

The reusable Chapter I rumor conditions are now source-checked and exercised
through the real map service. The shipping pilot retains its three original
records and all their bytes; later scenes are still campaign content to author.

| Original event | Source progress | Rumor changes |
|---|---|---|
| 00C | 1 | Enable0,1,2 |
| 02D | 8 | Retire1; enable3, The condition of Omdoria |
| 03A | 10 | Retire0; enable4/5, Destruction of the Death Corps!? / Attempted murder of Lord Dycedarg |
| 075 | 15 | Retire0..5; enable Chapter II's6..8, outside this bank |

`tools/rumor_rules.py` decodes the four original event instruction streams using
the retail operand sizes. It requires the paired ZeroVariable/AddImmediate
assignments in their final straight-line progress sequence and derives all six
lifetimes independently of the authoring constants. Source hashes, instruction
offsets, values and event-block hashes are in `x/bar-rumor-rules/source.json`.
This is original instruction/data inspection, not a new full PS1 scene replay.

All six source articles decode without guessed quest effects: the King alone
has the previously implemented profile8/9 prefix; the other five are passive.
The full fixture is4669 archived bytes versus the pilot's3264. The accepted
6360-byte artwork is shared, with no additional Save or heap storage.

Chapter specs declare `rumor_events`, an explicit mapping from original event
ID to the native **one-based completion flag**. The production pilot declares
only `{"event":12,"flag":2}`. Missing start events add no future rumor. Packaging
rejects a bank whose gates disagree with these declarations, duplicate flags,
unknown milestones and the unrepresentable flag256. Do not reserve the fixture's
33/34/35 flags in the real campaign: they are isolated test bindings. When
authoring later scenes, bind the flags actually written on completion of their
source event, including a permitted skip. Do not advance them on a Bar visit,
elapsed travel days, or an unrelated battle.

The same generator accepts `--chapter-spec PATH`; the full fixture binds all
four milestones without inventing/ripping the associated campaign scenes.
Each indexed page matches the actual C wrapper, retaining all prose across
19 pages. The33-character Dycedarg title fits the existing132px name badge;
the bank validators now allow40 characters while generation still enforces
the124px title-width budget. Lists retain the accepted three-row height and
shared scroll triangles, leaving the War Funds plate unobscured.

`check_rumor_rules.py --native` passes seven PC/ASan/UBSan images, the actual
four-stage replacement/input/back/save/reentry flow at all43 locations, and
one selected68000 full-image/state case for the longest title/deeper directory.
All nine pilot pictures remain byte-identical to the preceding proof.405 older
bank/save/review files preserve bytes and mtimes. The new native cached reader
remains70438 cycles; no emulator, new ABI/Save growth or package replacement.
The source/bank/runtime hashes allow later PC-only runs to reuse this evidence.

```sh
rtk proxy make -C games/fft/campaign rumor-rules-test
rtk proxy tools/pyenv/bin/python games/fft/campaign/check_rumor_rules.py --native
```

The Chapter I rumor mechanism is ready for content authoring. Next service work
is Propositions, then separate Fur stock/purchases; their reusable machinery is
still required before campaign extraction. Later chapter rumor/quest rules and
the full Brave Story browser remain separate.

## Original evidence and bounded content

**OBSERVED IN SOURCE:** `wldcore_list_open_rumors` combines a location mask
with variable0x2D1+id. The directory at WLDCORE0x80096A4C points to the actual
64-word table at0x800970AC. The mask is0x4000 >> location kind, using the same
world index as the native0..42 locations; the first three masks are0x1120:
Igros2, Gariland6 and Dorter9. Fields and every other town do not get these
three rumors.

Titles are source section25 of `EVENT/WORLD.LZW`, expanded by the existing
retail dictionary reader, paired by ID with block0 of `WORLD/WLDMES.BIN`:

| Source ID | Original title | Current pilot gate | Effect when opened |
|---|---|---|---|
| 0 | The Death Corps | Gariland victory flag1; source town mask | No durable write |
| 1 | The King Omdoria | Same | Profile8=1 and profile9=1 |
| 2 | The Fifty Year War | Same | No durable write |

**OBSERVED IN AN EXISTING ORIGINAL RAM SNAPSHOT:**
`sources/fft_ps1/world/ram0.bin` has story progress1 and exactly rumor variables
0..2 enabled. The live text section25 reproduces all three extracted titles.
Its hash, disc hashes and decoded records are in `x/bar-service/source.json`.
This reuses previous evidence; it is not a new full PS1 menu replay.

**OBSERVED IN SOURCE:** the King's text begins with F5/F6 stores. Setting
TEXT_STORE_INDEX0x66 to96 and storing to8/9 writes profile variables0x3C8/3C9,
then clears the index. `world_menu_display_text` executes those writes while
rendering the scrollable text. The native reader applies these prefix effects
on opening, before the first page. Repeated identical writes do not dirty the
session, and profiles already at12 or above retain the source's write guard.
This unlocks saved profile state; the full Brave Story browser remains separate.

**Initial pilot implementation:** the active pilot's existing victory flag1 supplies
the known progress1 availability. The compact chapter records support start/
stop flags and distinct source masks, but later story profiles and quest rumors
are not certified or populated. Do not infer that all64 rumors or all Chapter I
conditions are finished. The builder rejects unknown controls, non-profile
effects and mid-prose effects instead of dropping them silently.

## Storage and performance

- Core `ffbr/FBR1`:6360 raw bytes, one aligned348-byte portrait and the approved
 6000 raw plane bytes at30-byte stride. Those records match the accepted
 visual assets exactly. No duplicate Bar portrait or background per rumor.
- Chapter `ffru/FRU1`: pooled source titles/prose/effects and an offline page
 index, built with the current ffmg font. No whole-article RAM buffer.
 `bar_assets.py` owns extraction/validation; packaging validates each family
 and its ownership. The mandatory duplicate audit includes both.
- The existing town page allocation is reused. It is composed on entry/page
 changes, then blitted; only the glove or continuation cue is drawn live.
 `../bar_reader.inc` is shared with the historical A preview, which still
 reproduces the preceding accepted short frame exactly.
- Offline page offsets replace repeated full-article scans. For the original
 2549-byte/68-line/10-page War article, each indexed page is checked against
 the actual C wrapper, preserving every non-space character in order.
- Selected native cached frames: Bar home76778 cycles, readers70438. Last-page
 cold composition788818 cycles, versus2979638 before indexing. Cold entry is
 separate from the360k steady-frame budget; no hardware FPS claim.

## FSV8 profile persistence

Save appends32 bytes, one nibble for each of64 original Brave Story profiles:
**3992 ->4024 bytes**. All older offsets, party/guest/discovery/calendar/RNG
bytes remain in place. FSV2..7 migrate on read. FSV7 conversion preserves the
entire old prefix/calendar/RNG and initializes only the appended profiles.
Reading passive lore changes no Save byte. The King's two profile writes
survive session exit/reentry and title Save/Continue; battle-result writes
also preserve the profile tail. New Game clears it through the shared initializer.

The companion image's save reader grows5046 ->5140 bytes; its measured reserve
grows5048 ->5140 (+92). Keep42 actor frames and every other reviewed cache cap.
The battle's Save storage adds32 bytes; this does not certify installed16KiB
free/peak RAM. ABI95 is unchanged, while `MOD_SIG` includes sizeof(Save) and
therefore changes. Core/chapter keys and the FSV8/FBR1/FRU1 spec require a full
matching rebuild at release: never mix an old companion/battle core with FSV8
frontends. Previous installation groups and user saves are preserved.

## Reproduction and remaining work

```sh
rtk proxy tools/pyenv/bin/python games/fft/campaign/check_bar_service.py
# Only the changed archive/render/save paths on68000, no emulator or battle sweep:
rtk proxy tools/pyenv/bin/python games/fft/campaign/check_bar_service.py --native
rtk proxy tools/pyenv/bin/python games/fft/campaign/check_bar_service.py --save-regressions
```

Evidence: `x/bar-service/`, including `pc/bar-service.png`, the source hashes,
PC/sanitizer logs and four native doors.334 save/migration and59 supervisor
checks agree PC/sanitizers/68000. Nine Bar screens agree PC/sanitizers; four
selected native full images/state hashes agree.267 prior files retain their
SHA256 and mtime. Separate focused PC/sanitizer regressions cover Shop/Office/
travel, title sessions/slots, management, deployment and battle save paths.

Remaining before campaign extraction: reusable Propositions (Chapter II gate),
and poaching -> separate saturating255 Fur stock -> purchase/migration (III gate).
This lot does not ship later rumor text, new campaign maps/scenes, Propositions,
Fur transactions or a new emulator/installed-RAM certification. Development
`data/ffbr.bin/.89y` and `ffru.bin/.89y` are available, with archive attribute3;
`make fftmap_pc` builds the real PC entry with these banks. The private28-image
export fits the unchanged8932-byte zone and the measured5140 companion reserve.
Private packed map/battle builds pass; release groups have not been replaced.
