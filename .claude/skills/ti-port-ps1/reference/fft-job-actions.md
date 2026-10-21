# FFT job actions: requested rules and implementation boundary

These are the user's FFT PS1 design requirements, preserved for future job work.
The complete table below is not a claim that all twenty jobs have been verified
against the original or implemented. Before implementing another ability, check
its source timing, targeting and interruption behavior; record any discrepancy
with this requested model in `games/fft/RE_NOTES.md`.

## Four different execution mechanisms

- Immediate: apply the effect during the action, without a deferred cast.
- Vulnerable preparation: remain on the map and targetable until resolution.
- Protected Jump: leave the battlefield while airborne and become untargetable;
  strike the stored tile on landing, missing if it is empty.
- Performance: remain in a persistent state with periodic effects on eligible
  targets; do not implement this as a single delayed attack.

Share the CT clock where appropriate, while keeping distinct `CHARGING`,
`AIRBORNE` and `PERFORMING` states. CT ticks are not completed character turns.
Menus and dialogue must not advance combat time. Define interruption and action
cost per mechanism rather than inheriting Archer's rules for every job.

## Target identity is independent of execution timing

| Requested action | Target after movement | Caster during delay |
|---|---|---|
| Archer Charge | Stored tile; can miss if it becomes empty | Present and vulnerable |
| Lancer Jump | Stored tile; can miss if it becomes empty | Absent and untargetable |
| Mage, unit selected | Follow that unit | Present and vulnerable |
| Mage, tile selected | Keep that tile | Present and vulnerable |
| Samurai Draw Out | Immediate effect | No delayed protection state |
| Bard/Dancer | Reevaluate eligible targets at each pulse | Persistent performance |

Do not let a normal unit query, hit test, AI target list or line-of-sight target
selection include an airborne unit. Protected Jump and ordinary casting are
separate mechanics even if both resolve through the same scheduler.

## Twenty-job requirements supplied by the user

| Job | Requested examples | Execution category |
|---|---|---|
| Squire | Accumulate, Throw Stone; Monster Skill | Immediate; passive support |
| Knight | Weapon Break, Speed Break, Armor Break | Immediate |
| Archer | Charge +1 through +20 | Vulnerable preparation, fixed tile |
| Monk | Chakra, Earth Slash, Revive | Immediate |
| Thief | Steal Weapon, Steal Heart, Steal EXP | Immediate |
| Geomancer | Geomancy and terrain effects; Attack Up | Immediate; passive support |
| Lancer | Jump; Horizontal/Vertical Jump upgrades | Protected Jump, fixed tile |
| Samurai | Offensive, healing and buff Draw Out | Immediate |
| Ninja | Two Swords; Throw; Vanish | Passive; immediate; reaction |
| Dancer | Wiznaibus, Slow Dance, Nameless Dance | Periodic performance |
| Chemist | Auto-Potion; Throw Item; Phoenix Down | Reaction; immediate; immediate |
| Priest | Raise, Holy, Protect, Shell | Vulnerable casting |
| Wizard | Fire, Ice, Bolt, area spells, Frog | Vulnerable casting |
| Time Mage | Haste, Slow, Stop, Quick | Vulnerable casting |
| Oracle | Petrify, Life Drain, Foxbird | Vulnerable casting |
| Mediator | Invite, Praise, Preach, Threaten | Immediate |
| Summoner | Offensive summons, Golem, Lich | Vulnerable casting |
| Bard | Battle Song, Cheer Song, Angel Song | Periodic performance |
| Calculator | Math Skill and filters | Immediate, without casting delay |
| Mime | Imitation of compatible actions | Automatic after the copied action |

## Current implementation and evidence

`games/fft/opening/` exposes Archer Charge +1/+2. The US executable's table gives
4/5 CT ticks for those two actions. It stores a tile, leaves the archer targetable,
permits another unit's turn before impact, and cancels on death, displacement or
weapon change. PC regressions and actual-68000 state/pixel replays cover this
implementation; see `games/fft/opening/BUGFIXES.md` and `regressions.c`.

`DEFER_AIRBORNE` and `DEFER_PERFORMING` are reserved identifiers, not working
Lancer/Bard/Dancer implementations. Unit-following magic is also future work.
Existing Squire/Chemist/basic combat support is not proof of the complete table.
