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

`tools/shop_scene.py` takes a **160x100 window of the source at 1:1** (the
source's own pixels read far better in four greys than a scaled-down room),
frames it so the shopkeeper and his counter fall to the right of the menu box,
maps it to four greys between two measured luminance percentiles -- the store
is a bright room full of dark goods, the office a dim one, so each has its own
pair -- and ZX0-compresses the two planes. `ffsk` (FSK2) is about 5 KB for both
rooms and the portrait.

The **town's own menu stays over the world map**, as the source keeps it over
its world map: there is no town picture to rip, and none is needed.

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

## Still to frame

The store room reads; the office room's window still puts its clerk in an
awkward band and wants another pass.

`make shop-review` renders `x/shop-review/shop-store.png`,
`shop-office.png` and `shop-town.png` through `preview_shop.c` (PC only, no
shipping behaviour, no save write).
