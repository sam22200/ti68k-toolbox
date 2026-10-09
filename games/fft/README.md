# Final Fantasy Tactics: Magic City Gariland (tech demo)

An isometric battlefield on the TI-89 / Titanium in the style of Final Fantasy Tactics (PS1,
1997): the real Gariland map (10 x 15 tiles, heights 0 to 10) seen from four camera
orientations, an animated 90-degree rotation, three units, depth sorting and occlusion by the
buildings, and one unit moved over the terrain with FFT's Move / Jump rules. Not a port: our own
engine on the Portable Game Runtime; the heights and the scenery (the map's own textured mesh,
drawn in four greys on the PC) come from the local disc (`tools/extract.py`), the units, cursor
and rotation frames are ours (`tools/art.py`). Study notes: `RE_NOTES.md`; milestones:
`ROADMAP.md`.

## Controls

| Key | Action |
|---|---|
| arrows | move the cursor (relative to the view: UP = up-right, RIGHT = down-right) |
| [F5] / [F1] | rotate the camera 90 degrees clockwise / counter-clockwise |
| [2nd] or [ENTER] | on Ramza or Delita: show the reach; on a reachable tile: walk there |
| [shift] or [ESC] | cancel the selection; [ESC] with nothing selected quits |

The HUD shows the view (S, W, N, E: where the camera stands), the height under the cursor in
FFT units (`h2`, `h7.5` on a slope) and the unit there. Scenarios (`fft(n)`, `--scenario n`):
0 start, 1-4 the thief behind a house seen from each view, 5 Ramza's reach, 6 mid-rotation,
7 Ramza walking.

## Build

```sh
make map.h        # once: reads MAP/MAP022 from roms/ps1/Final Fantasy Tactics (USA)/ (local):
                  # map.h and the four views fftv0.bin .. fftv3.bin, ZX0-packed (~2 min)
make test         # headless tests (rotations, heights, reach, occlusion, keys)
make pc           # SDL window: ./fft_pc   (F1 / F5 on the PC keyboard too)
make ti           # fft.89z and fftv0.89y .. fftv3.89y: send all five, archive the four views
                  # (~10 KB each, read in place), run fft()
make xcheck       # the TI binary under ti-cycles = the PC, 8 scenarios
```

`map.h` and the views are generated from the disc and never committed: without the disc the
game does not build. `art.h` is ours and committed (`make art.h` regenerates it from
`tools/art.py`). Without the four views the game says `missing fftv0-fftv3`.

## Architecture

- **One map, four views.** World tiles `(x, z)` are turned into view tiles `(u, v)` per
  orientation (0: `(x, z)`, 1: `(H-1-z, x)`, 2: `(W-1-x, H-1-z)`, 3: `(z, W-1-x)`) and a tile's
  four corner heights are renumbered with it (`(c - rot) & 3`); `build_view()` keeps the
  current orientation's tiles in view order. Units, reach and paths live in world coordinates
  only: turning never moves anything.
- **Projection.** A tile is a 24 x 12 diamond, one FFT height unit is 6 px:
  `x = (u - v + H') * 12`, `y = 64 + (u + v) * 6 - h * 6` in the scene. FFT's own ratio
  (tile 28 units wide, h = 12 units) gives 6.3 px at this tile size.
- **The views are the game's map, drawn on the PC.** `tools/extract.py` reads the map's
  textured mesh (MAP022.9: 866 triangles once the quads are split; texture MAP022.8, 16
  palettes) and draws each orientation with a depth buffer in the projection above, 4x
  supersampled, averaged to the TI's pixels and cut into 4 greys by luminance (the 25 / 55 /
  82 % percentiles of the whole map: the same greys in every view). The mesh matches the
  terrain grid (x / 28 and z / 28 are the tile, -y / 12 the height). A view is one data file
  (`fftvN`, 52,320 bytes packed with ZX0 into 9.1-10.3 KB): the two planes of the 320 x 218 scene, then the **depth**: every
  pixel belongs to a tile (its face's centre pushed a little into the solid, so a wall
  belongs to the tile it bounds), and per image byte the two frontmost view diagonals `u + v`
  with the mask of their pixels (a third depth in one byte is dropped: 2.5 % of the bytes).
