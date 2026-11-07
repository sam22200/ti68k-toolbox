# Discovery, poaching and monster capabilities — delivery block D

The final six generic R/S/M producers have consumers: Finger Guard448,
Train470, Secret Hunt471, Monster Talk473, Monster Skill478 and Move-Find
Item509. Generic actions remain201/201; equipped generic R/S/M coverage is
78/78. These counts describe bounded executable consumers, not completion of
FFT, all monster content, all interactions or release certification.

## Talk protection and monster commands

Formula42 rejects sleeping targets and requires equipped/innate Monster Talk
for a source monster target. Finger Guard runs before intrinsic MA+X accuracy,
with primary/reaction/status eligibility and source Brave ordering. Brave100
blocks without a Brave draw; Brave0 still draws before intrinsic accuracy.
Forecasts combine both chances without consuming RNG. Steal Heart retains
its separate formula and does not gain these gates.

Jobs94..141 identify source monsters, with their actual primary skillsets
176..223. The first three positions are ordinary actions; adjacent same-camp
Monster Skill exposes position four without collapsing holes. Eligibility
uses the source PREVENT_REACTION mask, including Don't Act, and total-height
limits of-5..5. Total height includes body reduction, shared-tile borrowing,
Float and the two source water traits. Native camp/layer limits remain those
of the existing core; no mounting system is added.

The bounded executable monster producer is the source Chocobo94: Choco Attack265,
Choco Cure269 and the otherwise locked Choco Esuna268, with the real empty
third position retained. Cure uses MA×3, Magic Attack Up, zodiac and Undead
reversal without Faith. Esuna uses source(MA+65)% and status-set100 through
the existing pure preparation and frozen status commit. Choco Attack reuses
the native ordinary unarmed-attack policy, including physical forecasts and Golem. All48 source monster lists/stats
are present; their other actions remain disabled until their content/formulas
are implemented. No human sprite or portrait is assigned to a monster. Scripted monsters require valid raw stat data; no new random-monster generation or encounter tables ship.

## Physical impact, recruitment and poaching

Train and Secret Hunt consume one accepted ordinary/Choco Attack HP impact.
They do not run through misses, Petrify, Wall, Golem absorption, MP Switch or
magical HP effects. A zero-damage successful physical hit may Train an already
critical target. Train requires a surviving target at or below maxHP/5, and
retains Invite immunity and immortal filtering. Existing Invite cannot be reapplied to steal the unit again. Permanent camp, involuntary AI,
terminal checks and victory recruitment reuse the existing allegiance owner.

Secret Hunt removes a defeated non-immortal source monster once, clears its
push/trophies and avoids the ordinary crystal/chest lifecycle. One source
roll256<31 selects rare loot; otherwise common loot. All48 actual source loot
pairs are archived. The explicit native inventory policy is direct blue-party
receipt capped99, rather than a Fur Shop stock/shop UI. Enemy poaching cannot
add party stock. Repeated impact cannot reroll, recruit or duplicate loot.

Recruited monsters are persisted as SaveUnit.type3 with their source identity
and raw stats. The current campaign has no compatible monster art bank: they
remain in reserve, with their source name/stats and no borrowed human portrait.
Deployment rejects them, and management rejects equipment, learning and human
job changes. The headless scenario door exercises their real core commands;
this block does not introduce monster encounters or claim playable monster
art/content. Existing human parties and deployment remain unchanged.

## Move-Find Item and saves

The common archive exports all128 source maps' four hidden-item records,
including position, layer, rare/common IDs and the original flags. The current
chapter aliases are source Gariland22 and opening Orbonne56, verified against
the extraction pipeline. Both have no nonzero hidden item entries; no loot is
invented for those pilot maps. Positive tests inject real records from other
source maps into the bounded core door, rather than changing campaign scenery.

Accepted movement settlement checks the equipped/innate movement capability,
exact panel/layer and permanent found flag. Discovery always consumes one
source rare/common draw, including Brave0/100, then sets the map/entry bit.
Blue inventory includes worn equipment in the99-item cap; a saturated item
sells for its source price/4 with the gil cap. Existing item receipt presentation
is reused. Traps, mounts, a separate sold-item message and original discovery
animations are outside this bounded adaptation.

FSV4 appends64 discovery bytes to FSV3: Save3202, SaveUnit176. Old offsets are
unchanged; FSV3/3138 and FSV2/3110 migrate with a zero discovery tail, preserving
the roster/flags/inventory and the earlier FSV2 money defaults. Every campaign
program shares that loader. Victory preserves the live discovery tail while
refreshing the disk roster; the Python decoder accepts all three versions.

## Native ownership and capacity

ABI67 keeps ModCtx352 and adds NATURE21, for22 images. NATURE owns monster
refresh/post-impact, the isolated discovery door and cold Math masks. Native
owners stage requests and return before this image loads; no executing image
loads another. EQUIPMENT retains accepted-move settlement. PARTY only seeds
the monster-present mask during native setup; the resident refreshes after
return. Status, allegiance, impact and accepted moves invalidate the skill cache.

Cold chapter-file loading moved from SAVE to NATURE; global performance
ordering validates its known self-centre directly. Math AI freezes16 masks in
existing scratch before AREA_AI, retaining pure ordinary Math queries. These
relocations recover resident/image room without replacing the engine.

Unit112 and State2224 remain fixed. Work45588 adds two16-bit monster request/
presence masks; Save adds64 bytes. The five allocation blocks remain. FCJH49306
adds2720 archived bytes: loot96, maps2048, aliases64 and source prices512.
Old FCJG is rejected. Three FMB3/FMD2 shards7/7/8 are51204/51062/49714 bytes;
image zone8566. Actual non-BENCH TTPack input64581/64936 leaves355 bytes;
raw variable64579+2 matches it, and private packed data is39579 bytes.

## Verification

`make core-discovery-check` runs2916 unchanged retail MIPS profiles,17635
logic assertions(hash4880B193) and35 management/deployment assertions
(hashA633E2AD). PC, ASan/UBSan and actual68000 agree, including two management/
deployment frame hashes, native FSV4/FSV3/FSV2 VAT round trips and source monster
commands. The catalogue reader checks actual disc bytes and prices on PC/68000
(hashB9686645). All nine isolated campaign unit suites pass.

The source oracle explicitly isolates discovery tile lookup/current-map event
variables and Monster Skill's total-height callee; its decision/flag/RNG code
is unchanged retail MIPS. Native total-height calculation has separate boundary
fixtures. See `x/core-discovery/oracle/source.json` and `ffdn0.bin` for hashes and
scope. This is not an original whole-battle or AI replay.

Final affected closure, private campaign flows, preservation and production
measurements are recorded in `x/core-discovery/verification.json`.
Full ranged Hamedo/rejected-candidate RNG, broader weapon/proc/AI fidelity,
automatic Mime imitation, full monster content, whole-frame/stack/residency
and release checks remain the final integration/content work.

Final closure:35 affected suites pass after focused catalogue/fixture repairs,
including a visible hostile Dispel case and a separate Transparent rejection.
The scalar batch fixture now uses private IDs272..287 to avoid real Chocobo
records. No assertions were removed. Final native Protect, Combat, all22 image
bindings and five campaign carriers were rechecked after the last cold-index/
Train/Choco Attack adjustments. The generated height helper retains a byte
index instead of emitting `__mulsi3` for a112-byte pointer difference.
Twenty campaign input/pixel/state/save replays pass. All522 protected files keep
both bytes and dates. No live emulator, standard group, commit or push was used.
