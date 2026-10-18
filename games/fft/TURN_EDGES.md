# Live height contours during low-poly rotation

F1/F5 now draw one-pixel height contours during the three native 160x100 intermediate
frames. The textured ISO endpoints retain their existing offline contrast/contours.
No textured-turn bank, per-angle background, depth buffer or new assembly is added.

## Method and limits

`draw_turn` already projects all tile corners, sorts terrain columns back to front and
compares their corner heights to neighbouring columns. A four-bit local mask keeps
top lips bordering a lower neighbour. After filling each column's sides and top,
`turn_edge` draws these lips black on white/light-grey tops, white on dark-grey/black
tops. Equal-height tile boundaries are skipped; this is not a full terrain grid.

The face shade is already known, so line colour needs no framebuffer readback for
contrast selection. Short integer segments step byte pointers/masks with no multiply
or division per pixel. Wholly on-screen segments skip per-pixel clipping; partially
off-screen ones retain bounds checks. Nearer columns subsequently paint over earlier
lines using the existing painter order. Actors are drawn afterwards as before.

This is the same readability principle as the fixed ISO contours, applied to the
rotation's simplified height columns. It does not recover detailed mesh roof/wall
creases, add exact per-pixel visibility, or change the pre-existing lack of terrain
cover on actors during low-poly motion. Painter-order limitations remain. The lines
are computed live, unlike the fixed view's baked contours, and therefore cost CPU.

## Matched native measurements

Eight cases cover F1 and F5 from all four orientations, with current camera-review
banks, compact UI and identical actors/pivots/three intermediate angles. Timings are
MC68000 datasheet CPU counts at 12 MHz, including target decode and final ISO rendering;
they exclude grayscale interrupts, presentation and waits. They were measured without TiEmu.

The later interactive TiEmu check below verifies display/keys; it does not change
these headless timings into elapsed hardware results.

| Metric | Without live contours | With live contours |
|---|---:|---:|
| Mean complete turn, CPU ms | 1,084.154 | 1,209.423 |
| Compact Work, bytes | 22,762 | 22,762 |
| Interactive review executable, file bytes | 61,419 | 61,971 |
| Added asset bytes | 0 | 0 |

Added CPU averages **125.269 ms per turn (+11.55%)**, about 41.756 ms per moving
picture; the final ISO image is unchanged. Added code is **552 bytes**, with no heap,
asset or framebuffer growth. An initial generic clipped loop cost 136.954 extra ms;
the on-screen fast path removes 11.685 ms without changing any of 608 picture checksums.
These numbers are specific to Gariland and this build; they are not elapsed TI timings.

## Verification and review

```sh
make turn-edge-review
make test turn-direction-check
make pc ti cycles camera-config
```

The first command builds both controls sequentially. Each variant matches 32 native/PC
turn images plus 576 broad-camera images (corner/street/roof pivots and anchor phases).
The broad PC runs use AddressSanitizer/UBSan, excluding the existing fixed-point shift
idioms. All eight final images are byte-identical across variants; the 24 moving
images change only to the LCD's black/white tones. Unit positions, battle RNG and final
orientation remain checked. The independent direction oracle checks 42,240 corners.

Local artifacts: `x/turn-edges/comparison.gif`, `comparison.png`, `checked.json`,
per-variant `cycles.log`, `broad-cycles.log`, frame PNGs and interactive `fftcfg.89z`.
The GIF is a **PC framebuffer preview at a fixed illustrative cadence**, not an emulator
capture or FPS measurement. The current default interactive binary is also rebuilt by
`make camera-config`; its existing nine-bank send directory remains `x/config/data/`.

`-DFFT_TURN_NO_EDGES` preserves the prior plain low-poly renderer for comparison.
Existing textured research variants bypass this terrain drawing path.

## Interactive Titanium verification — 2026-10-17

The exact current `x/config/fftcfg.89z` (61,971 file bytes) was launched from a clean
boot in Docker TiEmu, Titanium / AMS 3.10, with all nine data banks archived. The
unaccelerated capture shows F5 twice, F1 twice, visible relief contours during motion,
the four ISO destinations, CATALOG's two-option menu and ESC returning to the map.
The emulator is left running for the user. This is the bounded compact-UI camera
review, not a full tactical+perspective release or a physical-calculator test.

Local evidence: `x/turn-edges/tiemu-rotations.gif` (25 seconds sampled at 20 Hz),
`tiemu-contact.png`, `tiemu-keys.json` and `tiemu-checked.json` with binary/source/bank
provenance. Capture cadence is not measured game FPS; this run did not time individual
rotations. The separate CPU measurements above remain the performance evidence.
