# Charm and Invitation: current camp and permanent ownership

2026-11-02. Steal Heart (109, formula0x29) and Invitation (116, formula0x2a)
are executable shared-core commands. Production has174 normalized records,
128 owned casts and168/201 generic abilities;33 remain: Throw12, Jump12
(deferred), Arithmeticks8 and Float1. This is a core milestone; the complete
content/release gate remains open.

## Original evidence and rules

`tools/allegiance_oracle.py` runs unchanged retail MIPS formula/finalizer,
status-store/control/restoration, ordinary outcome and initial-camp stock
instructions from the local disc. Its local JSON records executable/overlay/
function hashes,236 formula profiles,18 camp/control traces, repeated Charm,
two ordinary outcome cases and four stock cases. RNG and external graphical/
sort handlers are isolated as documented in the script. These are bounded
function trials, not a full original action/menu/AI/recruitment playthrough.

- Steal Heart uses (zodiac(MA)+zodiac(50))%, capped at100, no Faith, Magic Attack UP or
  evasion. Its opposite-sex restriction runs after the accuracy stage: failed
  same-sex attempts retain that RNG ordering. Sleep does not introduce the
  Talk Skill early rejection for this formula. Range3, no height restriction.
- Invitation uses the existing Talk Skill (zodiac(MA)+zodiac(20))% formula
  capped at100, without Faith, Magic Attack UP or evasion. The existing Talk
  Sleep gate applies. Range3 and height3 (native doubled height6).
- Status selection/finalization still filters current status, innate state,
  immunity and source context after formula preparation. Both commands use
  their actual source status rows (62/63), rather than an invented effect.
  Job and equipment immunity to Charm/Invite is read from the disc.
- Charm changes the current camp and forces AI. It preserves the initial camp,
  lasts32 clock ticks, refreshes to32 when inflicted again and restores the
  initial camp on expiry/cure or mutable HP-damage cancellation. Successful
  zero-HP-damage-type hits also cancel it; innate status remains protected.
- Invitation changes permanent allegiance. Clearing the Invite status does not
  reverse the conversion. The source camp/store cases distinguish this from
  Charm. A native blue convert remains under AI control for this battle.
- Ordinary battle outcome and item stock follow permanent allegiance. Merely
  Charming every enemy does not win the battle. Current camp continues to
  drive targeting/AI and source gil changes; fixed enemy-level victory bonuses
  retain the original spawn-side classification.

The oracle covers human native camps and their guest ownership mapping.
Original control-flag retention on already player-controlled Invite recipients
is not fully represented: native Invitation consistently forces AI until the
next battle. Full four-camp manual control, monsters, Monster Talk/FingerGuard
producers and source recruitment confirmation/dismissal menus remain outside
this milestone.

## Shared implementation

`allegiance.h` makes current camp, permanent camp and original enemy identity
explicit queries. `allegiance.inc` owns actor-aware conversion. The new byte
occupies former Unit padding at native offset73; Unit112, State2210,
Work45576, Save3138 and all existing member offsets stay fixed. Its low bits
retain Charm's home camp; separate flags retain permanent conversion and the
original enemy identity. Status refresh restores a cured Charm before deriving
control flags. Generic actorless status requests may remove Charm/Invite but
still reject adding them without an owning caster.

Strict `allegiance_record` accepts only the two measured14-byte source rows
and their status sets; changing any byte fails recognition. Existing targeting,
CastBatch/CastVictim, formula forecasts, frozen RNG results, typed status
commits, cancellation/timers, responses, rewards and impact presentation are
reused. A live root rejects an overlapping command without changing State,
RNG or pending actions. Charm also cancels a target's pending performance
through the existing source status cancellation rules.

The FCJB battle section appends416 byte-sized immunity entries:160 jobs and
256 equipment items, with one bit for Invite and one for Charm. The query
checks the live job and five equipped items only for these two commands;
there is no RAM cache or per-frame scan. Both commands cost zero MP. FCJB grows41406→41822 bytes. Its
format version is unchanged, but validation requires the enlarged section.
Only normalized ability rows109/116 change. FGS2/FCD3/FSV3 stay unchanged.

Inventory/capability/payment/AI stock labels, Draw Out break loss and stolen
gear ownership use permanent camp, including blue guests. Victory/defeat and
roster export also use permanent camp. Shared JP recipients and fixed spawn
bonuses keep their distinct source policies. Do not replace all `team` queries
with one helper: these concepts intentionally differ.

