# Propositions: source study and worker-picker review

## Current state (2026-11-23)

**Playable and durable**, in the actual management program through
`proposition_ui.inc`. Accepted A uses the existing Bar room, real unit sheet,
separate Team list, common grey prose reader/dialogue/choice/War Funds and
animated glove. Dispatch, duration, fee confirmation, away-unit exclusion,
completed-route-leg days, recall, saved frozen reports and origin-town
one-shot settlement all work. Chapter I keeps the original lock; source075
opens Propositions in Chapter II. Private flags33/34 are test fixtures only.

FSV9/4684 appends402-byte state,256-byte separate Fur stock and2 generic-identity
bytes without moving old offsets; FSV2..8 migrate. Eligible candidates use
explicit source identity metadata, not human type alone. Office recruits mark
it; legacy migration certifies the current pilot models/Office records only.
Future authored identities must use `save_generic_source`. Away units cannot
mutate or deploy. Manual roster edits are blocked during a pending receipt;
battle compaction remaps its participants/speaker without changing its rewards.

Core banks: FPR1/ffpr4079, FPE1/ffpe437, FPT1/ffpt16148 (96 descriptions and77
reward names). Chapter ffsv/FSV1 is8 bytes and binds source events to native
story flags; the pilot has no unlock bindings. Reuse source stock-tier flags
and the shared zodiac/calendar. Missing progress never invents later content.
PC writes are atomic whole-session replacements with rollback. Native saves
commit after teardown; supervisor reentry reloads before displaying a result.

Current evidence, package keys and RAM limits are in [HANDOVER.md](HANDOVER.md),
`x/town-complete/playable-results.json` and `native-check/results.json` there.
The historical private-backend/review notes below preserve their original
metrics and do not describe remaining implementation work. Prior proofs and
installation groups remain unchanged.

The user selected **A** on2026-11-22 in
`x/propositions-review/propositions-options.png`:

- A: original Bar room, shared unit sheet and separate selected-team list.
- B: Formation background, the same unit sheet, jobs and selection marks.

Both share the mission detail and final confirmation. A is the implementation
target; B remains a historical comparison. Acceptance is tied to the reviewed
image in `x/propositions-review/approval.json`; its gallery decision marker is
cleared. Preserve the accepted town portraits C/D/C/D and Bar reader A. Do not
ask again about the worker-picker layout.

`preview_propositions.c` includes the actual management implementation and
calls `plate_unit`, using real snapshots, stats and portraits. The room and
owner117/D are borrowed from FBR1. `town_reader`, `dlg_town_says`, choice panels,
scroll marks, animated glove and War Funds are shared components. There is no
second unit-card renderer. The four candidate slots and two selections are
explicit review fixtures, not a production eligibility implementation.

Reproduce from the repository root:

```sh
rtk proxy tools/pyenv/bin/python games/fft/campaign/review_propositions.py
```

Twenty-two PC/ASan/UBSan pictures agree, including glove animation and its
32-frame return. Save memory and the private save file stay unchanged;459 prior
bank/save/review files retain hashes and mtimes. This is a PC visual review,
not a native dispatch or performance certification. Shared helper factoring
separately preserves all nine pilot Bar pictures and seven full-rumor pictures;
one selected native long-title reader case also agrees. That new evidence is
in `x/propositions-review/shared-ui-check/`, without rewriting older proofs.

## Bounded original sample

`tools/proposition_source.py` reads original record30, **Testimony of Ex-miner**,
at Gariland (native location6, source7):600gil,8..11days, required stock tier5.
The decoded23-byte record and hashes of WLDCORE/WORLD/WLDMES are retained in
`x/propositions-review/source.json`. It is a Chapter II design fixture, not a
newly enabled Chapter I job.

The table pointer at0x8009D340 resolves to0x8009D344 in WLDCORE; its first table
contains96 records of23 bytes and its second contains the fee lookup. The title
comes from WORLD section26. The prose comes from WLDMES block1, offset0xCC63;
only its original Circle-button footer is replaced by native controls. The
original question mark and attribution remain. WORLD section31 has a different
rich instruction string; do not describe the short title as that prompt.

## Portable rules, dispatch and day backend delivered

