# Bounded Titanium camera demonstration

This historical motion/cinematic script now requires `FFT_CAMERA_MOTION` and its build
script explicitly enables it. The accepted replacement is the
[direct-view camera demonstration](CAMERA_CUTS.md), `make camera-cuts-demo`.

This build uses the original fixed-point camera renderer, actor animation and combat
logic, with a compact isometric presentation. The complete tactical UI plus perspective
currently exceeds one AMS variable: the latest complete build is 72,884 bytes before
the file wrapper. This demonstration is not the complete tactical executable.

The automatic sequence starts in scenario 65: Ramza at `(5,5)`, the original enemy at
`(6,5)`. It enters wide inspection, changes to close inspection, returns to ISO, selects
Act/Attack and the neighbouring enemy through the original update routine, then plays
the enabled cinematic. The target loses 12 HP; EXP/JP appear over the attacker. ESC exits.
The script waits for each transition to finish; it never skips render or combat updates.

The four ISO backgrounds use the selected contrast-plus-contours style. Two exact
manual inspection backgrounds (12,008 bytes including their descriptor) are prepared
offline using the PC texture reference. Native geometry still creates the depth/masks,
and the TI renders and animates the characters over these fixed backgrounds. The existing
two-endpoint `fftshot` bank supplies the settled attack texture. Moving camera frames
use the native flat geometry renderer: the PC's general texture sampler is not included.
This bounded review bank is not a production strategy for storing room orientations.

## Reproduce without opening the emulator

With the local extracted FFT assets and the PC texture reference already prepared:

```sh
make isometric-storage          # prepare the four selected compressed views, once
make tiemu-camera-demo
```

`tools/camera_tiemu_demo.py` builds `x/tiemu-camera/fftshow.89z` (56,141 bytes),
`fftshowc.89z` and the PC oracle. It stages ten data variables in
`x/tiemu-camera/data/`, marked for direct archive transfer. All 470 per-frame PC/TI
checksums match, covering both inspections, all transition frames, damage and rewards.
The PC replay also checks that both inspection presets and the cinematic were reached,
that Ramza returns to ISO, and that the enemy ends at 31 HP.

## Actual emulator capture

On a dedicated Titanium TiEmu profile/display, start a clean run with `fftshow.89z`
and every staged `.89y` bank. The sequence plays automatically, so no key script timing
depends on rendering speed. Record using `ti-gif` at 15 capture FPS; this sampling rate
does not establish game FPS. The GIF keeps the actual emulator timing, including geometry
preparation pauses. TiEmu timing remains different from real MC68000 timings.

Actual capture verified on 2026-10-16 in a private Titanium AMS 3.10 instance. The native
capture is `x/tiemu-camera/fft-perspectives-cinema-tiemu.gif`; `-4x.gif` is the same clip
enlarged with nearest-neighbour sampling. The raw capture is 142.14 seconds. Trimming
removes 1.07 seconds before the first game image and the idle tail after two seconds of
the completed attack, leaving 121.13 seconds, with no interior cuts or speed changes.
`capture.json` records the timing and scope. The actual GIF shows both textured inspection
endpoints, the swing, centered `12`, enemy HP changing from 43 to 31, EXP/JP above Ramza,
and the final isometric `Attack complete` screen. Native moving geometry remains slow.

The boot transfer of ten archived banks outlasts `ti-emu`'s five-second startup wait:
wait for the whole transfer to finish before typing `fftshow()`. The first premature
launch was discarded. The successful capture uses a clean boot and a completed transfer.
The ordinary desktop emulator and its saved profile are not modified.
