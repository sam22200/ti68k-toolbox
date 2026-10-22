# Management and dialogue prototypes — 2026-10-18

Branch: `explore/fft-management`. Separate portable C application in `management/`.
This visual/navigation milestone does not modify tactical or campaign saves. Job
changes update only this menu session. No TiEmu session was touched.

## Review artifacts

Generated locally from the actual C framebuffer, never committed:

- `x/management/capacity.gif`: small dialogue, the extracted103-character page,
  grey-to-white reveal and a short fitted reply.
- `x/management/dialogue-capacity.png`: ordinary, longest-page and short-box comparison.
- `x/management/dialogue.gif`, `dialogue-progression.png`: reveal checkpoints.
- `x/management/dialogue-themes.gif`, `dialogue-styles.png`: black/white inverse
  comparison. `theme-config.gif` shows the configurable preference and its reuse.
- `x/management/jobs.gif`, `job-change.png`: animated job previews, confirmation,
  session-local Squire/Chemist changes and locked Knight/Archer requirements.
- `x/management/small-grid.gif`, `small-characters.png`: normal versus smaller actors,
  three-row capacity and the CATALOG density setting.
- `x/management/dense-grids.png`, `dense-grids.gif`:4x3 and5x3 filled visual
  fixtures, explicitly labelled as repeated source records, including third-row navigation.
- `x/management/location.gif`: original Orbonne/Gariland lettering with a feathered
  reveal, hold and erase. `location-source.png` is the adapted source strip.
- `x/management/layouts.png`, `comparison.gif`: three original menu-layout controls.
- `x/management/menus.png`: context, Item, Equip, Ability, Learn, Set, jobs, Status, dialogue.
- `x/management/equipment-icons.png`: original16x16 icons beside8x8 conversions.
- `x/management/font-extracted.png`: original FFT glyph reference.
- `x/management/dialogue-audit.json`, `extraction.json`, `checked.json`: provenance,
  scope, character counts, source hashes, native screen matches and CPU measurements.

GIFs are **PC visual previews**,32 updates/s, sampled every four updates. Export cadence
is not measured TI FPS. Pillow assembles C-rendered frames; labels sit outside the LCD.

## Original dialogue capacity

`tools/management_research.py` reads `EVENT/TEST.EVT` from the local USA disc. It
walks valid8192-byte event blocks using the retail instruction-size table at
`0x8014d170` in BATTLE, collects DisplayMessage/ChangeDialog references and resolves
one-based text IDs. It expands the original byte-counted F0..F3 backreferences
(distance radix254), glyph pages and control operands before counting glyphs/spaces.
Unused strings and padding are not scanned as arbitrary candidate text.

The audit covers500 archive blocks, **2230 referenced entries and3573 pages**, with
zero text-decoder errors. Portrait and automatic-portrait boxes use **three source
lines**, as forced by `battle_text_character_handling_thread`. Newlines after every
three lines and explicit F4 page breaks are counted separately from characters.
The largest measured portrait page is **103 characters including spaces**:
event425, text ID10, page0, a navigation-setting explanation. The original hyphen
is glyph237 (D0 ED), checked on the extracted FONT bitmap; it becomes an ASCII
hyphen in the fixture. Its three original lines are retained intact.

This is a **static referenced-event corpus measurement**, including tutorials;
it is not a universal runtime upper bound. Ramza substitutions use the default
five-letter name; other dynamic values are marked. WORLD help, alternate names and
all runtime branches have not been exhaustively exercised in the PS1 game.
`battle_text_build_display_message` also verifies that FFT itself measures text width
and can reserve the widest later ChangeDialog width for a shared window.

The TI prototype fits that entire103-character page without an extra confirm/page.
Dialogue uses the small proportional5-row ink font. Layout measures the same stored
advances as drawing, wraps at words, preserves explicit source newlines and computes
both box width and actual used height. There are no reserved empty text rows. The
portrait sits above the box so it does not force unused space underneath the text.
The short fixture produces a narrow one-line box. The speaker badge uses the actual proportional name advances plus eight pixels
of horizontal framing, rather than a fixed45-pixel width. Delita uses28 pixels and
Help23. A12-row badge retains the added upper padding and removes the extra bottom
row; the fitted box uses the same geometry in both themes. Up to six wrapped rows fit; longer
sample text can paginate. This is not yet a complete event-script player/importer.

## Menus, jobs and smaller characters

Three retained layouts: A classic grid, B portrait/roster, C detailed unit card.
CATALOG opens the prototype configuration: **Small characters**, Dialogue preview,
Location introduction and **White dialogue** (unchecked/black by default). The first row cycles **normal /4x3 /5x3** with a changing
checkbox label. The unchecked default keeps16x26 actors. Compact modes use the same
offline10x16 ink in a padded16x16 sprite: **three rows**, either four or five columns.
Names sit below the actors; the selected unit's full name, job, Lv, HP and MP remain
below the grid. Long names may clip in5-column cells;4 columns preserve more label
space. A21-byte8x7 version of the existing hand is generated offline, with no
runtime scaling or extra archive bank. All five real party members retain their
idle animation and white visibility outline; empty slots remain empty.

