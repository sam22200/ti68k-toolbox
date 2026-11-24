# Chapter I as one pack: the measured inventory (2026-11-24)

**It fits.** Chapter I's whole content, measured from the local disc through the
production codecs, plus today's core, comes to **1,894,549 archive bytes against a
2,424,754 budget**: 530,205 bytes of margin after the 131,072 reserve, on an empty
Titanium.

This is sizing only. Nothing here authors art, picks a camera, approves a crop or
touches a shipping bank: the final game gets a human-supervised map/scene extractor.
The figures answer one question — does Chapter I have to be split? — and the answer
is no, on storage. It does have two runtime depth overruns to resolve.

Reproduce with `games/fft/tools/pack1_inventory.py`; see section 5.

## 1. Verdict

| Component | Bytes | How |
|---|---:|---|
| Core, 36 archived variables | 694,818 | Measured, ABI99 release |
| 11 battle maps, four views each | 770,348 | **Measured** this pass |
| 9 narrative rooms, one direction, 320px | 46,761 | **Measured** this pass |
| Actor frames, Chapter I cast, native30 | 253,813 | Measured, actor-storage study |
| Referenced event text, 4 KiB ZX0 chunks | 18,432 | Measured, text census |
| Map geometry and descriptors, 11 maps | 86,482 | Carried from the pilot, 7,862 a map |
| World map | 17,674 | Carried from the pilot (`ffwm`) |
| Rumors, pack, save, guest records | 3,714 | Carried from the pilot |
| Archive headers, ~109 variables | 2,507 | Measured rate, 23 bytes each |
| **Total** | **1,894,549** | |
| Budget: 2,555,826 ceiling − 131,072 reserve | 2,424,754 | Measured, `experiments/archfree` |
| **Margin** | **530,205** | |

The margin still has to absorb, and nothing below is in the total:

- flow and dialogue choreography beyond the raw event text;
- encounters, objectives and deployment for the eleven battles;
- chapter portraits still missing from the shared bank;
- the cold text reader and its indexes;
- the ten optional adapted effects;
- the C2 fidelity delta (section 2).

The earlier report put those at roughly 134,000..242,000 bytes together, so the
margin covers them about twice over. It assumes an otherwise empty calculator: run
`experiments/archfree/` on the target machine before treating 530,205 as real.

## 1b. What the actor figure covers

The 253,813 bytes are **60 complete bodies**, one per person, 30 poses each: the 38
generic job bodies plus 22 named humans. Chapter I's own cast is Agrias, Algus, Alma,
Delita, Dycedarg, Elmdor, Gafgarion, Larg, Ovelia, Ramza, Simon, Teta, Wiegraf and
Zalbag — every unique appearance the chapter needs, each with its own physical sprites.

It is deliberately conservative: it also front-loads eight recruits who only appear
later (Beowulf, Cloud, Malak, Meliadoul, Mustadio, Orlandu, Rafa, Reis). Chapter I's
cast alone measures **220,903 bytes**, so 32,910 of the total is future chapters paid
early. Keeping it is the safe reading; dropping it would put the margin at 563,115.

Monster families are already in today's core and are not double counted: `ffmb`,
`ffmc` and `ffmd` hold 135,584 bytes of monster bodies inside the 694,818. Chapter I
has no large unique boss form. Named generic-job enemies such as Gustav, Miluda and
Golagros reuse the job bodies; a proper name does not imply a dedicated bank.

Portraits are the one cast item **not** covered: `ffcp` carries 20,669 bytes of shared
portraits in the core, but the chapter portraits still missing from that bank stay in
the not-counted list of section 1.

## 2. Battle maps

Each map is read from the local disc, rastered in the four orientations and packed
with the production R4 view codec (`map_budget.archive()` → `extract.pack_view`), so
the figure already contains the colour, depth and cover planes.

| Map | Name | Four views | Width |
|---:|---|---:|---:|
| 022 | Magic City Gariland | 70,801 | 300 |
| 032 | Slums in Dorter | 77,878 | 312 |
| 034 | Cellar of Sand Mouse | 60,105 | 252 |
| 049 | Fort Zeakden | 72,778 | 264 |
| 056 | Orbonne Monastery | 77,978 | 288 |
| 074 | Sweegy Woods | 80,274 | 276 |
| 077 | Lenalia Plateau | 65,408 | 264 |
| 085 | Mandalia Plains | 60,932 | 264 |
| 091 | Thieves Fort | 68,837 | 264 |
| 103 | Windmill Shed | 72,810 | 216 |
| 076 | Zeklaus Desert (random) | 62,547 | 336 |
| | **Total** | **770,348** | |

