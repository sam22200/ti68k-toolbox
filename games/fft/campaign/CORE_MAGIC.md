# Magical damage scalar and private Holy integration

2026-10-27. Follow-up to CORE_INCANTATION.md. Formula08 now has one pure
resident evaluator shared by prediction, effect execution and actual native
AI. A private normalized Holy record tests the complete existing single-centre
cast/action boundary. Production Holy/Fire remain disabled and no AF_CAST record
ships. This is a bounded backend milestone, not general playable magic.

## Original evidence

`tools/magic_oracle.py` executes the original magical-damage pipeline under
Unicorn on104 ordinary male/female profiles. Function/disc hashes, decompilation
commit, raw Holy record, inputs, outputs and hook scope are recorded in
`x/core-magic/oracle/source.json`. The full formula is unchanged; weather is
stubbed to normal and the generic proc roll is observed/stubbed to pass.
Elemental affinities/strengthening are zero in these fixtures.

Holy0x0f uses formula08, power50, range5, radius0, CT6 and MP56. Its flags4
byte0x40 lacks magic-evasion bit0x02; Fire has0x42. Real shield MEV equipment
and Defend therefore do not make this Holy scalar test an evadeable spell.
Do not reuse its100% target eligibility for Fire or other magic families.

Observed order: MA; Magic Attack Up4/3; Magic Defense Up2/3; Shell2/3;
Frog/Chicken3/2 once; zodiac; power; Faith product/10000. Each XA modifier
truncates at its own boundary. Innocent overrides Faith; otherwise Faith status
means100. Serpentarius bypasses zodiac adjustment/clamping in the original, so
formula08 explicitly preserves it without changing the existing physical helper.
The existing Unit female flag represents ordinary humans, not monster gender.

The profiles cover zero MA, fractional Faith, modifier combinations, good/bad/
opposite compatibility, both ordinary sexes, Serpentarius, Faith/Innocent
precedence and maximum bounded power96. Charging and Sleep do not supply the
physical vulnerability multiplier. Holy's preview consumes no RNG, while actual
execution still calls the generic proc draw once with(100,19), even without a
status effect. The backend preserves its existing one execution draw.

These are scalar/execute-branch observations. Full original target membership,
HP commit, elemental/status/innate producers, monster zodiac, final display/
capping, magic evasion and refund/presentation traces remain unverified here.

## Native evaluator and integration

`magic_math.inc` handles formula08 through the pure resident `magic_amount`
callback. AI/ACTION/FORMULA never evict their own executing image for this
calculation. Prediction does not mutate State or battle rng32; execution uses
live stats/status/Faith when the pending cast becomes due. The physical damage
formula and shared physical zodiac helper remain unchanged.

The source support IDs0x1d3/0x1d4 read equipped Magic Attack Up/Magic Defense Up.
Shell, Chicken, Frog, Faith and Innocent read canonical active status bits.
Naming IDs and evaluating these bits does not enable their application, upkeep,
innate traits or other effects: existing supported-status request gates remain
unchanged. Elemental strengthening/weakness/absorb/nullification and weather
modifiers still require their own policy and original fixtures.

MA<=99, source formula08 power<=96 and effective Faith<=100 bound XA to297,
raw damage to28512 and the product to285120000. Existing verified MULS16 and
DIVS32_16 primitives handle the wide product and division by10000. The /3
modifiers use the exact bounded reciprocal21846; no new assembly was authored.
Compiler output contains MULS.W and one final DIVS.W, no software32-bit multiply/
divide helper in this kernel. Do not extend the stat/power/element domain without
revalidating these bounds. This is one scalar call, not division in a grid loop.

Private Holy uses source normalized parameters and AF_CAST, never a production
data edit. It arms without payment/RNG, pays56 once at due launch, naturally
lands at exactly56 MP, deals122 for MA5 and70/70 Faith, commits HP once, avoids
physical critical/Brave knockback, wakes mutable Sleep and settles Auto-Potion
before resuming the root. A separate case changes live MA/Faith/Shell during the
wait. Actual native AI candidate ranking responds to Faith and keeps its one
legitimate chooser draw. The current generic presentation is test scaffolding;
spell/empty-centre animation and player binding-choice UI remain gated.

## Packing and image ownership

ABI21/ModCtx348/signature60771 reject ABI20. Twelve images remain; largest5882.
Unit112/State2124/Work45516/SaveUnit176/Save3138(FSV3) and the five allocation
requests remain unchanged. No target list, new event queue or allocation is added.

The first resident scalar exceeded the packer. Unchanged cold banner metadata
moves to PARTY7; unchanged coordinate-cache setup moves to PARTY8. Cache setup
invalidates the image tag before rebinding Work/dimensions. The same helper
source supplies portable and image versions. End-turn CT remains resident;
relocating that frequent path was rejected as an avoidable copy cost. The
reviewed resident renderer, banner widths, reveal and hold timing stay identical.

Private42-variable group493049 bytes (+503): packed battle42352 (+77), raw
module bank54906 (+426). All28 non-code banks remain unchanged, including FCJ8.
TTPack input64919 leaves17 bytes below64936. The next resident addition requires
a module budget; this margin is not a production safety certificate.

## Scoped verification

`make -C games/fft/campaign core-magic-check` runs1352 assertions on PC,
ASan/UBSan and actual68000 images, hash4DCFDD40. It includes104 original scalar
profiles, State/RNG purity, shared prediction, live casting/impact/reaction/AI,
and13-actor end-turn flags plus unchanged banner metadata boundaries.

Scalar zone39 averages1972 cycles, peak2224 over104 profiles; CHECK overhead
is included. One due Holy launch costs47900 cycles. Cold coordinate setup is
61474 cycles including image loading and cache writes; warmed banner setup is
1526. These are isolated CPU zones, not complete-frame or cold-banner timings.
Relocation is a packing tradeoff, not a speedup. Full stack/fragmentation/combined
residency and worst-case frame budgets remain open.

Direct incantation/actions/dual Charge/control/single Charge regressions and one
native Orbonne/Gariland flow cover the shared ABI/image boundary. Native layout
is refreshed once; unchanged bitmap/timer/geometry/census evidence is reused.
Final hashes, package/data comparison, old-image rejection, compiler inspection
and identical eleven flow PNG bytes/dates are recorded in
`x/core-magic/verification.json`. The standard466045-byte playtest group and
live TiEmu remain untouched. No new review image or assembly was authored.

Next: source-backed elemental/target traits and magical evasion, then area
membership, per-target impact/reward/reaction and reviewed spell presentation.
Do not enable retail magic until its target/traits/producer/data-version gates
pass. Broader status/reaction, Quick/all-freeze, campaign, formats/editors and
residency gates remain open; Jump refinements stay low priority.
