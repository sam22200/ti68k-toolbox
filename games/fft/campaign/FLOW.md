# Campaign flow candidates: V01, S01, S02, V02

**Source-photo deployment refinement:** the straight-down indicator was rejected. A long inclined ground-plane wedge now sits below the grid, as in the user photo. The shared animated hand is closer to text; confirmation height33->27px. Existing geometry/stats/placement are unchanged. See [CHOICE_MENU.md](CHOICE_MENU.md). Review-6 PNG and actual-frame GIF supersede review-5; other decisions remain pending. Scoped unit/native checks are recorded there.

**Latest deployment presentation:** [CHOICE_MENU.md](CHOICE_MENU.md). The orientation indicator now points south, centered below the grid. Start Battle / Return uses the shared tabbed menu and white hand. Layout, frame/rows, selector and wrapping navigation are shared with battle, management and world map; owners retain validation and caches. Review-5 supersedes review-4; other pending visual decisions remain. Campaign/root/opening unit suites pass, including648 walking poses.29 campaign +18 opening/scene +8 deployment PC/68000 replays pass, with all139 preserved reference PNGs byte-identical. Gariland/Orbonne flow retains74+14 assertions and matching saved bytes/payout guards. PC entries build; no live emulator or shipping bundle changes.

2026-10-23. The user explicitly reactivated V02. These are implemented review
candidates, not a replacement of the live FCJ4 group. Visual approval, complete
native packaging/heap checks and hardware presentation remain separate gates.

**Latest shared UI/placement follow-up:** [UNIT_CARD.md](UNIT_CARD.md). One
configurable renderer serves deployment/management and battle HUD/Status,
preserving existing cache schedules.29 campaign +18 opening/scene native replays
retain every reference picture byte-for-byte. The overlay context signature now
includes its own size because it gains the shared-renderer callback. Placement
adds the depth0 outline-only5x5 grid, bottom-left orientation arrow and complete
white rims with a dark outer keyline. Five deployment replays and all88 flow
assertions/save checks agree PC/68000. Latest marked comparison:
`x/qa/flow/review-4/new/deployment-slabs-review.png`; review-3 is superseded.

**Latest placement refinement:** hide the light-gray nondeployable slabs; only
the eight legal Gariland cells are drawn. Their side depth is four pixels rather
than two, with dark sides and white rims. Cursor movement and legal-cell checks
are unchanged. `review_flow.py --deployment-only --out x/qa/flow/review-3
--before x/qa/flow/review-3/before` publishes the three-state before/after sheet.
`check_flow_menus.py --deployment-only --out x/qa/flow/review-3` passes all five
native/PC input/pixel/state/save replays, including invalid input and confirmation.
Deployment unit tests pass. The new `deployment-slabs-review.png` is marked as
a gallery decision; the previous placement sheet/GIF markers are superseded.

**Review follow-up:** Gariland's title now uses a black background. Deployment
calls the very same portrait/name/Lv/EXP/job/HP/MP/Brave/Faith renderer as Formation,
and uses separate raised slabs with white rims. Tiles and actors are painted
back-to-front so later grid cells cannot erase actors' feet. F1/F5 previous/next
selection is exercised in both directions and through the roster wrap boundary.
Each trophy row centres its icon and proportional-width name together.
The replacement comparisons and actual-input GIF are in `x/qa/flow/review-2/new`;
their gallery decisions supersede the first four sheets.

## Sequence and state

The active supervisor sequence is:

1. `fftscn(0)`: existing Orbonne white title and chapel.
2. `fftbat(100)`: outside dialogue, battle and Orbonne aftermath; no results pages.
3. `fftscn(1)`: original Academy narration on dark scenery, dark chapter and
   location titles, then normal Academy dialogue with identified portraits.
4. `fftdep(200)`: playable Gariland deployment.
5. `fftscn(2)`: Gariland location title.
6. `fftbat(200)`: pre-battle dialogue, battle, post-battle dialogue,
   congratulations, bonus money, war trophies with item icons, then world map.

