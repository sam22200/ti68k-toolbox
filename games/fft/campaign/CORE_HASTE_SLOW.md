# Haste/Slow clocks and finite AT forecasts

The canonical timed-status API now accepts Haste and Slow. Real CT gain and the
AT list use their source rules; the unit's stored base Speed is never modified.
This is a shared rules milestone, not a retail Haste/Slow spell unlock. The
production catalogue, normalized spell gates and existing artwork are unchanged.

## Source rules and evidence

`tools/haste_slow_oracle.py` runs unchanged original clock/status-check/AT
instructions from the local disc. It records45 gain/selection profiles,4 charge
phases and33 first-six AT lists. Function/disc hashes, inputs and the pinned
decompilation revision are in `x/core-haste-slow/oracle/source.json`.
Expiry/upkeep callbacks are isolated; earlier original lifetime staging evidence
is reused. Do not call this a full original spell or timed-effect dispatcher trace.

|Rule|Actual clock|AT forecast|
|---|---|---|
|Haste|Speed + (Speed >> 1)|Same rounded gain while its predicted duration lasts|
|Slow|Speed >> 1|Same, clamped to a minimum gain of1|
|Both|Slow wins|Slow wins; both durations decrease during the same ticks|
|Base Speed0|No gain|Source clamps base Speed to1 first|
|Dead/ordinary KO|Ignores both tempo modifiers|KO actors omitted|
|Stop/Sleep|CT frozen; mutable timers still expire globally|Initial wait consumes predicted tempo duration|
|Quick255|Consumed to0 on actual selection|Keeps the source55-remainder forecast quirk|

Mutable Haste and Slow cancel one another through the existing source masks.
Repeated mutable application refreshes its source timer; immunity and protected
innate effects retain their existing semantics. Ordinary KO clears mutable
tempo/timers, and revival does not restore them. Applying tempo during a Charge
does not cancel the command. Haste/Slow do not multiply the charge countdown.

Actual Slow at Speed1 can produce zero progress. The bounded M_CLOCK path from
CORE_SCHEDULER handles that case and resumes after expiry without fake turns.
Source timer0 underflow and AT's different timer0 forecast remain unchanged.

## Bounded calculation and native placement

The AT forecast skips intervals of constant gain rather than simulating ticks:
Slow, remaining Haste, then normal Speed. There are at most three such intervals
over the forecast. Stop/Sleep wait is subtracted before these intervals; innate
tempo uses the source512-tick forecasting horizon.

The existing51-entry reciprocal table is enough. Base Speed is0..50; with Haste,
gain51..75 reaches100 from any smaller CT in one or two ticks. That branch avoids
an expanded table, new bank data, a runtime division or software32-bit multiply.
The slower gains keep the existing exact reciprocal product.

AT construction moves from FORMULA6 to the dedicated ORDER0 image. Only resident
callers load it, after impact/status images have returned. ORDER itself performs
no nested image loading. FORMULA no longer owns the obsolete AT entry. ABI36
rejects35; ModCtx remains372 bytes, signature64914. There are17 images, split8/9;
the largest execution zone remains6264. ORDER is1018 bytes; FORMULA shrinks436
to5752. The cached image directory grows4 bytes to68. Core Unit112, State2192,
Work45576, FSV3 and the five heap requests remain unchanged. Code storage and
complete peak RAM/stack residency are separate budgets.

RESULTS and AREA_AI have same-size compiler rearrangements after an unused
inline declaration is added. Isolated controls with the previous rule bodies
reproduce both new images; the other13 existing images change only signature.
Evidence: `x/core-haste-slow/declaration-control.json`. The generated ORDER
assembly contains word reciprocal multiplication, no runtime division or
software32-bit math call, and no nested module loader.

Private43-variable package518582 bytes (+650): resident payload+56, ffbm-436,
ffbn+1030. Raw shards40644/39636; all28 non-code banks and archive attributes
are unchanged. Resident TTPack input64803 grows122 bytes, leaving133 below64936.
Further resident additions require an explicit code/storage budget.

## Scoped verification

`make core-haste-slow-check`:748 PC/ASan/UBSan/68000 assertions, digest30A798F0.
The fixture covers odd/max/zero speeds, cap254, Slow priority, dead/frozen actors,
finite/innate/both tempo forecasts, expiration before CT gain, Stop/Sleep waits,
public apply/refresh/cure/immunity, cancellation, real Slow1 idle/wake, unchanged
Charge timing, KO/revive cleanup and13-actor pure forecasting/expiry.

Measured backend fixture blocks (datasheet cycles, not TiEmu):

|Block|Cycles|
|---|---:|
|13-actor tempo AT, cold ORDER load|39086|
|Same forecast, warm|29724|
|13 tempo expirations, cold FORMULA load|81388|
|One CT-add/select phase,45 profiles|8728 average /8778 maximum|

The scheduler regression still passes263 checks3056BFD2 and both unchanged HUD
hashes. Its13-actor wake block peaks266028; cold timer loading can now follow
ORDER residency, so do not assume the previous warm FORMULA cost. These isolated
blocks do not certify a whole tactical frame, stack or combined residency.

The dependency closure is scheduler/due roots, public status adapters, owned
area CT/restarts, ABI binding/rejection, fresh layouts,12 existing action/response
frame hashes and both pilot save/result flows. `check_status_state.py
--integration-only` checks the affected public gate while reusing unchanged
bitmap-oracle evidence; its unsupported-state control now uses Reraise. No
geometry, Jump, full catalogue or unchanged scalar census is repeated. Native
builds are serial. No live TiEmu, standard playtest, user saves or old gallery
bytes/dates are changed.

Final closure: scheduler263/3056BFD2, deferred418/76F6E4AC, public status58/
E79EDE14, owned area CT353/11D6FCB4 plus5 Orbonne retries/00000230. Parser130/
00000047 and162 exact context checks/7DF38F11 cover34 cold/warm preloads,
three poisoned retries and seven rejected startups including ABI35. Fresh20-field
layouts and12 old action/response frame hashes match. Gariland81/AE9CFD31 and
Orbonne14/2CD70391 retain native-equal pixels, settled saves, no double reward
and trophy-overflow handling.

Final package and closure report: `x/core-haste-slow/verification.json`.
Broader status/incapacitation/response/targeting and AI families, campaign,
formats/editors and full performance/residency gates remain open. Jump stays
deferred. The full core and mass content-production gate are not complete.
