# Equipment steals: Steal Helmet, Armor, Shield, Weapon, Accessory

2026-11-01. The Thief's five equipment steals (formula 0x26), after Magic
Break. Production: 132 normalized records, 87 casts, 126/201 generic action
entries (75 remaining entries are not 75 effects).

## Source behavior (decompiled MIPS, `sources/fft_decomp`)

- `battle_formula_steal_equipment`: `battle_formula_apply_steal_break_might_sword_hardcoding`
  picks the slot as for the breaks, plus the accessory (0x72). A monster or an
  empty slot is a forced failure: no evasion roll, no Attack (unlike the breaks).
- Then physical evasion, `battle_formula_store_speed_and_x` (XA = Speed, YA =
  X) and `battle_formula_calculate_physical_status_accuracy`: the same chain as
  Steal Exp (Attack UP, Martial Arts bare-handed, target statuses, zodiac),
  (XA + zodiac(X))%, one roll; `battle_formula_apply_maintenance` then fails a
  hit on a Maintenance target.
- `battle_action_remove_broken_or_stolen_equipment` removes the item as a break
  does (a lost weapon ends a Charge);
  `battle_action_increment_item_quantity_for_steal_break` adds it to the
  inventory when the thief is on the player's team, or adds price / 4 gil when
  99 are already owned (worn ones included).
- Range 1, vertical 2 (the record's own range, not the weapon's).

## Implementation

- `steal_equip_record` (ids 0x6e..0x72): new AB_STEAL_EQUIP, X in the record's
  element byte, AF_EVADE, the source formula id.
- `equip_break_slot` maps the steals onto the breaks' slots and adds the
  accessory; `equip_break_percent` adds the attacker's WP for the breaks only,
  so a steal's percent is Steal Exp's `punch_strike` path (Speed).
- COMMAND: nothing to steal fails before evasion; otherwise evasion, the roll,
  then Maintenance; the slot rides in `st.dmg`. ACTION's commit records the item
  in `st.stolen` when the thief is a player unit, then runs `equip_loss`.
- The SAVE image's `save_party` adds `st.stolen` to the inventory with the
  source's 99 rule (price / 4 gil). Losing the battle saves nothing, as a
  reload in the original.
- Forecasts and numbers print "Steal" (0% with nothing to steal). The scalar AI
  does not choose steals (same policy as the breaks).
- `charge_query` now tests its seven `punch_strike` formulas with one 16-bit
  mask: the resident shrinks by ~110 bytes.

## Room

Images: NUMBERS 8148 (the zone, +40), COMMAND 8106, AI 8110. Production
headroom 313 (251 after Magic Break), thanks to the `charge_query` mask.

## Not modelled

No monsters natively (their forced failure is source only); unremovable items
(none in the generic gear); more than 11 steals in one battle keep only the
first 11 for the inventory. The 99 rule's gil path is not exercised by a test.

## Evidence

`make core-steal-equip-check`: 320 original profiles (`tools/steal_equip_oracle.py`;
315 native) compared on the slot, the item, the percent and Maintenance; a
production steal of each kind with its bonuses lost and the item kept; an enemy
thief (item lost, not gained); a lost weapon's Charge; nothing to steal
(forced failure, no Attack, forecast 0%); Maintenance over many rolls; range 1;
Silence does not block, Frog does; no AI use. 4478 PC/sanitizer/68000
assertions, F66BB1DC. `make save-test`: a stolen accessory reaches the
campaign inventory.
