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

1. **M1 run-time chapter description** (done). A descriptor (`ffod`, `ffgd`,
   `tools/chapter_desc.py`) replaces `battle.h`/`interface.h`/actor offsets, counts and small
   tables. It already carries map rows and the scene crop (`MAP_H`, `SC_H`, `SC_TOP`).
2. **M2 scripts out of the program.** Beat/scene script format; `fftscn` replaces
   `academy.c` and the title/chapel part of `opening.inc`; battle-map beats run in the engine
   from the script. Frees most of Orbonne's ~4.9 KB of scenario code.
3. **M3 general mechanics.** Orbonne's rule variants become data-driven paths of one
   implementation; Gariland must stay bit-identical.
4. **M4 one battle program.** Map rows and scene size become run-time values,
   `FFT_OPENING`/`FFT_STORY_GAR` disappear from `fft.c`; size cuts; launcher; then one TiEmu
   run of the whole chain.

Measured: reading `MAP_H` and the plane size from variables costs ~400 bytes of code (~75
uses, each an absolute address plus a relocation instead of an immediate), more than
Orbonne's program can spare before M2; so it waits for M4. Table pointers (`jobs`, `items`,
`abilities`, `skill_sets`, `unit_defs`) are globals set at load: smaller than the former
constant offsets.

## Status

- M1: done (descriptor; dimensions deferred to M4).
- M2: in progress. The scene player (`campaign/scene.c`, fftscn, 12,029 bytes) plays
  Orbonne's title and chapel (`ffs0`) and the chapter card (`ffs1`); their screens equal
  the opening program's former ones (title/chapel checksums, card 1523). The opening
  program starts at the gate (the end of the scene player's fade) and hands over after the
  aftermath: its packed core is 45,215 bytes and now passes TTPack. The aftermath is
  identical frame by frame; from the gate, only frame-counter phases differ (the dialogue's
  continuation blink, banner sweep, idle poses), the counter restarting with the program.
  The chain: fftscn(0), fftopen, fftscn(1), fftacad, fftgar (test door 99: fftopen(5),
  fftscn(1), fftacad(14), fftgar(61)). Next: the Academy as a scene set, then the map
  beats as a generic script in the engine.
- Known limit: the white dialogue palette (CATALOG) does not carry over between programs.
- RAM (TiEmu, Titanium): the test chain runs the opening door, the scene player's card and
  the Academy, then Gariland stops on "not enough memory". Measured with a HeapAvail probe:
  each archived program the launcher ran stays copied in RAM until the launcher ends (the
  scene player was 12 KB: now a 1.1 KB `-pack` stub), and Gariland nested under the launcher
  starts with ~187 KB free but not with 2-5 KB less: it needs ~183 KB of a Titanium's ~188 KB.
  Already moved out of its RAM, results unchanged: water glints (4,000 bytes, now in the
  descriptor, computed by the engine itself: `campaign/water_dump.c`) and the byte-doubling
  and tile tables (1,512 bytes, archive tail). Gariland needs several more KB of headroom
  before the chain can run on a calculator holding anything else.
