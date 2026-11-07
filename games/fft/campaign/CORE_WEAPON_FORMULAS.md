# Physical weapon formulas and strike preparation

> Superseded by [CORE_WEAPONS.md](CORE_WEAPONS.md): the complete native weapon-effect delivery now includes the effects/proc draw listed as remaining below. This document preserves the earlier physical-formula checkpoint.

Ordinary physical weapon families now select XA from the source item type,
using the existing archived equipment metadata. The compact resident lookup
maps PA, (PA+Speed)/2, PA*Brave/100 (minimum1), MA, variable PA and (PA+MA)/2.
Axes, flails and bags draw their base before the critical check; pure previews
use the retail random helper's midpoint16384 without advancing native RNG.
Ordinary guns71..73 start at WP*WP, retain physical support/status/zodiac and
critical stages, and bypass the four ordinary evade stages after pre-formula
guards. Charge uses the selected weapon formula and adds its source power.
Jump retains its separate PA/polearm rule.

Twenty-six additional source-compatible physical weapons now work with Two
Swords: knight swords, katanas, rods, staves and flails without special elemental,
status or spell behavior. Each hand selects its own base and random draw;
existing wake-up, retargeting, KO and reward settlement stay with the shared
strike executor. The FCJH layout and49306-byte length stay fixed: only27 low
HAND_DUAL bits change, comprising those26 weapons and the unarmed source marker.
The existing bare-hand Two Swords behavior was already enabled.

## Ownership and capacity

ABI71 replaces the old ModCtx attack_roll callback with a pure weapon_amount
callback in the same four-byte slot. The shared weapon_roll.inc executes in the
current RESPONSE/COMBAT owner, drawing hit/base/critical values and calling the
resident arithmetic. It loads no image while another image executes. Ordinary
Attack, Charge, Counter, Hamedo and borrowed Mime weapons share this preparation.
PC uses the same include. Test-only direct preparation remains available under
RT_CYCLES; the production resident omits it.

The facing selector now chooses the component on the target's facing axis after
comparing absolute deltas.1024 relative-position/facing cases retain the prior
classification, including diagonal ties and the coincident-tile back case.
Critical amplitude uses the existing verified MULS16 helper. A33-byte static
family map replaces repeated type tests. No renderer, art, portrait, palette,
projectile presentation, state/save field or allocation block is added.

Actual non-BENCH packed input is64929/64936 (7 bytes left), versus64897 before
this checkpoint. The private packed data file is39847 bytes. The raw variable
is64927 bytes. Recover resident headroom before expanding weapon effects.
ABI71/ModCtx352 keeps22 images, the8566-byte execution zone and7/7/8 shards;
Unit112, State2224, Work45608, Save3202/FSV4 and five heap blocks remain.
Stack and full tactical-frame certification are still separate.

## Original evidence and checks

weapon_formula_oracle.py executes hash-verified retail base-XA, Charge,
physical support/status/zodiac, critical and ordinary gun routines. The retail
random helper executes unchanged; only libc random values and recoil commit
are isolated.288 bounded scalar profiles cover every physical family, minima,
high legal stats, variable/critical boundaries, supports, Berserk, target
statuses and pure previews. Source hand initialization and used-weapon selection
also execute for the26 newly supported compatible weapons.

make core-weapon-formulas-check compares PC, ASan/UBSan and actual68000 through
source scalars and real commands: Charge, misses before a variable draw, an
independent variable critical, Counter/Hamedo, left-only/Frog and the two-hand
executor. It currently supplies2421 assertions (FEEA07E7) with equal layouts.
Original/raw evidence, scoped closure, campaign replay and preservation details
are recorded in x/core-weapon-formulas/verification.json.

The measured query zone includes assertion overhead and averages3178 cycles;
the ordinary/Charge constructor zone averages125991 across16 invocations and
includes cold image loads. These are bounded zones, not complete tactical frames.

## Explicit remaining weapon work

This checkpoint covers base physical arithmetic and its existing native strike
pipeline. Weapon elements, absorption, healing/draining weapons, magical guns,
additional status/spell procs and their exact dispatcher/RNG ownership remain.
In particular, the retail conditional-status draw after the physical formula
is outside this bounded RNG prefix; this is not complete original action-RNG
parity. Special elemental/proc weapons remain outside the expanded dual-hand
subset. Existing straight obstruction/presentation and the limits on exact arc
trajectories, lunge collision and complete overlapping-panel victim lists remain.
Broader innate/future-CT/Reflect/AI fidelity and the combined frame/stack/residency/
release checkpoint are still open. Generic coverage remains201 actions and78
R/S/M consumers; this does not create another missing-generic-family queue.

Final closure: all33 affected suites pass, without weakening an existing
assertion. Current parser140/bindings203 checks pass, and an actual three-shard
ABI70 export rejects before module execution. Gariland/Orbonne retain81/14
checks(AE9CFD31/2CD70391), with identical native pixels and exact FSV4 VAT bytes,
repeat-payment protection and saturated trophies. All555 originally protected
save/group files retain their bytes and access/modification dates. Standard
groups and live TiEmu remain untouched. The actual raw and packed-input headers
both confirm64929 bytes.
