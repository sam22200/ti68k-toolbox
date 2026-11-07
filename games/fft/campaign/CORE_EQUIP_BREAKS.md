# Equipment breaks: Head, Armor, Shield and Weapon Break

2026-11-01. The Knight's four equipment breaks (formula 0x25), after the stat
breaks. Production: 127 normalized records, 87 casts, 121/201 generic action
entries (80 remaining entries are not 80 effects). The Knight's set is now
complete except Magic Break (0x2c), which keeps its Orbonne adaptation.

## Source behavior (decompiled MIPS, `sources/fft_decomp`)

- `battle_formula_apply_steal_break_might_sword_hardcoding` picks the slot by
  ability id: the helmet, the armour, the right hand's shield then the left
  hand's, or the weapon of the higher required level (the right on ties). A
  monster or an empty slot: `battle_action_switch_ability_to_default_attack`,
  the ordinary Attack instead (no evasion or hit roll of the break).
- `battle_formula_break_equipped_hit_pa_wp_x_percent`: physical evasion, then
  XA = PA, YA = WP + Y (the attacker's weapon power) through the physical
  accuracy (Attack UP, Martial Arts bare-handed, target statuses, zodiac):
  (XA + zodiac(YA))%, one roll; `battle_formula_apply_maintenance` then turns a
  hit on a Maintenance target into a forced failure (no draw).
- `battle_action_remove_broken_or_stolen_equipment`: the item is removed, the
  target's stats refreshed; losing a weapon removes Charging from a unit
  charging a Charge.

## Implementation

- `equip_break_record` (ids 0x8a..0x8d): new AB_EQUIP_BREAK, Y in the record's
  element byte, AF_EVADE, the weapon's reach (`aim`, as Attack).
- FCJ's battle section gains the items' required levels after Charge's labels
  (`ITEM_LEVEL`, 256 bytes, archive only; the validator's minimum follows).
- `equip_break.inc`: the slot (natively one item per hand, a shield only on the
  left) and the percent through `punch_strike`, which adds the WP passed as its
  roll (X = 0: a multiplier of 1). Compiled in FORMULA/NUMBERS (forecasts) and
  COMMAND only.
- COMMAND: an empty slot returns 3 before any cost and `skill` runs `attack`;
  otherwise evasion, the roll, then Maintenance; the slot rides in `st.dmg`.
- ACTION's commit runs `equip_loss` (`equip_loss.inc`) and asks for the turn
  order again: the slot emptied, the item's HP/MP/PA/MA/Speed/Move/Jump bonuses
  taken off the single stat values (HP/MP clamped), a lost weapon ends its
  Charge. A player's broken item is gone from the campaign save.
- Forecasts print "Break"; an empty slot forecasts the Attack. The scalar AI
  still does not choose breaks; its rejection now precedes the hit evaluation
  (`ai_score`, same choices), so the AI image carries no break code.

## Room

ACTION would have grown to 8668 bytes, the zone with it. Instead:
- the resident's `formula_call` sends formulas 0x25..0x2b (breaks, steals,
  Talk) to FORMULA, and ACTION's evaluator copy keeps only the amount: the AI
  and ACTION copies carry no break or Steal Exp code (ACTION 8162 -> 7314);
  a player forecast made while the AI or ACTION image is loaded therefore
  never reads a copy without these formulas;
- the cold `skill_range` moves from COMMAND (op 2) to TARGETS (op 5).
Zone 8246 -> 8108 (AI), resident +~80, production headroom 255.

## Not modelled

No monster units natively (all human: their default-Attack rows are source
only); unremovable items (none in the generic gear); equipment-granted statuses
are not refreshed; the base/effective stat split near caps (one value per stat).

## Evidence

`make core-equip-breaks-check`: 228 original profiles (`tools/equip_break_oracle.py`;
224 native) compared on the slot, the item, the percent and Maintenance; a
production break of each kind with its bonuses lost; a lost weapon's Charge;
the default Attack forecast and executed; Maintenance over many rolls; weapon
reach; Silence does not block, Frog does; no AI use. 3255 PC/sanitizer/68000
assertions, 0AF4C46E.