Dense review scenarios48/49 repeat the five existing records to fill12/15 slots.
They are explicitly marked Formation test: these are not newly recruited campaign
units. All three rows are navigable. Up/down preserves the column and skips empty
cells in the ordinary five-unit fixture. Changing to another layout leaves the dense
fixture. Its indices map through the five original records, including status/job
previews, so the test cannot index outside the real source arrays. The static-layer
cache key reserves four unit bits to avoid aliases on selections8..14.
The card strip also uses the smaller scale; the text-only roster keeps its layout.
No runtime scaling, cache eviction or duplicate mirrored sprites are needed.

Change Job now has four readable cards instead of the old read-only list. Original
Squire, Chemist, Knight and Archer sprites animate. Left/right changes the selection;
confirm opens a yes/cancel panel. A second confirm applies Squire/Chemist **only to
this menu session**, updates the job/actor, equipment display and derived statistics.
Switching back restores the original snapshot. Nothing is written to the battle.
Ramza keeps his named portrait.

The five original Gariland player records supply stats, equipment and abilities.
Their unlock bytes are `c0 00 00` (Squire/Chemist only), and their packed first job
levels are `11` (both level1). Knight/Archer remain unavailable. The original EXE
requirement rows at`0x800660c4` both require Squire2; extraction asserts those bytes.
Alternative-job values use the already-checked `battle.stats` model with the original
raw stats. Equipment disallowed by the new job is omitted from the preview; this is
not a persistent inventory transfer or a complete strongest-equipment implementation.
The PS1 wheel, full job tree, growth, job JP progression and unlock updates remain future work.

Header numbers were two pixels above their labels. HP/MP/Lv and Move/Jump now use the
same y baselines as their labels. The roster/card readouts were corrected as well.
No text or window drop shadows are generated, at either font size.

Item/Ability/Set/Learn retain the original hierarchy. All five equipment slots fit.
The200-JP Learn balance is illustrative; equipment, learning, dismissal and ordering
remain read-only. Original costs: Squire Accumulate300, Dash80, Throw Stone90,
Heal150; Chemist Potion30, Antidote70, Eye Drop80, Echo Grass120, Phoenix Down90.

## Original location lettering

`attack_map_load_title_graphic`, `attack_map_show_title` and
`attack_gfx_build_shaded_panel_strips` establish the original technique: a bitmap
strip, a left-to-right growing textured area, a shaded leading edge, a hold and a
reverse erase. It is not a live handwriting/font renderer.

`EVENT/MAPTITLE.BIN` contains120 strips, each256x20,4bpp,2560 bytes. The palette and
upload rectangle come from `EVENT/ATTACK.OUT` loaded at`0x801bf000`: palette address
`0x801d5ff0`, rectangle address`0x801d5fc0` (64 VRAM words by20 rows).
The prototype extracts index55 **Orbonne Monastery** and21 **Magic City Gariland**,
crops empty margins and fits each into a144x14 strip, preserving the source contours.
No system font substitutes for those titles and no offset shadow is added.

Runtime drawing uses eighteen8-pixel masked chunks, a white revealed area, a
light/dark-grey sixteen-pixel frontier, then an erase. F1/F5 chooses the other place;
ENTER replays. This adapts the original reveal to four greys and32 Hz; it does not
claim PS1 timing or simulate individual pen strokes. Only the two narrow strips are
stored, **1512 bytes total**, no fullscreen title images.

## Font, icons, storage and RAM

Original `EVENT/FONT.BIN`:77000 bytes,2200 glyphs,35 bytes each,10x14 packed2bpp.
It matches WORLD offset`0x5b8f8` in full; proportional widths are at`0x733e0`.
The curved font remains a research reference. The user-selected clean UI shapes are
AMS F_4x6/F_6x8 bitmaps from the local OS:5-row menu/dialogue ink, optional8-row menu
comfort control. All95 printable ASCII glyphs use exact ink plus inverse mask,
without shadow, scaling, decompression or a RAM glyph cache.

| Bank | Bytes | Use |
| --- | ---: | --- |
| fftmfont |2668 | Two font sizes and advances |
| fftmicon |152 | Five original8x8 equipment icons and empty slot |
| fftmvis |7040 | Header8, small idle2400, job idle3120, location strips1512 |
| fftu (reused) |57200 | Existing actors and poses |
| ffti (reused) |16284 | Existing single-facing portraits and interface |

The compact idle bank omits Delita's unused small variant. The new bank is read in
place from archive; no complete source sheet or title atlas is copied to RAM.
Native program size and current exact CPU results are recorded in `checked.json`.
This isolated executable fits the ordinary TI-89 program-size limit; it does not
solve the full tactical executable's near64-KB packaging limit.

