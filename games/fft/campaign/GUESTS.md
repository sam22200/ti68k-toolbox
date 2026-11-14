# Persistent story guests

2026-11-13. Imposed guests now retain progression between encounters and through
Save/Continue. FSV6 has four separate guest records, preserving all16 ordinary
party slots. The pilot binds Gariland's Delita; Orbonne's fixed prologue actors
remain encounter-local. [Story choice presentation/effect opcodes](STORY_CHOICES.md) are now delivered
in the subsequent lot; its measurements supersede the scene/core sizes below.

## Identity and ownership

Source-code read, not a new retail run: `sources/fft_decomp/include/fft/unit.h`
defines16 ordinary slots plus four guest slots16..19. `main_party_save_unit`,
`main_party_find_free_slot` and the event party-save code retain these formation
records separately. `main_item_get_total_equipment_quantity` counts equipment
across all20 slots. Native guest saves follow those capacity/ownership rules.

Chapter specifications can declare:

```json
"persistent_guests": [{"route":"g", "slot":0, "id":4}]
```

The shipping pass generates one chapter-owned `ffgu` directory. ID4 denotes the
pilot's Delita, initially the source Chapter I identity0x04. These are stable,
authored character keys: keep a key when its encounter slot/job/sprite changes;
never derive identity from the current job, generic name or battle position.
FGP1 is magic4, byte length2, entry count2, then4-byte entries:
route byte, encounter actor slot byte, big-endian nonzero identity2. One directory
covers every encounter; an actor binds once at cold entry. No artwork is copied.
The generated pilot bank is12 bytes. Explicit banks are also accepted; a bank
disagreeing with an authored declaration is rejected. Duplicate keys/actor slots
within a route, more than four bindings, bad spans, absent routes and a binding
to a non-guest actor fail before distribution/start. Equal bytes still use the
normal shared-resource audit. A core must declare `guest_format: FGP1`.

Cold RESOURCES16 binds keys after encounter generation/player deployment. A
matching saved guest uses PARTY15 and the same `saved_unit_load` as normal party
members. A new identity starts from the encounter's definition. Restore preserves
the imposed role, position/facing and AI control; it restores the record's stats,
job/equipment, learned actions and equipped R/S/M, heals and resets battle-only
state. A different actor slot can therefore represent the same guest next time.
Unmapped guests remain fixed encounter actors. Absent saved guests stay in reserve.
Full guest capacity rejects a new identity without overwriting any saved slot.

Victory serializes guests through the shared new/progress writers: EXP, level,
current-job JP/TJP, equipment, quarter Brave/Faith and current-raw/Degenerator
state. Other career rows survive. Guest KO remains temporary; protected-guest
defeat still commits nothing. Repeated settlement neither duplicates a record
nor reapplies a quarter change. Initial guests retain their actual support and
movement instead of receiving the generic recruit's Equip Change default.
The99-owned limit now includes reserved guest gear; battle discovery substitutes
live equipment for the deployed saved records. Steals and rewards use the same
owned-count helper. The separate full Fur Shop/poaching work remains open.

`save_guest_remove` and `save_guest_recruit` are in-memory story hooks. Recruitment
copies a guest into a free normal slot and transfers its raw-stat marker; a full
16-slot party returns255 and leaves both rosters untouched. Removal clears its
record/key/marker without changing ordinary capacity. The future story-effect
caller must persist its flags and this durable Save together in one write;
these helpers do not create files, recruit automatically or provide a choice UI.

## Save and native budgets

FSV6 appends to FSV5, retaining every old field/offset:

| Offset | Field |
|---|---|
|3204|Four big-endian guest identities,8 bytes|
|3212|Guest current-raw bitmap, low four bits|
|3213|Reserved zero|
|3214|Four unchanged176-byte SaveUnit records|

Save is3918 bytes (+714). FSV2/3/4/5 migrate with empty guest slots; FSV5's
ordinary raw markers survive. Invalid FSV6 guest keys/bitmap/reserved fields are
rejected. Title, scene, deployment, management, world and supervisor readers use
the new length; existing slots are still written only by explicit Save. The
supervisor's allocation-free view accepts all five versions. `save_dump.py`
decodes both ordinary and guest records. No sidecar save or per-frame allocation.

ABI91/ModCtx352/signatureF8B5 rejects stale native images. Unit112/State2224 stay;
native Work29340 adds26 key bytes. There are still28 images and seven requested
battle blocks:99332 bytes (+884 =714 Save +26 Work +144 execution zone).
Cold grid/glyph construction and result capture move from PARTY/SAVE to
RESOURCES17..20, always returning before another image loads. Shared load/write
functions each serve ordinary units and guests in their existing owner. PARTY
is7836, SAVE8842, RESOURCES8370; zone8842. Shards63280/63608/63432, splits9/18.
Actual production battle input61381/64936 leaves3555 bytes. The48-frame cache,
3072-byte companion reservation,13 active actors and16 ordinary slots remain.

Pilot core416987 + chapter291789 =708776 transport bytes, matching key
`165279dae1c252e57b85cbc33e976fcb`. Old chapter art/encounters are byte-identical;
only `ffgu` and the chapter manifest are added/updated. Core programs change for
the new Save reader/length. Matching packages are under `x/guests/release/`.
These are requested allocations/build sizes, **not a new16KiB installed free-RAM
or peak certificate**. That gate remains open and difficult recovery stays deferred.

## Verification

PC/sanitizers and one actual68000 SAVE/restore carrier agree on104 checks,
11AC3656: migration, current stats, cold reentry, another actor slot, absent/fourth
guest, full-capacity rejection, duplicate/corrupt identities, atomic remove/
recruit hooks, repeated receipts and guest equipment at the99-owned cap.

The full private two-fight chain runs actual PC/sanitizer battle/title/supervisor
processes: Save, decline, and protected-guest defeat/Continue. Every session/slot/
child snapshot agrees byte-for-byte. Natural combat advances Delita from15/EXP91
and210 earned JP at the checkpoint to16/EXP0 and302 earned JP after the second
fight. Six full wins retain8553 updates/26 actions/3 guest actions; the injected
loss retains193 updates. This fixture is authored QA on borrowed Gariland assets,
not a newly extracted retail chain.

Package checks pass42 host +831 C checks, PC/sanitizers; generated guest metadata
survives repeat packaging/core reuse. Focused PC title/deployment/world/scene and
raw-management checks pass, including preservation of an existing guest tail.
The longstanding historical `test_manage` fixture still expects a three-unit
proof while `x/check/ffsave_won.sav` now has five; it is not this lot's regression
or a passing check. The deployment test's obsolete FSV4-size assertion is updated.
The actual68000 scene/save carrier agrees on198 checks/5501566B and the two
unchanged prompt screens. One selected13-actor equipment replay retainsAA26 and
355080 peak ordinary cycles (<360k). No all-family native sweep or TiEmu session.

```sh
rtk proxy make -C games/fft/campaign campaign-guests-check
rtk proxy tools/pyenv/bin/python games/fft/campaign/check_chains.py --guests \
  --input games/fft/campaign/x/guests/release/source.89g \
  --out games/fft/campaign/x/guests/chains
```

Native guest checks are explicit: `check_guests.py --native --data-dir PATH`,
using the matching installed `.bin`/`.89y` banks. Evidence, hashes, allocation
probe, reused/changed-byte report and protected save/file timestamps live in
`x/guests/verification.json` and its referenced reports. No new visual style
needs approval; the approved art, fonts, box style and renderer remain.
