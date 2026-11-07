# Throw: selected inventory weapons

2026-11-02. The twelve Ninja learning selectors (382..393) now expose the
available throwable items through the existing skill menu. All80 retail items
marked throwable and belonging to those twelve types have archived command
records. Generic coverage is181/201;20 entries remain: Arithmeticks8 and Jump12
(the latter remains user-deferred). This does not close the full core/release gate.

## Original evidence and model

`tools/throw_oracle.py` runs the unmodified retail formula99, its physical
support/status/zodiac/element helpers and target finalizer. The180 profiles
cover all80 throwable items, Speed and support controls, Protect, Sleep,
Charging, Frog/Chicken, Shell, Undead, absorption/null/half/weak elements and
accessory/shield evasion from front/side/back with controlled RNG. Sorting and
external RNG are isolated; source executable/overlay/function hashes remain in
`x/core-throw/oracle/source.json`. These are bounded function trials, not an
original full-menu/AI/campaign replay. Catch, Weapon Guard, Blade Grasp,
Concentrate and the wider reaction/support/movement producer work remain separate.

Throw starts at current Speed and the selected item's WP. Defense Up, Protect,
physical vulnerabilities and zodiac apply in source order; Attack Up, Martial
Arts, Magic Attack Up and Faith do not supply a new Throw multiplier. Elemental
half/weak/null/absorption apply without weather's spell multipliers or a weapon
proc. There is no critical or knockback draw. Even zero physical evades retain
the four source rolls; a failure stops at its actual evade stage. The original
uses the default Attack record's Counter eligibility; the existing adjacent
Counter/Auto-Potion ownership/settlement paths are retained.

The source type table is `(32,1,3,9,5,2,6,15,16,4,14,33)`. Learning positions
must come from skillset20: Ball is position1, before Knife, rather than the
last position of that type table. The checker isolates each learned bit.
The native five-slot equipment model uses its existing left-hand shield slot;
the original evasion fixtures populate that same hand explicitly.

## Shared implementation

FCJC retains all512 source metadata/learning/name entries. Its normalized battle
prefix now has640 records: the original512 plus128 item slots at512+item ID.
Eighty item records are executable; the other48 reject. The twelve source ids
are learning selectors and reject direct commands without an item. There are187
nonempty source rows (including those selectors) and255 executable commands
(175 earlier commands plus80 item adapters). Owned casts remain129.
This explicit distinction avoids reporting selectors as independent attacks.

Each archived item command carries WP, element, required level, source learned
position and inventory item ID. No new pending/action/AI payload, live cache or
heap block is needed. `known` exposes owned items in source item order; its
reserved index255 returns the count in one pass, so an80-item menu does not
repeatedly rescan the catalogue to count rows. Names and counts reuse existing
item strings, fonts and menu layout. Range is the actor's current Move, without
a self panel or centre-height restriction. Existing terrain/trajectory and
native equipment-model bounds remain in force.

Eligibility and forecasts are pure. A blocked, unlearned or empty-stock request
preserves State/RNG/pending action. A valid command consumes the selected stock
once before its hit/miss, never the consumable-item bag. Permanent/home camp
owns that stock; enemy stock retains the native unlimited/required-level policy.
The existing campaign Save inventory persists it at victory. Silence permits
Throw; Frog/Don't Act use the shared command restrictions. MP/CT costs are zero.

The shared frozen effect category carries absorption/nullification through
impact and secondary responses. Duplicate impacts cannot consume another item
or reapply damage. AI keeps the strongest available item per element, breaking
ties by lower item ID; this bounded native policy fits its existing17-entry
candidate array and does not claim the original inventory/price preference AI.

## Budget and verification

ABI56 keeps ModCtx388 by reusing its previously unused `opening_after` callback
slot for pure resident `can_use`. Executing images call that query without
loading another image. TARGETS owns native Attack range, reach/path/menu range
preparation; PARTY14 owns the cold scenario presentation reset. The resident
returns before each load. PC uses the same extracted bodies directly.
Unit112/State2210/Work45576/Save3138, FSV3 and the five allocation formulas stay
fixed. FCJC grows41822 to43358 archive bytes and rejects the previous shorter
FCJB format. FCD3/FGS2 and artwork/portraits/projection remain unchanged.

The18 images retain the9+9 split, with an8268-byte static execution zone.
Banks are62300/62554 bytes, below64000 each. Image sizes by stable ID0..17:

```
8258 3566 7350 6304 8268 8260 6476 6114 7582
8118 8004 5642 7614 7650 4338 8268 5212 7586
```

Production raw variable64489; actual TTPack input64491, leaving445 below64936
(previous family64773/163). No32-bit multiply/division or float helper occurs
in generated magic_hit/magic_xa/magic_heal/throw_available code. Actual layouts
and all five allocation formulas compare equal to the previous family.

`core-throw-check` passes4309 assertions27CD6D9B plus61 real-menu/Status checks
38B87D4D (4370 total), equal on PC, ASan/UBSan and actual68000.180 original
profiles include18 geared/directional/seed cases. A separate effect-flag case
keeps Oil's marker out of the HP category. Native forecast amount peaks57266
cycles; command preparation118888; marked impact51468. These include wrappers
and are not whole-frame costs.

The59 older scoped suites pass, followed by direct retries after the Throw-only
rounding/flag/Status refinements: targets, magic-evasion, actions, module/boot
boundaries and cell. The earlier target control treated formula0x63 as gated;
it now uses still-gated Jump0x62 and retains1193/FC02B091. Both chapter flows
retain81/AE9CFD31 and14/2CD70391, including private persistence and unchanged
pixels. All64 area-impact pictures retainF9BACFF5; bounds remain9DE20073.
The current catalogue restores structurally to the exact previous41822-byte
bank; the isolated current-engine control retains actions6FF41691/295, versus
F6547B2B/295 with the new selectors.511 protected hashes and modification dates
match (ten regenerated fixture dates restored only after byte equality).
111/120 data files retain their bytes; only two catalogue pairs, two code-bank
pairs and modules.h change. Common reader remainsB9686645; all170 module bindings
and eight rejection cases, including the captured ABI55 export, pass.

Final measurements and assertion digests are recorded in the local
`x/core-throw/verification.json`. The actual packed-input size includes the
raw variable's two-byte length prefix; do not confuse the two values.
Original source/save/group preservation, exact structural catalogue rollback,
source-enabled/control action digests, native module bindings and both private
chapter flows are recorded beside it. No live TiEmu, standard group or user save
is used; nothing is committed or pushed.

The extra80-item UI fixture compares36 first-frame pictures and real selection,
confirmation, cancellation and Status details across PC/68000. It also exposes an expensive
cold overlay transition. Update peaks261552 cycles. The cold render peak is1381186, versus1046576
for a matched existing Throw Stone command (both at transition14). Throw's
larger Move-based footprint adds cost to a transition already over360k.
These separate update/render measurements are retained explicitly, rather
than presented as a360k whole-frame certificate. Full cold-HUD/frame, stack high-water, allocation
fragmentation and combined residency gates remain open.

Validation applies to the active shared chapter core. The previously documented
legacy nonchapter standalone build problem remains separate; it is not reported
as passing here. No whole-original battle/recruitment/Mimic/RSM certification is
implied by generic action coverage.
