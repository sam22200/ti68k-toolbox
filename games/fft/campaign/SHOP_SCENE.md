# The town and the shop: five layout candidates

2026-11-20, awaiting the user's visual decision. The shop and the Soldier
Office ship as plain lists ([TOWN_SERVICES.md](TOWN_SERVICES.md)); the source
shows a room instead -- a shopkeeper behind his counter, Ramza in front of it,
the speaker's portrait over a line of dialogue, the menu on the left and the
War Funds at the bottom right. The user asked for at least **Ramza, the
shopkeeper and perhaps the counter**, and for several candidates to choose
from.

## The art

`tools/shop_scene.py` writes `ffsk` (FSK1, 670 bytes), review art for now:

- the **shopkeeper's portrait**, 32x29, read from the disc's `40M` townsperson
  sheet exactly as `common_portraits.py` reads a speaker's (the portrait block
  at 0x8200, rotated, scaled, greyed by luminance, outlined). `40M` is the
  middle-aged man the source shows behind a store counter.
- the **shopkeeper's body**, `40M` TYPE1 frame 9 (idle, facing the camera), and
  **Ramza's**, `RAMUZA` frame 14 (idle, facing away), both through
  `units.sprite` at the battle size 16x26 so the two face each other across the
  counter. The map's 67% poses are too small to read against a room.

The room itself is drawn natively -- shelves, goods, a counter, and for a town
roofs and windows. It suggests the source's room; it does not rip it. Nothing
here is isometric: at 160x100 an isometric interior costs far more than it
reads.

## The candidates

Rendered by `preview_shop.c` (`make shop-review`), PC only, no shipping
behaviour and no save write. Each is rendered twice, for a shop and for a town
(a town has no speaker, so it shows its own name instead of a portrait).

| # | shop | town |
| --- | --- | --- |
| 1 | portrait and dialogue band on top, the room below, the menu over it, War Funds bottom right -- the source's own arrangement | the town seen from its gate under the same band |
| 2 | a picture band on top, the item list underneath | the picture band over the three-row menu |
| 3 | the item list on the left, the counter kept in a narrow right-hand column | the menu left, the town in the right-hand column |
| 4 | a full arrival picture with a caption band, ENTER then opens the plain list | the same for the town |
| 5 | the portrait and the words on top, the menu on the left, a small picture beside it | the same without a portrait |

Candidate 1 is the closest to the original and the only one that keeps the
menu over the picture. Candidate 4 costs an extra key press but leaves the
working screens exactly as they are today. Candidates 2, 3 and 5 keep one
screen and give the picture a fixed share of it.

Known rough edges of the review renders, to fix once a candidate is chosen:
candidate 5 prints the town's name twice, and the pictures use placeholder
spacing rather than the source's framing.
