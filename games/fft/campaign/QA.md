# Campaign playtest, 2026-10-22

**Current audit —2026-10-24:** [QA_STATUS.md](QA_STATUS.md) gives the present
status of every checklist ID and later title/package report. Not all QA is
closed: B06/B07 are unreproduced, the shared font is implemented as a visual
candidate pending approval/integration, title selection remains open, and a
full current-group playthrough is pending. B01 retains the full-resolution
renderer accepted by the user, with expanded two-chapter input/geometry checks
and no new LCD timing claim. The sections below are chronological history,
including superseded "not implemented" and "not packaged" statements.

**Latest scope —2026-10-23:** perspective is retired to experiments. Both
Gariland scenery widths are now selectable in MODE for the user's actual
test; Orbonne retains its current scenery. See [EDGE_WIDTH.md](EDGE_WIDTH.md)
for the completed native checks, controls and472686-byte test group. This is
not a final width choice or a claim that all QA reports are closed.

**Title A/B and QA follow-up —2026-10-23:** [QA_TITLE_FOLLOWUP.md](QA_TITLE_FOLLOWUP.md) records the normal-start memory failure reproduced on private Titanium and the corrected packed path reaching Agrias, plus native Continue/Data2. Academy pan clamping, shortcut99 joining the full Gariland/map route, and the first-frame premature-victory fix are implemented. Serif title/result reveals and1px/2px scenery are marked review candidates. A/B remain isolated; campaign perspective is explicitly still absent and B06/B07 remain open.29 campaign +18 opening/chapel native replays,18 current/A and29 B title cases and split native flow checks pass.

**Package memory check —2026-10-23:** [PACKAGE_CHECK.md](PACKAGE_CHECK.md) records an isolated Titanium run. Plain archived map/management retained36550bytes under the supervisor; packed stubs reduce that pair to2020bytes. Deployment → battle → map/management twice → battle passes, with repeated returns stable at186980bytes free and the second battle visibly playable. The pre-title candidate group is452879bytes. Title-enabled full packaging/presentation and B06/B07 remain open; no live session or real save was changed.

**Title/three-save candidate —2026-10-23:** see [TITLE.md](TITLE.md). Source PS1 title art, New Game/Continue, shared animated hand and three isolated slots are implemented. Legacy saves are preserved/imported explicitly; overwrite defaults to Return.18 title PC/68000 cases and the88-assertion battle flow on slot2 pass.29 campaign +18 opening/scene replays retain all139 reference PNGs. Requested work order is2(package),1(QA/title),4(core mechanisms). Review images are under `x/qa/title/review-1`; final title packaging/presentation awaits the visual verdict.

**User approval —2026-10-23:** deployment review-11 is accepted, including A's2px arrow with horizontal lower edge. The decision marker is cleared. This closes that visual review; full native campaign packaging/heap/presentation checks and B06/B07 ghost reports remain open.

**Arrow A selected:** the doubled2px contour is now active, with both lower vertices at y87 to make its bottom edge horizontal. Tip/placement/menu remain unchanged. Review-11 supersedes review-10's A/B decision marker. Deployment unit and15 PC/68000 replay checks pass; see CHOICE_MENU.md.

**Arrow A/B pending:** `review_deploy_arrow.py` generates isolated current/A/B builds: A doubles the contour, B reduces height12->10px. Active deployment stays unchanged. Six PC/68000 cases pass; changes stay within the arrow region. Marked `x/qa/flow/review-10/deployment-arrow-ab.png` supersedes review-9; see CHOICE_MENU.md.

**Placement triangle shape:** review-9 increases the arrow height8->12px, preserving its accepted tip position and making the hollow triangle clearer. Vertices(56,75),(61,86),(84,87). Only its silhouette changes. Review-9 supersedes review-8's decision marker; see CHOICE_MENU.md.

**Placement arrow height:** raised11px from review-7, leaving the tip about2px below the grid; left offset and shape retained. Footer separator is continuous again. Review-8 supersedes review-7's decision marker. See CHOICE_MENU.md.

