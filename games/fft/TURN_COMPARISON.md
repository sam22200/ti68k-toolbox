# 80x25 textures versus native 160x100 low-poly turns

**Presentation correction:** the original comparison capture has misordered native
low-poly frames. See the direction audit below; use the numbered fixed-address
copy-control GIF in `x/turn-direction/` for the corrected visual comparison.

The user selected 80x25 as the preferred textured moving view and requested a GIF
against low-poly at 160x100. The previous low-poly renderer actually rasterized
80x50 and doubled it, so this comparison adds a genuine native-resolution variant
instead of relabelling the old footage.

`FFT_TURN_LOWPOLY_NATIVE` draws directly into the hidden 160x100 runtime planes.
Projection uses full-resolution coordinates and terrain corner heights, and painter
depth buckets retain their original world scale. It uses the existing word-span
polygon filler and native unit sprites. The expansion pass is skipped. Its edge
arrays grow from 58 to 108 rows; the work allocation is **43,546 B**, just **200 B**
more than the 80x25 textured variant's 43,346 B. No extra full-screen or depth buffer,
asset bank or new assembly is introduced.

Both rotate through 22.5, 45 and 67.5 degrees around the same cursor pivot and end
at the identical textured isometric view. They represent the same logical map using
different geometry: low-poly uses the tactical tile tops/sides and painter order;
the textured variant uses the detailed original mesh with merged surfaces and byte
depth. Actors are native-resolution in both, with terrain cover during motion only
in the textured variant. This is an architecture comparison, not a same-mesh
texture-off control. The [existing detailed-mesh flat control](NATIVE_TEXTURED_TURNS.md)
serves that other purpose.

## Reproduce

```sh
make turn-comparison-bench
# Once the native recordings and printed timer evidence are available:
../../tools/pyenv/bin/python tools/turn_compare_gif.py
```

Outputs are under `x/turn-comparison/`:

- `{lowpoly160,textured80x25}/fftturn.89z`: eight-case native timer/capture suite;
  send every archived bank in the adjacent `data/` on a clean boot and run `fftturn()`.
- `fftturnc.89z`: datasheet-cycle companion; `source.png` is the PC-rendered source
  title used for exact recording alignment.
- The `*-demo/` folders contain the continuous four-turn/direct-inspection demo.
- `comparison.png`: the same three intermediate angles and settled pictures.
- `textured80x25-vs-lowpoly160-tiemu.gif`: actual LCD recordings side by side.
- `comparison.json`, per-variant `headless.json` / `tiemu.json`, camera/sequence logs
  and `capture.json`: timings, checks and provenance.

Each variant passes **32 turn pictures, 576 broad camera pictures and 512 sequential
pictures** against its native TI binary. Settled pictures match between variants,
units/battle RNG stay unchanged and perspective travelling/attack cinematics remain
disabled. Ordinary game and camera-cut tests pass. These are bounded native review
executables, not a full-game stock-original-TI-89 release.

## Measurements

Eight complete F1/F5 turns from all four orientations, including destination decode
and final isometric rendering; setup and deliberate holds are outside the timer:

| Variant | Datasheet CPU mean/range at 12 MHz | Actual TiEmu internal-timer range |
|---|---:|---:|
| Textured 80x25 | 1.571 s / 1.532-1.620 s | 1.847-1.964 s |
| **Low-poly native 160x100** | **1.083 s / 1.048-1.113 s** | **1.273-1.355 s** |

TiEmu is Titanium HW3 / AMS 3.10 configured at 12 MHz, with grayscale, GrayDBuf and
the ordinary frame limiter active. The printed values use the emulated 256 Hz timer;
video wall-clock timestamps and GIF export rate are separate. No physical calculator
or smooth real-time FPS result is claimed.

Low-poly timer cases, total/worst frame in ms: `1332/523, 1332/523, 1355/558,
1320/523, 1273/500, 1332/535, 1320/535, 1296/523`. Every case passes.

The native low-poly timer binary is 59,039 B, SHA-256
`0d118df5989a266806e547f5fdff1e698bd98e767e17f1bf653bee71cb61d8be`.
The texture binary is 62,863 B, SHA-256
`0ed5c3dda1e6aead76bc7096a45525e69ad3e5caecb4769c2ba7f9bc955c3f43`.

## GIF presentation and provenance

The low-poly footage is a fresh isolated Titanium capture. The textured recording
is reused from the earlier native 80x25 test after verifying that the rebuilt binary,
all raw bank contents and all 32 PC/TI turn pictures match that run exactly.

The compositor aligns the first F5 source header using the actual PC/native font
pixels. Each side retains the recording's frame timestamps and speed; no intermediate
moving picture is removed or replaced with a PC rendering. Both recordings are
sampled at 20 Hz and enlarged fourfold with nearest-neighbor sampling. A common fixed
grayscale palette preserves the captured shades.

