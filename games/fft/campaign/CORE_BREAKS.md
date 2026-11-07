# Stat breaks: Speed Break, Power Break, Mind Break

2026-11-01. The Knight's three stat breaks (formula 0x2b), after Steal Exp.
Power Break and Mind Break are enabled; Speed Break, an Orbonne adaptation
until now (range 1, vertical 6, no source hit percent), moves to the same
source rules. Magic Break (0x2c, MP) keeps its Orbonne adaptation (MA -3).
Production: 123 normalized records, 87 casts, 117/201 generic action entries
(84 remaining entries are not 84 effects).

## Source behavior (decompiled MIPS, `sources/fft_decomp`)

- `battle_formula_physical_stat_reduction`: physical evasion, then
  `battle_formula_store_pa_and_y` (XA = PA, YA = Y = 50) and
  `battle_formula_calculate_physical_accuracy`: physical supports (Two Hands
  x2 with a compatible weapon, Attack UP x4/3, Martial Arts x3/2 bare-handed
  only), attacker Berserk/Frog (unreachable: such a unit cannot use skills),
  target statuses, zodiac; hit percent XA + zodiac(Y), one roll (none at 100
  or more; the recorded percent stops at 100).
- `battle_formula_determine_reduced_stat`: by ability id, Power Break lowers
  PA, Mind Break MA, Speed Break Speed, by X (2 for Speed, 3 for PA/MA).
- The finalizer applies it with `battle_unit_apply_stat_increment_decrement`:
  at least 1.
- Range 0 with the weapon-range flag: the weapon's reach, as Attack.

## Implementation

- `break_record` (ids 0x8f..0x91): AB_BREAK, Y = X (the loss), AF_EVADE, the
  source Y in the record's element byte.
- The hit percent shares Steal Exp's `punch_strike` path behind
  `charge_query` (PA instead of Speed). Martial Arts now applies to both only
  bare-handed, as in the source (Steal Exp's profiles were bare-handed: its
  hash is unchanged).
- `aim` builds a break's reach with `attack_range` (the weapon's).
- `command.inc`: evasion, then the percent roll, then the loss; the ACTION
  commit picks the stat by ability id (floor 1).
- Numbers print "SP-2", "PA-3", "MA-3". The scalar AI still does not choose
  breaks (conservative policy).

## Not modelled

Two Hands is not a native support; equipment bonuses are not stored
separately (one value per stat), so the base/effective split near the floor is
not compared.

## Evidence

`make core-breaks-check`: 213 original profiles (`tools/break_oracle.py`;
the three abilities, supports, armed/bare hands, target statuses, zodiac, stat
floor, seeds) compared on the percent, the loss and the stat after; a
production break of each kind through the ACTION commit; the weapon reach;
Silence does not block, Frog does; no AI use. 2016 PC/sanitizer/68000
assertions, AE26B19D.

## Budget

Resident +~120 bytes, zone 8246 (+14, the evaluator copies). Production
headroom 185 after the chapter-loading relocation (CORE_ROADMAP.md).
