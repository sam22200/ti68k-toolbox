# Talk Skill: Persuade, Praise, Threaten, Preach, Solution, Death Sentence, Insult, Mimic Daravon

2026-10-30/31. The Mediator family after CORE_PUNCH_ART. Eight of the ten
Talk Skill entries (formula 0x2a) are enabled as immediate owned area casts
(no CT, no MP); Death Sentence followed with its turn countdown. Production:
116 normalized records, 83 casts, 113/201 generic action entries (88
remaining entries are not 88 effects). Invitation (recruitment and guest
identity) and Negotiate (Gil) stay gated; Secret Fist (0x50: physical evasion
then a percentage status) also inflicts Death Sentence and stays gated.

## Source behavior (decompiled MIPS, `sources/fft_decomp`)

- `battle_formula_talk_skill_hit_ma_x_percent`: a sleeping target is a forced
  miss before any draw; a monster target needs Monster Talk; Finger Guard may
  cancel; then hit (MA' + X')% with MA' = zodiac(MA) and X' = zodiac(X):
  neither Magic AttackUP nor Faith. The draw is skipped at 100%.
- `battle_formula_apply_talk_skill` dispatches by ability id: Persuade sets
  the CT change to "zero", Praise/Preach add Y to Brave/Faith, Threaten/
  Solution remove Y, Negotiate steals Gil, the others apply their status set
  (Insult: Berserk; Mimic Daravon: Sleep, radius 1).
- `battle_unit_apply_stat_increment_decrement` clamps CT 0..255, Brave and
  Faith 0..100; Brave below 10 sets Chicken (CORE_CONTROL_STATUS).
- Ranges: 3 panels, vertical tolerance 6, the user may be a target; Mimic
  Daravon's radius includes the user when adjacent.

## Implementation

- `talk_record` (common_tables.py, ids 0x74..0x7d) chooses the kind by id:
  Persuade AB_QUICK, Praise/Threaten AB_BRAVE, Preach/Solution AB_FAITH (new),
  Insult/Mimic Daravon AB_STATUS. Brave/Faith records carry a signed loss in Y
  (Praise/Preach: 256-4), so Foxbird, Threaten and Praise share one commit.
- 0x2a joins `magic_formula` and `magic_status_formula`. `magic_xa` skips the
  support stage for Talk; the Faith product is 10000. `magic_hit` returns 0
  for a sleeping Talk target before the accuracy draw.
- AB_QUICK's commit now writes the prepared CT (Quick 255; Persuade's 0x100
  marker, non-zero for the packet, writes 0). AB_FAITH clamps 0..100.
- Numbers, forecasts and the area panel print Brave/Faith gains and losses
  (Br+4, Fa-20) and "CT 0" for Persuade.
- Immediate area AI uses Insult and Mimic Daravon (existing hostile status
  values); Persuade and the Brave/Faith speeches are not AI choices (their AI
  flags are outside the accepted status sets).

## Death Sentence

- `battle_status_build_upkeep_action`: a unit with Death Sentence decrements
  timer slot 15 at its upkeep; at 0 the status is removed and, unless Undead,
  Dead is inflicted (return 3: the Brave +1 below 10 is skipped). Undead only
  loses the status and continues the upkeep. The timer is set at infliction
  from the status rules and an existing counter is kept (`core_status_timer_set`).
- Native: `status_upkeep` (FORMULA15, end of turn, before Brave +1) runs the
  same countdown; expiry uses the ordinary HP KO lifecycle (`status_knockout`,
  Dead's cancellation row). No resident bytes; FORMULA image 8142.
- Observed: the source upkeep draws one `rand()` per living unit's turn; the
  native living upkeep draws none (pre-existing; KO-unit upkeep draws match).
- 12 original upkeep traces (undead, counter 1..3, Brave 5/50) and a
  production cast that kills after the rules' three turns.

## Not modelled

Monster targets (no monsters in the current content) and Finger Guard (no
native reaction producer); the two gated entries.

## Budget

Resident +96 bytes (magic_xa/magic_heal/magic_hit), zone 8182 (AI image).
Scratch raw build 64709, headroom about 227. No State, ModCtx or save change.

## Evidence

`make core-talk-skill-check`: 301 original formula/finalizer profiles
(`tools/talk_oracle.py`, controlled RNG; the native header keeps the profiles
with seeds below 6, as bytes, under the 68000 program limit), through
`cast_forecast`/`cast_batch` with draws; production commits (Brave clamp and
Chicken, Faith clamp, CT 0, Berserk with lost control, Mimic Daravon on the
user), the sleeper's drawless miss, gates and AI use. 3916 PC/sanitizer/68000
assertions, 707B1244; with Death Sentence (344 profiles, 12 upkeep traces)
4616 assertions, 4C2389A4.
