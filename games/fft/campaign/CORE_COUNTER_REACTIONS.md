# Counter Tackle, Counter Flood and Absorb Used MP

2026-11-03. Three equipped reactions now execute through the shared core:
Counter Tackle436, Counter Flood437 and Absorb Used MP438. They remain rejected
as direct commands. FCJD,43674 bytes, and201/201 generic action entries stay
unchanged; wider R/S/M and production certification remain separate.

## Source evidence

`tools/counter_reactions_oracle.py` executes unmodified retail MIPS for60
pre-impact dispatch/finalizer profiles, nine Absorb effects and four reaction
constructor cases. The original constructor calls Dash147 with target
validation1; Counter Flood calls the terrain-selected Elemental ability with
validation0. Its preview, MP-consumption and strike-init calls are isolated and
recorded, not replaced by a claimed full original replay. The finalizer, status
restrictions and controlled BIOS RNG are unchanged. Function/disc hashes, hooks,
rows and target policies are in `x/core-counter-reactions/oracle/source.json`.
The existing primary Dash reference (`oracle.h`/core-actions) and original
Geomancy math/terrain/proc evidence (`CORE_GEOMANCY.md`) are reused.

Counter Tackle uses raw flags4 bit0x10. Counter Flood uses bit0x80, including
ordinary attacks and Geomancy; this is not a broad HP-damage-category test.
A miss, zero damage, healing or a no-op may stage either if that flag is set.
Math and formula7 are excluded. Absorb uses the incoming effective MP cost,
independently of hit/category/physical flags. Zero-cost Math/Mime or rounded
Half MP costs cannot trigger it. Brave and ordinary reaction restrictions apply;
Brave0 consumes a failed chance draw at this pre-impact stage. Late KO, absence,
incapacity or invalid geometry can discard the queued request.

## Native behavior

Counter Tackle reuses Dash's amount helper and its three preparation draws:
one multiplier, then the two Brave-based knockback draws. It has one strike,
ignores weapon reach, receives no weapon critical/dual-hand extension, and keeps
independently armed casts and turn credits. The bounded native reach is one
orthogonal tile with at most two half-height steps; special actor/mount and
vertically stacked same-XZ targeting are outside this slice. It displays the
existing Dash action and strike/hit presentation.

Counter Flood picks the reactor's current lower/upper source terrain and the
existing one of twelve Geomancy abilities. Missing/truncated terrain never
invents Pitfall. The native centre is the original actor's current compact
cell, with no ordinary five-tile centre-range validation (source constructor
validation0); shared radius1/height0 geometry excludes the caster and includes
all other eligible occupants, including allies. Every victim's HP category,
amount and conditional status/Oil event is frozen before committing any HP.
The existing elemental, immunity,19% proc, cancellation and KO rules apply.
The reviewed area painting/labels display the response's own footprint.

The single existing Work cast owner is reused only after the primary has
committed. CoreAction still owns the root and the22-byte suspended scalar
packet; after return, phase DRAIN never resumes the completed root's area FX.
A secondary cast must match its current actor and ability; once applied, it
cannot commit again. All secondary rewards are excluded, and phase APPLIED
prevents nested reactions. Multiple queued actors still drain in slot order;
root rewards and victory/defeat wait for the complete drain.

Absorb freezes the incoming MP byte at staging. Its pending slot carries
0x8000|cost, decoded only by the response owner as source ID438. CoreAction's
normal ID validation runs before that opaque payload is stored. No State/save
field is added. This survives changes to the caster's Half MP support before
preparation. The prepared self effect adds that byte, capped by current maximum
MP at commit; late MP changes remain live. Full MP still allows a response,
Undead does not invert it, and Wall prepares a Miss. The inherited strict root
queue excludes Absorb triggered by the caster itself; original preparation also
skips the acting slot. Faith Up's previously explicit self-queue permission is
unchanged. No MP/stock/turn/reaction EXP/JP cost is introduced.

## Ownership, storage and limits

ABI63 keeps ModCtx360,18 images and9+9 FMB3/FMD2 shards. The logical execution
zone is8452 bytes (62 more); FMD2 still omits exactly358 zero context bytes per
image and reconstructs original code offsets. RESULTS3 now selects responses
and saves the primary packet; RESULTS4 owns follow-up strike target queries.
RESPONSE1 constructs Tackle/resources and RESPONSE7 launches follow-up strikes;
TARGETS11 freezes Counter Flood; FORMULA14 still prepares typed status reactions.
The resident returns between every image call. ORDER16 consolidates the existing
post-clock outcome/round/pick/Defend/Transparent and cache/menu reset operations.
Cursor/manual-control decisions keep their existing behavior.

Banks63962/63918 leave38/82 bytes below64000. Actual TTPack input64867/64936
leaves69 bytes; private packing produces a39969-byte data file. Gameplay/save
layouts and the five allocation formulas remain fixed. There is no new heap,
artwork, data format or ASM. Full original AI/dispatcher, mounts/special actors,
whole-frame/cold rendering, stack/residency and release gates remain open.

## Verification

`make core-counter-reactions-check` checks813 logic assertions (E904D4F4),
34 UI assertions (8BBF301F) and12 pictures on PC, ASan/UBSan and actual68000
archive images. They cover source flag/cost/RNG boundaries, frozen MP, cap/Wall/
Undead/late changes, Tackle amount/RNG/range/cancellation, all twelve terrain
responses, collateral damage, once-only secondary commit and root restoration.
The narrow frozen-packet fixture copies only the range/status/menu sections it
reads; production uses the unchanged complete catalogue.

The aggregate UI carrier exceeded the calculator variable limit. Three transport
partitions retain every assertion and picture; their native digests recombine
exactly to8BBF301F, counting both checks and picture events. Production UI paths
and the reviewed renderer are used. Preparation peaks at144864 cycles and impact
at124380 in these fixtures; sampled rendering includes cold image loads and
is not a whole-frame production certificate. All29 older scoped suites pass.
The area-bounds test's former unconditional secondary-FX exclusion was stale:
its157 assertions now accept the current secondary owner and reject wrong
actor/ability owners and DRAIN. The1012 existing area assertions and64 frame
samples still pass, retaining F9BACFF5 and bounds picture9DE20073.
Bank/old-export, unchanged layout, private-flow and496 protected-file evidence is in
`x/core-counter-reactions/verification.json`.
