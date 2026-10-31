# Current QA status

**UI approval —2026-10-24:** user accepted all latest follow-up previews
("bien pour tout"); eight Decision markers cleared. Visual review is closed
for that batch. A full latest-build hardware playthrough and B06/B07 remain
separate. Work now follows [CORE_AUDIT.md](CORE_AUDIT.md), workstream4.

**Latest batch2026-10-24:** see
[QA_FOLLOWUP_2026-10-24.md](QA_FOLLOWUP_2026-10-24.md) for every new hardware
report, implemented fixes, native evidence and remaining review limits.
New group466045 bytes has not replaced the live472686-byte build. Shared font
is accepted/integrated;1px edges are selected. Required interaction rules:
[GAME_DESIGN.md](../GAME_DESIGN.md). Earlier A/B notes below are historical.

2026-10-24. This is the current audit, superseding the chronological batches in
QA.md. **Not all reports are closed.** Implemented changes, source/native
checks, visual selection and an actual complete playthrough are separate facts.
The earlier group is472686 bytes and was launched on the user's Titanium
TiEmu at their request. Their active session must not receive automated keys.
The user has selected1px after playing; new group removes the2px bank streams
and active width setting.

## Original21 checklist entries

| ID | Current status | Evidence / remaining limit |
|---|---|---|
| B01 | Corrected; full-resolution renderer retained | User reports the return to full resolution looked good. Native160x100 low-poly and fixed-address presentation stay enabled. Current real F1/F5 inputs in all four views of both chapters pass144 assertions;80 rendered frames have matching PC/68000 hashes. Independent active-source-camera geometry checks81840 corners, endpoints and monotonic signed sweep. No new LCD timing/flicker measurement is claimed; user is playing the current build. |
| B02 | Corrected and checked | Complete item names, including Bronze Helmet;256 source names and native Status/equipment paths checked. |
| B03 | Corrected and checked | Mandatory first-frame masking for player/AI movement and attacks;32 real-input/glide transitions,1088 native/PC assertions plus independent depth/pattern oracles. |
| B04 | Corrected and checked | Virtual top headroom, clipped scene reads/cache patches and action framing preserve target details; native/PC and sanitizer coverage. Latest action framing still merits user observation. |
| B05 | Corrected and checked | Wait arrows and sprite facing agree with independently projected world directions in all16 cases. |
| B06 | Open | Persistent silhouette at the old position after knockback has not been reproduced. Warm-cache impact/recoil fixtures pass; this does not prove the user's ghost is fixed. |
| B07 | Open | Misplaced/hidden Delita and active actor remain unproven for the same reason. Need a reproducing state or actual presentation evidence. |
| B08 | Restored;1px selected | New V2 banks retain1px only, without2px stroke streams or MODE width setting. Historical comparisons stay local. Separate white-priority colour proposal has not been selected/integrated. |
| V01 | Implemented and checked; latest visual/runtime review pending | Three result pages, real centred trophy icons/rows, rewards/save and2nd/ENTER confirmation work. No result sequence after Orbonne. Updated white reveals await review and full in-game confirmation. |
| V02 | Implemented and checked; actual sequence review pending | Enemies/bodies/loot are hidden for aftermath, surviving allies face the scene and bystanders freeze; KO allies stay KO. Original combat/reward records remain intact. |
| S01 | Implemented and checked; full playthrough pending | Orbonne, Academy, deployment, Gariland title/dialogue, battle, aftermath, results and map are wired in order. Tests cover stages and native handoffs; there is no complete current-group hardware playthrough claim. |
| S02 | Gariland implemented and checked | Mandatory Ramza, five of seven source recruits, legal placement and reserves; approved orientation indicator, F1/F5 roster switching and shared menus/stats. Multi-area later maps remain capacity/content work. |
| W01 | Implemented; chosen design retained | Selected source texture/old lettering, stronger road contours, approved67% zoom. |
| W02 | Implemented and checked | Same source Ramza idle/walk poses, reduced offline at the approved map scale. |
| W03 | Implemented and checked | Circular location nodes and current/destination states. |
| W04 | Requested menu UI implemented; some commands unavailable | Shared command style and animated outside glove; Move, Formation and Data/save work. Brave Story, Tutorial and Option deliberately report unavailable; their actual features are not implemented. |
| W05 | Implemented and checked | Free hand, magnetic node capture and explicit confirmation before travel. Exact original cursor timing was not claimed. |
| M01 | Implemented and checked | Neutral source pose replaces walking extreme in management. |
| M02 | Implemented and checked | Compact Formation/Status headers. |
| M03 | Implemented and checked | Five columns, thick floor diamond, selected portrait/details, battle-style HP/MP bars and corrected level placement; gray floor bars removed. |
| M04 | Implemented and checked | Animated job carousel without diamond, hidden undiscovered jobs and gray jobs discovered by another party member; save/eligibility checks and dense roster navigation. |

