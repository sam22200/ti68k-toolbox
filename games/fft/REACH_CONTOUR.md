# Exterior-only movement reach: native comparison

2026-10-19. Orbonne and campaign Gariland now expose the contour in CATALOG:
unchecked **Move: full tiles** (default), checked **Move: contour**. ENTER/2nd or
left/right toggles it; CATALOG, ESC or SHIFT closes settings. The battle, CT,
RNG and actor animation pause while the menu is open. APPS retains the turn list.
The choice lasts for the current battle, including Move, rotations and AT-list;
a new battle resets to full tiles. Attack and ability ranges keep filled tiles.
No perspective cache or renderer is enabled by this chapter setting.

## Method

Compute four-neighbour membership boundaries from the unchanged movement reach.
Skip cells with no exposed side, then draw only the exposed edges of the other
cells. Unreachable holes and disconnected jump landing regions retain their
boundaries. This is an exterior contour, not a row of filled boundary diamonds.
The selected cursor remains visible; attack/ability ranges retain their existing
complete tiles. Gameplay, occupancy, movement costs and target selection do not
change.

The renderer reuses the existing black/white cursor, including sloped shapes,
depth tests and hidden dashes. `tools/contour_assets.py` partitions that artwork
into four edge-selection masks offline. No new character or scenery art.
World-neighbour bits are rotated/reflected using the source camera convention;
an independent test projects neighbouring world positions to check each edge.

## Results

Same direct Ramza position, four orientations per map, with Move opened on
frame120 after the camera/cache have settled. Each variant executes the actual
68000 binary and a matching PC replay. Native/PC pixels and state match in all
24 views. Original and optimized filled variants have identical pictures;
all three variants have identical tactical state and exact reach-cost bytes.

**MC68000 datasheet CPU time at nominal12MHz**, excluding grayscale, frame waits,
real AMS overhead and packed startup. These are creation-event costs, not FPS.
Means across the four views:

| Map | Original full frame | Optimized full frame | Contour frame | Contour gain vs optimized |
|---|---:|---:|---:|---:|
| Orbonne |114.69ms|105.71ms|86.62ms|18.1%|
| Gariland |134.40ms|123.15ms|87.67ms|28.8%|

| Map | Optimized reach-drawing stage | Contour stage | Stage reduction |
|---|---:|---:|---:|
| Orbonne |78.69ms|59.54ms|24.3%|
| Gariland |98.36ms|62.83ms|36.1%|

The previous grouped-patch optimization therefore also helps Orbonne:6.9–9.4%
on the complete entry frame, depending on camera. The selected contour control saves another18.1% on average there, versus28.8%
on Gariland (including its current setting and archived constants).

Warm rendering is effectively unchanged: the selected controls differ by about
300–600 cycles/update, below0.05ms at12MHz. This does not accelerate every frame by18–29%.
Orbonne has35 reachable cells,28 boundary cells and54 exposed edges. Gariland
has44 reachable cells,30 boundary cells and50 exposed edges. Orbonne's narrow,
fragmented area contains relatively few interior cells to eliminate.

## Native packaging follow-up

The setting adds four aligned State bytes (Orbonne1,086 ->1,090), with Work
unchanged at35,540 after the previous cache optimization. Mask data remains
3,408 bytes. Another1,640 bytes hold existing own render tables and UI strings,
which move out of the nearly-full native executable and are read from archive.
The combined5,048-byte tail plus10-byte footer and one alignment byte are appended
to each chapter's first compressed view. No ninth file slot or new heap block.
Equal numeric tables are deduplicated offline. PC uses the same original tables;
native/PC pixels check their archived equivalents, including menus and turns.

The footer stores a version signature, original stream length, aligned offset
and payload length. The loader accounts for the native six-byte OTH trailer.
Asset preparation is idempotent and precedes compilation; mixing an old header
with a newly generated tail is invalid. Old banks without this footer require
regeneration; they are never used to dereference relocated tables.

The current instrumented contour fixtures are98 bytes smaller than the filled
control on Orbonne and450 smaller on Gariland, because they also relocate those
existing constants. This is not the isolated code size of the checkbox.

The first implementation exceeded both AMS's65,514-byte executable payload
ceiling and TTPack's64,936-byte input limit. Archiving immutable constants fixes
packaging without deleting tactical features. The new bundle remains headless
validated; no new AMS heap/TiEmu lifetime claim.

## Historical prototype memory and readability

- 3,408 bytes of own mask data, plus one alignment byte in these fixtures,
  appended to the already loaded first view bank. No ninth file: the PC runtime
  has eight file-cache slots and Orbonne already uses all eight.
- No additional heap allocation or Work growth over optimized filled rendering;
  one native pointer and slightly larger temporary marker records.
- Instrumented native container growth versus optimized filled:362 bytes Orbonne,
  404 bytes Gariland. These benchmark executables are not a new packed campaign
  or a stock-HW2 launch validation.
- The scenery remains visible inside the area. On a detailed monochrome map,
  the boundary is less conspicuous than white filled tiles, especially where
  terrain occludes it. Keep the full version as the default during review.

## Reproduction and artifacts

```sh
make -C games/fft contour-bench
```

Requires the existing local opening/campaign banks and Git history at
`5cbffb5` (the pre-cache source control; override `--baseline-ref` if needed).
The script compiles native variants sequentially and removes its temporary C
wrappers. It creates private bank fixtures; it does not rewrite production art.
The first compressed view stream ends normally before the appended masks and render tables.
Align that tail to an even address before native word/long reads.

Local outputs in `campaign/x/contour/`:

- `benchmark.json`: all24 CPU/size/count/pixel results.
- `comparison.png`: native160x100 crops, filled versus contour, both maps.
- Per-map/variant/view PC and native PNGs, keys and cycle logs.

The current focused tests pass320 complete-scene/cache comparisons across four
views in both active maps, including styles, holes, changed costs and interrupted
removal. They independently check the neighbour-to-screen edge mapping.
No TiEmu keys were sent and the user's live session was not restarted.

## Location-title correction

The original `MAPTITLE.BIN` Orbonne lettering was present, but title skipping
accepted ENTER on the first update. The old presentation replay also pressed
ENTER at frame0, omitting the title entirely. The title now finishes its sweep
and holds until update110 before accepting a skip; automatic continuation stays
at update150. This protects against a launch key carried into the game.

`make -C games/fft/opening check` now includes launch-key and complete-title
native/PC checkpoints (16 total). A launch press produces the same title pixels
as no input. For a standalone PC presentation GIF, not a speed benchmark:

```sh
tools/pyenv/bin/python games/fft/opening/review.py --title-only
```

Output: `opening/x/review/orbonne-title.gif` (50 sampled frames).
