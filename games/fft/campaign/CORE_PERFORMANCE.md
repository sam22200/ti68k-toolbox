# Songs and Dances: all fourteen persistent performances

2026-11-02. Bard: Angel Song, Life Song, Cheer Song, Battle Song, Magic Song,
Nameless Song and Last Song. Dancer: Witch Hunt, Wiznaibus, Slow Dance,
Polka Polka, Disillusion, Nameless Dance and Last Dance (ids86..99).
Production has172 normalized executable records,126 owned casts and166/201
generic entries;35 remain. FCJB stays41406 bytes; only these fourteen normalized
12-byte records change. FCJB/FGS2/FCD3/FSV3 formats are unchanged.

## Observed original rules

`tools/performance_oracle.py` runs unchanged retail MIPS Song/Dance formulas,
common finalizer, target panel/list routines, recurring CT state machine,
end-turn credit and movement cancellation. Source executable, overlay and
function hashes accompany180 formula profiles, six global footprints,
fourteen three-event clock traces and a performer/Quick/performer wave.
RNG is controlled. Graphical status handlers and isolated expiry/upkeep/sort
are hooked where documented; the full original action dispatcher is outside
this oracle.

- Zero MP. Starting sets Performing and arms the raw source period:6,6,8,8,
  10,10,20 for each seven-entry set. Non-Charge does not shorten these periods.
  Each due pulse reloads that period and resolves a fresh global target set.
  Starting consumes no formula RNG and no effect occurs immediately.
- Songs affect the caster's camp, including the caster; Dances affect the
  opposing camp. The existing source camp predicate includes allied guests.
  Both layers and all distances/heights share the same global mask. Source
  status/absence restrictions still filter targets. Sleeping targets fail
  before the formula accuracy draw.
- Angel/Life Song restore MA+20 MP / MA+10 HP. Witch Hunt/Wiznaibus remove
  PA+max(1,floor(PA*Brave/100)) MP / HP. The source ordinary-ability setup
  chooses unarmed weapon0 for these dances; their source Y is not that damage.
- Cheer/Battle/Magic Song raise Speed/PA/MA by1; Slow Dance/Polka Polka/
  Disillusion lower them by1. Native current attributes clamp to Speed1..50,
  PA/MA1..99 through the existing stat commit policy.
- Nameless Song chooses Reraise, Regen, Protect, Shell or Haste. Nameless Dance
  chooses Blind, Confusion, Silence, Frog, Poison, Slow, Stop or Sleep.
  Selection precedes immunity/current-status filtering. A rejected choice
  does not reroll. Frog selected on an ordinary Frog removes it; innate Frog
  remains. Selected outcomes are frozen before impact.
- Last Song fills CT255; Last Dance clears CT. The common scheduler drains
  the whole current area effect before Quick may interrupt another due pulse.
- Resource effects have source100% accuracy; stat/random-status/Last Song
  use50%, Last Dance34%. Execution consumes the accuracy draw even at100%.
  No Faith, zodiac, evasion or Reflect stage is added. Silence permits these
  performances; existing Frog/Berserk action restrictions apply.
- Movement retains Performing. A replacement command ends it, as do cancelling
  statuses such as Sleep/Frog/Berserk/Petrify. Stop freezes its clock. Don't Act
  skips a due effect while retaining/reloading its next period. Performing
  counts as an Act for the source20 CT end-turn credit.

## Shared implementation and storage

Strict `performance_record` validates every byte of the fourteen retail rows
and the two status sets. Existing AF_CAST/AF_AREA/AF_SELF records, global
radius255 targeting, CastVictim amount/kind, status filtering/timers, resource
commit, response queue, rewards and area presentation are reused.

Each actor uses its existing pending slot plus DEFER_PERFORMING. Preparing a
pulse freezes formulas/RNG/status choices for every victim before mutation;
impact applies that packet once. Neither new RNG nor live stat recalculation
occurs at impact. Status application preserves an existing valid performance,
while source status cancellation clears its pending slot. No new Unit/State/
Work/Save/action packet field, heap request or save version is introduced.

ABI53 retains ModCtx388 and adds archived PERFORMANCE17 (eighteen images,
split9+9). PERFORMANCE3/4 owns start/repeated events and7/8 owns isolated/owned
mixed resource/stat/status commits. Resident code returns from an image before
loading TARGETS; no image may load another image while executing. Native
COMMAND excludes this separately routed family from its unreachable paths.

To keep the existing8240-byte static execution zone and resident budget:
SCREENS11 receives102-byte constant CoreBoot pointer bindings, allocates the
same State+Save and view/Work/actor blocks, binds the catalogue and slopes, and
uses the explicit resident campaign-flow policy. Resident cleanup still owns
these blocks. PARTY10 receives a transient story-flag output pointer; PARTY13
resets native scenario state and returns the manual-map-start decision. The
resident applies its contour-build policy explicitly after that reset.
No original allocation size/order changes or executable heap code are added.

