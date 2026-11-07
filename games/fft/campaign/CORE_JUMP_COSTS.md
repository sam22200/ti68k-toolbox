# Long-span costs: source propagation and the six-point boundary

2026-10-25, L1 prerequisite. See CORE_ROADMAP.md for the overall core work.
**Status: a source-checked cost primitive; not connected to playable movement.**
The game package, current paths/images and live TiEmu are unchanged.

## Finding and scope

The source main executable has six64-byte terrain-cost rows at0x8005EA50.
Their bytes are0,1,2 or255. The original movement configuration treats0 as a
weather-dependent cost, preserves255 as impassable, and otherwise lets Any
Ground select1. Weather is selected before that latter override: Any Ground
alone does not replace a zero entry's weather cost. Any Weather is a separate
policy. The original weather-cost function at0x80174370 (88 bytes, hash prefix
93bdb40ee3440793) maps severe storms to3. These producer/weather statements are
STATIC original-code evidence; those producer functions are not executed here.

`tools/jump_cost_oracle.py` supplies effective terrain costs1/2/3/255 to the
unchanged original full horizontal-spread routine and executes it. It uses a
five-cell eastward lane, no actors and ordinary water-exit mode, checking three
gap lengths, budget boundaries and four source/destination water combinations.
The destination's surface selects the explicit cost table entry. This is not a
full played movement trace or certification of the real unit/weather producer.
Function hashes are checked; no original instructions are injected or patched.
The raw384-byte source table and weather-code hashes accompany the report.

**Observed propagation:** remaining points become
`source_remaining - intervening_cells - effective_destination_cost`. A positive
remaining point is required for a recorded landing. Source/destination water
geometry does not by itself change that supplied table entry. One internal
remaining point must not be confused with the player's displayed Move limit.

A destination cost3 and three intervening cells cost6. With source remaining7,
the original records1; with remaining6 it rejects the landing. Impassable255
cannot be rescued by byte wrap or a larger gap. This catches a real missing
bound before runtime hookup: the currently tested link-kind/cost representation
only supports1..5. Eight frontier buckets are not on their own proof of cost6
support; encoding/evaluation and tests must be extended together.

## Portable primitive and verification

`../movement_cost.h` implements the small static terrain transformation and
forward remaining-budget calculation. It is included only by the fixture,
not the shipping battle engine. It uses a16-bit cost intermediate so255+3 does
not wrap. Movement special abilities, body/geometry, actor generation, reverse
AI propagation and default water tables remain separate policies.

57 executed original spread queries feed68 checks, hash `9F990EF2`, matching
PC, full ASan/UBSan and actual68000. Eight of these checks assert static producer
policy/boundaries; they are not additional executed producer queries. Native
input bytes match the exact generated oracle blob. Artifacts:
`x/core-jump-cost/oracle/source.json`, `results.json`, `native.log` and
`verification.json`. No census, UI/decoder suite or package rebuild was needed.

```sh
make -C games/fft/campaign core-jump-cost-check
```

Next: fold the destination-cost contract into the bounded adjacency/search
representation, verify actual unit/weather and reverse AI policies, then connect
actor records and long spans. Do not add a map-ID or storm-only special case.
L1 remains incomplete; later L2/L3/L4 work is listed in CORE_ROADMAP.md.