- **Views in memory**: one view at a time is unpacked into a 52 KB buffer (`zx0_asm` from
  `lib/unpack68k.s` on the TI, 2.2 M cycles = 0.18 s; a C copy on the PC), during the first
  frame of the turn that reaches it, and copied into the 17 KB scene buffer when the
  orientation or the reach changes. Two allocations: AMS refuses blocks above ~64 KB.
- **Every frame**: the camera's 160 x 100
  window is copied from it at any pixel (32-bit shifted words), the units are drawn back to
  front through a **cover mask**: the pixels of their 16 x 22 box whose depth is greater than
  the unit's diagonal, read from the view (both layers), so a building hides them exactly as
  it is drawn. Then the blinking cursor (always on top, as in FFT) and the HUD.
- **Rotation.** Three frames between two views, drawn at half resolution (80 x 50, doubled)
  as flat polygons projected at 22.5, 45 and 67.5 degrees around the cursor tile, sorted by
  depth (counting sort), only the faces towards the camera, units on top; the cursor tile stays
  where it was on screen and the camera eases back afterwards. The 45-degree entries of the
  sine table equal the cardinal projection exactly (6 x 256), so the last frame meets the
  composed view.
- **Reach.** FFT's rules for a squire: Move 4, Jump 3 (height difference in half units, both
  ways), canals, trees and chimneys block, enemies block, allies can be passed but not stood
  on (`bfs`). The reachable tiles get a dotted ring at their standing height, drawn into the
  scene buffer after a fresh copy of the view; the pixels in front of the tile (the view's
  depth) are put back over it.
- **Walking.** Tile by tile along the BFS path, 4 frames per tile, a hop on height changes;
  while between two tiles the unit is sorted after both.

## Numbers (ti-cycles, Titanium, datasheet timings)

| | cycles | at 12 MHz |
|---|---|---|
| ordinary frame (view copy 131k, units with their cover masks 69k, cursor + HUD 23k) | ~225k | 32 fps (frame-limited) |
| walking frame | ~237k | 32 fps |
| selecting a unit (view copied, reach rings drawn) | 0.56 M | 0.05 s |
| one rotation frame (half resolution) | 2.3 M | 3 frames + the next view unpacked (2.2 M): 0.75 s per turn |
| start (the first view unpacked) | 2.5 M | 0.2 s |

Program: `fft.89z` 17.5 KB, plus four data files of 9.1-10.3 KB (38.9 KB of archive; 209 KB
unpacked, see the history below). RAM: the unpacked view 52 KB, the scene buffer 17 KB, ~6 KB
of work. Checked in TiEmu (Titanium, `ti-run`, the four views archived
at the transfer: attribute byte 3, see the `ti89-emulator` skill): the views, F5, a
selection with its rings (`x/emu_real.png`; packed views: `x/emu_zx0.png`, S, W, N and a
selection, local). The first version drew
its own scenery on the calculator (24 KB program, 4 x 17 KB of RAM): start 1.6 s, walking
frame 370k, selection 3.8 M.

## Limits

- The rotation is 0.54 s and the selection 0.31 s, not instantaneous: drawing polygon rows in
  C costs ~550 cycles per row (both planes), and a rotation frame has ~200 polygons.
- The TI-89 HW2 (AMS 2.09) was not run; the program fits its limit, the game needs ~75 KB
  of free RAM (else it says so).
- Smaller data was measured and not kept: tile sprites deduplicated (FFT paints each face of
  each house: 686 sprites, 616 unique, 601 with 180-degree and mirror flips, 439 at 20 % of
  pixels tolerated: 102 KB), smoothing the speckles before packing (-10 %, windows lost);
  dropping the depth data (the old cover masks from the grid) would give ~22 KB but an
  approximate occlusion and a 370k walking frame.
- The four greys are a straight luminance cut: the textures read as speckles on roofs and
  paving (the user's choice, against our flat drawn version). The rotation frames are still
  our flat greys per material, not the textures.
- The camera has one elevation (FFT has two), no zoom; the map has only its first level (FFT's
  second level, bridges over passages, is empty on Gariland).
- Roofs are walkable as in the terrain data.
- No combat, menus, jobs, AI or turn order: a rendering and terrain proof of concept.
