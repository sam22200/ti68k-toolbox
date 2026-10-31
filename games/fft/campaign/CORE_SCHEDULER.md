# Quick priority and bounded idle clocks

Follow-up2026-10-28: CORE_HASTE_SLOW.md adds actual tempo gain and finite/innate
AT forecasts. AT is now ORDER0 (ABI36/17 images), not FORMULA6. Real Slow1 gains0
and reuses the idle/wake path below. Original Quick/HUD regressions still pass;
earlier sizes/ABI and timer-warm costs below are historical checkpoint values.

This closes the Quick255 ordering and permanent-freeze liveness gaps from
CORE_DEFERRED.md. It does not enable a retail Quick spell, Haste/Slow, performing
actions, Mime or dedicated Lancer Jump. The full core/content gate remains open.

## Original evidence

`tools/scheduler_oracle.py` executes unchanged local-disc MIPS instructions for
the clock, status eligibility and AT list. The disc and function hashes and the
pinned decompilation revision are recorded in
`x/core-scheduler/oracle/source.json`. Expiry/upkeep callbacks are isolated;
CT255 and completed-turn resume state9 are injected fixture inputs. These are
scheduler observations, not a full original Quick effect/animation trace.

- Ordinary selection takes the highest nonfrozen CT above99, with stable slot
  ties. CT255 consumes to0; CT100..254 consumes its remainder modulo100.
- After a charge, original phase9 accepts only CT255 before another due root.
  A Quick turn resumes the same wave; ordinary ready actors wait until the
  remaining charges and status-expiry phase have finished.
- The original clock yields eventNONE0xff00 after1201 internal phases when
  permanently stopped/asleep or unable to gain CT. It preserves its phase.
- Original AT forecasting clamps Speed0 to1 even though the real clock gains0.
  It also forecasts a55 remainder for CT255, while actual selection consumes0.
  Preserve that source discrepancy instead of changing the forecast silently.

Six selection cases, two Quick turns between two charge roots, three stalled
clock cases and three AT lists are checked. The AT oracle records8 entries;
the native HUD compares its supported first6.

## Native contract

`ct_next` returns a real actor0..12, CT_DUE254 or CT_WAIT255. Neither control
result may be stored in `st.act`. Compact tile255 remains a valid tile ID;
these clock results belong to a separate interface.

`open_ct_phase` retains charge-wave ownership across Quick turns. Root impact,
secondary reactions and rewards still settle before another turn/root opens.
There is no repeated charge decrement or status expiry when resuming a wave.

Idle searches yield after8 logical ticks. Ordinary living actors retain the
existing synchronous search, capped at128 ticks. Once M_CLOCK is entered, a
waking clock uses at most16 ticks per update. These are TI scheduling quanta,
not an attempt to duplicate the original's1201-phase work budget. Bounds also
cover stale off-field status-clock bits, Speed0, KO guests and absent actors.

M_CLOCK retains the last valid actor without granting a turn, rounds, end-turn
credits or commands. A later update resumes the clock. Rotation and the shared
Quit confirmation remain available; No/cancel restores M_CLOCK, and timers do
not advance while the dialog is open. An already terminal all-KO formation ends
before another clock search. Other outcome/status families are unchanged.

The wait mode uses the existing browse HUD, including an inspected second unit.
Two nonempty HUD plane hashes match browse exactly: EECC35C3 and4E934B27.
No renderer/art/layout candidate or new asset bank is introduced.

## Verification and boundaries

`make core-scheduler-check` runs263 PC/ASan/UBSan/68000 assertions, digest
3056BFD2. It covers Quick consumption/slot ties/frozen actors, a real two-root
wave with two intervening turns, ordinary ready actors waiting, permanent and
mutable Stop/Sleep, timer0 underflow through255 and eventual wake, Speed0,
KO guests, all absent/all KO, rotation, quit/cancel and paused timers. Native
and PC compare both actual HUD hashes and the original first6 AT entries.

Measured native fixture blocks: permanent freeze45178 cycles, stale off-field
expiry bit59322, near-ready wake39940,13-unit timer-underflow/wake quantum
max256892. The first synchronous prototype peaked914938 on the final wake;
the16-tick resume cap removes that spike. These are backend/turn-entry blocks,
not a full tactical-frame, stack or combined-residency certificate.

The affected closure is deferred roots/reactions, command controls, owned area
CT/restarts, AI pacing, ABI bindings/rejection, unchanged action frames, fresh
layouts and the two pilot save/result flows. Reuse unchanged scalar, bitmap,
geometry, Jump and catalogue evidence. Native builds run serially. No live
TiEmu, standard playtest group, user save or old gallery bytes/dates are changed.

Package, layout and closure numbers are recorded in
`x/core-scheduler/verification.json`. The ABI advances to35 for the new shared
mode/scheduler semantics; ModCtx remains372 bytes, signature64657. There are
still16 images, split8/8, largest6264. FORMULA grows14 bytes for AT Speed0.
TURN has same-size compiler changes and AI_SETUP shrinks6 bytes after enum
declarations change. Isolated declaration-only controls reproduce both images
exactly; thirteen images change their signature only. See
`x/core-scheduler/declaration-control.json`. Core State, Work, Unit, FSV3 and
the five heap requests remain unchanged.

The private43-variable group is517932 bytes (+195): ffbcore+187, ffbm+8 and
same-size ffbn with new signatures/token. All28 non-code banks and archive
attributes are unchanged. Raw shards are41080/38606. Resident TTPack input64681
leaves255 bytes below64936: further resident additions still need an explicit
cold-code/storage budget. Archive size is not peak RAM.
