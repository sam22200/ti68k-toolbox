# Chapter I monster battle: Sweegy rules and campaign persistence

2026-11-12. The first complete monster battle replay runs on PC and under
ASan/UBSan. It uses the production `game_update`, CT clock, movement, planner,
orders, impacts, KO/outcome and campaign save. No action, target, HP, CT or
victory is injected during the fight. The headless driver acknowledges a
completed HUD (`hud_ok`); it does not draw or certify the artwork.

## Source and fixed test inputs

The local disc supplies MAP074's actual 11x12 collision columns, both terrain
layers and surface types, and ENTD180's eight actors: Delita, Algus, two Goblins,
a Black Goblin, two Bombs and a Red Panther. Their jobs, primary sets, original
coordinates/facings and reward fields are retained. The five party members
come from an isolated FSV4 campaign save, giving 13 active actors. The two
unselected reserve records remain in the save.

Private FCD4/FGS2 descriptors and interface records reuse the existing loaders.
Generated commercial bytes stay under `x/chapter1-monster-battle/`; the pilot
banks, installed groups and the live emulator are untouched.

Generation is deliberately frozen: west-side party deployment, fixed levels,
Brave/Faith60 and midpoint raw-stat generation rolls. Monster raw stats use the
original monster generation table, zodiac12 and no equipment. Human guests have
their primary actions enabled; monsters use the core's ordinary learned mask7,
including the source list's holes. These are native integration fixtures, not
original encounter-generation RNG or PS1 AI traces.

## Replays and assertions

| Case | Party/enemy level | Frames | Actions / monster actions | Outcome |
|---|---|---:|---:|---|
| ordinary | 3 / 3 | 14,697 | 43 / 16 | victory |
| ordinary, second spread seed | 3 / 3 | 15,257 | 43 / 15 | victory |
| outmatched | 1 / 12 | 12,041 | 28 / 15 | defeat |

All three PC/sanitizer traces and final save bytes match. The replays cover
Eye Gouge, Turn Punch and Self Destruct, including the latter killing its
caster. Successful encounters settle their result pages, progress flag,
location, funds and party EXP/JP. Unselected reserve records stay byte-identical
through compaction; repeated receipt cannot pay again. Defeat leaves the input
save unchanged. Movement, HP and status events are retained in each case's log.

## Bugs found and fixed

- `fixed_units` did not call the shared status initializer. A Bomb loaded through
  `UNIT_RAW` therefore lacked its job's innate Float. Fixed encounters now seed
  statuses after statistics, like the ordinary generation/party paths. The new
  assertion fails on both Bombs before the fix.
- `save_campaign` only paid result rewards at location7 (Gariland). Sweegy's
  first complete replay won and prepared1800gil but saved no reward. Receipt now
  applies to persistent-party campaign battles with prepared rewards, retaining
  the progress-flag duplicate guard and excluding nonparty flashbacks.

Both changes are cold initialization/final settlement work. No per-frame
algorithm, rendering, hardware path, allocation envelope or save format changes.

## Selected native evidence

Only one actual68000 run is needed for the changed PARTY/SAVE boundaries:
`check_monster_boot.c`,45 checks/C024ACA4. It verifies all six monster stat
records, innate Float, non-Gariland reward persistence and duplicate prevention.
Its saved bytes equal the PC carrier's bytes. It does not replay the full fight
or draw pictures on the calculator.

Only SAVE4 and PARTY8 images change: logical sizes8094->8104 and8214->8222.
The other26 images are byte-identical. ABI86 and zone8698 remain; production
TTPack input57733 leaves7203 bytes. Private rebuilt shards are63782/61710/57470.

Affected PC controls also pass: Float525 checks with sanitizers and the pilot's
Gariland81/Orbonne14 flow checks with their unchanged AE9CFD31/2CD70391 hashes.
The prior103-target sweep is reused, not repeated. No TiEmu run was needed.

## Commands and limits

```sh
rtk proxy make -C games/fft/campaign chapter1-monster-battle-check
# Only when the native initialization/save boundaries change:
rtk proxy tools/pyenv/bin/python games/fft/campaign/check_monster_battle.py --native
```

The default target runs PC plus sanitizers. `--quick` and `--case` are development
controls; `--quick` does not claim sanitizer coverage. The optional native run
builds private archives. Results, source fixture hashes, traces, native evidence
and before-fix failure logs are in `x/chapter1-monster-battle/`.

This closes the first complete monster **rules/save** replay gap. Sweegy scenery,
monster actor banks, event dialogue/objectives and deployment content are not
shipped or visually reviewed by this fixture. Exact original AI choices, the
remaining RAM margin, the final archive census and the one-command installed
chapter flow remain separate Chapter I readiness work.
