# Ordinary Two Swords: independent hands and shared prediction

Follow-up2026-10-26: [CORE_CHARGE_HANDS.md](CORE_CHARGE_HANDS.md) now enables
dual Archer Charge; the gate below describes this earlier checkpoint. Its
586-check fixture also corrects RNG measurement: `rng32` is battle randomness,
while this checkpoint's AI assertion checked only `rt_seed`. Full AI selection
legitimately draws once; pure scalar/forecast queries must not draw.

2026-10-26. Follow-up to CORE_STRIKES.md. Ordinary Attack and Counter now use
their own first/second weapon without swapping live equipment. The enabled
subset is unarmed strikes and verified plain knives, ninja blades and swords.
Special formulas, elements and weapon procs are not enabled by this checkpoint.
Charge/weapon skills with two strikes remain unavailable to player and AI until
their own hand-specific source cases are implemented.

## Storage and source contract

The compact five equipment slots already represent head/body/accessory/right
hand/left hand. The left-hand slot accepts either a shield or a supported
weapon. A weapon has no shield evade; item attribute bonuses from both slots
already pass through the common stat calculation. FSV3/SaveUnit176/Save3138
remain unchanged. Party loading retains the source equipped-support ID in a
transient thirteen-entry array. Saved support/equipment selections survive the
existing party update, rather than being inferred from learned abilities.

FCJ8 reuses each30-byte item's unused final byte for the verified dual-weapon
flag and adds20 archived bytes for source job innate Two Swords bits. Ninja
and the three source special-job records share this data-derived policy; no
map/job-ID exception is needed at runtime. Old/short FCJ7 banks reject.
Ninja blades share the existing Speed/PA weapon category and dagger animation.
This is a reuse of existing native art, not new original-animation extraction.

`tools/hands_oracle.py` executes unchanged original MIPS for ten initializer/
used-weapon selections and twenty base-XA/status/support/zodiac scalar cases.
Both empty weapon slots can yield two punches; two weapons yield two strikes;
a single weapon remains one, including a weapon promoted from the left slot.
The scalar fixture multiplies original XA/YA without critical/element/proc.
Source/function/disc hashes and the scope are recorded in
`x/core-hands/oracle/source.json`. Earlier strike-phase/EXP observations are
retained in CORE_STRIKES.md. These are not a full original dual-weapon battle,
target rebuilding, probabilistic AI or animation certificate.

## Runtime and management

Ordinary Attack derives one/two strikes from equipped or innate Two Swords and
the supported hand pair. The strike owns a hand index; primary suspension also
saves/restores it. Counter can likewise perform both hands before returning to
the root; a dead/invalid target skips the remaining strike. Root effects,
reaction suppression, costs, CT, reward maximum and unique settlement retain
the shared context contracts. No preview temporarily equips the other weapon.

The existing preview line and AI score use a shared potential-damage sum. If
the first hit wakes mutable Sleep, the second uses normal vulnerability;
innate protected Sleep persists. This keeps the existing potential-hit AI
approximation and first-hit percentage, not a new probability tree accounting
for every miss, critical, recoil or reaction. Existing single-hand results are
unchanged. Source-specific full AI simulation remains a separate backlog item.

Management offers a supported left-hand weapon only with the capability and
the job's source equipment categories. Removing the capability, changing jobs
or equipping an incompatible right-hand weapon returns the left weapon to the
bag. Existing selector/layout/art is reused. Unsupported/proc/elemental dual
pairs are not offered; the metadata flag does not certify those formulas.

## Image ownership and packing

COMMAND prepares actions with local start/evaluator code and existing resident
hit/recoil callbacks. A due Charge returns from COMMAND before its caller can
advance AI or load another image. RESPONSE owns packet/stock/reaction preparation
and deferred reward display; ACTION keeps impact/policy and immediate rewards.
This removes cold work from the resident program and splits the previously
largest ACTION image. No callback evicts its executing caller. ABI17 adds the
resident hit/recoil callback bindings; every image and packed consumer rebuilds.
COMMAND must reject before the resident request fields change, not just before
costs/RNG. The context regression caught that distinction and now preserves
whole-State hashes for rejected/overlapping commands. A reserved preload-only
operation255 loads ACTION after COMMAND returns during wind-up; it executes no
module entry and avoids moving the cold copy onto the ordinary skill impact.

## Evidence and remaining scope

`check_hands.py` covers source hand/scalar cases, actual different weapons,
mutable/innate Sleep, unique rewards/equipment purity, both Counter hands and
first/second lethal returns, saved support loading, unsupported skill gating
and complete native AI ranking with unmodified world and runtime rt_seed.
The later Charge fixture checks the distinct battle rng32 and its legitimate
AI-choice draw; this earlier test did not establish battle-RNG purity.
`check_hands_manage.py` covers actual capability removal through the common
support picker, stock returns, job changes, innate eligibility and save sizes.
Direct regressions are strikes, reactions, root contexts, actions, Charge and
the changed catalogue reader, plus normal action/response planes and the
Orbonne/Gariland setup/save/results replay. Unchanged geometry/timer evidence
is reused; no map census or live TiEmu input is needed.

Final measurements and hashes: `x/core-hands/verification.json`. The standard
playtest group remains separate. L2/L3/content and full residency/frame gates
stay open; Jump refinements remain deferred. Next: broaden supported weapon/
skill policies from source fixtures, then further reaction/action families.

| Scoped evidence | Result |
|---|---|
| Hands/AI/Counter/source/save loading |196 PC/sanitizer/native checks, `510B58D2`|
| Management capability/stock |16 PC/sanitizer/native checks, `F7900518`|
| Existing strike / reaction / root / action regressions |250 /267 /348 /295 checks; previous hashes retained|
| Charge / catalogue reader |`276A2090` / `B9686645`, PC equals68000|

The six normal and six response-state pictures retain their preceding PC/native
plane hashes. All eleven pilot flow PNGs retain bytes and modification dates;
the flow rechecks setup/results/save, repeat payment and overflow handling.
Existing geometry/timer code and data are unchanged.

| Storage | Strikes checkpoint | Hands checkpoint |
|---|---:|---:|
| Private42-variable archive group |483414|488801 (+5387)|
| Raw image bank |45508|50710 (+5202)|
| Packed battle payload |42339|42300 (-39)|
| Battle TTPack input / headroom |64923 /13|64821 /115|
| Largest image / ModCtx |5830 /320|5750 /328|
| Native State |2094|2124 (+30)|
| Unit / Work / FSV3 save |112 /45516 /3138|unchanged|

The support array, current hand/alignment and22-byte primary packet add30 State
bytes without another allocation. Five requested battle blocks total114834 for
Gariland,114474 for Orbonne and126842 at the maximum topology. These are layout
formulas, not stack/fragmentation/full program-residency measurements. The115-byte
TTPack margin remains tight; further resident growth still needs a module budget.
FCJ8 adds20 raw archive bytes, with changed item metadata and shifted name
offsets. The other27 non-code banks remain unchanged; old ABI16/FCJ7 reject.

Native second-hand preparation averages38901 cycles, peak39200 in three scoped
cases; Counter preparation is27918 versus45966 in the preceding reaction tests.
The final23-call primary impact suite averages15364 cycles, peak54446 versus
13931/53892 before. A first COMMAND layout copied ACTION at impact and averaged
42614/peaked85210; preload moves that copy to startup. It does not remove the
copy's total cost or certify startup latency/full frames. These zones exclude
rendering and are not hardware FPS. No new assembler or emulator input is used.

```sh
make -C games/fft/campaign core-hands-check
```
