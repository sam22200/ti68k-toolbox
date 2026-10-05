# Alundra-style traversal (TI-89)

A tiny traversal engine for the TI-89 Titanium: one room, a player who walks in 8 directions,
jumps, and climbs or drops between terrain heights, with readable depth. Its rules were
studied in Alundra (PS1, Matrix Software / Working Designs 1997) with the `ti-port-ps1`
skill: `RE_NOTES.md` (findings labelled OBSERVED / INTERPRETATION / TARGET, the measured
numbers and how each was obtained), `alundra.sym` (RAM and code addresses). Not a port: no
code or data from the disc. The art is the disc's (decided): `tools/extract.py` runs the
local disc headless and generates `gfx.h` (Alundra's poses and four scenery textures); the
disc, `gfx.h`, `x/` and everything extracted stay local (`roms/ps1/`, `sources/alundra_ps1/`,
`.gitignore`).

```sh
make test                      # gfx.h from the local disc the first time, then the unit tests
make pc && ./alundra_pc        # PC window: arrows, [2nd] = Ctrl/Space/Z jump, C debug overlay
make cycles xcheck             # the TI binary under ti-cycles, TI = PC per scenario
make ti                        # alundra.89z
```

## Milestones

| # | Milestone | State |
|---|---|---|
| 1 | Behaviour understood: movement, jump, heights, ledges, landing, collision, depth (`RE_NOTES.md`) | done 2026-10-04 |
| 2 | Flat-ground simulation: a rectangle walks (8 directions) and jumps (gravity, landing), debug overlay, unit tests | done 2026-10-04 |
| 3 | Height map: per-tile levels 0-2, drawn as shades and numbers; floor = highest tile under the foot box, landing and falling on heights | done 2026-10-04 |
| 4 | Ledges: jump up +1, blocked at +2, walk/jump down; a key script over the whole room | done 2026-10-04 |
| 5 | Collision: walls, corners, obstacles, during ascent and descent; the test room complete | done 2026-10-04 |
| 6 | 16 × 16 presentation: tiles, the 14 × 20 player with outline, shadow, depth order | done 2026-10-04 |
| 7 | TI-oriented: data sizes, cycles per frame, xcheck, the emulator once | done 2026-10-04 but the emulator run (no X display in the session) |

Milestone 2 numbers (speeds since changed, see milestone 6): 3 px per step at 32 fps (2.25 per axis on diagonals), jump apex 13.9 px
in 12 steps (gravity 14/16 px), falls capped at 8 px per step; update ~760 cycles, render
~79k (a full-screen clear) per frame on the TI; `alundra.89z` 4 KB.
Milestone 3: the test room (10 × 6 tiles: open ground, platforms at 1 and 2, reachable
ledges 0→1 and 1→2, an unreachable 0→2, drops, a one-tile corridor, a pillar, walls), drawn
back to front (top faces raised 8 px per level, front faces, height digits in the overlay),
the player hidden by the tiles in front of it. The room is composed once into a background;
each frame copies it, draws the player and redraws only the tiles in front that overlap it:
render ~68k cycles per frame (~102k with the overlay), update ~3.7k; `alundra.89z` 13 KB
(the 7.5 KB background is a static array). The ledge rule (a tile blocks if it is a wall or
higher than the feet) is in, its own tests come with milestone 4.

Milestone 4: tests per ledge (walking into +1 stops flush, a jump is blocked while the feet are
below the top then passes and lands, +1→+2 by a jump, 0→+2 and the pillar unreachable walking
and jumping, walls at any height, walking and jumping down) and `keys/route.txt`, a run over
the whole room (both climbs, the drop, a failed +2 jump, row 4, row 5, the corridor) checked
at 7 points; the TI binary plays it under ti-cycles to the same final screen checksum (A8B2)
as the PC: render ~94k cycles per frame with the overlay.
Milestone 5: corners rounded as in Alundra (read in its move resolver, checked on the disc):
a straight move that cannot progress with only one of its two front corners blocked becomes
a sideways nudge of 1 px per step away from that corner (on foot and in the air), so the
player slips into the corridor or around the pillar; a flat wall (both corners) stops it.
Tests: the misaligned corridor entry, flat walls, around the pillar walking and jumping, a
jump that reaches the +1 ledge only on its way down (it slides down the face), a fall along
a wall. `keys/corners.txt` from scenario 6: TI = PC (B75D); `keys/route.txt` unchanged
(A8B2); update ~5.8k cycles per frame, render ~91k.
Milestone 6: the disc's art. The player is Alundra's own sprite: 36 images (4 directions ×
stand, 6 walk, take-off, air; left = right mirrored), each pose read from the game's GPU
packets while it walks, stops and jumps on the ship, composed from VRAM (body + legs), scaled
by 0.524 (42 px → 22, size B: ~12 × 22 standing) with a vote per pixel, its 16
colours mapped to 4 greys (gold hair and skin light, clothes dark), white outline. Walk image
every 5 steps (the game's 10 frames at 60 Hz), a diagonal faces up or down as in the game.
Scenery from the village of Inoa (reached from the user's memory card save): grass for the
tops (level 0 light grey, 1 white, 2 white framed), the stones of a retaining wall for the
front faces, cobbles for the walls, two greys each. Tests: the whole sprite
on screen pixel for pixel, the greys per surface, the hidden player leaving the wall's
pixels unchanged, the animation. The shadow (the game's, always there): the disc's ellipse
(13 × 8, 12 × 7 high in a jump) on the floor under the feet, adaptive so it always shows: two
greys darker on a light floor, black ringed with light grey on a dark one. Speeds are the
game's at the sprite's scale: 2.375 px per step across, 1.625 in depth (continuous, 1/16 px).
Each image is scaled into 32 columns then cut to the 16 around it, with its x offset from the
feet kept (`hero_ox`): the game's own placement (side walk frames sway up to ±2 px), nothing
clipped. Standing, Alundra breathes as in the game: the stand image 21 steps, in 5, out 2
(the game's 40, 10, 4 frames).
TI = PC on every scenario, the route (DA94, 200 frames) and the corners (615E).

Milestone 7: render ~36k cycles per frame without the overlay (~69k with it; was ~84k / 116k),
update ~4.9k: a frame costs ~41k of the ~360k budget. Each hidden plane remembers the
rectangle the player and the shadow covered and the next frame drawn into it restores only that
from the background (8.5k instead of a 45k full copy; `test_dirty`: the same screen as a full
copy on all 200 frames of the route); the shadow reads its floor through a 256-entry bit-count
table, unrolled (9.7k, was 22k). Data: the hero 6.6 KB (44 images 16 × 25 × 3 planes), the
background 7.5 KB (a static array), textures and shadows 0.2 KB; `alundra.89z` 24,965 bytes:
over the TI-89's 24,576 (fine on the Titanium; for the TI-89, allocate the background, which
needs an exit hook in the runtime to free it). The emulator run (greys, keyboard, real rate)
is still to do: `ti-emu start; ti-run alundra.89z; ti-play keys/route.txt`, one screenshot.
Scenarios: 0 the room's start (open ground), 1 in the air at the top of a jump, 2 in front of
the 0→1 ledge, 3 on the level-2 platform's front edge, 4 in the corridor, 5 below the
unreachable 0→2 ledge, 6 above the corridor, misaligned (a corner to round).
