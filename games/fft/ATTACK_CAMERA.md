# Target-centered attack camera and textured PC transitions

Historical cinematic experiment: the accepted default now keeps attacks isometric and
uses [direct close/wide inspection](CAMERA_CUTS.md). Motion/cinematic review scripts
explicitly opt in with `FFT_CAMERA_MOTION`; `[7]` cannot enable shots in the default build.

**Calculator status:** textured moving cameras/F1/F5 are a PC reference only. The native
TI demo uses flat moving geometry, with textures only on exact offline-prepared endpoints,
and its travelling frames are too slow for ordinary gameplay. See `CAMERA_AUDIT.md` for
the dedicated native timing cases and the PC/TI feature matrix. A PC GIF's smoothness is
not evidence that these effects have been ported to the calculator.

Current visual direction (2026-10-16): original ground/wall/roof textures throughout camera
motion, a close melee shot and a wider ranged shot. Camera movement retains the geometric
orthographic/perspective blend. The rejected white title-card transition and free tactical
play in perspective remain removed.

## Run and review

From `games/fft`, with the local FFT disc and extracted assets:

```sh
make attack-camera              # full PC game, general texture reference sampler
./fftcam_pc --scenario 61        # original deployment, direct Ramza turn
./fftcam_pc --scenario 65        # immediate melee review fixture
./fftcam_pc --scenario 66        # immediate Throw Stone review fixture
make attack-camera-review       # textured cinematic, inspection and F1/F5 GIFs
make isometric-style-review     # original / contrast / adaptive height edges
make textured-transitions-test  # full texture path, combat and rotation checks
make attack-camera-test         # portable camera/logic path
make attack-camera-check        # headless TI diagnostic WITHOUT the PC-only sampler
```

Shots are enabled in this exploration; `[7]` toggles them between actions. Selection,
movement and menus remain isometric. DIAMOND enters manual inspection; DIAMOND + MODE
enters the wide preset directly, MODE changes zoom, SHIFT/B returns. F1/F5 turn the main
isometric view. Manual perspective remains noninteractive tactically.

Scenario 65 places Ramza at `(5,5)`, beside the original enemy at `(6,5)`. Scenario 66
places him at `(4,6)` and grants Throw Stone. These are injected review fixtures, not
original opening turns. The cinematic scripts use the original menus and target validation.

Current GIFs are in `x/textured-transitions/`:

- `melee-textured-motion.gif`, `ranged-textured-motion.gif`: complete textured shots.
- `close-entry-all-transitions.gif`, `wide-entry-all-transitions.gif`: entry from ISO,
  zoom both ways, return, then F5/F1.
- `f1-f5-textured-live.gif`: the C-rendered isometric turn path.
- `review.png`, `manifest-pc.json`, per-case source PNGs and CSV logs.

GIF playback is **20 FPS for PC review**, not calculator cadence. Each cinematic GIF
contains eight context frames before/after its 55-frame shot (3.55 seconds total).

## Pacing and combat

Entry and return now use ten geometric samples at ease-table indices
`0,2,4,5,7,9,11,12,14,16`. Each lasts
0.5 rather than 0.35 seconds at the preview cadence. Attack/damage acceleration stays:
two consecutive original `attack_step()` updates per displayed action frame, stopping
immediately on exit from `M_ACT`. Every logical impact, reward, knockback, KO and CT/end
update still executes. Invalid targets do not trigger a shot; commands are ignored until
return. Support/healing actions remain isometric. The hook also covers eligible AI attacks.

A rewarded shot is **10 entry + 35 action + 10 return = 55 display frames**, or **2.75 seconds**
at 20 FPS, compared with the preceding 2.45 seconds and the earlier 5.2-second version.
Damage/Miss remains visible for twelve displayed frames above the target. EXP/JP now
appear in a compact outlined notice above the projected attacker, clamped to the viewport;
target recoil or knockback cannot move that reward notice. Values and both combat RNGs are
unchanged. Ordinary ISO combat and the manual inspection timing retain their earlier pace.

The target tile and elevation anchor the shot even during knockback. The camera preserves
ISO azimuth when actors separate well, otherwise turns toward the nearer lateral view by
at most 45 degrees. The ISO yaw is `56 - 16*rot`; its earlier opposite sign caused a
half-turn mismatch at orientations 1/3. Melee uses distance 48 / eye 40 / pitch 5; the ranged
fixture uses 96 / 84 / 6. Camera position, rotation and full ISO UI return afterwards.