`tools/proposition_rules.py` builds a4079-byte private FPR1 bank from all96
original23-byte rule rows and short titles. It pools exact titles and stores
the ten fees once; descriptions/art remain outside this rules-only catalogue.
`proposition_rules.h` reads it in place through bytes (unaligned input and the
native DAT trailer are accepted), with cold bounds/encoding/reference checks.
No shipping package ownership or active data file is changed yet.

The217-byte `PropositionJobs` backend retains eight nine-byte jobs,48 packed
status bytes and96 retry bytes. It provides location/stock/zodiac/prerequisite
eligibility; atomic dispatch with one to three distinct eligible free workers;
final-only payment; daily progress; recall on the next day; origin-town report
readiness; and roster-compaction remapping/rejection of away-unit removal.
Returned workers remain absent until settlement. Eligibility is an explicit
16-bit caller mask: this backend does not silently equate job/type/palette
with source generic sprite identity. It cold-validates loaded records, duplicate
workers, flags, durations, free records and retry/status consistency.

This is private backend state, **not a new campaign save version**. A private
4241-byte test snapshot round-trips the complete unchanged4024-byte Save prefix
plus217 job bytes, including funds/profiles/world RNG and returned-worker state.
The final append/migration must also cover settlement and collection dates;
217 is not a final whole-save or companion-RAM growth figure.

Reproduce PC/sanitizer checks with `make -C games/fft/campaign proposition-rules-test`.
Evidence: `x/propositions-rules/`.5018 assertions pass. Four actual original
functions are compiled unchanged through a narrow host shim:4704 availability
decisions across96 rows/all43 town IDs,96 fees,36 sign boundaries against the
existing breeding/calendar sign owner, and12 daily/recall updates agree. These
are source executable checks, not a complete retail UI/reward replay.

One selected native backend executable agrees on153 bank/dispatch/day/state
boundaries, digest99BD7737, state217; no emulator or broad TI sweep. Its input
hashes and `native-results.json` retain the proof for PC-only reruns. No changed
shipping Save/ABI, installed groups, actor caches or active banks. Protected
proof/save/bank files retain bytes AND dates.

## Frozen reports and settlement delivered (private backend)

`proposition_rewards.h` now computes scores, base JP/gil, discoveries/random
bonuses, extra-day percentages and the source-selected reporting unit. A24-byte
owned report freezes participant IDs/careers, outcome, bonus and exact payouts
before display. Reopening it consumes no new random draws. Collection accepts
only this stored report, checks its original town/job, caps gil99,999,999 and
JP/tJP9999, records completion/discovery dates, starts failure retry counters,
releases the workers and compacts the active jobs. The consumed report is cleared;
another confirmation or reload cannot collect it again. Returned workers remain
absent before collection. Removal is blocked while a report is pending.

`PropositionState` is402 bytes: the preceding217-byte job state,108 packed
completion-date bytes,35 treasure-date bytes,18 land-date bytes and24 receipt
bytes. A zero nine-bit date means unowned; all real month/day records are
validated, including dates crossing byte boundaries. This is still private
state, not an active FSV9 or a final native companion-size measurement.

The437-byte private FPE1 bank stores universal scoring/reward tables, planned
for core ownership alongside FPR1. It preserves a source layout detail: level
scoring reaches into the neighboring Faith bytes. The generator keeps that
observed31-byte span and aliases the Faith subspan at+16, avoiding a second
physical copy. Unused zero random-reward pairs remain legal, but live rule
references may not select them. All archive readers use bytes, not casts.

**Explicit native adaptation:** the original preferred-job fallback reads past
its participant list. The native rule scans the assigned workers safely and
retains the actual original `<61` percentage threshold, rather than silently
claiming equivalence to an undefined/stale-slot read. This exception is tested
separately at the last accepted/first rejected draw. The other reward pipeline
checks compile the original functions unchanged with that fallback disabled
and exclude cases where it could contribute. No complete retail report replay
is claimed. Caller integration must prevent job changes/dismissal while away;
the frozen receipt retains the career that earned each participant's JP.

