# Perspective visibility: PC visual review

This revision is PC-only, awaiting visual approval. No TI binary was built or tested:
no ti-cycles, xcheck, hardware or TiEmu run. Earlier TI measurements describe earlier
strategies and cannot establish the cost of this revision.

## Controls and reproduction

From `games/fft`, run `make fixed-review`. This builds the portable renderer and its
tests, then captures four cameras in all three styles, 32 frames per combination.
In the PC game, V (the ALPHA action) cycles Auto -> Solid -> Dotted. MODE (PC: M)
changes close/wide framing; F5 selects the opposite view.

Local review outputs:

- `x/fixed-review/solid-auto.gif`: full scene beside automatic selection; close/front,
  wide/front and wide/back rows correspond to the annotated examples.
- `x/fixed-review/three-modes.gif`: Auto, Solid and Dotted side by side in the same views.
- Matching PNG montages and `manifest-pc.json` with source hashes and frame checksums.

GIF playback is 10 FPS for inspection, not a measured rendering rate.

## Selection

Auto first draws opaque geometry, recording the winning component and reciprocal depth
at every pixel. It probes three small screen regions: Ramza's inner body, the central
view within roughly one tile ahead of him, and the foreground floor beside his shoulder.
At least three samples from the same architectural component must be nearer than the
protected subject/region. Terrain, water and sky never select a building.

Only selected components lose their fills; they retain black two-on/two-off architectural
edges. Other buildings stay opaque. Bounding-box overlap and camera proximity alone no
longer remove whole structures. Dotted edge fragments closer than 24 world units are
clipped locally (the ordinary near plane remains 16), avoiding extreme foreground lines.
This is a conservative presentation heuristic, independent of game LOS and collision.

The three requested cases are regression checks:

- Close/front: the left house remains filled.
- Wide/front: the foreground structure becomes dotted; the left house remains filled.
- Wide/back: the right-hand structure remains filled.

Solid keeps all architectural fills. Dotted omits all architectural fills and retains
their edges; terrain and water remain drawn. The latter is an optional comparison and
can become visually busy.

## Cost and verification

Auto redraws geometry once if a selected component must become dotted. This work occurs
only when constructing a cached background. Solid and Dotted need a single geometry
pass. Cold construction may therefore cost more; TI timing and executable size remain
unmeasured for this revision.

The temporary 16,000-byte ownership map reuses the existing pose-pool region in the view
scratch buffer. Selection flags and counters also reuse existing scratch; no additional
heap allocation or asset-bank change is introduced. The tests exercise cache rebuilds
after sprite storage has reused that memory.

Portable tests cover all three styles, synthetic visible ownership/depth, the actual
Gariland components in the annotated views, local edge clipping, opaque edge occlusion,
camera/cache/water behavior, tactical-state conservation and return to Move. Both test
suites also pass AddressSanitizer and UndefinedBehaviorSanitizer (with the existing
isometric alignment exclusion).

All 384 captured PC frames are checksummed. In close/front and wide/back, Auto world
pixels exactly match Solid for all 32 frames, excluding the style HUD; wide/front differs
as expected. The montages were visually inspected. Calculator validation must wait for
an explicit user request after review of these visuals.
