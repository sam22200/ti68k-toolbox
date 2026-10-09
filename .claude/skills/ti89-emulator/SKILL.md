---
name: ti89-emulator
description: Launch, drive and check a TI-89 program in the TiEmu emulator (Docker) — send .89z files, press keys, run name(), take screenshots to verify the result, save the calculator state. Use it whenever a compiled TI-89 program or game has to be run or tested, a bug has to be reproduced on screen, or the model has to change (TI-89 Titanium / TI-89 HW2).
---

# TI-89 emulator (TiEmu)

**The last step of the development flow** (`CLAUDE.md`): every check that can run without UI
(unit tests, PC headless runs, `ti-cycles`) comes first; the emulator is slow (restart, typed keys,
screenshots to read) and is used once, at a milestone, for what only the calculator runs.

Tools live in `tools/bin/` (put them on the PATH: `export PATH=$PWD/tools/bin:$PATH`).

| Command | Purpose |
|---|---|
| `ti-emu start\|stop\|restart\|status\|save` | TiEmu in Docker. `TI_CALC=89t` (default, Titanium AMS 3.10) or `TI_CALC=89` (TI-89 HW2, AMS 2.09 AMSpatch) or `TI_CALC=89u` (TI-89 HW2, official unpatched AMS 2.09: the one to test program size limits and execution protection; no saved state, boots to a fresh HOME) |
| `ti-run prog.89z [extra…]` | **the way to test a build**: `ti-emu restart` from the clean saved state with the files sent at boot (`-send=`, several files merged by `ti-group`), then `prog($TI_ARGS)` |
| `ti-emu restart file…` | clean state + files sent at boot, without running |
| `ti-send file…` | fallback only: types the path into TiEmu's file chooser (slow, needs HOME) |
| `ti-key [--hold S] TOKEN…` | keys: `ENTER ESC HOME CLEAR 2ND UP…`, or text `'hello()'` |
| `ti-shot out.png [--lcd]` | screenshot; `--lcd` = screen only, enlarged |
| `ti-gif out.gif [SEC] [FPS]` | animated GIF of the LCD (ffmpeg x11grab, x2, default 10 s at 15 fps); default-skin LCD coordinates scale with the window; run it with `&` before the `ti-key` sequence, then `wait`; seen in `ti-view` |
| `ti-play keys/x.txt [FPS]` | plays a runtime key script (`<frame> <keys>` lines) in real time through the PC keyboard: several keys held at once (diagonals), which `ti-key`'s clicks cannot; start it right after `ti-key ENTER`. Open loop: end each move against a wall, check the script on the PC with its timeline scaled ±10 % |

**Not everything needs the emulator.** TiEmu 3.04 (Ubuntu) has no command line control, no D-Bus
interface and no GDB, and exports none of its internals: memory and registers are only in its GUI
debugger (F11). For cycle counts and for checking what a routine drew, run the program on the PC
with `ti-cycles` (datasheet cycles per marked zone, the screen from memory as a PNG and a
checksum, data files `--file`, saves `--save-dir`, key scripts `--keys`, no window; runtime games:
`make xcheck`; `CLAUDE.md`, performance §10). Keep TiEmu for what touches the hardware: grayscale
driver, keyboard matrix, interrupts, link, real archive and Flash, and the final run of a build.

## Test loop

**Rule: every new build goes in with `ti-run` (or `ti-emu restart file…`).** Never quit a running
game to HOME to resend it, never type paths into the file chooser: a restart from the `.sav` takes
~7 s, leaves no stale variable, save or stuck key behind, and sends every file at once.

1. `ti-run build/prog.89z [data.89y]` (restarts from the saved state, on the HOME screen, and runs).
3. **Check the result**: prefer programs that print numbers (runtime `make bench`, checksums) and
   read them with one `ti-shot /scratchpad/x.png --lcd`; screenshots of drawn scenes only when the
   look itself is under test. Model: the Titanium (default); the TI-89 HW2 only for a release.
   For an animation, take a series of screenshots 0.3–0.5 s apart and join them with
   `convert a.png b.png +append strip.png`.
4. Quit the program (often `ti-key --hold 0.5 ESC`) before sending a new version: sending fails
   while a program is running.

## Known pitfalls

