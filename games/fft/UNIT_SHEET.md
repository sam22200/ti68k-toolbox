# The shared unit sheet

Started on 2026-11-22, at the user's request: *« l'écran de stats et de détails du perso
avec l'équipement il faudrait peut-être le mettre en commun dans le détail dans les
combats et dans le déploiement (peut-être aussi dans les shops) … il faut toujours
factoriser ce qu'on peut pour être cohérent »*. This is [CHOICE_MENU.md](campaign/CHOICE_MENU.md)
rule 9 applied to the biggest block of the game: the panel that says who a unit is.

## The contract

`unit_sheet.h` carries one presentation snapshot and nothing else:

```c
typedef struct UnitSheet {
    const u8 *portrait;        /* 32x32 planes, or 0: the owner draws that corner itself */
    const char *name, *job;
    u16 v[US_N];               /* one value a cell, grouped by the column it belongs to */
} UnitSheet;
```

`unit_sheet.inc` draws it: `unit_sheet_plate(&now, &soon, top)`. `top` is where the plate
goes, so the same block serves a screen that wants it at the top and one that wants it at
the foot. `soon` is the same snapshot as it *would* be after the change the cursor points
at; the cells in `US_LIVE` that moved are then drawn inverted -- black ink on a white
cell -- with a triangle for the direction, which is how FFT previews a fitting
(`fft_guides/orig/mgt_item_list.png`). `soon` is 0 when nothing is pending.

A layout table drives the four columns, so adding or moving a number is one row of three
arrays and never a second renderer. The module reads no bank, allocates nothing and keeps
no state: the owner fills the snapshot from whatever it has.

## Where it is used

| Screen | State it reads | Status |
| --- | --- | --- |
| Management, the five tabs | `SaveUnit` + `Stats` | **done** (`manage.c`, `plate_at`) |
| Management, Formation's foot | `SaveUnit` + `Stats` | **done**, replaced the old `UC_FORMATION` card |
| Deployment's head | `SaveUnit` + `Stats` | **done** (`deploy.c`, `plate_unit`) |
| Battle's Status screen | `Unit` (tactical) | **done** (`fft.c`, `status_header`) |
| The sheet's tab bar | -- | **done** (`unit_sheet_tabs`), shared by management and the battle |
| The Eqp. page | `SaveUnit` / `Unit` | **done** (`unit_sheet_equip`), management and the battle |
| The Abil. page | `SaveUnit` / `Unit` | **done** (`unit_sheet_sets`), management and the battle |
| The shop's fitting bench | the fitting draft | **the emphasis is shared**, the plate does not fit there |

Everything above shares one renderer and one set of numbers, so the roster, the deployment,
the shop and the battle can no longer disagree about a unit -- not only about its stats now,
but about its equipment and the abilities it carries.

## The battle's Status screen

It is the sheet, with three tabs of its own: **Eqp. | Abil. | Status**, and it opens on
**Eqp.**, because what a player checks mid-battle is what the unit is carrying.

- **Eqp.** is `unit_sheet_equip`: the source's own slot pictograms down the left, the item's
  own 8x8 icon, then its name -- the management sheet's page, drawn by the management
  sheet's code, in the same order (weapon hand, free hand, helmet, tunic, ring).
- **Abil.** is `unit_sheet_sets`: the job's own **Primary** set above a thin rule because no
  menu can change it, then **Secondary**, **Reaction**, **Support** and **Movement**. Again
  the management page, drawn by the management page's code.
- **Status** keeps what neither the plate nor those two pages carry: the unit's sign, its JP
  and what ails it.
- [2nd] opens the detail of whatever the glove is on -- the item's bonuses and FFT's own
  description, or the ability's name and FFT's own text for it. The keys sit at the tab
  bar's free right end, which leaves the whole body to the page.

Left and right change tab, up and down walk the rows, exactly as the management sheet does.
`st.page` is the tab and `st.status_col` is now derived from it (the detail panel's own
question: an item or an ability), so no state was added.

### The item icons

They come from **`ffmg` section 4**, the management bank's own icon sheet, read in place:
`status_icon()` looks the bank up once, finds the item's sixteen-byte record (section 1),
takes its `I_ICON` byte and points at the 24-byte icon. The battle ships nothing of its own
for them and no bank format changed -- which is why the two routes this file used to list
(an icon byte in `Item`, or a per-chapter icon table) were both dropped: the sheet already
existed, factoring beat adding.

