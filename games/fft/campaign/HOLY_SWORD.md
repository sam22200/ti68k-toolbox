# Holy Sword (formula 0x2D)

2026-11-10. Agrias's set (Stasis Sword, Split Punch, Crush Punch, Lightning Stab, Holy
Explosion), also used by the Chapter I Holy/White/Lune Knights. In the battle core,
nothing in the companion: it is a variant of the ordinary weapon strike (user rule,
`COMPANION.md`).

## Source (`battle_formula_damage_pa_times_wp_plus_y_status`)

Physical evade (Attack's stages), XA = PA, YA = WP + Y of the **equipped weapon**, the
weapon's element strengthen, the physical support/status/zodiac stages, a 4% critical with
knockback, the weapon's element Half/Weak, absorb/null, then the record's status at 100% on a
hit (status set type 0x20, one status). The pre-formula setup copies the equipped weapon into
`weapon_data`: the record's own element byte (Lightning Stab's lightning + holy) is unused.
A sword or knight sword must be equipped (flags_4 0x08, `main_unit_set_equipment_attributes`).

## Core

- `weapon_amount`: `awake` bit 2 puts the power on YA (Holy Sword) instead of XA (Charge).
- `sword_skill_amount` (resident, through `charge_query`, formula 0x2D): the primary weapon,
  strengthen, `weapon_amount`, then `magic_half_weak` (factored out of `magic_element_value`);
  `skill_kind` takes null/absorb from the weapon's element.
- 0x2D is a magic-pipeline formula (`cast_rules.h`): CT-0 cast, physical evasion (AF_WEAPON),
  `magic_prepare` draws the critical then its amplitude, knockback on a single-target
  critical, `EFFECT_STATUS` from `magic_status_value` (raw status set, type 0x20 accepted).
- `can_use`: flags_4 0x08 needs a sword/knight sword in a hand.
- `common_tables.holy_sword_record`: single target, radius 1, or Holy Explosion's line
  (flags_1/flags_2 0x15, range 5, length 5), exactly one status.
- Holy Explosion reuses Earth Slash's verified cardinal line (`CORE_LINE.md`: footprint from
  the caster, caster excluded, cardinal selection, distance order); `area_targets.inc` and
  `cast_area_supported` accept formula 0x2D lines.
- A single-target critical sets `st.dmg` before `knockback` (lethal test, fall damage added
  to the amount); in an area the critical goes to `st.crit` for the batch.
  Stasis Sword's former adaptation (no evade, no status, own splash) is replaced.

**Adaptation:** in an area a critical pushes only a sole line victim (as Earth Slash);
radial areas (Stasis Sword, Lightning Stab) and several line victims are not pushed.

## Checks (`check_holy_sword.c`, `tools/holy_sword_oracle.py`)

The oracle runs the retail 0x2D handler (dispatch table entry asserted) with five real
swords (two elemental), Attack UP, five target statuses, six element modes and spread seeds:
912 profiles, damage/healing/critical/draws. The check replays each profile's draws through
the core (`charge_query`, `skill_kind`) and runs production casts (sword gate, Death
Sentence, Dead, Silence at 100% on a hit; Holy Explosion's footprint, cardinal gate,
status on a hit and critical push): PC = sanitizers = 68000, hash DE3618E6, 8,376
checks (the oracle header packs 14 bytes a row to fit the 68000 check program).

## Cost

Resident TTPack input 56,929: headroom 8,415 → 8,007 bytes; zone 8,692. Chapter I: 132 of
155 actions executable (Stasis Sword, Split Punch, Crush Punch, Lightning Stab added).
Holy Explosion: 133 of 155; resident input and zone unchanged (56,929, 8,692).

## Next

The knockback of area criticals if the batch learns per-victim pushes. `weapon_skill_amount`'s AF_AREA
splash in `action_resolve.inc` no longer serves Stasis Sword (still used by Night Sword 0x30).