**Placement arrow offset:** moved30px left following the annotated screenshot, keeping shape/height and hand/menu unchanged. Footer gap/help follow the arrow. Review-7 supersedes review-6's decision markers; see CHOICE_MENU.md.

**Source-photo deployment refinement:** the straight-down indicator was rejected. A long inclined ground-plane wedge now sits below the grid, as in the user photo. The shared animated hand is closer to text; confirmation height33->27px. Existing geometry/stats/placement are unchanged. See [CHOICE_MENU.md](CHOICE_MENU.md). Review-6 PNG and actual-frame GIF supersede review-5; other decisions remain pending. Scoped unit/native checks are recorded there.

**Deployment direction/menu follow-up:** [CHOICE_MENU.md](CHOICE_MENU.md). The orientation indicator now points south, centered below the grid. Start Battle / Return uses the shared tabbed menu and white hand. Layout, frame/rows, selector and wrapping navigation are shared with battle, management and world map; owners retain validation and caches. Review-5 supersedes review-4; other pending visual decisions remain. Campaign/root/opening unit suites pass, including648 walking poses.29 campaign +18 opening/scene +8 deployment PC/68000 replays pass, with all139 preserved reference PNGs byte-identical. Gariland/Orbonne flow retains74+14 assertions and matching saved bytes/payout guards. PC entries build; no live emulator or shipping bundle changes.

User evidence is local in repository-root `fft_bugs/`. This checklist supersedes
the previous management/world-map presentation choices where the new requests
conflict. A rotating job carousel is now explicitly requested. Do not treat the
older static management layout or world-map motifs as approved.

**Latest common UI/placement revision —2026-10-23:** [UNIT_CARD.md](UNIT_CARD.md).
A shared configurable renderer now serves management/deployment, battle HUD
and Status. Existing pictures remain byte-identical through29 campaign and18
opening/scene native replays. Five deployment replays and88 flow assertions/save
checks pass PC/68000. Placement adds the requested depth0 outline-only5x5 grid,
bottom-left orientation arrow and complete left/right slab rims (white edge,
dark outer keyline on white ground). The marked review-4 placement sheet
supersedes review-3. No full package/hardware update; the new callback requires
matching `ffbm` and core, with context-size validation in the module signature.

**Previous placement revision —2026-10-23:** the user requested no light-gray cells
and thicker slabs. Nondeployable cells are now omitted; eight legal cells remain,
with four-pixel side depth instead of two. Rules, cursor navigation and saves are
unchanged. Deployment tests and five actual68000/PC UI/save replays pass. The
new marked comparison is `x/qa/flow/review-3/new/deployment-slabs-review.png`;
the review-2 placement sheet/GIF markers are cleared, other decisions retained.

**Previous review follow-up —2026-10-23:** Gariland title on black; actual Formation
detail layout shared by deployment; raised white-rimmed tiles and corrected
tile/actor paint order; F1/F5 previous/next, including wrap; trophy icon/name rows
centred together.88 battle-flow checks and10 flow UI/save replays agree PC/68000.
New before/after sheets and actual-key GIF are marked gallery decisions under
`x/qa/flow/review-2/new`, replacing the first flow markers. White-priority edges
have four isolated Gariland comparison views; source depth/masks stay unchanged
and active scenery banks are untouched. See FLOW.md for the outstanding verdict.
Original multi-squad deployment is confirmed: Gariland has one area; event39/
MAP034 has two2-unit groups. Future native maps must preserve distinct grids,
per-group capacity, upper level and unique roster assignments, without cloning
Gariland's grid. Source pyramids/arrows are also cursor/orientation indicators.

**Previous flow batch —2026-10-23:** the user requested V01/S01/S02 and reactivated
V02. All four now have actual runtime candidates; see [FLOW.md](FLOW.md).
The four `x/qa/flow/new/flow-*.png` sheets were the initial marked gallery decisions
(superseded above).74 Gariland +14 Orbonne native/PC checks
and10 flow-menu input/pixel/state/save replays pass, as do campaign unit tests.
FSV3 preserves FSV2 parties while adding funds and deployment. Approval, complete
new package/heap checks and hardware presentation remain pending. B06/B07 remain
open; these scene changes do not establish a ghost-silhouette fix.
The subsequent regression run passes29 campaign and18 opening/scene native
replays without refreshing historical title references. See FLOW.md for scope.

