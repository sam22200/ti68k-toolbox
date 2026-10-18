# Textured grey canal, smooth camera modes and scenery comparison

Historical animation experiment. The user subsequently rejected this motion after
native timing review. [Direct close/wide inspection](CAMERA_CUTS.md) is now the default;
the scripts below explicitly enable `FFT_CAMERA_MOTION` to reproduce the old experiment.

## Water and reviewed contrast

The user preferred the original textured canal to the flat replacement, and requested a
compromise. Water keeps its original texture but clamps its opaque levels to light/dark
grey; pure white is reserved for the existing animated glints. Original surface-water IDs
(rather than low terrain height) identify those faces. Dry scenery keeps contrast-plus;
water is excluded from that contrast conversion. Checks prove that changes from the flat
grey replacement stay within water pixels in all four cameras/32 poses. Sky, cutaway gaps,
architectural edges and character artwork remain unchanged.

`make fixed-modes-review` writes `x/perspective-modes/water-three-versions.gif`: original
bright texture, flat grey and the new middle version. Optional previous captures are
required for that comparison. Updated native views remain in `x/scenery-contrast/`.

## Real entry/exit on PC

DIAMOND starts a 17-frame entry at the remembered distance. MODE + DIAMOND from the tactical
view directly selects wide inspection. SHIFT, ESC or DIAMOND starts the reverse exit from
either distance. MODE during inspection animates close -> wide or wide -> close, preserving
the front/back direction. All movements have 17 geometry steps.
This is implemented in the portable fixed-camera engine, not a crossfade between pictures.
The camera yaw, pitch, anchor and orthographic-to-perspective projection blend use a
17-entry smoothstep table. Zoom keeps a perspective projection while interpolating actual
camera distance, eye height and sine/cosine of pitch; entry/exit interpolate from/to the
isometric projection. Geometry is flat shaded at 80x50, then doubled to 160x100.
Actor poses remain animated and scaled, with masks tested against expanded scenery depth.
The texture cache appears when the reviewed final perspective is reached; isometric art
returns after the reverse animation. Mid-animation inspection controls and tactical updates
are frozen. Direct scenario 62 still opens the final camera immediately for comparisons.

Transition planes reuse 1,000 bytes inside existing HUD/background scratch, alongside the
temporary 2,000-byte water mask. Projection buffers, actor masks and pose storage are reused.
No heap allocation was added. Camera reconstruction invalidates the ordinary pose/background
cache at each transition step. Full geometry, scaled poses and masks are rebuilt during
these steps; calculator frame times and code size have not been measured.

Intermediate geometry uses two-sided depth resolution and omits architectural edges.
Automatic obstruction selection is restored in the settled view. This reduces drawing
complexity during the movement but can produce scenery changes at the final cutaway.
Quantized angles and half-resolution silhouettes are also visible during the animation.

Outputs from the same command:

- `x/perspective-motion/entry-exit.gif`: native 160x100, actual DIAMOND/SHIFT key replay.
- `entry-exit-large.gif`: the same frames enlarged four times with nearest sampling.
- `steps.png`, `frames.csv`, `manifest-pc.json`: intermediate views, state and provenance.

The latest `fixed-modes-review` writes `x/perspective-modes/all-transitions.gif` (direct
close/wide entry side by side), `close-entry.gif`, `wide-entry.gif` and per-case CSVs.
Its 544 C key-replay frames include two entries, two exits and two zooms in each case,
correct remembered endpoints, and unchanged units, active turn, mode, AI and RNG. Tests
check zoom steps in four facings/both directions, endpoint cache reconstruction, bounded
completion, remembered wide/close entry, water and return to Move. Address/undefined
sanitizers pass, retaining the existing isometric alignment exclusion. The older
three-camera exploration tests pass. GIFs use an illustrative 20 FPS, not measured TI speed.

## Flat/textured scenery comparison, characters in both

The user clarified that "without sprites" meant flat scenery rather than textured ground,
walls and roofs; character sprites must remain in both cases. `make fixed-scenery-bench-prepare`
prepares eight cases in `x/scenery-benchmark/`: four textured cameras (0..3), then the same
flat cameras (4..7). It writes the reviewed contrast-plus backgrounds and 128 PC checksums
per case. Every first 32-frame sequence matches its corresponding reviewed flat/textured
capture exactly. Camera, geometry, depth, pose drawing, glints and HUD use the same code.
The shared C harness is `tools/fixed_scenery_bench.c`. The old make target remains an alias
for compatibility; it no longer disables actors.

This is cached background replay, not real-time texture mapping: mapping remains offline.
Warm frames perform the same copy and animated actor composition regardless of background
values, so there is no extra texture sampling stage per warm frame. Loading a textured
background adds a one-time copy in this harness; the actual cost of building textures on
the calculator is absent and cannot be inferred. No TI FPS result has been measured for
this iteration. A future report must separate cold mapping/construction, transitions and
warm replay, include uncapped milliseconds and the 32 FPS limiter, and avoid presenting
GIF cadence or PC checksums as calculator performance.

The user subsequently explicitly requested TiEmu FPS tests. The dedicated timer benchmark
and matching TI cycle binary are reproduced by `make fixed-scenery-ti-benchmark`; this
supersedes the earlier PC-only restriction for the requested measurements. The visual
mapping above still occurs offline, and the benchmark reports cached replay separately.
