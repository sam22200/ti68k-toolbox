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
8. **Rotation greys from the views** (done): each face of the rotation frames takes the mean
   grey of its pixels in the four views, rounded (solid: checkerboards for the half greys were
   tried and declined), instead of a flat grey per material; +2 % per rotation frame (2.06 -> 2.11 M), TI = PC on the turn
   frames. A grey conversion tuned per material was tried first and declined (the user kept
   the straight luminance cut).
9. **FFT's units** (done): Ramza, Delita, Agrias and a thief from the disc's battle sprites
   (`tools/units.py` -> `units.h`, never committed), four directions (two drawn, mirrored),
   FFT's idle and walk animations, facing set by the walk; frame ~270k (units 116k for four),
   program 23.2 KB.
10. **Proportions and readability** (done): the units scaled 0.6 (FFT's height against the
    tiles, measured on a capture of the game; 16 x 26, the eyes kept), a hidden unit's contour
    drawn over the building (its mask eroded twice on the covered rows), the reach marker as a
    black line with a white one outside (the user's pick of four); frame ~305k (units 152k),
    program 24,481 bytes (95 left under the TI-89's limit).

11. **Life and teams** (done): the units' sprites in an archived data file `fftu` (program
    24,481 -> 19,205 bytes), the eyes placed on their own (1 x 2, a skin pixel between, the
    face one grey lighter), the canal's water glints sliding along it (~13k cycles), an arrow
    above enemies; frame ~320k, TI = PC on 8 scenarios.

Next, if wanted:
- the TI-89 HW2 (AMS 2.09) run and its free RAM (single-buffer fallback);
- a faster rotation (fewer, larger polygons per column, or a 68000 span filler: ask first);
- FFT's second camera elevation;
