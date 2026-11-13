# Named-target and protection objectives

2026-11-13. **Approved and integrated.** User verdict: "ouais valide" for the
large Lato names. The core supports ordinary elimination, one named target,
one protected ally, or both. The pilot retains ordinary elimination; its
historical groups/saves and the live emulator are preserved. This is reusable
engine support, not an extraction of later authored encounters.

## Observed in original instructions

`tools/objective_oracle.py` executes12 HP-script cases and13 generic-importance
outcomes in the original US MIPS code on PC. HP unit lookup is isolated; the
comparison and generic outcome/status routines run unmodified. This is not an
authored event-chain or full action/Reraise-order replay.

`battle_script_check_scenario_condition` condition6 tests a resolved unit's HP
at or below a threshold; threshold0 succeeds at0 HP, fails at1 HP, and a missing
unit fails the condition. Condition5 with threshold1 supplies the complementary
living-unit test. Identity/misc-slot binding itself is outside this fixture.

`battle_action_check_battle_outcome` has separate importance3 handling before
ordinary elimination. It uses **initial** team flags and existence/status
state, not simply current allegiance or an important unit's HP. A named enemy
merely having Dead does not force a generic win while another enemy lives;
the scripted HP condition is distinct. Disabled/permanently lost important
units do decide the generic outcome. Simultaneous important blue/red loss
returns defeat first. Do not implement scripted protection by counting all
allies, or treat a guest flag as an objective on its own.

## Runtime contract

`objective_rules.h` is used by ORDER8's `battle_outcome`. Encounter-slot masks
remain fixed after initial binding, including slot0. At the existing settled
outcome boundaries, mandatory Ramza permanent loss, ordinary allied defeat and
protected HP0/permanent loss win priority over a simultaneous target win.
A target at0 HP or permanently removed wins while unrelated enemies can live.
A living target cannot be satisfied by incapacitating other foes. Protection
alone still requires ordinary enemy elimination. Petrify alone is not the named
HP predicate. Hidden, airborne or off-camera units are not permanent removals;
current allegiance never changes the authored identities.

Outcome checks retain the existing root/response/reward/Mime settlement order.
A temporary KO reversed before that boundary does not end the battle. Reraise
is consumed on the KO actor's later CT pick in this engine: it is not immunity
to an already settled HP0 script condition. This is an explicit native policy,
not a claim that all retail event scripts use the same trigger/priority.

## Metadata, titles and storage

Optional active-chapter `ffgo` is one **FGO1** bank shared across every encounter
route. No descriptor or save migration: FCD5/FSV5 remain. RESOURCES26/op15 binds
it after party/fixed-guest setup, before turn/outcome processing. It validates
spans, alignment, masks/counts, role and the actual target/protected names;
invalid or mismatched data refuses entry. Missing routes keep ordinary goals.
Do not bind a random generic enemy by runtime ordering. The Keane/Delita demo
uses explicit fixture identities at fixed seed61, not authored mission data.
Future targets with random names need fixed authored identities first.

FGO1 is magic4, raw size2, route count2, then16-byte entries:
`route1,reserved1,target2,protect2,strip0_offset2,strip1_offset2,width0_1,width1_1,
 target_name_offset2,protect_name_offset2`. Offsets are positive/even relative
to the entry. The final payload interns equal title strips and NUL-terminated
uppercase identity strings across the whole chapter. Strip records reuse the
existing `x,top,rows,stride + light/dark/mask` format. No glyph work, file lookup,
decompression or whole-bank copy occurs during banner frames. One borrowed
pointer is kept at Work's cold tail; cache capacities and allocations remain.

`tools/objective_bank.py objectives.json ffgo.bin` accepts, for example:

```json
[{"route":"g","target":{"slot":1,"name":"Keane"},
  "protect":{"slot":0,"name":"Delita"}}]
```

Add `ffgo` to the chapter specification. Packaging validates it, checks declared
routes and requires an FGO1-capable core. Core/chapter keys and ABI must match.
The default human pilot ships no unused objective bank. Different objectives,
names, positions and story aftermath remain encounter content.

The generator uses the exact approved Lato Black sampler: cap12, fitting11/10
only when necessary; names that cannot fit are rejected for an explicit large
layout. `DEFEAT KEANE!`/`PROTECT DELITA!` replace the redundant Battle Start
heading. Protection-only adds `DEFEAT ALL FOES!`. Relevant actors pulse. Reveal
length comes from each real strip width, followed by the complete hold time.
Victory/defeat strips and ordinary banner bytes remain once in core.

## Save receipt correction

Repeated result settlement exposed an existing bug: the live bag still held
pre-reward counts, so a second save could remove awarded Potions/Phoenix Downs.
Found/stolen queues could also be replayed. After a successful save request,
settlement now synchronizes the live bag and consumes those queues. Story flags
still gate one-time gil/trophies. Failed writes do not consume the found/stolen queues.
Tests check unchanged defeat saves and byte-identical repeated victory saves.

## Validation and remaining scope

Nineteen policy boundaries and25 original-MIPS conditions/outcomes pass.
PC/ASan/UBSan real action/save tests pass61 checks/0B3C2C52, including temporary
revival before settlement, simultaneous post-impact loss, surviving unrelated
enemies, allegiance/visibility/Petrify, bad identity/spans and repeated receipts.
Four runtime screens exactly match approved pixels:7371/2368/4980/168F.
The pooled two-route bank adds only16 directory bytes:2206 ->2222.

Actual68000 checks cover the changed archive/ORDER/PARTY/SAVE boundary, native
pointer/layout/binding contract and banner rasterization/budget. PC uses the
full `game_update` exit; the native receipt carrier retains Work deliberately
for the repeated-save assertion. No broad combat sweep or TiEmu session.
Final measured layouts, frame costs, package key and evidence are recorded in
HANDOVER.md and `x/objectives/verification.json`. The16KiB RAM/installed-peak
gate remains open; difficult recovery is still deferred.

```sh
rtk proxy make -C games/fft/campaign campaign-objectives-check
rtk proxy tools/pyenv/bin/python games/fft/campaign/check_objective_runtime.py \
  --native --data-dir games/fft/campaign/x/objectives/shipping-data \
  --out games/fft/campaign/x/objectives/shipping-check
rtk proxy tools/pyenv/bin/python games/fft/tools/objective_review.py
```

The original approved comparison stays under `x/objectives/review/`; rejected
small labels stay under `review-small-labels/`. New production comparisons go
to `production-review/`; its four LCD images are checked against the approved
ones. Approval is recorded and only its Decision marker was cleared.

Next: authored guest/chain-with-Save integration, story choices, then the full
remaining prerequisite queue. [Breeding, Bars/rumors/Propositions and the Fur
Shop](CAMPAIGN_SIDE_SYSTEMS.md) remain required before campaign extraction.
