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
| The shop's fitting bench | the fitting draft | **the emphasis is shared**, the plate does not fit there |

The three that are done share one renderer and one set of numbers, so the roster, the
deployment and the sheet can no longer disagree about a unit.

## The battle's Status screen

It is the sheet, with three tabs of its own: **Eqp. | Abil. | Status**, and it opens on
**Eqp.**, because what a player checks mid-battle is what the unit is carrying.

- **Eqp.** draws the source's own slot pictograms (`campaign/data/slot_icons.h`, the ones the
  management sheet uses) down the left and the item's name beside them, in the same order:
  the weapon hand, the free hand, the helmet, the tunic, the ring.
- **Abil.** lists what the unit goes into battle with.
- **Status** keeps what the plate cannot carry: the unit's sign, its JP and what ails it.
- [2nd] opens the detail of whatever the glove is on -- the item's bonuses and FFT's own
  description, or the ability's range, height, JP and effect. The keys sit at the tab bar's
  free right end, which leaves the whole body to the page.

Left and right change tab, up and down walk the rows, exactly as the management sheet does.
`st.page` is the tab and `st.status_col` is now derived from it (the detail panel's own
question: an item or an ability), so no state was added.

**Item icons are still missing from the battle's Eqp. page.** The battle's `Item` record
(`fft.h`) carries no icon index and no chapter bank ships the 8x8 icon sheet, which is why
the management sheet has them and this one has not: there they come from `ffmg` section 4,
which the battle does not load. Two ways out, neither free: give `Item` an icon byte and let
`campaign_assets.py` emit the chapter's own icons beside the table (about 120 bytes for
Gariland's five items), or let the battle read `ffmg` in place for presentation only. The
first is the clean one; both change what a chapter package carries, so it is a packaging
decision rather than a drawing one.

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
that may carry no relocation, and the Status screen is one of them (`MOD_SCREENS`), while
the shared renderers live in the core. So the plate is reached the way the unit card already
was: `status_header` is a core function, declared among the core's prototypes, entered in the
module ABI table (`module.h`, `module_defaults.inc`) and reached from the screens module as
`M.status_header`. `MOD_ABI` goes 92 -> 94 for the new entry.

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
