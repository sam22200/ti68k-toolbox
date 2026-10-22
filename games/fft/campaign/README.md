# Opening -> Academy -> Gariland

Local-disc campaign slice, sharing the existing tactical engine. The sequence is
Orbonne's chapel and encounter, battle, abduction, Chapter 1 Academy dialogue,
then the existing playable Gariland battle. The flashback correctly starts a
new level-one party rather than carrying Orbonne's level-nine stats forward.
Gariland's results end this slice; they do not restart Orbonne.

The Academy is a fixed native staging with the source MAP024 background, source
actors and all 15 observed messages. It is not a recreation of the world map,
deployment screen or every PS1 camera/actor movement. Orbonne's final punch,
collapse and Chocobo escape are sprite adaptations of movie choreography.
See [opening corrections](../opening/BUGFIXES.md).

## Build

With the local US disc, first generate/observe the opening as described in
`../opening/README.md`, including the added scene oracle:

```sh
tools/pyenv/bin/python games/fft/tools/opening_scene_oracle.py \
  'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'
make -C games/fft/opening assets check
make -C games/fft/campaign assets
make -C games/fft/campaign check bundle
```

Serialize GCC4TI builds. The bundle is `x/fftcampaign.89g`: 30 variables, 363,696 bytes:
the small `fftstory` supervisor, two packed launchers (`fftbat`, the battle program of every
chapter, and `fftscn`, the scene player), their archived cores (`ffbcore`, `ffscore`), the
management menus (`fftmng`, 18,203 bytes, and their bank `ffmg`), the battle modules (`ffbm`)
and the chapter banks. All data
and packed cores have the archive attribute; do not move all of them into RAM.
Retail art, text, binary data and captures remain generated/ignored.

On the calculator, send that group and run **`fftstory()`**. ENTER/2nd advances
dialogue; SHIFT skips the current Orbonne section; CATALOG inverts its dialogue
window. During battle, CATALOG selects **Move: full tiles / Move: contour**
(default full); ENTER toggles and ESC/CATALOG closes settings. Ordinary Gariland controls apply after the Academy. ESC cancels a chapter
without automatically starting the next. `fftbat(n)` alone plays one chapter (100 + an
Orbonne door: `fftbat(100)` its gate; 200 + a Gariland door: `fftbat(200)` its battle) and
returns to HOME; use `fftstory()` for the automatic continuation.

**Campaign save** (`save.h`, the variable `ffsave`, archived): `fftbat` writes it when it
leaves after a victory: the chapter's story flag, and after Gariland the new party (the
units still with it, FFT's job/item/ability ids, their jobs' JP and learned abilities, their
ability slots), the bag and the chests' items. `fftstory()` resumes after the last victory
saved (after Orbonne: the chapter card; after Gariland: the management menus); `fftstory(1)`
deletes the save and starts a new game. `python3 games/fft/tools/save_dump.py ffsave.sav`
decodes one.

**Management menus** (`fftmng()`, `manage.c`, after Gariland in the chain): the party in the
prototype's classic grid (`../MANAGEMENT.md`, still sprites). ENTER: Status; ALPHA or
DIAMOND: the unit's menu, Equip (from the inventory, within the job's equipment types),
Abilities (secondary skill set, reaction, support, movement among those learned), Learn (the
job's abilities for its JP) and Change Job (FFT's 20 generic jobs and their unlock levels;
Bard men, Dancer women; Ramza keeps his own sprite). F1/F5 change the unit, SHIFT goes
back, ESC writes the save and quits. Its bank `ffmg` (`tools/management_data.py`, 47.8 KB):
jobs, items, names, icons, still sprites and portraits from the disc.

On the PC:

```sh
make -C games/fft/campaign pc
tools/pyenv/bin/python games/fft/campaign/play.py
```

The supervisor starts a new process per chapter. Native `NG_execute` similarly
returns from one packed program before opening another: their frame buffers and
archive pointers never have to coexist. Jobs, items, templates, abilities and
skill sets are read from archive; birthday and JP fields use explicit big-endian
reads on both targets. Non-shipping research injection scenarios are omitted
from the Gariland executable to respect TTPack's 64,936-byte input limit; the
battle, direct Ramza (61) and full-AI (16) doors remain.

## Memory and validation

An isolated Titanium TiEmu test initially reached the Academy but Gariland
reported `not enough memory`. PC/68000 framebuffer checks alone did not catch
this AMS heap constraint. Sharing the full Status screen with the inactive
tactical portrait buffers saves **7,680 bytes**. Leaving Status invalidates both
portrait caches, so the old Status text cannot appear over combat. No menu,
ability or combat action was removed.

