# Shared choice menus

## The pattern every menu follows

Decided with the user on 2026-11-20, after the town menu shipped too wide, with
its glove off the screen and no room under its last row. **Every new menu obeys
these five rules, and a menu that cannot must say in a comment why.**

1. **One renderer.** `choice_frame` / `choice_row` / `choice_cursor_animated`
   from `../choice_menu.inc`. No menu draws its own frame, rows or hand.
2. **Width fits the labels.** `width = widest label + 12`, measured with the
   screen's own font, never a constant chosen by eye. A menu is as wide as it
   needs and no wider.
3. **The same air above the first row and below the last.** With the usual
   `body_y 7`, `row_y 12`, `pitch 9`, that is `height = 13 + 9 * rows`: five
   pixels over the first label and five under the last. The world map's own
   menu (`body_y 3`, `row_y 10`, `pitch 7`, `height 57` for six rows) already
   satisfies it with seven.
4. **The glove is wholly on screen.** The hand sprite is 32 wide and its
   pointing tip sits at its right edge, so `x + hand_x >= 0`: with the usual
   `hand_x -16`, the menu starts at `x >= 16`. A menu against the left edge
   moves right rather than losing its cursor.
5. **The glove is animated**, always: `choice_cursor_animated(layout, row,
   rt_frame)`, never the still `choice_cursor`. A menu that does not move its
   hand looks frozen.

Disabled entries are drawn in `C_DGRAY`, as the source greys what it will not
let you pick.


`../choice_menu.h` and `../choice_menu.inc` share the command-window frame,
title tab, proportional text rows, white hand and wrapping selection between
battle, management, world map and deployment. Layout owns screen coordinates,
width/height, body inset, title position/width, text column, row origin/pitch,
text limit and hand offset. Font hooks select the existing battle or management
font; callers supply labels and per-choice colors, including disabled entries.
The tabbed battle style, world map's plain inset and management's panel style
retain their existing pictures. `choice_cursor_animated(layout,row,frame)` offers
the battle-style0..3-pixel sway on a32-frame cycle. Other callers retain their
existing hand policy; deployment uses the shared animation with `rt_frame`,
without growing state or save records.

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
