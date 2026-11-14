# Egg and calendar presentation review (2026-11-14)

**Pending user approval.** These are isolated PC prototypes, not integrated
worldmap/management/save code. Keep the shipping story-choice release and
FSV6/ABI91 unchanged until the pictures have a verdict and the complete
calendar/save/roster paths are ready.

Run `../../../tools/pyenv/bin/python review_eggs.py` from this directory.
`--sanitize` checks the preview pointers/planes on PC and requires the same
six picture checksums as the ordinary run. No TI compile, benchmark, package
generation or TiEmu. Generated assets/pictures stay in ignored `x/egg-review/`;
unchanged published PNGs retain their modification times.

## Pictures and source

`review.png` compares the production formation screen's currently blank monster
slots with the proposed egg presentation, one-entry Dismiss menu and No/Yes
confirmation. The seven human records and renderer come from the existing
flow fixture. Three authored QA eggs illustrate normal/cracked/new eggs; these
are not retail campaign content. The egg card replaces fighter stats with
"Monster Egg", remaining days and a progress bar. No/Yes starts on No. Compact
menu offsets match the approved scene-choice design: text x6/row10/pitch6,
body y3, hand x-16/y7, no extra empty top row.

`calendar.png` compares the actual world map and a small top-left date panel.
Mar21 is a picture example, **not** the new-game/migration date. Both images
are pinned as decisions in the existing ti-view gallery (LAN port8000).

`tools/egg_art.py` reads source body/crack descriptors at WORLD0x8018c704.
`world_menu_load_common_graphics` loads EVENT/UNIT.BIN at LBA0x166b: its second
32768-byte page is64x256 words at VRAM(320,0), confirmed by both source RECTs.
`world_formation_draw_egg_sprite` uses body UV(233,184)18x24, crack UV(216,184)
16x16, tpage0x65 and four CLUTs at(320+16*color,255). Countdown1 adds the crack.
The prototype reduces the body to12x16 within the existing16x26 formation cell
and applies the established offline white outline. Each source body's luminance
range is fitted to four greys: direct common thresholds turned the green egg
into a dark blob. This contrast adaptation is part of the pending picture.
FEG1 is a private1256-byte review bank, not a new installed resource/format.
Before shipping, pool/factor approved art once in shared core resources; do
not retain duplicate art per consumer or assume identical grey variants need
separate physical frames. Growth/skillset tables remain shared.

Preview hashes: before DAB9, egg9C07, menuAA1B, confirmE3DF;
map before DDB3, dateEAAA. PC ASan/UBSan renders match. These are presentation
checks only; no native CPU/frame/heap or Save/Continue claims.

## Independent save/travel preparation

**OBSERVED ORIGINAL FIXTURE:** a read-only restore of the existing Gariland
deployment state (the same input as flow_assets) has event9, month1/day1,
DATE_ADVANCE0, era variable670. State SHA:
`b1d78e072a39093200c28926c495f83ec52019578d158f5ca63b217e4298d6f6`.
No frames run after restoring it; PSX's one initialization frame precedes
restore. The historical state retains its bytes and mtime. Exact values and
provenance: `x/egg-review/calendar-source.json`. The fixture boot skips Orbonne
to Academy with the existing oracle; this is a pilot anchor, not full retail
chronology or recovered legacy elapsed travel.

**TARGET:** use observed Jan1/era0 as the documented pilot baseline for new or
legacy calendar initialization. Older FSV2..6 saves never stored travel days:
retain their current roster/location/story without inventing historical elapsed
travel. Future chapter/event time jumps need their own source metadata; the
original multi-day advance does not run daily breeding.

Next integration must cover these paths together:

- Factor WorldParty's byte layout before Save; append it and durable RNG with
  an explicit new format/version. Preserve every FSV6 prefix/guest offset.
- Initialize all Save producers: title/new game, Academy/new roster, battle
  fresh-party path and scene/session; update title slots, supervisor progress,
  save_dump, fixture adapters, package capability and native layout signatures.
- Advance once per completed route leg before arrival dispatch, never from
  cursor hover, animation points, repeated ENTER, cancel or entry itself.
  Retain native Save buffers until deferred rt_save flushes.
- Move/clear egg/birthday/color metadata with permanent-death compaction,
  guest remove/recruit and every appended/rebuilt record. Egg dismissal needs
  atomic metadata/raw-current/roster changes and safe cancel.
- Bind the approved egg card/menus and date; eggs cannot enter fighter Status,
  equipment, jobs, abilities or placement. Source fighter browsing skips eggs.
- Existing deploy/party selectors reject **all type3 monsters**, not just eggs.
  Keep grown-monster deployability and job-derived source art/name support
  explicit rather than claiming a playable breeding loop from egg exclusions.
- Test actual PC title Save/Continue and map/management child reentry, multi-
  leg travel, full roster, hatch and dismissal. Reuse backend/native arithmetic
  evidence; export changed ABI consumers once stabilized and check only the
  affected native save/layout/overlay boundaries. Keep the16KiB gate open.

The visual gate is the project's "Visual changes are validated by the user"
skill rule. The backend/save tasks remain within the user's authorized scope;
do not integrate, optimize or benchmark a new picture before its verdict.
