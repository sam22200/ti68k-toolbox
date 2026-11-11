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

## Odd Soundwave (source 0x38 on the batch, 2026-11-11)

The Squidlarkin's Odd Soundwave runs the status application alone (formula 0x38): its
cancel set (Float, Reraise, Transparent, Regen, Protect, Shell, Haste, Faith, Reflect) on
radius 2 around the caster, the caster included, area vertical 1. No evade, no accuracy, no
draw; only present, non-innate statuses are removed, and nothing removed is a miss. The item
cleanses already use 0x38 outside the magic pipeline, so the batch knows this record under
the core id 100 (`FORMULA_AREA_CLEANSE`, `batch_formula` in `cast_rules.h`); the removal is
the existing cleanse path (`core_cleanse_pack`).

## Beaking (0x55, 2026-11-11)

The Cocatoris's Beaking: magical evasion (flags_4 0x12: the accessory's and the shield's
magical evade, one draw per stage), 0x3D's (MA + X)% without Faith, then PA - Y (2). In the
core it is a status formula using the Ruins' stat loss (`stat_loss_formula`,
`stat_loss_pa`, `RUINS.md`).

## Goblin Punch (0x4F) and Self Destruct (0x52, 2026-11-11)

Both deal the caster's missing HP (max HP - HP). Goblin Punch: formula 0x50's physical
evade and (MA + X)% accuracy, a single target. Self Destruct (Bomb): radius 2 around the
caster, never evaded, no draw; the others also get Oil at 100% (a blocked Oil does not stop
the damage; a full-HP Bomb deals 0 and still oils), and the caster takes its own HP: it
always dies. The area AI scores Goblin Punch as hostile HP; **Self Destruct is not offered
to the AI yet** (its sacrifice needs a scoring rule of its own).

## Numbers (2026-11-11)

`make core-status-strike-check` runs two programs (the 68000 size limit): strikes and 0x3D
D0FD4866, 9,779 checks; Odd Soundwave, Beaking, Goblin Punch, Self Destruct 88C99485, 3,237
checks; `make core-discovery-check` FEFE4CC1, 17,811 checks; all PC = sanitizers = 68000.
Resident input 57,285 (headroom 7,651), zone 8,698. Chapter I: 152 of 155. Secret Fist's
and the Ruins' checks and the 37 campaign replays are unchanged.

## Direct targeting: Black Ink, Look of Devil, Look of Fright (2026-11-11)

flags_4 0x20 (`DIRECT_TARGETING`). In the source the action's target map comes from the
weapon resolver with a direct trajectory (`battle_target_calculate_map_for_action` ->
`battle_target_validate_weapon_target`, `WEAPON_FLAG_DIRECT`): one panel, the aimed body or
the first body on the straight line, nobody when terrain is in the way. The record's line
and radius are not used (Black Ink has both).

- **Black Ink** (Squidlarkin, 0x11E): formula 0x50, Darkness; range 2 on the four axes
  (flags_1 0x10), vertical tolerance 2.
- **Look of Devil** (Ahriman, 0x12D): formula 0x50 with a random-one set (Petrify, Darkness,
  Silence, Don't Move, Don't Act); range 3. As for Mind Blast, one status is drawn after
  the accuracy roll, in status order, and a blocked choice is a miss.
- **Look of Fright** (Ahriman, 0x12E): formula 0x62
  (`battle_formula_lower_brave_y_without_faith`): Beaking's magical evasion and (MA + X)%
  without Faith, then Foxbird's Brave - Y (10); range 3.

Core: `cast_targets` (`area_targets.inc`) resolves a direct-targeting record with the
weapons' direct trajectory (`direct_trajectory.inc`, the panel variant already checked on
the original for guns, `check_direct_interception.c`). The trajectory stays in the weapon
planner image: the resident fetches the recipient (`cast_direct_fetch`, PLANNER op 6, the
pattern of `hamedo_line`) before every TARGETS entry, so the largest images do not grow
(a copy in TARGETS grew the zone by 1,100 bytes; the fetch costs 414 resident bytes); an unprepared query resolves to nobody. The
area AI scans many origins inside its image and holds its own copy (AREA_AI 4,108 ->
5,474 bytes, below the zone). `cast_area_supported` accepts the flag; the area preview
marks the recipients' panels. 0x62 is a magic status formula of kind `AB_BRAVE`; 0x50 takes
0x3D's random draw. Records: `status_strike_record` (the two exact 0x50 records),
`fright_record`. Area AI: the three are offered (AI byte 6: 0x40, direct); the query runs
from each candidate origin, so a blocked line is not chosen. `MOD_ABI` 86 (the new op and
three `Work` bytes).

**Adaptation:** an aimed panel without a unit resolves to nobody (the source traces to the
bare tile and can still hit a body on the way).

Checks (`check_status_strike3.c`, the third program): 432 original 0x50 profiles (MA,
zodiac, Attack UP, Defense UP, three evade fixtures, statuses already present, spread
seeds) and 68 original 0x62 profiles replayed (forecast, draws, hit, status or Brave
loss); the trajectory on a flat row (clear line, a body on the line takes the technique,
terrain leaves nobody, an empty aimed panel); production casts (Black Ink blinds, also the
interposed body instead of the aimed unit; Look of Devil leaves each of its five statuses
over the seeds; Look of Fright lowers Brave 70 -> 60). PC = sanitizers = 68000: 9807256F,
8,887 checks; the two other programs are unchanged (D0FD4866, 88C99485).

## Numbers (direct targeting, 2026-11-11)

`make core-status-strike-check`: three programs, D0FD4866 (9,779), 88C99485 (3,237),
9807256F (8,887), all PC = sanitizers = 68000. Resident input 57,733: headroom 7,651 ->
7,203 bytes; zone 8,698 (TARGETS 8,602). **Chapter I: 155 of 155 actions executable.** The
other families' checks (discovery, Ruins, Holy/Dark/Mighty Sword, Secret Fist) and the 37
campaign replays are unchanged.

## Open

No battle replay shows the AI choosing these techniques; Self Destruct is not offered to
the AI; a direct-targeting cast aimed at an empty panel resolves to nobody (adaptation).
