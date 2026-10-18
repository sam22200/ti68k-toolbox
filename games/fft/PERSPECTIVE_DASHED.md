# Near-camera removal and dotted cutaway edges

Historical revision: the bounding-box selection below was too aggressive. The current
[PC-only review](PERSPECTIVE_REVIEW.md) replaces it with visible-pixel/depth selection
and three manual/automatic styles. Its TI validation is explicitly pending user approval;
the measurements below do not describe that newer strategy.

Build/check: `make fixed fixed-check fixed-xcheck` in `games/fft`. Run `fftfix(62)`.
MODE, F5, ENTER/F1 and SHIFT retain the [elevated-camera controls](PERSPECTIVE_CAMERA.md).
ALPHA alternates automatic visibility with the complete solid scene.

Automatic visibility now has three component states:

| State | Selection | Drawing |
|---|---|---|
| Solid | Outside the camera obstruction corridor | Filled faces and continuous edges |
| Dotted | Further along the corridor, away from the eye | Architectural creases/ledges only, no fill |
| Removed | Eye inside or very near the component bounds | Neither faces nor edges |

The eye sample uses an eight-world-unit horizontal margin instead of the former broad
20-unit removal region. The other four corridor samples keep their 20-unit margin and
select dotted outlines. The four-unit upper margin is retained. As before, the close
corridor extends 56 units ahead of Ramza; the wide one ends at him. This is a bounding-box
heuristic, not an exact surface-distance or point-in-building test.

Dotted edges use two black pixels on / two off. They retain architectural creases rather
than every triangle diagonal. Opaque scenery in front still clips them through reciprocal
depth; they write no depth of their own. Actors are composed afterward, keeping their
white outline and real heights. Water glints avoid the black edge pixels. No game collision,
LOS, targeting, tactical state or geometry coordinates change.

The existing 128-byte component scratch stores states 0/1/2. Archive data and heap sizes
are unchanged: the mesh is still version 4, 16,585 bytes, and the two game allocations total
110,358 bytes. Selection and dotted-line rendering happen only when building a cached
background; warmed animation has no new geometry pass or per-pixel visibility work.
The normal `fftfix.89z` is 65,585 transfer bytes, 65,495 payload bytes. Original TI-89 stock
AMS's launch-size limitation remains as documented in the camera note.

## Demo and measurements

`make fixed-dashed-gifs` writes local/ignored `x/fixed-dashed/`:

- `before-after.gif`: previous complete cutaway versus dotted volume cues, close/wide,
  synchronized 32 FPS for visual review. The previous `x/fixed-elevated/` source frames
  are optional; if absent, only this comparison is skipped.
- `four-views.gif`: all four new warmed views, synchronized 32 FPS.
- `perspective.gif`: the full 512-frame sequence at its estimated cycle cadence, including
  cold construction and cache misses.
- `frames.csv`, `manifest.json` and the additional 256-frame solid/automatic replay logs.

At 12 MHz, a 9% grayscale reserve and a 32 FPS limiter:

| Warmed view | Mean render cycles | Estimated FPS |
|---|---:|---:|
| Close / front | 195,448 | 32, capped |
| Close / back | 265,683 | 32, capped |
| Wide / front | 239,621 | 32, capped |
| Wide / back | 195,350 | 32, capped |

The full 512-frame sequence averages 18.65 FPS including repeated zoom rebuilds; cold
construction takes about 0.91–1.88 s. Pose/mask peaks are 16,470 and 1,582 bytes, with no
pose-pool reset. These are MC68000 datasheet-emulator estimates with estimated AMS calls,
not physical-hardware FPS. GIF delays round to centiseconds; startup is excluded.

Checks: all 768 captured frames match PC/TI; the existing camera/cache/water/RNG/Move tests
and return replay pass. `make fixed-cutaway-check` adds focused tests for inside, near,
just outside the eight-unit margin, further obstruction, camera above the object, distant
geometry and the solid override. A synthetic edge checks 48 solid samples versus 24 dotted
samples, no depth writes, and rejection behind foreground opaque depth. Both test sets
pass address/undefined sanitizers (pre-existing isometric alignment checks excluded).

The normal binary also launches in a clean private TiEmu Titanium/AMS 3.10 profile with
all eight banks archived. ALPHA in both directions restores the dotted view, reviewed
once in `x/fixed-dashed/titanium.png`; SHIFT returns to tactics. The private emulator and
display were stopped afterward. Emulator timing is not used for the FPS estimates.
