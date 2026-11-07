# Reflect and the remaining offensive magic batch

Current follow-up: [CORE_HEALING.md](CORE_HEALING.md) adds all8 healing/
resurrection producers, for33 casts and52/201 generic entries. This Reflect
checkpoint below is historical.

2026-10-29. Follow-up to CORE_OFFENSIVE_MAGIC. All eleven remaining ordinary
formula08 producers are enabled together: Holy, Flare, Fire/Fire2/Fire3,
Bolt/Bolt2/Bolt3 and Ice/Ice2/Ice3. The first fourteen remain enabled.

## Shared behavior

Reflect is a primary pre-formula miss. It consumes no magical-evasion or
formula RNG on the protected victim. At impact, the existing bounded reaction
queue records source reaction1b8, independently of the equipped reaction and
Brave. Requests drain in actor-slot order after all primary victims settle.
Don't Act and the source deep-water restriction can suppress the response;
ordinary reaction incapacity such as Stop does not suppress Reflect.

The reflected endpoint is `(2*reflector - original caster)` in tactical X/Z,
with source raw-height layer selection. Equal distances select the higher
height, then upper on equal heights. Empty/blocked/outside endpoints produce
no attack or fake Miss animation on the reflector. Reflect preserves facing.
Only the endpoint's occupant is affected; there is no second AoE or recursive
reflection. Damage/evasion use the original caster, with live target stats.
The response costs no MP, never replaces an independently armed cast and
cannot recursively queue another response. Root rewards/outcomes still drain
after its responses. The existing approved miss/secondary-impact presentation
is reused; no new aura, font, sprite bank or summon cinematic is introduced.

The root packet's unused-on-magic kx/kz fields retain the original cast origin
across response packet save/restore. No extra gameplay field or queue is added.
RESPONSE owns endpoint lookup and setup locally, including `magic_prepare`;
its resident callbacks are pure arithmetic/lookup, never nested image loads.
COMMAND keeps ordinary cast preparation. This avoids resident dispatch growth.

Reflect status38 is now accepted by the public status adapter, with the
existing source immunity, cure and timer14 policy. The **Reflect spell and
Carbunkle producers** still belong to the upcoming shared status/buff family;
this milestone does not claim those formula11 spells are enabled. Math Skill
remains gated; its original Reflect exemption is recorded source-only.

## Actual producer coverage

There are now **25 executable ordinary offensive casts**: 15 spells and
10 offensive summons. Production has47 enabled normalized records in total.
Across the twenty generic careers, **44/201 action-table entries** are enabled;
**157 remain**. These entries include categories/upgrades and exclude R/S/M,
special jobs and monsters: they are not157 distinct effects.

| Next shared work groups | Remaining generic entries |
|---|---:|
| Magical healing/resurrection (0c/0d) | 8 |
| Status/control/barriers (0a/0b/0e/12/14) | 34 |
| Percentage HP/resource transfer (09/0f/10) | 5 |
| Other default formula/job families | 69 |
| Special dispatch, items, upgrades and categories | 41 |
| Total | 157 |

Continue by compatible family batches. Do not reopen per-spell arithmetic or
make each named ability a milestone. The next batch is healing/resurrection.
Immediate HP/team AI avoids treating a protected target as direct damage;
full original future-CT/reflection-origin utility and reaction simulation
remain incomplete. Broader campaign/objectives, formats/tools, R/S/M and
full worst-frame/stack/residency gates remain open. **The core is not done.**
Jump stays deferred.

## Evidence, size and limits

`make -C games/fft/campaign core-reflect-check` uses the actual production bank:

- 2736 PC/ASan/UBSan/68000 assertions agree, digest77DAC288. The suite covers
 all25 producers, real learned lists and AI, direct/reflected hits, original
 caster MA, one payment/impact, late Reflect, cure/immunity, second-Reflect
 targets, restrictions, empty endpoints and retained facing.
- `tools/reflect_oracle.py` executes unchanged original MIPS:18 endpoints and
 four reflect-guard/Math cases, with original function/disc hashes. This is
 bounded endpoint/guard evidence, not a new complete PS1 spell dispatcher trace.
- Affected closure: ordinary actions295/CB932B2F (additional catalogue previews),
 reactions267/EF8107A7, owned area353/11D6FCB4 plus five Orbonne restarts,
 magical integration159/E8EE3DB3, bindings162/7DF38F11, parser130/00000047 and
 seven bad/previous-ABI startup rejections. Unchanged deeper arithmetic,
 geometry, status bitmap and Jump censuses are reused.
- All20 PC/native layout fields and12 historical action/reaction screens agree.
 Both pilot saves/results pass natively, including settled save equality,
 receipt replay, overflow and first-frame victory behavior.

ABI38/context372/signature65428;17 images/split8+9. ACTION6398 (+82) is the
largest execution image. RESPONSE6170 and COMMAND5674 stay within that zone.
Raw code shards40656/41386. Unit112/State2192/Work45576/FSV3 save3138 and the
five core allocations are unchanged. Reflection launch backend peaks56018
cycles; this is not a full-frame/stack/residency certificate.

Private group `x/core-reflect/fftcampaign.89g`: **520339 bytes (+1512)**.
Resident input64887 leaves49 bytes below the64936 pack limit; additional
resident growth needs a measured relocation before the next engine family.
Fold any required cold-code relocation into the healing batch, rather than
returning a separate content-free prerequisite milestone.
Only ffbcore, the two code banks and ffcj change;27 other non-code banks remain
identical. FCJA stays41086 bytes, changing exactly eleven disabled records.
Old ABI37 images are rejected even though layouts are unchanged. TURN/BEATS
compiler drift from unused declarations is checked in an isolated build.

Evidence: `x/core-reflect/{verification,package,layout,frames}.json`,
`check/{results,coverage}.json`, `oracle/source.json` and scoped check folders.
The standard playtest, user saves and live TiEmu remain untouched. Previous
gallery/group bytes **and timestamps** are preserved.
