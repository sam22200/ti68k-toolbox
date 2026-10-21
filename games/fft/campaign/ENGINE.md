# One engine for every chapter

Goal (user request, 2026-10-21): chapters hold only their own data. Mechanics (reach
contours, AI, menus, Status, rules) live once, in shared programs; maps, units, dialogue
and scenario are data files. Today each chapter recompiles `fft.c` with `FFT_OPENING` or
`FFT_STORY_GAR`, so every mechanic counts once per chapter and each program sits at
TTPack's 64,936-byte input limit (Orbonne is 253 bytes over).

## Target

- **`fftbat`, the battle engine** (one packed program): tactical battle plus the
  dialogue/staging beats played on the battle map (Orbonne's gate, banter and aftermath).
  Chapter rules become general mechanics switched on by data (weapon range and shield
  evasion tables, Charge, Raise/Break, drain/area, water style).
- **`fftscn`, the scene player** (small program, from `academy.c`): scenes on a fixed
  picture: title, chapel, Academy, chapter cards. A scene script drives it.
- **Per chapter, data only**: views and map (dimensions read at run time), units, a
  directory of offsets instead of generated `.h` files, the scene/beat script.
- **`fftstory`** walks the chapter list and chains the two programs.

## Size

Measured unpacked: Gariland 62,797 bytes, Orbonne 65,189. Orbonne's own scenario code
is ~4.9 KB; Gariland-only code ~0.5 KB plus larger variants (Status page +2.2 KB, unit
setup, water). Estimated union for `fftbat`: 66.5–67.5 KB, so ~2–3 KB must go: one
version of each divergent screen (Status: user agreed, with a before/after to validate),
the title/chapel code moved to `fftscn`.

## Milestones (each checked: PC/TI checksums equal to the current references, unless a
visual change is shown and validated)

1. **M1 run-time chapter description.** The bank directory replaces `battle.h`,
   `interface.h`, `units.h`/`actors.h` offsets, counts and small tables; map size, unit count
   and scene crop are read at load. No behaviour change.
2. **M2 general mechanics.** Orbonne's rule variants become data-driven paths of one
   implementation; Gariland must stay bit-identical.
3. **M3 scripts.** Beat/scene script format; `fftscn` replaces `academy.c` and the title/
   chapel part of `opening.inc`; battle-map beats run in the engine.
4. **M4 one battle program.** `FFT_OPENING`/`FFT_STORY_GAR` disappear from `fft.c`; size cuts;
   launcher; then one TiEmu run of the whole chain.

## Status

- M1: in progress.
