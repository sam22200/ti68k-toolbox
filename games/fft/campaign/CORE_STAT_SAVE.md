# PA Save, MA Save and Speed Save reactions

2026-11-03. Three equipped reactions now execute through the shared action
pipeline. These are source reaction IDs 422..424, not ordinary commands. Generic
action coverage remains 189/201: only the twelve user-deferred Jump entries are
still disabled. Other reaction, support, movement and production gates remain open.

## Source behavior

`tools/stat_save_oracle.py` runs unmodified retail MIPS for 36 post-impact
finalizer profiles, 18 reaction effects and three self-target lists. Function,
disc and decompilation hashes and isolated external boundaries are recorded in
`x/core-stat-save/oracle/source.json`. This is bounded function evidence, not
an original whole-menu, AI or secondary-animation replay.

The original checks these reactions after HP and statuses settle. A successful
HP-damage event may react even at zero damage. Misses, healing, KO and secondary
contexts do not stage them. Sleep can wake before the check; Stop, Don't Act,
Frog and the existing prevention policy reject the relevant stage. One Brave
roll queues the equipped source ID. Existing original Brave-threshold evidence
from CORE_REACTIONS is reused.

The source reaction table selects the reacting actor itself. PA and MA gain
one, capped at 99; Speed gains one, capped at 50. A capped response still occurs.
The native model retains its existing single resolved value per stat rather
than the original separate base/equipment/effective copies. Stat changes are
battle effects; this does not add permanent raw-stat growth.

## Shared execution and storage

The existing post-HP-damage offer now recognizes the three equipped IDs alongside
Auto-Potion. They retain the shared slot-order queue, live revalidation and
root exclusion. Consecutive primary strikes settle before pending responses.
The new self response suspends/restores the same primary packet, applies its
stat change once, and cannot enqueue another reaction. It spends no stock or
MP and receives no EXP/JP. CT, independently armed Charge and turn credits
remain live. Speed changes refresh the AT order; primary rewards and terminal
checks still wait for the complete response drain.

The catalogue stays byte-identical FCJD: no new normalized command, record,
lookup table or learned-ID representation. Direct use of the three reaction
IDs remain rejected. Invalid Math criteria16/253/254/255 also reject before
reaching the shared query callback, preserving State and RNG. FSV3 already
stores equipped reaction IDs; no new save,
Unit, State, Work or pending-slot fields are added. Native Unit112, State2210,
Work45576, Save3138 and SaveUnit176, plus all five allocation formulas, compare
exactly with the Math checkpoint.

ABI58 uses a 360-byte ModCtx. Two unused evaluator callbacks are removed:
active images already use their local evaluator. Production drops the unused
resident checker wrappers as well. Pure `target_query(who,254)` now shares
reaction eligibility across effect owners without loading an image; 255 remains
ordinary target validity and 0..15 remain Math masks. The old deep-water guard's
byte2/0x0e test is redundant after PREVENT_REACTION and is omitted with identical
behavior.

AREA_UI14 now owns scalar/area impact numbers and wind-up glyph preparation;
NUMBERS15 keeps forecasts. Existing labels, plane composition, fonts and placement
are reused. Direct checker dispatches move with the renderer. No executing image
loads another image. The eighteen images keep the 9+9 archive split:

```
8242 4510 7398 7236 7674 8238 6536 6168 7560
7988 8218 5442 7614 8254 7724 5262 5184 7572
```

The execution zone remains 8254 bytes. Banks shrink 63990/63888 to
63684/63380 bytes, leaving 316/620 below their unchanged 64000-byte limits.
Production raw variable size is 64909; actual TTPack input is 64911, including
its two-byte prefix. The private packed build succeeds, leaving only 25 bytes
below 64936. Its compressed data file is 40106 bytes. More resident room must
be recovered before adding further core code.

## Verification

`core-stat-save-check` passes 697 logic assertions 1A5C07DC and 33 UI assertions
235C9F0E on PC, ASan/UBSan and actual 68000 execution. Nine full-plane samples
cover preparation, impact and restored primary states for each reaction.
Independent fixtures reset chapter Work, frame the actors and warm presentation
without advancing state. The three impact captures were inspected at LCD scale.
Source staging/clamps, duplicate impact, revalidation, Brave0, preserved Charge,
no reaction rewards, direct-command rejection and two queued area victims pass.

Thirty scoped older suites pass, with seven final direct replays after the
reserved-query boundary fix. Area impact retains1012 assertions/12D9C238 and64
samples/F9BACFF5; Math retains1360/9163135B plus238/138E34BC, and Throw's UI keeps
38B87D4D. Both private chapter flows retain81/AE9CFD31 and14/2CD70391. The actual
previous ABI57 export rejects before binding. All523 protected save/group files
retain bytes and modification dates;117 of122 data files remain identical, with
only the two code-bank pairs and generated module header changing.

Native secondary preparation peaks at 41620 cycles; the marked impact peaks at
100226. These are function zones, not complete frames. Cold render peaks still
exceed 360k; full-frame, stack, residency and release certification remain open.
The generated ACTION image matches production exactly; the stat increment is
a byte operation without long multiply/divide or floating-point helpers.

Final scoped regressions, previous-export rejection, private chapter flows,
unchanged approved pixel hashes, code generation and save/proof preservation
are recorded in `x/core-stat-save/verification.json`. The last native area-impact
carrier omits timing markers to fit its variable; all assertions, frame samples
and historical digests remain required. The other carriers retain their zones.
Tests use private PC directories and native VAT paths. Live TiEmu and the standard
campaign group are untouched; nothing is committed or pushed.
