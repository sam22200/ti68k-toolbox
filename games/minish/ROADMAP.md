# The Minish Cap: Minish Woods

Confirmed: a native engine on the Portable Game Runtime, with behavior
measured on the local USA GBA ROM; the first screens of Minish Woods are
the requested setting. This document does not redefine that scope.

The user authorized the next milestone. Provisional M1 choices: the western
opening, three horizontal GBA view widths (720 pixels) with a 320-pixel
vertical exploration margin; 1:1 geometry, a tighter 160x100 TI camera and
walking only. These choices can be revised without changing the engine.
M1 starts at (248,88) in the main alley; the earlier (32,88) reference probe
is an isolated clearing, retained as a scenario. M2 replaces M1's collision
visualization and placeholder with original scenery and outlined animated
Link. ENTER resets the entrance, ESC exits.

The user additionally requested a version zoomed out by about 30%. This is
the separate `minishz` build at 70% visual scale, keeping the same source
geometry and opening scope alongside `minish`.

| Milestone | Acceptance | Status |
|---|---|---|
| M0: reference access | Cold boot directly loads the original forest; input moves Link; offline maps/metatiles/types match loaded RAM; saved frames replay exactly | Complete |
| M1: native traversal | Walk the main alley to the third view of the 720x320 opening, with measured movement, partial collisions, corner slides, slope speed, camera and PC/TI equality below 360k cycles | Complete: 6310 original steps; 3560 native hashes, 10 screens; peak <=246762 cycles |
| M2: native scene art | Original scenery and animated outlined Link read clearly in four greys at the provisional 1:1 scale; extraction fidelity, walking/idle poses, canopy occlusion and PC/TI screens checked below 360k cycles | Complete: 44 poses, 16.8M source RGB pixels, 400 animation steps; 3606 native hashes, 56 screens; peak <=296152 cycles |
| M2 variant: 70% view | Additional playable build; scenery and Link reduced offline, source movement retained, canopy/clipping and all camera phases checked, normal version retained | Complete: 1.312M oracle pixels, 3642 native hashes, 92 screens; peak <=306898 cycles |
| M3: sword and bushes | A/2nd swings the ordinary sword; measured action timing and tile samples, original attack poses, cutting real bushes changes art and collision, reset restores the forest; both scales checked below 360k cycles | Complete: 40 poses, 1947 source steps covering all 53 bush cells, 672k additional oracle pixels per build; 4639/4675 native hashes, 216/252 screens; peak 326918/331056 cycles |
| M4: first enemy encounters | Original opening Octoroks, sword hits, movement/projectiles, contact damage, knockback and hearts on both scales; reset and PC/TI equality below 360k cycles | Complete: 20 source trials / 608 checked updates, 20 enemy poses and 960k oracle pixels per scale; 7521/7557 native hashes, 346/382 screens; peaks 359248/348320 cycles; both enemies defeated by the native demo. HUD polish removes the heart panel; nearby targeting and larger round balls make shots visible. |
| M5: roll and destruction effects | Measured directional roll with terrain collision, original roll poses, bush-cut animation and persistent readable earth, enemy death animation; both scales, save/reset, PC/TI equality and dense-effect frame budget | Complete: 330 source roll updates, 49 poses and 2352000 oracle pixels per scale; 9990/10026 native hashes, 703/739 screens; peaks 357666/343636 cycles. B (Shift/X) maps the original R roll. |
| Additional actions | Other destructibles, water/pit actions and story interactions as measured playable milestones | Later milestones |

The minimal source study save is not an assertion that the original story
has been played to the forest. M4 uses a separate enemy-enabled study save;
its local AI, eight-way recoil, death/removal, rock impact/expiry and retry
are native adaptations. Original bouncing deflections, drops, other objects,
story interactions, water/pit actions and room transitions remain later work.