## Battle regressions

| ID | Report / evidence | Investigation / acceptance |
|---|---|---|
| B01 | F1/F5 flicker / angular reversal | Campaign lacks `FFT_TURN_LOWPOLY_NATIVE` and uses buffer flipping, unlike the reviewed native 160x100 fixed-address presenter. Restore the selected renderer; independently check displayed1/2/3 order, both keys, all views. Historical1.2–1.5s is not a measurement of a changed build. |
| B02 | Lavian: Bronze Helme | FCJ4 stores only12 name characters. Preserve complete source item names, including Bronze Helmet; verify Status and equipment detail. |
| B03 | Move/attack selected tile lacks immediate hidden pattern | `cursor_mask` skips AI and can postpone player masking; selected-tile opacity must use the same depth/dash rule from its first visible frame. Evidence: Move_case_on_hover_is_not_masked_Checkered.png, Select_attack_case_not_masked_checkered.png. User priority: low. |
| B04 | Top-edge actors/diamonds/damage clipped | Camera clamps at scene row 0, preventing headroom. Review safe virtual blank rows; audit copy/cache/patch clipping rather than reading before the scene. Evidence: orbonne_screen_too_short_cant_see_on_top.png. |
| B05 | Wait direction arrows90degrees off | Arrow selection still uses the historical unreflected projection. Validate arrow direction, sprite facing and left/right input in all 16 facing/view combinations. Evidence: wait_arrows_wrong_orientation.png. |
| B06 | Old silhouette after knockback | Reproduce Throw Stone, Dash and critical attack, including recoil, final position, covers and cache reuse. A screenshot alone does not establish which cache is stale. Evidence: bug_throw_stone_elle_recule_et_ensuite_son_mask_reste_ancienne_case.png. |
| B07 | Misplaced/hidden Delita and active actor | Check B06 plus terrain cover, intentionally highlighted target, draw order and grayscale presentation history across all views. Evidence: masked_weird*.png. Keep unresolved until reproduced or native capture demonstrates the fix. |
| B08 | Missing scenery black/white edges | Existing generators contain offline ledge contours for both maps; audit shipping banks against those generators. Distinguish map structural edges from actor outlines. Restore reviewed edges, no invented terrain geometry. |

## Victory and scene sequence

| ID | Request | Acceptance |
|---|---|---|
| V01 | Original-style three victory screens — **runtime candidate ready** | Scenery-backed congratulations, bonus money, then live war trophies with source item icons. Native/PC rewards and confirmation/save paths agree. **No such screens after Orbonne**. Visual/package review pending; see FLOW.md. |
| V02 | After-victory map staging — **reactivated by user, candidate ready** | At cutscene entry hide enemies/bodies/crystals/chests, preserve battle/reward data, face surviving allies toward the scene and keep bystanders still; KO allies remain KO. Existing Orbonne choreography retained. Actual-renderer review and packaging remain pending; see FLOW.md. |
| S01 | Source cinematic order | White Orbonne title → chapel scene → fade → outside/dialogue → battle → Orbonne aftermath → dark Academy narration → ChapterI title → location title → normal Academy dialogue → fade → deployment → Gariland title → map/dialogue → battle → dialogue → victory screens → world map. Evidence: military_1..4.png, gariland_presenation_title.png. |
| S02 | Missing deployment — **runtime candidate ready** | `fftdep(200)` now precedes Gariland: five of seven, source legal tiles and automatic mandatory Ramza, explicit confirmation/save, selected battle positions and reserves preserved. Visual/package review pending. Evidence: miss_placement_screen_right_after_orbonne_victory.png; see FLOW.md. |

## World map

