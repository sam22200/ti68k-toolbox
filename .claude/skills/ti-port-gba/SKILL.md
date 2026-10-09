---
name: ti-port-gba
description: "Study a local Game Boy Advance ROM and port the user's chosen scope to the TI-89 in portable C: scripted headless reference runs, EWRAM/IWRAM and video-memory extraction, measured mechanics, milestone decisions and TI cycle checks. Use for GBA-to-TI ports or behavioral studies, including ROMs in roms/gba/."
---

# Game Boy Advance to TI-89: measured behavior, small milestones

Follow the same behavioral approach as `../ti-port-snes/SKILL.md`: run the
original as a repeatable instrument, build our own engine on the Portable
Game Runtime, and add ROM art after the mechanics pass. Read
`../ti-port-gb/reference/big-game.md` for behavioral study and
`../ti89-c-dev/SKILL.md` and its required references before non-trivial C.
Read [reference/gba.md](reference/gba.md) when interpreting memory, ARM/Thumb
code or video snapshots.
`games/minish/` records a first game-specific native traversal: offline
forest assets, a reproducible original Minish Woods study door, measured
walking/partial collisions/slopes and a 720x320 opening on the runtime.
M2 checks original scenery, 44 outlined Link poses and canopy occlusion
against source RGB pixels and PC/TI screens; `minishz` provides an additional
70% view with the same source mechanics. M3 measures ordinary sword timing,
input-edge restarts, facing changes and three tile samples; 40 attack poses
and cutting all 53 bush cells pass original fixtures and PC/TI checks in both
views. M4 adds the two opening Octoroks, shots, sword kills, contact/shot damage,
recoil, hearts and retry. Twenty restored trials check608 targeted updates;
twenty enemy poses add960k oracle pixels per build. Current PC/TI checks pass
7521/7557 hashes and346/382 screens below360k cycles. Hearts have a one-pixel
white silhouette outline; native nearby targeting and larger round balls make
ordinary shots visible. Its enemy-enabled study
save sets an original room-loader flag separately from the earlier walking
door. Local AI, eight-way recoil, death/removal and rock impact/expiry are
native adaptations; original bouncing deflections, drops and other actions
remain separate milestones. Injected saves are not natural story progress.

## Bound the next milestone

The user chooses the game and scope: mechanic, room, battle, section, level
or full game. Break that scope into measured, playable milestones without
replacing it with a smaller project. An unfinished game name is a missing
choice: ask for it while preparing independent tooling. Do not interpret a
successful title-screen replay as a tested gameplay model or a playable port.

Reuse established choices. Ask concise grilling questions only for unresolved
decisions affecting the next milestone: endpoint, source versus TI view widths,
scale/crop, controls (including L/R), camera, failure/reset, required
interactions, and acceptable simplifications. Keep provisional assumptions
distinct from confirmed answers; proceed with independent measurements while
answers are pending. If the user has already settled the choices, record them
and proceed without repeating questions.

Write `ROADMAP.md` before game C, with playable acceptance criteria, and label
`RE_NOTES.md` findings OBSERVED / INTERPRETATION / TARGET. Keep menu, dialogue,
sound and later content within the chosen scope. A tactical game can require
turn resolution, movement ranges, combat, RNG and AI rather than platformer
physics; measure the requested game instead of imposing a platformer checklist.

GBA's ARM7TDMI executes ARM and Thumb, not 68000 instructions. Recover the
needed arithmetic and update ordering and implement them in portable C.
Keep BIOS services, DMA, interrupts and cartridge peripherals in the PC
reference; they are not a proposal to emulate a GBA on the calculator.
New 68000 ASM still requires project approval; verified shared primitives
follow the project exception.

## Original as a repeatable instrument

Read `tools/gba/README.md`. `make -C tools/gba core` builds pinned mGBA;
`gbarun.py` provides scripts, provenance-checked states, traces, RAM pokes,
memory snapshots and RGB captures without a UI. `make -C tools/gba check`
checks local-ROM boot, native keypad, save restoration and per-frame replay.
This is an instrumentation check. Add a game-specific gameplay door and
mechanic checks for the user's game; the two START pulses in the generic
check are not a universal route into gameplay.

Record ROM SHA-256/header identity, core revision/binary hash, options, BIOS
choice, frame rate, cold-boot input and post-frame sample convention. Default
reference runs use mGBA HLE BIOS, no external saves, no frameskip and no idle
loop removal. If the original needs a real BIOS, RTC, link, sensors or another
peripheral, add and validate explicit reference support before relying on it.
Do not silently claim the current runner covers those dependencies.

