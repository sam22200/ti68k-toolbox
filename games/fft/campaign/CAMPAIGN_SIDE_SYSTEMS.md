# Pre-extraction requirements: breeding, Bars and poaching

2026-11-13, status updated2026-11-23. The user requires these systems in the core readiness queue **before
starting campaign extraction**. They extend CAMPAIGN_PREREQUISITES.md and build
on the delivered objective/scene-choice/calendar/monster work. The guide survey
below is orientation; current delivery status is in the table and linked notes,
not a claim of a complete source oracle for every side system.

Target: the original US PlayStation game. Keep PS1 names/limits, rather than
silently importing War of the Lions or Ivalice Chronicles changes. Chapter I
must retain the original unlock progression; later systems can be tested with
injection doors without extracting later chapters first.

## GameFAQs overview

GameFAQs rejected direct page fetches in this environment. The initial survey
uses its indexed guide excerpts; no claim is made to have downloaded complete
FAQs. Their approximations/conflicts need original-game checks before coding.

### Monster breeding

A single roster monster can produce eggs while days pass on the world map;
another parent is unnecessary. Eggs later hatch into that family, potentially
another variant. Eggs consume roster space. The monster guide describes the
new unit as determined when its egg appears, including a level selected from
a roster member, so reloading just before hatching must not reroll it.
[wpot, Monster Guide, reproducing section](https://gamefaqs.gamespot.com/ps/197339-final-fantasy-tactics/faqs/81475).

That guide now contains material for newer releases, so it is orientation,
not authority for PS1 limits or exact probabilities. The older PS1 dungeon
guide explicitly counts eggs within the16-member roster, and describes using
breeding to obtain higher monster variants for poaching.
[Gastrifitis, Deep Dungeon Exits Guide, poaching section](https://gamefaqs.gamespot.com/ps/197339-final-fantasy-tactics/faqs/8808).

Do not adopt claims about seasons, zodiac-compatible parents or rounded daily
chances from old community tips. Exact parent selection, offspring tables,
hatch counters, RNG boundaries and full-roster behavior remain source checks.

### Bar, rumors and Propositions

Bars and rumors already belong to Chapter I. The PS1 walkthrough lists city
menus and early rumor topics at Gariland and Igros, varying with progress.
Rumors therefore need location/story eligibility and readable detail pages;
they are not simply a late-game quest list.
[berevan, Guide and Walkthrough, Chapter I](https://gamefaqs.gamespot.com/ps/197339-final-fantasy-tactics/faqs/18478).

Propositions become available in Chapter II. Up to three ordinary human units
are dispatched for days, sometimes for a fee; party members away on a job are
unavailable for combat. Rewards include gil/JP and collection entries such as
treasures or unexplored lands. The choice of workers can affect success.
[RCarlos, Strategy Guide part6, Propositions and Treasure](https://gamefaqs.gamespot.com/ps/197339-final-fantasy-tactics/faqs/3872),
[BoardFourSixNineFour, Propositions section](https://gamefaqs.gamespot.com/ps/197339-final-fantasy-tactics/faqs/30113).

The core needs the shared dispatch/return/calendar/save mechanism before
extraction, with Chapter I gating it off. Do not populate Chapter I with jobs
from later chapters, or treat completion of a job as a new battle scene.

### Poaching and the Fur Shop

Secret Hunt is the PS1 Thief support ability. A qualifying lethal physical
attack poaches a monster instead of leaving an ordinary KO/crystal/chest.
The original economy sends a common/rare result to Fur Shop stock for later
purchase; it is not an immediate free item in the party inventory.
[MHobbs, Poaching List](https://gamefaqs.gamespot.com/ps/197339-final-fantasy-tactics/faqs/3884).

Do not infer eligibility solely from brief guides saying "regular Attack".
The mechanics guide also names Charge and Counter; audit the real impact
owners/reaction paths and original exceptions before changing support471.
[AeroStar, Battle Mechanics Guide, Secret Hunt](https://gamefaqs.gamespot.com/ps/197339-final-fantasy-tactics/faqs/3876).

The shop guide places Fur Shops in the Trade Cities from Chapter III. Keep
this gate even though the support ability and stock pipeline can exist earlier.
Some older guides disagree on the chapter or quote approximate rare-item
percentages; neither should replace original conditions/tables.
[SMaxson, Shop Guide](https://gamefaqs.gamespot.com/ps/197339-final-fantasy-tactics/faqs/30462).

## Current native code versus source leads

| System | Native state | Original routines identified for the bounded follow-up |
|---|---|---|
| Calendar/breeding | [FSV7 calendar/eggs](BREEDING.md), [adult Formation/Status](MONSTER_FORMATION.md), wide bodies and the real deployment/Save/battle/results loop are integrated; see [HANDOVER.md](HANDOVER.md). Eggs remain excluded from battle. | `world_unit_update_monster_breeding`, `main_party_create_monster_egg`, hatch progress/total helpers; one day at completed route-leg arrival. |
| Bar/rumors | [Chapter I mechanism delivered](BAR_SERVICE.md): source lifetimes of all six topics, prefixes/effects, explicit event/flag bindings, accepted A/117D reader and3-row scroll. Four-stage PC/sanitizer flow and a selected native long title pass; the active pilot keeps only its three original topics. FSV8 persists profiles8/9. | `wldcore_list_open_rumors`, detail/input handlers, WORLD F5/F6 stores and original events00C/02D/03A/075. Later chapter quest effects are separate. |
| Propositions | [Playable accepted A](PROPOSITIONS.md): actual management dispatch/day/recall/frozen report/one-shot collect, source descriptions, explicit generic identity/away restrictions, FSV9 migration and Save/Continue. PC/sanitizers and selected native pointer/render/layout checks pass. | All96 rules and181 original reward pipelines checked; safe preferred-job adaptation retained. Bind original075 when Chapter II content is authored; pilot remains locked. |
| Poaching/Fur | Actual impact preserves one loot roll/removes victim and adds separate cap255 stock; real battle exit retains it. Shared Shop Buy/Sell at half source price with atomic transactions and migration. | Original trade cities/Secret Hunt and Chapter III0CB gate retained. Counter reaches the same post-impact owner; no full new retail animation replay claim. |

**OBSERVED IN SOURCE (read, not a new executable oracle):** the original world
routine decrements existing egg timers before attempting daily reproduction,
chooses a roster monster and offspring-table entry, creates the egg, then grows
it to a level selected from a non-egg roster member. Creation stores a real
generated unit, not a recipe rerolled when hatching. Hatch updates its date/sign.
The source rumor list filters64 entries by location mask and script variable.
The original poach-stock writer saturates at255, independently of ordinary
inventory limits. Its exact rare/common selector is already covered by the
earlier discovery oracle; preserve it rather than rounded guide percentages.

These source leads are not complete retail flow certification. Travel advances
the date at completed route-leg arrival (see BREEDING.md). The Chapter I rumor
effects and Fur access/half-price rules are now bound as described above;
later chapter quest effects remain a separate content scope.

## Acceptance contract (delivered2026-11-23)

The implementation/evidence in the table above closes the reusable gates below.
This list records their requirements, not another pending service queue. The
remaining campaign steps are in CHAPTER1_READINESS.md.

1. **Shared world clock and durable state.** Advance time by original travel
   rules once, not by animation frames, repeated ENTER, cancelled travel or
   reentry. Define compact persistent dates/egg/job/rumor/poach state with
   migration from FSV2..8 into FSV9. Calendar/eggs, rumor profiles, job/poach
   state are delivered. Keep current saves, permanent raw stats, discovery,
   roster compaction and unused party records intact. Measure RAM costs; the
   existing16KiB gate is unpassed and cache capacities must not be reduced.
2. **Breeding and eggs.** Source-check generation/hatching order, offspring
   variants and timers; create fixed offspring state once, save it and hatch
   without reroll. Handle all16 slots, no-room failures, dismissed/changed
   parents and multiple events on one day. Eggs cannot deploy or behave as
   ordinary humans. Review roster/egg art and status before integration.
3. **Bar and rumors.** Reuse the common menu/text system; select by town/story
   and paginate details with safe back/cancel. Preserve original passive lore
   versus quest effects; no generic "read any rumor = advance story" flag.
   Known dialogue speakers retain portraits. Author/archive shared text by ID.
   The Chapter I mechanism above is delivered: bind later scenes' actual completion
   flags during content authoring, rather than inventing those scenes now. The six
   original Chapter I texts contain only the King's profile effects, already saved.
4. **Propositions.** Source-check candidate restrictions, fees, duration,
   seasonal/progress eligibility and rewards. Away workers must be excluded
   from deployment and duplicate dispatch. Return/collect exactly once after
   save/reload; retain JP/gil, active jobs and any collection results. Chapter I
   shows the correct unavailable state, with later unlocks tested by fixtures.
5. **Poach stock and purchases.** Keep one-shot kill/removal and original loot
   RNG; audit Attack/Charge/Counter eligibility. Store Fur Shop quantities
   separately, enforce access/progress rules, and make gil/stock/inventory
   changes atomic under insufficient funds, full inventory and reload. Define
   migration from the old direct-receipt policy so items are neither removed
   from existing saves nor credited a second time. Breeding and allied-monster
   poaching must share permanent roster removal rather than create duplicates.
6. **PC-first acceptance.** Focused PC/sanitizer runs cover travel/day/save
   boundaries, full-roster eggs, worker exclusion, rumor gates and transactional
   stock/reward flows. Use bounded original fixtures for disputed rules. Only
   changed ABI/layout/arithmetic/overlay/budget paths need selected68000 checks;
   no new whole TI sweep or emulator session is required for this survey.

Keep rules/universal tables in core and chapter text/eligibility/content in the
active chapter, pooled by ID. Full later-chapter text/maps are not required to
test the reusable machinery. No item above is complete merely because menus
exist or combat support471 already has a consumer.
