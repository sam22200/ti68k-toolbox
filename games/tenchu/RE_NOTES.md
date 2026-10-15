# Tenchu training study — 2026-10-14

## Decisions

User request: original TI-89 HW2 first; research and implement the five architectures
in order; start with Tenchu's training level. The user supplied a local USA disc.
Engine, reproducible tests and monochrome geometry precede character art.
An asynchronous question offered traversal/grappling followed by disc graphics as
the default scope; no response was received during this work. Combat and completion
have not been assumed part of this first engine milestone.

## Original disc and tools

OBSERVED: `SLUS_007.06`, entry `8001a578`, load `80011000`, 243,712 executable bytes.
The ISO lists 24 files. `DATA.VOL` is 31,714,732 bytes, CRC32 `1F83A85A`, SHA-256
`e7bdb99ea9e4a0871f4842b2cd44b7649e1d52b6896d55b5258e98afd741c139`.
Ghidra's boot-executable analysis exported 743 functions, zero decompilation failures.
The boot executable is not the full game: MAIN.EXE and other overlays are separate.
No MIPS function has been translated into C.

`tools/oracle.py` cold-boots the local disc under the existing patched pcsx_rearmed
HLE core, logs every input, and enters training at 15,000 video fields. It writes
`local/oracle/training.state`, RAM, PNG, walking/jump CSVs and a provenance JSON.
The explicit timing includes the title transition and character selection. No
Sony BIOS is required. The corresponding manual study artifacts are in
`sources/tenchu_ps1/reference/` and are local only.

```sh
../../tools/pyenv/bin/python tools/oracle.py
../../tools/pyenv/bin/python tools/extract.py --ram local/oracle/training.ram
```

## Archive and geometry

OBSERVED: AFS magic `AFS_VOL_200`, 545 big-endian 36-byte directory/file records.
Parent identifiers reconstruct `WORK/CDIMAGE/STAGE/TANREN`. Relevant files:
`MAP.MAD` 57,692 bytes; `STAGE.ACM` 848; `STAGE.CON` 672; `STAGE.POS` 824;
`TIM.TPD` 42,084. The neighboring TANREN_MV is a separate movie-related area.

OBSERVED: MAD magic `96102900`, 72 records. Each references a flags=0, one-object
TMD (`41`) relative to the MAD's eight-byte header. All 1,547 raw vertices match
the training RAM at `80125438` in the captured study. The 962 primitives use
unlit modes `3D` (891), `35` (10), `39` (60), `2D` (1). All vertex indices are valid.
TMD quads are strips; their boundary order is reconstructed before rasterization.

INTERPRETATION: the three MAD record coordinates are model centres in ten world
units. Adding them to the local vertices gives bounds X -6,000..48,000,
Y -17,000..0, Z -8,000..56,000, consistent with STAGE.POS's torch/target placements
and the observed opening platform. This is not a complete reverse engineering of
the original rendering/collision pipeline.

TARGET: `(X+8192)/64`, `(Z+8192)/64`, `-Y/64`, round to 16-bit world units.
Exact coplanar rectangle merging reduces 962 faces to 156, 221 unique native
vertices. Before rounding, the merged rectangle area equals the original area:
3,706,000,000 square original units. Textures/material boundaries are not preserved
by this geometry-only merge. The bank is 29,638 bytes including collision and ledges.
Original UVs, colours and textures are not part of this prototype.

## Observations, not writable physics state

OBSERVED: the displayed model translations at `80125280/84/88` (X/Y/Z) and duplicate
`801252A0/A4/A8` change every second NTSC field during normal motion. The preceding
copy at `801251D8/DC/E0` can lag by a logic update. Uniform walking samples show
123 units per changed step; the opening obstacle eventually limits Z to 3,912.

OBSERVED: CROSS from the captured idle state changes Y through 0, -240, -460,
-660 ... -1540, -1560, -1560 ... -240, 0. First rise at video field 33, landing
at 81 for a press at 30. Apex 1,560 units; 48 fields from first nonzero height
to landing (~0.801 s at 59.94 Hz). Including the starting zero update gives
50 fields (~0.834 s). These are model translation observations.

LIMIT: poking these copies, even repeatedly, did not establish a writable physics
master. Three attempted teleports returned to the initial position. Therefore no
teleport-derived floor probe has been accepted as a verified original rule.
Animation/collision influences on the observed translation require further study.

TARGET: walk alternates 4/5 native units per update at 12.8 Hz (57.6 units/s,
consistent with 123*29.97/64). Jump table is
`0,8,15,20,23,24,24,22,18,13,6,0`: apex 24 units (~1,536 original units),
11 updates (~0.859 s). This is a resampled adaptation, not a bit-exact original.
Walking is blocked at the opening platform; jumping clears its native +16 height.
Falling and grappling use native rules, with approximate collision intervals.

## Verified native checks

PC assertions cover the opening floor, raised platform, blocked walking, clearing
the platform during a jump, leaving its far edge, jump apex/landing, bridge
underside traversal, gabled-roof walking, LOS, guard-ray hits, render clipping,
portal door states, A/B/C sprite pixel equality and camera view return.
Address/undefined-behavior sanitizer runs cover all rendering paths.
Each benchmark compares PC and the actual TI binary framebuffer checksums.
The stock AMS 2.09 / HW2 Docker emulator started the program from archived banks.
One LCD capture is `local/emulator/hw2.png`; no physical-hardware timings exist.

## Remaining original questions

Writable player state and true collision bounds; allowable step/jump rules;
hook range, acquisition constraints, body trajectory and landing; original room
visibility and collision data semantics; full course route and ending; original
character/pose/texture decoding. The archived map is the full geometry, but a
complete playable mission with original rules and art has not been delivered.
