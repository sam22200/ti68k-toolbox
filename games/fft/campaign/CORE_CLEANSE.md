# Cleansing family: cancellation spells and restorative status items

2026-10-29. Follow-up to CORE_PERCENT. Esuna and Dispel Magic ship together
with Maiden's Kiss, Soft, Holy Water and Remedy. This is one cancellation
mechanism, not six runtime spell dispatchers. Existing Antidote/Eye Drop/Echo
Grass keep their reviewed compact pilot presentation and have original removal
profiles in the same fixture. HP/MP restoration items remain a later family.

## Rules and storage

The compiler accepts source cancellation mode 0x10 with a nonempty set of at
most 16 requested statuses. It preserves each spell's ordinary range, CT, MP,
Reflect and team flags. New item records retain their source status-set index,
item slot and level, with the existing item reach policy and once-only inventory
cost. No cancellation
set is treated as an HP quantity or an addition request.

A prepared u16 packs the removable subset in source bit order. Commit decodes
that frozen subset against the immutable source definition; a status acquired
later in the wind-up is not silently added to the cure. No larger CastVictim,
CastBatch, Unit, State, Work or save is needed. At most 16 source bits fit exactly;
retail Esuna uses 10, Dispel 9 and Remedy 8. All five status bytes participate.

Innate effects are protected. Infliction immunity does not prevent removing a
mutable effect. Source cancellation eligibility is tested independently from
ordinary damage eligibility: Soft/Esuna/Remedy can cure a statue, while other
items and Dispel cannot act on that blocked target. Dead, Wall, Crystal,
Treasure, Jump and absent actors retain their separate source validity rules.
Petrification removal retains HP and position and refreshes the clock/control
cache and turn order. This does not enable retail Petrify, Frog, Berserk,
Confusion or other unimplemented infliction/control/actor handlers.

Removal clears only the corresponding timers, refreshes the shared status
caches and invalidates the existing UI/AI caches. It neither changes HP/MP nor
offers damage reactions. New items use the original formula0x38 handler's lack
of RNG draws; legacy compact item adapters remain unchanged. A successful item
consumes one inventory entry when issued, and repeating impact cannot consume
another. Spell costs/CT and Half MP/Silence use the shared pipeline. Esuna
retains the original single-endpoint Reflect path; Dispel is not reflectable.

Forecasts and effects use the existing status-line and seven-row text renderer.
The new numeric label is Cured; no artwork/font/bank/layout change is required.
Immediate adapted AI values removal of adverse ally states and beneficial enemy
states, using only the actual prepared subset. It is not the original future-
turn resource/control simulation.

## Native budget and scoped evidence

The cold scalar forecast formatting and cache box move from FORMULA to NUMBERS.
NUMBERS compiles the same local pure evaluator, so its formatting never calls a
resident callback that loads another executing image. The resident wrapper
returns between loads; no nested loader or new heap block is introduced.
The formatter, box, fonts and margins retain their previous code and pixels.
The unchanged Wait facing-arrow renderer also moves to NUMBERS7; it runs only
after the caller has returned, never from an executing overlay callback.
Items and spells both commit cancellation inside FORMULA, with the existing
response/reward drain. No HP-action copy of the cancellation implementation is
needed.
ABI 42 rejects actual prior ABI 41 banks; ModCtx is unchanged.

`make -C games/fft/campaign core-cleanse-check` exercises production records,
78 original formula/filter/finalizer profiles for nine producers, packed subset
boundaries, frozen impacts, innate/immunity/timers, statues, inventory, CT/MP,
AI and reflection. Original routines are executed unchanged with controlled
native-equivalent RNG and isolated sorting; this is not a full original live
item inventory/reward/renderer or future-turn AI trace. Raw formula0x38 dispatch
is independently checked against the source handler table.

Final counts, private group sizes, affected regressions, old screen/layout
comparisons and preservation checks are recorded under x/core-cleanse.
Reuse unchanged bitmap-kernel, geometry and Jump evidence. Do not restart the
live TiEmu, overwrite the standard playtest/saves or change old proof bytes or
modification dates.

## Measured private package

`x/core-cleanse/fftcampaign.89g` is 531824 bytes, +4415 from CORE_PERCENT.
Only ffbcore (-67), ffbm (+6256), ffbn (-1774) and ffcj (same length) change.
The 27 other data/asset banks are identical. Catalogue length remains 41086;
exactly records 14, 56, 377, 378, 379, 380 change, with all other bytes preserved.
Opening and campaign catalogues agree.

ABI 42/ModCtx 376/signature 924 uses 17 images, split 9+8, with raw shards
50016/44228 bytes. The 7206-byte static resident zone grows 86 bytes, without
another heap allocation. Packed resident input is 64885 with 51 bytes below the
64936 limit. Native Unit 112/State 2192/Work 45576/FSV3 save 3138 and the five
core heap allocations are unchanged. This is not a full stack/residency certificate.

The 1711 family assertions are 1296/92582E7A rules, 232/6A8077BD edges and
183/4421B5B6 pixel controls, all equal on PC, sanitizers and actual 68000.
The existing buff suite retains 5221/3C724297 plus 461/E318A9A7. The native test
variable grew 16 bytes past its format cap, so the same assertions run in two
bounded carriers: 5032/DB4D36CB and 189/671BE44C. Recombining their rotating
hashes gives exactly the original aggregate digest. No game rules are changed
for this test transport. Existing 13-status-victim backend preparation/commit
cost 297292/299158 cycles; full frame/stack/RAM certification remains open.

Existing healing retains 2110/11072857 and the percent family retains all
2396 assertions/digests. Actions, reactions, owned area impacts, eight invalid
module starts (including the actual prior ABI 41 export), all 20 layout fields,
12 old frame hashes and both native pilot save/results flows pass. The action
aggregate now includes six additional forecast strings; replaying the same PC
and 68000 binaries with the prior catalogue restores exactly 295/0E56F883.
This isolates the aggregate change from the existing action behavior.

The resident program's Wait-arrow body and forecast-box body match their old
sources. The test-only arrow control covers 32 nonempty pixel cases; the six
new forecast boxes also match byte for byte and preserve plane pointers. The
standard 466045-byte playtest, user saves and live TiEmu remain untouched.
Detailed preservation hashes, source hashes and results are in verification.json.

## Remaining core gate

Production has 88 normalized executable records, 62 magical casts and 85/201
generic action-table entries. The 116 unavailable entries include upgrades and
categories, not 116 distinct effects: 10 remaining status/control/barrier,
69 other default-formula job entries and 37 special-dispatch/items/upgrades/
categories. Remaining groups, R/S/M, special actors, campaign/objectives,
formats/tools and full worst-case frame/stack/residency gates stay open.
Continue by compatible families and keep Jump deferred. The core is not done.
