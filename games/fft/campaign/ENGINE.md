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

## Core milestone (user, 2026-10-21): Titanium only, engine + modules

Then every Chapter I map and scene is ripped as data. Programs, each run by `fftstory`:
- **`fftbat`** battle engine: an unpacked core (rendering, map, units, state, input; at most
  AMS's 65,514 bytes) plus **modules** loaded only while used: AI thinking, Status/info
  screens, results and rewards, settings. TIGCC DLLs crash on an unpatched Titanium once
  the game has allocated its buffers (verified); modules are instead position-independent
  images (`-mpcrel`, `experiments/dll/picmod.sh`) kept in the archive and copied into a zone
  reserved inside the core program, called with a context of pointers (verified on TiEmu
  under greys). RAM = core + zone. Job abilities become data (FFT's formula families) read
  by one interpreter, so jobs add data, not code.
- **`fftscn`** scenes (done: title, pictures with depth, beats, cards, portraits).
- **`fftmap`** world map (to create): locations graph, travel, entering battles/scenes/menus.
- **`fftmng`** management menus (party, equipment, jobs).
- **A campaign save** shared by every program: party, items, story flags (done, below);
  the location comes with the world map.
- **Per map / chapter, data only**: views, descriptor, units, beat script.

Scaling limits measured, and their answers:
- map RAM: Gariland's buffers ~59 KB; Chapter I's largest maps (240 rows) ~+9 KB: run-time
  map width/height (only the map's size reserved) and the modules (core code ~63 KB in RAM
  today, a smaller core plus one module).
- units: 11 at most today; a larger run-time maximum.
- the TI-89 HW2 is not a target (DLLs need HW3Patch there).

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
  The chain (then; now fftbat, M4 below): fftscn(0), fftopen, fftscn(1), fftgar (test door
  99: fftopen(5), fftscn(1), fftgar(61)). The Academy is a scene of `ffs1` (its picture with the eight actors drawn in,
  15 beats; 4,142 bytes with the card instead of the Academy program and bank, ~13.6 KB);
  its dialogue now has the shared style (gradient, six lines, continuation blink; validated).
  Every dialogue shows a known speaker's portrait, framed beside the box (validated):
  `ffcp` (tools/common_portraits.py, 2,198 bytes: Ramza, Delita, Agrias, Gafgarion, Ovelia,
  Simon, and the scripts' other names for them); generic speakers keep the name box.
  The beats on the battle map are a generic script (`mapbeats.inc`): the chapter descriptor's
  MAP_SCRIPT lists sequences of the beats bank (stage, event, flags: after the victory, fades
  in, actors placed by the beats, camera) and the player walks them before the battle and
  after a victory; Orbonne's gate, banter and aftermath are its three sequences (screens and
  states unchanged in every replay). Gariland can get its own by data once it enables it. Status stays per chapter for now: unifying on Orbonne's text-only
  page would save 2.2 KB but lose what the management menus will need.
- Modules (core milestone): the battle engine's chapter builds on the TI (`FFT_MODULES`) keep
  three parts out of the core (Orbonne four: its map beats), in an archived bank (`ffbm` since M4, built by
  `tools/module.py` from the chapter's own source with `FFT_MODULE = k + 1`), Gariland /
  Orbonne image bytes: the AI (7,556 / 7,688), the rotation animation (3,458 / 3,476), the
  results and Status screens (4,050 / 1,630), Orbonne's map beats (4,794). `fft.c` marks them `MOD_PART(k)`, the rest
  `MOD_CORE`; `module.h` lists the context a module reaches the core through. One module at
  a time sits in a zone inside the program (`MOD_ZONE`: the bank's largest image, 7,604 /
  7,748 bytes, the context included; the
  load refuses a bank that does not fit or was built with other types), copied when another
  is called. Programs: Gariland 57,573 bytes (was 62,797), Orbonne 58,039 (was 64,931 for
  TTPack's 64,936). PC builds have no modules, so the PC/TI replays check the modules
  (campaign 11 with Status and the results, opening 12 with its Status page + 6, unchanged).
  Cost (ti-cycles): an AI thinking step +8 % on average (+3 % at its peak: the core's state
  through a pointer), a rotation frame +3 %; frame peaks unchanged; entering Status or the
  results copies that module once; Orbonne's story frames +1 % (dialogue drawing through the
  context). TiEmu Titanium, packed programs: the AI plays and the
  view turns from the modules. Next candidate: the settings screen.
- M3 (rules): Orbonne's rule variants are general mechanics of every chapter build
  (`rules.inc`, `FFT_CHAPTER`), each switched on by the chapter's data: a weapon's reach and
  its projectile (descriptor WEAPON_RANGE: bows), a shield's evade (SHIELD_EVADE), Charge on
  the CT clock (AF_CHARGE, CHARGE_TICKS), weapon skills, Raise/Break, drain and the cross
  splash (ability kinds and flags), fixed unit records (UNIT_RAW). Evasion follows the
  decompiled rule for both (RE_NOTES.md § Attack): each source rolled in turn, the class
  evade from the front, then the shield's from the front or a side. Gariland has none of
  these tables or flags: its 11 replays are identical (screens, states, frames). Orbonne's
  former single roll on the product of its evades became the per-source rolls: its battles
  play differently (fight-first and fight-complete have new references; the complete
  battle still ends in the aftermath; its 10 other replays unchanged). Programs: Gariland
  60,467 bytes (+2.9 KB: Orbonne's rules), Orbonne 58,279. An AI thinking step: Gariland's
  peak 284k cycles (was 268k: a unit's weapon reach checked per candidate), average +1 %.
  Left for M4: unit names and portraits, the Status page, water and towers per map, map
  sizes; then one battle program.
- M4 (one battle program): `fftbat` (campaign/battle.c, 62,765 bytes, under TTPack's 64,936)
  plays every chapter: `fftbat(100 + door)` Orbonne, `fftbat(200 + door)` Gariland (below 100:
  the door of the build's FFT_DEFAULT_CHAPTER, for the tests). `FFT_OPENING` and
  `FFT_STORY_GAR` are gone: the engine reads every chapter difference from its descriptor
  (Ramza's unit, a unit's frame cost, sprite portraits and Chemist bodies, the Status style,
  the start: a story start with map beats or the battle at once, the map script) and its
  sizes (map, scene: buffers for the largest, chapters.h). `game_init` loads what every
  chapter shares (blades, actor banks, interface, the modules `ffbm`, one bank from
  battle.c) and allocates; `chapter_load` takes a chapter's banks ("ff" + o/g + v0..v3, a,
  i, d, st) on its first scenario. Both chapters' replays are identical to M3's (Gariland 11,
  Orbonne 12 + 6), each check binary plays the other chapter identically too, and on TiEmu
  Titanium the test chain fftstory(99) (fftbat(105), the card and the Academy, fftbat(261))
  reaches Gariland's battle and fftbat(100) plays Orbonne's gate. The launcher chains
  fftscn(0), fftbat(100), fftscn(1), fftbat(200). Costs: Orbonne's buffers take Gariland's
  larger scene (~2.4 KB more RAM); an AI thinking step +4 % on average (sizes read at run
  time); frame averages +0.5-1 %, peaks within +3 %. Both Status pages remain (style by
  data) until Chapter I's job names are ripped; then one page.
- Campaign save (`campaign/save.h`, `ffsave`, format FSV2): one 3,110-byte variable every
  program shares, bytes only (u16 big-endian), FFT's own ids so any chapter reads it: 256
  story flags, the inventory per FFT item id, up to 16 units (FFT job id, sprite, generation
  table, name, level, EXP, Brave, Faith, zodiac, raw stats, equipment, secondary skill set,
  reaction, support, movement, per generic job its JP and FFT's learned bits). The descriptor
  says what a victory writes (`WON`: its flag; `PARTY`: its player's units are the campaign's
  party, Gariland's new one, not Orbonne's flashback) and maps the chapter's tables to FFT's ids
  (`JOB_ID`, `SET_POS`, `BAG_ITEM`, `RSM_POS`, `DEFEND_ID`, `EQUIP_CHANGE_ID`,
  `EQUIP_CHANGE_POS`, from `tools/battle.py`). `fftbat` writes it as it leaves after a victory
  (and then returns 1: the campaign goes on), from a fifth module (`MOD_SAVE`, save.inc, its
  buffer in the module's own image: no RAM of the core), then frees its buffers for AMS to
  write the archived variable. A unit keeps what it was drawn with (battle_units records it
  without another draw): its job's JP and abilities, a generic unit's Squire JP and abilities
  when it is another job, the reactions and supports it learned (Equip Change: always), the
  support it fights with; units turned to crystals or chests leave the party; the items picked
  from chests join the inventory. `fftstory()` resumes after the last victory saved,
  `fftstory(1)` deletes it (a new game). Checks: campaign `won` (the AI plays Gariland to the
  end, ENTER leaves): PC and TI saves byte-identical, Gariland's flag and party; `save-test`
  (a chest picked, the save's inventory and draws); Orbonne's handoff sets its flag only, and
  the two merge. TiEmu Titanium: `fftbat(105)` returns 1 and leaves `ffsave` (OTH), then
  `fftstory()` opens on the chapter card; `fftstory(1)` starts at the chapel and the save is
  gone.
- Known limit: the white dialogue palette (CATALOG) does not carry over between programs.
- RAM (TiEmu, Titanium): the test chain runs the opening door, the scene player's card and
  the Academy, then Gariland stops on "not enough memory". Measured with a HeapAvail probe:
  each archived program the launcher ran stays copied in RAM until the launcher ends (the
  scene player was 12 KB: now a 1.1 KB `-pack` stub), and Gariland nested under the launcher
  starts with ~187 KB free but not with 2-5 KB less: it needs ~183 KB of a Titanium's ~188 KB.
  Already moved out of its RAM, results unchanged: water glints (4,000 bytes, now in the
  descriptor, computed by the engine itself: `campaign/water_dump.c`) and the byte-doubling
  and tile tables (1,512 bytes, archive tail). Then the views split (`extract.pack_view`:
  colour stream, depth stream): colours decode straight into the scene buffer, the view
  buffer keeps the four depth planes (-16,160 bytes) and a 128-row backup keeps the colours
  of the rows reach rings are drawn on (+10,572; a reach touched up to 126 rows; past it the
  colours decode again, ~800k cycles once): net ~5.6 KB for Gariland, screens and states
  unchanged, loads/turns ~90k cycles faster, a reach's first frame up to +31k. The test
  chain (fftopen(5), fftscn(1) card and Academy, fftgar(61)) now reaches Gariland's battle
  on TiEmu with the 48-pose cache.
