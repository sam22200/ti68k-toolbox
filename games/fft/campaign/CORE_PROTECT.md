# Protections: Reraise and Golem

2026-10-30. The protection family after CORE_CONTROL_STATUS. Reraise (7) joins
the shared status-spell compiler; Golem (65) adds AB_GOLEM and a per-side pool.
Production: 103 normalized records, 72 casts, 100/201 generic action entries
(101 remaining entries are not 101 effects). Float (38) stays gated: its
height/water/lava movement belongs with the deferred L1 movement work (Jump).

## Source behavior (decompiled MIPS, `sources/fft_decomp`)

- Reraise is formula 0x0b (friendly accuracy) with status set 33 (byte 2, 0x20).
  Death does not cancel it (Dead's cancellation row leaves 0x20).
  `battle_status_build_upkeep_action`: at a KO unit's turn, Dead without Undead
  and with Reraise removes Dead and Reraise and heals max HP/10. One upkeep draw
  is consumed. Below 10 max HP the heal is 0: Reraise is spent, the unit stays
  KO and its counter does not tick that turn. Undead with Reraise ticks the
  counter as before. The native hook runs first in `status_dead_turn` (FORMULA).
- Golem is formula 0x14 (`battle_formula_set_golem`): accuracy is
  (MA [+ Magic AttackUP] + X) x the caster's Faith / 100, without zodiac or the
  target's Faith; a hit writes `g_battle_team_golem[side]` = the target's (the
  caster's) max HP. `battle_action_finalize_target_current_action`: a primary
  hit on a side with a non-empty pool, by an action whose range data has
  BLOCKED_BY_GOLEM (0x08 in flags_3), becomes a Golem guard: no HP, status or
  knockback, the pool loses the damage (floor 0), the hit is absorbed whole.
  Only ability 0 (Attack) carries 0x08; Charge, Jump and Throw reuse its range
  data, items clear it. Natively: Attack and Charge. The pool lasts until spent
  and is reset with each battle's State.

## Implementation

- `State.golem[2]` (allies, enemies) is the only new battle field: State grows
  4 bytes, so MOD_SIG changes and older banks are rejected. Nothing is saved.
- The prepared packet carries AB_GOLEM with a marker amount; the commit writes
  the target's max HP. The guard sets `effect_kind=AB_GOLEM`, so numbers read
  "Golem" for both. `attack_prediction` is 0 against a guarded side.
- Immediate area AI: Golem is worth casting while its side has no pool; Reraise,
  Float and Transparent count as helpful statuses (allies).
- Golem's accuracy reuses the shared magic path exactly: its target is its
  caster, so the zodiac compatibility is neutral (source table entry 0) and the
  target Faith factor is replaced by 100. The side index is `team & 1`
  (player and guest 0, enemy 1).

## Budget

The end-of-turn status upkeep (Poison/Regen, Brave +1 below 10) now runs inside
FORMULA15, which the CT credit already loads at every turn end: the resident
gate disappears, the order (CT credit, then upkeep) is unchanged and a no-op
upkeep changes nothing. Resident TTPack input about 64493 (headroom about 443).
The area-impact native check binary sits near the 65514-byte program limit:
it now defines FFT_CHECK_NO_ZONES (engine bench zones off, its own 57/58 kept)
and is 65271 bytes. Production builds have no bench zones and are unaffected.

## Evidence

`make core-protect-check`: 28 original formula/finalizer profiles
(`tools/protect_oracle.py`, controlled RNG; Golem's caster-side rows), six
Reraise KO upkeep traces, production casts, the Golem pool and guard (partial,
exhausting and absent pools, unguarded enemy side), prediction and AI use.

## Not done

Golem's original summon animation; the guard's own miss presentation beyond the
"Golem" number; mounts. Float, Charm, Blood Suck and the remaining job families
stay gated. The full core is not complete; Jump stays deferred.
