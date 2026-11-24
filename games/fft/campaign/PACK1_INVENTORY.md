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

## 2. Battle maps

Each map is read from the local disc, rastered in the four orientations and packed
with the production R4 view codec (`map_budget.archive()` → `extract.pack_view`), so
the figure already contains the colour, depth and cover planes.

| Map | Name | Four views | Depth max | Rows | Width |
|---:|---|---:|---:|---:|---:|
| 022 | Magic City Gariland | 70,801 | 18,288 | 204 | 300 |
| 032 | Slums in Dorter | 77,878 | **18,776** | 258 | 312 |
| 034 | Cellar of Sand Mouse | 60,105 | 13,460 | 179 | 252 |
| 049 | Fort Zeakden | 72,778 | 15,872 | 192 | 264 |
| 056 | Orbonne Monastery | 77,978 | 11,592 | 292 | 288 |
| 074 | Sweegy Woods | 80,274 | **19,304** | 244 | 276 |
| 077 | Lenalia Plateau | 65,408 | 15,608 | 180 | 264 |
| 085 | Mandalia Plains | 60,932 | 15,620 | 171 | 312 |
| 091 | Thieves Fort | 68,837 | 15,672 | 186 | 264 |
| 103 | Windmill Shed | 72,810 | 13,408 | 210 | 216 |
| 076 | Zeklaus Desert (random) | 62,547 | 17,040 | 180 | 336 |
| | **Total** | **770,348** | | | |

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

## 3. The real blocker: two depth overruns

`FFT_CORE_DEPTH_MAX` is 18,488 bytes of ragged depth. Two Chapter I maps exceed it:

| Map | Depth per orientation | Max | Over |
|---:|---|---:|---:|
| 074 Sweegy Woods | 16,960 / 18,608 / 17,432 / 19,304 | 19,304 | +816 |
| 032 Slums in Dorter | 13,224 / 15,784 / 18,776 / 17,800 | 18,776 | +288 |

Sweegy was already a known Chapter I gate. **Slums in Dorter is new**: it is not in the
thirteen-map census `chapter1-plan.json` caches, which covers map 031 Dorter Trade City
instead, so nobody had measured it. Both are runtime buffer limits, not Flash, so they
do not move the verdict in section 1 — but neither map is playable until its depth is
adapted or the buffer grows, and that adaptation is art-adjacent work for the
supervised extractor.

Every other map fits with at least 466 bytes to spare (077 Lenalia, 18,022).

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
- Two depth adaptations are now the Chapter I blocker, Slums in Dorter being new.
- `chapter1-plan.json`'s 112% scenery calibration is superseded for Chapter I.