The native wrapper and `flow_battle.c` enable `FFT_CAMPAIGN_FLOW`. Historical
direct Gariland/opening controls do not enable it; their old results are not a
preview of this flow. `fftstory(99)` remains a historical diagnostic shortcut,
not a complete deployment/sequence test.

The PC `play.py` uses the same six steps, one process per program. This releases
the software backend's bounded file cache between chapters. It keeps a private
save in `x/play-data`; `--new` explicitly resets that save. `make pc` builds all
entries. No window or TiEmu session is opened by a check script.

## Deployment

`deployment.inc` shares rules between the renderer and tests. Seven observed
source party records supply Ramza, four Squires and two Chemists. Their names,
stats, JP, learned bits and starting inventory are fixed from one unmodified
source recruitment state; native random recruitment is not implemented here.
Funds start at the observed 2,000 gil. The existing portrait/neutral actor and
equipment banks are reused.

The source-oriented 5x5 squad bitmap permits eight tiles and up to five units.
Ramza starts at the observed cell8 (row1, column3), world(4,11). Confirmation
requires Ramza; the player can field fewer than five. Invalid/occupied cells,
duplicate units and exceeding capacity are rejected. Reserve records survive
unchanged. The battle validates deployment again before loading selected slots.

Controls: arrows move the grid cursor using source handedness; F1/F5 changes the
roster member; ENTER places/moves; SHIFT removes; ALPHA opens confirmation;
ENTER confirms, SHIFT cancels that prompt. ESC exits without committing edits.
Saved placements can be reopened. A per-unit stat cache avoids recalculating
equipment-derived statistics every frame.

`tools/deployment_research.py DISC STATE` verifies the original squad variables
and both squad fields in disc scenario records. Gariland has count1, ID256.
There are85 scenario records with two squads (not85 distinct maps); the first
is event39/MAP034, IDs260/261, capacity2+2. The original controller advances with
Start and returns with Cross; L1/R1 selects roster members. Cursor pyramids and
the orientation arrow also appear with a single squad, so they must not be
treated as proof of a second area. The original has four25-cell squad buffers.

The active native deployment remains Gariland only. Later encounters must add
their own grids/world coordinates, per-squad capacities, upper-level bits and
unique assignments across groups. Do not clone the Gariland grid, silently
discard units to fit five saved slots, or add inactive area-switch controls here.
This is a source-verified future content/core requirement, not implemented
multi-area gameplay or a played MAP034 encounter.

## Results and aftermath

Gariland captures live trophy and money-modifier fields before hiding enemies.
The first money page pays the initial enemy-level sum times100. The trophies
page shows live money modifiers times100 and source item IDs. Removed/picked-up
holders no longer contribute their trophies/modifiers. Present KO, crystal and
chest records are retained. Initial Gariland metadata includes Mythril Knife,
Potion and Phoenix Down; the review fixture deliberately removes the Potion
holder, so its two-item picture is not a universal battle outcome.

Items reuse `ffmg` icons and FCJ5 names. Four trophy rows fit; arrows scroll a
longer list. ENTER/2nd advances three pages; ESC closes the results. Save credit
is guarded by the existing story-win flag, so reloading/replaying this story
receipt does not pay twice. This is not a random-encounter reward system.
Stock including equipped copies is capped at99; excess trophies pay the source
quarter price. Gil is capped at99,999,999. Ordinary KO party members return;
actual crystal/chest losses follow the existing party rules.

At map aftermath entry a visibility bitmask hides enemy bodies/crystals/chests
without destroying their tactical/reward state. Living allies face the scene;
unrepresented allies use fixed neutral poses and KO allies retain KO poses.
Scripted plot actors keep existing choreography. Orbonne retains the monastery
abduction staging. This is adapted choreography, not an assertion of all PS1
camera movements. It does not establish a fix for the separate B06/B07 ghosts.