Shared stat text handles PA/MA/Speed forecast and impact labels without
expanding the near-limit NUMBERS image. Existing area controls, artwork,
portraits and fonts remain unchanged. Conservative adapted AI admits all14,
scores immediate useful resources/stats/statuses/CT, and retains an active
performance instead of replacing it with Defend. Movement/Wait remain available.

## Verification and limits

`core-performance-check` compares all source profiles, forecast purity, exact
RNG, typed resource/stat/status results and once-only commits on PC,
ASan/UBSan and actual68000 modules. Native transport splits the same contiguous
math/runtime assertions and recombines counts and rolling digests, because
the aggregate diagnostic program exceeds AMS's variable size limit.
The full check has5507 assertions,7FF2F648. Native math3727/3CCC9CCD
and runtime1780/B2313A81 assertions recombine exactly; final cycle zones
are recorded in `x/core-performance/check/results.json` and `verification.json`.

Production cases exercise all fourteen repeated commands, zero MP, raw delays,
Non-Charge, caster healing, current teams/stats at the next pulse, frozen
amounts at impact, stat caps, readable PA/MA/Speed and CT0 forecasts, cancellation/pause/skip behavior, real CT tick
order, Quick interruption after root settlement, source global masks, actual
AI admission and retaining an already active performer. Original geometry has
eleven actors; the thirteen-slot native capacity fixture explicitly hides its
extra two actors. A targeted Nameless Dance source seed24 checks Frog removal.

The initial36-suite direct closure covers this family and existing Draw Out/line/
Punch Art/Secret Fist/Geomancy/Talk/gil, targeting/batch/forecast/AI/UI/impact,
buffs/healing/offensive/Reflect/percent/restoration/cleanse/status/control,
scheduler/deferred/incantation/casting/reactions/evasion, actions and native
bank/headroom/cell/hands/Charge paths. Historical family digests and area
pictures stay equal. Targets' unsupported-formula control now uses still-gated
Steal Heart0x29. Native allocation failure injection now intercepts ti-cycles'
writable synthetic ROM allocator in C so the archived bootstrap sees it,
then restores that table entry. No source assertion or original mask is removed.
The final five-suite UI closure repeats performance/area-UI/area-impact/buffs/
Talk after the label corrections, with native pilot flows and bank bindings.
The new text assertions initially call strcmp, whose AMS ROM call is absent
from ti-cycles; replacing that checker-only comparison with supported memcmp
keeps all assertions/digests and allows the final native family retry.
This is scoped validation, not an all74-suite census.

The common reader staysB9686645. Restoring only the fourteen normalized rows
recovers the exact prior FCJB SHA and old current-engine actions5C985C3F/295;
the enabled catalogue givesD24AB392/295. Native Orbonne/Gariland flows retain
AE9CFD31/81 and2CD70391/14. Bank checks cover170 exact bindings and reject
eight bad starts including the captured ABI52 two-shard export.

Images (id0..17):8228,3568,7800,6304,7936,8154,6344,6592,7424,7978,
8240,5620,6822,8136,4848,8240,5134,7788. Banks62472/62928 bytes;
TTPack input64665/headroom271,12 resident bytes above the prior family.
The8240-byte execution zone is static resident storage, not a heap allocation.
Scalar preparation:180 calls,5447648 cycles total,mean30264,peak40148.
Recurring event preparation:42 calls,4075792 total,peak134802; impacts:28 calls,
2395916 total,peak119616. These backend zones include image loads; they do not
certify complete rendered
frames or combined stack/heap/fragmentation/residency peaks.

AI uses immediate utility, not the original full future-cycle search. The
random-status forecast supplies a filtered byte subset for the existing
Status/chance UI and adapted AI; original preview can stage multiple bytes.
Native stat changes use current attributes rather than reproducing the entire
original base/equipment/stat-cache dispatcher. Performing effects run on the
shared scheduler, but the combined original AT ability-event listing is not
implemented here. Full original acquisition, Mimic triggers, simultaneous
external status-event semantics and menu/reaction/reward/animation dispatch
remain separate. Jump and the complete-core/content gate remain open.

Evidence: `x/core-performance/oracle/`, `check/`, `closure.log`, `rerun.log`,
`final-rerun.log`, `family-final.log`, `ui-closure.log`, `banks-final/`, `flow-final/`, `common.log`,
`headroom-final.txt`, `catalogue-control.json`, `preservation.json` and
`verification.json` and the merged per-suite `closure-final.json`. All491 protected save/group byte and timestamp checks pass;
six identical
regenerated oracle-save fixture dates are restored. Only eight of120 recorded
data files change: catalogue copies/archives and code banks/archives. modules.h
retains its bytes; descriptor/art banks are unchanged. The standard campaign
group is not rebuilt and the live TiEmu is untouched. Protected save/group bytes and dates are checked against
the pre-family snapshot; identical regenerated oracle fixtures have their
original timestamps restored only after byte equality is verified.