| ID | Request | Acceptance |
|---|---|---|
| W01 | Alternative map designs | Present same-scaleA/B/C proposals before replacing scenery. Current motifs are rejected. |
| W02 | Animated Ramza | Ordinary idle/walk poses from the source, current position preserved; management remains neutral/still. |
| W03 | Circular locations | Replace square markers with readable circles, distinct town/battle states. |
| W04 | Main menu | Source-style Move / Formation / Brave Story / Tutorial / Data / Option menu, fitted to160x100. Unsupported entries need an honest defined behavior. Evidence: map_miss_main_menu.png. |
| W05 | Free hand with magnetic location selection | Source behavior is the user's recollection: verify locally. Move hand freely, attract near visible nodes, confirm explicitly before travel/interaction. Never start travel on hover. |

## Management

| ID | Request | Acceptance |
|---|---|---|
| M01 | Neutral standing poses | Current generator extracts SHP frame9. Identify the actual neutral source pose; do not substitute a walking extreme. |
| M02 | Compact Formation/Status title bar | Recover space while retaining readable title and fitted margins. |
| M03 | Five-column roster, diamond selector, details | Remove per-cell level labels/hand, show five characters across, selected portrait/name/HP/MP/details as in managemnt_home.png; support dense rosters and last page. |
| M04 | Rotating job carousel / discovery | Review managemnt_jobs.png adaptation. Hide undiscovered jobs; Source rule found: show jobs unlocked by another party member in gray; hide jobs locked for everyone. Existing unlock eligibility is not proof of discovery. Unknown jobs must not leak names. |

## Review and completion boundary

Every changed picture needs a same-view before/after or alternative preview,
marked `ti-view --decision`. Keep review artifacts under ignored `campaign/x/qa`.
No replacement of the user's live TiEmu or shipping bundle until review. Native
memory correctness and logical checks precede any hardware presentation test.
No whole-core completion claim follows from these QA fixes: CORE_STATUS.md still
tracks the missing rule families, capacity and ChapterI content.

## First candidate batch

Source corrections, not yet repackaged or launched into the user's session:

- B01: native 160x100 terrain turns are enabled in the campaign, with live height
  contours. The native wrapper selects the existing fixed-address copy presenter
  used by the previous ordered-frame control. 40 actual68000/PC source, three-angle
  and destination pictures agree for both keys/all views (`A3E5393D`). The new
  campaign presenter still needs an isolated LCD capture to establish displayed
  order and elapsed duration; the review GIF uses an illustrative PC cadence.
- B02: FCJ5 expands only the shared Item record 26→30 bytes, retaining all source
  names (longest 15 characters) in 17-byte fields. Both chapters/management reject
  incompatible FCJ4 banks. 256 item names match the disc; native catalogue hash
  `164ABDB3`. Archive data grows 1024 bytes, to 39,610 bytes. No save layout changes.
- B03: cursor masks are mandatory from the first frame for player and AI,
  regardless of optional budget. Aiming highlights retain terrain cover when
  redrawn over other units. 15,902 native/PC checks include an independent
  per-pixel oracle, 16 sprite-facing/Wait-arrow pairs and fresh post-knockback covers
  through view changes; screen digest `BBAA1186`. The cursor comparison explicitly
  recreates the old full-marker fallback on the same tile, rather than claiming
  a second live-emulator capture.
- B04: up to 39 virtual blank top rows permit camera headroom. Copying clips to
  actual source row 0 and clears the padded rows; the screen cache and patches
  continue to agree. No larger scene/framebuffer allocation is introduced.
- B05: arrow selection reflects screenX just like the source camera; sprite
  facings and independently projected world directions agree in all 16 cases.
- B06/B07: tested knockbacks do not reproduce a stale cover or feet cache.
  These reports remain open. Target highlighting now preserves terrain masking,
  but that alone does not establish the cause of persistent shifted silhouettes.
- B08: confirmed Gariland packaging regression: `campaign_assets.py` mirrored the
  old unstyled root views. `campaign_scenery.py` restores the already selected
  contrast/edge colour planes, verifies original depth/mask bytes before the shared
  reflection/crop, and the Makefile makes them real bank prerequisites. All 27
  campaign replays pass with the restored colours. Orbonne already had contours.

