# Punch Art: Stigma Magic, Chakra, Revive, Wave Fist and Repeating Fist

2026-10-30. The first job family after CORE_PROTECT, in two batches. Support:
Stigma Magic (105), Chakra (106) and Revive (107) through the shared owned area
pipeline. Strikes: Wave Fist (102) and Repeating Fist (101) through the
single-target skill path, after a resident relocation. Production: 108
normalized records, 75 casts, 105/201 generic action entries (96 remaining
entries are not 96 effects). Spin Fist followed as an area cast (below). Earth
Slash (a line) and Secret Fist (0x50, physical then percentage Death Sentence)
stay gated.

## Source behavior (decompiled MIPS, `sources/fft_decomp`)

- Formulas 0x33/0x34/0x35 use PA as XA, Attack UP (x4/3) and Martial Arts
  (x3/2 when the strike's weapon is unarmed), then the zodiac
  (`battle_formula_apply_attack_up_and_martial_arts`). Punch Art has neither
  weapon-strike nor weapon-range flags, so `battle_action_store_used_weapon`
  always selects no weapon: Martial Arts applies whatever is equipped. No Faith.
- 0x33 Stigma Magic, `battle_formula_hit_pa_x_percent`: hit (PA'+X')% with
  X'=zodiac(X), then the status set (cancel, set 38). Radius 1 around the user,
  the user included. It also applies the attacker's Berserk x3/2 and Frog XA 1;
  natively unreachable: a Frog may only Attack or Frog, a Berserk only Attacks,
  and Punch Art has no CT. The source rows with those statuses are recorded in
  `source.json` but not compared.
- 0x34 Chakra, `battle_formula_heal_pa_times_y_and_mp`: heals PA'*Y HP and half
  that MP (Y=5), always hits, no Undead reversal; KO units are not healed.
- 0x35 Revive, `battle_formula_heal_y_percent_hit_pa_x_percent`: hit (PA'+X')%,
  then Raise's set and Y% max HP (rounded up); Undead reverses to damage, a KO
  Undead is a forced miss. Range 1, the user excluded.
- Ranges, effects, vertical tolerances and flags come from the ability range
  data; flags_2 0x10 (no follow), no Reflect bit (r[5] 0x80) and no MP or CT:
  Silence and Reflect do not apply, the cast is immediate.

## Implementation

- `punch_record` (common_tables.py) reads only 0x64..0x6b beyond the magic
  interval and maps the three formulas to AB_CLEANSE, AB_HP_MP_HEAL and
  AB_REVIVE with AF_CAST|AF_AREA records; Revive omits AF_SELF.
- `punch_formula` joins `magic_formula`; 0x33 joins `magic_status_formula`
  (the cancellation route). `magic_xa` takes the formula: Punch Art starts from
  PA, the support line chooses Attack UP instead of Magic AttackUP, Martial
  Arts adds x3/2; the Faith product is replaced by 10000. Revive shares
  formula 13's percent quantity, accuracy and KO applicability.
- `batch_commit` passes half the amount as the MP part (Chakra; no other
  batch producer has AB_HP_MP_HEAL). Area AI accepts Chakra's AI flags (0xc1)
  and values its HP part like a heal; Revive and Stigma use the existing
  revive/cleanse values.
- Attack UP and Martial Arts are new native support ids (0x1d1, 0x1d8); the
  ordinary weapon Attack still does not apply them (pre-existing gap).

## Budget (support batch)

Resident code grows about 112 bytes (the PA path inside `magic_xa`; a shared
Berserk/Frog helper cost more than it saved and was removed with the
unreachable rows). The module zone grows 26 bytes to 8156 (AI image).
Resident TTPack headroom about 330. No State, ModCtx or save field changes.

## Evidence (support batch)

`make core-punch-art-check`: 99 original formula/finalizer profiles
(`tools/punch_oracle.py`, controlled RNG; 75 reachable rows compared), through
`cast_forecast`/`cast_batch`, RNG draws included; immediate production casts
(Chakra on the user and a neighbour, Revive on a KO ally and an Undead,
Stigma's set removal), gates (the five other Punch Art entries, Frog, Silence,
Reflect) and immediate AI use. 1258 PC/sanitizer/68000 assertions, E18144C2 (1256, 6DAFCC6A, once
Wave/Repeating Fist left its gate list).

## Strikes (formulas 0x31/0x32)

- Source (`tools/punch_strike_oracle.py`, 276 original profiles, 14 criticals):
  Wave Fist deals PA' x (PA+Y)/2; Repeating Fist (PA' + zodiac((PA+Y)/2)) x
  1..X (X=9). PA' is PA with Attack UP/Martial Arts, then the physical support
  stage (attacker Berserk/Frog, target Protect, Sleep/Charging, Chicken/Frog),
  then the zodiac; a 4% critical adds rand x XA after the zodiac; 999 cap.
  Physical evasion is Attack's. A critical knocks back only a single target,
  so Spin Fist's area would have none. Defense UP is not modelled (as for Attack).
