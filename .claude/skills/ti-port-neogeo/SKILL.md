---
name: ti-port-neogeo
description: "Study a local Neo Geo MVS/AES cartridge ROM set and port the user's chosen scope to TI-89 in portable C: headless original traces, measured mechanics, offline sprite extraction, milestones and TI cycle checks. Use for Neo Geo cartridge-to-TI ports and ROM studies, including roms/neogeo/."
---

# Neo Geo to TI-89: measured behavior, small milestones

Follow `../ti-port-snes/SKILL.md`'s method: make the original a repeatable
instrument, build our own engine on the Portable Game Runtime, then add ROM
art. Read `../ti-port-gb/reference/big-game.md` for behavioral study and
`../ti89-c-dev/SKILL.md` before non-trivial C. Neo Geo CD and Neo Geo Pocket
need different media/CPU tooling; this skill covers MVS/AES cartridges.

Windjammers is the first local preparation case. Read
[reference/windjammers.md](reference/windjammers.md) when using that set.
Its ROM identity, program byte order, original cold boot and deterministic
reference replay are checked. A two-human Beach serve door, both characters'
walking, ordinary throws, swept wall contacts, neutral contact boxes and
Beach points/next serve have measured original timelines and replay checks.
`games/windjammers/` now supplies a first native Beach training engine,
with PC/TI state/screen checks and measured frame costs. Read its README
and roadmap before extending it: neutral mechanics and six complete ordinary
throw/catch/settled-hold timelines are checked against the ROM. Actor tiles,
palettes/flips/anchors are validated against original RGB scenes and drawn
as outlined grey sprites with native playback. M2b1 adds measured automatic possession releases and delayed ordinary return
strength, checked through 4058 steps across 29 normal-input trials. M2b2 adds
stationary timed lift/complete charge/recapture, capture-interrupting powerful
returns and high-speed catches: 6704 equality steps, 78 no-write trials.
M3a adds lobs, charged character specials, airborne ground rebounds and
counter windows: 8573 bounded steps from 62 twice-replayed no-write trials.
Read each trial's coverage interval: normal lob target jitter is conditioned;
failed-preparation block flight, goal celebration and special-lob pose subtype
remain adapted. M3b adds directional gestures, curved flights/wall transitions
and immediate/settled returns: 17764 steps from 180 twice-replayed no-write
trials. M3c adds dash motion (48 twice-replayed no-write trials, 816 comparison
steps), explicit charge guidance, a larger disc and fixed lob targets. Its
29 native scenarios pass 7380 PC/TI hashes and 30 final screens. Read the
notes for adapted horizontal-wall recovery and moving-pose contacts. Moving/other action-pose
contacts, rear flight, serve animation and AI remain separate.
Creating this skill does not select a playable port scope.

## Bound the next milestone

The user chooses a mechanic, court, match, section or full game. Keep that
scope and break it into playable milestones. Reuse confirmed choices; ask
concise grilling questions only about unresolved decisions affecting the
next implementation, while continuing independent ROM/reference work.

For a court game, establish character/opponent and arena, human versus AI or
two-player play, controls, court framing and scale, rules/timer/reset,
required actions and acceptable visual simplifications. A full court at
half scale can exceed the LCD's height; measure the playable geometry before
choosing a crop, HUD or different scale. Do not adopt Windjammers choices as
defaults for other games or assume calculator link play was requested.

Write `ROADMAP.md` with playable acceptance criteria before C. Label findings
in `RE_NOTES.md` as OBSERVED / INTERPRETATION / TARGET, identifying whether
evidence came from file inspection, emulator source or a running-ROM probe.
Record provisional decisions as assumptions. Menus, sound and additional
content are included only when the chosen scope needs them.

Both systems use a 68000, but a cartridge depends on its BIOS, memory map,
video hardware, interrupts and Z80/YM2610 sound system. Rebuild behavior in
portable C. Disassembly is evidence, not permission to relocate game code
into AMS. New assembly still requires the user's agreement; existing
verified shared primitives follow the project exception.

## Identify the set before running it

A cartridge is several P/S/C/M/V files, not one linear ROM. Inventory each
file's size, CRC32 and SHA-256; check against the selected emulator driver's
exact set definition. Account for renamed chips, parents and BIOS files.
Use `tools/neogeo/romset.py` for the local Windjammers set; see
`tools/neogeo/README.md` for checked audit, canonical ZIP and program export.
That helper's set manifest is Windjammers-specific.