Victory export retains a charmed player, excludes a charmed enemy, removes an
imported player permanently converted to the enemy and remaps live roster
slots after compaction. Newly invited unowned actors occupy a free roster slot
at victory. Repeated saves cannot duplicate a recruit; a full16-member roster
keeps every existing member. Automatic enrolment into a free slot is a native
adaptation; original recruitment choices and dismissal UI are not implemented.
No save is written merely by applying an ability. Existing raw attributes,
job/JP/learned/equipment export is reused. Manual Auto-battle cancellation
respects forced control from Charm/Invite in portable and native input paths.

## Modules and calculator budget

ABI54 retains ModCtx388,18 images and a9+9 bank split. The social batch
prepares/commits through PERFORMANCE7/8, so FORMULA's status path does not gain
another resident callback. COMMAND0 receives actor/ref/ability in transient
ModCtx fields, validates before mutation and returns4 for rejection. These
arguments no longer travel through gameplay State.formula. Production capability
queries are local; only native checkers use a PERFORMANCE5 query wrapper.

Final image bytes in id order0..17:

```
AI8228 TURN3566 SCREENS7878 BEATS6304 SAVE8254 FORMULA8240
AI_SETUP6366 RESULTS6640 PARTY7428 ACTION8106 COMMAND8312 RESPONSE5642
TARGETS6850 AREA_AI8214 AREA_UI4876 NUMBERS8240 ORDER5212 PERFORMANCE8134
```

The archived banks are63026/63708 bytes, within the64000-byte per-bank limit.
The static execution zone grows8240→8312 bytes. Production packed input is
64683 bytes, leaving253 below the64936 TTPack limit (fresh paired baseline64667/269).
Future families may need another bank shard. The five existing heap requests
are unchanged: State+Save5348, depth32320, scene+Work61736, actor10176 and
variable navigation storage (tested maximum17408). No executable heap code,
new heap request or handwritten assembly is introduced.

A fresh compiler probe on both captured baseline and final sources measures
native State2210/storage5348 (host State2264/storage5402). The previous family
note's2192/5330 figures were stale; all current/baseline layouts and allocation
formulas match exactly. `layouts.json` and `layouts-baseline.json` retain that
comparison, including the Unit padding assertion.

## Verification and practical limits

`make core-allegiance-check`:5866 assertions,CF70C96D, identical on PC,
ASan/UBSan and real68000 modules under `ti-cycles`. Native math4702/1D196AB4
and runtime1164/59DA98BC are contiguous partitions of the same assertions;
counts and rolling digests recombine exactly. Both strict records have all
28 single-byte mutation rejections. All416 immunity rows match original data.

Tests cover formula/forecast purity and exact RNG, same/opposite sex, Sleep,
current/immune/innate/terminal statuses, source team storage, once-only owned
commit, expiry/cure/damage, repeated Charm, forced AI turns, cancelling a
performance, real inventory ownership, ordinary outcomes and private roster
recruitment/loss/full/repeated-save cases. Native formula zone61 peaks34272
cycles; marked command/root zone62 peaks139158 and impact zone63 peaks122428
across15 owned casts. These are bounded backend timings, not a whole-frame,
stack high-water, peak heap or fragmentation certificate.

The affected dependency closure runs49 older suites plus this family. Native
Talk/area-impact diagnostic programs are split into contiguous carriers with
recombined hashes because the aggregate program exceeds AMS's variable limit.
Geomancy uses native assertion ordinals while PC retains source lines; its
math carrier omits resident timing markers, retained in its runtime carrier. No
assertion or sampled frame is removed. Sleep's untyped non-damage witness now
selects Magic Break explicitly: its previous first-AB_BREAK search had begun
selecting Draw Out. Unsupported target/Talk controls use still-gated Throw/
Jump rather than these newly executable commands.

Original area pixel checks, bank bindings, previous ABI rejection, private flow
and save-reader results accompany the closure. All50 scoped suites pass.
Catalogue rollback restores the exact prior41406-byte bank; under the same
restored two records the captured baseline and current PC engines both give
actions95AE68E5/295. Enabled production gives688793BE/295.
All503 protected save/group hashes and mtimes match the baseline;111 of120
data files retain their bytes, with only the two catalogue copies/archives,
four code-bank files and generated module header changing. `x/core-allegiance/verification.json`
records final counts, hashes, native timings, catalogue rollback and preservation.
The separate legacy nonchapter `fft.c` demo already fails compilation on its
Ability.formula/State.support/forecast references, reproduced from the captured
baseline; active chapter/portable core verification is separate.

Tests use temporary PC directories/private native VAT saves. The live emulator,
user save and standard chapter group are not used. Original artwork, portraits,
font banks and source-handed projection are preserved. No hardware change
requires an emulator run at this checkpoint.
