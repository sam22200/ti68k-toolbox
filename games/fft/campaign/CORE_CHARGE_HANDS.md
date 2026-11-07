# Dual Charge: per-hand power, delayed validation and criticals

2026-10-26. L2 follow-up to CORE_HANDS.md. The eight Archer Charge powers now
use the supported one/two-hand count for deferred execution, forecast and AI.
Other weapon-skill families, weapon procs/elements and broad original targeting
remain gated. This does not open the full L2/content-production gate; Jump
refinements stay deferred.

## Source observation

`tools/charge_hands_oracle.py` executes unchanged local-disc MIPS for eight
Charge initializations/hand selections and sixty scalar cases. The original
Charge menu gives two strikes for the equipped Two Swords pair. Each hand's
base XA receives Charge power before target-status and zodiac adjustments.
Critical chance is4%; amplitude adds `floor(roll * XA /32768)` after those
adjustments. A zero-amplitude critical still requests recoil.

Original initialization, used-weapon selection, base-XA, Charge addition,
status/support/zodiac and critical threshold/amplitude instructions run.
The random helper supplies fixture values; the recoil routine is isolated and
counted. The fixture multiplies the resulting original XA/YA. This is not an
original full delayed-turn, HP/target rebuilding, terrain recoil or animation
trace. Provenance/function/disc hashes and exact inputs are recorded in
`x/core-charge-hands/oracle/source.json`. Earlier48 controller-observed previews
and committed CT values remain the single-hand source evidence in CHARGE.md.

## Shared resolution

Arming retains both actual weapon IDs, including lone-left promotion. The
second ID occupies the existing deferred-record padding byte: record8 bytes,
State/Work/Unit/FSV3 and allocation count stay unchanged. Either weapon changing,
KO, displacement or Sleep cancels through the common rule; Stop pauses it.
Don't Act remains checked at due time. Menus do not advance CT.

At due time Charge retains the stored compact tile and one root. Its second
strike retains the ability ID, power and hand rather than falling back to a
plain Attack. Each strike rolls hit/critical separately and commits once;
an empty/KO tile after recoil or damage skips the next strike. Reactions drain
after the primary strikes, EXP keeps the best award and JP applies once. A
nonterminal resolution resumes CT once; terminal battle handling retains its
existing path. Counter/Auto-Potion use the existing bounded response ownership.

The existing physical kernel now takes an optional Charge power. Attack keeps
its old999 cap; Charge preserves the previous raw product policy, including
source-tested values above999. The pure `charge_query` callback supplies the
same hand/status/zodiac/critical arithmetic to local evaluators without loading
another image. No live equipment is swapped. Forecast and AI explicitly request
the potential sum through `FORMULA_FORECAST` (bit15 of a Charge query; ordinary
random inputs stay0..32767). After a successful first hit, mutable Sleep loses
its vulnerability for the second; innate Sleep remains protected.

The preview retains its existing potential-hit model and first-hit percentage,
not a new probability tree for misses/criticals/recoil/reactions. Existing
native Charge targeting/range and direct-line adaptations remain separate
geometry work; these scalar/adjacent-tile fixtures do not certify full original
weapon range, bow ballistics or arbitrary layered targeting. No art/style is
changed. Existing Charge poses/effects and ordinary critical recoil are reused.

## Image ownership and limits

Local COMMAND/FORMULA Charge preparation uses resident pure math/hit/recoil
callbacks and returns before another image can load. The impact preload from
CORE_HANDS.md is retained. Native pickup code moves unchanged into PARTY's cold
operation6 to regain resident packing space; its caller returns to the same
crystal/chest state and inventory/RNG behavior. Outcome/turn logic stays resident.
ABI18 includes the pure math callback and new deferred-byte meaning; old images
must reject even though State and save sizes do not grow. FCJ8/data are unchanged.

## Targeted evidence

`check_charge_hands.py` covers original scalar boundaries and all eight powers,
per-hand committed damage, preview purity, CT/menu timing, both weapon-change
cancellations, KO/displacement/Stop/Sleep, mutable/innate Sleep, first-hit recoil/
KO, actual critical rolls on either hand, post-strike Counter/Auto-Potion,
terminal/CT settlement, AI ranking and the relocated crystal/chest caller.
Native tests compare with PC/sanitizers. Direct callers are ordinary hands,
strikes, reactions, contexts, existing actions/Charge, control and Sleep;
13-actor boundaries, unchanged pictures, pilot flow and native layout/packing
close the ABI integration. Unchanged catalogue/geometry/timer policy evidence
is reused, without a map census or exhaustive sweep.

**RNG measurement correction:** battle randomness uses `rng32` through rand15;
`rt_seed` is a separate runtime seed and cannot establish battle-RNG purity.
Scalar/preview queries must preserve rng32. Complete AI selection legitimately
draws randomness even when its first candidate has100% hit chance. The scoped
AI case verifies exactly that existing choice draw and no additional query draw.
Earlier CORE_HANDS.md's runtime-seed assertion was narrower than its prose claim.

Final measured results: `x/core-charge-hands/verification.json`. Keep the standard
playtest group and the user's live TiEmu unchanged.

```sh
make -C games/fft/campaign core-charge-hands-check
```

## Final measurements

| Evidence | Result |
|---|---|
| Dual Charge/source/AI/cold pickup |586 PC/sanitizer/68000 checks, `0F008472`|
| Ordinary hands |197 checks, `A1332BDA`; Charge enabled, other gated skills rejected|
| Strike/reaction/context/action |250/267/348/295 checks; preceding hashes retained|
| Existing Charge/control/Sleep |`276A2090` / `15C96953` / `B6C26F11`; PC equals68000|
| Pilot Gariland/Orbonne flow |`AE9CFD31` / `2CD70391`; native save/settlement equal|

The thirteen-actor boundary integration passes. Six normal and six response
pictures preserve previous PC/native plane hashes; all eleven pilot PNGs keep
both bytes and modification dates. Existing catalogue/geometry/timer/management
evidence is reused. Source data and the standard playtest group are unchanged.

| Storage | Ordinary hands | Dual Charge |
|---|---:|---:|
| Private42-variable archive |488801|490371 (+1570)|
| Raw image bank |50710|52328 (+1618)|
| Packed battle payload |42300|42252 (-48)|
| Battle TTPack input / headroom |64821 /115|64845 /91|
| Largest image / ModCtx |5750 /328|5814 /332|
| Native State / Unit / Work |2124 /112 /45516|unchanged|
| FSV3 save / SaveUnit |3138 /176|unchanged|

Only ffbcore/ffbm change; all28 non-code banks retain hashes. ABI17 images are
rejected despite equal State/save sizes. Five simultaneous requested heap blocks
still total114834 for Gariland,114474 for Orbonne and126842 at maximum topology.
These are allocation formulas, not measured stack/fragmentation/combined residency.
The91-byte packer margin still requires relocating further resident growth.

Native second-Charge preparation takes42338 cycles in each of eight scoped
cases. The existing23-call primary impact suite averages15982/peaks57494,
versus15364/54446 at the hands checkpoint. Zones exclude rendering and do not
certify complete-frame latency/FPS. No new assembler or TiEmu input was used.

Next: fixed-cell versus unit-following magic, target sets/AoE and equal-tick
scheduling; then further reactions/statuses and broader weapon policies. Full
L1/L2/L3/L4 and map-production gates remain open, with Jump kept low priority.
