# Shared choice menus

## The pattern every menu follows

Decided with the user on 2026-11-20, after the town menu shipped too wide, with
its glove off the screen and no room under its last row. **Every new menu obeys
these nine rules, and a menu that cannot must say in a comment why.**

1. **One renderer.** `choice_frame` / `choice_row` / `choice_cursor_animated`
   from `../choice_menu.inc`. No menu draws its own frame, rows or hand.
2. **Width fits the longest option, with small margins.** Measure the actual
   options with the screen's own font: `width = widest label + 12` for the
   shared six-pixel text inset. This leaves six pixels before and after the
   longest label (including the border). Never reserve a broad empty area on
   the right, size the body to a long title tab, or retain a fixed-width body
   merely because its cache is wider. The animated glove protrudes outside
   the left border; its size does not add interior padding. If a cache copies
   aligned words, mask its unused pixels rather than widening the visible box.
   User clarification2026-11-22 applies especially to battle Menu and Act.
3. **The same air above the first row and below the last.** With the usual
   `body_y 7`, `row_y 12`, `pitch 9`, that is `height = 13 + 9 * rows`: five
   pixels over the first label and five under the last. The world map's own
   menu (`body_y 3`, `row_y 10`, `pitch 7`, `height 57` for six rows) already
   satisfies it with seven.
4. **The glove is placed by the rule, not by eye.** The sprite is 32 wide and
   14 tall; its pointing finger ends at +16 on rows 4..6 and the sway adds up
   to three. So `hand_x = CHOICE_HAND_X(text_x)` (= `text_x - 21`): at its
   rightmost frame the finger ends at `text_x - 2`: there is **exactly one
   empty pixel** at `text_x - 1` before the label begins. The finger **enters
   the box** at maximum sway; the rest of the glove leans out over its left
   border. Never keep the whole hand outside the body or add padding to make
   room for it. This exact gap and mandatory animation were reconfirmed by the
   user on2026-11-22. Vertically
   `hand_y = CHOICE_HAND_Y(row_y)` (= `row_y - 2`) puts the *finger*, not the
   fist, on the label's middle row; over a 12-pixel pictogram it is the
   pictogram's top row. Both macros live in `../choice_menu.h`.
   The glove must stay wholly on screen: a menu against the left edge moves
   right rather than losing its cursor.
5. **The glove is animated**, always: `choice_cursor_animated(layout, row,
   rt_frame)`, never the still `choice_cursor`. A menu that does not move its
   hand looks frozen.

6. **A list longer than its box shows where it continues.** A solid three-pixel
   triangle at the box's right edge, pointing up when rows are hidden above and
   down when they are hidden below, and nothing when the whole list fits.
7. **A screen made of a picture and a list is composed once, not every frame.**
   The town screens draw the room, their tabs, twelve icons and two plates into
   an off-screen page and blit it: 419k cycles a frame became 78k. Only the
   glove is drawn live, so it still sways. Recompose when anything shown
   changes, never on a timer.

8. **One look for every command window.** The source gives each of its boxes
   the same small title tab -- `Eqp.`, `Ability`, `Menu`, `Help`
   (`fft_guides/orig/mgt_item.png`, `mgt_ability.png`, `item_help.png`) -- so
   every command menu here uses `CHOICE_TAB`, never a second style for the same
   job. Management's Item, Order Unit, the formation's Menu and its Learn/Change
   Job confirmations all go through the one `overlay_at()` in `manage.c`, which
   fits the box to its labels and greys the rows it will not let you pick.

9. **Factor before you add.** A screen that shows what another screen already
   shows reuses that renderer instead of drawing its own: one stat plate
   (`../unit_sheet.inc`, [UNIT_SHEET.md](../UNIT_SHEET.md)), one unit card
   (`../unit_card.inc`), one item detail (`../item_detail.inc`), one ability
   detail. When two screens disagree about a number or a spacing, the fix is in
   the shared renderer, not in one of the callers. Shipping a second copy of an
   existing panel is a defect, not a shortcut. The shared renderer takes a
   presentation snapshot, never the owner's own state: that is what lets the
   management menus, the deployment screen and (next) the battle fill it from
   three different structures.

Management's own Item and Order Unit menus measure their rows with `card_width`
at draw time (`fit_width`), so rule 2 holds whatever the labels say.

Disabled entries are drawn in `C_DGRAY`, as the source greys what it will not
let you pick: Formation's Remove Unit is grey on Ramza, whom FFT refuses to
let go. **[MODE] opens the detail of whatever the cursor is on**, through
the shared `item_detail.inc` ([ITEM_DETAIL.md](ITEM_DETAIL.md)); [MODE] or
SHIFT closes it, and while it is up its owner draws no live cursor over it.
Over an ability (Learn's list, the four ability slots' choices) the same frame
carries the ability's own panel -- name, kind, JP and FFT's text for all 489
described abilities -- and over the Status tab, where the cursor points at no
one thing, it carries the legend of the plate's short names.


