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

Serialize GCC4TI builds. The bundle is `x/fftcampaign.89g`: 23 variables,
364,712 bytes after configurable reach, tactical facing and archived-table updates, including the small `fftstory` supervisor,
three packed launchers, three archived cores and the chapter banks. All data
and packed cores have the archive attribute; do not move all of them into RAM.
Retail art, text, binary data and captures remain generated/ignored.

On the calculator, send that group and run **`fftstory()`**. ENTER/2nd advances
dialogue; SHIFT skips the current Orbonne section; CATALOG inverts its dialogue
window. During battle, CATALOG selects **Move: full tiles / Move: contour**
(default full); ENTER toggles and ESC/CATALOG closes settings. Ordinary Gariland controls apply after the Academy. ESC cancels a chapter
without automatically starting the next. Calling `fftopen()` alone still returns
to HOME after the prologue; use `fftstory()` for the automatic continuation.

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