- Records: kind AB_DAMAGE, AF_EVADE, the source range and vertical tolerance.
  `punch_strike` sits behind the exported `charge_query` (no new ModCtx entry):
  formula.inc passes the random roll (AI 0x4000 = mid hits, forecast 0 = one
  hit); `command_effect` rolls the native evasion, the critical (knockback
  instead of the Brave knockback), its amount and Repeating Fist's hits.
- Draw order: the source has four evade draws and Wave Fist one status-proc
  draw after the damage; native evasion keeps Attack's one-or-two-draw model,
  so the check replays the source draws for the formula, not the evasion.
- `make core-punch-strikes-check`: 134 compared profiles, production hits,
  criticals with knockback, Repeating Fist multiples, gates, AI use: 4412
  PC/sanitizer/68000 assertions (1AFD349F).

## Spin Fist (2026-10-31)

- Formula 0x31 on an area: radius 1 around the user (AUTO), the user excluded
  (flags_2 0x01), evadable. `battle_formula_calculate_knockback` returns at
  once unless the action has exactly one target, so the area's criticals never
  knock back.
- Native: an AF_CAST|AF_AREA|AF_EVADE|AF_SELF record without CT. The area
  pipeline accepts 0x31 beside the magic formulas (`cast_area_supported`,
  `cast_targets`); `magic_hit` uses Attack's physical evasion for it (forecast
  `hit_chance`, execution `roll_hit`); preparation draws the critical and its
  amount, and the damage is `punch_strike` through `charge_query`. Area AI
  accepts its hostile-HP flags. Resident +74 bytes, headroom about 153.
- `core-punch-strikes-check` now replays 0x31 for Spin Fist too (its source
  status-proc draw included) and casts it: forecast mask without the user,
  hits, criticals without knockback, misses: 6524 assertions, BC005771.

## Resident relocation

- `chapter_grid` (surface profile, grid tables, per-tile heights/water/top)
  runs once per chapter load; it now compiles into PARTY op 10, after the
  resident `nav_reserve` (malloc and static buffer stay resident). PARTY grows
  6132 -> 6436, below the 8156/8170 zone. About 310 resident bytes.
- `FFT_PRODUCTION_CORE` (defined only by native_battle.c, the fftbat core)
  leaves out twelve exported entries that only tests and tools call
  (`battle_bind_grid/surface`, `battle_status_apply`, `cast_forecast`,
  `ai_plan/ranked/fallback/frozen`, ...). About 80 bytes. Check binaries include
  battle.c directly and keep them.
- Test doors 26..60 were never in the chapter core (non-chapter demo only).
- Totals: support +112, relocation -390, strikes +364 (zone 8170, AI image).
  Resident headroom about 323 (scratch raw build 64613).

## Not done

Punch Art animations; Spin Fist (an area of physical hits: per-victim
physical evasion in the area batch), Earth Slash's line and Secret Fist's
Death Sentence countdown. The full core is not complete; Jump
stays deferred.
