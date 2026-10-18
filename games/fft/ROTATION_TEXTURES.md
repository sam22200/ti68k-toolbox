# Textured F1/F5 turns: original offline feasibility study

A later [native affine-textured turn port](NATIVE_TEXTURED_TURNS.md) now implements
runtime texture sampling on TI as an opt-in experiment (`make turn-texture-bench`).
The historical PC study below is a different filtered renderer and is not its timing evidence.

The current `make attack-camera` PC build now renders these turns directly in C using
`texture_preview.inc`, alongside all perspective transitions. Current captures are in
`x/textured-transitions/`; see `ATTACK_CAMERA.md`. The study below is retained as the earlier
offline comparison. Neither version establishes a TI texture-mapping speed.

`make rotation-texture-review` generates `x/rotation-textures/f1-f5-textured.gif` and
`review.png` from the local FFT disc. It demonstrates the original ground, building and
roof artwork at intermediate isometric angles. Water keeps the approved grey treatment;
dry scenery uses the existing contrast-plus conversion. Playback is 20 FPS for review,
not a TI speed measurement.

The review driver sends actual F5/F1 inputs to `game_update`: four turns in each direction,
160 frames total, including 24 intermediate views (three samples per quarter-turn).
It uses the original MAP022 mesh and the current TI unit sprites. Integer C projection
and the existing span rasterizer produce winning polygon IDs and depth-tested actors.
The turn pivots on the cursor, retaining its screen position. Python then samples the
original UVs with the same 4x4 filtering as the perspective review. No isometric screenshot
is warped. Each of the 866 mesh faces is matched against the source mesh before sampling.

Checks: the original unit records, battle RNG and tactical mode remain unchanged; the
camera returns to its starting orientation; texturing preserves actor pixels and sky.
Between 11,225 and 12,270 scenery pixels receive textures in each intermediate image.
The C diagnostic uses approximate 8-bit affine depth, so this is not an exact reproduction
of FFT's original renderer. The actual settled views retain their existing art and UI.

This earlier tool is an **offline PC review**. Its C-rendered successor is now enabled
in `fftcam_pc`; neither is included in the TI game. The normal game has no new rotation bitmap cache, source texture allocation,
or runtime cost from this tool. No TiEmu test was performed.

The promising TI implementation is affine UV mapping: during an orthographic isometric
turn, planar-face UVs can advance by fixed additions across spans. Perspective-correct
per-pixel division is unnecessary. A compact quantized texture bank, an 80x50 transition
buffer, clipped faces and original outlined actors are the first implementation to measure.
An 80x50 two-plane image needs 1,000 bytes versus 4,000 bytes for packed visible 160x100
pixels (the current native padded plane layout occupies 6,000 bytes).

Filtered Python sampling in this tool does not establish the speed of that proposed C
mapper. The texture bank, UV records, sampling aliasing and per-span/per-pixel TI cost
remain to be built and benchmarked. The existing slow perspective geometry rebuild must
not be mistaken for evidence that this separate orthographic mapper is fast. Large sets
of full-screen room/angle images are not the proposed production representation.
