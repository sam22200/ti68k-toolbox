# The town's screens: the source's own arrangement

2026-11-20. The user asked for the original's layout, not a candidate of our
own: the shopkeeper's portrait over a dialogue box at the top, a battle-style
action menu at the bottom left (Buy / Sell / Fitting room / Leave store), the
War Funds at the bottom right, and the source's own room behind all of it.

## The room is a picture, not a scene to build

OBSERVED on the disc: the whole game holds **nine** 256x128 16-bit TIM rooms
and no others -- `MENU/BK_SHOP`, `BK_HONE`, `BK_FITR`, each with a 2 and a 3
for the other towns' styles. They are pre-rendered and **already contain their
people**. Which is which, read off the pictures against the user's own
screenshots:

| file | room |
| --- | --- |
| `BK_SHOP` | the store: the shopkeeper behind his counter at x 130..155, his customer at x 96..118 |
| `BK_HONE` | *hone*, bone: the **Fur Shop**, hides and a skeleton over the room |
| `BK_FITR` | the **Soldier Office**: books and candles, the guildmaster at x 160..180, a recruit at x 100..122 |

The bank shipped `BK_HONE` as the Soldier Office until 2026-11-21; it is the
Fur Shop's room, and the office now uses `BK_FITR`. **The Bar has no picture on
the disc at all**, and neither has anything else: those screens are drawn by
the engine, not stored.

So there is nothing to place on top: the room is one picture, the way a battle
map is one view, and no sprite rip, no isometric renderer and no second
orientation are involved. This replaces the earlier plan of drawing a room
natively and standing two 16x26 bodies in it.

`tools/shop_scene.py` takes a **160x66 window of the source at 1:1** (the
source's own pixels read far better in four greys than a scaled-down room),
frames it so the shopkeeper, his customer and the counter all fall to the right
of the menu box, maps it to four greys between two measured luminance
percentiles, and ZX0-compresses the two planes. Each room costs about 2 KB.

The whole 160x100 screen is ripped, not just the rows under the dialogue box:
an empty band above it looked wrong. Each room has its own mapping, chosen by
the user out of five:

| room | window | greys |
| --- | --- | --- |
| store | (8, 18) | 15th/97th percentiles through a 1.4 gamma (2026-11-20) |
| office | (26, 8) | 25th/85th, dithered **only above 0.62** (2026-11-21) |
| fur shop | (8, 26) | 45th/95th, dithered above 0.72 (2026-11-21) |

The Fur Shop needs no compositing: Ramza stands at source x 105..125, y 78..115
and the bald merchant at x 140..165, y 60..95, only thirty-five pixels apart,
so one window holds both. Both are outlined, the room being almost black
behind them.

Dithering a whole room turned both of the office's figures into noise. The
dither is confined to the highlights: a candle or a stained window keeps its
texture while the stone and the people stay on flat greys and read.

Both office figures still stand against the window's glare, clear on clear, and
no global grey mapping separates them. They get the port's own visibility rule,
inverted: **a one-pixel contour, black here because the background is the
light**. The same saturation mask that cuts Ramza out serves for it; the
guildmaster needs a stricter threshold (5 instead of 4), the papers piled
beside him being nearly as warm as his robe.

The office picture is **adapted, not only cropped**. In the source Ramza stands
sixty pixels from the guildmaster's desk, and no 160x100 window holds both once
the menu box takes the left half: framed on one, the other is off screen.
`move_actor` lifts him off his own floor -- the stone is grey, he is not, so a
saturation test and the largest connected blob cut him out cleanly -- patches
the hole with the floor twenty-four pixels to his left, and sets him down
twenty-two right and twenty-four up, one step along the room's isometric floor.
Nothing else in the picture moves. The window at (26, 8) then shows Ramza at
screen x 94..118 and the guildmaster at 134..154, both clear of the menu box
and with air left after the guildmaster.

The **town's own menu stays over the world map**, as the source keeps it over
its world map: there is no town picture to rip, and none is needed.

## Every world menu, read off the disc

OBSERVED in **`WORLD/WORLD.BIN`**, in FFT's ordinary text encoding, one
0xFE-terminated run after another. This is the whole set, in the order the game
stores it, and it settles what each screen owes:

| run | strings |
| --- | --- |
| store | **Buy / Sell / Fitting room / Leave store** |
| fitting room | **Try it on / Best fit / Quit fitting**, then Sell / Fitting over |
| | Sell equip / Stock |
| | Yes / No, then O K / N O |
| fur shop | **Buy / Sell / Leave store** |
| soldier office | **Male fighter / Female fighter / Change name / Leave store** |
| | then **Hire / Quit / Check status** |
| | then Yes / No / Ask name again, Num, Quit / No |
| name entry | A-Z, a-z, 0-9, one run each |

What that changed, and what it still asks for:

- the store's menu was already right.
- the office's labels were the War of the Lions wording ("Male Soldier",
  "Exit"); the disc we port says **Male fighter / Female fighter / Change name
  / Leave store**, and that is what ships.
- **the Fur Shop sells as well as buys** -- its menu is Buy / Sell / Leave
  store, not Buy alone.
