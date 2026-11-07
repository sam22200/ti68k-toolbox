# Gil Taking and Negotiate: shared battle funds

2026-11-01. Both gil producers are enabled together: Thief's Gil Taking
(0x6c, formula 0x27) and Mediator's Negotiate (0x7b, formula 0x2a).
Production: 134 normalized records, 88 owned casts, 128/201 generic entries.
The remaining 73 entries include upgrades/categories, not 73 different effects.

## Source behavior

OBSERVED in the local original MIPS formula/finalizer functions, executed under
Unicorn by `tools/gil_oracle.py`:

- Gil Taking uses physical evasion followed by the same Speed/support/status/
  zodiac accuracy chain as Steal Exp: `(Speed' + zodiac(200))%`.
- Negotiate uses Talk Skill's `(zodiac(MA) + zodiac(90))%`, without Faith or
  Magic Attack UP. A sleeping target does not listen and consumes no draw.
  The source Silence flag blocks Negotiate in its ordinary skill set;
  Gil Taking is permitted. Frog blocks both.
- Both call `battle_formula_calculate_stolen_gil`: amount is **raw Speed
  times the caster's level**, independent of MA, Faith, zodiac and support.
  A player target caps it to the available war funds. Zero forces a miss.
- The frozen target packet loses that amount; the caster packet gains it.
  `battle_action_add_war_funds` changes the balance only for the blue camp,
  clamping it to 0..99,999,999. Same-team transfers therefore have no net
  effect on the funds; enemy-to-enemy transfers do not change the player.
  Native TEAM_GUEST is an AI-control distinction within that camp: Delita's
  original Gariland ENTD record (0x184, slot 0) has team flags 0x84, camp 0.
  Guest casters gain funds and guest victims cap/lose funds like player units.
- A nonzero gil result counts as a successful pseudo-status effect for rewards.
  HP, MP, damage wake-up and Auto-Potion do not take part.
- Range/vertical limits are 1/2 and 3/6 respectively. Negotiate targets one
  hostile panel immediately, through the existing owned cast pipeline.

## Implementation

`steal_gil_record` accepts the two exact retail record shapes, producing
AB_STEAL_GIL while preserving the original formula and accuracy parameter.
Gil Taking uses COMMAND's existing physical percentage path; Negotiate uses
the shared Talk preparation and FORMULA status commit. The scalar and owned
packets freeze the amount before impact, and existing ownership prevents a
second application. Both forecast and number paths reuse the existing
"Steal" label and approved box/glyph renderer.

The balance lives in the **existing Save tail of the battle allocation**;
there is no second money field, Unit/State/Work growth, new allocation or save
format. PARTY11 loads the selected save's funds once at either ordinary or
scripted battle initialization (default 2000, with existing FSV2 migration).
Queries and commits read/write those bytes without file I/O or image loads.
Victory refreshes the save's roster/flags while preserving this live balance,
then applies ordinary item/result rewards. An unfinished or lost battle has
not written the changed funds to disk. Repeated saving does not add the theft
again. FSV3 remains compatible; module ABI 48 rejects older code banks because
the resident now requires PARTY11 and the new outcome.

The pure `magic_heal` callback also serves Gil Taking's amount query; modules
do not duplicate the money reader. COMMAND omits forecast-only percentage
branches which its own execution handler always intercepts. No image loads
another image while executing. The conservative scalar/area AI policy skips
both economy actions, matching the existing native equipment-steal policy.

## Room and verification

Production TTPack input: 64,725 bytes, headroom 211 (previously 313).
The static module zone is 8,186 bytes (NUMBERS); COMMAND is 7,996 and AI 8,118.
This is packaging evidence, not a complete frame/stack/RAM certificate.
Keep measured headroom before the next family; the larger native regression
carriers must also fit.

`make core-gil-check`: 3,262 assertions, digest **2020738D**, equal on PC,
sanitizers and 68000. It checks 134 original profiles, production learned lists,
forecasts without RNG or money writes, both preparations/commits, frozen
amounts, zero funds, caps, all four team combinations, one impact, unchanged
HP/MP, rewards, source Silence/Frog gates and the actual AI policy. PC,
sanitizers and 68000 must print the same digest. The wrapper also runs
`test_save.c` in a temporary data directory, including victory/reload,
uncommitted gains, enemy losses, repeated saves and the upper cap.

Current machine-readable evidence is in `x/core-gil/check/results.json`;
original function/input hashes and all signed packets are in
`x/core-gil/oracle/source.json`. The full 69-suite closure passes: all old
summaries agree except `actions` (D164D1B0, the two newly enabled records).
After the blue-guest correction, the seven direct suites pass again; both
Orbonne/Gariland native pilot flows retain their PC pixels/rules, settled
saves, no-double-payment and trophy-overflow results. See `closure.log`,
`final-closure.log`, `final-flow.log` and `verification.json`.

The two scoped preparation calls peak at 172,472 datasheet cycles; impact
peaks at 99,644. These include their current image/cache state and are not
complete rendered frames or hardware FPS measurements.

## Scope remaining

The original full AI's economic choices and Gilgame Heart reaction are not
implemented. Monster Talk/Finger Guard remain outside the supported human
producer scope. Charm/Invitation need effective allegiance and recruitment;
they must not be enabled just because their status bits or Talk dispatcher
exist. Other pending families remain listed in HANDOVER.md. Full core,
campaign/tooling and worst-case budgets are still open; Jump stays deferred.
The live TiEmu, standard bundle, user saves and historical proof artifacts
are not deployment targets of this work.
