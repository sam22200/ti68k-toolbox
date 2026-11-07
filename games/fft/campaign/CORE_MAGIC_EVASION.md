# Magical evasion and ordinary formula08 target eligibility

Current follow-up: [CORE_WEATHER.md](CORE_WEATHER.md) adds formula08 weather,
Float/earth and protected Oil removal; earlier environment gates/numbers are history.

2026-10-27. Follow-up to CORE_ELEMENTS.md. Formula08 now uses the same
magical-evasion percentage and target-validity policy in forecast, AI eligibility
and execution. A private normalized Fire carrier exercises evadeable magic;
production Holy/Fire and every AF_CAST producer remain disabled. This closes
this ordinary unmounted-human slice, not complete spell/target-list support.

## Original observations

`tools/magic_evasion_oracle.py` runs unchanged original MIPS magical-evasion,
formula08, target-presence and finalizer instructions on238 profiles. The source
item tables and secondary IDs provide accessory/right/left shield magic evade.
Normal weather and unmounted human units are explicit. The external roll helper
receives deterministic15-bit draws with its exact source scaling; graphical
sorting on final invalidation is isolated. Inputs, draws, function/disc hashes
and the decompilation commit are in x/core-magic-evasion/oracle/source.json.
This is not a full original battle/HP/status/reaction/EXP/area trace.

Magical evasion has no class, weapon-guard or facing contribution. The source
checks accessory, right shield, left shield, then class. Even a zero channel
consumes a draw; execution stops on the first failure. Fire can therefore consume
four evade draws plus the existing generic proc draw. Holy bypasses all evade
draws. The port preserves probabilities and execution draws; previews remain
pure even though the original preview path calls its random helper.

Defend halves base100; active Abandon halves it again. Transparent on the caster
and the existing Sleep/Stop/Confusion/Don't Act/Charging/Performing target policy
zero evade channels. Abandon's deep-water restriction uses source byte2 mask0x46
(Float/Chicken/Frog), not the older gated reaction adapter's0x0e. The latter
still needs its own movement/status-producer milestone before those producers
activate. No new Transparent/Abandon/Wall/monster/mount/movement producer is enabled.

For base100/50/25, percentages are100,100-2*evade or100-4*evade per channel.
The product is divided only after all channels; intermediate percentage rounding
would be wrong. The pure kernel uses existing verified MULS16/DIVS32_16, no
float, software32-bit multiply/divide helper or newly authored assembly.

The source presence predicate excludes absent slots, Crystal, Treasure, airborne
Jump and mounts. Dead/Petrify/Wall remain present, then ordinary formula08 effects
are rejected by finalization after their evade/formula/proc draws. The native
adapter distinguishes absent, eligible and blocked present targets. Canonical
Dead and hp0 are both blocked; this also handles the port's existing KO model.
General status-canceling/resurrection, Undead, mounts and monster policies remain
separate. Follow bindings retain their previous generic rules; formula08 checks
presence at due launch. Empty/absent centres pay once and skip effects; their
visual presentation and complete source target-list/refund trace remain open.

## Shared execution and data contracts

`magic_hit(actor,target,ability,execute)` returns pure percentage for execute0 or
source-order success for execute1. It never loads another image. `magic_target`
returns0 absent,1 eligible or2 blocked. Formula queries/forecasts and actual AI
eligibility use these resident callbacks. Execution pays once, performs evasion,
then elemental nullification, formula/proc draw and final validity rejection.
A miss clears quantity/category; it causes no HP change or effect reward.

Elemental nullification occurs after evasion: an unopposed Fire null consumes
four evade draws, then no generic proc draw. An unopposed Holy null consumes
none. Prepared elemental outcomes and response ownership remain as documented
in CORE_ELEMENTS.md. AI still uses the existing adapted potential-effect ranking;
this does not certify original probabilistic/delayed-turn/healer-role scoring.

FCJA replaces FCJ9 while preserving all14 section offsets and41086 raw bytes.
Item remains30 bytes: its existing last byte holds Two Swords eligibility bit0,
magic evade bits1..6 and shield tag bit7. All256 source records fit (maximum50).
Slots use the tag, not item-ID ranges. All unrelated catalogue bytes and every
previous dual-weapon bit remain identical. Old FCJ9 rejects explicitly; readers,
management/deployment and exporters use the same new format. FEA1 is unchanged.
The actual management support/equipment path retains its16-check native hash
F7900518; PC deployment save/resume/reserve/FSV2 migration also passes.

ABI24/ModCtx360/signature61752 rejects old ABI23. New callbacks are pure resident
functions; FORMULA/AI/COMMAND never evict themselves. To fit the resident packer,
RESULTS1 owns cold cached-row label construction through a transient union
pointer. It compiles the same availability helper locally. Frame/row geometry,
shared choice drawing and one-row-per-part scheduling stay resident. The source
extractor follows the moved tables; generated contour/art bytes stay identical.
No heap allocation or per-frame bulk asset copy is added.

## Evidence and budgets

`make core-magic-evasion-check` / `check_magic_evasion.py` checks238 original
profiles, all four facing pairs, pure forecast/RNG, actual COMMAND/ACTION payment,
miss/hit/HP behavior, late KO/Petrify/Wall and absent-unit invalidation, paid
nullification, actual AI rejection and cold menu-label loading.7510 assertions
pass on PC, ASan/UBSan and actual68000 modules:995C7993. The larger test fixture
lives in a private FME1 archive, rather than inflating the native test executable.

Native pure query average1714/max3056 cycles; sequential evasion average1858/
max3630. The cold command/preload probe averages87602/max89760 (not a full-frame
claim). Cold label construction/drawing costs45716, below the existing60000-cycle
HUD-task envelope. See check/results.json and native.log for exact zones.

Private group498763 bytes, +1944 from CORE_ELEMENTS. Resident TTPack input64899,
37-byte headroom; payload42113. Further resident work still needs relocation.
Raw module bank60026; images6146/3540/4250/5782/2838/5794/4966/5584/6072/6124/
5346/3434; largest6146. Only ffbcore/ffbm/ffcj and the two common-reader program
payloads change;27 other non-code banks retain their bytes.

Unit112/State2190/Work45516/ActionPacket22/SaveUnit176/FSV3 Save3138 and the five
battle allocation requests stay unchanged. The module zone grows50 bytes inside
the program; this is separate from battle heap. Fresh compiler layouts, targeted
scalar/element/affinity/incantation/Charge/action/flow/catalogue closure, twelve
reviewed frame hashes and old-ABI rejection are recorded in verification.json.
This is not a new peak-stack/fragmentation/combined-residency certificate.
No full geometry/census sweep is repeated. Existing image bytes/dates, standard
playtest group and live TiEmu are untouched.

## Next gate

Weather/Oil/Float and remaining target/trait policies, then area target sets,
per-target effects/rewards/reactions and reviewed cast presentation. Enable retail
spells only after their supported contracts pass. Reflect/Mime, other action/status
families, campaign lifecycle, map/scene v1/editors and worst-case residency still
block bulk content production. Jump refinements remain deferred.
