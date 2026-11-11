# Holy Sword (formula 0x2D) and Dark Sword (formula 0x2F)

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

- `weapon_amount`: `awake` bit 2 puts the power on YA (Holy Sword) instead of XA (Charge)
  and takes the raw PA whatever the sword (source `store_pa_and_weapon_power_plus_y`; a
  knight sword's Attack uses PA x Brave / 100, which Holy Sword does not: fixed
  2026-11-10, the check now runs at Brave 70).
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

**Knockback:** the source pushes only when the action has one target (`target_count` 1 in
`battle_formula_calculate_knockback`): the batch pushes a sole victim of any 0x2D/0x2F
footprint, never several.

## Dark Sword (0x2F, `battle_formula_absorb_mp_pa_times_wp`)

Physical evasion, XA = raw PA, YA = WP of the equipped sword (no Y, no element stage), the
physical support/status/zodiac stages and the 4% critical; the amount becomes the target's
MP loss and the caster's equal MP gain (`apply_mp_recovery`; not capped by the target's MP,
the commit clamps at 0). In the core: a magic-pipeline formula and a transfer formula
(`cast_rules.h`), the single-panel drain packets of formulas 15/16 (`cast_return`),
`sword_skill_amount` without the element, `common_tables.dark_sword_record` (exact record,
kind MP damage). **Adaptation:** a critical's push adds no fall damage to the MP amount.

The area AI scores Holy Sword as hostile HP and Dark Sword as MP damage
(`area_ai_supported`, like Spin Fist).

## Checks (`check_holy_sword.c`, `tools/holy_sword_oracle.py`)

The oracle runs the retail 0x2D handler (dispatch table entry asserted) with five real
swords (two elemental), Attack UP, five target statuses, six element modes and spread seeds:
912 profiles, damage/healing/critical/draws. The check replays each profile's draws through
the core (`charge_query`, `skill_kind`) and runs production casts (sword gate, Death
Sentence, Dead, Silence at 100% on a hit; Holy Explosion's footprint, cardinal gate,
status on a hit and critical push): PC = sanitizers = 68000, hash DE3618E6, 8,376
checks (the oracle header packs 14 bytes a row to fit the 68000 check program). Dark Sword:
318 original profiles (5 swords, supports, statuses, target MP 999 or 7, seeds) and a
production drain, in its own program (`check_dark_sword.c`, `dark.h`): 388C79A4, 3,225
checks, PC = sanitizers = 68000.

## Cost

Resident TTPack input 56,929: headroom 8,415 → 8,007 bytes; zone 8,692. Chapter I: 132 of
155 actions executable (Stasis Sword, Split Punch, Crush Punch, Lightning Stab added).
Holy Explosion: 133 of 155; resident input and zone unchanged (56,929, 8,692).
Dark Sword, the raw-PA fix and the AI filter: 134 of 155; input 56,965 (headroom 7,971),
zone 8,698. The 37 campaign replays are unchanged; no replay shows the AI choosing a
sword skill yet (not verified in a battle).

## Next

The knockback of area criticals if the batch learns per-victim pushes. `weapon_skill_amount`'s AF_AREA
splash in `action_resolve.inc` no longer serves Stasis Sword (still used by Night Sword 0x30).