The shared engine's unit suite, 27 campaign  + 12 opening + 6 scene native replays
pass. The extended PC QA harness also passes ASan/UBSan (excluding established
fixed-point shift idioms), including all 39-row camera padding and cursor fixtures.
No regression references were refreshed. Full program TTPack margin, live heap
and changed presentation costs must be checked after the visual verdict; older
size/timing figures are historical, not current measurements.

Isolated design candidates from `tools/qa_designs.py` / `preview_management.c`:

- `x/qa/map-designs-review.png`: three same-scale maps with circles, a menu layout
  and a free-hand example; `map-ramza-review.gif` shows source idle poses. Free
  movement, magnetism and menu commands are not implemented by these pictures.
- `x/qa/management-review.png`: neutral SHP 11 instead of walking extreme 9,
  five-column roster/diamond/selected portrait and 7 px Formation/Status titles.
  `carousel-review.gif` illustrates rotation/selection. Discovery gray previews
  remain provisional until the original discovery rule is observed. Shipping
  management code/banks are unchanged.
- `x/qa/victory-review.png`: three scenery-backed title/reward layout sketches,
  with source-reference example amounts explicitly labelled outside the LCD.
  No reward calculation or scene sequencing is changed by the mockups.
- `x/qa/battle-review.png`, `rotation-review.gif`, `scenery-review.png`: concrete
  candidate corrections and same-view controls. Derived artwork stays ignored.

S01/S02 (cinematic order, titles and real deployment) remain implementation work;
V02 remains explicitly deferred. The current 33-variable FCJ4 group and the user's
running TiEmu have not been replaced. Rebuild a complete matching FCJ5 group after
review; never send new individual catalogue/module banks into that old session.


## User verdict and second candidate batch

The user approved battle/scenery/rotation pictures, animated map Ramza, selected
world texture **C** (source relief and old lettering), and the three victory layouts.
Requested refinements: a hollow floor diamond around the selected actor's feet,
HP/MP bars like battle, a better level position, outlined roads and source trophy
icons. There must be **no rewards sequence after Orbonne**. The new refinements
remain a visual review; the subsequent clarification removes the carousel diamond while retaining
and thickening the Formation floor diamond. See the third batch below.

- B03/B04 remain active fixes, not deferred reports. In addition to39 virtual top
  rows and mandatory first-frame terrain dashes, long-range action framing now
  preserves both bodies' headroom when they fit and prioritizes the target when
  they cannot. `action-headroom-review.png` shows the same injected Orbonne view
  before/after the midpoint-framing correction. This extra framing picture is new.
- B06/B07: `check_qa_motion.c` primes feet, terrain covers and the scenery cache
  **before** the knockback. It checks every impact/recoil phase for weapon,
  Dash and Throw Stone across all four source views, including a one-decode
  animation budget.5724 checks agree PC/68000, screen hash `A22F5484`; ASan/UBSan
  passes. These fixtures still do not reproduce persistent ghosts. Do not claim
  the presentation-layer fix cures those user screenshots without an LCD check.
- M01/M02/M03/M04 candidates now use actual `manage.c` behavior: neutralSHP11
  banks (unchanged bank size), five columns with dense16-member navigation,
  selected portrait/details, compact7px titles, HP/MP bars and level beside name,
  floor diamond and a controllable animated job wheel. Confirmation changes the
  selected job and saves it. Discovery matches local reference
  `world_job_build_unit_list(mode2)` / `world_job_is_locked_for_all_units`: jobs
  known by another roster member appear gray, jobs unknown to everyone are absent;
  gender restrictions still apply. The preview sets up a second-unit discovery
  solely to demonstrate gray entries; it does not modify the campaign save.
