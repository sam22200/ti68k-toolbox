# Title and three save files

Latest A/B sizes, shared choices and native startup/save findings: [QA_TITLE_FOLLOWUP.md](QA_TITLE_FOLLOWUP.md).

2026-10-23. User order: complete package (workstream2), remaining battle QA
(workstream1), then core mechanisms (workstream4). The requested front end adds
Final Fantasy Tactics -68k, New Game / Continue, the shared animated white hand
and three independent data files. Mandalia is not the next workstream.

## Original and adaptation

**OBSERVED IN FFT:** `tools/title_assets.py --observe DISC.cue` cold-boots the US
disc with controller inputs. Start at360..363 and1501..1504 reaches the title at
frame1801. It has the original logo, parchment/map background, highlighted New
Game, Continue, Tutorial and Sound. Continue opens the zodiac-wheel memory-card
screen; an empty card reports No saved data. Original images/states and the input
recipe remain local under `sources/fft_ps1/title/`.

The local source routines `open_title_push_menu_controller`,
`open_title_update_menu` and `world_card_update_and_draw_slot_list` corroborate
the title/menu behavior. The original card list supports15 records with four
visible rows. Three data files are the user's TI adaptation, not a claim that
the PS1 had only three. Its title highlights text; the animated white hand here
is the user's requested shared selector.

**TARGET IMPLEMENTATION:** retain the source logo and map, add the68k subtitle,
keep only the two requested entries and display three fitted file cards with
the existing neutral Ramza, level and story location. No invented playtime is
shown. Original foreground menu records are hidden only in a separate art
capture, after recording the unmodified reference. Two backgrounds are
quantized offline, then packed with the existing verified ZX0 decoder.
`fftitle.bin` is2863bytes; one4000-byte decoded background is reused. No new
sprite bank, runtime resizing or handwritten assembly.

## Saving and resuming

`save_slots.h` names `ffsave1`, `ffsave2`, `ffsave3`. A six-byte `ffslot` selector
contains `FSL1`, slot1..3 and a reserved zero. Without a valid selector, existing
callers retain `ffsave`. `save.h` routes battle, scenes, management and world map
through this same selector; payloads remain FSV3,3138bytes. The supervisor reads
the matching metadata before choosing its resume point.

The title treats a readable legacy `ffsave` as Data1 only when no supported slot1 record exists.
Explicit Continue copies it to slot1, migrating FSV2 while preserving the
original file. New Game initializes only the chosen slot, with2000gil, and
requires an explicit overwrite confirmation for occupied/damaged supported
records. Return is selected by default. Empty/damaged files cannot be continued.
Selecting a file never erases another slot or the legacy source.

TI `rt_save` holds one pending write. The runtime first tears down and flushes
the chosen slot. `title_finish` then verifies it (exact bytes for new/imported
data) before queuing the selector as a second small post-teardown write.
`native_title.c` flushes that metadata and verifies it before returning success.
This avoids replacing the slot write with the selector write. The temporary
verification buffer is allocated after the runtime's screen buffers are freed.

Normal `fftstory()` enters `ffttitle()`; existing debug arguments bypass it.
PC `play.py` uses the same menu, a fresh process per program and private
`x/play-data` saves. `--new` now opens the New Game file chooser; it does not
silently delete the private save. These are checkpoint resumes, not in-battle
save states or PS1 memory-card import support.

## Validation and review

- `make title-test`: independent slot writes, navigation, protected overwrites,
  cancellation, legacy migration, invalid metadata and damaged supported files.
- `python3 check_title.py`:18 actual68000/PC pixel/state/write cases, including
  both native deferred writes and all eight glove animation phases.
- `python3 check_flow.py --native --slot 2`:74 Gariland +14 Orbonne
  assertions, complete saved bytes, repeat-reward guard and trophy overflow on
  a selected independent slot. No fallback legacy file is supplied.
- 29 campaign +18 opening/scene native replays pass. All139 preserved reference
  PNGs remain byte-identical; no old reference is refreshed. Campaign/opening
  unit, regression and648-pose presentation suites pass.
- Title tests pass ASan/UBSan with the established shift/leak exclusions. All PC
  entries and isolated native title/supervisor wrappers compile.

`review_title.py` renders `x/qa/title/review-1/title-review.png` and
`title-glove.gif` with the actual C renderer. The two occupied slots are clearly
labelled private illustrative fixtures, not the user's saves. Both are gallery
decisions. Visual review precedes final title packaging/hardware presentation.

The pre-title full package was independently rebuilt at466542bytes and copied
to `x/package-before-title/`. This is a baseline checkpoint, not a title-enabled
final bundle or a new whole-package heap/FPS result. The user's live emulator
and real save files are not part of these fixtures.

The package-memory follow-up packs map/management into small entry stubs and
checks repeated visits on an isolated Titanium; see [PACKAGE_CHECK.md](PACKAGE_CHECK.md).
The title recipe likewise uses a packed entry so its full code copy will not
remain resident through the story. Its final bundle/hardware check still awaits
the title visual verdict.
