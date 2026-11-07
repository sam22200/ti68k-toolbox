# Archer Charge family and bounded action lists

Current dispatch2026-10-26: [CORE_DEFERRED.md](CORE_DEFERRED.md) replaces
recursive turn entry with iterative due-wave dispatch. ABI/layouts are unchanged.

Earlier extension2026-10-26: [CORE_CHARGE_HANDS.md](CORE_CHARGE_HANDS.md).
All eight powers support the verified two-hand subset, per-hand criticals and
both deferred weapon IDs, with shared potential forecast/AI. Source arithmetic
is checked separately from the older controller observations below. Broader
original weapon/range/projectile targeting remains open. ABI18; no State/Work/
Unit/save growth. The following2026-10-22 measurements are historical.

2026-10-22 continuation, working-tree review. This follows the source-ID migration
in [ABILITIES.md](ABILITIES.md). No new art, assembler or bulk RAM allocation.

## Original-game observations

`tools/charge_oracle.py` starts from `oracle.py`'s Gariland first-turn state.
It injects actor statistics, primary Charge, the eight learned bits and, in one
configuration, the target's Charging status. Every selection, preview and commit
uses controller input. It checks the actual committed ability ID and its first
charged CT; it does not force a countdown or the outcome.

All 48 configurations are replayed twice identically: each Charge with a sword,
dagger, bare hands, good compatibility, bad compatibility, and a charging target.
The local generated JSON/header are `x/charge/source.json` / `source.h`; neither
is committed. The state SHA-256 is stored with them.

| Source ID | Charge power | Initial CT |
|---|---:|---:|
| 0x196 | +1 | 4 |
| 0x197 | +2 | 5 |
| 0x198 | +3 | 6 |
| 0x199 | +4 | 8 |
| 0x19A | +5 | 10 |
| 0x19B | +7 | 14 |
| 0x19C | +10 | 20 |
| 0x19D | +20 | 35 |

The disc's two-byte entries at `0x80061044` are CT then power. Runtime CT lookup
now uses the source ability index, never `power-1`: the latter reads the wrong
entry for +7/+10 and outside the table for +20. All eight normalized records
are enabled. Fixed encounter draw masks still leave existing abilities unchanged.

The source preview also proves two corrections to the existing evaluator:

- Bare hands use `max(1, PA * Brave / 100)` as power, rather than 1. With PA 5,
  Brave 70 and neutral compatibility, Charge +1 previews 18, not 6.
- The target's Charging modifier precedes zodiac compatibility and its integer
  rounding. PA 7, Broad Sword, good compatibility and Charge +4 against a charging
  target previews 80, not 76.

Relevant source routines: `main_ability_calculate_pointers_and_type`,
`battle_action_prepare_attack`, `battle_formula_calculate_base_xa`,
`battle_formula_apply_charge`, `battle_formula_apply_physical_xa_modifying_statuses_support`.
No broad claim is made for other weapon types, support/status combinations,
retail critical rolls or new AI ranking fidelity. The oracle observes previews
and initial committed CT, not complete original delayed-hit trials.

## Menu safety and visual review

The existing 47-row window holds five framed ability rows (15 + 5*6 = 45), not
six. Drawing a larger list previously made the final copy read beyond it.
Long action lists now scroll over those same five rows. The selected index stays
global; the cache key stores the first visible action, and the hand uses its
visible row. Wrapping still reaches the first/last learned action. Lists of five
or fewer retain their previous pixels. No window or State/Work buffer grows.

`make charge-review` reproduces first/last selection at the same view and scale:
`x/charge/menu-review.png` (first/middle/last, local integer enlargement).
**The user approved five-row scrolling** and requested indicators for hidden
options above/below. Cached triangles now occupy the unused top/bottom margins;
they do not add a row or grow a buffer. On user feedback, the white backing
rectangles are removed and the bottom triangle is raised one pixel, leaving
one intact interior row above the bottom border. Their before/after review is
`x/charge/scroll-indicators-review.png`; **indicator approval is pending**.
Do not optimize, ship or build further presentation on the indicators until
approved. At the user's request, `x/fftcampaign.89g` was rebuilt before this addition
for manual TiEmu review: 33 variables, 430,593 bytes, with all variable blocks
and archive flags checked against the singles. It contains matching FCJ4
programs and banks. The user's live TiEmu runs that preceding review package;
it has not been restarted or driven during the indicator work.

The common bank is now **FCJ4**. This semantic version barrier rejects an older
engine whose Charge lookup assumed that power was a table index. FCD2 and FMG2
layouts are unchanged. Always rebuild all programs and banks together.

## Correctness checks

```sh
make -C games/fft/campaign charge-source-check  # repeats the actual original run
make -C games/fft/campaign charge-check         # shared test, PC and actual 68000
make -C games/fft/opening check
make -C games/fft/campaign check
make -C games/fft/campaign charge-review
```

`check_charge.c` checks the source previews/CT, both formula-module entry paths,
every power's CT scheduling, no time during a menu, another unit's intervening
turn, empty-tile resolution and interruption by KO/displacement/weapon change.
It also traverses the eight-action menu in both directions, wraps, checks the
window bound and selects Charge +20 through the real input handler.
Its digest now includes every visible byte of both cached menu planes at all
sixteen cursor steps: the first/middle/last continuation indicators match on
PC and actual 68000, hash **276A2090** after the backing/position correction.
Only the two indicator margins change in the three review images.
This is a correctness check, not visual
approval or a new performance measurement.

The existing 27 campaign and 18 opening/scene replays retain their frame counts,
screen checksums, state and saved bytes. No golden reference was refreshed.
The real uncompressed review program is **63,509 bytes**, leaving
**1,427 bytes** below TTPack's 64,936-byte limit. `MOD_ZONE` is
8,852 bytes; the module bank is 29,804 bytes. State/Work and the common bank
payload length are unchanged. No new hardware/emulator or performance claim is
made for the pending view.

## Next encounter facts

Read from the local disc and named source structures, rather than guessed IDs:
`EVENT/ATTACK.OUT` loads at `0x801BF000`; its 24-byte scenario table begins at
`0x801CF938` (`target/event.yaml`, `attack_load_scenario_conditionals`). The first
post-Gariland battle records, events 15..23, use **MAP085, ENTD 0x185, squad 257**.
This is the next Mandalia extraction door; it has not been integrated.

ENTD 0x185 has eight nonempty source records: Delita, Algus, a level-two Thief,
four level-one Squires and a level-one Red Panther. Its Thief set 0x0E includes
Gil Taking, Steal Heart/equipment/EXP. Red Panther job 0x67 uses monster set 0xB9:
Scratch 0x118 (formula 1), Poison Nail 0x11B (formula 0x50), an empty slot, then
Cat Kick 0x119 (formula 0x37). These actions must be checked before enabling them.

Before choosing Mandalia's native deployment, observe the running encounter's
active units and conditional story actors: eight ENTD records plus a player
squad can exceed the engine's fixed eleven-unit limit. Check generation, draw
ordering and data-derived counts; do not silently drop an actor to fit.

`tools/mandalia_preflight.py` now reproduces the live original deployment-table
read: squad 257, MAP085, limit four (record `a05700000601001204550101`). Eight
populated ENTD records plus four player slots gives a declared maximum of twelve;
conditional active actors still require an actual Mandalia observation. Full
remaining core work and priorities: [CORE_STATUS.md](CORE_STATUS.md).
