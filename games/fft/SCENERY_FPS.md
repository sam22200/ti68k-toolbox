# Flat versus textured perspective: actual TiEmu FPS

The user requested a comparison of flat ground/walls/roofs against textured scenery,
with character sprites retained in both cases. The TiEmu timer run confirms essentially
identical warmed performance. This is cached scenery replay: the texture projection is
still prepared offline on PC, so these figures do not measure real-time texture mapping.

## Measured display cadence

Docker TiEmu 3.04, TI-89 Titanium HW3 / AMS 3.10, private display/profile. CPU rate is
explicitly configured to 360,000 cycles per 30 ms (12 MHz). Grayscale interrupts are active.
Each case warms 32 animated frames, measures 512 update/render iterations without swapping
or waiting, then measures 128 displayed frames with GrayDBufToggleSync and the usual
eight-tick / 32 FPS limiter. Timing uses the emulated 256 Hz auto-int-1 counter.

| View | Flat + actors | Textured + actors | Raw rendering capacity, both |
|---|---:|---:|---:|
| Close/front | 32.0 FPS | 32.0 FPS | 51.5 FPS |
| Close/back | 28.4 FPS | 28.3 FPS | 36.4 FPS |
| Wide/front | 28.5 FPS | 28.5 FPS | 40.9 FPS |
| Wide/back | 32.0 FPS | 32.0 FPS | 52.2 FPS |

The 0.1 FPS close/back difference is within timer/interrupt-phase resolution. The raw
column is a render benchmark with grayscale IRQ overhead, not a displayed frame rate:
it omits buffer-swap synchronization and the frame wait. The displayed loop includes
keyboard reading, buffer swaps and waiting. Its lower rates in two views show why a
CPU-only estimate of `min(32, capacity)` cannot establish actual synchronized cadence.

## First frame

| View | Flat construction | Textured construction/cache load |
|---|---:|---:|
| Close/front | 2148 ms | 2152 ms |
| Close/back | 1687 ms | 1691 ms |
| Wide/front | 4378 ms | 4378 ms |
| Wide/back | 1996 ms | 2000 ms |

These first-frame figures include the current geometry/obstruction passes, depth masks and
initial sprite scaling. Textured cases then copy in a 6,000-byte PC-prepared background;
its original texture projection time is absent. The wide/front view needs a second geometry
pass for actual-pixel cutaway and has the largest stall. Cached backgrounds avoid repeating
that construction during idle animation. Camera transitions are outside this measurement.

## Independent TI binary checks

`ti-cycles` verifies all 1024 source frames against PC checksums. Its datasheet-counted
warmed render costs match exactly between corresponding flat/textured cases:

| View | Mean cycles | Minimum | Maximum |
|---|---:|---:|---:|
| Close/front | 202906 | 202888 | 202916 |
| Close/back | 273424 | 273386 | 273526 |
| Wide/front | 247228 | 247146 | 247346 |
| Wide/back | 200579 | 200092 | 201140 |

Each textured cold case costs 36,268 extra cycles for loading the prepared image. The
cycle runner has estimated AMS costs and does not run actual grayscale/display timing.
Its 12 MHz / 9% grayscale-reserve / 600-cycle input-update model estimates enough CPU
capacity for a 32 FPS ceiling, but does not model blocking GrayDBufToggleSync waits.
Actual TiEmu timer values above therefore take precedence for this emulator run.

Neither set of figures is a physical-calculator measurement. TiEmu also undercounts some
68000 instructions (especially movem and multi-bit shifts), so no hardware FPS guarantee
is implied. The separate cycle results retain correct datasheet timings for those paths.

## Reproduction and evidence

`make fixed-scenery-ti-benchmark` builds `x/scenery-benchmark/fftemu.89z` and `fftscyc.89z`,
stages the original archived banks plus four cached backgrounds, runs all eight TI cycle
cases and validates their PC images. It requires the existing local-disc review assets.
`ttbin2oth` must run in the destination directory with a bare output prefix: passing a full
path would create an invalid calculator variable name. The build script checks the names.

For the timer run, use `ti-run fftemu.89z banks/*.89y` from that output directory in an
isolated Titanium profile, with the configured CPU rate above. All banks carry archive
attribute 3. The program automatically runs the eight cases and ends at one printed table.
Large boot transfers can outlast ti-run's fixed startup delay; if the first command is
lost, wait for the transfer to complete, then invoke `fftemu()` at the empty HOME prompt.
No file chooser or resend into a running program is needed. Executable transfer sizes:
22,975 bytes for the timer program and 21,377 bytes for the cycle program.

Local ignored evidence under `x/scenery-benchmark/`:

- `tiemu-results.png`: actual calculator result screen.
- `tiemu-results.csv` / `.json`: transcribed timer readings, method and hashes.
- `tiemu-profile.ini`: exact emulator profile configuration.
- `cycles-summary.csv` / `.json`, `cycles-0..7.log`, `validated-ti.json`: cycle figures,
  all source checksums, binary/source/bank hashes.
- `fftemu.89z`, `fftscyc.89z`, archived banks and PC oracle logs.

The private emulator and X server were stopped after the test. The user's desktop and
normal emulator profile were left untouched. No assembly was added; the shared verified
ZX0 decompressor is linked as in the game.
