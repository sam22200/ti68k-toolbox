# User campaign prerequisites (2026-11-13)

These requests precede the installed-route automation in CHAPTER1_ARCHIVE_PLAN.md.
The2026-11-13 extension also puts [breeding, Bar/rumors/Propositions and the
complete poaching economy](CAMPAIGN_SIDE_SYSTEMS.md) before campaign extraction.
Delivered: [scene controls/permanent stats](SCENE_SAVE_FIXES.md) and
[four traps/level-down persistence](TRAPS.md). Evidence: `x/campaign-prerequisites/`
and `x/tile-traps/`. Historical groups/saves stay intact.
Use PC/sanitizers first; only selected changed-layout/native checks at the end.

## Current audit and delivery queue

| Request | Actual code state | Remaining work |
|---|---|---|
| Scene skip/quit | Shared Skip scene? confirmation delivered for map and standalone scenes; default No, paused scene, explicit Yes skips without exiting to title. User approved the box on2026-11-13. | Preserve the tested handoffs when adding choice scenes. |
| God mode | MODE or CATALOG, then ALPHA opens DEBUG; God mode toggles with ENTER/2nd. | Document the keys clearly; no new feature needed. |
| Four trap tiles | [Delivered](TRAPS.md): all four effects, first-record lookup, pickup/benefit/discovery ordering, per-encounter source-map identity, reaction/RNG boundaries and FSV5 persistence. Approved box; matching native overlays, frames and package checks pass. | Author future encounters with their source-map ID and source locations; mounts remain out of scope. |
| Monster KO sprites | [Approved and integrated](MONSTER_KO.md): optional FKO1 art renders six source-specific KO families, including small Behemoth/Bomb/Chocobo poses.24 approved pictures and32 PC/68000 screens agree;48 cache entries and layouts remain. | Standing/action monster art, other species/palettes and authored encounters. The human pilot ships no unused KO bank; generic raw-KO support is ready. |
| EXP/JP/level | Saved for deployed party slots by `party_update`; undeployed records survive compaction. | Retain regression coverage while adding level-down/trap behavior. |
| Items/stolen/found/loot | Live inventory, steals/chests/discovery and battle rewards have save paths. Existing save/flow suites cover receipt and caps. | Retain these paths; test new trap/discovery precedence rather than invent pilot loot. |
| Brave/Faith | Saves now keep one quarter of the net battle change, truncating toward zero; initial values and idempotent settlement are checked PC/sanitizers/68000. | Original warning/departure UI remains separate and is not claimed implemented. |
| Degenerator persistence | Current raw stats and source level-down/growth persist across levels/jobs. FSV5 adds a two-byte roster bitmap; PC/sanitizers and actual native overlay/save checks pass. | Does not reconstruct historical PS1 growth for legacy native records. |
| Permanent death | KO counts3..0 survive; crystal/chest/gone removes party slots at victory. Undeployed members and other records survive compaction. | Keep regression tests; scripted objective losses remain distinct. |
| Consecutive battles/save proposal | `pack_story.h` already calls successive steps directly; `FP_SAVE_PROMPT` calls the shared Save screen between completed steps. No map is required between steps. | A real authored chain/integration test; this is backend support, not completion of every retail chain. |
| Imposed guests | Fixed TEAM_GUEST encounter slots survive party loading and run AI (including current pilot/monster fixtures). They are not treated as ordinary deployed/recruited party slots. | Encounter-specific guests and their cross-fight story progression/content; distinguish guest metadata from permanent roster persistence. |
| Objective types | Shipping `battle_outcome` is ordinary elimination plus mandatory Ramza permanent-loss/allied incapacitation defeat. [Prepared policy and PC review](OBJECTIVES.md) cover named targets/protection;25 original MIPS cases and18 PC/sanitizer boundaries pass. | Visual verdict, validated encounter binding, runtime/action/Reraise boundaries, banners/pulses and exactly-once aftermath/save integration. Prepared code is not shipping behavior. |
| Story choices | Scene steps only play titles, stories, cards, narration and overlays; no choice/effect opcode exists. | Recruitment/refusal and conditional story effects through the common menu, exactly-once saves/flags. User's final phrase about other effects awaits clarification. |
| Monster breeding | Recruited monsters can be saved as type3. No persistent world date, egg generation/hatch lifecycle or egg roster state exists. | Fixed offspring, source family/variant/timer rules, shared travel clock,16-slot/full-roster handling, save migration and reviewed egg UI. |
| Bar / rumors / Propositions | World-map menu placeholders are not Bar functionality. No town/story-filtered rumors, dispatched workers or saved job/return state. | Chapter I rumors/detail text and conditional effects; reusable fee/duration/away-worker/JP-gil/collection settlement, gated to Chapter II onward. |
| Poaching / Fur Shop | Support471 has a one-shot kill/loot consumer, but directly credits the blue party's inventory. No separate Fur Shop stock or purchase flow. | Source eligibility/stock/access/price rules, atomic purchases, breeding/allied-monster interactions and migration from direct receipt. Preserve the Chapter III shop unlock; machinery is a pre-extraction requirement. |

