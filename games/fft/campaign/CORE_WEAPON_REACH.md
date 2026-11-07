# Resident headroom and source weapon reach

Historical capacity/ABI checkpoint. The shared range behavior remains current;
[CORE_WEAPON_FORMULAS.md](CORE_WEAPON_FORMULAS.md) supersedes the capacity,
callback and physical-damage limits below.

The turn-clock loop and deferred-slot selection now live together in ORDER16,
using operations17/18. The resident returns before either cast or turn execution
loads another image. ORDER uses its local status/dead-turn helpers and local
pending selector inside the loop; there is no per-tick image swap. PC and legacy
nonchapter builds retain the same shared C loop. This first recovers786 bytes
of actual production input:64917 becomes64131 before the weapon extension.

The four source targeting shapes now share one pure predicate for ordinary
Attack, manual ranges, AI, Counter/Hamedo, subsequent strikes and Charge/Mime
copies. The source flags already exist in the archived equipment table; no new
data catalogue, Unit field, allocation or save format is added.

- Striking: one Manhattan step, height difference from-6 through+5 half units.
- Lunging: cardinal distance1 or2; lower height limit is2*distance-10 and the
  upper limit is+7. Diagonal distance2 is not a legal lunge.
- Direct: Manhattan distance3 through the weapon's fixed range; no downhill bonus.
- Arc: minimum distance3 with the original signed height adjustment, including
  the previously missing uphill penalty. Quarter-height rounding truncates
  toward zero, implemented with shifts on nonnegative magnitudes.

Weapon selection retains right-hand priority, left-only fallback and bare-hand
Frog. Source-effective origin height includes Float and water movement traits;
AI origins may be hypothetical tiles. Target panel heights use water surfaces.
Manual strike/lunge masks enumerate only cardinal neighbors. Arc/direct masks
prepare the origin once and scan the board. Both fill existing storage with the
same rule; point queries for AI and reactions remain pure.
No renderer, art, portrait, timing or projectile presentation is changed.

## Original evidence

`make core-weapon-reach-source-check` executes hash-verified retail MIPS:
weapon selection, striking/lunging/arc/direct marking, direct spread/close-range
filtering and effective height.160 real-weapon/hand/Frog/height profiles produce
54080 panel results.64 further height profiles include Float, water movement
and byte-height wrapping. Only final target-selection filtering is isolated;
the weapon-range algorithms execute unchanged. The footprint export excludes
the origin, as the original attack-panel caller does. Evidence and callee scope:
`x/core-weapon-reach/oracle/source.json`.

`make core-weapon-reach-check` compares source footprints and live native
commands: both UI ranges, cardinal lunge, left-only/Frog, Hamedo/Counter at two
tiles, uphill/direct limits, unchanged RNG queries, actual AI choice and clock
entry. PC/sanitizer/native results and cold/warm query costs are recorded in
`x/core-weapon-reach/verification.json`.

## Bounds and next steps

This closes weapon-panel reach, not every property of every weapon. The existing
native straight obstruction query and projectile/strike graphics are reused.
Exact original arc trajectories, lunging collision, complete overlapping-panel
victim listing, weapon damage formulas/procs and special innate interactions
remain in the weapon checkpoint. Native mounts and full original multi-floor
semantics remain outside the current adaptation. Existing non-range ability
geometry is not silently converted into weapon reach.

ABI70 adds ORDER operations17/18 and changes shared range semantics. ModCtx352,
Unit112, State2224, Work45608, Save3202/FSV4, five allocation blocks and the
8566-byte execution zone remain fixed. No standard group or live TiEmu is changed.

## Final measured checkpoint

55703 assertions(88CBF461) agree PC, ASan/UBSan and actual68000. All22 affected
suites close, with status-lifetime4140(D0F9207E) and incapacity195(D045EDF2)
checked after their fixture repairs. The old incapacity RNG expectation also
failed with preserved ABI69 sources; the repaired test now checks actual pre-RNG
Petrify rejection and retains post-formula/finalizer checks. No gameplay rule
was changed for that test. Six affected suites received final rechecks after
batch preparation; sparse-cardinal enumeration was then checked by the full
focused suite and five native campaign carriers. The resident and20 other images
are byte-identical across that last improvement.

The four cold/warm manual-mask pairs average36328/50324/223688/134750 cycles
for strike/lunge/arc/direct respectively in the fixture. Cardinal masks avoid a
board scan; ranged masks prepare the origin once. The100-tick stress clock costs
517844 cycles; a ready warm clock entry costs1896. These bounded setup zones
include the runner's estimated ROM costs and do not certify a whole tactical
frame. Combined frame/stack/residency remains a separate checkpoint.

Final archive shards51140/51810/53244 retain7/7/8 images and zone8566.
Actual non-BENCH input64897/64936 leaves39 bytes; raw variable64895 and private
packed data file39842. The final resident payload is byte-identical to the
previous measured packed build. FCJH49306 is unchanged; all128 weapon ranges
fit the packed nibble (actual maximum8), and all carry exactly one shape flag.
Current-bank bindings203/parser140 and actual old ABI69 three-shard rejection
pass. Gariland/Orbonne remain81/14 checks(AE9CFD31/2CD70391), with final native
pixel carriers and exact FSV4 save bytes. All550 protected save/group files
retain their original bytes and access/modification dates.
