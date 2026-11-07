# Equipment and progression — delivery block B

Implemented source IDs:454..461,463,464,476,480,493..496,439 (seventeen).
Source names, category/weapon masks and support innates come from the local
retail disc. Source457 is labelled Equip Knife but grants the katana category.
It does not grant ordinary knives; retain the original label and behavior.

## Shared equipment

FCJF appends2048 archived bytes:160 job category/support records and256
item type/slot/weapon-flag records. Management and battle use the same category
and hand policy, including sex-specific items, forced two-handed weapons and
Two Swords. The existing native hand layout keeps shields in the left slot;
right-hand shield layouts are rejected. Existing bounded weapon formulas
remain an integration limit; an Equip permission does not certify every
weapon proc, axe/gun formula or sprite. Changing jobs or removing an Equip support validates all slots.
The save remains FSV3/3138 bytes; empty save slots0 convert to battle255.

Equip Change appears in Act for the equipped/innate support. Its existing
choice frame presents a five-slot draft and Done. Cancelling leaves live gear,
inventory, HP/MP, action history and RNG intact. Done validates the complete
loadout and net stock before a single commit, then spends Act without EXP/JP.
A held copy can move between hands with no spare stock. Insufficient stock,
invalid hands, incapacity and an overflowing returned stack reject atomically.
Max HP/MP and affinities rebuild; current HP/MP are preserved and capped.
A pending weapon charge is cancelled after acceptance. AI does not optimize
loadout changes; human/manual Auto-battle units can use the command.

Two Hands doubles compatible ordinary-Attack/Charge XA only with the opposite
hand empty. Forced two-handed weapons get no extra multiplier. Jump and
unrelated skill formulas do not inherit this stage. The ordinary physical
stage now applies the existing Attack Up, bare-hand Martial Arts and target
Defense Up in source order, shared by forecasts, AI, execution and Counter.

## Progression and movement

Gained Exp Up doubles the staged action EXP after the lower bound and before
99; Gained JP Up multiplies the actor's ordinary JP by3/2. Spillover retains
one quarter of the unboosted, pre-reward JP. Deferred reaction drains do not
multiply rewards a second time.

Move HP/MP Up restore ceil(max/10). Move-Get Exp/JP use path edge count;
source special travel uses one bounded draw. Source support boosts double
these movement awards (JP differs from ordinary action JP). Movement benefits
settle at walking completion, including AI movement. Their short existing
pickup message names the resource and amount, and prevents undo/farming.
Crystal/chest presentation keeps priority on a combined landing. No source
EXP/JP spillover or extra action reward is manufactured from a movement award.

Gilgame Heart uses the common post-HP-damage Brave trigger. Each pending
victim freezes the source's nominal damage (positive HP damage capped999),
then a typed self response adds funds for its current blue camp, capped99999999.
A secondary does not recurse or earn rewards; Wall and late incapacity reuse
the common response filters. MP/healing/misses/Golem do not create HP-damage
responses. No files are written until the existing victory/save flow.
That writer now retains all256 live inventory counts when it refreshes story
flags/roster, and copies surviving deployed units' current equipment. Previously
it retained only the ten Draw Out katana counts and funds. A dedicated private
FSV3 round trip checks changed gear, returned/consumed stock, EXP/JP, gil and
repeat-write idempotence against actual native VAT bytes.

## Overlay ownership

EQUIPMENT19 owns the cold inventory UI, transactions, movement benefits and
stat rebuilds. ACTION/RESPONSE enqueue a13-bit Work rebuild mask at level up;
the resident drains it only after the executing image returns. No overlay
loads another executing overlay. Unit and State/save layouts stay fixed;
Work adds one halfword. ABI65 rejects old images; context remains360 bytes.
The module zone stays within the former8640-byte ceiling.
Equip Change reuses the idle Status planes. The measured drawing peak is293060 cycles (first-frame cache construction),
steady construction101868; the exercised command-update peak is302986.
These are bounded UI zones, not tactical whole-frame certification.
The frame/selected row appears
immediately; two remaining rows are cached per later render. Returning to
tactics invalidates the aliased portrait caches. No new allocation is needed.

## Verification

Evidence and exact current sizes: `x/core-equipment/verification.json`.
The retail oracle executes unchanged MIPS category, physical, EXP, JP and
movement/reaction preparation routines with controlled records. JP storage,
unlock and level-growth callees are isolated explicitly; it is not a full
original equipment UI/AI/animation replay. Fixture bytes stay local.
`make core-equipment-check` checks battle rules/transactions, management
support removal and first-frame equipment UI on PC, ASan/UBSan and68000.
Full-frame/stack/residency and release gates remain separate.