- W01/W02/W03/W04/W05 candidates use FWM2:51,416 bytes versus19,616 previous
  bytes (+31,800). Of these,47,616 are the exact two source texture planes and
  1,248 the eight Ramza poses. The texture is read directly in archive; no complete
  decoded world-map heap block, tile merging or lettering loss. Circular nodes,
  free-held hand movement, a12px magnetic radius on release and explicit ENTER
  before travel/interaction are implemented. The source feel is an adaptation;
  precise original cursor timing/magnetism has not been measured. ALPHA/DIAMOND
  opens the source-style menu; Formation and Data/save work. Brave Story, Tutorial
  and Option are visibly unavailable. Outlined roads now use the actual candidate renderer; all five map PC/68000 replays pass again.
- V01: `victory-icons-review.png` adds the real Mythril Knife/Phoenix Down icons,
  outlined for the dark scenery. It remains a layout sketch with example rewards,
  not implemented result accounting. Do not show it after Orbonne.

`manage-test` checks discovery, payment/equipment/save rules and16-member paging.
`map-test` checks free movement, no travel on hover, magnetism, committed travel,
arrival, Formation and location persistence.13 menu/map PC/68000 replays pass, plus three moving/settled job-wheel pictures. Dense paging also restores the visible page after switching units in Status.
Battle QA remains15902 checks (`BBAA1186`) and40 turn frames (`A3E5393D`).
Current package/heap/presentation timings are still unmeasured, and the live
FCJ4 group/TiEmu have not been touched.

### Source checks for the remaining scene/deployment/reward work

`tools/qa_source_audit.py DISC.cue campaign/x/mandalia/gariland-deployment.state`
reads the original ATTACK overlay without inputs. It confirms Gariland event9,
MAP022, ENTD0x184, squad256, bitmap0x00c61086, center(5,12), **five** player slots
and **seven** initial roster recruits. The existing fixed selection must not be
mistaken for the complete roster. Implement roster creation before deployment,
legal placements and reserve preservation together; never discard two recruits
to fit a fixed battle fixture. The source result routine reads current present
units' trophy/money fields, not the native demo's picked chest list. The audit
exports initial reward inputs but deliberately does not claim they are final
awards. Money persistence, real deployment, source cinematic order/titles and
Gariland pre/post-battle dialogue remain implementation work. V02 remains the
previous explicitly deferred request.


## Third candidate batch —2026-10-23

The user clarified Change Job: **no carousel diamond**; the front/central choice
already identifies selection. Formation keeps its floor diamond, with twice the
stroke thickness, and removes the gray floor lines under each roster actor.
The world-map command menu now follows the combat command panel (raised black
label, light-gray framed body), with its glove outside the text columns; the map
cursor glove is hidden while that menu is open. Routes have a two-pixel white
clearance. `x/qa/review-3/new` contains actual-renderer same-view comparisons and
new contrastA/B proposals. None of these is a new shipping-package launch.

### Compression without changing approved pixels

The active source-texture bank is **FWM3,33,218bytes** versus FWM2's51,416:
**18,198bytes /35.4% saved**. The full43-node/48-route map, metadata and animated
Ramza remain present. Comparison variants: strong contrast30,906bytes; light
paper32,316bytes. Both are pending visual selection; the active data remains
source contrast. Do not use a changed quantizer as evidence of lossless savings.

- Texture: independent8-row ZX0 streams, with a16-slot strip cache. This uses
  15,872 decoded bytes, not a47,616-byte whole map. Five shared neutral/walk poses
  reproduce the eight original idle steps byte-for-byte: the sprite section
  shrinks from1248 to519 archived bytes (including the pose sequence).
- Total map cache/sprite heap:16,652bytes. Map and battle are separate programs;
  this is still a real additional allocation versus FWM2's direct archive reads.
- Existing verified `zx0_asm` is used on TI; no new assembly. Every native map
  build/check now links `lib/unpack68k.s`. FWM2 banks are rejected by the new reader;
  do not mix banks into the user's old live group.
- Exact8x8 tile dedup found2661 distinct tiles out of2976; compressed atlas plus
  cell indices costs31,400bytes before other metadata.16x16 gives733/744 tiles
  and28,436 packed bytes, only68 fewer than one compressed full bitmap, with
  cell indirection/full-atlas decoding. The chosen small-strip representation
  prioritizes bounded decoded storage and scroll locality. ZX0 reuses identical
  portions of related patterns without replacing slightly different pixels.
