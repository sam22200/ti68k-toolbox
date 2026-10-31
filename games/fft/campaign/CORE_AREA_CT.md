# Owned area casts through commands, CT and impact

Current follow-up: [CORE_HEADROOM.md](CORE_HEADROOM.md) recovers634 resident
bytes without gameplay/layout changes. Numbers below are the owned-area checkpoint.

2026-10-27. Follow-up to [CORE_BANKS.md](CORE_BANKS.md). Supported formula08 area
carriers now use cast_order, ordinary CT dispatch, attack_step impact and the
existing response/reward drain. Retail AF_CAST records remain disabled. No area
AI/forecast or reviewed multi-target spell presentation is enabled by this slice.

## Lifecycle

Work owns one60-byte CastBatch, independent of AI, portrait, Status, reach and
render caches. It survives wind-up, queries, image eviction and secondary actions.
Only one CoreAction owns resolution; another order/due action cannot overwrite
its packet. The packet is not saved. The isolated caller-owned cast_batch API
remains available for backend fixtures; a live command uses the Work owner.

Initial COMMAND validation keeps effective MP, restrictions, initial range/height,
self-selection and unit/tile binding. Only the supported ordinary formula08 area
family is accepted: unsupported shape, performance, weapon/item/drain families
or missing raw collision geometry reject before payment or pending/output changes.
No map-ID exception is added. Normal non-area casts/Charge retain their policy.

Arming spends Act, preserves Move and latches CT/explicit binding without MP/RNG
or HP effects. At due time, ordinary KO/Sleep cancellation, Stop freeze, Silence/
Don't Act/MP revalidation and stable due-slot order apply. A unit centre follows
current coordinates beyond its original range; a fixed cell stays fixed and can
have an empty centre with other actors in its footprint. Crystal/Jump removes
that actor from the target set but does not remove a still-valid followed centre.
A completely vanished centre pays once and produces an empty prepared root.
Missing raw geometry is invalid data rather than an empty payable effect.

COMMAND returns internal CAST_AREA_DISPATCH3 after staging only the request.
The resident continuation then calls TARGETS2 to prepare Work.cast, and preloads
RESPONSE during wind-up. No executing image loads another image. TARGETS owns
failure cleanup; this avoids growing the resident continuation unnecessarily.
cast_order still exposes0 rejected/1 immediate/2 armed. command_pending uses the
same handoff; no recursive scheduler, per-cast event queue or new pending fields.

At ACT_HIT, attack_step selects RESPONSE11 for READY Work.cast and ACTION0 for
ordinary/secondary effects. The existing full-context guards consume the area
impact once. All quantities were prepared before HP, including for a caster KO
by its own earlier target result. Later queries/MA/weather do not replace them.
The packet stays APPLIED through supported responses and maximum-reward drain;
it resets to EMPTY only after the root closes, before another due cast starts.
Single-target impact cannot consume the prepared area root accidentally.

Scenario/retry reset must clear both State action ownership and the Work phase.
Gariland's battle_units and the fixed-unit setup used by Orbonne both do so. A
State-only reset previously left READY Work data behind; the new fixtures caught
that omission. Renderer/AI cache invalidation is not sufficient to clear it.

## Source and limits

Reused original evidence: CORE_INCANTATION's four original MIPS initializer/
payment/binding sequences pay before target setup, including empty/KO/absent
centres; CORE_CASTING supplies prepare/cost arithmetic; CORE_DEFERRED supplies
unit/tile binding and due ordering. CORE_TARGETS covers79 actual original footprint
profiles/42 columns, and CORE_BATCH covers13 compound formula preparations with
no intervening HP commits. Their input/source hashes remain unchanged.

The new command/CT/owner fixture tests native composition of these policies. It
is not a new full original spell/campaign/reaction/reward/presentation trace.
The vanished-centre empty-root behavior extends the prior observed payment order;
full original empty-spell presentation/refund behavior is not newly certified.
Current scope stays ordinary unmounted/uncharmed human formula08, supported typed
outcomes/Oil and current Counter/Auto-Potion/reward subset. Reflect, other status/
reaction/trait families and new producers remain gated. Jump is not activated.

## Validation and budget

`make core-area-ct-check`:353 PC/ASan/UBSan/actual68000 checks, hash11D6FCB4, plus
5 separate Orbonne retry checks00000230. Cases cover immediate Non-Charge/exact MP,
real CT/menu pause, moved unit versus fixed empty centre, Crystal/Jump/vanished
centres, status/resource interruption, query/cache purity, automatic impact/replay,
reaction-before-next-cast and thirteen equal-tick roots with one reused packet.

Orbonne restart runs in a separate PC process. The software rt_file cache holds
sixteen files; loading both complete chapter banks in one process exhausts it.
The existing campaign supervisor uses one process per chapter. The runtime cache
and calculator file path are not changed to accommodate this fixture.

Measured native zones: immediate command+prepare/preload127872 cycles; actual
impact41372; two due launches average85304/max85376. These are backend zones,
not complete-frame/animation/grayscale or cold-HUD certificates. Production
presentation is unchanged and no private area picture is a review candidate.

ABI29/ModCtx364/signature63107; thirteen FMB3 images, one6204-byte zone. New ops:
TARGETS2 owned preparation, RESPONSE11 owned impact. Existing isolated ops1/10
remain. Both archive files are still required. All28 non-code banks and FCJA
ability enablement remain identical. No assembly is authored.

Private package504139 bytes, +533 from CORE_BANKS: ffbcore+125, ffbm+10, ffbn+398.
Raw shards33782/31632; image sizes6150/3544/4254/5786/2842/6132/4976/5588/6092/
6204/5566/5044/3052. Resident TTPack input64927 leaves9 bytes; the next resident
addition needs a cold-code budget before integration.

Work grows60 native bytes: native45576; host46168 grows56, reusing4 bytes of prior padding. Native State2192/Unit112/ActionPacket22/
SaveUnit176/FSV3 Save3138 remain unchanged; host State2248. The packet sits before
the large UI caches so ordinary fields retain their offsets; native offset/near-
displacement and packet size are checked independently: native offset23244,
60 bytes ending at23304. Five allocations remain:
the scene+Work block and total requested battle bytes grow60, first State+save
block stays5330. Native requested totals are114962 for Gariland,114602 for
Orbonne and126970 at the current256-node maximum. Host totals grow56 because
the new packet consumes prior padding. Layout/allocation formulas are not peak stack/fragmentation/
combined-residency measurements. C07/L4 remains open.

Targeted closure: isolated batch, prior single-centre incantation, due-wave,
ordinary actions, Charge, pilot save-flow, twelve old action/reaction frame hashes
and fresh layouts. Geometry/census/scalar sweeps reuse unchanged evidence. Old
banks reject the new signature. Exact results: x/core-area-ct/verification.json.
Live TiEmu, standard playtest group and old gallery bytes/dates remain untouched.

Next: area forecasting/AI semantics and reviewed per-target/empty spell
presentation before retail activation; broader core families, campaign/formats/
editors and cold-HUD/full-frame/residency gates remain open. Jump stays deferred.