Depth is a separate, runtime constraint and has its own measurement in section 3.

70,031 bytes a map on average. The earlier estimate was 780,736..890,736 for these
eleven maps, so the measurement lands 10,388 bytes **below** its low end.

**Fidelity.** These views use `extract.views()`'s default four greys, not the approved
C2 treatment. Against the two maps the pilot actually ships: Gariland 70,801 measured
against 78,011 shipped (−9%), Orbonne 77,978 against 74,776 (+4%). Treat the total as
a proxy good to about ±10%, which the margin absorbs several times over.

**The old calibration is wrong.** `packages/chapter1-plan.json` scales legacy R2 view
figures by 112%. Measured against those same maps, production R4 costs 2.00× (map 22),
2.22× (74), 2.41× (34), 2.51× (56) and 2.66× (49) the R2 bytes. Any projection still
using the 112% calibration understates map storage by roughly half. This inventory
supersedes it for Chapter I.

## 3. The real blocker: two maps need a shorter scene

`FFT_CORE_DEPTH_MAX` is 18,488 bytes: `fft.c` unpacks one view's ragged depth into
`vbuf` and that buffer is the cap. The stream is

    4 * sc_h          one DepthRow per scene row
  + 4 * sum(span)     four planes over each row's even byte span

so a map's cost splits into a term the authoring pass sets (`sc_h`, which
`chapter_files.inc` already reads per map) and a term the source geometry sets.
Measured with the production encoder, worst view per map, each at its own `sc_h`:

| Map | sc_h | Worst view | Margin |
|---:|---|---:|---:|
| 034 Cellar of Sand Mouse | 178 | 13,448 | 5,040 |
| 103 Windmill Shed | 178 | 12,728 | 5,760 |
| 085 Mandalia Plains | 145 | 15,164 | 3,324 |
| 091 Thieves Fort | 186 | 15,608 | 2,880 |
| 076 Zeklaus Desert | 148 | 15,616 | 2,872 |
| 077 Lenalia Plateau | 180 | 15,808 | 2,680 |
| 049 Fort Zeakden | 192 | 16,400 | 2,088 |
| 022 Magic City Gariland | 202 | 18,304 | 184 |
| 056 Orbonne Monastery | 190 | 18,480 | 8 |
| **032 Slums in Dorter** | 202 | **21,152** | **−2,664** |
| **074 Sweegy Woods** | 202 | **21,640** | **−3,152** |

**Orbonne is not an overrun: it is where the cap came from.** 4 × 192 + 17,720 =
18,488 exactly, its worst view at its shipped 192-row scene. Gariland's measurement
is confirmed against the shipped bank, whose R4 header stores depth = 18,304.

**Two maps have to lose rows, and the measurement says how many and from which end.**
Sliding a full-height window never helps — Sweegy's seven legal windows all give
21,640 — so the scene itself must be shorter. The tallest scene that fits, over every
window of that height with the worst view counted:

| Map | Scene now | Tallest that fits | Rows lost | Cut from | Worst view | Spare |
|---|---:|---:|---:|---|---:|---:|
| 074 Sweegy Woods | 202 | **165** | 37 | 43 rows off the **top** | 18,380 | 108 |
| 032 Slums in Dorter | 202 | **172** | 30 | 54 rows off the **bottom** | 18,440 | 48 |

The two cut from opposite ends because their costs sit in different places. Sweegy's
rows cost 41 bytes at the top, 136 in the middle, 44 at the bottom: its canopy is
cheap, so the sky goes. Slums' rows cost 4 at the top, 151 in the middle, 81 at the
bottom: its sky is nearly empty and cutting it buys nothing, so the foreground goes.

Where exactly to cut is a picture decision and belongs to the supervised extractor.
`games/fft/tools/depth_budget.py` writes the per-row cost of every map so the cut is
priced before it is made, not guessed.

### What was ruled out

Compressing the height axis looks like the obvious fix and is not: 96% of the stream
is span bytes (20,832 against 808 of row headers on Sweegy), and the rows a shorter
height axis removes are the cheap ones at the top.