After reaching the target, the faster side is held while the other finishes; the
overlay explicitly says `TARGET HELD`. A shared 1.2-second ending hold makes the result
readable before looping. Those presentation holds do not contribute to the reported
rotation times. The full recordings contain independent benchmark case resets;
the composite uses only the first F5 case, not a continuous battle replay.

The first target header can appear while grayscale planes are still changing. The
compositor retains at least another 150 ms of recorded frames before freezing the
destination, avoiding a permanently frozen mixed-plane image. It retains the actual
transfer frames; this settling delay is separate from the internal turn timer.
Recording hashes, exact source/arrival/settling offsets and final GIF hash are in
`capture.json`.

## Direction audit

`make turn-direction-check` independently checks the actual low-poly projected grid
against host trigonometry and exact cardinal isometric coordinates. Across six pivots,
four source orientations, both keys and five angular positions, all **42,240 corner
projections** pass. Every consecutive signed angular sweep follows the requested
direction; its source and destination agree with `view_of`, rather than merely
checking the final orientation index. This checks calculated geometry, not the
ordering of images actually presented by the grayscale driver.

The diagnostic `x/turn-direction/direction-proof.png` includes extra low-poly source
and destination views, with one enemy marked throughout. These endpoint drawings
are diagnostic PC images, not additional frames in the native animation. The
low-poly tile columns differ substantially from the original roof/building mesh;
that representation switch makes visual continuity harder to read even though the
rotation direction is correct. However, checking the original TiEmu recording
against those reference pictures exposed an actual **2, 1, 3** presentation order
in the native low-poly run. A fresh capture of the unchanged binary reproduces it.
The previously supplied comparison GIF must not be treated as a correctly ordered
native animation. Presentation synchronization is under investigation; reversing
the angular formula would not fix this issue.

The extra-wait diagnostic (`FFT_TURN_ORDER_WAIT`) does not fix it: numbered native
frames also return to older pictures. A fixed-address control
(`FFT_TURN_ORDER_COPY`) leaves GrayDBuf's hidden allocation as a render buffer and
uses the existing verified ExtGraph `FastCopyScreen_R` to publish its two finished
planes into the standard grayscale planes, without toggling their addresses.
That control displays **1, 2, 3** in order in the native low-poly capture. It changes
presentation only, not projection, angles or world geometry. It is a diagnostic
alternative to flipping buffers; no global runtime change or physical-TI result
is claimed. The previous timer numbers must not be reassigned to this presenter.

Build controls with `tools/turn_order_tiemu.c`, the corresponding flags from
`tools/turn_comparison.py`, and `-DFFT_TURN_ORDER_LABELS`. Choose
`-DFFT_TURN_ORDER_WAIT` or `-DFFT_TURN_ORDER_COPY`; send the corresponding comparison
variant's archived `data/*.89y` banks on a clean boot. Capture the native LCD at
20 Hz. `tools/turn_order_review.py` compares the first F5 cases from
`x/turn-direction/copy-labelled.gif` (low-poly, `fftcopy.89z`) and
`copy-textured.gif` (textured, `ffttexcp.89z`), preserving recorded speeds and
checking **all three** intermediate pictures before composing the corrected GIF.
Number labels are outside the geometry-match area. `ordered-capture.json` records
the frame-order evidence and build/recording hashes.

Both fixed-address captures pass: the textured steps first appear at1.350,1.850,
2.400 s after the aligned settled source; low-poly at1.150,1.450,1.800 s. These are
recording timestamps including the deliberate source hold, not render timings.
The compositor trims350 ms of the initial source hold so grayscale inference has
settled, retains every moving picture at its recorded speed, and adds a shared
1.2-second ending hold. The reviewed3.9-second GIF is
`x/turn-direction/ordered-textured80x25-vs-lowpoly160-tiemu.gif`; both its first
and final pictures were inspected. Only this diagnostic presenter is changed;
the shared runtime's default buffer-flip implementation is unchanged.

`tools/turn_compare_gif.py` now also rejects the old recordings: low-poly fails as
`[2, 1, 3]`, and textured as `[1, 2]` (no matching third picture before arrival).
Matching PC/TI memory checksums alone did not catch the presentation failure.
TiEmu's `src/core/ti_hw/gscales.c` detects plane addresses and reconstructs grayscale
using a 16-entry address history and 48 plane snapshots; changed double-buffer
addresses pass through that inference. Its source is available in the
[Debian TiEmu source archive](https://deb.debian.org/debian/pool/main/t/tiemu/tiemu_3.04~git20110801-nogdb+dfsg1.orig.tar.gz).
This is relevant to the investigation, not proof of a corresponding physical
calculator failure. Keep the angle formula unchanged and test native presentation
order separately from framebuffer correctness.
