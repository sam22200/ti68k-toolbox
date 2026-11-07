# Shared heights, AI evaluations and combat stabilization

2026-11-06/07. This delivery closes the shared-height and immediate-evaluation
interactions for the current native combat engine. Generic coverage remains
201 actions and 78 reaction/support/movement consumers. It does not expand
the campaign's content or promise an exact PS1 future-turn AI.

The user explicitly excluded mounts from the completion scope. Historical
mount/rider research notes do not define a missing native combat family.

## Shared heights

`target_height.inc` is the common body-height query used by short-weapon
targeting. It resolves the first shared-cell donor before checking that donor's
Dead eligibility. A live original actor cannot borrow an invalid donor;
a crystal donor is not rejected merely for having zero HP. Float, water depth,
Frog/Chicken and the existing shared-cell offset retain their source meanings.
No tactical coordinates or projection conventions change.

The unchanged retail height routine supplies 1,152 controls. They distinguish
original and donor eligibility, crystals, finite/innate status, depth and
movement capabilities. Native fixtures also exercise actual strike preparation
and check that queries preserve RNG and unit state.

## Immediate AI evaluations

Weapon planning includes the other victims of a shared panel: friendly damage
can make an otherwise attractive attack undesirable. Immutable scalar rows are
prepared first; a separate bounded pass builds panel utility. Candidate-local
hit and geometry checks remain in place.

Reflect execution and planning share one pure recipient query. Planning caches
the receiver for each hypothetical origin and reflector in packed nibbles.
Weapon-spell forecasts include the existing conditional proc probability and
the actual receiver's hit/utility. Area planning caches ordinary and returned
effects separately, then routes them at each candidate origin. An already
returned spell does not bounce again at an innate Reflect receiver. Different
reflectors remain distinct effect events even when they share a receiver.

Mutable Frog can be valued as a cure; an innate Frog is not valued as removable.
Planning preserves unit records and does not consume RNG for these forecasts.
The native chooser still uses immediate utility, stable tie order and its
existing conservative action policy. This is not the original's complete
status-duration, reaction-tree or future-CT search.

## Bounded work and storage

AI graph preparation yields every four nodes. Edge and ceiling tables are
prepared before graph rows, using idle search/AI storage. Nothing can search a
partial graph. Weather/capability changes cancel partial work; configuration
lives in `Work`, so an overlay replacement cannot erase it. Synchronous manual
movement retains the same graph. Four 256-node profiles and a mid-build weather
change compare complete and sliced graphs, including forced cold image loads.

Scalar planning resumes inside an origin/target loop. Rejected pairs consume
the work budget, and weapon candidates reuse their previously validated
geometry masks. Area victim preparation handles two units per update and
centre enumeration handles three queries per update. The original visitation
and ranking order are retained.

Idle turn menus no longer evict/reload SCREENS and BEATS without input. This
preserves screen pixels while removing an unnecessary cost on every frame.

Module compilation uses GCC's whole-program elimination. The image entry and
compiler arithmetic wrappers are explicitly externally visible, and the
context-size symbol is retained. The executable zone stays in the program,
never in a heap block. Overlay owners return before the resident loads another.

ABI77 retains the 348-byte context and adds AI_PREP25, for 26 images in the
existing three shards. AI_PREP owns area victim/Math preparation and panel
utility; AREA_AI13 enumerates origins/centres. Existing PLANNER/FORMULA/WEAPON
strike ownership remains unchanged. Unit112, State2224, Save3202 and FSV4 stay
fixed. The execution zone is 8,880 bytes; the three raw shards are
56,638/62,736/55,730 bytes. Actual production raw is 64,795 bytes; TTPack input
is 64,797/64,936, leaving 139 bytes. The private packed stub/data files are
1,095/39,473 bytes. This remains a tight executable budget.

Actual AMS startup initially failed for memory even though the headless runs
passed. Final native Work is 37,460 bytes, down from 48,870. Status and equipment
painting reuse the idle AI arena. Equipment drafts, movement configuration and
deferred requests stay outside that union. Closing Status resets planning and
cancels an incomplete graph; entering equipment cancels incomplete graph work.
The tests cover these transitions, including forced image replacements.

Portrait light/dark/mask data occupy three ten-byte slots at the unchanged
30-byte row pitch. The right portrait's known overhang is cleared before the
cropped opacity is built. HUD black text needs one plane. Rotation scratch
reuses reach-ring backups only after the scene has been replaced. The actor
cache stays at 48 frames and all reviewed screen checksums stay unchanged.