`../choice_menu.h` and `../choice_menu.inc` share the command-window frame,
title tab, proportional text rows, white hand and wrapping selection between
battle, management, world map and deployment. Layout owns screen coordinates,
width/height, body inset, title position/width, text column, row origin/pitch,
text limit and hand offset. Font hooks select the existing battle or management
font; callers supply labels and per-choice colors, including disabled entries.
The tabbed battle style, world map's plain inset and management's panel style
retain their existing pictures. `choice_cursor_animated(layout,row,frame)` offers
the battle-style0..3-pixel sway on a32-frame cycle. Every active choice menu
uses this animation (the battle's shared tick or `rt_frame`), without growing
state or save records.

Battle retains its strided cached window, one-part-at-a-time construction,
menu-state validation, ability pagination/scroll indicators and moving hand.
Only the frame/row/hand drawing and ordinary wrapping choice navigation are
shared. Management retains its page cache; world-map travel is unaffected.
There is no extra graphics bank, allocation, persistent state or overlay ABI
change. Shared headers are build dependencies of the affected core and modules.

Deployment now has two explicit choices: Start Battle and Return. UP/DOWN wrap
between them; ENTER/2nd confirms the selected choice, SHIFT/ESC returns without
saving. The existing confirmation byte stores selection+1, so no state/save
format grows. Only Start Battle commits the chosen party. Missing data still
allows ESC to exit.

The direction indicator now follows the user's source photo: a long, hollow,
inclined ground-plane triangle beneath the grid, pointing down-right. Vertices
are(56,75),(61,87),(84,87). The user selected A: a2px contour,
with the lower vertices sharing y87 so its lower edge is perfectly horizontal.
The accepted tip position and12px height are retained. The upper edge follows
the ground grid, with a hollow triangular interior.
The contrasting white halo stays inside the LCD.
The straight-down triangle from review-5 was rejected. The footer leaves space
for the new wedge; the grid, legal slabs, actors and portrait/stats stay fixed.

Deployment's menu hand moves five pixels closer at rest and sways by up to
three more pixels, stopping before the text column. The box height drops33->27
pixels, retaining both choices. Animation is composited on each full deployment
redraw, so neither the hand nor its mask leaves trails; it never changes party
placement or saves. A32-frame idle test verifies visible motion, exact return
to the initial image and unchanged placement/save data.

Review: `x/qa/flow/review-11/new/deployment-slabs-review.png`, actual renderer,
same-scale before/after for initial placement, five units and confirmation.
The review-11 sheet supersedes review-10's A/B decision marker. The hand and box
are unchanged. The arrow now clears the footer, whose separator is continuous.
Footer help retains its two columns. Other decisions stay pending.
Validation: campaign/root/opening unit suites;29 campaign +18 opening/scene +8
deployment PC/68000 replays; all139 existing reference PNGs byte-identical.
The74 Gariland +14 Orbonne flow assertions, saved bytes, repeat payout and
overflow checks still agree. All PC entries build. Native checkers and modules
compile; full-package storage/heap/timing are not claimed. Visual review precedes full
packaging or presentation benchmarks; the user's live emulator is untouched.

Latest scoped validation: deployment idle-cycle/confirmation unit tests and15
PC/68000 input/pixel/state/save replays, including all eight animation phases;
PC deployment builds. These supplement the earlier full suites above.

## Isolated A/B arrow review

`python3 review_deploy_arrow.py` builds temporary source variants without editing
`deploy.c`: current(12px high,1px contour), A(12px high,2px contour),
B(10px high,1px contour). A uses a2x2 contour brush; B lowers the upper-left
vertex by2px, keeping the tip fixed. The sheet includes identical initial/full
placement views and a6x close-up at the same scale for each variant.

`x/qa/flow/review-10/deployment-arrow-ab.png` supersedes review-9's decision marker.
Six actual68000/PC input/pixel/state checks pass. Pixel differences are confined
to the arrow area; the control reproduces review-9 exactly and active source
bytes remain unchanged. The user selected A with a horizontal lower edge; that revision is now in
`deploy.c` and shown in review-11. The A/B script reconstructs its historical
thin/sloped control for reproducibility, without changing the active renderer.

User verdict2026-10-23: review-11 accepted. Its gallery decision marker is
cleared; the selected thick arrow with horizontal bottom remains active.
This does not establish a new full-package memory or hardware result.

## Battle fitting check (2026-11-23)

The accepted Menu/Act bodies fit their actual longest labels (reviewed52/34px)
while retaining the existing four-word strided cache. `window_blit.inc` masks
unused pixels instead of expanding the visible box. Native companion opcode252
is pinned: no per-frame zone change or extra buffer. Actual13-actor static
Menu/Act frames agree PC/TI, ordinary peaks319576/310952 cycles below360k;
cold/turn peaks1639794/1641128 are separate. Other cached modes retain their
reference planes (189 PC checks). The glove follows `CHOICE_HAND_X`, with eight
poses/32-frame return and one blank pixel at maximum sway. See
`x/town-complete/battle-width/glove-results.json` and `menu-frame/results.json`.
