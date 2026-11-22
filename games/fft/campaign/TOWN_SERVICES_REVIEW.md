# Town services: source portraits and Bar proposals

2026-11-22. The user asks to continue Chapter I readiness through town
services, finally use the correct portraits, reuse the shared menus/design
system and offer several concrete choices for new screens.

**Visual decision pending.** This delivery prepares real PC-rendered proposals;
it does not integrate a Bar route, change the installed portraits, migrate a
save or certify the remaining services. Do not mistake the isolated candidate
`x/town-services-review/ffsk.bin` for the shipping bank.
Work branch: `feat/fft-town-services-review`; keep it open for the verdict.

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

The gallery's two decision-marked sheets are:

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

## After the user's decision

1. Record the exact approved pictures/hashes, clear their gallery markers,
   and integrate the corrected portrait reader into `shop_scene.py` without
   regenerating approved rooms. Retain one shared grey conversion; avoid a
   circular dependency between the two asset modules during that change.
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
