# Steal Exp

2026-11-01. The Thief's first steal, after Guts. Steal Exp (115, formula 0x28)
is enabled as a single-target physical skill. Production: 121 normalized
records, 87 casts, 115/201 generic action entries (86 remaining entries are not
86 effects). The other steals stay gated: equipment (0x26) needs an equipment
inventory, Gil Taking (0x27) war funds, Steal Heart (0x29) effective-team Charm.

## Source behavior (decompiled MIPS, `sources/fft_decomp`)

- `battle_formula_steal_exp_hit_sp_x_percent`: physical evasion first, then
  XA = Speed through Attack UP/Martial Arts, the attacker Berserk/Frog stage
  (unreachable natively: a Frog/Berserk unit cannot use skills), target
  Protect/Sleep/Charging/Chicken/Frog and the zodiac; the hit percent is
  XA + zodiac(X) (X = 70). `battle_formula_use_hp_damage_as_action_hit_percent`
  rolls it (one draw, none at 100% or more).
- `battle_formula_set_exp_stolen`: amount = min(raw Speed + Y, 100, target
  EXP), Y = 5; 0 is a forced miss. The attacker records +amount, the target
  amount + 0x80.
- `battle_action_handle_steal_exp`: the target loses the amount (floor 0, no
  level loss); the thief gains it (at most 255) and `main_unit_check_level_up`
  runs at once (100+: level +1, EXP 0; level 99 keeps 99). The ordinary
  post-action reward (`battle_unit_update_attacker_earned_experience`, 10 +
  level difference for a success) and JP follow as for any action.

## Implementation

- `steal_exp_record`: AB_STEAL_EXP (new), Y, range 1, vertical 2, AF_EVADE; X is
  kept in the record's element byte (no element for this formula).
- The hit percent shares `punch_strike` behind `charge_query`: Speed and X take
  the place of PA and (PA+Y)/2 in Repeating Fist's sum, with a multiplier of 1.
  The amount lives in the evaluator copies (`formula.inc`). The forecast is
  evasion x percent / 100, 0 when nothing can be stolen.
- `command.inc`: evasion, then the percent roll, then the amount. The ACTION
  commit calls `steal_exp` and the ordinary `action_rewards(0)`; the level check
  is the new shared `exp_gain` (also used by `award_rewards`, same results).
- Numbers and forecasts print "EXP-n". The scalar AI does not choose it
  (conservative policy, as for the stat breaks).

## Evidence

`make core-steal-exp-check`: 100 original profiles (`tools/steal_exp_oracle.py`;
supports, target statuses, zodiac, EXP and level edges, seeds) compared on the
percent and the amount; a production theft that levels the thief up and then
pays the ordinary reward; no forecast hit with nothing to steal; Silence does
not block it, Frog does; no AI use. 1038 PC/sanitizer/68000 assertions,
31ED7172. Evasion keeps Attack's native draw model (as for Punch Art).

## Budget

Resident +~110 bytes (`charge_query`), images AI 8228 (the zone), ACTION 8162.
Production headroom 17 bytes: the talk-skill and area-impact check binaries
exceed the 68000 program limit again. A further relocation is required before
the next family.
