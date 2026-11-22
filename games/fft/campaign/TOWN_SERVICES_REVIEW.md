# Town services: source portraits and Bar proposals

2026-11-22. The user asks to continue Chapter I readiness through town
services, finally use the correct portraits, reuse the shared menus/design
system and offer several concrete choices for new screens.

**Approved and applied2026-11-22: C / D / C / D**, in order Shopkeeper,
Guildmaster, Fur Merchant, Bar owner. The actual FSK4 extractor and active
`data/ffsk.bin/.89y` now carry the first three selected faces; the fourth is
selected in the shared reader and Bar prototype. The Bar still has no shipping
route or persistent rumor machinery. No save/ABI change or installed package
replacement is included. The historical review banks/results remain intact.

The user also requests the original game's page indicator policy. Source
inspection finds graphical continuation/scroll windows, **no numeric page
counter**. Both Bar prototypes now reuse the campaign's exact3x2 blinking cue,
factored into `dialogue_box.inc::dlg_continue`. No new symbol or style is drawn.
The campaign call preserves its readiness, position, colour and blink phase;
the Bar uses the same ready-to-advance cue on the final page too.

Current pictures: `x/town-services-accepted/selected-portraits.png`,
`bar-continuation.png` and `bar-continuation.gif`. The old decision markers
are cleared. The precise earlier A/B reader-layout letter is not inferred from
the contrast choice; both layouts implement this correction.

`x/town-portrait-contrast/approval.json` records the exact selected sheet hash
and supersedes the historical capture's pending field. Acceptance verifies all
24 original review records remain exact after factoring and all four selected
screens reproduce their approved pictures. Production ffsk is8368 bytes, the
same size and directory layout; all three room payloads are byte-identical.
Thirty-eight PC/ASan/UBSan screens agree. The two blink phases differ only at
the expected3x2 pixels, and both first/final pages return exactly after32
frames. The actual map service tests pass on PC/sanitizers, and the shared
campaign dialogue retains every word across37 pages/35 beats.147 existing
bank/save/review files retain bytes and mtimes; copies of the intentionally
replaced active ffsk pair preserve the preceding bank. No TI run was needed:
no archive layout/loader/arithmetic/hardware path changed.

Reproduce acceptance in its new private directory:

```sh
rtk proxy tools/pyenv/bin/python games/fft/campaign/accept_town_portraits.py
```

The original Bar source evidence is in
`wldcore_text_init_scrollable_window`, `wldcore_text_render_scrollable_window_page`,
`wldcore_text_update_scroll_indicators` and `wldcore_text_is_window_finished`.
Their source hashes and the scoped proof are recorded in the acceptance
directory. This is source inspection, not a new full PS1 gameplay replay.

## Five portrait contrast treatments (user follow-up2026-11-22)

`review_town_portraits.py` prepares all four accepted speakers in five new ways,
with the preceding contrast retained as column0:

| Choice | Treatment |
| --- | --- |
| A | Natural: absolute source luminance, the existing7/16/22 thresholds. |
| B | Balanced: normalized palette, gamma1.35 and uniform four-grey quantization. |
| C | Deeper midtones: gamma1.85. |
| D | Crisp: gamma1.35 and1.5x contrast around the midtone. |
| E | Ordered2x2 dither in midtones, gamma1.35; flat shadows/highlights. |

The sheet shows each shared-renderer portrait at6x nearest and its actual
160x100 screen at1x. Each speaker also has a separate `contrast-1..4.png`
strip. Identity,17x27 sampling, outline, transparency, framing and scene remain
unchanged; users can choose a common treatment or one per speaker.

```sh
rtk proxy tools/pyenv/bin/python games/fft/campaign/review_town_portraits.py
```

The PC-only `--portraits` door in `preview_town_services.c` uses the same
`town_render`/Bar and shared dialogue renderer as before. Twenty-four screens
(four controls plus twenty candidates) agree on PC and ASan/UBSan. Controls
are exact record copies; all masks are identical, and pixel differences stay
inside the portrait. Complete Save unchanged;113 existing bank/save/preceding
review files retain their bytes and mtimes. The original review evidence is
untouched. No shipping, native, save-format or runtime change is included.
Proofs and exact treatment definitions are in the new directory's `results.json`.

## Source findings

The user's `Fft_shop_protrait.png`, `Fft_warriors_portrait.png`,
`fft_fur_portrait.png` and `Fft_bar.png` identify these actual WLDFACE indices:

| Speaker | Correct index | Current shipping index |
| --- | ---: | ---: |
| Shopkeeper | 114 | 8, a different story character |
| Guildmaster | 115 | 12, a different story character |
| Fur Merchant | 116 | 9, a different story character |
| Bar owner | 117 | No shipping Bar yet |

**OBSERVED IN SOURCE:** `world_gfx_load_wldface_to_frame_buffer` uploads
`EVENT/WLDFACE.BIN` as four0x8000-byte pages. The rectangles at WORLD
0x80189C4C describe four256x256 4bpp pages. The texture routine
`world_gfx_set_portrait_poly_texture` uses forty32x48 faces per page,8x5,
with ordinary CLUTs starting at page row240, four16-colour CLUTs per row.
Faces63..82 instead use the separate block at row250 and palette bits10..11.
The old continuous168-face/palette-zero interpretation is incorrect.

