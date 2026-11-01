# Guts: Yell, Cheer Up and Scream

2026-10-31. Ramza's special set (chapters 2..4) after Spin Fist. Yell, Cheer Up
and Scream are enabled as immediate owned area casts (no CT, no MP, not
blocked by Silence). Wish was already enabled. Production: 120 normalized
records, 87 casts; Guts is not one of the 201 generic action entries (still
114/201, 87 remaining entries are not 87 effects).

## Source behavior (decompiled MIPS, `sources/fft_decomp`)

- Formula table entries (BATTLE.BIN, index = formula - 1): 0x39
  `battle_formula_add_speed_y`, 0x3a `battle_formula_add_brave_y`, 0x3b
  `battle_formula_add_brave_x_stats_y`. No hit roll and no draw: the
  pre-formula clear leaves hit 1 and accuracy 100; no reflect bit.
- Yell: Speed +Y (1), range 3. Cheer Up: Brave +Y (5), range 3. Scream:
  Brave +X (10) and PA/MA/Speed +Y (1), on the user (range 0).
- `battle_action_finalize_target_current_action` turns a Dead target (Undead
  included), a Petrified or a Wall target into a cancelled miss (miss type 8).
  An Undead or Chicken target is a normal hit.
- `battle_action_finalize_attack_and_flag_reactions` applies the changes with
  `battle_unit_apply_stat_increment_decrement`: Speed 1..50, PA/MA 1..99 on the
  base value, effective = min(cap, base + equipment bonus); Brave 0..100, or
  10..100 for a rider.

## Implementation

- `guts_record` (common_tables.py, ids 0x96..0x99) maps Yell to the new
  AB_SPEED and Cheer Up/Scream to AB_BRAVE. Y is a signed loss as for Praise
  (Yell -1, Cheer Up -5, Scream -10); Scream's stat Y is in the record's
  element byte (no element for these formulas).
- `guts_formula` joins `speech_formula` (Talk and Guts: the record's signed
  change, no support stage or Faith), hence `magic_formula` and
  `magic_status_formula` (the status batch commit). `magic_hit` returns a
  certain hit without a draw, as for formula 12.
- The status commit raises Speed (cap 50) for AB_SPEED; for Scream it raises
  Speed, PA and MA (cap 99), then Brave through `status_brave` (Chicken is set
  below 10, lifted at 10 and above).
- Numbers, forecasts and the area panel print "Sp+1"; Cheer Up and Scream
  print the Brave gain ("Br+5", "Br+10").
- AI: the immediate area AI does not accept their AI flags (as for Praise):
  not an AI choice.

## Not modelled

Equipment stat bonuses are not stored separately natively (one value per
stat), so the source's base/effective split near the caps is not compared
(9 recorded rows); no mounts, so the rider Brave floor is not compared (9 rows).

## Budget

Resident +70 bytes (records, speech/guts formula tests, the commit and three
labels). The FORMULA image grew to 8392 bytes, the module zone with it: the
production core no longer fit (the "headroom about 83" first noted came from a
stale `data/modules.h`). The module rebalance (CORE_ROADMAP.md, 2026-11-01)
restored it: zone 8094, headroom 235. No State or save change.

## Evidence

`make core-guts-check`: 117 original formula/finalizer/clamp profiles
(`tools/guts_oracle.py`; 108 native rows, Scream's KO/petrified user rows
skipped because that user cannot act), forecast and cast commits without a
draw, Chicken lifted by Cheer Up, the Speed cap on an enemy, Silence not
blocking, Frog blocking, no AI use: 1546 PC/sanitizer/68000 assertions,
F8F2AD6B.