RAM: **7680-byte heap cache,24-byte native MenuState,19-byte dialogue layout**, a few
pointers/keys, small drawing temporaries and the ordinary runtime planes. Stack high
water and actual AMS free heap were not measured. ESC frees the cache safely.
Static layers rebuild only on content changes; actors, pointer and title remain live.
The dialogue background and dim upcoming text are cached, with only the white prefix
and two leading light-grey characters recoloured. The dark-grey/checker/black gradient
comes from the user's FFA reference, without its old text shadow.

The White dialogue checkbox stores the existing `dialog_light` flag for the current
menu session. Both body and speaker badge use an exact four-grey palette inverse:
white/black exchange, dark/light grey exchange, including gradient and bevel. Future
text is light grey, the advancing pair dark grey, and completed text black. Name
padding, wrapping, page capacity, original actor/portrait colours and shadow-free ink
are unchanged. DIAMOND during a dialogue remains a shortcut to the same preference.
Leaving/reopening the dialogue or configuration retains it until the app exits.
Only cached window bytes are inverted at a content rebuild, with byte masks on the
first/last columns; no extra image bank, RAM allocation or moving-frame inversion is
added. Unit checks assert exact `light == 3 - dark` body/badge pixels, reversal and
configuration persistence, including partial/complete native replay checkpoints.

Equipment icons come from `EVENT/ITEM.BIN`: packed4bpp pixels then16 BGR555 palettes;
the original item palette/sprite IDs select the crops. All1280 source pixels of the
five16x16 crops plus512 palette bytes match original deployment VRAM exactly via
`management_reference.py DISC --icons-only`. Offline conversion produces8x8 icons.
The whole atlas is not asserted equal, as its map-name region can be overwritten.

## Build, controls and checks

```sh
make -C games/fft management-review  # unit tests, PC build, PNGs/GIFs
make -C games/fft management-check   # also native68000 checks/cycles
make -C games/fft management-ti      # isolated management/fftmenu.89z
cd games/fft/management
./fftmenu_pc
```

ENTER/2nd opens/selects; ALPHA/DIAMOND opens the unit context menu. SHIFT backs out;
ESC exits at Formation. F1/F5 cycles units; APPS cycles Formation layouts. MODE changes
menu font size. CATALOG opens configuration; select White dialogue and press ENTER
to invert the theme. In Dialogue, ENTER completes text then advances/
closes, F1/F5 cycles normal/103-character/short fixtures, DIAMOND switches dark/light box.
Scripted input files use `B` for SHIFT (the runtime parser has no `SHIFT` token).

Scenario IDs0..29 retain layout tens0..2 and screen units0..9;38 is light dialogue.
New doors:40 configuration,41 title,42 small grid,43 longest dialogue,44 Chemist
selected,45 confirmation,46 locked Knight,47 short fitted dialogue,48 dense4x3 test roster,49 dense5x3 test roster.
A later requested calculator run needs archived `fftmfont.89y`, `fftmicon.89y`,
`fftmvis.89y`, `fftu.89y`, `ffti.89y`, staged under`x/management/data/`.
**No emulator launch during this review.**

Navigation, wrap/back, confirmation/cancel, unavailable jobs, reversible local changes,
small-grid configuration/navigation, shadow-free menu text, original icons, the
103-character single-page fixture, short-box margins, dialogue progression, title
replay/place selection and safe cleanup are unit-tested. Native checks compare actual
PC/68000 framebuffer checksums, including partial reveals and complete input tours.
The runtime's CATALOG/APPS alias is also recognized by `ti-cycles`.

**78 PC/native framebuffer checkpoints pass**, covering4916 updates across
three448-update tours, both font controls, job confirmation/change/cancel, small-grid
navigation, title reveal/replay and partial/complete source-dialogue reveals. The
current isolated executable is **18,637 bytes** (container).

MC68000 datasheet cycles include estimated AMS calls; they are CPU measurements,
not TiEmu/hardware timings or measured FPS. Cache-reusing frames peak at **270,258
cycles (22.52 CPU ms at12 MHz)**, including confirmation-driven text recolouring.
Dense4x3/5x3 cached animation peaks at118122/137782 cycles; title replay
peaks below228k cycles. Complete window rebuilds reach **657,250
cycles (54.77 ms)**: content changes can exceed the ordinary360k frame budget.
This is a standalone UI milestone, not evidence that all screens fit the tactical
game's combined frame budget. Native screen parity does not validate LCD timing,
keyboard or archive hardware paths; those await the requested emulator review.

## Campaign program (2026-10-22)

The user chose layout A without animation. `campaign/manage.c` (`fftmng`) is the real
program built from this prototype: it reads and writes the campaign save, with every job,
item and name from the disc (`tools/management_data.py`); see `campaign/README.md` and
`campaign/ENGINE.md`. This prototype stays as the visual laboratory.

## Next

Select the layout, implement persistent party/inventory with conservation, equipment,
learning/JP/unlock legality and saves, then connect battle results. Import complete
story events separately. Package native modules before integrating into the tactical
binary, and validate the selected hardware UI in TiEmu when requested.
