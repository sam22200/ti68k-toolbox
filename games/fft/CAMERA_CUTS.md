# Accepted camera policy: direct fixed inspection

User decision, 2026-10-16: keep close and elevated wide inspection, remove perspective
entry/exit/zoom animation and automatic perspective attack shots. Fixed views retain
animated actors, the paused tactical state, front/back selection and obstruction styles.
Direct cuts avoid intermediate camera renders; they do not eliminate cold preparation.

This is the default policy of the fixed exploration source on **both PC and TI**:

- DIAMOND opens the remembered close/wide preset; MODE + DIAMOND opens wide directly.
- MODE switches close/wide directly; SHIFT/ESC/DIAMOND returns directly to ISO.
- F5 selects the opposite inspection view; F1/ENTER restores the front view.
- Attacks remain isometric. The old `[7]` cinematic toggle has no effect.
- Historical smooth/cinematic experiments require explicit `-DFFT_CAMERA_MOTION`.
  Their audit/review scripts opt in so earlier measurements remain reproducible.

**F1/F5 is a separate policy.** The user clarified that isometric rotation must remain
animated. The current native fixed exploration path uses three low-poly intermediate
angles, then the textured destination. Close/wide perspective entry, zoom and return
remain direct. See [native low-poly rotation](ROTATION_LOWPOLY.md). Textured moving
F1/F5 filtering belongs to the PC reference. A later opt-in
[native affine mapper](NATIVE_TEXTURED_TURNS.md) implements quantized nearest-sampled
textures at 80x50; the faster low-poly policy remains the default.

## Checks and native demonstration

```sh
make camera-cuts-check
make camera-cuts-demo
```

The fixed-view tests check one-update entry/exit/zoom, remembered presets, all four
facings and both front/back views, exact cache reconstruction, animated water and
tactical/RNG conservation. A separate real-input Attack test confirms that `[7]` cannot
enable cinematic shots and that the isometric attack produces exactly the same units,
rewards and RNG as ordinary combat resolution. The historical 12 cinematic cases also
pass in an explicitly enabled research build.

`tools/camera_cuts.py` now creates `x/camera-cuts-poly/fftcut.89z` and a headless companion.
Its automatic sequence is F5 twice, F1 twice, direct wide, direct close, direct ISO.
All **512 per-frame PC/TI screen checksums** match. Units and battle RNG remain unchanged;
no perspective transition/cinematic state is entered. Native executable: **57,991 bytes**.
The archived assets and executable hashes are recorded in `checked.json`.

The demonstration uses the existing compact ISO presentation and two offline-textured
inspection endpoints at the unchanged review position. It is not the complete tactical
release, nor general runtime texturing at arbitrary unit positions. A full fixed-view
cuts-only build without cinematic support failed the single-variable packaging limit:
67,708 bytes versus 65,514, before the wrapper. Packaging remains a separate blocker.

## Actual TiEmu GIF, 2026-10-16

**Historical hold/switch capture, superseded by the animated low-poly version.** The
old files below are preserved as evidence of the rejected behavior. New animation
captures are linked from [ROTATION_LOWPOLY.md](ROTATION_LOWPOLY.md).

Recorded on an isolated Docker TiEmu 3.04 Titanium HW3 / AMS 3.10 profile configured at
12 MHz, with the ordinary grayscale driver, GrayDBuf swaps and 32 FPS limiter active.
The ten banks were sent on a clean boot; the automatic native input script then ran.

- `x/camera-cuts/f1-f5-tiemu.gif`: F5 twice then F1 twice, **10.26 s**.
- `x/camera-cuts/direct-views-tiemu.gif`: the same switches plus direct wide/close/ISO,
  **21.33 s**.
- The `-4x.gif` files show exactly the same frames enlarged with nearest sampling.
- `full-run-tiemu.gif` retains the entire 55-second recording; `capture.json` records
  binary/evidence hashes and trim boundaries. Only launch and idle tails were trimmed;
  there are no interior cuts, speed changes or inserted holds.