The native graph builder reserves its actual maximum of eight links per lower
node or sixteen with both layers; injected diagnostic graphs retain their
wider capacities. Ordinary Gariland topology shrinks from 5,400 to 3,000 bytes.
Together these changes save 13,810 bytes of runtime residency. The largest
scene+Work allocation is 53,620 bytes, plus its two-byte malloc handle: 53,622,
leaving 11,898 below AMS's 65,520-byte block limit. A resident compile-time guard
enforces that bound. These savings do not reserve room for arbitrary future
map buffers; remeasure residency when changing capacities.

## Headless evidence

The final ABI77 regression runs all 99 suites with `--all --force`: 99 executed,
zero cached, zero failures (1,299 seconds). No production source changes during
that run. The focused carrier below is part of that same run, rather than an
unrelated pre-layout export. `x/core-final-combat/regressions.json` records the
suite summaries; `verification.json` ties them to layout, image and hardware
evidence. Reproduce with:

```sh
rtk proxy tools/pyenv/bin/python games/fft/campaign/closure.py --all --force -j 10
rtk proxy tools/pyenv/bin/python games/fft/campaign/check_flow.py --native --out /tmp/fft-flow-review
```

|Carrier|Checks|Hash|
|---|---:|---|
|Shared heights|5,298|F9631D68|
|Weapon decisions/collateral/Reflect|39|F6F6168E|
|Area Reflect/innate status|103|8B7BE311|
|Sliced graphs/cold owner replacement/aliased-buffer transitions|1,165|D102BBAC|

All 6,605 checks agree on PC, ASan/UBSan and the actual 68000 binary. The
13-actor planner fixture passes 1,784 checks, hash 2E4DFC57; its longest general
quantum is 294,798 cycles and its longest area quantum is 342,716 cycles.
Forced-cold graph slices peak at 209,826 cycles.

Thirteen-actor board, rotation endpoints, last-actor Status and equipment
screens retain checksums 0546/ECF7/A87E/AA26 on both CPUs. Their ordinary frame
peaks are 337,170/337,170/330,278/354,050 cycles, below 360k. Cold setup and
existing low-poly rotations are reported separately: the measured peaks are
2,606,350 and 6,043,522 cycles. Cold movement/impact work can also exceed a
frame. These results are not a claim that every cold transition runs at 30fps.

The private CPU runner records SP high-water without changing native code.
Production-style pilot flows peak at 4,788 bytes of stack and 112,226 bytes of
runtime heap. Board/rotation/Status/equipment tests return that heap to zero.
A settled save is a persistent 3,210-byte VAT allocation, not a leaked battle
buffer. The 256-node fixture exercises navigation-buffer replacement: its
observed heap peak is 128,734 bytes, including a 6,780-byte diagnostic fixture
after the runner has released its 7,680-byte display planes. Restoring those
planes and excluding the fixture gives a conservative 129,634-byte runtime
replacement peak; steady navigation residency is 126,634 bytes. Hardware
grayscale, executable, AMS and parent-program residency must be added
separately. The runner does not model AMS fragmentation or interrupt stack.

An isolated Titanium AMS run uses archived scratch banks and the actual packed
production executable, with a private copy of the OS image and saved state.
The numeric startup probe reports matching signature 078F, Work37460, all banks
present and valid, grayscale active with 15,382 bytes consumed. All six
conservative executable/runtime allocations succeed and the probe returns to
its initial free heap (189,566 bytes). The same 13-actor battle opens twice
under an additional 4,096-byte pressure allocation. Grayscale, keyboard,
rotation and the AT list work. Both battle exits return zero and leave exactly
187,064 bytes free (stable after the first loader initialization, starting at
188,074). This is a repeated-boot check, not a fragmentation endurance test.
The private emulator and X server are stopped after the review.

Gariland81/AE9CFD31 and Orbonne14/2CD70391 retain native rules/pixels and exact
settled FSV4 bytes; repeated receipt and full-stock trophy conversion pass.
Loader checks compare all 26 contexts exactly and reject malformed, missing,
mixed and real previous-ABI76 shards before executing an image.

Private evidence is under `x/core-final-combat/`. Preserve older proof/save
bytes and both timestamps, the standard groups and the user's live TiEmu.

## Scope after this delivery

There is no missing generic action or R/S/M family queue. Exact PS1 ballistic
rounding, future-turn AI and the previously documented native adaptations do
not become new automatic deliveries. More campaign maps, events, objectives,
monster art, editors and full-game encounter certification remain separate
content/tooling work. The historical demo's `make test` has the same 31 compile
errors in the pre-task snapshot and current source; current campaign/core
regressions are checked independently. This delivery does not repair that
retired entry or certify all future maps.
