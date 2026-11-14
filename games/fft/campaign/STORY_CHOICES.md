# Story choices, branches and persistent effects

2026-11-13. The standalone scene player now supports two-answer choices,
conditional branches and exactly-once flag/guest effects. The user approved the
compact design after requesting a smaller box, less top padding and the glove
protruding left like combat. It reuses those exact combat-menu offsets and the
existing portrait/dialogue renderer. Matching groups are in `x/story-choices/release/`.
The pilot gains engine support; no fictional recruitment scene is shipped.

## Effect and save contract

Two answers have independent result flags and one completion flag. Markers are
distinct nonzero IDs1..256, separate from an optional condition flag. A clear/
set condition can gate a decision. Once resolved, the saved answer wins even
if the condition subsequently changes or the user requests the other answer.
Contradictory markers reject processing; they are never treated as a refusal.
Chapter authoring must allocate these IDs without colliding with encounter WON,
other decisions, world flags or later chapter progress.

Recruitment currently converts an existing keyed guest; new event-candidate
staging/imports are not silently implemented by this opcode.
Each answer has up to eight effects: set flag, clear flag, recruit persistent
guest, remove persistent guest. All effects are preflighted before changing any
Save byte. A missing guest, full16-person party, duplicate guest/flag operation,
malformed record or reserved marker mutation rejects the whole answer. No flags
or earlier effects are left applied when a later effect fails. A full roster
keeps the decision unresolved; it does not dismiss a character or choose refusal.
Recruitment/removal reuse `guest_roster.h`, preserving all record bytes and the
current-raw marker. Other guest/normal records stay untouched. Refusal effects
are authored explicitly; declining does not inherently remove a guest.

`story_choice_apply` transforms the caller's existing durable Save. It allocates
nothing and needs no second3918-byte transaction buffer. `story_choice_commit`
uses load/store callbacks to persist all effects and markers together in one
record. A reload sees the resolved answer and cannot apply recruitment twice.
This is logical whole-record atomicity, not a power-loss recovery format.

The scene adapters implement read-your-writes: native `scene_save` remains dirty
and visible between decisions, reentry and `scene_progress`, until the runtime
writes its borrowed pointer after teardown. No second Save is allocated. On PC,
a same-directory temporary file plus checked write/close/rename preserves the
old session on failure; the in-memory candidate reloads that intact baseline.
Tests inject failures in all three phases and then retry successfully. Slots
remain under the existing title Save/Continue flow; choices only update the
session. Neither adapter claims power-loss durability.

## Data

FCX1 records are byte-decoded, including unaligned inputs. Offsets are even
relative to the record and bounded by its declared length, allowing a future
shared scene resource pool without retaining another buffer.

| Offset | Field |
|---|---|
|0|`FCX1`|
|4|Declared record length, BE16|
|6|Completion flag, BE16|
|8|Condition flag, BE16;0 means unconditional|
|10|Condition value0/1|
|11|Answer count2|
|12|Two8-byte answer entries|

Each answer entry is result flag2, label offset2, effects offset2, count1,
reserved zero1. Labels are1..31 printable ASCII bytes plus NUL. Each4-byte
effect is opcode1..4, reserved zero, flag/guest ID2. Empty effects use offset0.
Exact labels/effect lists share physical spans in `tools/story_choice.py`.
Effects cannot modify their own completion/result markers. UI pixel-width
constraints belong to the future presentation/authoring integration.

Example authored QA, not a retail Delita recruitment:

```python
dict(done=40, guard=dict(flag=3, value=True), options=[
    dict(flag=41, label='Do not recruit', effects=[]),
    dict(flag=42, label='Recruit', effects=[dict(op='recruit_guest', id=4)])])
```

## Scene operations and shipping

