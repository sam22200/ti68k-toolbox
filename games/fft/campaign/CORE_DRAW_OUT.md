# Draw Out: all ten Samurai techniques

2026-11-02. Asura, Koutetsu, Bizen Boat, Murasame, Heaven's Cloud,
Kiyomori, Muramasa, Kikuichimonji, Masamune and Chirijiraden (ids76..85)
share the existing frozen area-action owner. Production has158 normalized
executable records,112 owned casts and152/201 generic entries;49 remain.

## Observed original rules

`tools/draw_out_oracle.py` executes the four unchanged retail MIPS formulas,
common target finalizer and katana finalizer under Unicorn. Source function,
executable and battle-overlay hashes accompany142 scalar profiles and36
compound groups. Twelve original Kikuichimonji green/list footprints cover
three terrain profiles in four directions. RNG is controlled; graphical target
sorting is hooked in scalar fixtures and executed in geometry fixtures.

- Immediate,0MP. Nine techniques affect a radius2/height3 around the actor.
  HP/MP attacks affect enemies; healing/buffs affect allies including the actor.
  Kikuichimonji uses a cardinal ray of length8/height3, based at the caster,
  with the shared nearest-floor/nearest-target ordering and caster exclusion.
- Formula0x20: MA x Y with Magic Attack UP, Magic Defense UP, Shell,
  Chicken/Frog and zodiac stages; no Faith. Heaven's Cloud has a19% Slow
  proc. Muramasa has a19% proc selecting Confusion or Death Sentence before
  immunity/current-status filtering. Empty sets still consume the proc draw.
- Formula0x21: Bizen Boat removes MA x Y MP with the same magical modifiers,
  no Faith and no status-proc draw.
- Formula0x22: Kiyomori grants Protect/Shell; Masamune grants Haste/Regen.
  Existing status filtering and timers apply, without an accuracy draw.
- Formula0x23: Murasame heals plain MA x Y, without supports, zodiac or Faith;
  Undead reverses the HP direction.
- No evasion, Reflect, Silence block, critical or knockback. Frog cannot
  select these techniques; the existing Berserk controller restricts actions.
- A blue-camp actor needs the corresponding katana in the bag; an equipped-only
  katana is insufficient. Enemy availability uses its source required level.
  The native Mime identity bypasses stock and break rolls, but this does not
  enable the original Mimic trigger/job dispatcher.
- Before each target's formula, roll0..99 against that katana's source weapon
  power. The last target's staged result wins. The action loses at most one
  katana. If no target succeeds, the loss is cancelled. A late Dead/Petrify/Wall
  finalizer also clears the staged loss, even after earlier successful targets.
  Merely failing an immune status does not clear it when another target hit.

## Shared implementation and storage

The strict `draw_out_record` accepts exactly the ten source rows and status
sets. Catalogue size and FCJB/FGS2/FCD3/FSV3 formats remain fixed. The existing
area masks, range preview, command owner, quantities, status timers, numbers,
response queue and rewards are reused. AI uses the existing conservative
immediate utility policy; Bizen Boat can deny MP to a learned caster.

Preparation freezes the selected status in amount bits10..11 (damage uses
bits0..9), and uses the existing action-exclusive `st.crit` scalar for pending
katana loss. Draw Out never treats this value as a critical or push. Numbers
strip the packed choice. Impact applies the prepared status and decrements
stock once, without RNG or recomputing MA. No packet/Unit/State/Work/Save
layout or allocation grows.

Katana quantities live in the existing Save tail beside the live funds.
Failed save loading clears these ten quantities. Victory save refreshes roster
and flags while retaining the live quantities; repeated saving cannot consume
another katana. Retry/reload restores the persisted inventory. Tests use
isolated PC files and the native runner's private VAT.

ABI52 keeps ModCtx388 and17 images. Ordinary nonstatus batch commit moves
from RESPONSE to SAVE7/8; status commit stays in FORMULA12. Cold file/catalogue
bootstrap moves to SCREENS11, supplied with a50-byte constant pointer-binding
template and the resident campaign-flow policy through the existing transient
context union. Allocators and live module-loading callbacks remain resident; no image calls another image.
No assembly, art or portrait renderer is added.

