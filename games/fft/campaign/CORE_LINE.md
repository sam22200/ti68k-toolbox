# Linear attacks: Earth Slash

Current follow-up: [CORE_DRAW_OUT.md](CORE_DRAW_OUT.md) enables all ten Samurai
techniques on this shared line/area path. The numbers below describe the
historical Earth Slash checkpoint.

2026-11-01. Earth Slash (id103) enables the remaining generic formula31
producer. Production has148 normalized executable records,102 owned casts
and142/201 generic action entries;59 remain. This is the shared cardinal
line-targeting family, not an activation of Draw Out, Throw or Jump.

## Observed original rules

`tools/line_oracle.py` executes unchanged retail MIPS instructions through
Unicorn2.1.4, checking the local decomp function hashes and recording both
disc executable/overlay hashes. It separately observes27 green-panel/list
footprints and75 formula/finalizer profiles. RNG and graphical target sorting
are controlled hooks in formula fixtures; geometry executes the real source
list sorting.

- Immediate,0MP, cardinal selector up to8 tiles from the caster. Selection
  height is unrestricted. The affected ray is based at the caster, extending
  through the selected direction; it does not stop at the selected panel,
  a victim, a gap or an occluding wall. There is no terrain propagation rule.
- The finite footprint uses source radius8 and vertical tolerance2: choose
  the nearest raw floor in each column, ties lower, then test doubled height
  tolerance. Existing water/half-height and blocked-floor semantics apply.
  Only the actual caster panel is excluded: the opposite floor at the origin
  may be hit when source floor selection picks it.
- The source query kernel accepts two rays for a diagonal reference and none
  for identical x/y. Public commands, manual range and AI selectors enforce
  the source cardinal selection. This distinction is tested explicitly.
- Source lists are sorted by distance from the caster, preserving slot order
  for ties. Formula RNG follows that order; every result freezes before HP.
- Formula31 uses PA times floor((PA+Y)/2),Y0. Strengthen precedes Attack UP /
  Martial Arts, then Defense UP / Protect, Sleep / Charging, Chicken / Frog,
  zodiac and the critical bonus. Faith and magical supports do not apply.
  Serpentarius leaves the XA stage unchanged. No weapon-power stage is used.
- Earth Slash ignores evade multipliers and draws (retail flags4=0), although
  its formula calls the physical-evade helper. Class evade99 with a frontal
  target does not change the observed result.
- The formula still rolls a4% critical and, when critical, its amount.
  Nullification/Float occurs after these draws. Non-null results also draw
  the ordinary conditional-status roll, despite the empty status set.
- Earth strengthen/half/weak/null/absorb and Float use shared typed results,
  signed narrowing and positive999 cap. Absorption heals; null consumes no
  later status-proc draw. The ordinary finalizer rejects Dead/Petrify/Wall
  after formula draws; absent/Jump targets never enter the footprint.
- Silence and Reflect do not block/bounce it. Frog cannot select it, and
  existing Berserk control restricts automatic actions to Attack.

## Native implementation

`line_record` accepts exactly id103's14 source bytes; other producers stay
strictly gated. FCJB's normalized entry carries AB_DAMAGE,range8,height255,
AF_CAST|AF_AREA,formula31 and element8. No format is changed.

`area_cell` is shared by target masks and the approved panel preview. The
same raw cardinal flag constrains `cast_can_order`, `skill_range` and
`ai_reaches`. Area AI retains its bounded query/scan budget and evaluates
origin/direction pairs; radial centre pruning must not discard a linear
centre whose direction changes at another origin.

Punch Art strikes now reuse the shared PA/physical XA stages rather than
copying the new Earth strengthen/Defense UP stages into resident code.
The source quantities and typed null/heal result feed the existing frozen
batch, response queue, damage wake, KO and one-time reward paths. A sole
critical line victim can use the existing scalar knockback fields and
geometry; multiple victims do not knock back. Packets retain their layout.
Native pure forecasts display ordinary noncritical damage without preview RNG.
Native response draining keeps its established actor-ID queue order; this
is not an original reaction dispatcher/order certificate. The ordinary
knockback adapter's broader mount/importance/absorbed-critical/fall policies
remain outside the new scalar oracle, which deliberately uses target_count0.