Regenerate gameplay states from cold boot with deterministic inputs. Verify
EWRAM, IWRAM, cartridge saves, VRAM, palette, OAM, I/O mirrors and RGB video
on every restored frame, including a fresh core load. The runner's state
container restores cartridge save bytes separately: bare libretro state
reload does not do that. RTC is not frozen by this runner. Per-frame equality
on the actual trials is required before using a game as an oracle.

Use canonical little-endian memory ranges; read both EWRAM and IWRAM. The
runner rejects mirrors and provides raw memory, not emulated bus semantics.
I/O snapshots do not prove raster effects or complete hidden renderer state.
RAM pokes establish causal candidates, but poking an I/O/VRAM pointer bypasses
hardware side effects and is intentionally unsupported. Community maps and
decompilations are reading aids; confirm addresses on the local revision.

Measure one rule at a time on controlled terrain or a controlled turn: full
positions/fractions, velocities, flags, counters, actor slots and transitions.
Keep the source camera and activation conditions when they affect the rule.
Separate rendered frames, actual logic updates and lag frames. Confirm a
candidate logic clock against independent actor fields before filtering it.
For turn-based rules, include input edges, cursor selection, movement costs,
blocked routes, action legality, damage rounding, RNG consumption, end-turn
effects and save/reset behavior when required by the milestone.

Then test combinations and replay the chosen path from normal spawn through
its endpoint, including failure and recovery. Include boundary timing (support
loss, last valid jump, simultaneous attack/turn, last action before end turn),
allocation exhaustion and slot reuse when relevant. Compare complete timelines
and suppressed actions, not only final positions or successful actions.
Distinguish original rules from viewport, update-rate and AI adaptations.

## Terrain and graphics

Extract only the needed maps/placements plus camera margin. Prefer measured
decompressed in-game data when ROM codecs are not yet understood. A screenshot
cannot establish hidden collision, movement costs or height. Confirm terrain
semantics through controlled placements and traversals. Decode GBA packed
4/8-bpp tiles and OAM offline; SNES planar decoders are not interchangeable.
Check source extraction against original RGB pixels before reducing palettes.

Determine how post-frame game coordinates relate to the OAM that produced the
image. An actor can draw using pre-update coordinates. Measure anchors over
moving frames, attribute overlapping OBJ ownership, and explain excluded
pixels rather than hiding extraction mismatches. Text backgrounds, affine
backgrounds/OBJ, bitmap modes, window/blend state and raster DMA need explicit
support or a documented allowed visual adaptation.

GBA is 240×160; TI is 160×100. Resolve crop/scale with the user: scaling by
two thirds gives 160×approximately107, so it cannot show the whole image
without vertical adaptation. Do not silently crop required HUD/dialogue.
Scale geometry and art consistently at rendering. Generate host-order and
big-endian TI banks; reduce palettes, mirror, dilate masks for white outlines
and pre-shift offline. Main actors and essential UI must read at a glance in
four greys; important details need at least two pixels. Keep ROMs, upstream
sources, states, extracted art and trace fixtures in ignored local paths.

For reduced views, resize unoutlined source pixels before dilating the final
silhouette: shrinking an existing outline can erase the white halo. Preserve
the existing build when the user requests an additional scale variant.

## Runtime and acceptance

Use portable C against `runtime/core/rt.h`, scenario doors and explicit field
hashes. ARM's 32-bit ABI is not the TI's 16-bit ABI: preserve measured value
ranges and signedness, fixed-point fractions, arithmetic shifts and overflow
deliberately. Avoid undefined signed overflow. Do not translate every source
32-bit field into a runtime `long`; use small storage and short computations
where measured ranges allow them. Precompute expensive tables and do codecs,
scaling, palette conversion and sprite preparation offline. Use existing
ExtGraph/TileMap/pre-shifted primitives and archived banks through `rt_file`.

Run PC mechanic tests against original traces, keyed headless traversals,
PC/TI screens and per-field state hashes. Choose source logic and TI drawing
rates from measurements; two source updates per draw are an option, not a
default for every game. Measure compiled TI frames with `ti-cycles`, including
cold rendering and the worst actor/effect density, against about 360k cycles
per 30 fps grayscale frame. Inspect hot-path assembly for float, 32-bit
multiply/divide and compiler helper calls. Optimize measured hot paths.

TiEmu comes last, once at a hardware milestone. Follow
`../ti89-emulator/SKILL.md`, clean Titanium boot with every bank; after handover
the keyboard belongs to the user. Commit through `../ti-commit/SKILL.md` when
requested. Report reference validation and native gameplay validation
separately, with remaining gaps stated plainly.
