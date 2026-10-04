# The grilling tree for a TI-83 port

Run after steps 1-2 of `SKILL.md` (program decompiled, notes written, data extracted). Find
the facts yourself; the user decides. Every question carries its data and a default. When
the user wants to move fast, ask round 1 and the screen question in one go and take the
defaults for the rest (say so in the README).

## Round 1: the roots

1. **Source of truth.** Which build (TI-83, 83+, 84+; shell), what else exists (source on
   request from the author, other ports). Default: the TI-83 build run under `ti83run.py`.
2. **Licence and repository.** Freeware with "do not decompile/edit" (Desolate): the
   program, its art, maps and text stay local; the repository gets our code, the spec and
   the build tools that regenerate the data from the local file. Open source: assets may be
   committed with attribution.
3. **Fidelity.** (a) Same rules, same rooms, same text, same art (the default for a game
   whose logic is small), (b) same rules with redrawn art, (c) a remake.
4. **Scope.** The whole game (the default when the logic is a few thousand instructions) or
   a first slice (the first rooms) end to end.

## Round 2: the screen

5. **96×64 into 160×100.** Measure the layout (playfield, HUD, frame). Options:
   (a) 1:1, centred, the rest of the screen for a re-laid HUD (tiny on the TI's LCD: the
   TI-89 pixel is smaller than the TI-83's; reads badly), (b) the art redrawn or scaled at
   1.5× (8×8 tiles → 12×12: 96×64 becomes 144×96, fits with a 2-pixel frame; scale with a
   pixel-art scaler then retouch the main sprites), (c) 1:1 tiles but a larger view of the
   world (only for scrolling games: more tiles visible), (d) ×1 with the HUD moved to the
   side band (32 px each side). Default: (b) for single-screen rooms (fills the screen, keeps
   the composition), (c) for scrolling games.
6. **Speed.** The original's main loop runs at an uneven rate (it shares the CPU with the
   grey interrupt). Measure its period in the runner (ticks per iteration at the game's
   default timer) and pick a fixed rate on the TI (one logic step per frame at ~30 fps, or
   every 2nd frame). Default: the feel of the original, measured, as a step per N frames.
7. **Controls.** The original's keys (arrows, 2nd, alpha, mode, X,T,θ,n, clear) → the TI-89's
   (arrows, 2nd = `K_A`, alpha = `K_D`, ◆ = `K_C`, shift = `K_B`, ESC = quit/menu, ENTER).
   Default: the same key names where the TI-89 has them (2nd, alpha, ESC for clear/mode).

## Round 3: the rest

8. **Visibility.** White outline on the hero and enemies; grey levels as in the original
   (dark buffer = dark plane); text in the runtime's fonts.
9. **Persistence.** The game's save → `rt_save` (one slot as the original).
10. **Timer prompt and shell artefacts** (interrupt-frequency menus, "press any key" for the
    LCD driver): dropped.
11. **Model.** Titanium during the work, the TI-89 HW2 for a release (program size).
