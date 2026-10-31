# Dynamic lettering, optional edge widths and perspective size study

2026-10-23. Engineering proposal, not a new integrated campaign build or a
visual verdict. Existing title A/B and scenery-width decisions remain open.
No emulator, live save or shipping asset was modified for this study.

**Subsequent decision:** retain both Gariland edge widths in the temporary
in-game test build; perspective stays outside the campaign under experiments.
[EDGE_WIDTH.md](EDGE_WIDTH.md) records the implemented controls, package/native
checks and exact scope. The estimates below remain the original study.

## Shared dynamic lettering

The current title's11.4KB includes its program and title/file art. Each title
is not10KB. The ten campaign chapter/location/result strips together consume
9600 raw bytes,960 per160x16 three-plane line. Title B adds two such strips
before compression. Existing battle objective/victory banners use another
4128-byte set of strips.

Recommended design: one archived font bank, strings plus style/layout metrics,
and shared C composition/reveal code. Generate glyphs on the host through the
existing SVG conversion pipeline or direct font rasterization. The calculator
only composes glyphs and advances the existing left-to-right grey/white reveal.
Compose once when the text changes, not on every frame. A160x16 three-plane
line cache needs960 bytes; simultaneous lines require an explicitly bounded
cache or reuse of inactive UI scratch. Preserve glyph bearings, spacing and
the reviewed white/shadow appearance; review before integration.

Measured storage layouts, **uncompressed data only, renderer code excluded**:

| Candidate | Bytes | Scope |
|---|---:|---|
| Cropped glyph bitmaps,9px and12px | 4495 | 48 characters used by current titles, digits and GIL; six-byte glyph records,48-byte shared character index and16-byte header |
| Same layout, full printable ASCII | 8554 | 95 characters, two sizes; no accented extension |
| Normalized SVG contours | 2910 | Same48-character repertoire, one outline set; character index/header and arbitrary-height rasterizer/cache still needed |
| Text strings | 164 | Ten flow titles plus FINAL FANTASY TACTICS and68k, NUL terminators |

The bitmap numbers count three row-packed planes per cropped glyph, including
a one-pixel shadow. These are layout counts, not a validated runtime font bank.
Two fixed sizes alter the existing mixed8..15px title sizes and need a visual
comparison. Outline quantization also needs review for lowercase bearings and
descenders. No new font picture, timing or final compressed-package saving is
claimed. Extra glyphs should extend the shared bank rather than turn phrases
back into separate image assets.

The existing converter is now shared at `lib/svgfont.py`, with its defaults
preserved by `tools/banner.py`. Its696-byte old alphabet and4128-byte banner
output compare byte-for-byte with the before snapshot. This shares the host
converter; it does not claim that a general SVG library or the new dynamic
runtime renderer already exists. Sharing C source between chapter modules
does not itself share executable code: count the small renderer in each owner,
but keep font data in one archive variable used by title, scenes and battle.

## Configurable1px /2px scenery outlines

On the current four Gariland chapter views:

| Additional representation | Bytes |
|---|---:|
| Duplicate complete second banks | 45576 |
| Keep only second compressed colour streams | 24891 |
| Compressed XOR of both colour planes plus directory | 9851 |
| Compressed one-bit changed-pixel mask plus directory | 6140 |

The last representation reproduces the candidate2px image exactly. At marked
pixels both colour bits become the complement of the old dark bit; elsewhere
the1px image is retained. All four views round-trip and match the existing2px
candidate; depth streams remain identical. Applying this one-way mask requires
the original1px decoded view. Toggle/reload from the bank rather than apply it
twice. The earlier two-plane XOR candidate is reversible directly.

Decode the8080-byte selected mask into existing cold-load view scratch before
that scratch receives the depth planes, then patch the decoded scene colours.
This is a proposed scratch lifetime, not an implemented native decode path.
Ordinary cached frames need no contour pass. Switching width or view has extra
decode/apply work; native latency and safety are not yet measured. Budget about
6–7KB archive for Gariland plus the small option/application code, not another
complete set of map banks. Orbonne and future maps require their own masks.

Expose an explicit scenery-width preference in the shared settings layout.
The user requested MODE access; reconcile it with the current MODE highlight
inspection and CATALOG settings input rather than silently dropping a binding.
Persist both representations only after the chosen visual/configuration review.

## Close/wide perspective

Both zooms share one renderer. Current Gariland prototype bank sizes:

- `fftmesh`:16585 bytes, shared geometry/tables.
- `fftlook`:3902 bytes, compressed close and wide background fixtures together.
- Total extra prototype data:20487 bytes before variable overhead. Existing
  actors, weapons and ordinary ISO banks are excluded from this addition.

A controlled GCC4TI `-Os` object comparison of current `fft.c` uses the same
`RT_CYCLES` and native low-poly turn flag in both cases. Adding
`FFT_EXPLORATION`, `FFT_FIXED`, `FFT_CINEMA` and `FFT_CONFIG` changes emitted
`.text` from58792 to74464 bytes: **15672 bytes additional code before linking
and compression**. The configuration includes optional idle preparation and
cinema support; final shared functions/inlining/linker removal can change it.
Neither object is presented as a runnable campaign program. This is not a
comparison between the compact prototype and the full tactical executable.

An order-of-magnitude integration allowance is therefore **about36KB raw
code+data for the existing Gariland feature set**, roughly35–40KB. Final
archive growth after packing/module boundaries is unmeasured. This includes
both zooms, not that amount for each. It does not include an Orbonne mesh or
new general native texture assets. Existing texture fixtures retain their
camera/position restrictions; they do not supply arbitrary-map texturing.

RAM is a separate constraint: fixed scratch reaches52320 bytes, versus the
chapter allocation30720, a21600-byte gap if transplanted directly. Its optional
cooperative cache costs another20284 bytes. The almost-full tactical core
cannot absorb this by enabling a define: plan a bounded module/overlay, restore
chapter live-unit/mask correctness and test native memory before integration.

## Evidence

Ignored audit outputs live in `x/cost-study/`:
`font-cost.json`, `edge-cost.json`, `native-code-cost.json`,
`banner-before.json`, `banner-shared-check.json`, and the two native objects.
The font costs are data-layout counts. The edge representations were actually
compressed, decoded and compared to their target colour bytes. The code sizes
are native object section sums, not source line counts or final flash sizes.
No FPS or calculator timing conclusion follows from this size study.
