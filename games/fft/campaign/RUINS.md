# Ruins (formulas 0x1A, 0x1B): the Arc Knight's sword set

2026-11-10. Magic Ruin, Speed Ruin, Power Ruin, Mind Ruin (set 0x4B), used by the Chapter I
Arc Knights. In the battle core, nothing in the companion: they are magical variants of
families the core already has (the Dances' stat losses, the percent MP formulas).

## Source

- Both: radius 1, range 4, a sword required (flags_4 0x08), CT 3, 20 MP (record bytes).
- **Speed/Power/Mind Ruin, 0x1A** (`battle_formula_magical_stat_reduction`): a chance of MA + Y (Y 250, the zodiac on Y; MA through the magical support, status
  and zodiac stages) times both Faiths; at 100 or more there is no draw, otherwise one roll.
  On success the stat loses X (Speed 3, PA 5, MA 5; the stat is chosen by ability id).
- **Magic Ruin, 0x1B** (`battle_formula_damage_mp_percent`): the magic
  accuracy (MA + X, X 250, the same stages), then MP loses ceil(max MP x Y / 100), Y 50.
- **Never evaded (corrected 2026-11-11):** flags_4 is 0x08 without 0x02 (evadeable), so the
  magical-evade helper's final stage is skipped, as for the sword skills (`HOLY_SWORD.md`,
  same gate, proven there on the oracle). The records carry no `AF_EVADE`.
- The retail pre-formula setup fills the action's base hit (100) and Faith copies: the
  oracle sets them as it does.

## Core

- `cast_rules.h`: 0x1A and 0x1B are magic formulas, 0x1A a status formula (the status
  batch commit); `stat_loss_formula` / `stat_loss_pa` hold the Dances' and Ruins' Speed/PA/MA
  rule once, for `status_commit.inc` and the area AI (`area_ai_prepare.inc`).
- `magic_heal`: the percent path reads Y for 0x1A (X otherwise), hostile magical stages;
  0x1A's amount is X; 0x1B's is the max-MP percent (with formula 15's MP case).
- `cast_area_supported`: Ruins' kinds (Speed/Break, MP damage) on the batch.
- `common_tables.ruin_record`: the four exact retail records (kinds MP damage, Speed, Break).
- Area AI: the Ruins are scored like the Dances (stat losses) and as MP damage.

## Checks (`check_ruin.c`, `tools/ruin_oracle.py`)

The oracle runs the retail 0x1A/0x1B handlers (dispatch entries asserted) over MA, both
Faiths, zodiac signs, Magic Attack UP, target Magic Defense UP, Shell/Frog/Chicken, the
target's max MP and spread seeds: 528 profiles (hit, MP damage, stat change, accuracy,
draws). The check replays the chance, the draw and its result through the core, the MP and
stat amounts, then production casts (sword gate, the CT deferral, an immediate cast with
Non-Charge lowering Speed 8 -> 5, PA 12 -> 7, MA 11 -> 6, MP 300 -> 150).
PC = sanitizers = 68000: A43C2B25, 4,660 checks.

## Cost

Resident TTPack input 57,031: headroom 7,967 -> 7,905 bytes; zone 8,698. Chapter I: 142
of 155 actions executable. The 37 campaign replays are unchanged; the AI choosing a Ruin
is not yet seen in a battle.