Keep the original chip files untouched. P-ROM dump byte order and emulator
RAM storage order are separate questions. Our local Windjammers P1 needs
adjacent-byte swapping for a big-endian CPU image; verify the header and
vectors on other sets, and respect their loading, banking and decryption.
Do not apply that swap to S/C graphics or Z80 code. Ghidra imports the
normalized CPU image as 68000 big-endian; record the mapping and distinguish
BIOS vectors from cartridge entry points.

## Original as a repeatable instrument

Use a pinned PC reference core, preferably FBNeo/libretro, without a UI.
Read `tools/neogeo/README.md`. `make -C tools/neogeo core` builds the pinned
FBNeo Neo Geo subset; `neogeorun.py` supplies two-player scripts, states,
normalized work RAM, traces/pokes and video/palette/status snapshots.
`make -C tools/neogeo check core-check` validates ROM/frontend and compiled
core preparation without a BIOS. `make -C tools/neogeo reference` passes
with the local Windjammers set and `roms/neogeo/neogeo.zip`: cold boot,
native input mappings, CPU-bus pokes and 120 exact RAM/video replay frames.
Use `BIOS=/path/neogeo.zip` for another local archive and validate its actual
selection. This integration reaches the game's intro. The separate
`make -C tools/neogeo gameplay` checks Windjammers' first-service door,
walking/release/reversal/clamps and ordinary disc trajectories. Its labelled
diagnostic injections remain separate from the no-write trials.
`make -C tools/neogeo rules` checks exact ordinary wall contact positions,
pose-dependent overlap/catch classification, both goal boundaries and BCD
point zones, then two missed-shot/loser-service sequences. It uses original
poses/collision angles when checking contact descriptors; a complete native
animation model and full match ending remain separate work. The native
fixture generator independently checks the neutral collision-angle model;
native `make validate` compares per-field state hashes and screens on PC/TI,
and `make profile` checks individual TI frames against the game cycle budget.
For the native Windjammers extension, `tools/measure_actions.py` records and
fully replays eight normal-input action trials. Six ordinary manual trials
feed 660 complete native action steps; ordinary possession is extended by
`tools/measure_holds.py`: 29 no-write trials, 7140 deterministic replay frames
and 4058 native equality steps, including two 600-step automatic rallies.
Those possession fixtures compare age only during possession actions; the
same word has different nonholder uses. `tools/measure_timing.py` adds 78
no-write trials, 10920 replay frames and 6704 native equality steps. Its ready
word packs an age and timing-window flag; compare the complete word while
ready/charging rather than treating it as ordinary hold age. Preserve the
declared coverage bounds when extending equality coverage. M3a's
`tools/measure_advanced.py` checks complete charge/release flights, strong
receptions and counter windows. Confirm the selected arena index before
choosing airborne bounds/target tables: the checked Beach is arena1, whose
LOB rows/bounds differ from arena0. The B control is libretro bit8 (TI K_B),
not bit1. Native sprite words now live in the required archived wjart bank,
with unchanged validated pixels and separate host/TI byte orders.
`tools/measure_curves.py` sweeps gesture segment timing and both arc signs on
both sides; it checks curved wall conversion, captures and manual returns.
The ROM recognizer reads only nine recent direction samples, with neutral
encoded like Up. Its strength threshold is checked after power normalization;
curve release timing comes from separate animation event tables. Save that
history and retained facing, and distinguish the original contact normal
(upper wall128, lower wall0) from a wall-position label. Dynamic curves become
straight arbitrary-angle flight on rebound, requiring dynamic ordinary recoil.
Read the native notes for windows and history/bonus normalization.
`tools/measure_dash.py` checks both characters/eight directions and three A
durations, including signed component decay and fraction-preserving clamps.
Keep its bounded motion interval separate from wall animation/recovery and
moving-pose contact fidelity. The native renderer guides stationary precision
reception with a pre-contact cue; test that cue through a complete charged
special rather than merely checking the bar pixels. `make showcase` captures
real LCD frames with the game's name and control captions outside the court.
`make art-check` validates original
actor colors and sprite depth across actions and all walk directions.

Record core revision/binary hash, chip and BIOS hashes, actual loaded BIOS,
MVS/AES mode, region, DIP settings, NVRAM/memory-card initialization, core
options, reported frame rate and sampling convention. Avoid optional
overclocking, frameskip and timing hacks. Our runner explicitly sets the
initial RTC calendar to 2000-01-01 Saturday 00:00:00, then lets emulated ticks
advance normally; record this choice because host local time otherwise changes
BIOS RAM/NVRAM between cold boots. Use a separate process for each cold boot;
the pinned core cannot safely reinitialize a loaded driver after unloading.
Reproduce the gameplay door from
cold boot with documented coin/start/select inputs. Keep two controllers
independent; confirm the mapping of Neo Geo A/B/C/D, coin and start from
the selected core's descriptors rather than assuming SNES button names.

