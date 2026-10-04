# From the ROM's machine code to runtime C

Ghidra's pseudo-C is the map, the traces are the judge. The port is hand-written C on the
same variables as the game, checked frame by frame against `gbtrace.py`.

## 1. Read before naming

- Start at `entry` (0100): init calls, then the outer loop (title → game → game over) and the
  inner per-level loop. The per-frame body is the list of calls between the input read and
  the VBlank wait (`halt` then `while (flag == 0)`). Write that list in the README first: it
  *is* the update order the C keeps.
- Skip the noise: `uRamfffc = 0x1a3;` before a call is the return address being pushed;
  `IME(0)`/`IME(1)` = `di`/`ei`; the init block repeated inside a function = the soft-reset
  path (A+B+Select+Start) inlined.
- Name as you go, in Ghidra (GUI: `L` on a label, then re-run `gbdecomp.sh`: it re-exports
  the saved project) and in `games/<name>/<name>.sym` (`00:c0ac hall`), so `gbtrace.py
  --sym` traces by name. Function names say what they do to the state (`ghost_update`,
  `bubble_collide`), not how.
- Confirm each meaning with an experiment: `--poke` the variable and watch the screen
  (`--shot`), or hold a key and `--trace` the range with `--diff`.

## 2. Two kinds of state

- **Gameplay state**: what changes what happens next. Exact: same width (`u8`, `u16`), same
  wrap-around, same update order, printed in the traces.
- **Video state**: OAM shadow, VRAM writes, tile animation counters, palette fades. Rebuilt by
  `game_render()` from the gameplay state; not traced (or traced only to check a sprite's
  screen position once).
- A variable written by both (an animation frame that also gates a hit box): gameplay.

## 3. Program shape

| ROM | Port |
|---|---|
| init block (WRAM clears, initial values) | `game_init()`: one `memset` of the state struct + the ROM's initial values |
| the per-level setup (`play_hall` prologue: load map, place objects) | `level_start(n)` |
| the per-frame calls, in order | `game_update()`, the same calls in the same order |
| VBlank handler (OAM DMA, scroll registers, music tick) | nothing / `game_render()` / dropped |
| `pre.state` + `--poke` | `game_scenario(n)` |

All gameplay variables in one POD struct, named as in the `.sym` (one field per RAM
variable or table; a comment with its GB address). Keep the GB's memory layout only where the
code indexes across variables (`(&DAT_c0b4)[i]`: a table).

## 4. Translation patterns

| SM83 / Ghidra | C |
|---|---|
| `ld a,(x); add a,b; ld (x),a` | `s.x = (u8)(s.x + b);` |
| 16-bit in two bytes, `add`/`adc` | a `u16` field (the trace prints the pair), or two `u8` with `u16 t = lo + b; lo = t; hi += t >> 8;` |
| `cp n; jr c` (unsigned <) | `if (a < n)` on `u8` |
| `bit 7,a; jr nz` / `jr` offsets | `if ((s8)a < 0)` |
| `daa` (BCD) | `bcd_add(&score, n)`: digit-wise add with carry, the HUD prints nibbles |
| `swap`, `rlca`, `rra`, `xor` RNG | the same operations on `u8`, with the carry bit kept explicitly |
| `CONCAT11(h, l)` | `(u16)h << 8 | l` |
| jump table on a state byte | `switch (s.state)`; the targets made functions with `gbdecomp.sh ROM OUT TABLE:N` |
| table in ROM (`&UNK_310f`) | a `static const u8 name[]` generated from the ROM by a build tool, local if the ROM is commercial |
| busy wait on LY / `halt` loop | removed (the runtime frame) |
| OAM shadow writes | the sprite list of `game_render()` |
| VRAM tile/map writes during play (a door opening, a counter) | a state change the renderer reads; redraw that cell |
| sound register writes / sound request byte | dropped, or an event counter for the tests |

- **Never `int`** in ported code: GCC4TI's is 16-bit, the PC's 32 (the Celeste port lost a
  sign only on the TI). `u8`/`s8`/`u16`/`s16` everywhere; `u32` for sums that need the
  carry.
- **Multiply / divide**: the SM83 has none; the ROM does it with shift-and-add loops or tables.
  Replace the loop by the C operator only when the result is the same for every input
  (truncation, overflow); keep the ROM's tables (sines, speeds) as tables.
- Speed on the 68000: byte ops are as cheap as word ops; tables in ROM order are fine;
  avoid `u32` products in the hot path (`__mulsi3`).

## 5. Logic rate

- The GB updates at 59.73 Hz. Decided in the grilling: (a) two `game_update()` per TI frame
  at ~30 fps (exact; double logic cost, usually small), (b) one per frame and every speed
  constant doubled (not exact: the traces only check a decimated run), (c) one per frame,
  the game at half speed (exact, slow). Default (a); the trace tests run the update at 60 Hz
  regardless, the frame loop only decides how many per render.

## 6. The trace test (C side)

- `test_<name>.c` loads the same key script (`sw_load_script`, `sw_script_keys`), maps the
  keys as `gbtrace.py --map` (A=a, B=b, C=start, D=select), steps one GB logic frame per
  script frame (not `game_update()`, which runs two at 30 fps) and prints `"<frame>
  addr=hex ..."` with the same names and widths as `--trace`.
- The reference comes from `gbtrace.py ROM --load pre.state --boot-keys A --poke ...
  --poke-at <level start> --logic <after the update> --stall N --keys K --trace <vars>`: frame
  0 is the first logic frame of the level on both sides, whatever the loading takes.
- Ghidra's pseudo-C drops what a callee leaves in registers. When a trace diverges after a
  call, read the callee in `disasm.s` for the registers it changes (Bubble Ghost:
  `blow_bubble` overwrites E, which its caller tests next).
- The state the game had before the level matters too: the joypad bytes keep the title's
  last read (A held), so `keys_pressed` at frame 0 differs unless the port starts the same.
- The ROM is commercial: `make traces` regenerates `traces/` from `roms/gb/` when the ROM is
  present; the tests print "ROM missing: trace tests skipped" otherwise. Never commit traces
  that contain ROM data (level bytes); traces of positions and counters are our own
  measurements but stay local too unless the user decides otherwise.
- On a difference: the first divergent frame and variable. Debug by tracing more variables
  around that frame on both sides (`--from F-5 --frames F+5`), and read the routine that
  writes the variable (`disasm.s`, the exact instruction order matters for carries).
