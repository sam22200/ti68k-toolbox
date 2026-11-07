# Title A/B and campaign QA follow-up

2026-10-23. This note supersedes the earlier title/package review status.
The user's live TiEmu, calculator profile and real saves are untouched.

## Title decisions

The current title uses11433bytes in its three TI transfer files,
excluding already shared `ffmg` and runtime assets. A/B storage measurements are
in `x/title-ab/sizes.json`: A11611bytes, B10808bytes; they count the stub, compressed code and title-art
variable once, not both candidates together. Three saves are separate3138-byte
payloads; they are not three copies of the title module. The title's two main
temporary buffers total7138bytes (decoded art4000 + save3138). A verification
buffer is allocated only after the runtime's screen buffers have been freed.

- **A:** original logo/map pixels, with a brighter centre wash applied offline.
  The existing logo is preserved. No new map/actor sprite bank is introduced.
- **B:** the same serif face as the chapter/results strips, on black. Reveal
  FINAL FANTASY TACTICS, then the horizontal rule, then68k, and finally enable
  New Game/Continue. Any newly pressed key finishes the introduction and is
  consumed; it cannot also select a choice or exit. This is a TI adaptation.
- `choice_menu` now supports `CHOICE_BARE`, with no title or frame, and a shared
  option-list renderer. Main choices, file-row labels, selection navigation
  and the animated hand reuse this module. Save summaries, portraits and
  validation remain the title owner's responsibility.

The active default remains variant0 until selection. Review1's decision markers
are superseded by:

- `x/title-ab/title-ab-review.png`: current/A/B at equal scale, same save fixture.
- `x/title-ab/title-ab-animation.gif`: actual C renderer, A/B side by side.

These are marked Decision at the gallery top, including the GIF-only view.
The animation cadence is illustrative PC playback, not measured Titanium FPS.
Source assets remain from the existing local PS1 title capture; no extra PS1
art extraction or invented sprite is used.

## Campaign fixes and evidence

**Normal start memory failure — reproduced, corrected.** The481185-byte group
in `x/qa/followup/package-before.89g` contains a full archived title program,
as well as plain map/management programs. AMS retains their executable RAM
copies under the supervisor. On a private Titanium AMS3.10, normal
`fftstory()` → New Game/Data2 → Orbonne title/chapel → SHIFT reproduces
Missing data or memory exactly. All child entries now use the already verified
small packed stubs. The matching complete candidate is under
`x/qa/followup/package-fixed.89g`; its manifest/hash is `package-report.json`.
The same normal-start sequence reaches Agrias's outside dialogue. A second
invocation with Continue/Data2 reads the newly written native save and reaches
Orbonne's title again. This exercises both post-teardown title writes through
real AMS, in addition to headless byte checks. Native captures:
`x/qa/followup/tiemu/startup-before-after.png`, `continue-slot2.png`.

**Academy oscillation — corrected.** NARRATE's fade-in bit must not initialize
the camera to1px. Camera steps now clamp to their destination instead of
overshooting nonmultiples of8 and oscillating. Unit coverage starts on both
sides of a3px target, including all residual offsets0..23. The native Academy
replay completes with the same state and picture as PC.

**Shortcut99 — corrected.** The previous diagnostic deliberately jumped from
the Academy straight to direct battle door261, then returned HOME. It omitted
deployment, Gariland's title/opening dialogue and the world-map loop. It now
skips Orbonne only, then joins the normal supervisor at the Academy: deployment,
Gariland title, pre-battle dialogue, battle, post-dialogue, results and map.
Normal `fftstory()` remains the complete opening route.

**Premature Congratulations — corrected.** The frame immediately after the
last enemy falls used M_END while the aftermath had not entered yet. Result
drawing now requires the post-scene completion flag. A new native/PC first-frame
fixture checks that the upper battlefield is unchanged before any update, then
checks the resulting actual frame. This covers the automatic-battle outcome as
well as manual attacks, since they use the same end trigger.

**White writing and results — candidate ready.** `serif_reveal.inc` provides one
left-to-right leading edge for the B title and the existing chapter/location/
result strips. Chapter/location overlays reveal rather than appearing whole.
The first result page gradually darkens the scenery; each page reveals its
white text. Fresh2nd or ENTER always advances one page and resets its reveal
clock. The middle-page command and final save/return are exercised with both
keys. Orbonne still has no reward-page sequence. Native equality/reward guards
and payout/overflow checks pass; final visual acceptance is still required.
Marked actual-renderer previews:
`x/qa/followup/animation/scene-title-reveal.gif`, `victory-reveal.gif`.
`victory-order.png` records the last-kill frame and the following dialogue.

## Contours and perspective

`review_edge_width.py` produces `x/qa/followup/edges/edge-width-ab.png`: four
Gariland views,1px versus2px geometric scenery contours, identical units/camera.
Only the geometric stroke width changes; the source depth/mask bytes are checked
unchanged. Active scenery banks are untouched. This interprets "edges" as the
map's relief outlines, consistent with earlier QA; an optional clarification
about scenery versus actor contours is pending. Neither width is newly selected.

**Perspective remains open.** `fftstory` currently ships the chapter engine's
isometric renderer and native low-poly F1/F5 turns. Close/wide fixed perspective
still exists in the separate Gariland configuration/camera prototype, not in
the campaign. This is a packaging/engine integration gap, not a disabled setting
that can honestly be restored by one switch. The existing fixed renderer's
scratch offsets assume a larger nonchapter view allocation. Reusing it in a
chapter needs a bounded overlay/scratch plan, chapter-specific meshes, shared
projection/facings and live actor/mask checks within the tight64KB/RAM margins.
No perspective restoration or Orbonne perspective is claimed here.

## Verification

- Campaign unit suites pass, including title/save/deployment/scene tests.
- 29 campaign +12 opening +6 chapel native replays pass; no golden references
  were rewritten. The Academy-only run also exercises3 native scene cases.
- A18 + B29 title PC/68000 cases pass, including skip, all glove phases, file
  isolation, migration, overwrite, both deferred writes and every visible pixel
  after normalizing preview palettes. Current title retains its18-case suite.
- Flow:81 PC Gariland assertions and14 Orbonne assertions. Native checkers are
  split below the variable-size ceiling:79 Gariland,14 Orbonne and4 dedicated
  first-frame checks. Their pixel/state digests and final saved bytes match PC;
  repeated payout and trophy-stock overflow are checked on independent slot2.
- Actual-renderer result preview advances via ENTER,2nd,ENTER and saves/returns.
- Private Titanium reproduces the old failure, validates corrected normal entry
  and Continue/Data2. This is not a full two-battle hardware playthrough or a
  physical-calculator timing measurement. B06/B07 ghosts and final rotation
  presentation/duration remain open.
