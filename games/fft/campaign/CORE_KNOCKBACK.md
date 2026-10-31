# Source recoil landing milestone

2026-10-25, workstream4, following [CORE_MAP_PROFILE.md](CORE_MAP_PROFILE.md).
Source-bound battles now select the recoil destination floor from the original
facing-edge/headroom rules. This is a focused geometry/action milestone, not
complete source movement, actor lifecycle or the complete battle core.
Validation now follows [CORE_TESTING.md](CORE_TESTING.md): affected dependency
only; no full profile census, decoder sweep or unrelated UI suite rerun.

## Original evidence

`tools/knockback_oracle.py` executes unchanged instructions from the local US
BATTLE overlay in the existing Unicorn2.1.4 harness, with explicit map/unit RAM:

| Function | Address | Bytes | Original SHA256 prefix |
|---|---|---:|---|
|battle_move_check_knockback_destination|0x80184610|712|fb9351b2bfdb7550|
|battle_map_calculate_location|0x801810A0|52|3200325a78320b2b|
|battle_target_get_unit_id_if_tile_targetable|0x8017DC88|32|42a1cc17fbffa4a8|
|battle_unit_find_at_tile|0x8017DCA8|552|60e500d728c8441b|

The68 named cases cover17 semantic situations in four directions: lower drop,
upper preference, equal floors, both floors too high/blocked, exact/insufficient
headroom, reversed lower/upper ceilings, slopes, water, occupied or untargetable
preferred floor and a negative underside. The original occupant lookup runs;
unit records are reset explicitly, with requested occupants present even at0HP.
Full function hashes and per-case results are in `x/core-knockback/oracle/`.
This is original-instruction fixture evidence, not a complete PS1 battle trace.
The decompilation/source disc remain read-only and generated assets stay local.

Observed rules: compare the source exit edge and destination entry edges,
choose the highest eligible floor, prefer lower on equal heights, and require
four half-height units under the other floor when only one landing is low
enough. The ceiling-depth subtraction here is NOT doubled, unlike ordinary
movement ceilings. Once the preferred floor is selected, an occupied or
untargetable tile rejects recoil without retrying the other floor. Water is
not rejected by this geometry/targetability query.

## Implementation and limits

`../knockback_geometry.h` holds the portable selector. Edge heights are16-bit
and unwrapped; the ordinary byte-ceiling helper is intentionally not reused.
Signed headroom comparison preserves negative undersides on the16-bit TI C
model. Selection returns1 lower,2 upper,0 rejected, independently of compact
node IDs (node255 is still a valid payload,256 remains a temporary no-node).

`knockback` retains the dominant-axis direction and nonlethal-hit guards. For
source-bound chapters it selects the source layer, reads the destination
source column, then resolves the chosen compact node. The existing targetable
Tile and actor occupancy query reject unavailable landings, with no fallback.
Fall damage now uses that facing-edge drop, then the existing Jump/max-HP
formula and999 cap. Actual hit application still calls `unit_place`, changing
the canonical cell and feet-cache key before the next draw.

The per-map fast path is retained. When the existing profile proves no upper
movement geometry, lower exits/body6<=127 and no compact upper nodes,
`knock_lower` checks only the lower source/destination edges. The blocked upper
sentinel underside is then safely above recoil's four-half-unit headroom.
Other maps use the full selector. No map-ID exception or new metadata field.
Unbound synthetic/legacy controls retain their earlier dry same-floor rule.

Known boundaries: deployed/KO occupancy uses the current canonical unit query;
complete crystal/chest/temporary-unit lifecycle and mount/Stepping Stone rules
remain open. No changed renderer, raster bridge cover, projection or sprite
bank is claimed. The fixture checks that the stored old feet key no longer
matches after a displaced unit moves; it does not require clearing it to zero.
Full source endpoint movement, longer jumps and airborne/performing/casting/
reaction contexts remain separate milestones.

## Targeted verification and package

- 172 geometry checks, hash `8202B6EE`, match PC, full ASan/UBSan and actual68000:
68 original queries with selected floor/drop assertions and fast/full equality
where the cached single-floor proof applies.
- 82 actual battle checks, hash `FC9F5C3A`, match PC/sanitizers/68000. Fifteen
scenarios use3/6 compact cells and the13-actor descriptor: both source floors,
chosen-floor occupancy/targetability without fallback, low ceilings, water,
fall HP, off-map/lethal/same-coordinate guards, source-edge versus stand height,
single-floor flags, ordinary KO obstruction and canonical feet-key invalidation.
The battle harness excludes legacy signed shifts; the pure kernel does not.
- 2574 focused profile controls pass after converting the runner to an opt-in
`--deep` policy. The unchanged3,234,825 certificate is retained, not rerun.
- All28 non-code packaged variables have the preceding package's exact hashes.
No decoder, font, map-art or unrelated management sweep was needed. No new
images were generated and no live emulator input was sent. This is not a new
full169-image/campaign-replay certificate; prior reports retain their own scope.

Private `x/core-knockback/fftcampaign.89g`:470229 bytes,42 variables, +395 against
469834. SHA256 `dd8b03eb71e0c088dbc690efea96cfa63c72abbfbf3e43b8442ce6d9f8e3db37`.
The packed battle payload is41816 bytes, module payload33726. The existing
`ffca` planning alert remains; no new data bank is shipped. ABI5/eight modules,
FGS1/SGE1/FCD3, FSV3 and State/Work fields are unchanged. Modules are rebuilt
because the shared C translation unit changed. The native fixture exercises
the rebuilt bank and actual hit/reward callbacks.

The unchanged layout's previous native figures (Work45514, Unit80, State1586,
Save3138, five Gariland blocks116094) are referenced from CORE_MAP_PROFILE,
not presented as freshly measured heap high-water. Code/stack residency can
change. No combined maximum-RAM claim. The standard playtest group and the
user-owned live TiEmu are untouched; map production remains closed.

Reproduce only this mechanic:

```sh
make -C games/fft/campaign core-knockback-check
```

`results.json`, `battle/results.json`, `package.json`, `unchanged-assets.json`,
`verification.json` and logs live under `x/core-knockback/`. Original oracle
uses the existing host-only pinned Unicorn dependency. The next geometry
milestones are endpoint slope/water policies, longer horizontal spans and
cross-floor actor obstruction, followed by source raster cover checks.
