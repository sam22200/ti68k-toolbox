# Camp-fire scene (Chrono Trigger)

The whole 256×224 SNES scene at 1:1 in 4 grey levels, scrollable with the arrows (2nd = faster,
ESC = quit). Runs on the TI-89 Titanium and on an unpatched TI-89 (15,457 bytes, well under the
24 KB limit of AMS 2.xx: tiles, sprites and fire, 20,776 bytes, are stored as one 11,138-byte ZX0
stream and unpacked at start-up by the asm decoder `zx0_asm`, ~0.1 s).

- **Map**: trees, ground and glow as 217 unique 16×16 grey tiles, drawn by ExtGraph's TileMap engine
  straight into the GrayDBuf hidden planes.
- **Sprites**: the 7 characters (Robo with Lucca, Frog, Ayla, Marle, Crono, Magus) are separate
  masked 32-pixel-wide sprites with a white outline, each with its own grey contrast.
- **Fire**: 8 unique 32×48 frames played in the GIF's 13-step order, 200 ms per step.

```sh
tools/pyenv/bin/python games/campfire/tools/extract.py /tmp   # GIF -> data.h (+ previews in /tmp)
python3 games/campfire/tools/pack.py                           # data.h -> zdata.h (ZX0)
cd games/campfire && ti-cc -o campfire campfire.c ../../lib/unpack68k.s ../../tools/extgraph/lib/tilemap.a
ti-run campfire.89z
```
`tools/pyenv` is a venv with numpy, scipy and pillow (`python3 -m venv tools/pyenv &&
tools/pyenv/bin/pip install numpy scipy pillow`). The pipeline is described at the top of
`tools/extract.py`; the character outlines are hand-traced polygons (`POLY`) refined automatically.
