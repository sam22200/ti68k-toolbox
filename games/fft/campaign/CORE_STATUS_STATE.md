# Canonical status state and protected cures

2026-10-26. L2 follow-up to CORE_STATUS_RULES.md. The core now stores the
original forty status bits, separates inflicted/innate/immune state and shares
the source bitmap filtering/cancellation policy. Existing cures, previews and
AI use the same removable-status rule. This is not forty playable effects:
durations, upkeep, control/team changes and status-specific handlers remain open.

## Runtime contract

`status_policy.h` stores three five-byte, MSB-first source bitmaps per unit.
`Unit.status` remains the existing four-bit hot cache (Defend, Poison, Darkness,
Silence); canonical bitmaps are authoritative. No forty-status scan is added to
each frame. Current cures use a small shared removable mask, excluding innate
effects even when the corresponding inflicted bit is also set. Item consumption
on an unsuccessful cure is preserved; no removed effect means no fake cure EXP.
Formula previews, committed effects and AI usefulness/scoring agree.

`battle_status_apply(target, delta, removal_only)` is a resident-only entry to
the FORMULA image. It checks actor ID, living/deployed state and supported effect
bits before modifying scratch or loading an image. It returns the source sum of
accepted mask bytes, or `STATUS_REJECTED`; that sum is not a status count. A
removal-only runtime request cannot include additions. Successful changes clear
formula/Status/frozen caches and restart an in-progress AI candidate pass.

The runtime permits adding Defend, Darkness and Silence, and removing those
effects or existing Poison. New Poison and other unsupported additions reject
until their gameplay/upkeep hooks exist. The pure bitmap kernel represents all
forty IDs for source-backed policy tests. Equipment/job producers do not yet
populate innate/immunity fields in ordinary play; fixture-injected traits prove
the policy, not complete equipment support. KO/revival remain in their existing
HP lifecycle; this API excludes KO actors and does not synchronize HP with the
canonical Dead bit.

Charging/Jump/Defend/Perform share the original command-state nibble, with
mutable command bits replaced together. Existing Archer Charge sets Charging;
resolution/interruption clears it. Attack/new actions and Defend cancel pending
Charge consistently without deleting unrelated ailments. Start-turn Defend
removal is the explicit source command-state upkeep rule, including its innate
bit; ordinary cures cannot remove innate statuses. This adds neither Lancer
Jump nor Perform gameplay. Jump research remains deferred.

## Source policy and evidence boundaries

`status_policy.inc` implements bitmap filter/remove/store in source ID order:
immunity, current versus innate protections, incompatibility/cancellation,
Frog toggle, rider whitelist and immortal formation restrictions. Source rows
remain archived; no per-unit copy of policy tables is made.

`tools/status_state_oracle.py` executes the original MIPS filter at0x80184B24,
remove at0x8018E248 and store at0x8018E310. Main initialization at0x80059854
checks the derived formation mask. Function/disc/table hashes and the decomp
commit are recorded in `x/core-status-state/oracle/source.json`.177 cases cover
all forty IDs as new, repeated, immune and innate-protected cures, plus mixed
requests, upper bits, cancellation order and rider/immortal contexts.

The external status-specific handler is hooked to return without side effects;
no instruction in the tested bitmap routines is changed. Original CT setters
execute, but timers, team/control, HP, equipment and graphics are not native
certification targets. The bitmap oracle cannot certify those missing handlers.

`check_status_state.py` passes4842 PC/ASan/UBSan/actual68000 assertions, hash
13140475. Besides the pure kernel, it exercises actual resident-to-FORMULA
application, immunity, protected cures, previews/items/rewards, AI candidates,
Charge/Defend consistency, cache invalidation and rejected-request side effects.
Existing433 Darkness/Silence checks retain A5131B09;295 action checks retain
EDCEBFAE; Archer Charge retains276A2090. The catalogue source/native reader
hash is B9686645, including FCJ6 rejection and missing-policy rejection.

The13-actor recoil boundary retains82 assertions/hash FC9F5C3A;68 module-loader
checks pass on PC/native. The actual new bank accepts the measured native
signature/context layout and the captured ABI9 bank rejects. Six ordinary
action frames retain their accepted PC/native hashes. Gariland/Orbonne flow
retains81/14 assertions (AE9CFD31/2CD70391), native save bytes, repeated rewards
and trophy overflow behavior. All11 flow PNGs are byte-identical; their dates
are preserved. No exhaustive geometry/decoder/UI sweep or TiEmu was used.

The177-case pure bitmap batch costs5968444 datasheet cycles. Five actual runtime
requests total161640 cycles, peak66150, including image dispatch/copy. These
are scoped calls, not a complete frame or gameplay latency measurement.
The existing primary-impact fixture now peaks40876 cycles versus34500 at the
previous checkpoint, with the larger FORMULA image. This is extra CPU at impact,
not a speedup; ordinary presentation hashes remain unchanged.

## Formats, size and memory

FCJ7 adds650 archived bytes to CJ_STATUS: forty raw16-byte rows, five formation
immunity bytes and five rider bytes. Earlier catalogue versions and incomplete
policy spans reject. Existing item/ability/name/save IDs remain stable; the
two encounter descriptors remain byte-identical. The650 bytes are read in place.

The changed Unit/State layout requires ABI10. All nine FMB2 images carry
signature56956; old ABI9 signatures reject. Native Unit is96 bytes (was80),
State1804 (was1586); Work45516 and FSV3 save3138 are unchanged. The extra218
State bytes comprise13 times16 per actor plus10 request-scratch bytes. State
and deferred save still share one4942-byte block; no sixth allocation is added.

Native layout requests are114514 bytes at Gariland,114154 at Orbonne and126522
for the maximum256-node two-layer case, all +218. These are sizeof/allocation
formulas, excluding stack, fragmentation, allocator overhead and combined
supervisor/module/grayscale residency; they are not measured peak RAM.

Private `x/core-status-state/fftcampaign.89g`:475516 bytes,42 variables, +1877
versus CORE_STATUS_RULES. SHA256:
`75387e9ebdc4befaeee9bf391e887233d21894af616be01e768239908661e08f`.
Module bank37630 (+1246), catalogue40484 (+650), packed battle payload -19;
management/deployment readers change version checks at unchanged packed sizes.
The other27 non-code banks are identical. Largest image5724; FORMULA3128.
Battle TTPack input64673 leaves263 bytes of its64936-byte input limit.
Further resident growth needs budgeting before adding code.
The standard466045-byte playtest group and the user's live TiEmu are untouched.
Full current provenance/results are in `x/core-status-state/verification.json`.

## Next bounded slice

Implement status lifetime and turn upkeep, starting with Poison/regen and
duration expiry, then incapacitation/cast interruption and reaction contexts.
Populate innate/immunity from supported equipment/jobs through shared producers.
Keep unsupported effects unavailable until their downstream behavior has a
fixture; storage or a successful bitmap test alone cannot enable a mechanic.
Full L2, the remaining L1/L3/L4 gates and bulk content production remain open.

```sh
make -C games/fft/campaign core-status-state-check
tools/pyenv/bin/python games/fft/campaign/check_common.py
```
