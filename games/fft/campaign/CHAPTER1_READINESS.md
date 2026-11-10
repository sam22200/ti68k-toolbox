# Chapter I readiness: next steps (2026-11-08, battle RAM updated 2026-11-10)

Chapter I has about 20 battle maps and 20 scenes; the pilot ships 2 of each. Tackle these in
order before producing Chapter I content. State of the 2026-11-08 session: saves as in FFT,
scene skip, hidden DEBUG page, APPS selection, live AI-pause animation, `fftmem.py`/`tilog`
instrumentation and the `fftcore` TiEmu state ([CORE_PACKAGES.md](CORE_PACKAGES.md), "Saves,
debug aids and logs" and "Native test loop").

## 1. Battle RAM margin (first, mandatory)

**Status 2026-11-10: ~11.4 KB free on every map (was ~1.2 KB); 2 and 3 below remain to reach
the 16 KB target.** Measured natively (Titanium AMS 3.10, Orbonne, `ti-mem heap`): 187,554
bytes free when the supervisor calls `fftbat`, the battle reserves 176,158:

| Block | Bytes | Notes |
|---|---:|---|
| decompressed program | 64,934 | ~56 KB code + module zone 8,690 |
| `sbuf` | 53,622 | scene colours 2×40×202 (16,160, hot path) + Work 37,460 |
| `vbuf` | 20,482 | **ragged depth, `FFT_CORE_DEPTH_MAX` 20,480** (was 32,322 full planes) |
| `acache` | 10,178 | 48 actor frames: keep (CORE_OPTIMIZATION rule 1) |
| `navbuf` | 5,122 | **the map envelope, 256 nodes × 20 bytes**, reserved once (was per map: Gariland 3,002) |
| grayscale | 7,688 + 7,690 | hidden render target (copy-present mode) + visible planes |
| `battle_state` | 5,428 | State 2,224 + Save 3,202 (+ malloc handle word) |
| stub copy | 1,008 | freed by the supervisor after the battle (`SymDelTwin`) |

**Done (2026-11-10), pixels and states identical** (37 PC/68000 replays compared with the
previous build: same checksums and state hashes; rotation, range-cancel, banks, AI-pause,
headroom, area-impact and package checks pass):
1. **Ragged depth.** A view's depth stream (`extract.ragged_depth`, banks `R2`/`R3`,
   `extract.view_bank`) stores per scene row the even byte span holding mask bits, then the
   four planes as their rows' spans; masks stay row-XORed where spans overlap. `fft.c
   depth_rows()` fills each `DepthRow` offset; readers use `DEPTH_ROW()`/`cover_bytes()`
   (`cover()` keeps its own inline loop: 13 units after a turn). Views: Orbonne 16,400–18,488
   bytes, Gariland 16,968–18,304; the envelope `FFT_CORE_DEPTH_MAX` (`core_limits.h`) is 20,480
   and a bigger view is refused by `chapter_desc.py` and at load (`chapter_files.inc`).
   Archive: −1.7 KB for the 8 pilot views (smaller ZX0 streams). Local banks are converted by
   `tools/view_split.py opening campaign` (idempotent). CPU: view unpacks are 75–150k cycles
   faster; the frame that ends a turn recomputes 13 covers +20–31k (563k instead of 532k, a
   stall frame); the rotation scenario's 100 frames total less than before.
2. **Map envelope.** `navbuf` is reserved once for `MAP_NMAX` (256) nodes in the cold
   bootstrap: no map can fail on it.
3. **A real message.** "Not enough RAM / need 93 KB, free N KB" (RESOURCES op 12, cold; the buffers
   allocated after the program and grayscale planes, 94,842 bytes);
   "Missing data" otherwise. Free = AMS's free RAM + what the battle already holds.

**Remaining to reach 16 KB (~4.6 KB):**
1. **100-row hidden gray buffer (−1,688).** Copy-present mode never flips it, but every draw
   path of `fftbat` must stay within 100 rows first: `rt_ti.c` sprite clipping allows rows
   up to 127 (`RT_PH`), `fft.c` clears `RT_PSIZE`, and the copy to the visible planes is
   `FastCopyScreen_R` (3,840 bytes: a 3,000-byte C copy costs about the same).
2. **Ring backups by row span (−~2.5 KB of Work, `W->bk` 10,240 bytes).** Keep the
   800k-cycle re-decode fallback out of reach; `W->bk` also serves as the width-mask scratch
   (`EdgeMaskFitsBackups`, 8,080 bytes).
3. If still short: tighten `FFT_CORE_DEPTH_MAX` once Chapter I's views are measured (each
   KB of envelope is a KB of RAM), or give the second depth layer its own span (~−0.7 KB).

**Every map of the game against the envelope (2026-11-10, `tools/map_budget.py`).** The
battle's RAM depends only on the map being fought, never on how many maps a chapter has; with
the envelopes it is the same number on every map, so the question is whether each map fits
them. The census rasterizes the 109 maps the game references (`maps.json`) in the four views
and counts their ragged depth (checked on Gariland: 16,808–18,288 estimated, 16,968–18,304
shipped). "Uncropped" bounds any crop; the TI canvas is 320 × 202 (`SC_HMAX`).

| Maps | Uncropped depth | Rows | Width | Verdict |
|---|---:|---:|---:|---|
| St. Murond Temple 050/051, Hall 003, Chapel 052 (Loffrey, chapter 4) | ≤ 19,348 | 186–222 | ≤ 288 | fit 20,480 (050 needs a 202-row crop) |
| Chapter I: 056 Orbonne, 022 Gariland, 085 Mandalia, 031 Dorter, 034 Sand Rat, 091 Thieves Fort, 077 Lenalia, 072 Fovoham, 073/103 Windmill, 049 Zeakden, 024 Academy | ≤ 19,760 | 171–292 | ≤ 312 | fit |
| 074 Sweegy Woods | 22,184 (19,304 cropped to 202 rows) | 244 | 276 | fits once cropped to 202 rows |
| 076 Zeklaus Desert | 17,248 | 180 | **336** | **wider than the 320-px scene**: crop 16 px or widen the scene (RAM) |
| 26 other maps (worst 029 Colliery 2F 26,088; 202-row crop: 100 Public Cemetery 23,032) | 20,700–26,088 | | | over: an envelope of ~23,552 (+3 KB RAM) would hold them all once cropped |

Rows above 202 are a crop decision per map (Orbonne keeps its top and cuts its bottom). 22
maps are wider than 320 px (`over_width` in `x/map-budget.json`), independent of RAM.

**What does not cost RAM.** The number of maps, scenes and battles in a pack: banks, scene
pools and manifests are read in place from the archive (one VAT entry each).

**Resident headroom.** TTPack input 64,829 of 64,936 (107 bytes left, `tools/headroom.sh`;
was 397): ring/cursor/cover readers +~300, the RAM message and the envelope live in cold
modules (ABI 81). Item 4 (freeze the core) must win room back before any resident growth.

Rejected: reading depth in place from the archive (−32 KB RAM but ~+120 KB archive per map:
impossible for 20 maps), shrinking the actor cache (rule 1), lowering `BK_N` (800k-cycle
stalls), sizing buffers to `sc_h` (no gain at the peak, Gariland is the 202-row maximum),
sizing `vbuf` per chapter (RAM would vary per map; the envelope keeps one number to check).

## 2. Archive budget for 20 maps

Core 395,780 + pilot chapter 213,249 transport bytes for 2 maps and 2 scenes; the planning
budget leaves about 1.96 MB. Measure the archive cost per battle map and per scene from the
pilot packs, project 20 + 20, and decide before ripping content whether Chapter I needs two
installable packs (the pack/supervisor format already chains `previous`/`next` packs).

## 3. One-command native chapter run

A script that boots `TI_STATE=fftcore`, sends only the chapter pack + `tilog.89y`, plays New
Game with scene skips, deploys a party, sets `fftmem.py set god on`/`fast on` or skips each
fight, and ends with the `tilog` and `fftmem watch` reports. Needs: a deployment log of
placements (`tilog` in `deploy.c`; today cells are placed blind) and a key script per step.

## 4. Freeze the core interface

Every core change forces a new `fftcore` state (three times on 2026-11-08). Chapter content
must never require a core rebuild: keep chapter data, scripts and encounters in packs; grow
resident headroom by moving more cold code into modules and shrinking the largest images
(WEAPON 8,690 sets the zone, then AI 8,630, PARTY 8,346, SAVE 8,244, RESULTS 8,030). Resident
fftbat: 64,539 + 4 of 64,936 TTPack input bytes after the 2026-11-08 session.

## Known open items

- `make test`: `manage-test` expects 3 units in `x/check/ffsave_won.sav`, which `check.py`
  now writes with 5 (the previous build writes the same fixture: a stale test).
- `core-capacity-render-check`: `equipment-last` frame 0 costs 370,642 cycles (368,076 before
  the ragged depth), over the 360k budget it asserts.
- The standalone demo (`games/fft`, `make test`) no longer compiles (`nature_rules.h`
  needs campaign-only Work/State fields), before and after this change.

- `make qa-check`: `check_qa.c` calls `wait_arrow()`, which moved into the ORDER module
  (pre-existing link error, unrelated to the session).
- `check_title.py --variant b` needs its own title bank (`--bank`); the shipped variant 0 passes.
- God mode does not rewrite deferred weapon panel/line victims (only single blows and
  `W->cast` area batches).
- The live AI-pause animation is checked on PC/68000 (`core-ai-pause-check`); it was not yet
  reviewed visually in TiEmu.
