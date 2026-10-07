---
name: ti-port-md
description: "Study a Mega Drive / Genesis ROM and port the user's chosen scope to the TI-89 in portable C: headless original traces, RAM/VDP extraction, grilling, milestone decisions, native graphics and measured TI costs. Use for Mega Drive-to-TI ports or studies, including ROMs in roms/md/."
---

# Mega Drive to TI-89: measured behavior, small milestones

Use the Game Boy big-game approach: study the running ROM, implement our own
engine on the Portable Game Runtime, then add the original graphics. Read
`../ti-port-gb/reference/big-game.md` for the behavioral-study method and
`../ti89-c-dev/SKILL.md` before non-trivial C. `games/sonic/` is the working
example; its local ROM and generated data are not distributed.
Its GHZ1 section and screen count are project choices, not skill defaults.

## Decisions before implementation

The user chooses the scope: a mechanic, room, section, level or full game.
Do not prescribe level one, five screens or another fixed limit. Divide the
chosen scope into measured, playable milestones without changing that scope.

Apply `/grilling` to choices that affect this port. Distinguish confirmed
answers from provisional assumptions, and ask concise, concrete questions
about unresolved choices before dependent implementation. Reuse confirmed
answers instead of repeating questions; skip grilling only when the relevant
answers are already established, and document why. Continue independent
reference work while choices are pending. Establish:

- The playable slice and its endpoint; whether "screens" means original or
  calculator views. Start directly at gameplay if menus are outside the slice.
- Fidelity of feel/rules, scale, ROM graphics versus redrawing, and contrast.
- The next mechanic, camera/HUD, input mapping and failure/reset behavior.
- What may be simplified (bridge flexion, backgrounds, sound, optional routes).

Write `ROADMAP.md`, with a playable acceptance criterion for each milestone,
and keep findings in `RE_NOTES.md` as OBSERVED / INTERPRETATION / TARGET.
Complete the requested milestone before studying further content. Do not
silently expand or reduce the user's requested scope.

Both machines use a 68000. This helps inspect arithmetic and may make reuse of
a genuinely isolated routine simpler than rewriting it. It does not relocate
absolute RAM references, provide a VDP, or reproduce interrupts, sound and
stack-dependent calling behavior on AMS. Compare the actual dependencies and
costs before choosing reuse. New ASM still needs the user's agreement; verified
shared primitives remain governed by the project exception.

## Original as a repeatable instrument

`tools/md/README.md` describes the local pinned Genesis Plus GX core and
`mdrun.py`: scripted buttons, states, normalized CPU RAM, traces, pokes,
images and VDP snapshots. `make -C tools/md core check` builds and validates
it against the local Sonic REV00 ROM. That integration check is Sonic-specific;
add equivalent boot/replay checks when studying another game.

Record the ROM hash, core revision/hash, boot script and sampled frame index.
Regenerate the direct-play state from the boot script, rather than relying on
an undocumented manual save. Confirm candidate variables through pokes. Clear
irrelevant objects and use controlled ground when measuring one actor; preserve
the original's camera/activation conditions and keep the observer alive.
Compare positions, fractions, velocities and state transitions over timelines,
not just the final position. Community disassemblies are reading aids: revision
options or bug fixes must not replace observed local-ROM behavior.

The pinned little-endian core stores RAM/VRAM bytes swapped within words.
Use the runner's normalized APIs. `MD.vdp()` needs our local exposure patch:
ID3 is VRAM, 0x10000 core CRAM, 0x10001 VDP registers. CRAM retains the core's
internal packed representation; identify the game's CPU palette when useful.
For Sonic addresses and extraction details, read [reference/sonic.md](reference/sonic.md).

## Runtime engine, then ROM art

Keep original fixed-point units for faithful small calculations when convenient;
scale positions and graphics together only at rendering. Choose logic/drawing
rates from measurements (Sonic: two logic steps per ~30.1 fps draw). Use scenario
doors and per-field state hashes; raw-struct hashes are not portable across
the PC and 16-bit TI ABI.

Extract only needed terrain and placements. Original decompression into RAM
can provide chunks, block maps and patterns without reimplementing every
decoder. Validate ROM offsets with a hash and original-video samples. A
source screenshot alone cannot recover hidden terrain or collision profiles.

Generate both host-order and big-endian banks. Do scaling, mirroring, contrast,
mask dilation and pre-shifting offline. Keep main actors readable with a white
outline; use light scenery behind dark characters. Archive data files and read
them in place through `rt_file`; bounds validation must allow the TI OTH
extension/tag trailer. Use source priority/parallax only when required by the
slice and within the measured budget; document any flattening explicitly.

## Acceptance and handover

Run PC mechanic tests, keyed headless traversals, TI scene checks (`xcheck`)
and per-frame state comparisons before TiEmu. When optimizing rendering,
compare every frame's plane checksum with the straightforward renderer.
Measure each compiled TI frame through `ti-cycles` prefix differences, including
cold TileMap creation and particle bursts; an average can hide an over-budget
first frame. Inspect hot-path assembly for 32-bit arithmetic/float helpers.
TI's 16-bit signed/unsigned promotions differ from the PC: assign camera
differences to signed locals before signed bounds tests, and cross-check the TI.

Use `../ti89-emulator/SKILL.md` for the final Titanium run, loading all code/data
together through `ti-run`. A requested GIF is a real TiEmu capture while
`ti-play` supplies held/diagonal keys. Give its final hold an explicit release
event: the tool releases all keys when the script ends. Observe the run before
claiming a win; wall-clock replay can diverge from headless frame timing.
After handover, the keyboard belongs to the user until they ask us to play or
restart again. Commit through `../ti-commit/SKILL.md` when requested.
