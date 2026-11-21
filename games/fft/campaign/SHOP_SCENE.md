# The town's screens: the source's own arrangement

2026-11-20. The user asked for the original's layout, not a candidate of our
own: the shopkeeper's portrait over a dialogue box at the top, a battle-style
action menu at the bottom left (Buy / Sell / Fitting room / Leave store), the
War Funds at the bottom right, and the source's own room behind all of it.

## The room is a picture, not a scene to build

OBSERVED on the disc: **`MENU/BK_SHOP.TIM`**, **`MENU/BK_HONE.TIM`** (the
Soldier Office) and **`MENU/BK_FITR.TIM`** (the fitting room) are 256x128
16-bit TIM images, pre-rendered, and **they already contain their people** --
the shopkeeper leaning on his counter and a customer in front of it, the office
clerk at his desk and a recruit before him. `BK_*2` and `BK_*3` are the other
towns' rooms.

So there is nothing to place on top: the room is one picture, the way a battle
map is one view, and no sprite rip, no isometric renderer and no second
orientation are involved. This replaces the earlier plan of drawing a room
natively and standing two 16x26 bodies in it.

`tools/shop_scene.py` takes a **160x66 window of the source at 1:1** (the
source's own pixels read far better in four greys than a scaled-down room),
frames it so the shopkeeper, his customer and the counter all fall to the right
of the menu box, maps it to four greys between two measured luminance
percentiles, and ZX0-compresses the two planes. Each room costs about 2 KB.

The user chose, out of five mappings, the **15th and 97th luminance percentiles
through a 1.4 gamma** (2026-11-20). The whole 160x100 screen is ripped, not just
the rows under the dialogue box: an empty band above it looked wrong.

The **town's own menu stays over the world map**, as the source keeps it over
its world map: there is no town picture to rip, and none is needed.

## The shared pieces

The three overlays are the campaign's own, not new ones:

- **`dialogue_box.inc`** (new) holds the reviewed dialogue look: a detached
  portrait in its own frame, a detached name box, and a black box with white
  text under a three-row ordered gradient. It is extracted from `beats.inc`,
  which now calls it, so the opening, the scenes and the towns draw one box.
  The store puts it at the top of the screen, as the source does.
- **`choice_menu.inc`** draws the action menu, narrowed to its own labels and
  pushed right so the glove is wholly on screen.
- **`war_funds.inc`** (new) draws the War Funds plate in the title screen's
  style: a black box per line with white text, the label above the amount, one
  pixel between them, each box as wide as its text needs, the pair one pixel
  from the bottom and the right. The world map uses the same plate. The
  battle's result screen keeps its own reviewed layout.

## The portrait is not settled

The user was right to ask. The bank currently carries the **40M townsperson's
battle portrait**, read exactly as `common_portraits.py` reads a speaker's.
That was an informed guess, not an identification, and the office clerk is
plainly a different man (bald, where 40M is bearded).

The real bank is **`EVENT/EVTFACE.BIN`**: one 4bpp TIM whose pixels are stored
as a 32-pixel-wide column, 32 faces of 32x48 with 16 CLUTs. It is decoded and
its faces are legible, but which index the store and the office use, and which
CLUT goes with each face, are still open. Until that is answered the portrait
in the bank is a placeholder and the office borrows the store's speaker.

## In the game

`worldmap.c` ships the three screens: the town menu over the world map, the
store's Buy / Sell / Leave store over its room, its item list, and the Soldier
Office's four hire rows. The map shows the same War Funds plate. The item list
takes the room's full height and drops the dialogue box, as the source does.
Steady shop frame: 328,264 cycles of the 360k budget; the room decompresses
once per entry into the map's own 32 KB strip cache, which is idle meanwhile,
so no second allocation.

A sprite read in place from a bank must start on an even address: the portrait
landed odd and the 68000 took an address error while the PC build was happy.
The bank header is padded; the lesson is in `ti68k-c-patterns.md`.

`make shop-review` renders `x/shop-review/shop-store.png`,
`shop-office.png` and `shop-town.png` through `preview_shop.c` (PC only, no
shipping behaviour, no save write).