- `make world-storage-check` compares every visible byte in240 camera windows,
  all bit shifts, all map edges, forward/reverse cache eviction and eight idle
  poses: **248 checks, hashE65DF071, PC/68000 equal**. The reference comes from the
  uncompressed source picture. This is not a full live-calculator budget claim.

Background-only performance (MC68000 datasheet timings, no grayscale/interrupts):
the former byte-copy loop averaged477,482 cycles for a warm viewport, peak576,482.
Reusing battle's word/long copy shifts reduces this to98,322 average /112,140 peak,
with all248 source-pixel/cache checks unchanged. Calls including cache misses peak
at792,030 cycles when jumping to uncached areas. These are isolated texture-copy
measurements; new road/menu rendering and the complete native map still need their
full-frame budget audit after visual review. No new assembly was written.

### Cursor reactivity checked through actual updates

`check_cursor_frames.c` exercises **32** first-visible-frame transitions: four
views × Move/Attack/Throw Stone/Charge × player/AI. Player cases send the real
arrow input through `game_update`; AI cases advance the ordinary cursor-glide
state. Reach artwork is already present and the former cursor/mask are cached.
The first new rendered frame is compared to an independent depth/pattern oracle,
then checked again after eight updates. **1088 PC/68000 checks, hash81047F76**,
pass. This proves immediate framebuffer masking, not the elapsed response of the
old TiEmu session. Battle sources were not changed further by this follow-up;
the existing mandatory-mask fix is the code under test. Persistent actor ghosts
remain a separate open report.

### Honest image dates

The old all-in-one review script rewrote historical sheets/GIFs and made their
file modification dates look new even when the picture was identical. This
follow-up uses `qa_refinements.py` with versioned output and temporary native
captures. `review_files.py` publishes only changed bytes; `check.py` and
`check_qa.py` also avoid rewriting identical test captures. The historical broad
`qa_designs.py` regeneration now requires explicit `--all`. Dates are not forged
or used for gallery priority. A repeat of the new review retained exact bytes
and nanosecond modification times on **247 existing images**. A changed picture
still gets its real new modification time. Gallery decision markers remain
separate from image timestamps. The listing now explicitly says "Fichier modifié
le" with a full day/month/year and time; previously the clock alone could make an
old capture look like today's. The listing version refreshes already-open clients
without touching any capture date or decision priority.

Current checks additionally pass management/map rules,15 menu/map PC/68000
replays and3 moving-carousel pictures. New decisions supersede the earlier
management/map/carousel markers; unseen victory-icons/headroom/scroll-indicator
reviews stay pending. Source cinematic order, true pre-Gariland roster/deployment,
real reward accounting and the whole-core scope remain as recorded above.

## Fourth candidate batch —2026-10-23

User verdict: retain **the same source texture with stronger road contours**.
The strong/light contrast alternatives are superseded. Remove the remaining
gray floor bars from Change Job; its diamond stays absent. `make manage-test`
passes after removing that single draw call. New still/animation comparisons
are under `x/qa/review-4/new` and replace the former carousel review.

The user requested more world-map context from Gariland: the road north of
Mandalia otherwise reaches the top edge. Source coordinates confirm Gariland
(131,174), Mandalia(89,163), Igros(75,125). With the camera centered on Gariland,
Igros's marker center is at screenY1, cutting its top half. Isolated75% and67%
offline-scaled FWM3 candidates put that center atY13/Y17. The same road contour,
circular marker size, font and command/cursor renderer stay readable in LCD
pixels. The source palette is unchanged; nearest resampling changes spatial
detail and therefore requires a new visual verdict.

`tools/world_zoom_review.py` decodes the existing bank, reduces the texture,
locations and all route points with one scale convention, and reduces the same
five existing idle frames around their(8,22) feet anchor. It restores a one-pixel
white rim after reduction, preserves all eight pose indices, and pads the small
poses to the existing16x26 bank layout. No new sprite rip, runtime scaling, new
assembly, format migration or shipping asset replacement. Candidate banks are
21,622bytes at75% and18,066bytes at67%; these are preview files, not a package or
native cycle/heap claim.75% is recommended for readability, pending user review.

