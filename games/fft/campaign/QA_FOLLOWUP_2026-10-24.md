# Hardware QA follow-up,2026-10-24

**Subsequent user verdict:** "bien pour tout" accepts this batch's visuals;
all eight Decision markers have been cleared. Historical pending-review notes
below describe the handoff before that verdict. Hardware playthrough and
B06/B07 remain separate. Active work now follows [CORE_AUDIT.md](CORE_AUDIT.md).

Required rules: [GAME_DESIGN.md](../GAME_DESIGN.md), linked from working-rules
and the handover. The user's live TiEmu and its saves were not touched.

New42-variable group:466045 bytes, `x/fftcampaign.89g`,6641 bytes smaller than
the472686-byte playtest group. SHA256:
`8f98d7470453ca558cde03557d1d80ae9efe99c6d0d5a8ee12e42674d97b4a66`.
`x/play.89g` remains the old group. No new hardware-playthrough claim.

## Reports and changes

| Report | Implemented change / evidence |
|---|---|
| Deployment F1/F5 and arrows slow | Cache static5x5 geometry/orientation; redraw stats only when the unit changes. Previously roughly1000 tiny grid segments were redrawn every frame. |
| Illegal tile / cursor leaves available cells | Scan only legal cells in the requested direction; stop at the legal boundary. Normalize initial/restored cursor. Direct illegal API placement still rejects. |
| Overhead tile arrow | White floor selection with dark rim; approved orientation triangle preserved. |
| ENTER /2nd placement | Both place the unit, with occupancy/capacity checks; explicit2nd unit coverage and native input replays. |
| Already-deployed selection | Blink the selected actor in place; browsing does not move/save it. |
| ALPHA:go | Code listened to Diamond only; physical ALPHA (`K_D`) now works, with Diamond as an alias. Common Start Battle/Return confirmation. |
| Delayed range removal | Restore all old highlight rows on the first render after cancellation. Optional frame work cannot defer cleanup; backup overflow reloads clean colours immediately. |
| Keep1px edges | Active V2 banks contain selected1px colours and shared depth. No2px stream/active MODE width row. Historical V3 control is gated by `FFT_EDGE_WIDTH_REVIEW`. |
| Vertical map artifact | Decode every source upload rectangle: each2048-byte WLDTEX sector has a rectangle count and multiple8-byte headers. The old concatenation treated internal headers as pixels. New rectangles cover each image once, without holes/overlap. |
| Magnetic glove | Radius12→20 pixels, nearest shown node only. Visible glove catches the node during movement; release snaps its free anchor. Hover never travels. |
| Confirm empty space | Current interpretation: return glove to Ramza without travel/entry. Second confirmation needed to enter. ENTER/2nd unit checks plus native replay. User clarification pending. |
| Data menu | Map flushes pending changes, then supervisor invokes exact shared `ffttitle(3)` file program. Same slots, metadata, animated glove and overwrite menu. Verify full snapshot before updating active-file metadata; cancellation preserves files. |
| Formation diamond | Four-update interpolation of approved diamond, drawn below feet. Page stays cached; interpolation does not recompute stats. |
| Job confirmation | Common choice module, animated glove, Change Job/Return and cancellation. Learn confirmation shares it. Other active management/map gloves are animated overlays too. |
| Job change | Original controller observation shows a light column, not a jump. Native grey/white strips, interior flash, old/new poses over96 target30Hz steps, no effect-image bank. ENTER/2nd skips; SHIFT/ESC can skip/back. Exact original pixels/poses/timing not claimed. |
| Unavailable jobs | Party-discovered jobs unavailable to selected unit use white silhouettes with thin dark rim; discovery/unlock rules preserved. |
| Learn JP | Header shows current job's available JP, separately from ability costs/learned state; purchase updates it. |

## Responsiveness

`x/qa-followup/before/deploy-cycles.log`:16 old native frames, mean render7044582
cycles (about587ms CPU at12MHz). New same trace averages665366 including initial
geometry preparation; F5 render365032 cycles (about30.4ms CPU). Datasheet CPU
estimates, not LCD/input latency. Initial grid preparation still costs more than
a warm frame. Cache reuses the management allocation and is freed on every
deployment exit, including cancellation.

Native light-column78-frame menu/carousel/animation trace: mean260575 render
cycles, maximum357940 at confirmation.130-frame trace: mean270127. No guarantee
is claimed for every future20-job carousel.

## Checks

- Campaign `make test`, followed by later management/map-specific tests, passes:
  legal directions,2nd placement, ALPHA, overwrite/cancel/full snapshot/slot isolation.
-24 current-title and35 title-B PC/68000 complete-image/state/write cases,
  including six Data/save cases. B remains an isolated title choice.
-20 deployment/scene native replays;15 deployment cases rerun after exit cleanup.
  Scene cases include integrated glyph text.
-23 management/map replays: moving/settled diamond, job-effect phases, blank
  confirmation, travelling and save results.
- Range cleanup:144 PC/68000 assertions per chapter, all views, Move/Attack,
  backup overflow and explicitly exhausted budget before render resets it.
  Hashes `68F87CD0` (Orbonne), `3576CBAA` (Gariland).
- Corrected source map:696 PC/68000 source-pixel/scroll/cache/decoration/pose
  checks, `03DDD563`. Bank17668 bytes.
- Shared font:6602 PC sanitizer/native assertions and37 plane screens,
  `EBD63473`. Active flow9736 bytes includes6556-byte font.
- Native battle flow:79 Gariland,14 Orbonne,4 first-frame checks; exact settled
  saves, duplicate-payment guards and trophy overflow preserved.

Reproduce with `make range-cancel-check`, `shared-font-check`, `check_title.py`,
`check_flow_menus.py`, `check.py --menus-only`, `check_world_storage.py` and
`check_flow.py --native`. Keep native compiler invocations serialized.

## Review / limits

`review_qa_followup.py` generates actual C previews with private saves: five
GIFs and map before/after, white jobs and JP views pinned Decision under
`x/qa-followup/review/`. Unchanged images preserve timestamps. Job-effect GIF
pairs an instant control with the column at the same view/scale. Playback
cadence is illustrative, not measured calculator FPS. Visual acceptance and
current-group hardware playthrough remain pending.

`tools/observe_job_change.py DISC STATE` records the original controller-only
reference under `x/qa-followup/source-job/`, with inputs/fixture hash. Source
imagery stays local. Existing world fixture is controlled, not proof of an
unmodified full PS1 campaign playthrough.

Shared font was accepted by "good" and is integrated. Title A/B is separate.
Older B06/B07 ghost reports remain unreproduced/open; this batch does not close
them or the full core.