## Verification and limits

`core-draw-out-check`:4625 assertions,5AF7EF14 on PC and ASan/UBSan.
The full checker exceeds AMS's variable limit, so native transport uses the
same contiguous math3202/84AED311 and runtime1423/3377AD43 assertions.
Their counts and rolling digests recombine exactly to the full PC result.
No assertion/profile is removed. Geometry compares masks and every panel;
compound cases check exact RNG, last-target breaks and once-only stock loss.
Production cases cover the actual command owner, frozen MA/status results,
HP/MP channels, stock/equipped-only/Mime/enemy gates, Silence/Frog/Reflect,
Undead healing, conservative actual AI and private victory/retry save bytes.

Native scalar preparation:142 calls total4396756 cycles,mean30963,peak33552.
Ten real commands total1394500,peak146932; their impacts total860886,
peak151422. These include cold image loads where needed. The largest image
is COMMAND8240; SCREENS7476 now contains bootstrap and SAVE7936 batch commit.
All17 image sizes and source/artifact hashes are in verification.json.
Production TTPack input64653/headroom283; the static execution zone counts
against resident size, not heap. Neither this measurement nor the backend
zones certify full rendered frames or combined stack/heap maxima.

The32-suite direct closure covers Draw Out, line/Punch Art/Secret Fist/
Geomancy/Talk/gil, targets/batch/area forecast/AI/UI/impact, buffs/healing/
offensive/Reflect/percent/restoration/cleansing/status lifetime/reactions/
evasion/weather, actions/banks/headroom/cell/hands/Charge/control status.
Thirty pass initially; Talk and area-impact native carriers exceed the
variable cap by42/76 bytes. Their native failure diagnostic now reports the
assertion ordinal (PC keeps source lines); every assertion and scenario stays.
Both rerun green. Historical family digests and area picturesF9BACFF5/
9DE20073 remain equal. This is scoped validation, not an all73-suite census.

A final performance follow-up removes cast_dispatch's obsolete RESPONSE
preload: commit now belongs to SAVE/FORMULA and this copy was never used.
The eight directly affected suites and native pilot flows rerun green, with
unchanged digests/pictures. Command preparation drops1768300 ->1394500 cycles
for ten actions (37380 cycles each), peak184312 ->146932; impact totals stay
860886. This is a targeted backend comparison, not an overall frame-speed claim.
The eight-suite closure takes239s; the initial32-suite closure takes354s and
the two diagnostic-only retries104s. Logs and exact totals are in verification.json.

All266 protected save/group byte and timestamp checks pass; three regenerated
identical oracle-save dates are restored. Only nine recorded data files change:
the catalogue's two chapter copies/archives, two code banks/archives and
modules.h. Descriptor/art banks retain their bytes. The standard campaign group
is not rebuilt and the live TiEmu remains untouched.

Only the ten normalized12-byte records change in the41406-byte FCJB.
Replacing them with their previous disabled bytes recovers the exact previous
catalogue SHA and current-engine actions5324E3C7; enabled actions5C985C3F.
The common reader remainsB9686645. Native Orbonne/Gariland pilot rules/pixels,
settled save, repeated receipt and trophy overflow remain equal
(AE9CFD31/81 and2CD70391/14). Bank checks retain162 exact bindings and reject
eight bad starts, including the real captured ABI51 two-shard export.

Private evidence: `x/core-draw-out/verification.json`, oracle/{source,geometry}.json,
check/, final-closure.log, carrier-closure.log, preload-closure.log,
preload-flow.log, flow-final/, banks/, headroom.txt, catalogue-control.json and
preservation.json. The source
oracle covers the four formulas, status filtering, exact RNG and break staging;
it does not certify a full original menu/AI/reaction/reward/animation dispatcher.
Original Mimic activation and job acquisition remain separate. The shared
native response order and conservative AI retain their documented bounds.
Backend cycle measurements are not a full rendered-frame/stack certificate.
Jump and the complete-core/content gate remain open.
