# Roadmap: Metal Gear Solid (GBC) on the TI-89

The game is too big to port in one go (2 MB, CGB, MBC5: VR Training with its many missions,
the story's stages, codec, items, weapons, bosses). It goes milestone by milestone: each one is
playable end to end on the PC and the Titanium, tested, and committed before the next. Every
milestone follows the skill's big-game loop: **measure on the ROM → spec in README → engine →
tests on the numbers → PC → TI (ti-cycles, xcheck, tihash) → art review (one PNG) → emulator
once**.

## Milestones

| # | Milestone | Content | Needs (new) | State |
|---|---|---|---|---|
| 1 | **VR Sneaking Practice Lv.01** | one level, Snake (8 directions, walls), one guard (patrol, vision), spotted → MISSION FAILED, goal → clear | measured map, collision, patrol, cones; ROM art in 4 greys | **done** 2026-10-04 |
| 2 | VR Sneaking Lv.02 to Lv.05 | several guards per level, other patrol shapes, the guard's chase after an alert, corner sliding | **decode the level format from the ROM** (the loader that fills WRAM5 `D000`, the guard table) instead of measuring each level; patrol scripts as data | next |
| 3 | VR menus and progression | title, MODE SELECT → level select, the intro pan to the goal, briefing texts, time and best times, completion %, save | the ROM's font and texts (WRAM2 `D800`… holds the strings), `rt_save` | |
| 4 | Snake's actions | crawl, wall press (hugging), knock, punch/throw; the guards' reactions (hearing a knock) | measure each action's buttons, timings and boxes | |
| 5 | Weapon mode | Five-seveN: aim, bullets, guards hit and down; ammo | projectiles, damage, more sprites | |
| 6 | The rest of VR Training | every Sneaking / Weapon / Advanced level from the decoded data, Time Attack | data volume: levels in the data file, maybe several files | |
| 7 | Story, first area | the opening rooms: room transitions, radar, items, codec screens, the HUD | rooms graph, item system, codec text | |
| 8+ | Story, area by area | bosses, cut-scenes kept or summarised (decided per area) | | |

## Open questions (to decide when the milestone starts)

- Milestone 2: does the vision stay "only on screen" (the ROM's rule) once the TI view is 100
  rows instead of 128? So far yes (fair: a guard you cannot see cannot see you).
- Milestone 3: the 4-grey palette per level (floors and walls change colour per VR theme).
- Milestone 7: the GB HUD (radar, item, weapon, life) into 160 x 100: overlay or a toggled panel.
- Art: keep the converted ROM art or redraw (the `ti-art-refs` banks) once the engine is stable.

## Rules for every milestone

- No screen comparison against the ROM beyond one review PNG per milestone: the port's screen
  is different by design. Logic is checked on measured numbers and key scripts, TI = PC by
  state hash.
- Measure, do not guess: each number in `README.md` comes with how it was measured.
- The data stays local (commercial ROM): `tools/extract.py` rebuilds it from the local ROM.
