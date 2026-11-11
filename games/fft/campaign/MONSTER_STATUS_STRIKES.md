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

## Formula 0x3D: Mind Blast, the monsters' Death Sentence (2026-11-11)

`battle_formula_hit_ma_x_percent_magical_evade`: the status spells' (MA + X)% with the
magical support/status/zodiac stages but **without Faith**, then the status. flags_4 is 0:
never evaded (the checks keep evasive gear on the target). Mind Blast (Squidlarkin: range
3, radius 1) has a random-one set (type 0x40: Confusion or Berserk): on execution one status
is drawn after the accuracy roll, in status order, and a blocked choice is a miss; the
forecast treats the set as all-or-nothing (a hit when any status applies). Death Sentence
(Ahriman: range 3, one panel) has Secret Fist's one-status set.

Core: 0x3D is a magic status formula (`cast_rules.h`), without Faith and with the hostile
stages in `magic_heal`; `magic_prepare` draws the random choice; `magic_status_value`
accepts its random-one set for forecasts; `common_tables.monster_status_magic_record`
holds the two exact records. Oracle: 252 profiles (MA, zodiac, Magic Attack UP, Magic
Defense UP, statuses already present, Faith pairs that must not matter, seeds); the check
replays forecast, draws, hit and status, then production casts (Mind Blast leaves both
statuses over the seeds; Death Sentence its 3-turn countdown).

## Turn Punch (0x31, 2026-11-11)

The Black Goblin's Turn Punch is Spin Fist's record with area vertical 1 (the footprint
reads the raw record) and flags_4 0x01: never evaded, where Spin Fist (0x03) is.
`common_tables.strike_record` accepts it; `check_discovery.c` checks the same amount as Spin
Fist, the raw vertical, and a forecast of 100 against the gear that lowers Spin Fist.

## Numbers after Turn Punch and 0x3D

`make core-status-strike-check`: D0FD4866, 9,779 checks; `make core-discovery-check`:
FEFE4CC1, 17,811 checks; both PC = sanitizers = 68000. Resident input 57,109 (headroom
7,827), zone 8,698. Chapter I: 148 of 155. The 37 campaign replays are unchanged.

## Next in the family

Black Ink (0x11E: a line with flags_4 0x20, direct trajectory) and Look of Devil (0x12D: a
random-one status set with flags_4 0x20) need the trajectory rule; Odd Soundwave (0x38)
uses the same status application with a cancel set.
