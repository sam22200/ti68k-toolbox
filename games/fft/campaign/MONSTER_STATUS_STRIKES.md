# Monster status strikes (formula 0x50, evadeable)

2026-11-11. Eye Gouge (Goblin: Blind), Poison Nail (Red Panther: Poison), Beak (Cocatoris,
Steel Hawk: Petrify). In the battle core, nothing in the companion: they are Secret Fist's
formula (`CORE_SECRET_FIST.md`) with the evade stage switched on.

## Source (`battle_formula_hit_ma_x_percent_physical_evade`)

- Records: range 1, a single target, flags_4 0x12 (0x02 evadeable, 0x10 Blade Grasp), an
  all-or-nothing status set with one status, X 45 / 40 / 37.
- **Evadeable**, unlike Secret Fist (flags_4 0) and the sword skills (`HOLY_SWORD.md`): the
  evade helper's final stage runs. Execution draws once per evade stage, in the order
  accessory, right hand, left hand, class, even when a stage's evade is 0, and stops at the
  first stage that evades; then one accuracy roll when the (MA + X)% chance is below 100.
- The forecast is the evade product times the accuracy (95% x 51% shows 48).

## Core

- `magic_hit` (`magic_evasion.inc`): formula 0x50 takes the physical evade stages, then the
  accuracy roll (execution) or the product (forecast). Secret Fist is unchanged: without
  `AF_EVADE` it never reaches the stages.
- `common_tables.status_strike_record`: the three records (kind status, `AF_EVADE`).
- Area AI: byte 5 of the primary record may be 2 for the monster strikes
  (`area_ai_supported`); the status utility is Secret Fist's.
- Only monsters use monster actions (`can_use`).

## Checks (`check_status_strike.c`, `tools/status_strike_oracle.py`)

The oracle runs the retail handler on the three records: MA, zodiac, Attack UP, Defense UP,
three evade fixtures (none; class evade; class + accessory + shield, attacker in front, the
target's job and items the same on both sides) and eight seeds: 288 profiles with the
preview chance, the hit, the number of draws and the status request. The check replays them
through the owned cast (`cast_forecast`, `cast_batch`): same forecast, same RNG state after
the same draws, same status; then production casts inflict Blind, Poison and Petrify.
PC = sanitizers = 68000: BC2F369D, 5,199 checks; Secret Fist's check is unchanged
(1FCD638D, 4,067).

## Cost

Resident TTPack input 57,103: headroom 7,905 -> 7,833 bytes; zone 8,698. Chapter I: 145
of 155 actions executable. The 37 campaign replays are unchanged.

## Next in the family

Black Ink (0x11E: a line with flags_4 0x20, direct trajectory) and Look of Devil (0x12D: a
random-one status set, type 0x40, flags_4 0x20) need the trajectory rule and the random
choice; Odd Soundwave (0x38) and Mind Blast / Death Sentence (0x3D, magical evade) use the
same status application.
