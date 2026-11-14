# Consecutive battles and intermediate Save checkpoints


Later guest update (2026-11-13): [GUESTS.md](GUESTS.md) supersedes this delivery's
FSV5 lengths and missing-guest section. FSV6 now has four persistent guest slots;
`check_chains.py --guests` checks natural guest progression and identical Save/
Continue snapshots alongside16 ordinary party slots. The authored retail chain
content remains future work. The original checkpoint measurements below are dated.

2026-11-13. The supervisor's existing direct battle sequence is now checked
through real, separate PC battle and title processes. Save, decline and
defeat/Continue resume the right encounter without a world-map detour. The
fixture reuses Gariland's map, art and fixed Delita role twice; it is authored
QA data, **not a retail story chain or newly extracted campaign content**.

## Corrections

`fp_story` used to read `step[1]` after returning from a child. AMS can relocate
that manifest while the child saves/compacts memory, invalidating the borrowed
pointer. The supervisor now copies the flags together with the program/argument
before calling the child. Archive/save pointers are reacquired afterward.
The previous supervisor reproduces a lost Save proposal when its old manifest
view is invalidated; the corrected implementation passes that same control.

The supervisor's old progress reader omitted FSV4, although `save_load_from`
supports it. Shared `save_progress.h` now reads FSV2/3/4/5 with each version's
minimum length and roster bound. Missing, short, wrong-version or invalid-roster
data clears the progress view. FSV2 derives the completed pilot flow like the
existing save migration. No full Save allocation or file pointer is retained.
The save layout and actual slot migration remain FSV5/3204.

The core declaration now includes `checkpoint_version: 1`; matching keys reject
reuse of a core with the previous supervisor. FKP1's format is unchanged.
Packaging requires a battle's `skip_flag` to match its descriptor's `WON`, and
an intermediate battle Save proposal must have that persisted completion flag.
This prevents a checkpoint from replaying a committed fight after Continue.

## Authoring

Use distinct completed-encounter flags and ordered steps; no map step is needed:

```json
"steps": [
  {"program":"battle","argument":200,"skip_flag":8,"save_prompt":true},
  {"program":"battle","argument":300,"skip_flag":9}
]
```

Here the two descriptors must set `WON`8 and9 respectively. The test doors use
216/316 to enable the existing all-AI replay door; shipping encounters use their
normal authored door. Save copies the completed session into the selected slot
through the existing title screen and explicit overwrite confirmation. Decline
continues directly, leaving the slot alone. Defeat does not complete the step,
offer its Save proposal or change the session; Continue reloads the last slot
and skips already committed battles. A declined checkpoint naturally leaves
Continue at the older saved position. Nothing autosaves the user's slots.

## Verification and bounds

`check_chains.py` checks three complete scenarios on PC and ASan/UBSan:

- Win -> Save -> win. The second cold child reads the first child's exact save.
- Win -> decline -> win. The original slot stays byte-identical.
- Win -> Save -> deliberate protected-guest defeat -> Continue -> win.
  The first fight is skipped, no second Save prompt is repeated, and only the
  unfinished fight is replayed. The defeat uses an injected lethal packet at
  the action door; commit/protection/outcome/exit run through production code.

Six full wins each take8553 updates,26 actions and3 guest actions; one injected
defeat takes193 updates. All session, slot and entry/exit snapshot bytes match
between PC and sanitizers. The manifest cache is freed after **every** child,
so ASan also detects a borrowed pointer surviving a program call. Existing
overwrite defaults, input, sprites and fonts are reused without a visual change.

The starting party is full at16; the imposed Delita remains an AI guest,
consumes no deployed player slot and is not silently recruited. Player XP/JP/
levels and reward inventory carry through the checkpoint; eleven undeployed
records remain unchanged. The existing shared protection/display binding
is the already approved FGO1 path. The ordinary pilot sequence is unchanged.

One small actual68000 carrier checks38 progress/relocation boundaries,
FFFFFB80, matching PC/sanitizers. Package checks pass42 host +819 C checks.
No full native fights, overlay rebuild, render-budget sweep or TiEmu session
is needed: `binary-reuse.json` proves the battle executable,28 images, scenes,
menus and all content payloads unchanged. Only `fftstory` grows74 bytes; new
pilot core414169 + chapter291735 =705904 transport bytes,
key89b137c4932f78229aec9a87f0965a26. ABI90/F4D0, Work29314, Save3204, zone8698,
seven requested blocks98448 and battle input61317/headroom3619 remain. The
16KiB installed RAM/peak gate is still open; difficult recovery stays deferred.

```sh
rtk proxy make -C games/fft/campaign campaign-chains-check \
  CHAIN_INPUT=x/chains/release/source.89g
rtk proxy tools/pyenv/bin/python games/fft/campaign/check_chains.py \
  --input games/fft/campaign/x/chains/release/source.89g --native-progress
```

Evidence and normalized fixture packages: `x/chains/verification.json`, logs,
`previous-supervisor-control.log`, `binary-reuse.json`, `fixture-package/` and
the real pilot `release/`. The displayed Save screen capture is under `review/`.
Historical groups, saves and unrelated user files retain bytes and timestamps.

## Historical guest gap (now delivered above)

**Source-code read, not a new retail execution:** `unit.h` defines16 ordinary
party slots plus guest slots16..19 (`PARTY_ROSTER_SLOT_COUNT=20`).
`main_party_find_free_slot`, `main_party_save_unit` and
`require_party_save_battle_units_and_find_join_candidates` retain guest formation
records separately. They do not consume the ordinary16-slot party capacity.

Native FSV5 only stores the ordinary party. Fixed encounter guests are recreated
from their encounter definitions; guest EXP/JP/equipment/story membership is not
persisted by this chain test. Persistent guest IDs/records, bind/save/remove/
recruit rules and migration therefore remain **core prerequisites** before
campaign extraction, together with story choices and the breeding/Bar/Fur Shop
queue. Do not mark the whole guest requirement done from a correct TEAM_GUEST
flag. Keep the source's separate capacity and share the existing stat/identity
readers rather than stealing normal party slots or duplicating character art.
