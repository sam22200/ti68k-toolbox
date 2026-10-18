# Coarse geometry for camera transitions

The user rejected this transition prototype after viewing its native timing and art.
The accepted default is [direct close/wide inspection](CAMERA_CUTS.md); this experiment
is retained only for research with explicit `FFT_CAMERA_MOTION` and `FFT_ULTRA_TRANSITION`.

This opt-in experiment uses `FFT_ULTRA_TRANSITION` with the native fixed/cinematic
renderer. It replaces only travelling frames; settled views retain the original
textured scenery, animated actors and combat presentation. It does not change the
default game build or implement textured travelling/F1/F5 rotation.

## Representation and costs

`tools/ultra_assets.py` merges connected coplanar same-material triangles into convex
patches and removes small patches. Gariland becomes 151 vertices / 46 convex faces,
instead of 479 vertices / 866 triangles. The archived `fftlod` bank is **1,670 bytes**,
read in place. Two short span arrays add **232 bytes** of persistent work RAM; transform,
projection and ordering scratch reuse the existing view buffer. Peak stack use has not
been measured. The timer executable grows from 57,511 to 60,489 bytes (+2,978 bytes).
These are bounded Titanium demo builds, not stock original-TI-89 release binaries.

The renderer caches transformed/projected vertices, rejects faces outside the frustum,
clips remaining convex faces, sorts far to near and fills word spans into an 80x50
two-plane buffer, expanded to 160x100. It avoids the detailed renderer's pixel-depth,
sprite-mask and scaled-pose preparation during travel. Actors are cheap outlined
rectangular silhouettes during movement, with their actual world positions/heights.

This is deliberately approximate transient art: convex hulls can close recesses/holes,
small stairs/trim disappear, painter ordering is imperfect, and silhouettes are drawn
over scenery without exact actor occlusion. Collision, LOS, targeting and tactical
geometry continue to use the existing game data. Detailed endpoints are unchanged.

## Headless comparison

Run from `games/fft`, after preparing `make tiemu-camera-demo`:

```sh
../../tools/pyenv/bin/python tools/ultra_transition_bench.py
```

The script builds both variants against the same archived assets and runs the same
13 cases. Each variant matches its PC oracle on **468 TI framebuffer checks**. Across
variants all 468 state/RNG hashes match; **343 settled framebuffers match exactly**.
Camera trajectories and frame counts are identical, including 18-frame entries/zooms
and 56-frame complete attacks. Only transient pictures differ.

Datasheet 68000 CPU totals at 12 MHz exclude interrupts, display swaps and waits:

| Operation | Detailed travel | Coarse travel | CPU speedup |
|---|---:|---:|---:|
| ISO to close | 22.292 s | 3.786 s | 5.89x |
| ISO to wide | 22.317 s | 3.957 s | 5.64x |
| Wide to close | 14.792 s | 3.377 s | 4.38x |
| Close to wide | 14.800 s | 3.394 s | 4.36x |
| Wide to ISO | 21.322 s | 2.964 s | 7.19x |
| Complete melee cinematic | 28.490 s | 6.709 s | 4.25x |
| Complete ranged cinematic | 28.718 s | 5.780 s | 4.97x |

Cold detailed preparation remains about 1.26/1.28 CPU seconds. Coarse travel does not
remove that final preparation cost. F1/F5 and prepared-view animation are unchanged.
Results and binary hashes: `x/ultra-transition/comparison.json` and each variant's
`headless.json`, `cycles.log` and `pc.csv`. `pc-frames/` contains actual C output,
not an artist's reconstruction.

## Emulator reproduction

Load `x/ultra-transition/ultra/fftaudit.89z` and every archived bank from
`x/ultra-transition/data/` on a clean Titanium profile. Wait for the complete transfer,
then run `fftaudit()`. The automatic suite retains grayscale, GrayDBuf swaps and the
32 FPS limiter; ENTER after the first table displays cinematic stage totals. The
baseline protocol and its earlier results are in [CAMERA_AUDIT.md](CAMERA_AUDIT.md).
Neither datasheet cycle counts nor TiEmu timer readings establish physical hardware
performance.

## Actual TiEmu comparison, 2026-10-16

Titanium HW3, AMS 3.10, configured 12 MHz; active grayscale, swaps and 32 FPS cap.
The printed 256 Hz timer digits were decoded against the local AMS font. Same cases,
camera paths and frame counts as the earlier detailed audit:

| Operation | Detailed travel | Coarse travel |
|---|---:|---:|
| ISO to close | 28.441 s | **4.886 s** |
| ISO to wide | 28.570 s | **5.074 s** |
| Wide to close | 18.796 s | **4.371 s** |
| Close to wide | 18.796 s | **4.394 s** |
| Wide to ISO | 27.257 s | **3.761 s** |
| Complete melee cinematic | 36.292 s | **8.472 s** |
| Complete ranged cinematic | 36.808 s | **7.382 s** |

Melee stages: 1.980 s entry, 4.230 s action including cold detailed preparation,
1.968 s return, 0.294 s final ISO response. Ranged: 2.144 / 2.800 / 2.144 s,
plus 0.294 s final ISO response. These totals preserve the existing action duration.
Cold close/wide construction remains **1.582/1.605 s**; prepared views remain
28.4/32.0 FPS. F1/F5 remain hold/decode/switch at 0.386/0.398 s.

Evidence is under `x/ultra-transition/ultra/`: `timer-results.png`, `stage-page.gif`,
`tiemu.json`, `tiemu.csv` and `audit-run.gif`. `cinema-tiemu.gif` is an excerpt of
that real-time capture, without playback acceleration. This proves a major reduction
in travel cost, but **does not meet a 0.5-second transition target**. Endpoint preparation
is still too expensive even if travel were free. The coarse path stays opt-in for
visual review; it does not replace the approved textured endpoints.
