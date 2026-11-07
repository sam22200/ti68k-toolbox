# Equal-tick deferred dispatch and the next casting boundary

Follow-up2026-10-28: CORE_SCHEDULER.md supersedes the historical Quick/liveness
limitations below. Quick priority/CT consumption, bounded frozen-clock resumes
and quit/cancel during idle wait now pass263 PC/sanitizer/68000 checks3056BFD2.
Retail producers and broader status/action families remain gated.

2026-10-26. Follow-up to CORE_CHARGE_HANDS.md. Due Archer Charges now drain
without recursive turn entry. This closes the tested Charge-wave dispatch
boundary, not the complete magic/AoE scheduler or the full L2 gate.

## Source observations

`tools/deferred_oracle.py` executes unchanged local-disc MIPS for six clock
sequences and four attacker-coordinate cases. It records original function,
disc and decompilation hashes in `x/core-deferred/oracle/source.json`.

The clock and status eligibility execute unchanged; expiry/upkeep callbacks
are isolated, and cancellation/Quick changes are injected between returns.
The cases establish:

- Already-ready ordinary turns precede the next charge decrement phase.
- Due eligible charges return in ascending unit-slot order, including thirteen
  simultaneous records. Each selected record is consumed before returning.
- Stop preserves its countdown. Removing a later due command between returns
  skips it. All remaining due charges precede expiry callbacks and the next CT
  addition; there is no extra decrement between those returns.
- Original CT255/Quick can interrupt a charge wave. This is research evidence
  only: Quick is not an enabled native producer and this priority is still open.

The original attacker-coordinate routine reads live coordinates for target-kind6
(unit) and stored coordinates for kind5 (tile). Moving the selected unit changes
only the former. These four cases do not establish absence/KO, layer, area,
MP, interrupted casting, full target-list rebuilding or presentation behavior.

## Native dispatch

`charge_resolve` now returns whether presentation is owned by an action. It
rejects a poll while a root/impact/response is live, before touching pending
records, CT, RNG or request fields. A due empty/KO/forbidden target consumes its
record and returns zero. `turn_start` then iterates through the same clock phase
instead of being called recursively from the dispatcher. Each rejected record
is consumed once; at most thirteen records can drain in one unchanged wave.
A successful launch yields until both hands, reactions and rewards finish.

There is no new event queue, allocation or State/Work/save field. The existing
thirteen records are the bounded storage. A discarded slot does not advance
combat time, consume RNG/MP/items or grant rewards. Invalidated later casters
are rechecked after the preceding impact, not snapshotted into a stale queue.
Terminal battle handling retains priority over another pending launch.

This bound applies to draining due records. It is not a liveness certificate for
the complete CT loop with no eligible actor/all permanent freezes. That case,
Quick interrupts, unit-following casts, area target sets and recurring actions
remain explicit scheduler work; no unsupported producers are enabled here.

## Target identity contract for magic

Keep targeting separate from execution/interruption policy. Native Charge
continues to store a compact tile, including valid tile255; there is no new magic
producer or on-device target-kind field in this milestone. The next casting
entry must store an explicit tile-versus-unit tag and stable identity, then
resolve a unit's live compact cell at due time. It must not retarget a missing
unit to whichever actor occupies its former tile. A recycled actor slot must
not silently become the old target: preserve identity or invalidate the binding.
Target validity and the effect
kind must decide how KO/absent/airborne units are treated.

Do not inherit Archer's movement/weapon-change cancellation or movement cost for
every spell. Check original MP payment, caster interruption, AoE membership and
equal-tick ordering before enabling Fire or another representative spell.
DEFER_AIRBORNE/PERFORMING remain reserved and Jump stays low priority.

## Evidence and budget

`check_deferred.py` exercises actual dispatch,13 roots in one phase,13 empty
records, busy-poll whole-State/RNG purity, one expiry/CT advance after the wave,
KO/displacement/weapon invalidation of a later caster, stopped countdowns,
already-ready turns, Counter presentation ownership and terminal victory.
PC, address/undefined-behavior sanitizers and the actual68000 binary are compared.
Direct regressions cover dual Charge, control/CT and existing single Charge;
one Orbonne/Gariland native flow checks the turn-entry integration. Unchanged
geometry/catalogue/timer arithmetic and previous action pictures keep their
evidence; there is no full census or TiEmu input.

ABI18/twelve images and all types remain unchanged. The private42-variable
group is490379 bytes (+8). Battle TTPack input64857 leaves79 bytes of headroom;
the packed battle payload is42260. The code-image bank and all28 non-code banks
are unchanged. Native State2124, Unit112, Work45516, Save3138/SaveUnit176 and the
five requested heap blocks retain CORE_CHARGE_HANDS.md's layout evidence. These
are allocation formulas, not new stack/fragmentation/full-residency measurements.
Further resident additions still need a module budget.

Final scoped results: `x/core-deferred/verification.json`.

```sh
make -C games/fft/campaign core-deferred-check
```

## Final scoped results

| Check | Result |
|---|---|
| Due-wave dispatch |418 PC/sanitizer/68000 assertions, `76F6E4AC`|
| Dual Charge regression |586 assertions, unchanged `0F008472`|
| Control/CT regression |594 assertions, unchanged `15C96953`|
| Existing Charge |unchanged PC/68000 `276A2090`|
| Gariland/Orbonne integration |unchanged `AE9CFD31` / `2CD70391`; native save equal|

All eleven integration PNGs retain content and modification dates. Only ffbcore
changes in the private group; code-image/data banks and the standard466045-byte
playtest group are unchanged. ABI18 remains compatible; no loader, layout or
format change needs another census or a fresh size probe. Previous normal/
response frame and native allocation certificates are explicitly reused.

The twelve transitions to another due root average73851 cycles, peak76260.
The complete thirteen-empty-record dispatch plus next ordinary turn is126714
cycles in its scoped test. Both exclude drawing/grayscale and are datasheet CPU
counts, not full-frame/hardware FPS or measured stack high-water. The source
change removes the dispatcher's call back into turn_start; it does not claim
that every unrelated battle path is nonrecursive.
