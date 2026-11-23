# A town's services and a field's encounters

## Current services (2026-11-23)

Propositions are an actual mode of management, reached from the Bar when source
Chapter II event075 is bound/completed; accepted picker A, source prose,
dispatch/away/day/recall/saved report/one-shot collect are integrated.
Poaching now feeds separate cap255 `Save.poach`, retained by real battle exit.
The Fur room uses the shared Shop Buy/Sell/detail UI at half source prices,
with atomic funds/bag/stock changes. Access requires source III event0CB,
Dorter9/Warjilis12/Zarghidas14 and an equipped/innate Secret Hunt unit.
The Chapter I pilot intentionally binds neither later service event.

FSV9/4684 migrates FSV2..8, preserving previous offsets and slots. Core owns
ffpr/FPR1, ffpe/FPE1, ffpt/FPT1; chapter owns8-byte ffsv/FSV1 event bindings.
Current proofs/private matching groups are under `x/town-complete/`, detailed
in [HANDOVER.md](HANDOVER.md). Installed groups are preserved; no current
installed-RAM or16KiB claim. The2026-11-22/20 notes below are historical.

Current follow-up2026-11-22: [playable pilot Bar](BAR_SERVICE.md).
The user selected C/D/C/D for source faces114/115/116/117. The active FSK4
bank and extractor now use the correct Shop/Office/Fur portraits; the Bar
uses its selected owner and the shared campaign continuation cue,
with no numeric counter. The Bar's stored room is in WLDBK, outside MENU TIMs.
Town -> Bar -> source rumor list -> grey reader is now implemented for source
IDs0..2 at progress1, with town masks, original titles, pages/back and FSV8 profile
effects. All six Chapter I rumor conditions now have source-checked event bindings
and actual input/scroll/reentry proof; the pilot still ships only its original
three. Propositions and separate Fur stock remain; later chapter quests are separate.
Installed groups are preserved and need a matching release rebuild. The original
Shop/Office implementation and earlier measurements below are retained.

2026-11-20. The world map's locations do something when Ramza stands on them
and no story battle is pending: a town or a castle opens its **Shop** and its
**Soldier Office**, a field rolls one of its **random formations**. Everything
lives in `worldmap.c` (fftmap), which had the room the battle resident does
not, and reads one new bank, `ffsh` (`tools/shop_assets.py`, FSH2, 1,468 bytes,
core group).

## Source behavior

OBSERVED on the disc, `battle.py` `ITEMS` (12 bytes an item):

- the **price** is a little-endian u16 at offset 8. The bank stores it
  big-endian like every other record we ship. Checked against the Shop Guide:
  Potion 50, Dagger 100, Broad Sword 200, Bow Gun 400, Phoenix Down 300.
- the **stock tier** is the byte at offset 10: 0 never sold, 20 the rare items
  no shop carries, 1..16 the tiers the story opens one after another. 198 items
  are sold at some point; 49 by tier 4, which is as far as Chapter I goes.
- the item's **category** is the byte at offset 5 (1 dagger, 3 sword, 19
  shield, 34 basic item, and so on), and it matches the guide's families
  exactly.

OBSERVED in `world_assets.py` `DOTS`: a location record's flag byte is the kind
of place, **0 a castle, 1 a town, 2 a battle site**. Only the first two have
services; only the third rolls encounters, and the tool refuses a table that
disagrees.

OBSERVED in `battle.py` `gen()` (`GEN_BASE`/`GEN_VAR`): the out-of-battle unit
generator's rows. Row 0 (man) and row 1 (woman) hold the five raw-stat seeds,
their five variances, and the equipment a recruit arrives with: Leather Hat,
Clothes, and a Broad Sword for a man, a Dagger for a woman. `breeding.h`
already runs that generator for monster row 3; the Soldier Office reuses it
unchanged, same draw order, so a recruit and a hatchling come from one routine.

INTERPRETATION (Shop Guide, S. Maxson 2004): which categories each of the six
shop types carries and which type each town is. The game derives both from data
we have not located; the tool's tables reproduce the guide's lists. Gariland's
is type A, Igros's type B, Dorter's type C.

INTERPRETATION (Final Fantasy Wiki, *Warriors' Guild*): a recruit costs 1,500
gil, 100 less for a woman. We have not found that price on the disc.

## What the runtime does

A town's ENTER opens **Shop / Soldier Office / Leave**.

**Shop.** One page, seven rows, LEFT/RIGHT switching buy and sell. An item is
on sale when its category is in the shop type's set (or it is that type's lone
exception, Gariland's Broad Sword) and its stock tier is open -- a tier opens
with the story flag the bank names, 0 meaning from the start and 255 meaning no
battle of this build reaches it. The pilot therefore sells tier 1 only:
Gariland's ten items, exactly the guide's list. Buying costs the price and
stops at 99 copies; selling back returns **half** the price, as FFT does, and
the row leaves the list with its last copy. Both write `save.money` and
`save.inv`, which the battle already reads.

**Soldier Office.** Four rows: Squire or Chemist, man or woman. Hiring spends
the price and appends a level-1 unit generated by the source generator --
Brave and Faith 40..71, raw stats from the row's seeds and variances, an unused
generic name, a zodiac sign from a drawn birthday -- equipped as the source
equips it. The party stops at 16.

**A field.** One of the location's three formations, drawn from the same
durable world stream that drives breeding. Those doors are authored, like
`world_assets.py`'s `STORY` list, and all five Chapter I fields currently hold
0: no random battle is ripped yet, so a field still answers "Not ripped yet".
The roll, the table and the door return are in place; what is missing is the
battles themselves in the chapter package.

## Cost

Measured with `ti-cycles` on the TI binary, steady frames, 360k budget:

| screen | cycles a frame |
| --- | --- |
| map, idle | 198,436 |
| town menu | 149,136 |
| Soldier Office | 242,054 |
| Shop | 306,638 |

Two things were needed to get there. Walking the 256 item ids every frame to
build the list cost **1,276,832 cycles a frame**, three and a half times the
budget; the list is now a 256-bit set rebuilt once per trade or page switch,
and walked by skipping empty bytes. Framing the screen with two full-screen
fills (`panel(0,0,160,100)`) cost about 130k more for nothing, since the
cleared screen is already the panel's inside: the border is four thin
rectangles.

## Checks

- `make map-test`: Gariland's stock and prices, buying with and without the
  gil, selling back at half, the Soldier Office's prices, a refused hire, a
  hire and the unit it writes, and a field with nothing ripped.
- `check.py` (`--menus-only`): five new PC/68000 replays -- `town`, `shop`,
  `shop-sell`, `office`, `hire` -- with identical pixels and identical state,
  the recruit included. 28 map/menu replays pass.
- `make packages` / `packages-check`: 43 host checks, 927 C checks.

A TI data variable reports its trailing tag bytes as well as its payload, so a
bank's size check must be "fits", not "matches": the first 68000 run bound no
shop at all because `ffsh` reported 1,474 bytes for 1,468.