Three pixel-exact re-encodings were measured and rejected. A span per depth/mask pair
instead of their union, exact byte bounds instead of bounds rounded out to even, and
two runs per row instead of one span, give 19,264 / 20,736 / 18,326 on Sweegy and
19,822 / 20,292 / 19,286 on Slums. Even all three together clear Sweegy by 162 bytes
and still leave Slums over. Three runs is worse than two: the header grows faster than
the recovered gaps. Spans are 67–70% useful, so the holes are real but small.

They also are not free. Exact bounds lose the word alignment `depth_rows()` relies on
— `x0`, `w` and every derived offset are even today, and that XOR loop runs on each
view change, so it would drop to byte operations during animated rotations. Per-pair
spans and runs add a test to the per-actor-row cover path in `seen()` and the sprite
blend, against a measured 13-unit Zeklaus peak of 356,832 cycles on a 360,000 budget.
Spending 3,168 cycles of margin to gain 162 bytes on one map of two is a bad trade.

Raising the cap is also closed: `INSTALLED_RAM_ABI99.md` measures 436 free bytes on
that same 13-unit case, against the 3,152 Sweegy would need.

## 4. Narrative rooms

Nine scene-only locations, both source directions, three canvas widths, six picture
planes each, ZX0 with a verified round trip. Same bounded probe as the accepted
`x/pack-refinement/measure_scene_compression.py`, so the figures stay comparable.

| Width | 9 rooms, dearer direction | Cheaper direction | Worst room |
|---:|---:|---:|---:|
| 160 | 36,626 | 27,786 | 4,977 |
| 256 | 45,524 | 36,533 | 5,982 |
| 320 | 46,761 | 37,669 | 6,100 |

Section 1 uses the 320px dearer-direction column, since which direction each scene
takes is an art decision this pass does not make. Against the earlier allowances —
45,000..120,000 one-direction, 72,000..180,000 conservative — the measurement comes in
at 46,761, below even the optimistic low end. Scene pictures are not a budget risk for
Chapter I; at 320px they are 2.5% of the pack.

Per room at 320px, direction 0 / direction 2: Citadel of Igros 4,451 / 5,325; Office of
Igros 4,055 / 3,360; Beoulve residence 2,811 / 5,423; Academy Auditorium 4,302 / 5,881;
Orbonne Chapel 4,713 / 6,100; Windmill interior 3,747 / 4,345; Wooden shed 4,581 /
4,805; Stone shed 5,619 / 5,838; Beoulve Room 4,989 / 4,085.

## 5. Reproduction

```sh
tools/pyenv/bin/python games/fft/tools/map_budget.py \
  --maps 22,32,34,49,56,74,76,77,85,91,103 \
  --archive 22,32,34,49,56,74,76,77,85,91,103 \
  --out games/fft/campaign/x/pack1-inventory/pack1-maps.json
tools/pyenv/bin/python games/fft/tools/pack1_inventory.py \
  --maps-json games/fft/campaign/x/pack1-inventory/pack1-maps.json \
  --out games/fft/campaign/x/pack1-inventory/inventory.json
tools/pyenv/bin/python games/fft/tools/depth_budget.py
```

The map pass takes about four minutes, the scene pass about ten; `--scenes-json`
reuses a previous scene run. Outputs live in `x/pack1-inventory/` and stay out of Git
with the rest of the extracted commercial material. No emulator, no installed bank, no
save and no shipping asset is touched.

## 6. What this changes

- Chapter I stays **one pack**. The split contingency in the pack report's gate 1 is
  not needed on storage grounds.
- The pack report's Chapter I high forecast (−22,918 one-direction, −82,918
  conservative against the measured ceiling) was driven by allowance ranges, not by
  content: measured content is 530,205 bytes inside the budget.
- Two maps need a shorter scene, and the numbers say which rows: Sweegy Woods to 165
  rows by losing 43 of sky, Slums in Dorter to 172 by losing 54 of foreground. Slums
  in Dorter is a new finding; the cached thirteen-map census covers map 031 instead.
- Orbonne is not an overrun. `FFT_CORE_DEPTH_MAX` was sized on it, so the earlier
  reading that it exceeded the cap was an artefact of assuming a 202-row scene.
- The previously quoted depth figures came from `map_budget.ragged()`, a second
  implementation of the format that keeps different rows. Section 3 uses the
  production encoder and is confirmed against the shipped Gariland bank.
- `chapter1-plan.json`'s 112% scenery calibration is superseded for Chapter I.
