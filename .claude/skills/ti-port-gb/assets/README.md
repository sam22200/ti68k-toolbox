# ti-port-gb assets: the whole-game port's skeleton (from Bubble Ghost)

Bubble Ghost's port (`games/bubble_ghost/`, local: commercial) kept as templates for the next
Game Boy game. The engine-independent parts work as they are; the game-specific parts are
listed per file. Copy them into `games/<name>/` and adapt.

| File | What it is | Adapt |
|---|---|---|
| `gb.h` | the GB memory model (`R8 W8 H8 IO`, inline `rd`/`wr` with WRAM and ROM fast paths), `fill`/`copy`, `mul16`, protothread macros (`PT_BEGIN PT_WAIT1 PT_CALL PT_EXIT PT_END`), the wait helpers' prototypes | nothing |
| `PORTING.md` | the translation conventions every routine follows (and every sub-agent) | the routine names of the examples |
| `flow.c`, `flow.h` | memory (malloc'd WRAM/VRAM, the ROM as the data file `bgrom`), `rd_slow`/`wr_slow`, `read_keys` (04A4), the waits (0390, 0387, 04F6), `r_0379` (a VBlank job), the VBlank interrupt (033A: job flags, callbacks, OAM DMA into `gb_oam`), `gb_vblank` (ISR + the main protothread), `door_loop`, the scenarios, the state hash, the runtime hooks (two VBlanks per 30 fps frame), the save | the addresses of the input routine and its tables, the ISR's job flags and callbacks, the scenario door, the save range |
| `render.c`, `render.h` | the GB screen from VRAM / I/O / OAM: BG and window map caches (256 x 256 plane pairs, dirty cells, redrawn only while the LCD is on and only for the maps in use), BGP/OBP through-palette, 8 x 8 sprites cached per palette and flip (6 KB blocks on demand), DMG priority (x, then index; one u16 key), BG priority from the map cache, a play view (GB rows 16..95 + the port's HUD) and stitched bands for the other screens | the play view's rows, the HUD, the bands per screen, the `in_play` / `screen_kind` hints set by the game |
| `test_game.c` | trace tests: the door memory loaded (`build/door_NN.bin`), samples by a hook at the logic point (0283) or at every key read (04A4, scenario 99 = power-on, 100 + n = door n), variables + Fletcher-16 of whole RAM regions, `--vars` for gbtrace, `--dump`, `--shot` (the screen at a logic frame), `--hash` | the variables, the regions (leave out the sound driver's RAM, the stack, the VBlank counter and lag-sensitive flags), the coverage |
| `door.py` | the ROM's memory when the level routine starts, after the door's pokes | the level routine's address and the pokes |
| `screencmp.py` | the port's screen (`--shot`) against PyBoy's at the same logic frame, pixel by pixel | the playfield rows, the door |
| `bot.py` | records a key script that pushes an object along waypoints (a level finished for the ending's traces) | everything game-specific: the addresses, the "near" rule |
| `fuzz.py` | seeded random key scripts | nothing |