The capture samples the LCD at 15 Hz; this is not a measured game frame rate. The GIF
shows the real hold/switch implementation, not the PC's textured turning animation.
Physical TI hardware was not used. The earlier timer audit measured native F1/F5 at
about 0.4 s and cold inspection construction at about 1.6 s for this fixture; those
remain distinct from the absence of perspective animation in the accepted source.

## Why the PC preview misled us

The error was treating a visual preview as evidence of native responsiveness. There
were three different workloads and two different renderer implementations:

1. **The PC GIF had an imposed cadence.** Scripts wrote actual C output, then assigned
   every frame 50 ms (20 FPS). This made geometry/motion review convenient, but did not
   measure the C render time or simulate a 12 MHz 68000. The 55-frame PC cinematic shown
   over 2.75 s took about 36 s in the native TiEmu implementation. Export FPS is metadata.
2. **Warm animation was confused with cold or moving cameras.** At a fixed camera, cached
   scenery and scaled poses allow 28.4/32 FPS. Cold close/wide preparation costs about
   1.6 s. Each moving frame invalidated depth, obstruction masks and pose caches and
   repeated that work: an 18-frame entry took about 28.5 s. Averaging warmed frames hid
   the latency of pressing the actual control. Those figures describe different paths.
3. **The PC mapper was not the TI implementation.** The PC review has a general filtered
   UV sampler using doubles and rejects TI compilation. Native travelling geometry was
   already flat 80x50. Only reviewed fixed endpoints received offline textures. The PC
   had textured intermediate F1/F5 frames; the native exploration path only held/switched.
   Framebuffer parity of a bounded native fixture cannot validate an absent TI feature.

The slow native work included triangle rasterization with pixel depth, occlusion/mask
construction and scaled sprite preparation. Halving resolution did not eliminate
geometry, clipping or rebuilding costs. A 46-face proxy reduced entry to about 5 s,
but retained the detailed endpoint's 1.6 s preparation. Disabling textures during travel
could not solve the problem because native travel textures were already absent.

The machine difference matters too: desktop floating-point/CPU throughput is not a
68000 budget. However, no desktop-to-TI speed ratio can repair a comparison between
different code paths. TiEmu additionally undercounts MOVEM and multibit shifts; it can
be optimistic relative to physical hardware. That timing defect was **not** the source
of the PC GIF's chosen 20 FPS cadence.

## Rules for subsequent work

- Label every GIF as **PC visual preview**, **datasheet-cycle paced estimate**, or
  **actual TiEmu capture**. Report the renderer, resolution and texture provenance.
  A chosen playback cadence is never measured FPS.
- Before promising a new animated interaction, build its native path and measure one
  cold frame plus several consecutive moving frames with `ti-cycles`. Run the complete
  real-input sequence, including endpoint preparation and return; track worst-frame
  time and total elapsed time separately from warm animation FPS.
- Reuse one scene, one camera/input driver and matching framebuffer/state checks for
  PC and native binaries. List all code/asset substitutions and missing features.
  Matching output proves equivalence of the checked path, not performance of another.
- Record cache invalidations and distinguish offline preparation, archive/decode,
  geometry/depth/masks, pose generation, warm composition and presentation. Report
  cold and warm costs together.
- Check native code size, archive banks and working RAM before expanding visual polish.
  A compact review build is useful evidence, but is never the full-game executable.
- After headless checks, record one real-time TiEmu interaction with ordinary grayscale
  and frame limiting. Keep binary hashes and timer/cycle evidence; never accelerate its
  interior frames to suggest responsiveness. Physical hardware remains a separate test.
- If complete interaction cost exceeds the accepted latency, change the design before
  adding more preview polish. In this case the accepted fallback is direct inspection.

Earlier measurements and detailed feature matrix: [CAMERA_AUDIT.md](CAMERA_AUDIT.md).
The rejected proxy experiment is preserved in [ULTRA_TRANSITION.md](ULTRA_TRANSITION.md).
