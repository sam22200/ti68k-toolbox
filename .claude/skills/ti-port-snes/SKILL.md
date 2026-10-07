---
name: ti-port-snes
description: "Study a local SNES ROM and port the user's chosen scope to the TI-89 in portable C: scripted headless reference runs, WRAM/cartridge RAM and PPU extraction, measured mechanics, grilling, milestones and TI cycle checks. Use for SNES-to-TI ports, including ROMs in roms/snes/."
---

# SNES to TI-89: measured behavior, small milestones

Keep the flow of `../ti-port-md/SKILL.md`: run the original as a repeatable
instrument, build our own engine on the Portable Game Runtime, and add ROM
art after the mechanics pass. Read `../ti-port-gb/reference/big-game.md` for
behavioral study and `../ti89-c-dev/SKILL.md` before non-trivial C.
`games/yoshi/` records the first experiment. Its level 1-1 and five original
screen widths were requested for that project; they are not skill defaults.
`games/megamanx/` records the second: an opening Highway section, running,
jump/wall kick, charge tiers, roller combat and original animations. Its
endpoint and initial equipment are project choices, not defaults either.

## Bound the next milestone

The user chooses the scope: a mechanic, room, section, level or full game.
Do not prescribe level one, five screens or any other fixed limit. Break the
chosen scope into measured, playable milestones without changing that scope.

Reuse the user's confirmed choices. Before implementation, distinguish those
answers from provisional assumptions. Ask concise, concrete grilling questions
for unresolved choices affecting the next milestone: level and endpoint, original versus TI
screen widths, scale, controls, camera, failure/reset behavior, required
interactions and acceptable simplifications. Do independent reference work
while choices are pending. Skip questions only when their answers are already
established; document why. Do not silently treat assumptions as confirmed
answers, broaden the requested scope, or count a tutorial as the chosen level.

Write `ROADMAP.md` before C, with playable acceptance criteria. Label findings
in `RE_NOTES.md` as OBSERVED / INTERPRETATION / TARGET. Mark tentative addresses
and rules until experiments confirm them. Menus, sound, later levels and
unneeded effects stay outside the chosen slice.

The SNES CPU is a banked 65C816, not a 68000. Study arithmetic widths, carry,
signedness and fixed-point units; reproduce required behavior in C rather
than relocating original instructions. Super FX, SA-1 and other cartridge
chips are reference dependencies, not proposed calculator emulators. New
68000 assembly still requires the project approval; existing verified shared
primitives follow the project exception.

## Original as a repeatable instrument

Read `tools/snes/README.md`. `make -C tools/snes core` builds pinned Snes9x;
`snesrun.py` provides buttons, states, traces, pokes, WRAM dumps, video and
PPU/cartridge RAM snapshots without a UI. `make -C tools/snes check` validates
the local European Yoshi's Island ROM; add an equivalent integration check
for other games instead of assuming its boot script or RAM map applies.

Record ROM SHA-256, copier-header status, region, measured frame rate, core
revision/binary hash, options, boot input and post-frame sample index.
Regenerate the gameplay state from a cold boot with deterministic inputs.
Save/reload must reproduce RAM, cartridge workspace and video on every frame.
Disable optional overclocking, fast-ROM and timing hacks for reference runs.
PAL and NTSC mechanics must be measured on the actual local revision.
An unpopulated cartridge RAM region is valid: Mega Man X has no exported
SRAM, whereas Yoshi uses it as Super FX workspace. Build snapshot checks
from the regions actually present rather than requiring every export.

WRAM is 128 KiB at `7E0000..7FFFFF`, little-endian; low-bank mirrors are not
additional RAM. Cartridge workspace can contain player state (Yoshi uses
`700000` RAM), so WRAM alone is insufficient. Respect bank mapping and exported
sizes; do not apply Mega Drive word-byte swapping to SNES memory. Community
maps/disassemblies are reading aids; confirm addresses on the local ROM by
controlled input and pokes, especially when their revision is US and ours PAL.

Measure one mechanic at a time: clear interfering actors when needed, park
the camera on controlled terrain, sample position, fractions, speed and
state transitions over complete timelines. Keep original activation/lag
conditions when they affect the observed rule. Distinguish displayed frames,
game logic updates and lag frames before choosing the target update rate.

After isolated mechanics pass, measure their combinations: firing while
running, jumping, turning and touching a wall, including same-frame input
edges. A projectile's formation may follow only one coordinate and preserve
its original launch direction; its muzzle can depend on the current pose.
Probe emission on the exact update leaving a run: the cannon can retain the
running origin after motion has entered jump or fall. Measure each charge
tier separately; their vertical offsets need not select the same pose.
Keep source-active trajectories in the comparison, disclose viewport/pose
adaptations explicitly, and retain the isolated exact oracle separately.
Probe both launch directions and the last active offscreen updates. Stored
formation velocity need not encode launch side; unsigned source coordinates
may represent negative positions. Measure removal before/after integration
and kind-specific margins instead of treating every wrapped coordinate as
an invalid object.

After isolated and coupled probes, replay the entire chosen path from its
normal spawn through the endpoint. Include recovery from a failed jump.
Validate a candidate source logic clock against independent unchanged actor,
charge and projectile fields before filtering lag frames. Preserve input
edges and disclose any omitted source actors; a natural no-write control can
show how they change the path. Test support loss, the last allowed jump and
the first wall clamp independently. Collision extents, contact sensors and
the update which enters a fall need not use the same width or ordering.
Include direction release plus jump, stopping while unsupported and reversal
on that update. Selecting idle before jump/support checks can ignore a valid
jump or leave an unsupported actor suspended. Compare visible orientation
with the source; a native wall-side sprite-bank selector is not necessarily
the same semantic field as the source facing byte.

