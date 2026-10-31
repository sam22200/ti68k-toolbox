# Sleep and source physical vulnerability

2026-10-26. L2 follow-up to CORE_CONTROL.md. Sleep is accepted by the shared
status API; its source timer is60 ticks. This enables the tested engine effect,
not a new spell/weapon/terrain producer or original Sleep pose/status artwork.
Full L2, general reactions and content-production gates remain open.

## Runtime contract

- Sleep freezes CT, including an already-ready actor, and prevents commands
  through the existing player/AI eligibility cache. Its duration still ticks.
  Expiry/cure refreshes eligibility and invalidates affected caches.
- Its source cancellation row removes Charging and the pending Charge together.
  This differs from Stop, which pauses a retained Charge. Cure never restores
  the cancelled command. Both effects' AT wait is their maximum remaining timer.
- A sleeping or charging target receives one XA multiplier of3/2 for the
  currently supported physical formula families. Both conditions together do
  not multiply twice. Apply this before zodiac compatibility and criticals.
  Physical preview, actual damage and AI use the same canonical-state rule.
- Successful HP-damage-type impacts wake mutable Sleep at ACT_HIT, even when
  damage is zero. Misses, healing, stat effects, Poison upkeep and self-cost do
  not use that primary wake hook. Existing cross-splash victims use it too.
  KO follows the existing source-cancellation cleanup. Innate Sleep remains
  protected. The impact is committed once; forecasts never wake their target.

The previous runtime used pending-record presence for Charge vulnerability,
after zodiac in ordinary Attack and sword skills, and omitted the modifier in
Dash/Throw Stone. Canonical Sleeping/Charging and the original order now govern
all these supported families. No new formula family, support/elemental policy
or arbitrary interrupt of a committed movement/strike is claimed.

## Original evidence

`tools/sleep_oracle.py` executes unchanged original disc MIPS.140 bounded XA
cases run the physical-status/support caller, including the target modifier and
zodiac, with no equipped supports. Nine finalize cases execute HP arithmetic,
attack-type wake staging and actual bitmap/timer resolution: ordinary damage,
zero damage, miss, heal, MP damage, status-only, untyped HP damage, innate Sleep
and KO. The zero-damage and untyped-damage cases show that wake depends on the
successful action's HP-damage flag rather than an HP difference. Four turn-only
AT lists check Sleep and combined Stop/Sleep wait.

Reaction, equipment, reward/control bookkeeping and status-graphics handlers are
explicitly isolated by runner hooks. MP-damage and untyped HP commits remain
source-only evidence, without a native producer. This is not a full PS1 battle,
MP Switch, elemental absorption, innate producer or reaction certificate. Hashes,
revision and all external hooks are in `x/core-sleep/oracle/source.json`.

## Validation and scope

`check_sleep.py` passes1070 PC/ASan/UBSan/actual68000 assertions, hashB6C26F11,
with a13-actor descriptor. It covers the source XA matrix through actual
Attack/critical/Charge/sword/Dash-style evaluators, primary impact timing,
zero-damage wake, no wake on miss/heal/stat effect, protected innate Sleep,
KO cleanup, Charge cancellation, CT freeze/expiry, source AT lists, independent
Stop cure, actor12 timer ownership, Poison separation, AI purity and splash wake.

The source no-evasion cache and duration storage already existed; no field,
allocation or save-format change is needed. The main structures remain native
Unit112, State2014, Work45516, ModCtx320 and FSV3 save3138. ABI13 rejects old
images so older FORMULA/AI code cannot bypass the new effect contract. Packaging
and targeted regression results are recorded in `x/core-sleep/verification.json`.
No emulator input or standard playtest bundle is changed.

Private `x/core-sleep/fftcampaign.89g`:478136 bytes,42 variables, +112 versus
CORE_CONTROL. The module bank is40224 (+174); packed battle payload is62 bytes
smaller. The largest image stays5742; FORMULA5068, AI5458, signature58357.
Packed battle input64837 leaves99 bytes below TTPack's64936-byte ceiling.
This is archive/program packaging, not a combined peak-RAM certificate.

Direct regressions pass on PC and68000: control594/15C96953, lifetime4140/
D0F9317E, protected bitmap4843/262BF732, primary actions295/EDCEBFAE and
Charge276A2090. The Charge source fixture now seeds canonical Charging alongside
its pending record; source expectations stay unchanged. The Sleep impact zone
totals58168 cycles across seven prepared cases, reported maximum26680. Native
cold13-actor AT costs51156, warm18454; cold13-actor expiry71582. These are
scoped measurements, not complete cold-frame/scheduler bounds. The new XA/wake
and refreshed-cache helpers contain no software arithmetic/division calls.

All28 non-code banks are unchanged. Six ordinary action frame hashes and all11
flow PNGs match the previous checkpoint; captures keep their original dates.
The flow checker stages native split-fixture pictures privately until their
final publication, avoiding transient rewrites of a shared output filename.
Gariland81/AE9CFD31 and Orbonne14/2CD70391 also pass fresh PC/native integration,
including save-byte equality, duplicate receipt and saturated trophy stock.
The standard466045-byte bundle and live TiEmu remain untouched. Private SHA256:
`767f35aa065197984f75b6483b3fbbaaaac9bed8e6d22de6882b54c70c0eb975`.

## Next work

Make action/reaction contexts explicit and bounded before implementing reaction
families or multiple strikes. Then cover further incapacitation/team/KO effects
and their producers with source fixtures. Remaining timing/residency, general
scene/campaign contracts and versioned map/scene tools stay in CORE_ROADMAP.md.
Jump refinements remain deferred by user decision.

```sh
make -C games/fft/campaign core-sleep-check
```
