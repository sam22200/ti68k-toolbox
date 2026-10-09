# Roadmap

Each milestone is tested headless (`make test`, `make xcheck`), the emulator at the end.

1. **Terrain in four orientations** (done): the Gariland map from the disc, view rotation of
   tiles and corners, the projection, back-to-front composition of flat tiles.
2. **Heights and walls** (done): wall segments down to the front neighbour, outlines where
   heights differ, sloped tiles as polygons.
3. **Buildings and depth** (done): houses with sloped roofs, chimneys, trees, boxes; one scene
   per orientation composed once; textures and materials (`tools/art.py`).
4. **Units, occlusion, movement** (done): three units, cover masks for occlusion, cursor, reach
   (Move 4 / Jump 3), paths, walking with hops; the thief hidden from two views, seen from two.
5. **Speed and polish** (done): the rotation animation at half resolution, four scenes at the
   first frame, reach drawn locally, the program under 24 KB, TiEmu run (Titanium).
6. **The game's own scenery** (done, the user's choice): the textured mesh drawn per view on
   the PC in 4 greys with a two-layer depth per byte, four archived data files read in place,
   cover masks and reach rings from that depth; no composition on the calculator.
7. **Packed views** (done): ZX0, 209 KB of archive down to 38.9 KB, one view unpacked per
   turn (0.18 s), the same screens (xcheck checksums unchanged).

Next, if wanted:
- the TI-89 HW2 (AMS 2.09) run and its free RAM (single-buffer fallback);
- a faster rotation (fewer, larger polygons per column, or a 68000 span filler: ask first);
- FFT's second camera elevation;
- a hand-tuned grey conversion per material (as Alundra's village: lighter paving, calmer
  roofs) if the speckles are too busy on the LCD;
- rotation frames closer to the textures (a mean grey per polygon from the views).
