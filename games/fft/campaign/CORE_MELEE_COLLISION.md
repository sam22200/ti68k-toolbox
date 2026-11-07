# Strike and lunge collision integration

Current panel/preparation ownership: [CORE_PANEL_VICTIMS.md](CORE_PANEL_VICTIMS.md).
The checkpoint below is historical.

2026-11-06. Continuation of the open interactions delivery. Generic action/RSM
coverage remains201/78. This closes short-weapon collision integration; exact
arcs, full panel victims and the remaining AI/Reflect work are still open.

## Implemented behavior

Contact strikes use target/actor body-height overlap. One-tile lunges have the
source two-half-height allowance. Two-tile lunges inspect both middle terrain
layers and their occupant spans, retaining an open part of the attacker/target
height window. If that window closes, an intermediate body containing the
attacker's midpoint intercepts the hit. A solid empty column instead stops it.
Floor sorting, strict boundaries, byte heights and lower-before-upper fallback
match1536 unchanged-retail control-flow decisions.

The native adapter includes normal/Frog/Chicken bodies, Float/water movement
and the shared-panel body offset/donor convention. Absent, KO and airborne
actors provide no ordinary target span. These actor adapters are a native model:
the interval oracle supplies controlled height/occupancy records and does not
certify the retail height producer for every overlapping KO/live/mounted case.
Complete overlapping-panel victims remain separate work.

PLANNER24 now prepares physical strikes and selects the actual intercepted
recipient before evasion, critical, variable-power and proc RNG. A blocked blow
consumes no formula RNG. The original chosen panel stays fixed for later hands,
Charge and Mime. Ordinary strikes, due Charge, both hands, Counter and Hamedo
reach this same preparation owner. Hamedo also uses it for its pure preflight.
Existing direct projectile obstruction is applied at this shared preparation
point, including the first ordinary strike. The old ray algorithm is unchanged;
a bow still needs its original ballistic-arc integration.

The resident no longer runs redundant line queries before Counter, a later hand
or Mime: preparation performs the query after the constructor returns. Rejected responses restore their suspended root
packet; a rejected later hand cancels without a new animation. The selected
primary weapon determines geometry for every hand, independently of the hand
whose damage is being calculated. This keeps one geometry decision per strike and recovers resident bytes. No executing
image loads or replaces another image on its stack. Existing impact/weapon
presentation is retained; no renderer or art bank changes.

AI cache preparation now builds a13-bit target-clear mask per candidate origin.
Four reachable origins per update, with a bounded64-cell sparse scan, reuse the
same pure short-weapon geometry. Interrupted farther targets are rejected by
candidate ranking; the AI does not intentionally value shooting through a friend.
The original scalar/drain/proc rows remain four pairs per update. PC and native
execute the same quanta. Direct/arc weapons keep their existing planner policy.

## Ownership and capacity

ABI74/ModCtx348 keeps25 images. PLANNER24 owns preparation, pure line preflight
and the short-weapon origin masks. WEAPON22 owns impact and scalar/utility cache
rows (op6), plus its forecast queries. This intentionally supersedes ABI73's
PLANNER-only scalar-cache ownership: separating the two cold tasks keeps the
execution-zone growth bounded. Do not route scalar rows to PLANNER0 or restore
WEAPON0 preparation. Resident AI dispatches the512 mask phase separately from
the nine scalar row bits; no new nested overlay call is introduced.

Unit112/State2224/Save3202/FSV4 and five heap blocks remain. Work46632 adds514
transient bytes (512 mask bytes and a two-byte scan cursor). The execution zone
is8604 (+54); total requested resident/work growth is568 bytes. The25 images
occupy8/9/8 archived shards56358/61580/57032, below64000 each. Actual non-BENCH
raw64891/TTPack64893 leaves43 bytes, versus5 at ABI73. Packed data is39799 bytes.
No catalogue, save, portrait or equipment format changes. Production line-query
callers already validate actor slots; diagnostic builds retain the extra entry
guard. Keep that precondition when adding production call sites.

## Evidence and remaining boundaries

melee_oracle.py executes the hash-verified retail lunge routine unchanged, with
only external height and occupant providers isolated.1536 cases cover body and
relative heights, one/two-tile strikes, solid/absent floors and either/both
middle occupants. All1570 focused assertions agree on PC, ASan/UBSan and actual
68000, digestF5544F48. Integration includes interception/fixed panel, solid
blocking without RNG, KO middle bodies, Hamedo and pure origin-mask rejection.
The earlier4741 weapon and21127 temporal/weapon-AI assertions are retained.

Area-AI quanta change deliberately: the extra geometry-cache updates raise the
fixture from68 to75 steps and add35 state checks (791/7A1C8C57). PC/native steps
and state digests agree. The area quantum remains311538 cycles; general stage0
still reaches12334482, so the full-planner360k gate stays red.

The source controls do not certify original arcing-projectile stepping or the
complete target/response dispatcher. Scalar preview/panel-recipient integration,
all overlapping-panel victims, wider cooccupant/innate details and candidate-origin
Reflect utility still belong to the interactions delivery. Worst tactical frame,
stack, residency and hardware remain in the later stabilization/release delivery.
No new missing-generic-family list is opened.

Current validation reports, actual export/rejection, pilot flows, codegen,
capacity and protected-file preservation are under x/core-melee-collision/.
Standard groups, user saves and the live TiEmu session are preserved.
