# What HW3Patch allows, and what FFT does with it

2026-11-10. HW3Patch is required (`CHAPTER1_READINESS.md` § 4): the Titanium runs code from
any RAM block. Used today for one thing only: the module zone is a heap block (8,743 bytes
of program headroom, RAM unchanged). This note studies code shipped in chapter packs and
parks the other uses until a need appears.

## 1. Code in chapter packs (study)

**Goal.** Add abilities a chapter needs (monsters, special character sets, later bosses)
without rebuilding or unfreezing the core: the chapter pack carries the code.

**How much code Chapter I actually needs** (`tools/chapter_audit.py`, original formula id of
each missing action, against the formulas the core already evaluates):

| | Formulas | Actions |
|---|---|---|
| already in the core: tables and gating only | 01, 0B, 27, 2D, 31, 36, 37, 38, 50 | ~28 (monster Tackle/Bite/Scratch..., Eye Gouge family, Small Bomb, Cat Kick, Holy Sword ...) |
| new formulas | 1A, 1B (Zalbag's Ruins), 2E (Wiegraf), 2F (Dark Sword), 3D, 4E, 4F, 52, 55, 62 (monsters) | ~22 |

(Deathspell 2 and Mbarrier, listed in the audit file, are done since.) So **most of Chapter
I is data, not code**: enabling an action whose formula exists only needs its ffcj record.
Only ten formulas are new. Later chapters add many more special sets (Holy Knights, Cid,
Zodiac monsters), so the mechanism matters most after Chapter I.

**Proposed mechanism: a formula extension, the core still applies the effect.**
- The core already reaches formulas through a few shared entries, exported to every image in
  `ModCtx`: `skill_kind` (category), `magic_hit` (chance), `magic_amount`/`magic_heal`
  (amount or packed status byte/mask: `magic_heal`'s query 0/1/2 already returns all three
  for formulas 0A-0D), `weapon_amount` (weapon formulas). Forecast, AI scoring, area numbers
  and commit all go through those answers.
- A chapter image answers the same three questions (kind, chance, amount/status) for formula
  ids the core does not know
  (e.g. ffcj record kind `AB_EXTERN`, its formula id passed to the image). It computes;
  the core keeps commit, status rules, animation, UI and AI. No new effect types: an
  extension returns damage/heal/MP/status packets the core already applies.
- Built by `tools/module.py` like the core's images (same `ModCtx`, `MOD_SIG` check), stored
  in a chapter data variable (FMB3, e.g. `ff1x`), validated by RESOURCES at chapter load.
- **Its own heap block, loaded once per battle** (HW3Patch), not the shared zone: the AI
  calls formulas inside candidate loops, and swapping an 8 KB image per call (~35k cycles)
  would be prohibitive. The block costs its size in RAM (target ≤ 2 KB for Chapter I's ten
  formulas; battle RAM free today ~11 KB).
- PC builds have no modules: the extension source is compiled into the PC checks as the
  core's modules already are (`MOD_PART`), so the PC/sanitizer/68000 oracles stay possible.

**Contract changes (decision needed).**
- `chapter_packages.py` today rejects code ("Chapters contain data only"): allow one
  extension bank, validated by signature.
- A chapter extension is compiled against one core's `ModCtx`: it binds to the core key
  (already true for all chapter data). A core change rebuilds the chapter's extension.
- The interface (three queries, `AB_EXTERN`, the ext block) must be frozen with the core.

**Cost / benefit.** Mechanism: one hook in the formula entry, one loader op in RESOURCES,
the builder rule, a check carrier; core growth a few hundred bytes. Benefit: the core can be
frozen before Chapter II's specials exist. Alternative: put Chapter I's ten formulas in the
core (Formula/AI images grow, core refrozen for each later chapter).

**Recommendation.** Implement the ~28 data-only actions first (no mechanism needed), then
the extension with Chapter I's ten new formulas as its first client.

## 2. Parked uses (not needed now)

- **Several images resident at once.** Each hot module keeps its own heap block instead of
  sharing the zone, removing image copies (up to 8.6 KB `memcpy` per switch; the AI switches
  every step). Costs RAM equal to the images kept; measure the AI's switch cost with
  `ti-cycles` first. Same idea as the extension's own block above.
- **Code generated at run time** ("compiled sprites": each sprite becomes a draw routine).
  Often much faster than masked blits on the 68000, but RAM per sprite and close to asm
  (ask first). Only if a measured render path needs it.
- **Splitting the resident** into heap-loaded parts: program space only, no RAM gain; not
  needed with 8,743 bytes of headroom.
- **TSRs** (resident hooks): no background task to run (no sound); `tilog` covers debugging.
- **GCC nested-function trampolines** (code on the stack): convenience, no measured gain.
- **Running from the archive (Flash)**: HW3Patch removes the RAM protection; whether Flash
  execution is allowed is not verified, and images write their own context, so they need RAM.
