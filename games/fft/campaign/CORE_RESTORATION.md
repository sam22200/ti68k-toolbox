# Resource restoration: seven source item producers together

2026-10-30. Follow-up to CORE_CLEANSE. Hi-Potion, X-Potion, Ether, Hi-Ether and
Elixir are enabled as one family. Existing Potion and Phoenix Down use the same
source-derived scalar preparation/commit. All 14 ordinary Item records are now
enabled; this does not certify all Chemist R/S/M or every legacy item adapter.

## Source rules and representation

- Potion/Hi-Potion/X-Potion restore nominal 30/70/150 HP; Ether/Hi-Ether restore
  20/50 MP. Elixir prepares maximum HP and maximum MP separately. Positive
  quantities cap at 999 before recipient saturation, including synthetic maxima
  above 999. Faith, magic defense, evasion and Silence do not modify item effects.
- Living Undead receive HP damage from Potion-family items and Elixir. Phoenix
  Down kills living Undead using current HP; dead Undead remain invalid targets.
  A normal KO receives 1..20 HP, with one source draw. Ordinary restorative items
  consume no formula RNG. Native HP-based KO is adapted to the source Dead bit;
  innate Dead remains protected. Petrify/Wall/absent actors remain distinct.
- Elixir is a two-channel result even against Undead: the original reversal sets
  attack_type to HP_DAMAGE, but leaves mp_healing populated. The original commit
  reads numeric HP/MP fields independently of those flags. A bounded original
  store trace confirms HP100/max300, MP20/max100 becomes HP0/MP100. Do not discard
  a numerical resource channel solely because its source attack_type bit is absent.
- Prepare both resource quantities before wind-up. The existing scalar self-cost
  storage is now an explicit tagged union: effect_mp for paired target-resource
  kinds, self for other scalar caster costs. ActionPacket preserves the same
  bytes through a response. The paired kind returns before the Wish cost path.
  Unit, State, Work, CastVictim, CastBatch and FSV3 do not grow; no new heap block.
- Only changed resources earn an item reward. Full HP/MP with no change consumes
  the issued item but grants no EXP/JP. Normal MP/HP healing does not wake Sleep
  or offer damage reactions; inverted HP damage uses the shared KO/wake/response
  hooks. Auto-Potion now prepares all three enabled Potion tiers through this
  family and keeps its existing source stock-selection order.
- Forecasts use actual recoverable HP/MP, including both channels for Elixir.
  Action start invalidates the scalar cache, even when target/ability IDs stay
  equal. Existing text/font/cache/style is reused; paired impact labels are +HP/MP or
  -HP+MP. No image bank, font bank or original spell cinematic is added.
- Adapted immediate AI values beneficial HP restoration, enemy Undead damage,
  and MP restoration only for a unit with a learned enabled cast. MP utility is
  below immediate HP/KO utility. It is not the original future-turn or scarce-
  inventory conservation simulation. Existing reach/Throw Item policy is reused.

## Native budget and unchanged input policy

SCREENS now owns existing turn-menu/Act/Auto-battle input, view cursor/facing and
AI cursor-glide policy. It returns an action request before the resident runs
loading Move/Aim/Defend code. It never evicts its executing image. The 1.5-second
AI pause clock remains resident. The host's old direct policy and an immutable
reference test all 20 menu/control cases plus 16 view/facing cases; full State and
relevant Work caches agree on PC and 68000. No keyboard matrix or IRQ code changes.

ABI 43/ModCtx 376/signature 1181 uses 17 images, split 8+9. The final report records
code-bank sizes, the static resident-zone size and packed headroom. The image
zone belongs to resident executable storage, not a sixth core allocation.

Final native measurements (private `x/core-restoration/verification.json`):

| Measure | Result |
|---|---|
| Private campaign group | 537806 bytes, +5982 from cleansing |
| Raw code banks | 47710 / 53218 bytes |
| Static resident image zone | 8172 bytes, +966; not a heap block |
| Packed resident input / headroom | 64829 / 107 bytes |
| Archived resident wrapper | 40314 bytes |
| Native Unit / State / Work | 112 / 2192 / 45576 bytes, unchanged |
| Save FSV3 / core heap requests | 3138 bytes / five, unchanged |
| Other data banks | 27 byte-identical |

## Scoped evidence

`make -C games/fft/campaign core-restoration-check` uses 80 original formula /
finalizer profiles and 38 original numerical HP/MP store traces. Source handlers
48..4b are checked against the original dispatcher; randomness is controlled.
Store execution isolates reaction dispatch and MP Switch/Distribute/Damage Split
and stops before unrelated status/reward handlers. Source EXP eligibility is
also read from the original outcome calculation. This is not a complete original
inventory, reward, graphics, AI or Auto-Potion selection replay.

The 2481 family assertions agree on PC, sanitizers and actual 68000:
1683/AB2D4E2B resources, 437/F9DBABDF boundaries, 361/D10B5DD9 controls.
Check seven real catalogue producers, frozen paired quantities (including a
changed maximum during wind-up), one item debit, no-effect rewards, mixed MP/HP
outcomes, source draw counts, typed response suspension, stock upgrades,
Silence, actual AI, legal/absent/KO targets and exact relocated controls.
The old reaction fixture supplies a prepared source result and now sets its
explicit HP category. Do not add a shipping fallback for an untyped test packet.

Affected actions/reactions, cleansing, owned area work, pause, loader/bindings,
20 unchanged layouts, 12 old screens and both native pilot save/results flows
are the closure. The final small cache invalidation retains the same image
zone/ABI; unchanged native carriers replay against final code banks.
Reuse unchanged geometry/bitmap/Jump evidence; preserve live TiEmu, user saves,
standard playtest and 18682 old proof/group paths, including bytes AND
modification dates. The prior standard group remains 466045 bytes.

## Remaining core gate

There are 93 executable normalized records, 62 casts and 90/201 generic action
entries. 111 unavailable entries include upgrades/categories, not 111 effects:
10 remaining status/control/barrier, 69 other default-formula job entries and 32
special-dispatch/upgrades/categories. Remaining producer families, R/S/M,
special actors/monsters, campaign/objectives, formats/tools and full worst-case
frame/stack/residency gates remain open. Keep further work grouped by mechanic
and Jump deferred. The full core is not complete.