- **Session without a desktop `DISPLAY`** (Minish, Titanium, 2026-10-08): do not drive the
  user's live KDE display `:1` (TiEmu opens over their windows, `ti-play` keys follow the
  focus, and its `fr` layout scrambled typed text: `minish(256)` arrived as `596938brmnc`).
  Use `Xvfb :99 -ac` (layout `us`, typing exact), an empty file as `XAUTHORITY`, and, with no
  window manager, an `xdotool` shim first on the PATH that turns `windowactivate` into
  `windowfocus`. Never start a KDE window manager there (it broke the desktop's Alt+Tab).
  Stop Xvfb by its PID.
- **Large data banks must be archived** before the run: ~150 KB of banks left in RAM made
  the TileMap and GrayDBuf allocations fail (no scenery, HOME showing through every other
  frame). `Archive a,b,c` at HOME after the boot transfer (wait ~20 s for it), then run.
- **Open-loop timing**: measure a key script's tolerance on the PC with its events shifted
  ±40 frames as well as scaled ±5 %; the run started ~0.2 s after `ti-key ENTER` returned.
  Prefer scripts whose first events need no precise timing (let enemies come to the player).
- **Unexplained dark periods in `ti-gif` captures** (Minish, Titanium): the light-grey
  scenery shows dark for ~0.3 s about twice per 14 s, at different moments each run,
  including idle frames; `ti-cycles` screens of the same states are normal. Not resolved.

- **End a held-key demo explicitly** (Sonic, Titanium, 2026-10-05): `ti-play`
  releases all keys at the last script event, including the keys named on that
  event. To continue holding RIGHT after frame128 until frame250, append
  `250` with no keys. Headless scripts keep their final state instead: the
  same short file can win headlessly but stop early in a real-time demo.
- **Verify the captured game and LCD**: a running TiEmu process can be at HOME,
  and ENTER alone does not guarantee gameplay. On a requested relaunch use
  `ti-run` with all data files. `ti-gif` now scales the default skin's native
  LCD rectangle to the actual window; the previous fixed 166x108 crop showed
  the casing and cut off the LCD on a resized 464x1037 Titanium window.
  Custom skins need their own rectangle. Review a few GIF frames and observe
  the endpoint before reporting a winning replay; timing can drift.
- **Games that read the keyboard with `_rowread`** miss short presses → `--hold 0.3` (or 0.5).
- **CLEAR during a game** can switch the calculator off (Puzzle Bobble calls `off()`):
  blank screen → `ti-key --hold 0.3 ON`.
- Blank screen after `restart`: auto power-off, or an unreadable `.sav` → `docker logs tiemu`.
  A `.sav` stores the **absolute path of the OS image**: do not move `tools/tiemu/<calc>/`
  (otherwise delete the .sav, clear `sav_file=` in tiemu.ini, set the calculator up again, then
  `ti-emu save`).
- Expected starting state: empty HOME screen, apps desktop disabled (MODE → F3 → Apps Desktop
  OFF, on the Titanium), then `ti-emu save`.
- Do not use the system `tiemu` (Ubuntu 24.04): it corrupts files sent to the Titanium (program
  received as EXPR, "invalid variable name").
- Never run `import -window` without a valid window id (it waits for the user to click): use
  `ti-shot`. Never send Print Screen (keycode 107 starts Spectacle/Flameshot) or the Menu key
  (it opens TiEmu's menu). `ti-key` handles this.
- **Playing with the PC keyboard** (the shim below): arrows = TI arrows, AltGr = 2nd, Right Ctrl =
  alpha, Shift = shift, Left Ctrl = ◆, Enter, Esc, Backspace. TiEmu 3.04 reads XFree86 keycodes and
  today's X servers send evdev ones: without the translation the PC arrows did nothing.
- **Stuck keys** (a key the program still sees held after release; FFA's hero walking alone):
  TiEmu clears a key only on its GTK release event, and loses it when the release goes to
  another window or a skin click is released off the key. `tools/tiemu-keyfix.c` (LD_PRELOAD in
  the image) replays releases the X server says happened and moves mouse releases back to the
  press point (**verified**: a synthetic press without release sticks with `TI_KEYFIX=0`, is
  released within 50 ms with the fix). Debug a game's keys by printing `rt_keys` on screen.
- One instance at a time (container named `tiemu`): `ti-emu stop` before switching models.
- **No digits in program names**: typing digits through the PC keyboard is unreliable (`int5rate()`
  came out as `tan(`, and the keypad keycodes opened MEMORY/2nd menus and left a modifier stuck).
  Name test programs with letters only. If the toolbar or typed text looks shifted (2nd/◆/alpha
  stuck), `ti-emu restart` (back to the saved state).
- A program's final "press a key" screen swallows the next `ti-key` sequence: send `ENTER` first
  (or check with `ti-shot`) before `ti-run`ning the next program, otherwise its name is typed into
  the waiting program.
- Waiting for a long test: poll `ti-shot` in an `until`-style loop and check the image (e.g. count
  dark pixels in the text area) instead of a fixed `sleep`.
- Big files take seconds to transfer (60 KB ≈ 8 s): the next `ti-send` right after fails with
  "the file chooser did not open". Wait ~10 s after a big file, and retry once.
- The calculator RAM fills up after a few 30–60 KB test programs: a send then silently does not
  arrive (`getType(name)` answers "NONE", running it gives "Program not found"). `delvar` old tests, or `ti-emu restart` (the saved state has none). A refused send shows in
  `docker logs tiemu` as "hand-held returned an uncaught error", and the calculator then ignores keys
  for a while.
- A program ended with `ngetchx()` is still waiting after its screenshot: press ESC before the next
  `ti-send`, otherwise the file chooser does not open.
- A restart restores the saved state, which also resets anything a previous program left behind
  (e.g. an int-5 start value): restart before measuring defaults.

## How `ti-key` works (if a key does not respond)

- Named keys (2ND, ESC, HOME, CLEAR, arrows…) = **a click on the skin** (coordinates per model in
  `CLICK_89T` / `CLICK_89`, scaled to the window size).
- Text, ENTER, F1–F5, STO, ON = **raw keycodes**: TiEmu matches the hardware keycode against its
  Linux PCKEY table (XFree86 keycodes, US layout: HOME=97, UP=98, DOWN=104…), so this works
  whatever the host keyboard layout (AZERTY here).
- TiEmu menu (right-click on the calculator): F10 = Send file, F11 = debugger, F12 = Set ROM.
