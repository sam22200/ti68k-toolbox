# Direct projectile interception

2026-11-06. This closes intermediate direct-weapon interception within the
remaining-interactions delivery. Generic coverage remains201 actions and78
R/S/M consumers. Broader interactions and TI stabilization remain open.

## Behavior

The direct weapon trace now returns an actual unit slot, including slot0, or
255 for terrain obstruction. Travel samples test terrain before actor boxes;
the first encountered body receives the shot regardless of team. A nearer
body wins over a farther unit with a smaller slot. A later wall cannot erase
an earlier interception. Occupants of the intended endpoint remain members of
that endpoint panel rather than obstacles to one another.

The existing panel owner freezes every targetable occupant of the intercepted
cell before impact, in stable slot order. The original selected cell remains
the action's fixed panel. No extra command, resource payment or reward root is
created. Ordinary Attack, due Charge and Counter/Hamedo use this shared path;
Mime retains its existing preparation owner. Preview/card uses the same
recipient, including xN for several occupants and Miss0% for terrain.

Normal human bodies are36 source units tall; Frog/Chicken bodies are24 and
native KO bodies12. Direct launch/aim height for Frog/Chicken is16 rather
than24; a KO target retains the existing12-unit aim. Bounds remain strict.
KO/Stone may obstruct or belong to a panel, with ordinary impact guards;
absent, crystal/chest/gone and airborne actors do not obstruct. Native mounted
content and complete original height producers are not added here.

The generic nonweapon ai_clear trace remains a boolean query with its prior
policy. Direct weapon AI origin masks require direct_target(actor,target) to
equal the intended target. An intercepted shot is therefore rejected for that
candidate, including interception by an ally. This is conservative geometry,
not a new collateral-utility evaluator. The legacy battle_line_clear preflight
accepts any resolved recipient, matching its existing arc/lunge contract.

## Ownership and capacity

Only PLANNER24 image bytes change,6592 ->6744 (+152). ABI75/ModCtx348, the
25-image arrangement and8596-byte execution zone remain. PLANNER1 geometry
still returns before deferred FORMULA14 preparation; PLANNER5 continues
frozen victims/hands. WEAPON6 remains scalar-cache preparation. There is no
nested image load or new transient/persistent field.

Unit112, State2224, Work46770, Save3202/FSV4 and five heap blocks remain.
Shards are58230/61718/57068 bytes. Actual non-BENCH resident bytes are identical
to the prior checkpoint: raw64895, TTPack input64897/64936,39 bytes left;
packed data file39776. No bank format, item catalogue, save or portrait change.

The trace adds26 bytes of local actor IDs/heights. Generated direct_target
reserves136 local bytes plus saved registers; this is not a complete stack
bound. Its three inherited32-bit divisions remain in setup, outside the
sampling loop. No32-bit multiply/divide or floating helper is added to that
loop, and no assembly kernel is authored.

A private thirteen-slot benchmark places eleven Frog bodies along a four-panel
shot, exactly below its strict top boundary. The cold panel_preview call costs
260592 datasheet cycles, including geometry, scalar forecast and owner loads;
fixture setup, assertions and State-purity copies are outside the timed span.
Six additional PC/68000 controls00000423 agree. This is a bounded preview
measurement, not a complete frame or a global worst-case trace certificate.

## Evidence and remaining gates

Fifty unchanged retail direct-target dispatcher executions check clear,
intercepted and terrain results, including slot0 and slot12. Their trace,
geometry and math-library providers are supplied controls. They establish
recipient routing, not bit-exact PS1 trajectory sampling or body-height
producers. Separate live native scenarios consume the exported expected rows.

PC, ASan/UBSan and actual68000 agree on508 logic checks8B7A2337 and24 HUD
pixel/card checks04F50DC5. These cover original routing controls, nearest
body/terrain ordering, strict small/KO body boundaries, absent/airborne
exclusion, frozen cooccupants, deferred Auto-Potion, an interceptor entering
after Charge starts, Counter/Hamedo and pure previews/AI masks. Four HUD
screens use the actual renderer on a neutral background refreshed each frame;
the blocked case verifies an empty right-card region. Dormant cache keys need
not be cleared when no recipient card is drawn.

Three older regression fixtures treated an intervening body as a blocked
direct ray. Their obstruction scenarios now use a temporary solid-column copy
and restore the archived surface pointer after the case. Every existing
assertion remains; allocation checks add three Hamedo assertions and one each
to reactions/strikes. The new suite separately covers body interception.

All13 affected regression suites pass after those fixture updates. The panel
suite retains527 assertions; its digest becomes8864C72B because preflight now
accepts the intercepted body. Its HUD33/3FEC3500 and capacity66/C32255C0 remain.
Gariland81/AE9CFD31 and Orbonne14/2CD70391 keep identical native pixels, rules,
exact settled saves, repeat-payment and trophy-overflow checks.

Affected dependency and private campaign-flow results are recorded in
x/core-direct-interception/verification.json. Full tactical-frame budget,
worst stack, peak residency and hardware remain stabilization gates. Wider
mounted/cooccupant height producers, innate effects and collateral/Reflect
AI utility remain in the interactions delivery. Full PS1 effect scripts,
complete original dispatch/RNG and exhaustive monster/campaign content are
outside this checkpoint. Standard groups, the live TiEmu session and existing
protected save/proof bytes and timestamps are preserved.