Two real bugs fell out of wiring it:

- **`result_ui.inc` read the icons at stride 14** where an `ffmg` item record is **16**
  bytes, so every reward on the victory screen drew the wrong icon (Bronze Helmet drew the
  empty one). It has been 16 ever since the record grew by its two evade bytes.
- **the battle read `ffid` as FID1** (`memcmp(help,"FID1",4)`, offsets one word short) while
  the bank has been **FID2** -- magic, item count, ability count, then the offsets -- so
  *every item description in battle came back empty*. Fixed, and the same table's second
  half now feeds `ability_help()`, which is what the Reaction / Support / Movement rows
  describe.

### Secondary, wired

It used to be empty, and honestly so: `saved_unit_load` set a unit's skill set from its
**job** (`u->set = job_sets[s->job]`) and dropped the roster's Secondary, so a unit that had
paid for a second command fought without it. The sheet could only print `-`.

It now reaches the battle, end to end:

- `Unit` carries `set2` and `learned2`. The save gives the set; what the unit has learned of
  it lives under **that job's own career row**, so the set names the career to read.
- `known_of(who, set, learned, k)` lists either command; `known()` is the job's own (what
  every existing caller meant), `known2()` the Secondary, `known_cmd()` whichever the menu
  is showing (`st.skill2`).
- The turn menu grows a row when, and only when, the unit has a second command it can act
  with (`ACTS_TWO`): **Attack / Guts / Basic Skill / Defend**. Defend and Equip Change move
  down with it.
- The cached menu window is keyed on which command is listed (the top bit of `win_k[1]`),
  because the two list different abilities under the same mode, scroll and unit -- without
  that the second command would show the first one's list.

`check_secondary.c` holds the whole path: the save's set and learned bits, the two lists not
leaking into each other, the extra menu row and its name, a unit without one left exactly as
it was, and a Secondary equal to the job's own set not shown twice. The 62 replays are
unchanged, because no unit in Gariland carries one -- the party there comes from `unit_defs`,
not from a save.

What is *not* done: the AI still enumerates the primary set only (`area_ai_prepare`,
`restoration_ai`), so an AI unit with a Secondary would not plan with it. No unit in the
chapters that exist has one, so nothing regressed; it is listed here rather than hidden.

### The glove

`choice_menu.h`'s rule 4 says where it goes: `CHOICE_HAND_X(text_x) = text_x - 21`, which
with the sway puts the fingertip two pixels short of the label. The battle had it at a flat
`x = 2` against a pictogram at 20, so the finger landed **on** the picture. Both pages now
take their place from the sheet itself (`US_EQ_HAND_X/Y`, `US_SET_HAND_X/Y` in
`unit_sheet.h`), and management reads the same macros, so the two cannot drift again.

### The slot pictograms were two swapped

`slot_icons.h` is generated in the source's own order -- R.Hand, L.Hand, Head, **Accessory**,
**Body** -- and the pages show the slots weapon, shield, head, **body**, **accessory**. Every
sheet therefore drew a tunic beside the ring and a ring beside the tunic. `us_pict[]` maps
the row to the pictogram once, in the shared renderer.

## The snapshot

`draw_status()` fills a `UnitSheet` from the tactical `Unit` (`status_header`) and draws the
shared plate, in one piece, where it used to draw a light-grey card over three scheduled
frames. A unit therefore reads the same in battle as in the management
sheet, in Formation and on the deployment screen.

Two things the battle has and a saved roster has not, which is why the snapshot grew:

- **live HP and MP.** `max_hp` / `max_mp` make those two cells print `51/51`; a roster
  leaves them zero and the cell prints the value alone, so management is untouched.
- **CT.** It takes the free cell at the foot of the reach column. `shown` says which cells
  an owner actually has, so management leaves CT blank instead of printing a meaningless
  zero -- and the battle, which has every evasion (`items[].sev` per slot, the magic evade
  packed in `items[].pad`, `jobs[].cev`), fills the lot.

What the plate carries came off the page: Brave, Faith, Move, Jump, Speed, PA, MA, CT and
the job level were six text lines, and the Status tab now prints only the sign, the JP and
what ails the unit.

