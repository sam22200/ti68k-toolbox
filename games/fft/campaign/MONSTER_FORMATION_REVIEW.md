# Standing monsters: one appearance per family (2026-11-14)

Branch `feat/fft-monster-formation-review` prepares a PC-only picture proposal
for the next [breeding gate](BREEDING.md). It is not integrated. The current
FSV7/ABI92 release under `x/eggs/release/` remains the shipping baseline.

**User decision:** use only the most readable appearance of each family; the
three monster types are distinguished by their job names. Palette choice was
delegated to the agent. The old three-appearance proposal is superseded; do not
ask for another palette selection. `tools/monster_appearance.py` records the16
chosen source jobs for reuse by later builders. Real job IDs, names, stats,
abilities, offspring and saved identities remain48 distinct entries.

The user then approved the selected picture ("oui tres bien") and extended the
same no-color-swap policy to human art. See [CORE_RESOURCES.md](CORE_RESOURCES.md):
one chosen palette per distinct model, retaining real design/identity differences.
No further palette-selection approval is pending.

## Review pictures

Current files are under `games/fft/campaign/x/monster-family-review/`; the old
three-palette pictures/proofs under `x/monster-formation-review/` are preserved.

- `review.png`: current/proposed Formation, Behemoth Status and placement at
  the same view and3x nearest scale. The placement fixture bypasses the closed
  selectors solely to show bodies on the existing deployment grid.
- `atlas.png`:16 selected family appearances, each beside its three real job
  names. Each cell shows source RGB, four greys and the portrait. This is a
  static first-idle review, not action animations or a combat-art census.
- Individual160x100 pages include `formation.png`, `status.png`,
  `status-chocobo.png`, `status-hydra.png` and `deploy.png`.

The production grid, font, floor diamond, header, stat bars and deployment
orientation are reused. The proposed actors fit32x26 cells with the source
feet origin. Families stay at0.6 or shrink uniformly to fit their full body
and one-pixel white outline; Behemoth is0.547, Chocobo0.533. No opaque source
part is cropped to the human16-column format. Each family's three jobs share
one physical body and portrait at the same scale. Monster portraits use the
existing32x29 converter and SPR CLUT8+selected palette. There are no human
eye/skin corrections. Selection considers both internal body details and the
portrait at native size; a simple variance score is not the visual verdict.

Proposed cards show the monster's own generated name and its species rather
than repeating the species twice. The Status proposal suppresses human Job Lv
and Total JP. Human pages, the egg and all actual rules remain unchanged.
The Behemoth fixture uses the source name Apomorph; its statistics are private
render-fixture values, not a retail-generated combat snapshot.

## Source and reuse

Existing human/object/egg banks contain no compatible adult bodies. FKO1 holds
only Dead poses; its original source compositor/first-pair SEQ reader already
provides the required SHP parts, flips and feet convention. This review reuses
those helpers, the existing palette thresholds, portrait converter, shared
glyphs and actual management/deployment pages. No new image generator is used.

Retail job records supply the SPR sheet and palette, and the BATTLE.BIN extent
table resolves filenames.16 unique sheets reuse four SHP/SEQ families; first
idle is animation6. The previous monster-KO original-MIPS evidence covers these
four families' first-pair policy; it is not rerun for a static art proposal.
The job/sheet/shape/SEQ inputs and source pages have SHA256 records in the
private results. Monster names use the same SMALL.OUT text list as the human
name reader, at the source monster class0x0300/text0x4300, entries768..1022.

`review_monster_formation.py` generates explicit source copies under the private
output directory. Anchored hooks fail if their production source no longer
matches. Only those copies gain the proposed names, portraits,32-column actors
and monster Status fields. The source files and shipping banks are not edited.
Private FMR1 uses a48-byte job-to-frame directory,255 names and16 physical
body/portrait pairs:14696 bytes, replacing35768 in the former FMR0 review.
This is a private carrier, not a new installed format or shipping-size claim.
Any eventual ownership and storage design follows [CORE_RESOURCES.md](CORE_RESOURCES.md).

## Verification and next step

```sh
rtk proxy tools/pyenv/bin/python games/fft/campaign/review_monster_formation.py
rtk proxy tools/pyenv/bin/python games/fft/campaign/review_monster_formation.py --sanitize
```

Eleven actual-page scenarios match on PC and ASan/UBSan, with the entire Save
unchanged after each rendered frame and the temporary session file unchanged.
All48 job references share the correct family record, and the actual Chocobo/
Black Chocobo cards retain distinct job names despite sharing their picture.
Shape composition rejects missing/truncated parts and out-of-sheet opaque
pixels; fitted masks retain space for their whole outline. This does not
certify native cycles, arithmetic, pointers, layout or installed memory.
No TI build, native sweep, TiEmu, CPU benchmark or package rebuild ran.

**Next:** bind the selected art
to management/Status/deployment with real asset availability, egg exclusion and
durable placement. The battle loader currently rejects every type3 unit too.
It needs matching standing/action art, correct palette/job identity, wider-body
cache/cover handling and KO support before allowing an adult into combat. Do
not remove only the menu veto or substitute human art. FKO1's six palette-zero
Dead samples do not cover all16 selected families/appearances.

Keep16 ordinary party slots, four separate guests,13 active actors,48 cache
entries and3072 companion bytes. Bar/rumors/Propositions, Fur Shop and the
installed16KiB/peak gate remain open. Historical proofs, saves and their dates
stay protected; the live emulator is untouched.
