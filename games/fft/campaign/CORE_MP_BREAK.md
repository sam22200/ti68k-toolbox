# Magic Break

2026-11-01. Magic Break (formula 0x2c) leaves its Orbonne adaptation (MA -3)
for the source rules, after the equipment breaks. The Knight's action set is
now complete. Production: 127 normalized records, 87 casts, 121/201 generic
action entries (unchanged: the record was already executable, adapted).

## Source behavior (decompiled MIPS, `sources/fft_decomp`)

- `battle_formula_physical_mp_percent_damage`: physical evasion, then
  `battle_formula_store_pa_and_y` (XA = PA, YA = Y = 50) and
  `battle_formula_calculate_physical_accuracy`, as the stat breaks: (XA +
  zodiac(YA))%, one roll.
- `battle_formula_calculate_mp_percent_damage`: MP lost = ceil(max MP x Y /
  100) (Y = 50), an MP damage result; the finalizer caps it at 999 and
  `battle_action_finalize_attack_and_flag_reactions` clamps MP at 0.
- Range 0 with the weapon-range flag: the weapon's reach, as Attack.

## Implementation

- `break_record` accepts 0x2c (id 0x8e): AB_BREAK, AF_EVADE, the source
  formula id kept (the generator no longer writes 0x2b for every break), Y in
  the element byte.
- The hit percent is the stat breaks' (`charge_query` -> `punch_strike`); the
  loss is `skill_amount` in FORMULA (16-bit: 999 x 50 fits; module images have
  no 32-bit division), and `formula_call` routes 0x25..0x2c there.
- COMMAND keeps the loss in `st.dmg`; the ACTION commit clamps MP at 0.
  Numbers and the forecast print "-NMP". The scalar AI still rejects breaks.

## Evidence

`make core-mp-break-check`: 74 original profiles (`tools/mp_break_oracle.py`:
supports, armed/bare hands, target statuses, zodiac, max MP 0..999 with low
current MP, seeds) compared on the percent, the MP lost and MP after; a
production break through the ACTION commit; the forecast; weapon reach;
Silence does not block, Frog does; no AI use. 790 PC/sanitizer/68000
assertions, C7D8B92A.

## Budget

Headroom 255 -> 251, zone 8108 (AI) unchanged.
