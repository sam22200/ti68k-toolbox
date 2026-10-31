# Native campaign package checks

Latest normal-start/title native reproduction and corrected route: [QA_TITLE_FOLLOWUP.md](QA_TITLE_FOLLOWUP.md). Earlier measurements below retain their pre-title scope.

2026-10-23. Work order: package, remaining battle QA/title, then core rules.
These checks use a private X server, Titanium AMS3.10 and fixture saves. The
user's live emulator, profile and saves are untouched. Title review is still
pending; this is not a final title-enabled release or a full campaign playthrough.

## Archived-program retention

The rebuilt pre-title FCJ5 group is466542bytes, SHA256
`08d0239c7b9ff4fe8a9319135370af86424d0e4c92044cab91d4c9d67805368d`.
Its bounded `fftstory(99)` handoff reaches the playable Gariland command menu
after Orbonne's ending door and the Academy. This diagnostic skips real deployment
and the full source-order introduction. Capture:
`x/package-before-title/tiemu/handoff.png`.

A separate four-program probe identified avoidable retained code:

| Returned program | Free heap, bytes |
|---|---:|
| Before |188034|
| Deployment |190172|
| Battle |189022|
| World map |173492|
| Management |152472|

The map and management were plain archived programs,15615/21105bytes including
transfer headers. The drops15530/21020 match their executable RAM copies. This
is the already documented AMS `NG_execute` behavior: copies remain until the
top-level supervisor returns. The files are still archived; rearchiving them
would not release those temporary copies. The initial increase includes
rewriting/archiving the private RAM save, so before/after totals alone are not
a leak measurement.

## Packed map and management

The Makefile now gives each child entry a1095-byte packed stub, including map,
management and the prepared title recipe. Map's `ffwmcore.89y` is8750bytes and
management's `ffmgcore.89y` is12249bytes. Code banks and child stubs are archived.
No renderer, asset or rule changes are involved; existing verified TTPack support
is reused. Title's packed recipe is prepared but was not hardware-tested here.

The pre-title group with these two replacements is452879bytes (13663 fewer),
SHA256 `dc9ca45bc879c3d43c3cd89be9bb0cbd2c80d650e5d0237ff7afc15311aa04f0`.
The actual hardware check adds only the probe and a private fixture save to
those same programs/banks. `x/package-memory/results.json` records the exact
group hashes, archive flags and results.

`make package-probe` builds `x/package/heapprob.89z` from `probe_package.c`.
With a private seven-unit Gariland-deployment fixture, the interactive sequence is:

1. `fftdep(200)`: DIAMOND, ENTER to confirm the five preplaced actors.
2. `fftbat(261)`: ESC, ESC, UP, ENTER to confirm quitting the command-menu door.
3. `fftmap()`, `fftmng()`, `fftmap()`, `fftmng()`: ESC from each entry screen.
4. `fftbat(261)`: inspect the actual battle, then the same quit sequence.
5. Read the heap report; F5 dismisses it.

Use ordinary short text key presses to type `heapprob()` at HOME; longer holds
are for the game keys. Long text holds caused unreliable typing in this isolated
TiEmu check. Do not count an empty HOME capture as a program run.

**Verified in isolated TiEmu:**

| Returned program | Result | Free heap, bytes |
|---|---:|---:|
| Before |—|187872|
| Deployment |1|190010|
| Battle |0|189000|
| World map |0|187990|
| Management |1|186980|
| World map, repeated |0|186980|
| Management, repeated |1|186980|
| Battle, repeated |0|186980|

The second battle is visibly playable, with Ramza52/52HP and16/16MP. Repeated
visits reuse the small retained stubs: the free heap is unchanged after the
first visit to each child. The first map/management pair retains2020bytes
instead of36550bytes, a34530-byte reduction. This establishes stability for the
measured sequence, not every battle action's peak memory. Captures:
`x/package-memory/packed-second-battle.png`, `packed-heap-report.png`.

## Still required

- User verdict on the title/file screens, followed by the matching complete
  title-enabled bundle and native New Game/Continue/deferred-write check.
- Actual displayed rotation order/duration and ghost-silhouette reproduction
  B06/B07 on the final package. Equal PC/68000 buffers cannot establish these.
- New peak frame/cold-transition costs where changed paths require them;
  no blanket30fps claim and no original-TI-89 release check here.