Nothing in the battle's rules moved, so its existing replays stay the reference.

**The module build.** With `FFT_MODULES` the battle is cut into position-independent images
that may carry no relocation, and the Status screen is one of them (`MOD_SCREENS`). The
split follows what each half needs:

- **the plate and the tab bar stay in the core**, because they need the portrait bank the
  core owns. `status_header` is a core function, entered in the module ABI table
  (`module.h`, `module_defaults.inc`) and reached from the screens module as
  `M.status_header` -- the way the unit card already was. `MOD_ABI` stays 94.
- **the two list pages went to the screens module**, beside `draw_status`, which is where
  they belong anyway: everything they need (`ui_text`, `draw_sprite`, `rt_file`, `items`,
  `st`) is already in the ABI. In one piece (no `FFT_MODULES`) that block is skipped, so
  `fft.c` defines `US_PAGES` at the core's include site instead.

That second move was forced, and it is worth recording why: with the pages in the core the
packed battle program reached **65,417 bytes** against `ti-cc`'s 64,936 limit. With them in
the screens module it is **63,981** -- *466 bytes under HEAD*, because the page rows that
used to be written out by hand in `draw_status` are gone and the icon sheet is a bank, not
data in the program. On the Titanium that is not bookkeeping: the battle program is resident
and locked in RAM, so every byte of it is a byte the battle cannot allocate
(`campaign/CORE_PACKAGES.md`). Wiring the Secondary command afterwards put about 950 of them
back.

Two relocation traps, both now documented by having been hit:

- **a helper that touches `st` must be defined after `module.h`**, because `st` is
  `*battle_state` and a module reaches it through the ABI. Putting `status_set_id` in
  `fft.h` brought the image back with two `DISP16 battle_state` relocations.
- **no array of pointers in a module image.** `static const char *const us_set_row[4]`
  cost one `32 .text` relocation; the same four strings returned by a function cost none.

## Cycles: a still full-screen panel costs nothing

The user's own idea, on 2026-11-22: *« quand on affiche le detail c'est du plein ecran donc
en vrai on pourrait ne render plus que ca jusqu'a ce qu'on ESC le detail »*. It holds, and
it is better than that -- such a panel can cost **zero**.

The TI alternates two hidden planes (`GrayDBufToggleSync`), so a picture that reaches both
of them is already on screen for every frame after: as long as nothing on it moves, the
frame has nothing to do. `manage.c` counts the planes it has filled (`settled`) and, while
the modal detail panel is up, returns before restoring anything. Measured with `ti-cycles`
over twenty frames with the panel open: **2,185,844 -> 1,441,272 cycles, a third less**, and
the `memcpy` of the page drops from 47 calls to 15. A still panel is free.

**The battle's Status screen now does the same, and more.** It is the case that matters:
when it is up, `game_render` draws it and nothing else, so what was left was its own
per-frame copy of the cached page. Two measures, on 120 frames of each page with the cursor
walked over the skills list (`make -C games/fft/campaign status-frames-test`):

- the **Status** page settles and then costs nothing at all;
- the **Eqp.** and **Abil.** pages cannot settle, because their glove sways -- so they
  restore only the band the glove stands on (5 words by 14 rows instead of 10 by 100, a
  fourteenth of the bytes), plus the band it has just left when the cursor moves.

Over 360 frames, 120 of each tab, the **full-screen copies fall from 360 to 28**: 222 frames
restore a band and 110 do nothing. Every one of the 360 frames is **pixel-identical** to a
build that redraws everything, which is what the check asserts, along with the copy count.

## What the shop still needs

**The shop has no unit sheet.** Its fitting bench is a one-line strip of five numbers
(HP, MP, PA, MA, Speed) under a list of the party, on top of the room picture; there is no
room there for a 32-pixel plate without covering the room the user approved. What it did
have was its *own* way of showing a change (`44>51`), which is the thing that had to go: it
now calls `us_changed` from `unit_sheet.inc`, so the value the unit would have is lifted out
of the strip with the same white cell and the same triangle as a cell of the plate. One
emphasis for a preview, whichever screen is asking.

Giving the bench the whole plate stays open, and only makes sense once the shop screen is
reorganised: the strip would become the plate and the room would lose its lower third.