## General PC texture reference

`FFT_TEXTURE_PREVIEW` enables `texture_preview.inc` in the current PC executable. It reads
`ffttex.bin`: original MAP022 4-bpp atlas, face UVs, palettes converted to luminance, and
existing contrast thresholds. The bank is **138,532 bytes** (20-byte header, 6,928 UV bytes,
512 palette bytes, 131,072 atlas bytes), generated locally by `tools/preview_textures.py`.
It contains no camera screenshots or per-room rotation images.

The existing C rasterizer supplies the actual winning face ID and depth at each pixel.
The reference sampler traces this face to its original UVs with 4x4 filtering, contrast-plus
on dry scenery and the approved grey water. Native edges and sprite masks retain their
existing depth. Perspective entry/exit, close/wide zoom changes, settled views and F1/F5
turns all use the same atlas. Textures no longer depend on two exact camera endpoints in
this PC build. The old endpoint bank is bypassed when the general sampler is available.

This is a **PC reference implementation**, using double-precision arithmetic for filtered
UV sampling. A compile-time error prevents inclusion in a TI build. It demonstrates the
requested appearance in the actual portable game, rather than recoloring a GIF afterwards.
It is not a fast 68000 texture mapper. The TI-oriented geometry remains fixed point; its
optional original `fftshot.bin` still textures only two exact settled cameras. A compact
fixed-point/affine mapper and its TI costs remain separate work before release.

## Checks and limits

The texture review checks **754 real game frames**: 346 cinematic/ISO frames and 408 manual
inspection/rotation frames. Every moving or settled perspective/turn has sampled texture
pixels. Both review attacks apply the expected single HP loss (12/6) and return to ISO.
Manual entry in both presets, both zoom directions, exit and F1/F5 preserve units, tactical
mode and both RNGs. Native reviewed melee pixels match the previous filtered endpoint;
ranged differs at twelve pixels from luminance quantization.

The texture-enabled shot suite passes twelve differential attacks: melee/ranged from all
four ISO orientations, miss, forced knockback, KO and victory. It also performs four F5
turns followed by four F1 turns through all orientations. Tactical state, RNGs, input lock,
first-sample ISO/mesh alignment and bounded sprite buffers are checked. ASan/UBSan pass.

The separate portable diagnostic (without general textures) passes **560 PC/TI framebuffer
matches**, including 110 cinematic frames. It is 45,821 bytes and omits the ISO UI. Current
headless datasheet counts:

| Stage | Melee | Ranged |
|---|---:|---:|
| Entry mean cycles/sample | 14,887,470 | 15,749,217 |
| First native action frame | 16,872,154 | 15,892,338 |
| Remaining action mean | 700,816 | 291,175 |
| Return mean cycles/sample | 14,882,096 | 15,765,382 |
| Entire portable cinematic | 338,395,554 | 340,938,262 |

These measurements validate the fixed-point path and its timing change, **not the PC-only
texture sampler**. Rebuilding geometric travelling remains too expensive for smooth TI
playback. Neither the 20 FPS GIF nor the 2.75-second preview establishes hardware speed.
The previous 104-frame measurements remain in `x/attack-camera/history/direct-104/`;
the rejected iris/title-card measurements are in `history/iris/`.

No TiEmu or physical calculator run was made for this visual revision. The last complete
TI package attempt, before these revisions, exceeded a variable (73,108 vs 65,514 bytes).
Packaging and geometry/texture performance remain before a complete calculator release.


## Isometric contrast and relief

The current PC build also loads the reversible ISO scenery variants described in
`ISOMETRIC_STYLE.md`. `[8]` cycles original / contrast / contrast + height contours.
Only view colours change; character sprites and occlusion data remain original. The
default is the contour variant. This adds no change to the perspective sampler or the
portable TI diagnostic, which continue to use their separate rendering paths.


The texture-enabled PC F1/F5 path now renders six intermediate angles per quarter-turn
instead of three (twice the moving-frame duration). The inspection replay asserts six
frames in both directions. Fresh review names are `f1-f5-slower.gif`,
`melee-attacker-rewards.gif` and `ranged-attacker-rewards.gif` in `x/textured-transitions/`.
ISO style storage/decode measurements are recorded in `ISOMETRIC_STYLE.md`.
