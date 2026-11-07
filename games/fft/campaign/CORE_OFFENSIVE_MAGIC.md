# Production offensive magic batch

Current follow-up: [CORE_REFLECT.md](CORE_REFLECT.md) enables all25 ordinary
offensive casts (44/201 generic action entries). The14-producer milestone below
is historical; `core-reflect-check` is the complete current family check.

Date: 2026-10-29. User direction: implement whole mechanic families, not one
effect or named ability per milestone. Engine fixtures and retail enablement
must be counted separately. This checkpoint finally enables retail casts.

## Delivered together

One source-record classifier in `tools/common_tables.py` enables fourteen
ordinary nonreflectable formula08 actions:

- Wizard: Fire 4, Bolt 4, Ice 4.
- Time Mage: Meteor.
- Summoner: Shiva, Ramuh, Ifrit, Titan, Bahamut, Odin, Leviathan,
  Salamander, Cyclops and Zodiac.

The compiler derives power, element, range, evasion and self targeting from
source fields. Source radial tolerances and team/self filters stay in the raw
records. There is no spell-id dispatcher or fourteen copies of an effect.
Unsupported shape, rider, performance and Reflect flags reject the family.
The existing learned lists expose these actions only when actually learned;
normal jobs, JP and unlock requirements are retained.

All fourteen use the existing command/CT/payment/area packet/impact/response
pipeline and approved Unit/Panel menu and generic area presentation. These
are executable gameplay effects, **not original PS1 summon cinematics**.
Immediate HP/team area AI is supported; original future-CT, response and
Reflect-aware planning remains incomplete.

## Coverage and remaining batches

Production now has **36 executable normalized records**, including **14 casts**.
For the twenty generic careers, **33 of 201 distinct nonzero action-table
entries** are enabled, leaving **168 entries**. Three enabled records belong
to other skill sets. The 201 figure excludes R/S/M positions. It includes menu
categories and upgrades, so it is not a count of distinct effects.

The remaining generic action entries form these reproducible data buckets:

| Batch | Source formula(s), hexadecimal | Remaining entries |
|---|---|---:|
| Reflect and the rest of offensive HP magic | 08 | 11 |
| Magical healing and resurrection | 0c, 0d | 8 |
| Status, control and protective barriers | 0a, 0b, 0e, 12, 14 | 34 |
| Percentage HP and resource transfers | 09, 0f, 10 | 5 |
| Other default formulas (techniques and job families) | other default formulas | 69 |
| Special dispatch, items, upgrades and menu categories | IDs >= 170 | 41 |
| Total | | 168 |

These are work groups, not six promised single patches. Each group must enable
its compatible producers together and record any prerequisite that remains.
Special jobs, monsters, R/S/M, campaign/objectives, tools/formats and complete
memory/frame/stack gates are additional scope. The core is **not complete**.
Jump stays deferred.

The eleven reflectable formula08 entries (Holy, Flare and Fire/Bolt/Ice tiers
1–3) remain disabled as a single prerequisite batch. Do not implement their
damage again or silently drop their Reflect behavior. Healing, status summons
and percentage summons also remain gated with their corresponding families.

## Checks and size

`make -C games/fft/campaign core-offensive-magic-check` runs the whole batch
from the production catalogue, with no synthetic spell overrides:

- 1290 checks, digest `08E75784`, agree on PC, ASan/UBSan and actual 68000 code.
- Real source skill sets and learned bits expose every enabled producer.
- All fourteen run with unit/panel binding, ordinary CT and immediate support,
  per-victim amounts, team/self exclusions, one MP payment and once-only impact.
- All fourteen check insufficient MP, Half MP, initial/late Silence and actual
  area planner inclusion. Unsupported Reflect producers stay unavailable.
- Compiler guards reject unsupported shape/status/reflection flags as a family.

The new source classifier changes exactly fourteen previously disabled
12-byte records. All old enabled records and all other source sections are
byte-identical. FCJA remains 41086 raw bytes. The private group
`x/core-offensive-magic/fftcampaign.89g` is **518827 bytes: zero byte growth**.
Only ffcj changes in the 43-variable group; all code images, the resident and
the other 27 non-code banks are identical to CORE_INCAPACITY. ABI37/context372/
signature65171, resident input64765/headroom171 and RAM layouts are unchanged.

Two legacy tests now explicitly select a non-cast splash: selecting the first
area record in numeric order is no longer a valid way to identify Stasis Sword.
The production effect implementation has not changed. Scoped ordinary-action
checks pass295 times on PC/sanitizers/68000 (new digest39FCE2B3 because forecasts
now visit14 additional records); reactions retain267/EF8107A7. Both pilot
PC save/result flows retain81/AE9CFD31 and14/2CD70391.

This batch reuses the already recorded original formula08, elemental/evasion,
radial geometry and charge-policy evidence. It does not claim fourteen new
complete original-PS1 battle traces. Exact long summon scenes, original
future-turn AI and broader remaining feature families are separate scope.

Evidence: `x/core-offensive-magic/check/{coverage,results}.json`, `package.json`
and `verification.json`. Old scalar, geometry, status and area integration
evidence remains applicable to unchanged code. No new full-frame performance
certificate is claimed. Standard playtest, user saves, live TiEmu and previous
gallery bytes **and timestamps** remain untouched.