## Subsequent title/package QA

- **Missing data or memory after the chapel:** reproduced with the former group
  and corrected by packing every child program. Private Titanium reached Agrias
  from normal New Game and read Continue/Data2. Later edge/configuration changes
  have headless native checks, not a new full hardware playthrough.
- **Academy endless pan:** fixed fade-bit initialization and exact clamping;
  unit coverage plus actual68000 Academy replay.
- **Shortcut99 missing dialogue / return HOME:** now skips only Orbonne and joins
  the ordinary Academy/deployment/Gariland/map loop.
- **Premature/repeated Congratulations:** first kill render is gated until
  aftermath completion; dedicated native/PC first-frame fixture passes.
- **White reveals:** shared glyph review accepted; new group uses glyphs for
  chapter/location, Orbonne, results headings and money.37 PC/68000 screens,
  6602 bounds/layout assertions, native scene/results and35 title-B cases pass.
  Bank6556 bytes. Old font Decision markers cleared; title A/B stays separate.
- **Shared New Game/Continue/file choices:** implemented with the common choice
  module, animated glove and three isolated save slots; native pixel/write cases.
- **Perspective:** intentionally removed from campaign scope by the user;
  retained under experiments, not an unresolved promise to restore it.
- **Image dates/Decision ordering:** corrected tooling preserves unchanged
  captures and uses independent decision markers. New hardware-QA previews are
  pinned first; accepted shared-font markers are cleared.

## Current white-title GIF review

`review_white_titles.py` builds the actual current C preview renderers with
private data/saves and produces six separate animations in `x/white-titles/`:
`title-intro.gif` (isolated B), `orbonne-title.gif`, `chapter-title.gif`,
`academy-title.gif`, `gariland-title.gif`, and `victory-titles.gif`.
Source images stay temporary; `review.json` records logical/encoded frames,
durations and scope. GIF playback is illustrative, not measured TI speed.
These historical previews do not implement the shared glyph font. Their
Decision markers are cleared. [SHARED_FONT.md](SHARED_FONT.md) describes the
accepted actual-renderer before/after comparison and current integration.

## Rotation regression check

`make rotation-check` runs each chapter in a fresh process, matching the
supervisor's bounded file-cache lifetime. `check_qa_turns.c` exercises real
F1/F5 updates through all three moving steps and the final rotation, checks
unit/RNG preservation, and renders40 frames per chapter on PC and68000.
Hashes are `FBA1A09C` for Orbonne and `5E09B132` for Gariland,72 assertions each.
The independent PC floating-point oracle is test-only and reads active campaign
geometry, never historical perspective/control banks:39600+42240 corners agree
within2px, cardinal endpoints are exact, and every intermediate sweep has the
expected sign. Reports live under `x/rotation-check/`. No renderer changes,
live-emulator interaction or historical1.2–1.5s timing reuse was needed.

## Core scope remains separate

The full combat core is still incomplete: broader statuses, deferred magic,
Jump, performances, reactions/support/movement hooks, action families/AI and
descriptor-driven capacity for later battles remain. Some cold map/AI frames
still exceed the target budget. See CORE_STATUS.md; QA UI fixes are not proof
of completing those systems.
