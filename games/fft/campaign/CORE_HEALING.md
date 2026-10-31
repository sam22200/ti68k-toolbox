# Healing and resurrection: eight retail producers together

2026-10-29. Follow-up to CORE_REFLECT. One shared formula0c/0d batch enables
**Cure1–4, Raise1–2, Moogle and Fairy**, with real source skill-set/learned-bit
access, casting costs/CT, targeting, forecast, AI, once-only impacts and Reflect.
The existing approved common area/number presentation is reused. No new art,
font bank, menu layout or original summon cinematic is introduced.

## Source rules and implementation

- Formula0c uses MA, Magic Attack Up, zodiac and both Faith values. Target Shell,
  Magic Defense Up, Frog and weather do not reduce or amplify these heals.
  A normal heal requires a living valid target. Undead reverses it into HP damage;
  dead Undead, crystals/chests, Petrify, Wall and absent actors are not healed.
- Formula0d success is Faith-scaled zodiac(MA) + zodiac(X); both terms get their
  own zodiac rounding. The HP restored is **ceil(maxHP * Y / 100)**, independent
  of Faith. Raise restores50%, Raise2 restores100%, with the shared999 cap.
  Normal living targets fail; living Undead takes damage instead. Innate Dead
  cannot be removed. Dead Undead remains KO. Mutable Dead is cleared on a
  successful resurrection, the death counter resets3 and CT is retained.
- Cure consumes no evasion or generic formula08 proc RNG. Raise rolls accuracy
  only below100%, including chance0; there is no additional proc draw. Native
  forecasts remain pure. Element-zero producers are validated offline, rather
  than silently inheriting offensive elemental/support/status modifiers.
- Raw team flags keep Moogle/Fairy on allies. Other healing spells can affect
  both camps. Immediate area AI shares the same quantities, categories and hit
  rates and can choose a heal or resurrection. This remains adapted immediate
  HP/team scoring, not complete original future-CT/response/reflection simulation.
- The existing owned area pipeline also handles radius0 Raise. Unit/Panel
  binding, empty/late targets, single payment, packet freezing, response drain,
  HP typing, rewards and outcome ordering are shared. Reflect redirects to one
  endpoint, uses the original caster and costs no extra MP, including when the
  reflected endpoint needs resurrection.

`magic_heal` is one pure resident query callback (amount/chance/category), not
an image loader. AI, TARGETS and RESPONSE may call it while their own image is
running. No shared gameplay, Work or save field is added. This family deliberately
does not alter the legacy item formulas or enable other status/monster families.

## Budget work included in this batch

The unchanged banner shade and strip reveal move to the existing NUMBERS image,
with two resident wrappers. A baseline reference captured before relocation
checks32 partial/full fade/reveal plane results against the actual new entry on
both PC and68000. No font/asset byte changes. Native callers return before any
other image is loaded; the banner entry itself performs no image loads.

ABI39/context376/signature153 (the signature is intentionally u16) adds the
four-byte pure query pointer. All17 consumers are rebuilt; split stays8+9.
ACTION6650 is the largest image; NUMBERS3336 includes the relocated banners.
Raw shards41170/44052. Unit112/State2192/Work45576/FSV3 save3138, the20 recorded
layout fields and the five core heap allocations remain unchanged.

Private group `x/core-healing/fftcampaign.89g` is **523264 bytes (+2925)**.
The resident's compressed payload shrinks255 bytes, while raw packed input is
64927, leaving **9 bytes** below64936. This batch fits; further resident growth
must include another measured cold-code relocation as part of its next family,
not as a standalone content-free milestone. Only ffbcore, ffbm, ffbn and ffcj
change; the other27 non-code banks and all assets remain identical. FCJA stays
41086 bytes and changes exactly8 previously disabled normalized records.

## Scoped evidence

`make -C games/fft/campaign core-healing-check` uses the production records:

- 2110 PC/ASan/UBSan/68000 assertions, digest11072857: all8 producers, real
  learned lists, MP/CT/HalfMP/Silence, AI inclusion, HP/Dead/counter/CT lifecycle,
  Undead, crystals, Reflect and32 unchanged banner plane comparisons.
- `tools/healing_oracle.py` executes75 bounded profiles through the actual
  original Cure/Raise formulas, status filter and finalizer, including supports,
  Faith/Innocent, zodiac/Serpentarius, rounding and blocked targets. Accuracy
  rolls are observed and forced to pass except chance0; target-list sorting is
  isolated. This is not a new complete original cast/renderer/HP-commit trace.
- Original25 offensive/Reflect producers retain2736/77DAC288. Scoped ordinary
  actions295/5D93DCDC includes eight additional catalogue forecasts; reactions
  retain267/EF8107A7; owned area353/11D6FCB4 plus5 retries and magical integration
  159/E8EE3DB3 pass. Unchanged deeper bitmap/geometry/Jump censuses are reused.
- Parser130/00000047, actual binding162/7DF38F1D,34 cold/warm loads and3 stale
  rebindings pass. Eight invalid starts include the **real prior ABI38 export**,
  not only a manufactured old signature. Native/PC20 layouts,12 previous action/
  response screen hashes and both native pilot save/results flows pass.
- Measured backend maxima: preparation111812, impact30880 and banner entry74190
  cycles. These are bounded fixture calls, not a full-frame/stack/residency
  certificate or the worst13-victim map certification.

Evidence: `x/core-healing/{verification,package,layout,frames}.json`,
`check/{results,coverage}.json`, `oracle/source.json` and scoped check folders.
The standard playtest, user saves and live TiEmu remain untouched; old gallery/
group bytes **and timestamps** are preserved.

## Remaining family work

Production has **55 enabled normalized records, including33 casts** (21 spells
and12 summons). Generic careers expose **52/201 action-table entries**;
**149 remain**, including categories/upgrades, not149 distinct effects.

| Shared work group | Remaining generic entries |
|---|---:|
| Status/control/barriers (0a/0b/0e/12/14) | 34 |
| Percentage HP/resource transfer (09/0f/10) | 5 |
| Other default formula/job families | 69 |
| Special dispatch/items/upgrades/categories | 41 |
| Total | 149 |

Next implement compatible buffs/status producers together. Reflect/Carbunkle
producers still belong to that family. Special jobs/monsters, R/S/M, complete
campaign/objectives, formats/tools and full worst-case budgets are additional
scope. The core is **not complete**. Keep Jump deferred; retain family batches.
