# The grilling tree for a PICO-8 port

Run `/grilling` (skill `grilling`) with this tree **after** the extraction and the reading
(steps 0 and 1 of `SKILL.md`), **before** any C. Find the facts yourself first (census in
`info.json`, measures with `p8trace.py`, map counts with a script): the user decides, you
never ask them for a fact. Every question below comes with the data it needs and a default;
put the numbers in the question, not "it depends". Rounds follow the tree: a branch is asked
only once its parent is settled. Record every answer in `games/<name>/README.md` § Port
decisions (the decision, the reason, the date): it is the spec the tests check.

## Round 1: the roots (no prerequisites)

1. **Source of truth.** Which cart and version (BBS id and post date, `.p8.png` hash, a
   GitHub copy), and what else exists (a C/C# port, ccleste, a remake)? Default: the BBS
   `.p8.png` decoded by shrinko8 is the reference; other ports are a second opinion only
   (the C# Celeste Classic uses floats and rewrote the map layer arguments: it is not
   bit-equal to the cart). When a line-by-line C port exists (ccleste for Celeste), ask
   whether to start the C from it (its floats turned into `fix`, diffed against the cart's
   traces like a hand translation) or from the Lua: it saves the translation, not the tests.
2. **Licence and repository.** A BBS cart is all rights reserved unless the post carries the
   CC4-BY-NC-SA tag (Celeste has none; ccleste has no licence file either); the art and the
   map are part of the cart. Default: our engine code in the repository, the cart, its
   extraction and every generated data file local (`sources/`, `.gitignore`, built by the
   Makefile as FFA's `rooms.h`), unless the licence allows more.
3. **Fidelity.** (a) *Exact*: the C logic reproduces the cart frame by frame (same 16.16 bits,
   same object order, same rnd), the tests diff traces against `p8trace.py`; (b) *Feel*: a
   rewrite tuned by eye, like `ti-port-sdl`. Default: (a) for precision games (platformers,
   physics puzzles: Celeste, jump arcs, dash lengths, frame windows such as coyote time),
   (b) for slow games (puzzles, RPG, card games) where only the rules matter.
4. **Scope.** Whole game, or a milestone (title + first N rooms/levels, no ending)? Default:
   a vertical slice first (title, 2 to 3 rooms with every object type of the game), then the
   rest room by room, `/ti-commit` per milestone.

## Round 2: fitting the machine (needs 3 and 4)

5. **Screen: 128×128 into 160×100.** Give the numbers: the visible fraction of a room, how
   many play-relevant cells fall outside, where the HUD goes. Options:
   - (a) 1:1 pixels, 128 wide centred (16 px bands left and right, free for a HUD), a
     vertical camera that follows the player inside the room (28 px hidden). Keeps the art and
     the physics; hides threats above/below (a falling player sees 50 px under them, not 100).
     Mitigation: a look-ahead (camera leads the vertical speed) or a dead zone.
   - (b) 1:1 and redraw the rooms 16 × 12 cells (the game redesigned: only for a remake).
   - (c) Scale 0.78 (100/128): destroys 8×8 pixel art and hit boxes; rejected unless the
     game is made of plain shapes.
   - (d) Rotated 90° (100 wide × 160 tall view of a 128×128 room: still crops): rarely better.
   Default: (a) for 128×128 single-screen rooms; for scrolling PICO-8 games (camera already
   moving) the same camera with a 160×100 window and the original camera logic adapted.
6. **Frame rate.** PICO-8 runs `_update` at 30 fps (`_update60`: 60). The runtime steps at
   256 / `RT_FRAME_TICKS` Hz (8 = 32 fps, 9 = 28.4). Options: (a) one cart update per runtime
   frame at 32 fps (6.7 % faster, exact logic), (b) `RT_FRAME_TICKS 9` (5 % slower), (c) a
   runtime extension alternating 8 and 9 ticks (30.1 fps average). For `_update60` carts:
   (d) two updates per frame at 30 fps (exact, double logic cost), (e) rescale every constant
   to 30 fps (no longer exact). Default: (a) for 30 fps carts; for 60 fps, (d) if the logic
   bench allows it, else (e) with the feel tests of `ti-port-sdl`.
7. **Numbers.** (a) `s32` 16.16 everywhere the cart keeps a number (exact; adds and compares
   are cheap, a 16.16 × 16.16 product is a 32×32 multiply: `__mulsi3`, ~10× a `muls`); (b)
   `s16` 8.8 or integers where the range and precision allow it (fast, traces diverge);
   (c) mixed: 16.16 for positions, speeds and timers that touch the physics, integers for
   counters, indices, flags. Default: (c), with the product of two non-constant 16.16 values
   listed and measured (most cart products are by a constant: shifts and adds).
8. **Controls.** PICO-8 has 4 directions + O + X (+ pause). Map O and X to two TI keys that
   are easy to hold together with the arrows (`K_A` [2nd] and `K_B` [shift] by default, as in
   `p8trace.py --map A=4,B=5`), and say which PICO-8 button is which in the game (Celeste:
   O = jump (4), X = dash (5)). Diagonals: the runtime reads the key matrix, so two arrows
   and two buttons can be held at once; whether the matrix ghosts with 4 keys down (two arrows
   + both buttons, Celeste's diagonal dash while holding jump) is to check once on the TI.

## Round 3: the look (needs 5)

9. **16 colours into 4 greys.** Show `sheet_grey.png` (luminance bins) next to `sheet.png`,
   and list the colours that carry gameplay information (the census `pal` calls; Celeste:
   the hair colour = dashes left, red/blue/green flash). Options: (a) one global table
   colour → grey (fast to try, often loses contrast), (b) per sprite group (the player, the
   enemies, the scenery each get their own mapping), (c) redraw. Default: (b), then a
   visibility check (white outline on the player, enemies, collectibles) on headless shots.
   Every colour-coded state needs a grey-readable equivalent (a pattern, a blink, a different
   sprite): ask for each one.
10. **Tiles: 8×8 cells on the TI.** The runtime's TileMap engine draws 16×16 tiles. Give the
    number of distinct 2×2 blocks of the map (`map.bin`, script) and the memory per option:
    (a) 16×16 metatiles made of 2×2 cells (counted; only works when the room offset is on a
    16 px grid), (b) pre-render each room into a background plane pair once per room (a
    room is static: 2 × 3840 bytes, a full copy per frame or dirty rectangles), (c) an 8×8
    tile renderer added to the runtime (measure first). Default: (b) for single-screen
    rooms (changes such as falling floors are redrawn as sprites over it), (a) for scrolling
    maps.
11. **Effects.** For each effect in the census: palette swaps (`pal`), fill patterns
    (`fillp`), screen shake (`camera`), particles, `circfill` hair, flashes. Keep, simplify
    (fewer particles, a sprite instead of circles) or drop, each with its measured cost.

## Round 4: the rest (needs 3 to 11)

12. **Sound.** The TI has none: drop it, or replace the sounds that carry information
    (a death, a pickup, a dash recharge) with a visual cue already in the game. Default:
    drop, keep the trace's `__sfx` list in the tests (it marks events).
13. **Random numbers.** The reference and the port draw the same numbers: both implement
    PICO-8's own generator (`p8shim.lua`, `p8_rnd`/`p8_rndi`), from `srand(0)`. Ask only
    whether the game's randomness is cosmetic (particles, clouds) or gameplay (spawns, AI): a
    cosmetic stream may be decoupled (its own cheap PRNG in both, not traced) to save cycles,
    at the price of a `--lib` override in the reference.
14. **Persistence.** `cartdata`/`dset` → `rt_save` (a save file); a game without save may
    still want one (Celeste: the timer, deaths, berries). Default: only what the cart saves.
15. **Frame budget plan.** The heaviest screen of the game (most objects, effects), the
    expected cost (`ti68k-performance.md` §7 and `runtime/README.md` costs), and the order of
    the fallbacks if it does not fit: redraw only what changes, fewer effects, then asm on the
    measured hot spot (**ask**, `CLAUDE.md`).
16. **Model.** Titanium only during the work, the TI-89 HW2 for a release (24,576-byte
    program limit on AMS 2: data files from the start if the build is near it).

The session is over when every answer is recorded and the user confirms the summary.
