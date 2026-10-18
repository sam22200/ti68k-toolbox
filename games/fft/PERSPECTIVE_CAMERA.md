# Elevated overview and camera cutaway

The [latest dotted-cutaway refinement](PERSPECTIVE_DASHED.md) limits complete removal to
the immediate camera neighborhood and keeps other obstructing volumes as dotted edges.
The measurements and GIFs below describe the preceding complete-removal version.

Build `make fixed fixed-check fixed-xcheck` from `games/fft`, then run `fftfix(62)`.
This extends the [water/zoom experiment](PERSPECTIVE_WATER.md); that earlier demo and
measurements remain local in `x/fixed-water/` for comparison.

## Camera and controls

- MODE: close shoulder / elevated wide view. Wide keeps distance 80, raises the camera
  from 28 to 76 world units above Ramza's feet, and looks down 33.75 degrees instead of
  5.625. Close keeps distance 40, height 28 and the previous 5.625-degree pitch.
- F5: opposite side of Ramza at the current zoom/height/pitch.
- ALPHA (PC V, script D): toggle selective cutaway / solid buildings. Cutaway is enabled
  initially. This setting survives switching zoom/direction during inspection.
- ENTER/F1: forward direction. SHIFT/ESC/DIAMOND: return to tactics.

The wider view now looks down across the canal, terrain and units rather than pulling a
horizontal eye-level camera backward. The shoulder offset and real map/unit heights are
retained. Camera operations do not turn, move or otherwise change any unit.

## Selective cutaway

The offline pipeline groups architectural triangles into connected components, keeping
ordinary street/canal triangles in group zero. Shared ground edges alone cannot merge
neighboring buildings. Gariland produces six components, each with a small world-space
bounding box, one group byte per face and one per architectural edge.

On a cold view build, five samples test a corridor from the camera towards Ramza's torso.
In close mode it extends another 56 world units forward, clearing the nearby building
that previously filled the left of the view just ahead of him. Boxes have a 20-unit
horizontal margin and a four-unit upper margin. Wide mode ends at Ramza, retaining more
of the city for an overview. A matching component's fill **and** edges are omitted.
Entire components disappear together, avoiding diagonal triangle-shaped wall holes.

This is a small bounding-box heuristic, not an exact camera/world intersection solver.
It can hide more of a nearby building than strictly necessary. Roof units keep their
true height and can look unsupported when their supporting building is hidden; ALPHA
restores the complete structure for comparison. Street/canal geometry, water animation,
unaffected buildings, and actor/world depth tests remain active.

The underlying terrain, collision, Move/Jump, targeting, game line-of-sight and combat
rules are unchanged. Presentation visibility does not make attacks through walls legal.

Version-4 `fftmesh` retains the old tables/mesh/material layout and appends component
metadata: 1,391 extra archive bytes, total 16,585 bytes (16,681 transfer bytes). The legacy
three-camera renderer accepts version 4 and ignores that appendix. Native fixed view
still accepts versions 2/3 without cutaway metadata. A 128-byte scratch flag array reuses
the existing unpacked-view allocation; no additional heap block or assembly is used.
Selection happens once per view rebuild, with no component tests in warmed animation.

The first normal TiEmu run exposed a RAM-allocation failure that the headless runner did
not reproduce. The fixed variant now omits 2,748 bytes of unused isometric rotation work:
span arrays, the half-resolution planes, byte-doubling table and rotated vertex/sort
grids. Those rendering paths were already excluded from this variant. Other builds keep
their original buffers. Game scene/work plus unpacked-view allocations fall from 113,106
to 110,358 bytes without reducing cache limits or tactical/water features.
The reduced build launches successfully on the same clean private Titanium profile.

## Demo and benchmark

```sh
make fixed-camera-gifs
```

The script builds the capture TI binary, compares all 512 source framebuffers with the
PC reference, then checks another 256 solid/cutaway frames in both zooms and directions.
Outputs stay local in `x/fixed-elevated/`:

- `before-after.gif`: close and wide before/after, synchronized 32 FPS visual comparison.
  Requires the previous local `x/fixed-water/` source frames; it is skipped if absent.
- `four-views.gif`: the four new warmed views at synchronized 32 FPS.
- `perspective.gif`: full sequence paced by measured cycle estimates, including stalls.
- `frames.csv`, `manifest.json`, `walls-cycles.log`, `walls-pc.log`: evidence and hashes.

At 12 MHz with 9% grayscale reserve and the 32 FPS limiter:

| New warmed view | Mean render cycles | Estimated FPS |
|---|---:|---:|
| Close / forward | 195,485 | 32, capped |
| Close / reverse | 265,683 | 32, capped |
| Elevated wide / forward | 239,621 | 32, capped |
| Elevated wide / reverse | 195,422 | 32, capped |

The 512-frame demo averages 18.83 FPS including repeated zoom construction and pose
misses. First close/forward construction takes 1.349 s, close/reverse 0.857 s; wide takes
1.002–1.870 s. A later direction change incurs an 81.72 ms pose miss. The largest observed
pose pool occupancy is 16,470/24,000 bytes, masks 1,582 bytes, with no pose-pool reset.
Counters are reset when changing zoom, so maximum counters are reported per zoom.

These are MC68000 datasheet-emulator cycle estimates with estimated AMS calls, not
physical hardware FPS or TiEmu timing. GIF delays round to centiseconds. Startup is
excluded. The normal `fftfix.89z` is 65,535 transfer bytes, 65,445 payload bytes, within
the TI variable limit; stock original TI-89 AMS still requires the separate packed-release
effort because of its 24,576-byte launch limit.

Tests cover the two pitch settings, solid/cutaway round trips and reconstruction, actor
and tactical/RNG preservation, 48 cached/rebuilt street/roof images over four facings and
both zooms/directions, four-phase water, and returning to Move. PC/TI return replays and
768 capture frames pass, as do address/undefined sanitizers (alignment checks excluded
for the pre-existing isometric word-aligned casts). The legacy three-camera tests and
their three PC/TI return replays also pass.

The final normal binary was run in a private TiEmu Titanium/AMS 3.10 session with eight
archived banks, exercising ALPHA, MODE, F5 and SHIFT. `titanium.png` shows the close reverse
view; `titanium-zoom.gif` records the successful elevated-wide change after using a longer
MODE press. A short press can be missed during cold construction. This emulator GIF is
functional evidence, not a performance benchmark. The private container/display were
stopped after review.