- **the office does not hire on the spot.** Choosing a fighter opens a second
  menu, **Hire / Quit / Check status**: the recruit is generated, shown, and
  only then taken or refused. That is built: the candidate is generated into
  the party's own free slot, which nothing reads until `save.n` grows, so no
  byte of state is added for him; Check status shows his name, job, sex, Brave,
  Faith, level and sign; Hire is the only answer that spends the gil. The name
  comes from `ffcp`, the campaign's own generic-name bank. **Ask name again**
  is the menu's fourth row and draws another, never one the party already
  wears, and **Change name** opens a picker over the party and does the same
  for a member who is not Ramza.

  What is *not* built is entry letter by letter. `SaveUnit` carries a generic
  name **index**, not a string: free text would need a name per slot in the
  save -- 16 bytes times twenty, an FSV8 and its migration -- before a single
  letter could be typed. The disc's alphabet runs are in `WORLD/WORLD.BIN`
  whenever that is wanted.
- **the Fitting room is a try-on bench**, not a shop: *Try it on* puts a piece
  of the store's stock on a unit, *Best fit* picks the best the stock offers,
  *Quit fitting* leaves. **Best fit is built** as one bench: the party on the
  left, and under the cursor the best piece the store carries for that unit
  back to the bag. "Best" is ranked on the item's own bonuses, which `ffmg`
  with its price; ENTER buys it and puts it on, the piece it replaces going
  back to the bag. **Try it on is in the same bench**: under the item, the
  unit's HP, MP, PA, MA and Speed as they stand and, where the piece changes
  one, what it would become -- "HP 44>52". "Best" is the piece that makes the
  most of the unit by those same figures, not a rule of thumb.

  The derivation is the campaign's one copy, `unit_stats.inc`, factored out of
  management's own `stats()`: raw stats grown by level with the job's growth,
  then its multipliers, then what it wears. Management's pages come out
  byte-identical, which its PC/68000 replays prove. The bench shows no Move or
  Jump: those take a movement ability's bonus, which needs `ffcj`, and the
  world map does not carry it.

The Bar's own strings are not in this run; `WORLD/WLDMES.BIN` holds its rumours
(the Death Corps, King Omdolia's collapse...), several thousand lines of them.

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

## The portraits

Found: **`EVENT/WLDFACE.BIN`** is the world-map and menu portrait sheet, 4bpp,
256 pixels wide, **168 faces of 32x48**, eight to a row -- not the battle
sheets, and not `EVENT/EVTFACE.BIN` (which holds the 32 event faces). The
shopkeeper is **face 8**, the moustached man of the user's screenshot, and the
**Fur Shop's merchant face 9**, the bald one. Both are INTERPRETATION: matched
by eye against the user's screenshots, not read from the game's own table. The
Soldier Office's guildmaster is blond with a white collar and has not been
found in the sheet yet; he borrows the shopkeeper's portrait, though his name
is right.

The bank carries one speaker a room, so each screen names and shows its own.

**The sheet's own palettes are its last 2048 bytes**: the 168 faces take
129,024, and what follows is 64 CLUTs of 16 BGR555 colours. The 4-bit value is
a palette slot, not a brightness -- a face's skin is index 8 and its hair
index 1..3 -- so reading it as luminance printed the portrait in negative.
Those faces also live in a narrow band (palette 0 runs 5.3 to 26.2 of 31), so
the port's fixed thresholds left every pixel mid-grey: the fifteen drawn
colours are stretched over the full range before the four greys are chosen.

## The Buy and Sell lists

Rebuilt to the source's own screen on 2026-11-20:

- **six category tabs**, by the item's category byte -- weapons 1..18, shield
  19, head 20..22, body 23..25, accessory 26..31, item 32..34 -- each shown by
  the icon of the first item it ever sells, the chosen one outlined.
  **F1 and F5 move between them**, wrapping, as L1 and R1 do on the original.
- **every row carries the item's 8x8 icon**, the one `ffmg` already holds for
  management's Equip screen, then its name, its count when selling, and its
  price (half when selling).
- the list **scrolls** six rows at a time with the shared triangles.
- **[MODE] opens the item's detail** through `item_detail.inc`, the campaign's
  one detail panel: icon, name, family, a weapon's power, **FFT's own
  description** and every non-zero bonus. See [ITEM_DETAIL.md](ITEM_DETAIL.md).
- **Eqp./All** sits beside the War Funds in the same plates: how many of the
  party can wear the item the cursor is on, out of how many are in it.

## In the game

The world map's own furniture moved with it (2026-11-20, the user's layout):
the place the cursor is on is a **floating plate at the top of the screen**,
centred, black with white text, one pixel of air over and under it and no wider
than its name -- never a bar at the foot of the screen. The day sits in a small
panel right under it, a third narrower than before. The town's menu is centred.

`worldmap.c` ships the three screens: the town menu over the world map, the
store's Buy / Sell / Leave store over its room, its item list, and the Soldier
Office's four hire rows. The map shows the same War Funds plate. The item list
takes the room's full height and drops the dialogue box, as the source does.
Steady shop frame: **77,682 cycles** of the 360k budget. The screen is composed
once into an off-screen page of two 100-row planes and blitted; only the glove
is drawn live. Redrawing it whole cost 418,766, over budget. The rooms are
stored at the runtime's own plane stride so they decompress straight into that
page, which costs 6,000 bytes beside the map's strip cache.

A sprite read in place from a bank must start on an even address: the portrait
landed odd and the 68000 took an address error while the PC build was happy.
The bank header is padded; the lesson is in `ti68k-c-patterns.md`.

`make shop-review` renders `x/shop-review/shop-store.png`,
`shop-office.png` and `shop-town.png` through `preview_shop.c` (PC only, no
shipping behaviour, no save write).
