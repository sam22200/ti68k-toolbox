# Monster combat body review (2026-11-14)

**Private visual proposal, not integrated combat.** Formation/Status remains the
approved [FMA1 delivery](MONSTER_FORMATION.md). Real deployment and party loading
still reject type3. This step prepares complete standing, walking, ordinary
level unarmed attack, hurt and Dead body poses for16 selected family appearances;
their48 real jobs remain distinct. No palette reselection, shipping bank, save,
ABI, heap, actor cache or installed package changes.

## Observed in the original

`tools/monster_animation_oracle.py` extends the existing bounded monster oracle.
It executes the retail MIPS SEQ unpacker/player and all three coordinate-shift
helpers. Audio and downstream GPU loading are isolated; EVENT mode prevents
post-attack victim dispatch, continuation is off and the input gate is released.
A dependency guard rejects unexpected execution outside the measured player/
shift functions. Frame, wait, script position, flags, loop/end state, coordinate
offsets and isolated audio calls are recorded. This is a body-timeline fixture,
not a full PS1 battle replay or an oracle for effect/impact synchronization.

The study covers44 distinct front/back timelines,278 displayed records, each
replayed from reset. Direct-pair starts agree with the existing strict reader;
control-leading hurt/attack scripts are executed rather than guessed. Retail
sheet metadata selects hover/flight for Ahriman, Steel Hawk and Tiamat. Normal
walking uses animation0x0c; haste-idle0x04 is not walking. All observed state
flip flags are0; individual SHP part flips remain in the shared compositor.

Attack pictures use a separate compressed page in each SPR, not the idle page.
SHP attack starts are61 for CYOKO,84 for TYPE1,76 for TYPE2 and29 for MON.
The bounded offline nibble-RLE reader agrees byte-for-byte with the original
MIPS decompressor on all16 selected sheets:25600 packed bytes each. Its output
fills200 texture rows; the source32-row portrait strip is copied underneath,
as in the original upload. Four offline boundaries cover empty/truncated input,
an oversized transparent run and an exact full transparent image. Reusing idle
pixels for an attack produces disassembled fragments and is explicitly rejected
by this page selection; do not ship such preliminary captures.

## Visual proposal and evidence

```sh
rtk proxy tools/pyenv/bin/python games/fft/tools/monster_combat_review.py
```

Outputs live only in ignored `x/monster-combat-review/`: `poses-1.png`,
`poses-2.png`, `flight.png`, `animations-1.gif`, `animations-2.gif` and
`results.json`. The two GIFs are the pending gallery decisions. They show
original front/back facings plus explicit mirrors, with the selected family
palette, a white outline and the exact approved uniform scale. All16 original
standing planes remain identical to the accepted FMR1 bank, including when
rasterized into the larger preview canvas. The shared thumbnail converter's
default32x26 output is unchanged.

The1002 body renderings include an outlined union of x[-17,17], y[-27,8]
relative to the feet:35x36 pixels before projecting script offsets. The64x64
study canvas is not a cache format or an increase in the runtime sprite size.
GIF phases restart and sample source waits for review; they do not certify
native movement speed, hit timing or source coordinate-offset projection.
Dead poses retain the original tiny remains where applicable, including the
Behemoth tail. No full-body corpse is substituted.

No TI build, native sweep or TiEmu is needed for this offline-only step.
The approved pictures, historical proofs/saves and prior shipping groups retain
their bytes and dates; `preserved.json`/`verification.json` record the audit.
The current ABI92/FSV7 release remains `x/monster-formation/release/`,733226
transport bytes. Installed16KiB certification remains open.

## Target implementation after visual approval

Bind complete wide animated bodies into the actual battle renderer: standing,
action/KO selection, facing, cover/depth, water crop, damage flash and cache
ownership/eviction. Reuse compatible FMA1 standing/portrait/name records and
existing verified rendering primitives before storing another physical copy.
Keep16 party, four separate guests,13 actors,48 cache entries and the3072-byte
companion reserve. The measured body extent exceeds both the normal16-column
actor slot and Formation's26-row cell; do not crop or independently shrink
each action to force it into those formats. Project recorded SEQ offsets using
the documented world-to-screen convention, or document a reviewed adaptation.
Additional status/ability-specific poses remain outside this basic review.

Only after compatible art/rendering is available may the deployment/party veto
be removed. Then run actual deploy/Save/Continue/battle/results flows on PC and
sanitizers, plus selected native render/cache/CPU checks for those new paths.
Bar/rumors/Propositions and Fur Shop remain after playable breeding and before
campaign extraction. Pending approval is a visual gate, not a new palette vote.