Measure damage together with held actions: a recoil state may block firing
while charging still progresses, and its last update can consume a press
without emitting a shot. Check both projectile births and suppressed births,
not only trajectories of shots which already exist. Test held jump versus
release/repress when control returns. A value retained in a free actor slot
is not necessarily an initialization constant: vary its previous contents
independently before interpreting formation VX or unused fractional carry.
Object-pool size need not equal the gameplay projectile limit. Measure
repeated presses, charge after repeated taps and release with all allowed
slots occupied; capture whether failed emission consumes or defers charge.
If target viewport culling changes later slot availability, compare the
common allocation prefix explicitly and keep the camera adaptation visible.

Probe neighboring animation phases and repeated firing around a wall jump.
Horizontal velocity need not determine gun direction: X can move away while
firing toward the wall. Keep wall-pose age separate from a firing animation
restart. Measure the projectile origin before/after horizontal collision
clamping; an apparent one-pixel offset can come from the intended movement.
At a forced transition, charge consumption and projectile allocation can
occur on different updates. Compare both, including a normal press that
starts charging without producing a pellet. Retained normal-shot fractions
can affect later motion; do not discard them as unused charged-shot residue.
An unused field for one projectile kind can become meaningful after reuse
by another kind. Copy inactive slot history into a scenario when it affects
the next action, and distinguish it from the target's camera adaptations.
If wider target culling changes slot reuse, identify that exact history and
fraction difference rather than relaxing all trajectory comparisons.

For enemy impacts, capture the whole actor and its reaction state, not only
HP. A hit may switch to a finite death sequence without another HP decrease;
a persistent charged shot can strike armor and body on successive updates.
Separate gameplay immunity from a visual damage flag. Vary an apparent
threshold independently (velocity, distance, timer) before accepting a rule
that merely fits one natural timeline. Hitboxes can change with animation;
validate source descriptors by controlled probes and generate small lookup
tables for the target rather than decode them or use modulo in a hot loop.

## Terrain and graphics

Extract only the chosen band plus camera margin. Use decompressed in-game
maps when simpler than rebuilding every ROM decoder. Video cannot establish
hidden collision geometry: compare tile semantics with placement/landing
pokes. For Yoshi-specific addresses and initial results, read
[reference/yoshi.md](reference/yoshi.md).
For Mega Man X's LoROM scene hierarchy, buster and wall-kick measurements,
read [reference/megamanx.md](reference/megamanx.md).

The runner exports VRAM, native-word CGRAM, OAM and raw PPU register mirrors.
These mirrors do not capture all latched scroll values or mid-frame HDMA.
Obtain needed scroll/collision variables from game memory or explicit core
instrumentation. Decode 2/4/8-bpp planar tiles, tilemap flip/palette/priority
and OAM size bits offline. Confirm extraction against the original video.
Mode 7, color math, mosaic, windows and Super FX rendering can be flattened
or omitted only when the milestone allows it; document the visual change.
`tools/snes/ppu.py` supplies reusable offline planar/OAM and Mode1 background
decoding. Other background modes still need explicit support.

Measure RAM-to-OAM timing before choosing sprite anchors. Mega Man X's
post-frame RAM position is ahead of the drawn object: saving its screen
anchor before the update removes artificial shifted-pose duplicates. Confirm
the chosen anchor over moving/jumping frames against opaque original pixels.
Keep charge formation and projectile flight in separate animation spans;
their timing/state transition can differ even when initial velocity matches.

When comparing an isolated actor with original video, resolve OBJ ownership:
another palette may contain a gun flash, wall dust or charge spark covering
the actor. Verify excluded pixels against the covering OBJ in the video;
do not hide unexplained extraction errors. Keep extraction fidelity distinct
from native pose cadence and simplified layer priority.

Scale geometry and art together at rendering. Generate native host-order and
big-endian TI banks; reduce palettes, mirror, dilate actor masks for white
outlines and pre-shift offline. Main actors must read against light scenery
on 160×100 in four greys. ROMs, third-party code, states, captures and extracted
assets remain ignored; commit only our tooling, engine and measured notes.

## Runtime and acceptance

Use scenario doors and explicit field hashes (raw structs differ on TI's
16-bit ABI). Keep small arithmetic and measured fixed-point units; lookup
tables, shifts, pointers, ExtGraph TileMap and pre-shifted sprites take
precedence over expensive runtime computation. Read archived banks in place
through `rt_file`, allowing the TI OTH trailer during size validation.

Run mechanic tests against original traces, keyed headless traversals, PC/TI
screen checks and state hashes. Measure actual compiled TI frames with
`ti-cycles`, including cold rendering and worst actor/effect density; the
grayscale budget is about 360k cycles per 30 fps frame. A PAL scheduling
decision needs its own budget. Inspect generated assembly for 32-bit
multiply/divide and float helpers; optimize only measured expensive paths.

TiEmu comes last, once at a milestone, for hardware behavior only. Follow
`../ti89-emulator/SKILL.md`, clean Titanium load with all banks. After handover
the keyboard belongs to the user. Do not claim a playable port from successful
reference tests alone. Commit through `../ti-commit/SKILL.md` when requested.
