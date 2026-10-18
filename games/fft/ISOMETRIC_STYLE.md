# Isometric scenery: contrast and readable height boundaries

The selected style is **contrast + height contours**. The PC exploration keeps two
reversible colour-only variants of the existing four ISO views: perspective-style contrast, and that contrast plus adaptive height contours.
The original art remains available. Character frames and their white outlines are unchanged.

The reusable method is also recorded in the shared knowledge base:
[`ti68k-c-patterns.md` §3](../../.claude/skills/ti89-c-dev/reference/ti68k-c-patterns.md),
"Offline scenery contrast and height contours". It includes material exceptions, edge
visibility, cache consistency and limits for moving cameras; measured costs are indexed
in the shared performance and game-technique references.

```sh
make isometric-style-review   # three styles, identical actual-game inputs and actors
make attack-camera           # current PC preview; default = contrast + height edges
./fftcam_pc --scenario 61
```

In the ordinary ISO menu/browse view, `[8]` cycles original -> contrast -> contrast + edges.
No tactical state is changed by this preview key. Review files: `x/isometric-style/`.

## Colour and geometry

`tools/isometric_style.py` reuses the original local MAP022 mesh, texture atlas, 4x4
supersampling, and the approved perspective contrast thresholds (range width 0.65).
It first reproduces each original colour plane byte-for-byte. Canal tones are retained
from the original ISO image; the existing C water glints still animate normally.

Contours come from mesh edges shared by an upward/sloping top face and a vertical wall.
There are 302 candidates. Coplanar texture seams and triangle diagonals do not qualify.
Each projected sample is tested against the original supersampled mesh depth before
adding a one-pixel stroke, so a roof or foreground wall can hide an edge behind it.

The stroke is black on underlying white/light grey, white on underlying dark grey/black.
It outlines actual ledges, stairs, canal banks and roof/platform lips rather than drawing
a grid on every tile or detecting noise in the textures. Adjacent edge pixels may switch
colour as the underlying material changes. The user selected this combined style.

The generated `fftiso.bin` contains two sets of four colour-only views. The PC loader
copies the selected colours into the original unpacked view while preserving its four
occlusion planes. Highlight restoration therefore uses the same art as the scene, and
existing characters, weapons, cursor, reach markers and water remain in the ordinary game
rendering path. The loader does no contour detection or drawing each frame.

The original assets are not overwritten. For a production asset build, the selected colour
planes can replace the corresponding original planes before compression: four views,
unchanged raw size and unchanged depth data, rather than shipping all three review styles.
The generator writes such six-plane candidates as `variant-N.raw` for review only; these
are now ZX0-packed and decode-checked under headless 68000. Full game integration remains
the PC exploration; no complete calculator release was deployed. The initial style trial
was PC-only. The later [bounded Titanium TiEmu demo](TIEMU_CAMERA_DEMO.md) uses the selected
packed ISO assets with a compact UI; it also passes 470 PC/TI frame comparisons. This does
not establish a hardware FPS result or port the general PC texture mapper to TI.


## Validation

### Occluded actor silhouettes on textured scenery

The stronger scenery made an existing hollow x-ray presentation ambiguous: covered
body interiors were transparent, so roof texture appeared inside the actor's contour.
Depth data and cover masks were unchanged. Covered body interiors now draw opaque
white, retaining the sprite's original black/white contour and all exposed body art.
Covered shadows remain transparent, avoiding a white shadow patch beyond the feet.
This uses the existing per-pose interior mask and per-unit cached cover; it adds no
asset bank, mask storage, framebuffer or edge-generation pass. The bounded native
configuration executable increases from62,271 to62,351 bytes (+80 B).

`make iso-silhouette-check` checks768 cases (four orientations,32 idle ticks, Ramza
and the adjacent enemy, three background tones) on PC and the real68000 binary under
`ti-cycles`:38,166 hidden body pixels must be white, exposed sprite art must match,
and covered shadow pixels must retain the background. The old transparent-interior
renderer fails the white-interior check. Both updated backends pass, and the
configuration replay retains60 matching PC/TI pictures. Comparison artifacts are
`x/occlusion/comparison.png` and `comparison-detail.png`. No fresh TiEmu run or
physical-calculator measurement is claimed for this pixel-composition change.

The old visible-area assertion counted every changed scenery pixel, which increases
when white silhouettes replace textured background. Its reveal-after-turn assertion
now measures actual mid-grey body detail instead; masked/exposed semantics remain
tested independently of background contrast.

The actual C game replays 144 frames in each style (432 total), with identical units,
RNG, camera and cursor logs. On every settled frame the four occlusion planes are compared
against a fresh decode of the original archived view. Four rotations return to the starting
orientation; the `[8]` preview toggle changes only the style. Original and high-contrast
water tones match before contour insertion. Changes between contrast and contour variants
are confined exactly to the generated geometric mask.

A sanitizer capture exercises the contour variant over the same 144 frames. It exposed
an existing host PNG writer passing a null pointer to a zero-length `fwrite` for IEND;
the writer now skips that empty payload. PNG contents remain identical. This affects only
the PC capture utility, not the TI runtime or rendering rules.

The ordinary game test suite and the twelve cinematic combat tests pass. The separate
portable headless TI diagnostic matches 560 PC frames, including the current 110 cinematic
frames; it does not include the PC ISO style loader or general texture sampler.


## Measured storage and loading cost

`make isometric-storage` packs the complete six-plane views with the project's ZX0 v2
packer and checks every round-trip. `make isometric-decode-cost` uses the existing verified
`lib/unpack68k.s` decoder under `ti-cycles`, checking all twelve output sizes/checksums.
The second command expects the packed files from the first. Results are local in
`x/isometric-storage/storage.json` and `decode.json`.

| Four-view set | Packed bytes | Mean load cycles/view | CPU-only ms at 12 MHz |
|---|---:|---:|---:|
| Original | 38,869 | 2,187,966.5 | 182.331 |
| Contrast | 39,108 | 2,168,873 | 180.739 |
| Contrast + contours | 41,505 | 2,189,089.5 | 182.424 |

Contours add **2,397 bytes** relative to contrast alone (6.13% for the four packed views),
or the selected combined style adds **2,636 bytes** relative to original (6.78%). Mean
loading changes by +20,216.5 cycles / **1.685 ms** versus contrast, +1,123 cycles / **0.094 ms**
versus original. The headless timings exclude display/interrupt waits and are not hardware
FPS. They measure loading, not drawing contours each frame.

For the production representation, selecting one style replaces the four existing image
streams. The raw six-plane total stays 209,280 bytes, the existing single-view RAM buffer
stays 52,320 bytes, occlusion data is identical, and there are **zero runtime contour draw
calls**. Thus there is no added render-buffer RAM or per-frame edge-generation work.
The 139,528-byte multi-style PC comparison bank is a review convenience, not the proposed
calculator storage/RAM overhead.

The current PC F1/F5 path now uses six intermediate angles rather than three, doubling its
moving-frame duration (0.30 rather than 0.15 seconds at the review's 20 FPS). The existing
portable/TI turn path is unchanged while the textured renderer is still a PC reference.
