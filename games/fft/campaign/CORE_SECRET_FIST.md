# Physical status family: Secret Fist

Current follow-up: CORE_LINE.md now enables Earth Slash; this note preserves
the earlier Secret Fist checkpoint and its measured scope.

2026-11-01. Formula0x50's only generic-career producer, Secret Fist (id104),
is enabled. Production now has147 normalized records,101 owned casts and
141/201 generic entries;60 entries remain. Earth Slash still requires a
line-targeting implementation. This checkpoint does not complete the core.

## Observed source rules

`tools/secret_fist_oracle.py` executes the untouched retail
`battle_formula_hit_ma_x_percent_physical_evade`, its support/status/zodiac
helpers and `battle_action_finalize_target_current_action` under Unicorn.
Function bytes are checked against the local decomp's address/size/hashes;
the report includes the executable and battle-overlay hashes.

- Start with MA, apply Attack UP x4/3 and Martial Arts x3/2 when present, then Defense UP
  x2/3, Protect x2/3, Sleep/Charging x3/2, Chicken/Frog x3/2 and zodiac.
  Add separately zodiac-adjusted X=50. Each stage uses the source rounding;
  Serpentarius leaves both terms unchanged. Accuracy caps at100.
- Faith, Magic Attack UP, Magic Defense UP and Shell do not modify this
  physical-status formula. It has no weapon stage: Martial Arts also applies
  with an equipped weapon. The selected attacker's ordinary weapon stays intact.
- **Secret Fist ignores evasion.** The original formula calls the physical
  evade helper, but the retail record's flags4 byte is0: the helper's final
  stage skips both the evade multiplier and all evade RNG. Front-facing
  class evade99 plus shield/accessory profiles give the same result as no gear.
  Older Punch Art/Talk notes describing physical evasion were incorrect.
- Below100%, execute draws once for the percentage, even when the subsequent
  status filter rejects Death Sentence. At100%, no formula RNG is consumed.
  Preview consumes no RNG.
- Success requests status set61 (Death Sentence), with no HP/MP channel.
  Immunity, innate status and cancellation rules use the shared source rows.
  An existing counter is preserved. Expiry after three of the target's turns
  kills a living target; an Undead target only loses the status.
- Immediate, no MP, range1, equal standing height, radius0, no self target.
  Silence and Reflect do not affect it. Frog cannot select it; Berserk's
  existing automatic control chooses Attack instead.

The185 source profiles cover MA and100% boundaries, supports, zodiac/sex,
Serpentarius, Faith independence, ordinary weapons, evasion gear, Protect,
Sleep/Charging/Chicken/Frog, magical-only modifiers, Poison, Undead,
Death Sentence refresh/immunity/innate status, and pass/fail seeds.
External RNG and graphical target sorting are controlled hooks; no original
area traversal, menu, reaction, reward or campaign simulation is claimed.

## Implementation

`secret_fist_record` accepts the exact14-byte id104 entry and its singleton
status set, rejecting changed fields and other formula50 producers. The
normalized record carries AB_STATUS/set61, range1/height0 and AF_CAST|AF_AREA;
the raw record retains X. Monster/weapon-proc formula50 entries stay gated.

The shared `magic_xa` chooses physical supports and target modifiers for
formula50 while retaining MA as the base. `magic_heal` supplies its pure
chance without Faith, and the existing preparation/filter/FORMULA commit
freezes and applies the packed status request. The ordinary countdown, KO,
rewards and number/forecast paths are reused. No new status, packet field,
save field or allocation is added. Area AI accepts the exact source primary
flags and uses its existing conservative status heuristic.

## Resident space and cold starts

An initial separate physical evaluator made the renderer/Talk native test
carriers too large. The final evaluator shares the existing modifiers.
`scenario_prepare` moves ordinary unit generation, initial bag, funds,
saved-party loading and Gariland story pointers into PARTY13, in their
previous order. These operations already belonged to PARTY individually;
the caller retains allocation, RNG seeding, actor-cache reset and CT/camera
callbacks. No module loads another while executing.

ABI50 rejects49; ModCtx remains388 bytes, with17 images and zone8186.
The existing isolated `cast_batch` and `battle_weather` injection APIs are
omitted from FFT_PRODUCTION_CORE, as other test-only exports already are.
They remain available in the ordinary PC/native check builds. Production
uses the owned packet and chapter weather paths.

TTPack input64559/headroom377, versus64741/195 after Geomancy. FCJB, FGS2,
FCD3 and FSV3 are unchanged. Unit/State/Work/Save and the allocation requests
are unchanged. These are file/zone measurements, not a combined heap/stack
or full-frame budget certificate.

## Verification

`make core-secret-fist-check`:4067 assertions,1FCD638D on PC, ASan/UBSan
and actual68000. Besides the source profiles, check learned lists, pure
forecast, frozen result, one impact/reward, HP/MP preservation, Sleep and
Charging preservation, refresh/immunity, three-turn expiry/Undead, command
gates/range/height, and actual AI inclusion/rejection. Native packets use
the production overlays. Final scoped closure:30 suites,30 run,0 cached,0 red (373s):
Secret Fist, Punch Art/strikes, Talk, Geomancy, Gil, steals/breaks, area AI/
impact/forecast/targets/batch, actions, banks/headroom, status lifetime,
reactions, evasion, healing/offensive/percentage magic, control, weather,
chapter cells and hand/Charge handling. The185-profile preparation peaks
at67778 native cycles; the first status impact takes115300 cycles, including
its image transition. These are backend zones, not full rendered frames.

Native Orbonne/Gariland pilot flows keep their pixels/rules, settled save,
repeat-payment and trophy-overflow behavior (AE9CFD31/81 and2CD70391/14).
The catalogue reader stays B9686645. Bank checks verify162 bindings and
reject eight bad starts, including a captured real ABI49 two-shard export.
Only the id104 normalized record changes in FCJB: replacing it with its
previous disabled bytes exactly recovers the old bank SHA and the current
engine's old actions digest60795172 (new D4899EEB). The old Punch Art gate
lists drop their obsolete Secret Fist rejection; their new summaries are
1254/4EA46E40 and6523/65EAACF8. Other checked behavior summaries are unchanged.

Private evidence: `x/core-secret-fist/verification.json`, oracle/source.json,
check/, final-closure.log, flow/, banks/, catalogue-control.json and
preservation.json. All260 historical save/group byte and date checks pass;
three regenerated identical oracle-save timestamps were restored. Only the
paired catalogue and executable-bank files change; the other recorded data
banks, chapter descriptors, live TiEmu and standard group remain intact.

## Remaining boundary

The AI uses the existing adapted status utility, without original future-turn
simulation. Full reaction/support/movement and innate-job support combinations
are still a separate core gate; the oracle uses the native-representable
support choices. Other formula50 monster/weapon producers need their own
eligibility and trigger fixtures. Jump remains deferred. No new presentation
is introduced; current approved status labels and effect drawing are reused.
Live TiEmu, standard campaign group and user saves are preserved.