ABI51 rejects the previous50 image contract: target/panel operations now
support linear sets and preparation can carry their frozen scalar push.
ModCtx388,17 images, Unit/State/Work/Save/FSV3 and allocation requests are
unchanged. No new assembly, sprites, portraits or presentation style.

## Verification

`core-line-check`:2786 assertions,59AFCB33 on PC, ASan/UBSan and actual68000.
The27 footprints compare both masks and every native panel against source
marks, including reverse compact IDs, two floors, ties and256 nodes.
Formula cases compare amount/category/hit/RNG, including absorbed/null and
blocked victims. Production cases cover learned lists, cardinal/self gates,
range panels, zero cost/no delay, frozen impacts/once-only commit, nearest
RNG order, a single critical push, Float, absorption, Silence/Frog and actual
AI selection. Existing area presentation is reused.

Measured backend zones:27 footprint calls total1688260 cycles,mean62528,
peak73068; the two-victim cold preparation190126 and impact8906 cycles.
These are backend measurements, not full rendered-frame or combined
heap/stack certification. Production TTPack input64735/headroom201; module
zone8236 (RESPONSE). The static zone is part of resident input, not heap.
A first native Talk carrier exceeded the variable limit by14 bytes. Removing
redundant formula/element tests from commit (only a sole line victim can
prepare scalar push) reduced the largest image28 bytes; no checks were dropped.
A final monotonic cursor keeps radial slot scans linear. In the13 batch
fixtures, preparation total1831928 ->1786488 cycles and peak159314 ->154986
versus the first line implementation; commit total1074012 remains equal.
This is a targeted backend comparison, not an overall frame-speed claim.

Final direct regression closure:26 suites,26 run,0 cached,0 red,235s:
line,Punch Art/strikes,Secret Fist,Geomancy,breaks,Steal Exp,gil,Talk,
targets,batch,area-forecast/AI/UI/impact,weather,magic-evasion,actions,
banks/headroom,healing,offensive-magic,Reflect,reactions,Charge/hands.
After the radial cursor adjustment, seven directly affected suites rerun
green in224s:line,batch,Geomancy,area-forecast,area-impact,buffs,healing.
Their summaries remain equal; the other unchanged-family evidence is reused.
This is scoped validation, not a new72-suite certification.

Only id103's normalized12 bytes change in FCJB. Restoring its disabled record
recovers the exact previous catalogue SHA and current-engine action digest
D4899EEB; the enabled catalogue gives5324E3C7. No golden is refreshed. Punch
Art/strikes drop their obsolete id103 gating assertion only:1253/BE27214C and
6522/891FD13C. Existing checked family summaries and area pictures
F9BACFF5/9DE20073 remain equal. The common reader remainsB9686645.

Native Orbonne/Gariland pilot flows preserve their rules/pixels, settled save,
no duplicate payment and trophy overflow (AE9CFD31/81 and2CD70391/14). The
final images replay through the existing unchanged resident native carriers;
PC pilots are rebuilt. Bank checks compare162 exact context bindings and
reject eight bad starts, including the actual captured ABI50 two-shard export.
All263 historical save/group byte and date checks pass; three regenerated
identical oracle-save timestamps were restored. Only eight catalogue/code
archive files and generated modules.h change among120 recorded data files.
The user's live TiEmu is preserved. The standard campaign group is not rebuilt.

Private evidence: x/core-line/verification.json, oracle/{source,geometry}.json,
check/, final-closure.log, scan-closure.log, final-flow.log, final-banks/,
headroom.txt, catalogue-control.json and preservation.json. Jump remains
deferred and the complete core remains open.
