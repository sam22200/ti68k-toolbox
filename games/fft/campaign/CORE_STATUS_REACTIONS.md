# Self-status reactions

2026-11-03. Caution426, Dragon Spirit427 and Regenerator428 are equipped
reactions through the shared core. They remain unavailable as direct commands.

## Source and behavior

`tools/status_reactions_oracle.py` executes unmodified retail MIPS:60 staging
profiles,33 prepared/committed effects and three self-target lists. Function and
disc hashes, isolation hooks and controlled BIOS RNG counts are recorded in
`x/core-status-reactions/oracle/source.json`. Equipment/reward bookkeeping and
graphics handlers are isolated; this is not a full original UI/AI replay.

Caution adds Defending6; Dragon Spirit adds Reraise18. Both use the original
pre-impact physical-eligibility flag, before hit/HP changes: an eligible miss,
healing packet or no-op can queue them. Math and formula7 are excluded. A lethal
hit can stage them, but the dead reactor cannot drain its queued response.
Regenerator uses the post-HP-damage hook: successful zero damage is eligible,
miss/heal/no-op/secondary/KO are excluded, and Sleep may wake before the guard.
All use Brave. Brave0 consumes a failed chance draw for Caution/Dragon Spirit;
Regenerator consumes none after original Chicken derivation. The native guard
implements that boundary without introducing a new mutable Chicken cache.

A prepared request is frozen and committed once through the existing status
owner. Already-current Defend/Reraise and immunity/innate restrictions may reduce
it to a successful source no-op; Wall prepares a miss. Late immunity/status is
checked again at commit. Reraise cannot stack with Undead. Mutable Regen cancels
Poison, refreshes36 ticks, heals at ordinary upkeep and expires normally. Innate
Regen keeps its normal immutable behavior. Defend clears at next turn entry;
Reraise survives KO and is consumed by revival. Charging and Performing remain
intact when these non-canceling statuses apply.

## Integration and limits

The bounded reaction queue, primary-packet suspension, root reward settlement,
status filters/timers and existing Defend/Reraise/Regen/Miss presentation are
reused. No reaction costs MP, stock, turn credits or secondary EXP/JP. No new
art, renderer, ASM, saved fields or gameplay allocation is introduced.
ABI61 retains ModCtx360,18 images,9+9 FMB3/FMD2 shards and the8390-byte execution
zone. The banks use62170/62930 bytes (1830/1070 below64000). The actual
TTPack input is64905/64936, leaving31 bytes; private packing succeeds with a
40039-byte data file. FORMULA13 owns the scalar status commit; no executing image loads another.
FCJD and201/201 generic action coverage are unchanged. Full R/S/M remains open.

The active magical-status producer census has no raw eligibility-bit0x10 rows;
its status-only batch owner continues to skip the physical pre-reaction offer.
This slice does not claim arbitrary future synthetic status producers, complete
original special-actor/mount dispatch, damage splitting or the remaining R/S/M.
Full-frame/cold rendering, stack/residency and release certification stay open.

## Verification

Run `make core-status-reactions-check`. Its logic and18-picture production UI
fixtures compare PC, ASan/UBSan and actual68000 results. Source rows cover timing,
RNG, misses/zero/lethal/secondary hits, Math/formula guards, status restrictions,
late filtering, timer ownership and once-only effects. Ordinary root/response
cases check rewards, CT, direct-command rejection, late cancellation, Regen
upkeep/expiry, Reraise revival and Defend turn-entry clearing.

The family has1276 assertions:1159 logic (7A00ACD0),117 UI (744FECB1).
The actual source-derived response preparation peaks at53430 cycles; status
commit at113692. UI figures include cold loads and do not certify a whole frame.

Final packing, scoped regression results, layout equality, bank bindings,
previous-export rejection and protected-save evidence are recorded in
`x/core-status-reactions/verification.json`. Tests use private native VAT/save
paths; the standard campaign group and live TiEmu are not used.

The23 affected older suites all pass, retaining their previous digests:
stat-save, critical-reactions, reactions, context, strikes, hands, charge-hands,
actions, scheduler, deferred, batch, buffs, reflect, status-lifetime, control,
status-state, protect, lancer-jump, area-impact, area-forecast, targets, math and
boundaries. This is an affected closure, not the full census. Bank parser130 and
exact binding170 checks pass, including36 cold/warm loads, stale contexts and
rejection of the actual ABI60 export. Private Gariland81/AE9CFD31 and
Orbonne14/2CD70391 agree PC/TI. Native/host layout probes exactly match the Jump
checkpoint. All486 pre-existing protected files retain their bytes and dates;
eight byte-identical oracle save fixtures had their original dates restored.
