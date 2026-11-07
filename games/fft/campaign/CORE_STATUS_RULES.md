# Darkness and Silence hooks

2026-10-26. The next L2 slice after CORE_ACTIONS.md. This makes the existing
Darkness bit affect physical evasion and supplies the source-based Silence
restriction to player commit, preview and AI. It does not implement the full
40-status policy, poison upkeep or new magic formulas.

## Rules and integration

`status_rules.h` shares the physical base between `hit_chance` and `roll_hit`:
100 normally, halved by attacker Darkness and then target Defend, so both
produce25. This changes the denominator of each applicable evade test, not an
unconditional50% hit chance. Back attacks with no usable evasion still hit100%.
Dash remains unaffected because its formula is not evadable; Throw Stone,
Attack and Archer Charge use the common physical rule.

Silence checks the source secondary ability flag (`flags_3 &0x20`), ordinary
ability ID range and source skillset menu dispatch. Only default/monster menus
are blocked. Items, Charge and bypass menus such as Math retain their source
policy. `can_use`, formula preview and `ai_usable` use the same predicate;
rejection precedes action state changes, RNG and costs. Existing Eye Drop and
Echo Grass removal immediately restores the corresponding capability.

Current enabled execution records do not have the silence-sensitive flag.
This is a working restriction hook for the action pipeline, not a claim that
Fire or the other unsupported spells are now playable. An explicit test-only
copy of the catalogue gives existing Wish that flag to verify real player,
formula/overlay and AI rejection without enabling a fake shipping formula.

## Shared data contract

FCJ6 appends224 source menu-kind bytes from0x80065CB4 to CJ_SPECIAL, after its
unchanged168 bytes. Directory offsets and all shared readers remain relative;
the archive is read in place. `cj_action_menu` bounds the set index. FCJ6 and
the increased minimum section length explicitly reject old/incomplete banks.
The battle loader now validates the complete catalogue span rather than only
its magic. Item/ability/name/learned/save IDs and FSV3 remain unchanged.

The module ABI becomes9 even with identical field layouts, so an older AI or
formula image cannot silently bypass the shared rule. All nine images rebuild.

The common reader checks all moved catalogue sections against the disc, all224
menu kinds through PC/native reads, missing-table rejection and rejection of
FCJ5. Packaged management/deployment readers are rebuilt with FCJ6. Existing
descriptor exports remain byte-identical. No map-ID or job-ID exception is used.

## Original evidence and tests

`tools/status_rules_oracle.py` executes unchanged original MIPS:

- 0x80185328, Darkness/Confusion base modifier;
- 0x801853F4, target status evasion modifiers including Defend;
- 0x80185814, final hit percentage;
- 0x8017DA20, commit restrictions/MP policy.

36 evasion cases cover Darkness/Defend combinations, front/side/back and three
shield cases.50 restriction cases cover two silence-sensitive spells plus
Dash/items/Charge across five actual menu kinds, awake and silenced. Main and
battle code hashes, function hashes and decomp commit are recorded. The RAM-only
runner supplies a fixed external BIOS rand result; the tested preview percentage
ignores roll outcomes. No tested formula/restriction instructions are patched.
Evades and enough MP/normal terrain are supplied; this is not a full original
turn or a proof of native PS1 RNG sequence identity.

`check_status_rules.py`:433 PC/sanitizer/actual68000 assertions, hash A5131B09.
These include current real formulas, committed class-only roll thresholds,
cure restoration, source menu bypass, rejected-action side effects and the
actual AI candidate list with the explicit injected-flag integration fixture.
`check_common.py`: source catalogue/224 menus, native hash7A67010B. Existing295
action checks/three rankings/all forecasts retain hash EDCEBFAE; Charge retains
276A2090. Existing action impact peak is34500 cycles versus33480 before this
slice, mostly the larger formula image copy; this is not a full-frame certificate.

Gariland/Orbonne flow retains81/14 assertions and matching native save bytes,
including repeated rewards and trophy-stock saturation. All11 flow pictures
match the previous checkpoint. No full map census, decoder benchmark, exhaustive
input sweep or TiEmu is needed.
Normal action/flow pictures are compared with the accepted previous checkpoint;
identical images keep their timestamps and require no new visual decision.

## Current package and remaining scope

Private `x/core-status-rules/fftcampaign.89g`:473639 bytes,42 variables, +1174
versus CORE_ACTIONS. Changes: catalogue +224, module bank +510, packed battle
+440; management/deployment reader hashes change with the new version, at the
same packed sizes. The other27 non-code banks are identical. ABI9/nine images,
zone5730; State1586/Work45516/FSV3 size3138 and heap allocation expressions are
unchanged. Battle TTPack input64575 bytes, headroom361 (previous759).

Full status storage/application/cancellation/immunity/innate effects, duration,
KO cleanup, effective teams, poison/regen upkeep, cast interruption and reaction
contexts remain open. The existing four-bit field is still a limited subset.
This checkpoint deliberately does not pretend that a status flag alone enables
every downstream mechanic. Jump remains deferred. Standard bundle and the
user's live TiEmu remain untouched.

```sh
make -C games/fft/campaign core-status-rules-check
tools/pyenv/bin/python games/fft/campaign/check_common.py
```
