# Pre-extraction requirements: breeding, Bars and poaching

2026-11-13. The user requires these systems in the core readiness queue **before
starting campaign extraction**. They extend CAMPAIGN_PREREQUISITES.md; they do
not replace the active target/protect objective work. This is an initial rules
survey and code audit, not an implementation or a completed source oracle.

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
| Calendar/breeding | [Portable backend](BREEDING.md) checked on PC/sanitizers/one68000 carrier, plus ten original-MIPS policy/hatch fixtures. FSV6/worldmap/deployment/management integration and reviewed egg UI remain; current shipping Save has no date/eggs. | `world_unit_update_monster_breeding`, `main_party_create_monster_egg`, formation hatch progress/total helpers; travel day is at completed location-to-location leg arrival. |
| Bar/rumors | No Bar/rumor route in worldmap.c. The generic menu's unavailable entries are not working Bars. Save has no rumor-specific state. | `wldcore_list_open_rumors`, `wldcore_bar_push_rumor_detail_level`, rumor input/close handlers and their script variables. |
| Propositions | No worker dispatch, away-unit state, active job timers or return settlement in the native save/UI. | `wldcore_list_open_available_propositions`, dispatch candidate/confirmation handlers, proposition states, `main_job_add_proposition_jp`. |
| Poaching | `nature_runtime.inc::nature_hit_commit` removes the victim and rolls source loot. Its explicit policy gives blue-party loot directly to capped inventory. There is no independent Fur Shop stock or purchase UI. | `battle_action_add_poached_item_to_fur_shop_inventory`, `world_shop_adjust_poached_item_quantity`, shop eligibility/price/transaction handlers. |

**OBSERVED IN SOURCE (read, not a new executable oracle):** the original world
routine decrements existing egg timers before attempting daily reproduction,
chooses a roster monster and offspring-table entry, creates the egg, then grows
it to a level selected from a non-egg roster member. Creation stores a real
generated unit, not a recipe rerolled when hatching. Hatch updates its date/sign.
The source rumor list filters64 entries by location mask and script variable.
The original poach-stock writer saturates at255, independently of ordinary
inventory limits. Its exact rare/common selector is already covered by the
earlier discovery oracle; preserve it rather than rounded guide percentages.

These are source leads, not complete retail flow certification. In particular,
travel advances the date at completed route-leg arrival (source read, see
BREEDING.md). Still identify which rumor interactions write story
state, and how purchase access/prices depend on chapter and equipped support.

## Required work before campaign extraction

1. **Shared world clock and durable state.** Advance time by original travel
   rules once, not by animation frames, repeated ENTER, cancelled travel or
   reentry. Define compact persistent dates/egg/job/rumor/poach state with
   migration from current FSV6 and older formats. Keep current saves, permanent raw stats, discovery,
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
