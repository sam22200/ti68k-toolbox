# Long horizontal jumps: source-checked geometry prototype

Subsequent checkpoint: [CORE_RESIDENT.md](CORE_RESIDENT.md) resolves the resident
packer ceiling through a ninth PARTY module and Status input relocation. The
wider-cost routine is now included in the current engine; long links and actor
record generation are still pending. Optional-only packaging statements below
describe the preceding checkpoint.

2026-10-25, workstream4, after [CORE_STEPS.md](CORE_STEPS.md).
**Status: geometry verified; NOT connected to shipping movement.**
Subsequent prerequisite: [CORE_SPAN_SEARCH.md](CORE_SPAN_SEARCH.md) verifies
costs1..5 in an optional bounded queue with unchanged storage. It is not enabled
in the bound shipping build: resident code exceeds TTPack's input limit until
capacity is addressed. Bounded links and actor spans also remain open.
`../long_jump_geometry.h` is included only by the dedicated fixture. No
archive, save, ABI or runtime allocation changes from this prototype.

## Source evidence and scope

`../tools/long_jump_oracle.py` runs unchanged local-disc original MIPS under
Unicorn2.1.4. It executes the full original horizontal spread, not just an
isolated landing predicate. Function bytes are hash-checked; the original
preset pointer table is checked against the east-direction function.

|Function|Address|Bytes|SHA256 prefix|
|---|---|---:|---|
|Horizontal spread|0x801764D8|476|87e689eaa38fd655|
|Intermediate span|0x8017622C|684|551a8a31fcf7897c|
|East preset|0x801754D0|36|21380e6d5d58f243|
|Coordinates|0x801755E8|48|3685b0b0b943fe6a|
|Bounds|0x80175618|100|817c8e48177c2808|
|Unit movement configuration, STATIC evidence only|0x80174430|1884|24f5bfdb9efb73c1|

The source flag/source-panel/destination routines are also executed as recorded
in CORE_STEPS. Unit configuration itself is not executed: its static code caps
ordinary Jump at7 and derives `jump_half`; the oracle configures that value
before the original spread. Fly/IgnoreHeight have separate overrides and are
outside this fixture. Do not infer their rules from this normal-unit model.

The22 original cases cover Jump0..8 and255, gaps, raised first/second/third
intermediate spans, high/low bridges, upper landings/source, descending versus
ascending landings and bounds. The fixture is an eastward lane of up to5 cells,
remaining budget16, unit terrain costs1, no actors and ordinary body height6.
It records both lower/upper landing masks and source costs2..4. This is not a
complete played PS1 battle, actor collision or water-cost certificate.

**Observed:** ordinary Jump6/7 permits up to3 intervening cells (landing4 cells
away). Every intermediate solid span matters. A blocking span stops the rest
of that direction. A rejected landing alone need not end further exploration.
Endpoint checks retain the accepted edge/steep/headroom rules. Forward landing
height cannot exceed the source edge for a horizontal leap.

## Native prototype and focused checks

`long_jump_masks` returns three lower/upper masks, one per possible gap count.
It uses fixed direction/side tables consistent with world coordinates, checks
bounds before addressing each column and caps Jump through the shared helper.
Per-map flags still choose one/two solid layers and skip unnecessary room tests.
No terrain is inserted as virtual compact nodes, and no tile ID is a sentinel.

278 checks, hash `7255D93F`, agree PC, full ASan/UBSan and actual68000. These
include22 original trials plus portable policy/directional/bounds checks;278
is not a count of independent PS1 trials. Source execution is eastward; the
portable flat-profile fixtures additionally exercise all four directions and
an invalid-direction guard. Artifacts: `x/core-long-jump/results.json`,
`oracle/source.json` and `native.log`.

```sh
make -C games/fft/campaign core-long-jump-check
```

## Runtime integration gate

Shipping movement still has only one intervening cell. Do not extend its loop
without changing all of these contracts together:

1. **Bounded graph capacity.** Current rows hold16 links (4 directions, ordinary
and one-gap destinations, up to2 floors), plus one via ID per link:48 bytes/node.
Four possible distances and two landing floors can require32 links. Account
for source record degeneracies; do not assume just one valid floor per span.
Measure a compact bounded representation rather than blindly doubling every
map's allocation. There are no spare compact IDs when all256 nodes are valid.
2. **Search costs and FIFO buckets.** Current Dial queues support costs1..3 and
use `distance &3`. Long dry spans already reach cost4 in the original trials;
water policy needs separate verification. Costs>=4 violate the present queue
proof. Extend bounded queue/cost validation before allowing those links,
preserve stable tie order, yielded-search behavior and the node255 guarantee.
3. **Actor spans and path extraction.** One via ID checks only one intermediate
node today. Every intervening column/layer must apply the actual actor policy,
including bodies not represented by a stoppable node. Test blocked second and
third columns, landing occupancy and lower/upper actors. Review animation timing
and path extraction for leaps that cross several cells.
4. **Integration/budget checks.** Add actual reach/path fixtures for all new span
lengths and terrain costs, update allocation rollback/capacity fixtures if layout
changes, rebuild affected module consumers and measure cold graph/search CPU.
Run only directly affected battle/view cases; unchanged large certificates stay
valid until their dependencies change. Review any changed visible pixels.

Movement abilities/body sizes, source actor policies, bridge raster cover,
action/status contexts and full combined RAM peaks remain separate open gates.
This prototype is progress toward core completion, not production approval.