Before using traces as an oracle, verify save/load by replaying the same
inputs and comparing every frame's work RAM, required video state and RGB
output. State load does not restore frontend held keys. Validate live input,
byte/word pokes and bounds. A serialized state is opaque and revision-bound;
do not infer a stable RAM offset from its file layout.

The ordinary cartridge work-RAM window is `100000..10FFFF`; mirrors are not
additional RAM. Our pinned Linux runner swaps adjacent bytes in work RAM,
graphics RAM and palettes, exporting CPU-order big-endian words. The local
Windjammers integration verifies work-RAM byte/word/long accesses against
the real CPU bus. Validate candidate
fields with controlled input and pokes; neither driver source nor a community
RAM map proves Windjammers' player/disc addresses. Our core patch exports
graphics RAM, both palette banks and selected bank/animation/FIX/brightness
state. These are post-frame snapshots; raster changes need further instrumentation.

Measure isolated movement/actions, then their combinations and a normal
playable sequence. Capture positions, fractions, speeds, states, timers and
input edges across complete timelines. Separate render frames, logic updates,
hit-stop and lag before selecting the TI schedule. Measure both players and
directions. Disclose injections that suppress AI or reposition objects, and
retain an ordinary no-write control for the final rules comparison.

## Graphics after mechanics

Neo Geo scenery often uses the same chained sprite system as characters,
with a separate FIX text layer. Do not infer a SNES background map or OAM.
Decode C-ROM paired bitplanes and S-ROM FIX tiles offline using the pinned
core's actual loading/decoding conventions. Validate pixel layout with known
patterns and then original video; an atlas without scene validation is only
a diagnostic. Resolve sprite chains, flips, zoom, palette, animation and
priority from captured descriptors; include the backdrop and FIX ownership.

Measure RAM-to-video timing before selecting animation anchors. A moving
actor's post-frame coordinates may differ from the position drawn that frame.
Confirm ownership/anchors across motion, catches and throws; do not hide
unexplained pixels by cropping or shifting each pose independently.
In the checked Windjammers case, entity RAM describes the scene that the
following reference step presents in VRAM/RGB. Pair samples at that measured
boundary. Entity byte+3 overrides pose-descriptor palette when nonzero; use
the selected VRAM palette bank and recorded brightness mode. Match visible
pixels only after deriving occlusion independently from opaque later VRAM
banks, not from mismatched colors. The local decoder/scene validator lives
in `games/windjammers/tools/extract_art.py` / `validate_art.py`; its selected
scope is unshrunk ordinary actors on Beach, not a generic zoom/raster renderer.

Extract only the chosen content. Scale art and geometry consistently; record
any camera/aspect adaptation. Flatten scenery only when the milestone permits
it. Generate four-grey native banks, mirrors, dilated white actor outlines
and pre-shifts offline. Disc, hands, goals and action cues must remain legible
at 160×100; keep essential cues at least two pixels wide. Host-order and
big-endian TI banks stay separate. ROMs, BIOS, third-party sources, states,
captures and extracted assets stay ignored; commit our tools and notes only.

## Runtime and acceptance

Use `game_scenario(n)`, PC input/state files and per-field state hashes;
raw struct hashes differ across the PC and TI ABI. Preserve measured
fixed-point units with small integer arithmetic. Lookup tables, shifts,
pointers, ExtGraph blits/TileMap and changed-region redraws take precedence
over runtime division, trigonometry or decoding. Read archived banks in place
through `rt_file`, allowing the TI OTH trailer during size validation.

Run mechanic tests against original timelines, then complete headless
playable replays, PC/TI state comparisons and screen checks (`make xcheck`).
Measure actual compiled TI frames with `ti-cycles`, including cold drawing,
maximum actor/effect density and transition/HUD work. Budget about 360k
cycles per 30 fps draw with grayscale; account separately for multiple logic
steps and the measured source rate. Inspect generated hot-loop assembly for
32-bit multiply/divide and float helpers.

TiEmu comes last, once at a milestone, for hardware paths only. Follow
`../ti89-emulator/SKILL.md`, clean Titanium load with all data banks. After
handover the keyboard belongs to the user. Reference preparation is not a
playable port. Commit through `../ti-commit/SKILL.md` when requested.