`proposition_transactions.h` supplies save-aware preparation/collection. Its
caller provides an atomic whole-session writer whose failure leaves the old
file intact. A failed first preparation restores the empty receipt and world
RNG; failed collection restores jobs, flags, dates, funds and JP with a119-byte
undo record, without copying another4KB Save or allocating a heap buffer. A
successful cached preparation performs no write. Collection performs no draw.
The integration writer must satisfy that failure contract on both PC and TI;
do not assume the current native variable writer is atomic merely because the
PC temp/rename test is. Audit its replacement/failure path when adding FSV state.

`make proposition-rewards-test` passes1056 PC/ASan/UBSan assertions,181 original
reward pipelines plus random-tier and preferred-job boundaries. Tests include
multiple jobs, recall, full discovery collections, byte-packed dates, limits,
wrong-town receipt, repeated confirm, and actual private-file temp/rename
failure before replacement. The4426-byte private snapshot preserves the full
FSV8 prefix plus state across reload both before and after collection. It is
not written as `ffsave` and does not replace the campaign's current save loader.

ONE selected native executable agrees on244 arithmetic/receipt/settlement/
rollback boundaries, digest54D3CF1E, state402. Existing native dispatch/day proof
is reused, not rerun. Evidence is isolated in `x/propositions-rewards/`;563 old
bank/save/review/proof files keep bytes AND mtimes. No emulator, package rebuild,
ABI/save-version change or actor-cache reduction. Native evidence is pinned by
runtime/fixture hashes and retained during ordinary PC-only checks.

## Rules observed in source

These findings come from the local original disc and
`sources/fft_decomp/src/wldcore/`, not a new complete retail gameplay replay.

| Boundary | Observed behavior and source |
|---|---|
| Candidates | `wldcore_proposition_load_send_unit_candidates` scans the first16 roster slots, excluding absent entries, guests, monsters, special sprite identities and nonzero proposition status. Selected candidates remain visible with flag8. Do not use `SaveUnit.type < 2` as an unreviewed substitute for the sprite-identity restriction when named humans are added. |
| Team | One to three participants. Candidate selection can undo a mark and replace a selection at capacity. |
| Payment | `wldcore_proposition_handle_accept_confirm_input` checks workers and funds but does not pay. Only final Yes in `wldcore_proposition_handle_dispatch_confirm_input` deducts the fee and commits participants, location and assigned duration. |
| Active jobs | The original has eight nine-byte records (`main.h`), with proposition ID, flags, elapsed/assigned days, town and up to three participant indices. Dispatch marks the job attempted/in progress and each participant absent. |
| Daily travel | `wldcore_proposition_advance_daily_counters` increments elapsed days once, marks returned at the chosen duration and stops advancing returned jobs. Existing packed retry countdowns decrease independently. Reuse the existing completed-route-leg day owner; animation and repeated input must not advance time. |
| Recall | `wldcore_proposition_handle_cancel_confirm_input` sets recalled; the next daily update returns the group. Recall is not immediate collection or a fee refund. |
| Collection | `wldcore_list_open_completed_propositions` filters returned records by their dispatch town. Reaching the duration alone does not release participants. |
| Settlement | `wldcore_proposition_step_result_level` commits gil (cap99,999,999), participant JP, completion/collection records and dates, releases units and compacts the active-job array. It clears in-progress and sets completed on success. Failures get32/16-day retry counters with0x40/0x80 severity. |

The source has a separate bulk `wldcore_advance_calendar_days` path whose
incomplete-duration behavior differs from repeated daily increments. The current
native route clock advances one day per completed leg; preserve that observed
daily path rather than silently generalizing the bulk helper.

Availability and reward/date/retry mechanisms now have the bounded executable
fixtures above. Source `wldcore_proposition_determine_success`'s leftover-index
fallback is the explicit safe native adaptation described above. Full campaign
save/UI/service flows remain separate from these private backend proofs.

## Remaining campaign work

The reusable service integration is delivered. Do not rerun the accepted
proposal generator or repeat the source catalogue/reward census. Bind the
actual Chapter II event075 when that content is authored; retain the Chapter I
lock. Full chapter content, route automation, assembled archive budgets and
release-time installed RAM/hardware checks remain, as listed in
[CHAPTER1_READINESS.md](CHAPTER1_READINESS.md). The16KiB target is not certified;
measured caches remain intact.