## Delivered scene correction (approved2026-11-13)

ESC opens Skip scene? with No selected. No, ESC or SHIFT resumes without changing
story progress. Yes via ENTER/2nd skips to the appropriate battle/aftermath handoff;
it never means quit-to-title. The prompt pauses scene timers/fades. Pending fade
stages determine whether the skip is before or after a battle. An old cancelled
battle-quit selector cannot open an aftermath prompt by itself.

The common `choice_menu.inc` supplies frame/rows/animated glove. Art remains one
shared archive tail; a small allocation-free tail validator is reused by map and
standalone scenes. Historical extraction controls borrow the same first-view
artwork when ffcu has no tail. No duplicate hand bank is introduced.

Review: `scene-escape-before-after.png`; user verdict: "ok pour le visuel,
continue". Approval is recorded in the private evidence directory.

PC/ASan/UBSan map-control tests pass: held opening ESC, default No, cancel,
UP/DOWN, ENTER/2nd, frozen timers, skip into a running battle and skip aftermath
into normal reward pages. Standalone scene unit tests also pass PC/sanitizers. The software backend
receives aligned host-order glove words decoded once from the same archive art;
TI keeps direct archive pointers. Selected real68000 scene/save checks and three
standalone prompt/resume/skip replays pass against final shipping data.

## Delivered permanent-stat correction

Original rule source:
`sources/fft_decomp/src/event/require_apply_permanent_brave_faith_changes_and_collect_warnings.c`.
It applies base + trunc((current-base)/4), then resets the current value to that
permanent result. Native signed constant-division sequences are represented by
unsigned magnitude shifts with explicit sign. No division in a hot loop is added.

`Work.permanent_base[UNIT_CAPACITY][2]` records initial values at fixed/random
unit creation and saved-party load. This adds26 bytes at the cold tail of Work,
outside UI/AI scratch, preserving existing renderer/cache field offsets.
Save settlement resets the baseline/current values to the saved result, so a
second settlement cannot apply the same change twice. Initial party creation and
recruitment use the same captured baseline. Raw24-bit stat bytes are also copied
back for existing slots, preserving future level-down changes.

The preceding scene/save lot used FSV4/3202, ABI88, ModCtx352, Work29308 and
zone8698, rejecting87. [Traps](TRAPS.md) now supersede it with ABI89/FSV5;
Unit112/State2224 and zone8698 remain.
PC and ASan/UBSan save tests pass signed changes-70,-8,-7,-4,-3,0,3,4,7,8,30,
repeat settlement, raw-stat round trip, initial party creation and the existing
KO/inventory/undeployed cases. A selected real68000 carrier passes198 checks,
including virtual-VAT save/reload. This is not retail-MIPS replay or
persistent-departure certification.

## Resume

Continue with target/protect objectives, guest/chain/choice content and the
new world/party side systems. Full standing/action monster art remains content
work. Keep every remaining item explicit; do not describe the chapter/core
as ready merely because generic combat coverage and archive planning pass.