Existing FSC1/FSC2 step rows remain8 bytes. Kind6 is CHOICE: `a` references FCX1,
`b/c` are the first/second answer destinations. Its immediately preceding STORY
supplies the final question, portrait, picture and final actor positions. `arg`
is the forward destination when a guarded record is ineligible (zero when
unconditional). Kind7 is BRANCH: `a` is a flag1..256, `arg` the expected0/1,
`b/c` the mismatch/match destinations. All destinations move forward; the step
count means end. This prevents hidden automatic loops. Resolved choices follow
the saved answer; ineligible ones follow their explicit destination.

Question tables/actors/text spans, printable strings, actor IDs, picture pan,
record lengths, label widths and forward targets are validated before play.
The final question must fit one complete dialogue page; it is never truncated.
Known speakers keep their portraits. Only the menu animates while the choice
is open; actors/camera wait at the question's final positions. A held confirm
cannot answer the newly opened choice. UP/DOWN feedback and rejection notices
appear on the same rendered frame. ESC/SHIFT resets selection/error without
answering or exiting. The whole-scene Skip confirmation walks branches but
stops at the next unresolved eligible choice. It cannot silently select refusal.

Choice records use the existing chapter-wide FSP1 pool with pictures and beats.
Equal records across two different scene sets have one physical span. Shipping
requires `story_choice_format: FCX1`; incompatible core reuse is rejected.
FSV6/3918 and battle ABI91/F8B5/28 images remain. Only native scene program
`ffscore` changes; the affected shared BEATS3 image is byte-identical and all
battle programs/banks are reused. Scene input21949 ->26283 bytes; battle input
61381/headroom3555, zone8842, Work29340 and seven game-block requests99332 stay.
Scene buffers/cache retain their existing set-sized six planes and48 entries.

Core420024 + chapter291789 =711813 transport bytes, key
`668a8d4b8281e62a6087d23389480f73`. Old chapter content is byte-identical; only
its matching manifest changes. These are build/request sizes, not a16KiB
installed free-RAM or peak certificate. Difficult recovery stays deferred.

## Verification and reviewed pictures

The effect backend retains81 PC/ASan/UBSan/68000 checks/FAD68EE4: complete
preflight, refusal/recruit/remove, conditions, conflicting markers, full/missing
guests, record/raw-stat preservation, retry and deferred two-decision lifetime.
Five host suites cover malformed data, pooled records and forward flow.

The actual pooled scene path passes251 checks/1322A29E identically on PC and
ASan/UBSan. It plays real modal keys, skip barriers, held confirmation, full
roster, refusal, recruitment, branch conditions, cold reentry, real filesystem
short-write/close/rename failures and retry. Actual separate title children Save
and Continue the resulting session; another cold scene process skips the
resolved choice without changing any save byte. Final session SHA is identical.
The shared subset passes116 checks/0DC86CDC on actual68000, including queued
Save pointer lifetime and same-process reentry. Peak complete measured frame is
339426 cycles (<360k), including choice entry/render in this bounded fixture.
No claim is made for every future authored question/actor set.

Actual production pixels match the approved prototype: F19D/4580, portable
plane hashes87D5491A/FF057F97. The captures use existing Academy artwork and a
fictional Ramza/Delita question solely for QA; they are not retail story content.
Existing chapel/Academy/title/skip tests pass PC/sanitizers. Package checks retain
42 host/831 C checks; native combat/art evidence is reused. No full TI sweep,
new overlay export or live emulator session.

```sh
rtk proxy make -C games/fft/campaign story-choices-check
rtk proxy tools/pyenv/bin/python games/fft/campaign/check_story_scene.py --native
rtk proxy tools/pyenv/bin/python games/fft/campaign/review_story_choices.py
```

Private evidence: `x/story-choices/verification.json`, `scene/`, `review/`,
`delivery.json`, `beats-reuse.json` and matching `release/` groups. The toolkit
still needs breeding/calendar/egg handling, Bar/rumors/Propositions and the full
Fur Shop loop before campaign extraction. Source-specific character templates,
additional event effects and authored branches must be supplied/tested as content;
these bounded operations are not a port of the original event VM.
