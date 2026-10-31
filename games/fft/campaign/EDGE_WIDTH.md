# Temporary selectable scenery width

**Superseded2026-10-24:** user selected1px. Active V2 banks and MODE menu now
retain1px only, removing2px stroke streams and the width toggle. Shared glyph
font is accepted/integrated. Current group and QA:
[QA_FOLLOWUP_2026-10-24.md](QA_FOLLOWUP_2026-10-24.md).
The remainder records the historical comparison, not active controls.

2026-10-23 user decision: retain both1px and2px for an actual in-game test.
This authorizes a configurable test build, not a final visual choice.
Default remains1px. Perspective is explicitly retired to experiments.
The subsequent animated comparison did not settle the choice: the user will
play to assess it. Both widths stay available. Its review markers are cleared
while the white-title GIFs are the current visual decisions; no width is approved.

## Controls and scope

In Gariland, open **MODE** (CATALOG remains an alias), select **Edges**, then
press ENTER/2nd or left/right to toggle1px/2px. MODE, SHIFT or ESC closes it.
Up/down navigates the settings. Held keys are consumed once. Battle state,
CT and RNG pause while this menu is open. The selected width survives menu
changes and rotations during that battle; a fresh battle starts at1px.

The menu also retains **Move: full tiles / contour**. The former MODE direct
highlight switch is now the explicit **Tiles: covered / over scenery** row.
APPS still opens the turn list. Settings remain blocked during AI actions,
attacks, rotation and results, as before.

Only Gariland has the reviewed width pair. Orbonne keeps its existing scenery
and displays **Edges: unavailable**; it does not silently claim a2px option.
Actor outlines, tactical cells, positions, depth and collision are unchanged.

## Bank format and memory

Gariland uses V3 split views: `V3`, BEu16 colour-stream length, BEu16 width-mask
length, ZX0 default1px colours, ZX0 one-bit changed-pixel mask, ZX0 shared
row-XOR depth/masks. V2 remains supported for Orbonne and historical controls.
The shared UI/table tail still uses its existing aligned FC02 footer.

On complete colour reload, applying the width mask sets both pixel planes to
the complement of the old dark bit at changed pixels. This reconstructs every
byte of the previous2px candidate, including adaptive white/black contrast.
The128-row colour-backup buffer is obsolete at that point and safely holds
the8080-byte mask. Backups are reset after applying it. Depth is never used as
mask scratch and needs no extra decode on colour-only ring restoration.
No new heap block or resident full-map colour copy is allocated. The state
preference and module context add only small metadata; ordinary cached frames
do no extra edge drawing.

The four masks total6130 bytes; four larger headers add8 bytes. Shared settings
strings and code/module metadata also grow. The complete42-variable test group
is **472686 bytes**, up7082 from465604. It contains no perspective mesh/image
banks. `x/edge-width/package.json` records the final hash/manifest.

The expanded configuration code would exceed the packed battle core's limit.
It now uses operations2/3 of the existing Status/results module. No new module
slot or executable was added. Ordinary game frames bypass the module call
unless a settings key is pressed; paused settings use the loaded module.

## Checks and practical cost

`make -C games/fft/campaign edge-width-check` performs82 assertions and checks
all8 native/PC pictures, both widths in all4 orientations. Source fixture
colour hashes, hashes across all depth bytes, real/held inputs, frozen units/tick/RNG,
warm caches, interrupted Move highlights and complete backup-overflow
restoration pass. The fixture uses the pre-existing isolated2px V2 bank and
the source-handed1px raw views as independent expected colours.

The corrected split-view reach oracle reads pristine colours from the archive,
instead of treating depth in vbuf as colours. It passes320 Gariland scene
comparisons (two widths and two reach styles) and160 Orbonne comparisons.
Campaign unit suites pass. Gariland/Orbonne flow checks retain81/14 PC asserts
and the split79/14/4 native checks, with identical saves, no repeated payout
and trophy overflow handling (`x/edge-width/flow/`).

Actual68000 datasheet counts for complete colour reload, four views averaged:
1px957885 cycles (79.82 CPU ms at12MHz),2px1627434 (135.62ms). The additional
55.80ms occurs on full colour reload, including entering another orientation
or the uncommon backup overflow, not on ordinary cached frames. These figures
exclude grayscale/presentation and are not TiEmu or physical-calculator timing.
No new full-frame rate or rotation presentation claim is made.

The full packed group builds. No emulator session or real save was touched.
In-game user testing is still pending. Actual-renderer comparisons
`x/edge-width/edge-width-runtime.png` and `settings-review.png` are marked
Decision in the gallery; they supersede the old isolated width sheet without
declaring either width accepted.

The requested animated decision review is `x/edge-width/edge-width-decision.gif`,
also pinned Decision, including the GIF-only gallery. Reproduce it with
`make -C games/fft/campaign edge-width-review`. It compares both widths side by
side at identical logical ticks, with actual animated idle actors and water,
48 images per orientation and direct cuts through all4 views. Playback is
13.44 seconds,192 GIF frames, not a TI speed measurement or a recording of F1/F5
rotation. Intermediate captures stay in a temporary directory. The renderer,
matching poses and frame counts are recorded in `animation.json`.

## Perspective and SVG decisions

The renderer's code is shared, but every perspective-enabled map needs extra
archived geometry/visual data: the existing Gariland prototype has20487 such
bytes. This is separate from RAM and does not mean another35–40KB engine per
map. The user therefore excludes perspective from the campaign. The entry
point is `experiments/fft-perspective/`; chapter builds reject
`FFT_EXPLORATION`. Existing F1/F5 isometric low-poly rotation stays active.

The shared `lib/svgfont.py` converter still generates the old combat banners
on the host through `tools/banner.py` and `tools/interface.py`. It is not an
on-calculator SVG renderer or a perspective dependency. Current serif flow
titles still use offline strips; the shared dynamic glyph font in
`UI_SIZE_STUDY.md` remains implementation work. Converter extraction preserves
the old glyph/strip bytes exactly.
