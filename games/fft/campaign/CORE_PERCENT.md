# Percentage HP and resource transfer: five retail producers together

2026-10-29. Follow-up to CORE_BUFFS. Demi, Demi 2, Lich, Spell Absorb and
Life Drain share the existing cast/range/CT/MP/area/impact/response pipeline.
The offline classifier enables all compatible records together; no named-spell
runtime dispatcher or new gameplay/save field is introduced.

## Source behavior

- Formula09 uses Faith-scaled magic accuracy. MA Up, MD Up, Shell, zodiac and
  elemental strengthening affect accuracy; they do not scale the percentage
  quantity. Demi takes ceil(25% maximum HP); Demi 2 and Lich take ceil(50%).
  Elemental affinity and the common weather/element stage still apply to Lich.
  Undead alone does not reverse these attacks. Demi/Demi 2 are reflectable;
  Lich is not. Formula09 retains the successful formula08-style proc draw.
- Formula0f Spell Absorb takes ceil(33% maximum MP); formula10 Life Drain takes
  ceil(25% maximum HP). Faith/support/status/zodiac modify success probability,
  not the percentage amount. Both retail drains are single-panel, exclude the
  caster's panel and cannot be reflected. These are validated compiler
  invariants, rather than per-map or per-unit exceptions.
- A successful drain writes a separate caster result. It copies the prepared
  nominal amount, including when the victim has less current HP/MP than that
  amount. Life Drain against Undead heals the victim and damages the caster.
  A miss or empty panel creates no caster return. Quantities are source-capped
  at 999; actual HP/MP then clamp independently to each recipient's bounds.
- Resource identity remains explicit: new MP damage/heal event kinds do not
  change HP, wake Sleep or offer Auto-Potion. HP damage keeps the existing
  damage/KO/status hooks. The caster return does not earn a second reward;
  draining an already empty MP pool does not earn a false target reward.
- The caster's existing unused victim slot stores the paired result. The
  geometric mask returned by cast_targets stays unchanged; the prepared and
  forecast effect masks include the caster return. EFFECT_RETURN uses a free
  bit in the existing effect byte. No unit, packet storage or Work buffer is
  added.
  A single-panel source permits one live victim; the defensive ordered scan
  preserves the source's last-successful-target overwrite for overlapping
  synthetic occupants, rather than summing caster returns.
- Target and caster forecasts, numeric glyphs and MP suffixes reuse the approved
  generic area renderer/font/layout. Unit/Panel binding still refers to the
  selected centre, independently of the extra caster result row. MP and HP
  transfers remain frozen until the once-only batch commit. No new artwork,
  font bank, summon cinematic or screen design is introduced.
- Area AI combines victim utility with the caster return and rejects harmful
  Undead reversals. MP utility is scaled below immediate HP/KO utility; MP
  denial counts only against units that know an executable cast. It remains adapted immediate scoring,
  not the original future-turn resource/response simulation.

The original oracle executes 57 bounded profiles through unchanged MIPS
formula09/0f/10 instructions and finalizers, including both result packets.
It supplies native-equivalent RNG getters/percentage rolls and isolates target
sorting. It is not a full original animation, live commit or AI replay.

## Shared math and bootstrap budget

MA/support/zodiac and weather/element adjustments are shared pure C helpers,
using the existing verified MULS16/DIVS primitives. The status/healing query
also handles percent quantities and probabilities. It does not load an image.
Existing source elemental/healing/status fixtures certify the shared paths.

The unchanged catalogue validator moves from resident initialization to SAVE5.
The static executable zone already exists before heap-backed Work. Native boot
first validates both code banks, preloads SAVE, supplies only the catalogue
pointer/size, then executes the pure validator with Work still null. It clears
module residency before allocating/binding live Work/map pointers. The schema,
validation algorithm, allocation count and archived assets are unchanged.

Important memory accounting: mod_zone is a static executable array inside the
resident program, not a sixth heap allocation. Image-zone growth increases both
resident packed input and active program RAM. ABI 41/ModCtx 376/signature 667
rejects the actual prior ABI 40 export. No FSV3/Unit/State/Work layout changes.

## Scoped verification

`make -C games/fft/campaign core-percent-check` builds bounded native test
carriers for original paired packets, runtime impacts/AI/presentation/bootstrap,
and reflection/empty/missed/capped resource boundaries. Native equality and
sanitizers run on production catalogue records, not normalized test substitutes.
Private evidence is under `x/core-percent/`; final sizes and test counts are
recorded in verification.json and package.json.

The direct closure includes prior healing/status/Reflect families, the changed
scalar/element/weather helpers, actions/reactions/owned area impacts, actual
module binding/old-bank rejection, 20 layout fields,12 old screen hashes and
both native pilot save/results flows. Original input fixtures are reused; no
geometry/Jump/map census or asset regeneration is needed. The weather AI fixture
now learns only Fire while comparing Fire's ranks across weather, so unrelated
stronger entries cannot evict it from the bounded eight-candidate ranking.
The standard playtest, user saves and live TiEmu remain untouched; old image/
group bytes and modification dates are preserved.

## Measured private package

`x/core-percent/fftcampaign.89g` is 527409 bytes, +1392 from CORE_BUFFS.
Only ffbcore (-110), ffbm (+490), ffbn (+1012) and ffcj (same length) change.
The 27 other data/asset banks stay identical. FCJA is still 41086 bytes;
exactly normalized records 42, 43, 47, 48 and 73 change, with every other byte
identical. Opening and campaign catalogues agree.

Raw code shards are 43760/46002 bytes. The static image zone is 7120 bytes,
+100 inside the resident program. Packed resident input is 64845 bytes with
91 below the 64936 limit. Native Unit 112, State 2192, Work 45576 and FSV3 save
3138 remain unchanged, as do all 20 host/native layout fields and the five core
heap allocations. The shared return/reward checks slightly raise the existing
13-status-victim backend impact from 296906 to 298764 cycles; preparation is
292348. These timings are not a full frame/stack/RAM certificate.

The family has 2396 PC/sanitizer/68000 assertions: 1710/82DE1E51 original
profiles, 476/90D1C72C runtime checks and 210/A45DC0D6 edge checks. Prior
healing 2110/11072857, Reflect 2736/77DAC288 and buffs 5682 retain their values
when replayed with final code banks. Elements 1525/92063E01 and weather
6493/8685AD66 also agree. Final closure includes actions/reactions/area,
eight rejected module starts (including the actual ABI40 export), 20 layouts,
12 old frame hashes and both native pilot save/results flows. Unchanged bitmap
and geometry/Jump evidence is reused rather than regenerated. All 18658 old
image/group paths retain both bytes and modification dates; the standard
466045-byte playtest group, user saves and live TiEmu remain untouched.

## Remaining production gate

The catalogue now has 82 executable normalized records, including 60 casts
(45 spells and 15 summons), and 79/201 generic action-table entries. The 122
remaining entries include categories and upgrades, not 122 distinct effects.
The percentage/resource-transfer group is closed for these retail producers.
Remaining groups: 12 status/control/barrier entries, 69 other default-formula
job entries and 41 special-dispatch/items/upgrades/categories. Other R/S/M,
special actors/monsters, campaign/objectives, format/tool and full worst-case
frame/stack/residency gates remain open. Keep further effects in compatible
family batches and Jump deferred. The core is not complete.
