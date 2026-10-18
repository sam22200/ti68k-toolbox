# Native calculator camera audit

Historical motion benchmark: its scripts explicitly define `FFT_CAMERA_MOTION` and
`FFT_TURN_HOLD` to reproduce the original unanimated F1/F5 behavior.
The accepted default is [direct inspection without attack shots](CAMERA_CUTS.md).
F1/F5 now has a separate [native low-poly animation](ROTATION_LOWPOLY.md).

The textured 20 FPS PC review is a visual target, not an implemented TI animation path.
The native calculator demo is visibly slow. This audit measures the existing TI path;
it does not add a texture mapper or silently substitute pre-rendered travelling frames.

The later opt-in [coarse transition experiment](ULTRA_TRANSITION.md) measures a
46-face proxy against this baseline, preserving the same camera trajectory and game
state. The baseline results below remain the original detailed renderer's timings.

## Feature status

| Operation | PC texture reference | Native TI exploration |
|---|---|---|
| F1/F5 | Six textured intermediate angles | Hold old view, decode and switch; no intermediate rotation |
| ISO to close/wide | Textured geometric movement | Flat 80x50 geometric movement, then fixed-view preparation |
| Close/wide zoom | Textured geometric movement | Flat 80x50 geometric movement, then fixed-view preparation |
| Fixed inspection | General texture sampler | Only exact offline-textured review endpoints in the bounded demo |
| Attack cinematic | Textured movement and action | Flat moving geometry; exact prepared action cameras textured |

`texture_preview.inc` uses double-precision filtered UV sampling and deliberately rejects
TI compilation. No native general texture mapper for travelling roofs/floors/walls is
being measured. Texture cache copying must not be described as runtime texture mapping.

## Reproduce the headless checks

With `make tiemu-camera-demo` already prepared, run `make camera-audit`.
`tools/camera_audit.inc` drives the original update/render/collision/combat code, using
the bounded demo's compact ISO presentation. It checks both F1/F5 switches, direct entry
in both presets, both zoom directions, wide return, melee/ranged cinematics, first fixed
view construction and 128 fixed-view frames after a 32-frame warmup. All cases use the
same original assets/units and scenario 65; the ranged case uses scenario 66.

The `fftaudc.89z` headless binary and PC oracle match 468 timed framebuffers; each case
also checks completion, camera state and/or exact final HP. Setup/warmup lies outside
the measured intervals. Cinematic intervals begin at valid attack confirmation and end
after the returned ISO frame is rendered: 56 frames, including the final ISO response.
`headless.json`, `cycles.log` and `pc.csv` retain provenance and CPU counts.

## Actual TiEmu timer run

Load `x/camera-audit/fftaudit.89z` and every archived bank from
`x/tiemu-camera/data/` on a clean Titanium profile. Wait for the complete transfer,
then run `fftaudit()`. The suite is automatic and takes minutes because it retains all
slow geometry frames. Grayscale, buffer swaps and the ordinary eight-tick/32 FPS cap
remain active. It prints total elapsed milliseconds, rendered frame count and the worst
frame for each case from the 256 Hz timer. ENTER displays cinematic phase totals.

The test covers the actual native renderer and its original actors/combat. It uses a
compact ISO UI to fit one calculator variable; it is not a full tactical release. F1/F5
timings measure hold/decode/switch, not the missing textured animation.

Datasheet CPU counts exclude display/interrupt/frame waits; actual TiEmu timer readings
include those paths. Neither is a physical-calculator measurement. TiEmu's instruction
timings undercount some shifts and MOVEM operations, so a fast emulator result alone
would not establish a real-hardware frame budget.

## Results: actual TiEmu Titanium, 2026-10-16

Measured with the configured 12 MHz profile, active grayscale, buffer swaps and the
32 FPS cap. The table is read from the actual calculator screen; the local AMS font
decodes the printed digits exactly (`tools/camera_audit.py --read-tiemu`). Evidence:
`x/camera-audit/timer-results.png`, `timer-results-lcd.png`, `stage-page.gif`,
`cinematic-stages.png`, `tiemu.json` and `tiemu.csv`. These are emulated timer readings,
not PC GIF timing or physical-calculator results.

| Native operation | Total time | Frames | Worst frame |
|---|---:|---:|---:|
| F5, hold/decode/switch | 0.398 s | 4 | 0.175 s |
| F1, hold/decode/switch | 0.386 s | 4 | 0.164 s |
| ISO to close | 28.441 s | 18 | 1.792 s |
| ISO to wide | 28.570 s | 18 | 1.769 s |
| Wide to close | 18.796 s | 18 | 1.582 s |
| Close to wide | 18.796 s | 18 | 1.605 s |
| Wide to ISO | 27.257 s | 18 | 1.769 s |
| Melee cinematic, through returned ISO frame | 36.292 s | 56 | 1.781 s |
| Ranged cinematic, through returned ISO frame | 36.808 s | 56 | 1.839 s |
| First close fixed view, no travelling | 1.582 s | 1 | 1.582 s |
| First wide fixed view, no travelling | 1.605 s | 1 | 1.605 s |
| Prepared close view, 32-frame warmup | 4.500 s | 128 | 0.039 s |
| Prepared wide view, 32-frame warmup | 4.000 s | 128 | 0.031 s |

Prepared close/wide views display animated actors at **28.4/32.0 FPS**. Entry renders
about **0.63 frames/s**, zoom about **0.96 frames/s**. These are different workloads.
The original four-camera scenery benchmark remains a different fixture; its figures
must not replace these complete-transition measurements.

Actual cinematic stages:

| Shot | Entry | Action, including first fixed-view construction | Return | Final ISO response |
|---|---:|---:|---:|---:|
| Melee | 15.878 s | 4.242 s | 15.878 s | about 0.294 s |
| Ranged | 16.828 s | 2.800 s | 16.886 s | about 0.294 s |

The first native action frame still builds geometry/depth, masks and scaled poses before
copying the reviewed endpoint texture. Every travelling step invalidates the background
and pose cache, repeating geometry rasterization and sprite preparation. Removing travel
textures cannot fix this measured path: travel texturing is already absent. The separate
12 MHz datasheet audit counts about 22.3 CPU seconds per entry and 28.5/28.7 CPU seconds
per complete cinematic, before display/interrupt waits. Both checks reject the current
animation design; warmed actor FPS does not validate camera motion.

## Historical production recommendation

The later user decision restores animated low-poly F1/F5 and removes perspective
travelling/automatic shots; see [ROTATION_LOWPOLY.md](ROTATION_LOWPOLY.md).

- Keep the isometric game, selected contrast/height contours and the inexpensive native
  view switch. F1/F5 above does not deliver the PC's textured intermediate rotation.
- Keep the current smooth perspective travelling and automatic cinematic camera **off
  in a production TI build** until a replacement passes native timing budgets. This audit
  preserves the research implementations; it does not silently change gameplay defaults.
- A fixed inspection view is a separate option: this fixture takes roughly 1.6 seconds
  to construct, then animates well. Its texture bank covers exact review positions only;
  arbitrary-unit textured inspection is not established by this demonstration.
- Do not present fewer very slow geometry steps as a 0.5-second smooth transition. Even
  one of the current moving frames can cost 1.5-1.8 seconds in TiEmu.
- A compact native texture mapper, reduced geometry/visibility work or an explicitly
  approximate 2D zoom over existing scenery are future alternatives to measure. The PC
  filtered mapper and the 20 FPS review GIFs provide no timing result for those alternatives.
