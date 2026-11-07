# Ranged Hamedo integration

Historical checkpoint. The newer [CORE_WEAPON_REACH.md](CORE_WEAPON_REACH.md)
supersedes the extended-reach assumptions and ABI/capacity figures below;
Hamedo now shares source strike/lunge/direct/arc panel ranges.

Hamedo now uses the reactor's native weapon reach and the existing projectile
obstruction query. A clear eligible return replaces the unpaid command, including
both original hands, Throw and due Charge. A blocked/out-of-range return leaves
the original action to execute and pay normally. Successful pre-emption cancels
the incoming action even when the return subsequently misses. It grants no
primary EXP/JP, starts no reaction chain or Mime donor and retains the reactor's
independent pending command and CT.

## Source and implementation

OBSERVED: `battle_reaction_prepare_hamedo_for_pending_action` requires an accepted
physical weapon/Charge/Throw command and one other selected target with Hamedo.
It draws the target's Brave chance before checking PREVENT_REACTION and preparing
the return's target/range/preview. Failed incapacity and counter-preview cases
therefore retain that first random draw. The return can use a ranged weapon;
there is no melee-only condition in the original outer routine.

The bounded original oracle executes unchanged retail outer selection, Brave and
status checks. It isolates MP validation, the incoming weapon-target/list,
return geometry/preview and strike setup.36 Brave/status/return profiles and
four early gates verify actual instructions and function hashes. This does not
certify original arc/direct/lunging geometry or a whole PS1 battle.

TARGET: `hamedo_begin` retains incoming-family gates, draws Brave and then checks
live eligibility/reach/trajectory. Native resident `hamedo_line` preflights the
pure return trajectory before RESPONSE4 or COMBAT0 executes; it restores the
forward scalar request and stores the result in idle `Work.ai_traj` scratch.
No image loads another executing image. PC calls the same pure line query
directly. The query consumes no RNG, MP, stock or pending command. Each command
refreshes the scratch, so a rejected line cannot reuse a previous clear shot.

The existing pre-action packet/cursor128 still owns cancellation, extra strikes,
once-only impact, response draining and terminal checks. No new gameplay fields,
allocation, binding or save format are needed. ABI69 prevents an older image
from bypassing the new preflight contract.

## Verification and bounds

`make core-hamedo-source-check` regenerates the bounded retail evidence;
`make core-hamedo-check` compares the PC, sanitizers and actual68000. The focused
logic suite covers obstruction, stale clear/blocked queries, both incoming hands,
misses, unpaid Throw, due Charge, terminal/Mime boundaries and source RNG prefix
composition with the independently checked ordinary strike implementation.
The renderer suite compares existing Counter shots and Hamedo wind-up/impact planes;
there is no new art, palette, projectile renderer or timing.

The native weapon rules remain shared with ordinary attacks: current extended
reach assumes a three-tile minimum and the existing height extension/straight
obstruction model. Exact original bows/arcs, guns, reach-two and other weapon
geometry/formulas/procs remain in the broader weapon checkpoint. Native scalar
target ownership does not implement the original complete panel-target listing
for overlapping occupants. Brave parity here applies after an accepted native
single-target candidate; it is not an exhaustive original dispatcher/RNG claim.

Final results, storage/capacity, old-ABI rejection and preservation evidence:
`x/core-hamedo/verification.json`. Generic counts stay201 actions and78 R/S/M.
Remaining integration: broader weapon/proc/innate and AI/future-CT/Reflect fidelity,
combined tactical-frame/stack/residency checks, then release. Full monster art/
content and a full campaign remain separate.

Final closure(2026-11-04):345 logic assertions(1AAFBA6F) and18 renderer
assertions(3DD52CCA), identical PC, ASan/UBSan and actual68000. All13 affected
regression suites pass, including Mime's1734 checks. Both native campaign flows
retain81 Gariland and14 Orbonne checks(AE9CFD31/2CD70391), exact FSV4 VAT save
bytes, repeat-payment protection and saturated trophies. Current parser/bindings
and actual ABI68 rejection pass before module execution. Four shot frames use
the existing secondary renderer with identical pixel planes.

ABI69 retains22 images, zone8566, ModCtx352, Unit112, State2224, Work45608,
Save3202/FSV4 and five heap blocks. Three7/7/8 archive shards are51210/51832/
52010 bytes. Actual non-BENCH production TTPack input64917/64936 leaves19 bytes;
raw variable64915, packed data39969. Standard groups and live TiEmu are untouched.

The accepted ranged Hamedo setup in the render fixture costs97474 datasheet
cycles, including estimated ROM costs (one measured invocation); the existing
Counter reference setup costs314210. These are bounded action-setup zones,
not full tactical frames or a worst-case engine guarantee. All545 initially
protected save/group files retain their exact bytes and access/modification dates.

Final purity check: invalid-count and busy action requests preserve the entire
State and RNG, including transient formula actor/target fields across the line
preflight. The focused PC/sanitizer/native suite and native pixel/cycle carrier
were rerun after this resident-only correction; archive images are byte-identical
to the13-suite closure above. The final pack headroom is19 bytes. Recover
resident space before adding further weapon/AI functionality.
