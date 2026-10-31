# Decisive effects: Death, Quick, Zombie and Petrify

2026-10-30. Four production records are enabled as one shared target/status/
clock family after CORE_RESTORATION. Source data and rules remain separate;
no battle/save field, new image count or heap allocation is introduced.

## Source behavior

`tools/terminal_oracle.py` executes the unchanged original formula dispatchers
and final target clamp for 56 profiles. Controlled xorshift draws replace the
original random source; target-list sorting is isolated. Original instruction
and disc hashes are recorded in `x/core-terminal/oracle/source.json`.

- Zombie and Petrify use the existing source all-or-nothing status filter,
  immunity/innate/rider/immortal rules, Faith/MA/zodiac accuracy, evasion and
  final Dead/Petrify/Wall validation. Repeated immutable statuses miss.
- Death carries the source Dead infliction together with its nominal maximum-HP
  percentage damage. AB_DEATH retains that distinction from ordinary HP damage:
  a changed maximum during wind-up cannot turn an accepted Death into a partial
  hit. Its nominal damage caps at 999. Late Dead immunity blocks the infliction
  while retaining the prepared HP damage. Living
  Undead instead receive HP healing; that path skips magical evasion and target
  magical defenses, but still uses Faith and its accuracy draw. Ordinary Dead
  immunity and immortal status protection prevent the offensive effect.
- Quick has no ordinary status bit. Its prepared packet carries explicit
  AB_QUICK and the CT255 request. Accuracy uses friendly Faith/MA/zodiac rules.
  A successful impact fills CT; an executing action retains ownership until
  its responses/rewards settle. The existing Quick-priority scheduler consumes
  the next Quick CT to zero, retaining a pending charge-wave phase.
- Petrify retains HP, CT, coordinates and tactical occupancy. Source cancellation
  clears incompatible states/pending preparation; cached controls freeze the
  statue and ordinary camp elimination no longer counts it. Shared Soft/Esuna/
  Remedy can remove mutable Petrify and restore legal control without moving it.
- Zombie uses the common source Undead bit, so existing cures, Raise, Drain,
  Death and restoration items use their shared reversal/eligibility rules.
  At its KO counter expiry, the source upkeep roll's low bit selects revival;
  a second draw gives 1..maxHP. Otherwise the same roll chooses crystal/chest.
  Twelve original upkeep traces cover counter progression and both expiry
  outcomes. Guests/immortal units keep their existing protected lifecycle.

Ordinary pilot KO RNG remains its established adapter: one draw at expiry.
New Undead KO picks consume the original upkeep draw on each counter tick.
This is not a claim that every living unit now consumes all original upkeep
RNG, nor a complete original animation/reward/AI simulation.

## Shared implementation

The compiler enables all four compatible records from their source family fields.
The 4-byte CastVictim still holds a typed effect and a 16-bit amount. New Death
eligibility uses a pure resident status-filter query; no image calls a loader
from an executing callback. Quick and ordinary status commits belong to FORMULA;
Death retains the shared HP/KO/reaction commit with a typed lethal request.
Unchanged Quick CT and full-HP Undead Death healing grant no new reward. The existing scalar/area target
inspection, text font and generic impact labels are reused; no new sprite/font/
background bank or original spell cinematic is added. A dedicated PS1 stone
palette/animation is outside this rules milestone.

The four-cardinal Wait conversion uses the dimension-independent Jacobian of
core_grid_world in a 16-byte table, retaining the source-camera swap. The old
projection-based reference checks all 16 rotation/direction pairs and full
State/Work control results. This reduces the input image by 34 archived bytes without changing facing.
The largest image remains AI, so this alone does not recover resident space.

The KO counter policy moves from resident ct_next into FORMULA14, called only
from resident clock code after previous images return. This includes the old
ordinary crystal/chest policy and the new Undead branch, without new storage.
The unchanged end-turn CT-credit policy similarly moves to FORMULA15; the
resident wrapper packs nonzero Move/Act flags, returns from every active image
and then loads FORMULA. Existing request storage carries the two flags. No
executing image calls this loading boundary. Native closure includes the
actual turn-credit boundary and cold/warm cost; this is not a full-frame budget
certificate.
Adapted immediate AI uses shared forecasts, values Petrify and enemy HP damage,
and grants useful allied Quick only to another actor with CT below 100. It is
not an original future-turn resource/Reflect simulator.

## Evidence and remaining gate

`make core-terminal-check` checks production catalogue/learned entries, source
formula and RNG results, both Unit/Panel orders, single MP debit, once-only
impact, legal/absent/KO targets, Silence, actual AI, reflected single endpoints,
Petrify/cure occupancy and the real Quick cast-to-turn handoff. The private final
verification report records native checks, package sizes and affected closure.
Reuse unchanged geometry, bitmap-kernel and Jump evidence. Preserve live TiEmu,
standard playtest, saves and old proof bytes AND modification dates.

Production now has 97 executable normalized records, 66 casts and 94/201 generic
action-table entries. The 107 unavailable entries are not 107 effects: six
remaining status/control, 69 other default-formula job entries and 32 special
upgrades/categories. Reraise, Frog, Float, Berserk, Confusion, Brave/Faith control,
Golem, remaining jobs/R/S/M, special actors, campaign/objectives, formats/tools
and full RAM/stack/frame certificates remain open. Keep effects grouped and
Jump deferred. The full core is not complete.