After this correction, a fresh archived-group boot on **Titanium AMS 3.10**
passes the actual packed-program sequence: prologue return -> Academy return ->
Gariland, ending at Ramza's usable menu. The bounded `fftstory(99)` injection
uses the final prologue screen, final Academy message and direct Ramza door;
it verifies the AMS/heap/program lifetime, not a complete manual playthrough.
Capture: `x/tiemu-check/handoff-final.gif`, final LCD `handoff-final.png`.
That capture validates the prior 353,955-byte checkpoint; the new reach-cache
package is headless-checked, with no new TiEmu heap-lifetime claim.
The user's live emulator and keyboard were never touched. No original HW2 or
physical-calculator validation is claimed for this group.

Independent checks:

- Opening: 16 exact PC/actual-68000 pixel/state checkpoints, CT/tile-lock and
  interruption regressions, complete AI victory and non-repeating conclusion.
- All 11 Orbonne actors animate; 648 walking poses match their requested bank
  frames, cold portraits take 5–12 PC updates, and four water phases are distinct.
- Academy and Gariland: ten PC/68000 replays including Status, Move settings
  and the contour actually on screen; `x/check/checked.json` and `x/check/status.json`.
- Academy dialogue: all words from 15 source messages survive across 17 fitted
  pages. Unnamed shouts remain body text, and speaker whitespace is trimmed.
  The final text-only bank correction also passes both native Academy replays
  (`x/check/academy.json`); the TiEmu capture predates that correction.
- `test_status.c` checks all three restored portrait planes after using the
  shared storage. Existing Gariland and Auto-battle tests still pass.

These are correctness checks and datasheet CPU measurements, not PC-derived
hardware FPS. The camera handedness fix is enabled in this campaign and Orbonne;
historical standalone research controls retain their old projection/banks.

## Chapter storage

Data the two chapters share is stored once in the campaign group:

- **Interface, `ffcu`** (`tools/common_ui.py`, run by both chapter generators): character
  advances, font, slope markers and banners (7,916 bytes) were byte-identical in `ffoi`
  and `ffgi`. Both banks drop them; `fft.c` reads them through `cgfx` (`UI_SHARED`), and
  every later `igfx` offset (tables, Orbonne's name directory) is shifted.
  `campaign/data/interface.h` is the stripped copy of the root header.
- **Actors, `ffca`/`ffcb`** (`tools/common_actors.py`): every distinct 208-byte actor
  frame of both chapters (characters, armed bodies, crystals, chests) is one ZX0 stream,
  numbered across the two variables (64,893 + 19,550 bytes, 597 frames, 71 used by both
  chapters). A frame that mirrors another is stored once: bit 15 of a chapter's logical
  frame entry (`opening/data/ffoa.bin`, `campaign/data/ffga.bin`) flips it after decoding
  (ExtGraph `SpriteX8_MIRROR_H_R` on the TI). Blades alone are `ffcw` (3,024 bytes).
  Orbonne's actors and Gariland's raw `fftu`/`fftw` (125.6 KB) become 89.7 KB.
- **Decoded frames:** one LRU cache of 48 frames (`AC_N`, 10.2 KB, its own block) shared by
  every actor, at most one ordinary decode a frame; each decode is charged to the frame
  budget. Gariland's AI battle wins at frame ~9,073 (raw frames: 8,882; 32 cached frames:
  10,445; one frame per actor: 12,615). `FFT_RAW_ACTORS` builds the raw-bank control.
- **Scenery views** (`ffov0..3`, `ffgv0..3`) store each occlusion-mask row XORed with the
  row above before ZX0 (`tools/extract.py:rowxor_masks`). `fft.c:unxor_masks` undoes it after
  each decode, one long at a time: Orbonne −3,031 bytes, Gariland −2,940. A view decode
  (load or turn) costs 78–114k more datasheet cycles (6.5–9.5 ms at 12 MHz). Colour and
  depth planes stay plain: row XOR would save only ~1 KB / ~0.3 KB per map for the same CPU.
- **Gariland's views drop their 16 top rows** (`SC_TOP`, empty in all four orientations;
  `tools/campaign_assets.py` checks them): 202 scene rows, 5,120 bytes less RAM for the view
  and scene buffers. The camera no longer climbs into that white band (it stopped once 5
  pixels lower in the AI battle; user-validated). Orbonne's views were already cropped.

RAM (TiEmu Titanium, every bank archived, `fftgar(61)` launched alone): with the crop,
Gariland starts with 48 and with 56 cached frames; before it, 36 or more ran out of memory.
All 16 opening and 10 campaign PC/TI checks pass; Gariland's keep the baseline's screen
checksums and state. Eight Orbonne screens change (poses no longer lag a frame behind),
awaiting visual review in `x/actors/opening/avant-apres-orbonne.png`. Orbonne's packed program (65,189 bytes) is still over TTPack's 64,936-byte
input limit: the next step merges the chapters into one shared battle engine and scene player
with chapter data files.
