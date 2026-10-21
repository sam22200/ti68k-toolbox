# Orbonne and Gariland: separate steady rendering from UI creation

2026-10-19. Actual native68000 execution, MC68000 datasheet timing at nominal
12MHz. No TiEmu or physical-hardware FPS claim. Grayscale interrupts, fixed
frame waits, packed launch and real AMS overhead are excluded.

Run `tools/pyenv/bin/python games/fft/tools/profile_maps.py` from the repository
root after generating local chapter assets. Each map uses its actual direct
Ramza fixture, player-centered camera and all four orientations. Each run lasts
120 updates; the steady-frame mean uses updates40–119. Different maps do not
have identical scenery/coverage/actors, so this is a workload comparison, not
a controlled renderer speedup trial. PC/native final pixels match in all8 runs.

| View | Orbonne warm CPU/frame | Gariland warm CPU/frame |
|---|---:|---:|
| Orientation0 |23.23ms|15.21ms|
| Orientation1 |17.52ms|18.38ms|
| Orientation2 |19.73ms|16.35ms|
| Orientation3 |22.61ms|21.50ms|

The default views differ: Orbonne starts in orientation2, Gariland in0. Their
means are therefore19.73ms versus15.21ms, about30% more CPU at Orbonne's initial
view. The four-view means are20.77ms versus17.86ms (about16% more; camera matters).
Both initial steady averages remain inside the30Hz CPU budget, but UI creation,
turns, cold poses and the grayscale runtime add other costs/occasional stalls.
Do not invert CPU times into displayed FPS.

## Main difference: actor storage

Gariland reads ready-to-draw poses and mirrored variants directly from its
archived actor bank. Orbonne compresses and deduplicates canonical poses to save
storage. Its per-actor decoded slot must be refreshed as the pose changes;
mirrored records also reverse each row at runtime. The fair queue budgets one
ordinary miss per update and can copy a matching pose from another actor.
This is an existing archive-space/CPU tradeoff, not perspective preparation.

In the initial views, zone4 (actor draw/coverage work, including decode calls)
averages128,169 cycles on Orbonne and66,745 on Gariland over the complete120
updates. Orbonne's decode/mirror zone20 averages39,892 cycles per miss (3.32ms),
with131 calls including cold preparation; Gariland has no corresponding decode.
These zones overlap; do not add them. Some of the remaining difference comes
from each map's actor coverage, draw positions and visible work. Even the
bounding-box actor count is lower in Orbonne's initial view (7 versus8), so
simply claiming it has more visible actors would be incorrect.

The scene-copy/water zone3 is cheaper on Orbonne's initial view:40,645 versus
47,571 cycles. Its192-row decoded scene is also smaller than Gariland's218 rows.
Static geometric relief edges were drawn offline and add no live edge pass.
The Orbonne build contains no idle perspective prefetch.

Move is another operation: the four-camera creation comparison measured
optimized filled Orbonne at105.71ms versus Gariland123.15ms, with contour86.62ms
and87.67ms respectively. Those entry events include composing the
range; they are not steady-frame times. See `REACH_CONTOUR.md` for current controls.

## Source-facing correction

The source event-05 snapshot has Ramza's logical unit-facing byte at unit+0x49
set to0 (-y, our-z), while its scene actor angle at scene+0x70 still reads3072
(+x). At the original first Ramza turn both agree on0. The importer previously
used the transient scene angle. It now maps the logical facing for every unit:
Ramza changes from world+x to-z, with all positions and shared handedness intact.
The original snapshot's other ten imported facings already agree.

`opening/regressions.c` checks world face3 and all four projected facings.
`opening_assets.py DISC --facings-only` updates existing local unit records and
metadata without regenerating scenery. `make -C games/fft/opening check` then
checks16 native/PC story, battle, turn and settings checkpoints.

Local outputs: `campaign/x/map-costs/profile.json`, eight native/PC screenshots
and zone logs. The current packed campaign is364,712 bytes/23 variables, with
selectable contour and the corrected facing. It was not relaunched in TiEmu;
the user's existing emulator session remains on the earlier package.


## Title and shared-asset audit (2026-10-19)

Offline storage measurements; no shipping format or runtime changed here.
The756-byte strip is a complete place name (including all words), not a glyph
or a per-word charge. Independently ZX0-packed and round-trip checked, Orbonne's
strip is352 bytes and Gariland's410 bytes. All113 distinct nonempty source strips
at the existing144x14 conversion occupy85,428 raw bytes or40,527 individually
compressed bytes (101–496 per strip), before per-title indices/container headers.
Only required chapter titles would ship; these are whole-corpus controls.
Exact shared vertical chunks of1/2/4/8 pixels across the113 strips need46,848 /
56,340 /62,190 /68,160 bytes including raw chunk indices/dictionary, so these
particular uncompressed dictionary controls lose to individual ZX0. This does
not disprove a future glyph/font system; original title strips are artwork and
whole-title fitting can change individual glyphs. No replacement alphabet was
implemented or visually validated. A per-title decode could occur once at scene
entry, separately from combat pose decoding; native cost remains unmeasured.

The45,438-byte compressed Orbonne actor bank contains characters, action/scene
poses and loot, not scenery materials. Its four original compressed view streams
are38,681 bytes, compared with38,848 for campaign Gariland, excluding each first
bank's appended shared-render/contour tail. Each view stores projected monochrome
scenery plus depth/cover layers. There is no TI texture atlas for wood/grass/etc.
`shared_floor_materials()` already takes Gariland's wood/stone UV artwork for the
corresponding flat Orbonne surfaces offline. Grass/water still come from Orbonne's
source art. Reusing source textures does not make their differently projected,
occluded/lit destination pixels and depth masks identical between maps.

An exact decoded-byte comparison also finds59 unique oriented actor/loot poses
in common with Gariland, corresponding to55 Orbonne canonical records up to a
horizontal flip. Their current compressed streams plus record-directory entries
occupy7,496 bytes: a gross reuse candidate, not a measured net saving or a shipped
shared bank. Both ordinary Chemist-M/Squire-M families have24 of44 exact slots in
common; the other opening families have no ordinary exact match. Ramza's opening
outfit differs from his Gariland one. Approximate art reuse and shadow conventions
need separate review. New bank indices/file slots and pose latency must be accounted
for before claiming the entire7,496-byte saving.

Local audit reports: `opening/x/storage-audit/reuse-title-audit.json` and
`all-titles.json`. Production binaries and the user's TiEmu were not modified.
