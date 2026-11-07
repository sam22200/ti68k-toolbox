# Brave Up, Faith Up and Sunken State

2026-11-03. Three additional equipped reactions now use the shared bounded
response pipeline: Sunken State425, Brave Up429 and Faith Up430. Direct command
use stays rejected. The source catalogue and201/201 generic action coverage are
unchanged; broader R/S/M and full production certification remain open.

## Original evidence and rules

`tools/brave_faith_reactions_oracle.py` executes unmodified retail MIPS for69
staging profiles,37 isolated effects and three self-target lists. It also checks
two self-cast Faith trigger profiles, Transparent's four cleared evade bytes,
its four execution RNG draws and four
turn-removal boundaries. Function/disc hashes, controlled BIOS RNG, isolated
bookkeeping/graphics handlers and raw rows are in the private
`x/core-brave-faith-reactions/oracle/source.json`. This is not a full original
UI/AI/animation replay.

Sunken State and Brave Up use the same pre-impact physical-eligibility flag as
Caution/Dragon Spirit. A miss, zero damage, healing or a no-op may still stage
a response. Math, formula7 and blocked/secondary reactors cannot stage it.
Faith Up uses the recorded effective MP cost: any nonzero value can stage a
response, independently of the physical flag, hit or HP category. It includes spells
cast on oneself, checked with two original zero/nonzero-cost profiles. Formula7 and
ordinary reaction restrictions still apply. A free Math/Mime cast or a cost
rounded to zero does not trigger Faith Up. Cost eligibility reads no remaining
MP and makes no payment. A Brave chance draw occurs only after eligibility;
Brave0 consumes one failed draw for these three pre-impact reactions.

Brave Up adds3 Brave and Faith Up adds3 Faith, capped at100. They use the
existing signed-stat effect encoding and commit owners. Brave's ordinary
Chicken rules apply: crossing10 removes mutable Chicken. Wall prepares a miss;
late KO/incapacity cancels a queued response through the existing drain guard.
These self effects receive no secondary EXP/JP, MP/stock cost or turn credit;
the primary packet and its deferred reward remain owned until the queue drains.
The queue count byte has an explicit CTX_QUEUE_SELF permission bit, used only
by Faith Up; its lower seven bits keep the same bounded actor count. Strict
callers still reject the root actor and every secondary/nested enqueue. No
new context fields or allocation are introduced.
Permanent campaign Brave/Faith carry-over is outside this reaction slice.

Sunken State requests canonical Transparent19 (byte2/bit0x10), with ordinary
current/innate/immunity and late-commit filtering. An already-current request
can be a successful source no-op. HP-damage-type hits remove mutable Transparent,
even at zero damage; misses/healing preserve it. Ordinary attacks from a
Transparent actor bypass evasion and consume the source's four evade draws.
The existing spell/Throw path already clears all four evades while retaining
its separate formula accuracy. Manual targeting, tile occupancy and collateral
area effects remain legal. Hostile scalar AI rejects it, nearest-enemy/facing
queries skip it, and area/Math utility ignores it while the real footprint still
resolves every occupant. This is the existing native AI policy, not a complete
original AI replay.

A turn-start flag arms action-based removal. A fresh infliction clears that
flag, so a reaction gained during one's own turn survives that same turn's Act.
Walking/Wait preserves it; Act on a later turn removes it. Jump removes mutable
Transparent immediately after its order is accepted. The flag uses the unsaved
spare bit6 of the existing packed allegiance byte; Invite preserves that bit.
The restricted generic status-injection API stays unchanged: this producer is
owned by Sunken State. For TI readability, actors retain their reviewed outlined
sprites; the existing impact-label cache displays Hidden. No new sprite/art
bank, alpha renderer or PS1 fade animation is added.

## Storage and module ownership

ABI62 keeps ModCtx360,18 images,9+9 FMB3/FMD2 shards and the8390-byte logical
execution zone. RESPONSE selects reactions. After returning to the resident,
FORMULA14 prepares the self-status/Brave/Faith packet using its existing status
filter; FORMULA13 commits it. HP/MP/Quick construction remains in RESPONSE.
Consecutive-strike target/preparation moves from FORMULA7/8 to RESPONSE6/7;
ORDER16 owns Defend clearing and Transparent arming at turn entry. No executing
image loads another. The pure callback table and gameplay/save layout sizes are
unchanged; no heap allocation is added.

Banks use62374/63840 bytes (1626/160 below64000). Actual TTPack input64927 is
within64936, leaving9 bytes; private packing produces a40122-byte data file.
The next addition must budget resident and both shards before adding code.
Full-frame/cold rendering, stack/residency and release gates remain open.

## Verification

`make core-brave-faith-reactions-check` compares1372 logic assertions
(300DDA42) and98 UI assertions (568EC696),14 pictures, on PC, ASan/UBSan and the
actual68000 with archived images. They cover source staging/RNG/clamps/statuses,
late effects, once-only impact, root restoration, both HP-free status owners,
Transparent damage/heal/turn/Jump behavior and actual AI planning. Isolated
source effects with Chicken/Frog are injected after response selection, since
those states correctly block normal draining.

Response preparation peaks at107290 cycles and status commit at63550 in these
fixtures. UI measurements include cold image loads and do not certify a whole
frame. Final scoped regressions, bank bindings/old-export rejection, actual
layout probes, private Gariland/Orbonne flows and protected-file hashes/dates
are recorded in `x/core-brave-faith-reactions/verification.json`.

The size-bound area-impact regression carrier exceeded its variable ceiling by
16 bytes. Its first/last carriers now omit only diagnostic cycle markers; part4
retains zones57/58. All1012 assertions and64 exact framebuffer samples remain,
with their previous digests. The production core and its execution zone are not
changed by this checker-only transport adjustment.

All27 affected older suites pass. Six context/reaction/batch suites were rerun
again after opt-in self queueing; the previous default-queue digests remain.
The area-impact native transport rerun supersedes its size failure in the first
closure; all64 images and the separate154 impact-bound assertions pass.
Parser130 and binding170 checks,36 cold/warm image loads, stale contexts and
rejection of the actual ABI61 export pass on the final banks. Host/native layout
probes exactly match the previous checkpoint. Private Gariland81/AE9CFD31 and
Orbonne14/2CD70391 flows also pass (before opt-in self queueing; strict behavior
is unchanged and checked again). All491 pre-existing protected files retain
their bytes and dates. Full evidence and the fixture-date restorations are in
verification.json; live TiEmu and the standard campaign group were untouched.
