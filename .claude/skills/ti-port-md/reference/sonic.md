# Sonic REV00: verified starting point

Read for a Sonic study or an analogous MD extraction. These are findings for
the pinned ROM, not a generic format contract for other games.
GHZ1, half scale and the 4.8-view endpoint below are this project's agreed
choices; they do not prescribe a level or screen count for another port.

ROM: local `roms/md/Sonic_1.md`, 524,288 bytes, SHA256
`46160baa06362c711c9f1a5017cb7371026444936c8af5e93a78996cf32ff2a6`.
Core: Genesis Plus GX `58c341487e5bfcf979ea68413c7987633adb0c56`,
with the local read-only VDP exposure patch. Full provenance, commands and
physics/actor traces: `games/sonic/RE_NOTES.md` and its `tools/` directory.

## Direct-play door and units

`keys/ref_boot.txt` boots the original in 1,000 frames to GHZ1 mode0C, Sonic
(80,944), camera (0,768). The original reports ~59.923 fps. Target geometry is
half scale, with original 8.8 velocities and fractional position bytes. The
agreed stop is X1536 (4.8 original 320-pixel views); art extends to X1664.
The target starts in play and deliberately uses a rigid bridge.

Original acceleration/braking, jump release, three badnik timelines and recoil
were measured separately on controlled ground, then used as PC tests. The
first slice contains 14 rings, one Motobug, one Buzz Bomber and one Chopper.
The spring and upper route are not prerequisites for the verified lower route.
Do not infer general loop or wall-running support from these tests.

## Scenery and sprite extraction

- GHZ RAM layout at FFA400 has 128-byte interlaced rows. Chunk descriptions
  begin FF0000: each 256-pixel chunk has 16x16 block descriptors. FFB000 holds
  four pattern attributes per 16x16 block. Apply descriptor flips to the whole
  block as well as individual pattern flips; collision uses separate profiles.
- VDP patterns are 8x8, 4 bpp (32 bytes). Attributes select pattern, palette,
  X/Y flips and priority. Decode a sprite's multi-tile pieces column-major.
  Map pieces and loaded patterns can be obtained from an injected original
  actor instead of decoding unrelated graphics archives.
- CPU palette FFFB00 has 64 words in 0BGR, three-bit channels at bits1/5/9.
  Original-video validation must reproduce the core's normal-intensity
  four-bit/RGB565 conversion; linear 255/7 colors differ visibly from it.
- Sonic mappings: ROM 0x211E2; DPLC: 0x217FE; raw art: 0x21AFE. A mapping piece is five
  bytes (signed Y, size, attribute word, signed X). DPLC cues give count minus
  one in the high nibble and source tile index in the low 12 bits. Compose on
  an anchored canvas, then crop each target sprite with its own offset.
- `tools/check_art.py` checks 496 visible opaque Sonic pixels against original
  video, allowing one RGB565 decode unit. Foreground grass covering the shoes
  is excluded; native data-bank order, bounds and outlines are also checked.

## Measured target presentation

An opaque TileMap flattens foreground and a repeated initial plane-B template.
This is a static background adaptation, without runtime parallax or priority
compositing. Four-grey art uses offline 2x2 sampling; Sonic/badnik masks are
dilated one white target pixel. Rings/logs stay small and dark without an
exterior outline. The bank has 232 tiles and 148 sprites including mirrors;
source pose selection is simplified rather than frame-exact.

Masked ring descriptors are cached. For a dense spill fitting 32x32, compose
pre-shifted rows in the original paint order, then blit once. The four ordinary
ring poses have dark plane equal to opacity; the extractor verifies this
special-case invariant. Pre-shifts cost 6,400 archived bytes and replace
variable 32-bit shifts in the particle loop. Keep the last identical-position
ring when culling duplicates: intervening overlaps can repaint an earlier one.
An outlined actor is not an opaque rectangle, so the old diagnostic player's
occlusion shortcut cannot be reused. Full-render equivalence is tested on all
780 frames of play, damage and 32-ring stress.

Per-frame TI update+render peaks: play 323,092, ten-ring damage 314,400,
32-ring stress 353,666 cycles (cold first frame included), under the 360k budget.
Binary 16,201 bytes; terrain variable 23,142; art variable 54,280. These are this
milestone's measured results, not budgets to assume for another port.

TiEmu playback verified arrows, jump, scrolling, ring collection/loss and bridge
traversal in the local GIF `games/sonic/x/sonic-titanium.gif`. The headless winning
script ends at frame 245; the real-time demo diverges from that timing and is a
controls/presentation check, not proof of a completed TiEmu traversal.