The comparison has two explicitly labeled rows: current campaign locations,
then a **context-only** fixture additionally showing Igros and its northbound
road. Igros is not unlocked by the active campaign or written to the save.
The actual C renderer supplies every image; `map-zoom-native.png` also gives
the160x100 pictures at native size, and `ramza-zoom-review.gif` compares the
reduced existing idle animation. GIF cadence is a review cadence, not measured
calculator FPS. Original bank/save/live TiEmu stay unchanged. Zoom integration,
native checks and benchmarks wait for the user's visual selection.
Repeated generation preserved exact bytes and nanosecond modification dates of
all312 existing QA images. The gallery lists the new comparisons with decision
markers; the previous contrast/road/carousel decisions have been cleared.

## Selected67% integration —2026-10-23

User verdict: **"go67"**. `world_assets.py` now defaults to2/3 offline scale for
texture, every source location/route point and the same existing idle poses.
The production bank is **18,066bytes**, byte-for-byte identical to the selected
review-4/67 candidate. Gariland's actual PC picture is pixel-identical to its
approved preview. Source contrast and the stronger road contour stay selected;
no runtime scaling, new sprite rip or new assembly. Igros's extra visibility
exists only in comparison/test fixtures, not in campaign progression.

The map is336x256 (padded as before). Strip/sprite heap falls from16,652 to
11,532bytes; the bank falls from33,218 to18,066bytes, or64.9% below the original
51,416-byte FWM2. This reduction includes the approved spatial resampling; only
ZX0 packing is lossless relative to that selected image.

Full-frame profiling exposed the repeated immediate-mode roads/markers as
unnecessary work. They now paint once into a decoded strip, before the ordinary
background copy. This uses the existing strip allocation plus a43-byte visibility
table and small cache-state fields. Story flags are fixed during the map module;
arrival recomputes visibility and preserves strips when that set is unchanged.
An unchanged already-shown destination no longer causes a full cache rebuild.

Validation:

- World navigation/arrival/menu/save unit tests pass.
- **696 PC/68000 checks**, hashDA3A7C1C, zero failures:432 raw viewport comparisons
  (all16 word shifts, edges and cache eviction),8 idle poses, and256 cold/warm
  decorated viewport comparisons against the retained original line/marker
  renderer. The latter cover128 views with current progression and an isolated
  all43-location/48-route fixture. Every visible byte is compared directly;
  the aggregate hash alone is not the pixel oracle. ASan/UBSan also passes.
- **7 map replays** match PC/68000 screens, state and final save. Their7 pictures
  are pixel-identical before/after decoration caching. No reference was refreshed.
- Native map program/archive rebuilt separately. No new hardware code; no keys,
  restart, individual bank transfer, complete group rebuild or push.

Full-map MC68000 datasheet CPU evidence, in `x/world-storage/*native.log`:

| Case | Update average / peak | Render average / peak |
|---|---:|---:|
| Before decoration cache,120 idle frames |440 /440|470,237 /464,742|
| Cached,120 idle frames (includes entry)|440 /440|190,977 /177,586|
| Gariland-Mandalia trip,428 updates|2,907 /53,334|109,297 /207,992|
| Free pan through map limits,624 frames|52,298 /52,850|145,021 /415,666|

The runner's peak excludes the initial warm-up; its average includes it. A
separate one-frame entry measures **1,808,576 render cycles** (about151ms CPU at
12MHz), total boot/check wrapper2,354,654 cycles. Free-pan strip-cache misses
still produce isolated frames above the360k target. These are complete map
render/update zones, but exclude grayscale, interrupt/presentation waits and
real Flash/LCD effects. No claim of measured calculator FPS or a universal30fps
frame budget. The live oldFCJ4 campaign group remains untouched. Remaining
visual decisions and full native campaign packaging are independent of this
approved map integration.