## Formats and storage

- `FSV3`: 3,138 bytes, `SaveUnit` still176 bytes. The old 3,110-byte prefix is
  unchanged except for the magic. The 28-byte tail contains big-endian gil,
  flow phase, encounter ID and up to five roster/world-coordinate/facing records.
  Story flags remain at offset4. FSV2 loads preserve the entire old roster/bag/
  flags, default the newly introduced balance to2,000 and do not reconstruct
  historical earnings. Phase1/2/3/4 means Academy/deployment/title/Gariland done.
- `ffflow`, FQF1: 13,126 bytes; seven big-endian u16 section offsets for source
  roster, deployment, rewards, compact title strips, prices, dialogue and script.
- `ffs1`: 4,116 bytes; repeated Academy pictures are deduplicated. `ffs2`:
  35 bytes; black replaces the previous3,107-byte world crop. Serif strips adapt the already
  reviewed typeface; no full runtime font or new source sprite rip.
- `fftdep` has a separate packed core `ffpccore`. Complete new bundle size,
  TTPack margin, combined live heap and changed frame costs are not yet measured.

## Validation and review

`make test` passes campaign Status, KO/save/stats, abilities, management, map,
scene/dialogue and deployment checks. All dialogue words survive37 fitted pages
across35 beats, including the formerly separate Academy narration.

`python3 check_flow.py --native` checks actual battle update/render and 68000
modules:74 Gariland checks (`8B4BB22B`, centred trophies) and14 Orbonne checks (`2CD70391`),
identical selected world tiles, Ramza52HP, stopped CT during dialogue, live loot,
KO staging, production victory/fade/result confirmation and saved bytes.
Separate native/PC runs also verify no duplicate payout and saturated-stock
quarter-price credit. These are controlled fixtures, not a full manual campaign.

`python3 check_flow_menus.py` passes10 native/PC input/pixel/state/save replays:
five deployment, three Academy and two Gariland-title cases. `make pc` builds
the supervisor entries. The bounded battle fixtures pass ASan/UBSan with legacy
fixed-point shifts and leak accounting excluded.

The shared-layout regression run also passes29 campaign replays plus12 opening
battle and6 original title/chapel checks. The latter keep their historical screen
references. The five active PC entries execute headless; every supervisor resume
checkpoint is checked and the isolated native supervisor compiles. Repeated
opening captures now also skip byte-identical writes, preserving their dates.

`python3 review_flow.py` publishes actual-renderer sheets under `x/qa/flow/new`,
skipping byte-identical image writes. Before requesting review, mark these four
files with `ti-view --decision`: `flow-scene-order.png`,
`flow-deployment-review.png`, `flow-victory-runtime.png`, `flow-aftermath.png`.
The gallery serves them first with a Decision badge. Their markers do not imply
approval. Full packaging and any hardware launch wait for the visual verdict;
the user's running TiEmu and its keyboard remain untouched.

The follow-up uses `--out x/qa/flow/review-2` on the three flow scripts, and
`--before x/qa/flow/review-2/before` on `review_flow.py`. The source Academy
narration/title/dialogue screens stay identical; Gariland's black title and the
new deployment screens pass all10 updated PC/68000 UI/save replays. The original
review pictures are preserved rather than republished with newer dates.

`review_white_edges.py` is an isolated Gariland colour proposal using the exact
existing geometric edge mask. It turns an edge white on any nonwhite face,
retaining black on white faces. Four actual C-rendered before/after views are
published in `white-edges-review.png`; original depth/cover bytes and all pixels
outside the edge mask are unchanged. Generated banks are isolated under
`review-2/white-edges`; neither active chapter's scenery is replaced. White
edges increase from4,587 to7,618 across10,621 source edge pixels. The shared
colour helper supports this policy, with the reviewed adaptive default retained
until a verdict. Orbonne regeneration/native packaging remains after review.
