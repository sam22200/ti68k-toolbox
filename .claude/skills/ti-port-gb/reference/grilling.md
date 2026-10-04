# The grilling tree for a Game Boy port

Run `/grilling` (skill `grilling`) with this tree after steps 1 and 2 of `SKILL.md` (ROM
decompiled, variables named, numbers measured), before any C. Find the facts yourself
(`info.json` census, `gbtrace.py` measures, VRAM dumps, level screenshots through the
injection door): the user decides, you never ask them for a fact. Every question carries its
data and a default. Rounds follow the tree. Record the answers in `games/<name>/README.md`
§ Port decisions (decision, reason, date).

## Round 1: the roots

1. **Source of truth.** Which ROM (title, header checksum, revision, region; `.gb` vs `.gbc`),
   and what else exists (a disassembly project on GitHub, e.g. pret-style `pokered`, a
   remake, other ports). Default: the ROM run under PyBoy is the reference; a community
   disassembly, when it exists, replaces Ghidra for reading (better names) but not the traces.
2. **Licence and repository.** Commercial ROM: the ROM, tiles, maps, level data and traces
   stay local; the repository gets our code, our spec, and build tools that regenerate the
   data from a local ROM (`make` fails clearly without it). Homebrew under a free licence:
   assets may be committed with attribution.
3. **Fidelity.** (a) Exact logic: every gameplay variable bit-equal to the ROM's trace (the
   default when the logic is small: one cartridge of game code, integer arithmetic), (b)
   **our own engine, behaviour-equal** (same numbers and feel, own code, tests on measured
   numbers only: **the default for a big game**, MBC banks, CGB only, many level types;
   `big-game.md` §1 gives the signs; say what (a) would cost, e.g. "2 MB of banked code"),
   (c) a remake.
4. **Scope.** All levels or a first slice (one level, one world) to finish end to end before
   the rest; title, high scores, attract mode, game over, ending. Default: one level end to
   end, then the rest with the same engine. A big game: propose the **first milestone** (one
   level or mission started directly by the injection door, no menus) and the roadmap after
   it (`ROADMAP.md`, `big-game.md` §2); the answer fixes milestone 1 only.

## Round 2: the machine (needs 3)

5. **144 rows into 100.** Give the measured layout (title bar, playfield, HUD rows; does the
   level scroll vertically?). Options: (a) the playfield 1:1 and the HUD re-laid out in the
   remaining rows or in a side panel (the TI is 160 wide like the GB: no side band unless the
   playfield is narrower), (b) a vertical camera on a taller playfield (scrolling games:
   following the player, look-ahead), (c) a HUD shown only on a key or on events, (d) scale
   (destroys the pixel art: rejected unless the game is plain shapes). Default: (a) when the
   playfield fits in 100 rows, (b) otherwise.
6. **Frame rate.** GB 59.73 Hz; the runtime ~30 fps (`RT_FRAME_TICKS2=17`: 30.1). Options
   (`asm-to-c.md` §5): (a) two exact updates per rendered frame, (b) one update with doubled
   constants, (c) half speed. Default (a) if the logic bench allows (`ti-cycles` on the
   update alone), animations then show every 2nd frame.
7. **Controls.** GB: D-pad, A, B, Start, Select. Map to the TI keys (`K_A` [2nd], `K_B`
   [shift], `C` and `D` for Start/Select, ESC = quit), say what each does in the game, check
   the combinations the game needs (two buttons + a direction) for keyboard ghosting once on
   the TI.

## Round 3: the look (needs 5)

8. **Shades.** The 4 GB shades map to the 4 greys 1:1 (`gb-facts.md` §3). Ask only about
   visibility: the main sprites get a white outline (mask dilated by one pixel), the LCD
   contrast of light grey on the TI vs the GB (a light-grey-heavy scenery may need its
   palette shifted), palette fades and flashes (BGP writes) as information (damage,
   invincibility): keep as a plane swap or a blink.
9. **Background.** Static single-screen levels: (a) pre-render each level into a plane pair
   once (2 × 3,840 bytes), sprites over it with saved backgrounds or a full copy per frame
   (Celeste: 100 byte-aligned rows copied, cheap). Scrolling levels: (b) the runtime's
   16 × 16 TileMap with metatiles (count the distinct 2 × 2 tile blocks), (c) an 8 × 8 tile
   renderer (measure first). Animated tiles and tiles the game rewrites during play (doors,
   counters) are redrawn as cells. Default: (a) static, (b) scrolling.
10. **Sprites.** Count frames × orientations × sizes (OAM dumps, `_tiles.png`), the memory as
    ExtGraph pre-shifted or plain masked sprites, the flipped variants (baked at build time
    vs drawn mirrored), the 10-per-line flicker of the GB (not reproduced: the TI has no such
    limit). Default: baked, masked, outlined, pre-shifted only for the sprites that move every
    frame if the budget needs it.
11. **Raster and screen effects** from the census (STAT/LYC splits, wobble, shake, fades):
    keep, simplify or drop, each with its cost.

## Round 4: the rest (needs 5 to 11)

12. **Sound.** None on the TI: drop; replace only the sounds that carry information with a
    visual cue (a blink, a HUD flash). The sound request writes stay in the traces as events.
13. **Randomness.** The game's own generator (often an 8-bit LFSR or a counter mixed with the
    divider register FF04: DIV depends on CPU timing, which the port cannot reproduce). Ask:
    exact (seed from a deterministic counter, both sides) or own PRNG for that stream (the
    traces then poke the same seed or skip the random parts).
14. **Persistence.** Battery RAM (high scores, saves) → `rt_save`; passwords stay passwords.
15. **Frame budget plan.** The heaviest level (most sprites, effects), its expected cost, the
    fallbacks in order: redraw only what changes, fewer effects, asm on the measured spot
    (**ask**).
16. **Model.** Titanium during the work, the TI-89 HW2 for a release (24,576-byte program
    limit on AMS 2: level data in a data file from the start if near it).

The session is over when every answer is recorded and the user confirms the summary.