`tools/town_service_assets.py` follows that layout and retains the existing
TI conversion:17x27 face inside the even-aligned32x29/348-byte portrait,
the existing grey mapping and white outline. The candidate overwrites only
the three existing portrait spans in a private FSK4 copy; all room bytes and
offsets stay unchanged. There is no reason to rerip approved room artwork.

**OBSERVED IN SOURCE AND DISC:** the Bar has a stored background, contrary to
the previous SHOP_SCENE note. `wldcore_bar_push_menu_level` selects background
set0,22 or23. `wldcore_gfx_load_background_set` reads the directory at
WLDCORE0x8009E260, relative to LBA0x12995, the start of `WORLD/WLDBK.BIN`.
Set0 is sixty2048-byte sectors: a256x240 raw BGR555 image with the owner,
Ramza, customers and bar already drawn. No synthesized room is needed.

**TARGET, pending approval:** crop(50,65),160x100 at1:1,15th/97th luminance
percentiles and gamma1.2. This keeps the owner and Ramza visible beside the
left command window without moving actors. This is a readability adaptation,
not the original game's crop or a newly measured performance result.

## Concrete choices

The initial review sheets were:

- `x/town-services-review/portraits-before-after.png`: the actual existing
  Shop, Office and Fur screens, before/after,4x nearest at the same framing.
- `x/town-services-review/bar-options.png`: the common Bar entry, then two
  rumor presentations at that same scale. A places the list on the left and
  reads in a wide command-style panel; the sample fits one page. B centers
  the list and reads in the shared black dialogue frame; the same sample
  takes two pages and leaves more scenery visible.

`bar-hand.gif` shows the shared animated glove. `speakers-source.png` shows
the four original RGB faces. `bar-original.png` is the unadapted source room.

The entry shows **Rumor / Leave bar** for Chapter I. This follows the source
menu builder, which adds Propositions only when its enabling script variable
is set. No later-chapter jobs are inserted into the preview.

The prose is the actual first `WORLD/WLDMES.BIN` entry, flattened for native
font wrapping without dropping its text. The three short topic labels are
descriptive review fixtures, **not certified original title IDs or a tested
Gariland eligibility list**. Source title/progress extraction is still needed.
The photographs are presentation samples; L/R and SHIFT are proposed controls,
not an implemented playable Bar input route.

## Reuse and validation

`preview_town_services.c` includes the actual `worldmap.c`; its existing town
renderer produces the three portrait comparisons. The Bar uses `says`,
`dialogue_box.inc`, `choice_menu.inc`, `war_funds.inc` and the existing
`item_detail_line` font-based wrapping. There is no alternative menu, cursor,
portrait frame or font. New command windows use CHOICE_TAB, fitted label
widths, equal top/bottom spacing and CHOICE_HAND_X/Y.

Reproduce from the repository root:

```sh
rtk proxy tools/pyenv/bin/python games/fft/campaign/review_town_services.py
```

Normal PC and ASan/UBSan runs agree on their rendered PNG hashes. The C
fixture asserts that the complete Save is unchanged. A private data directory
contains linked banks and a copied fixture; no campaign session is written.
The script verifies78 existing bank/save files retain their SHA256 and mtime,
and proves the three room records remain byte-identical. Source/output hashes,
scope and pending approval are in `results.json`; `preserved.json` records the
protected inputs. No TI binary or emulator run is needed for these isolated
PC proposals. Existing performance figures are not re-certified here.

## Remaining town work

1. The portrait selection/reader integration and counter correction above are
   done. At the next release, rebuild the core package to include the corrected
   ffsk and shared cue; preserve old installation groups and matching keys.
2. Bind the approved Bar layout into `worldmap.c` using the same cached town
   page and shared components. Reuse the borrowed archive pointer; retain the
   moving hand and invalidate the page only when displayed state changes.
3. Source-check the64 rumor location masks, original title/body IDs and story
   variables0x2D1+i; map the original town-kind mask to campaign locations.
   The address0x80096A4C is a pointer to the masks at0x800970AC, not the first
   mask word itself. Never interpret that pointer directory as eligibility
   bits. Keep passive reading separate from any actual authored quest effects.
4. Add a real town entry/list/detail/back flow, long-text pagination and
   held-input/reentry checks on PC with sanitizers. Define persistent rumor
   effects before changing FSV7. Then continue Propositions and separate Fur
   stock/migration/transactions, preserving chapter gates and the existing
   one-shot poach roll. Run only selected native checks for changed archive
   binding/layout/render/arithmetic/ABI paths, after the visual decision.

The adult monster placement/battle/results loop is already delivered; do not
reopen the old type3 veto. Campaign extraction remains after these side-system
gates, as the user requested. The installed16KiB free/peak target is still
uncertified; this preview does not change its status.
