# Chapter I readiness: next steps (2026-11-08)

Chapter I has about 20 battle maps and 20 scenes; the pilot ships 2 of each. Tackle these in
order before producing Chapter I content. State of the 2026-11-08 session: saves as in FFT,
scene skip, hidden DEBUG page, APPS selection, live AI-pause animation, `fftmem.py`/`tilog`
instrumentation and the `fftcore` TiEmu state ([CORE_PACKAGES.md](CORE_PACKAGES.md), "Saves,
debug aids and logs" and "Native test loop").

## 1. Battle RAM margin (first, mandatory)

**Problem.** On a fresh Titanium (AMS 3.10) about 187,150 bytes are free when `fftbat`
starts; the battle reserves about 185,950 (measured with `ti-mem heap`, Gariland), leaving
**~1.2 KB**. Peak = steady state (nothing allocates during frames).

| Block | Bytes | Notes |
|---|---:|---|
| decompressed program | 65,014 | ~56 KB code + module zone 8,690 |
| `sbuf` | 53,622 | scene colours 2×40×202 (16,160, hot path) + Work 37,460 |
| `vbuf` | 32,322 | depth1, mask1, depth2, mask2: 4×40×202, read-only after decode, cold readers |
| `acache` | 10,178 | 48 actor frames: keep (CORE_OPTIMIZATION rule 1) |
| grayscale | 2×7,690 | visible planes + hidden render target (copy-present mode) |
| `battle_state` | 5,428 | State 2,224 + Save 3,202 (+ malloc handle word) |
| `navbuf` | 3,002 | **20 bytes per map node**: Gariland 150 nodes; 256 nodes would need +2.1 KB |
| stub copy | 1,008 | freed by the supervisor after the battle (`SymDelTwin`) |

**What does not cost RAM.** The number of maps, scenes and battles in a pack: banks, scene
pools and manifests are read in place from the archive (one VAT entry each).

**What can break Chapter I.** A map with more navigation nodes than Gariland (`navbuf`), any
growth of Work/State/zone/code, and calculators with other variables in RAM.

**Target.** At least 16 KB free during the heaviest battle, measured natively with
`ti-mem heap` and the supervisor's `tilog` line (`story call fftbat(...) free N`).

**Plan (pixels identical, frame paths unchanged, archive cost ~0):**
1. **Ragged depth planes (−~14.6 KB).** Each view's depth/mask content covers only about
   half of a 40-byte row (Gariland 4,040–4,374 of 8,080 bytes per plane; the second layer is
   non-zero in ~1,450 bytes). Store per-row spans with a row table in the archived view,
   allocate `vbuf` for the largest view of the active set. Readers to adapt (all cold): ring
   marks, `cursor_mask`, `cover`, blade cover, `ai_camera.inc` (`VDEPTH`, `fft.c:111-117`,
   `unpack_view` `fft.c:1639`). Tools: `extract.pack_view`, `view_split.py`,
   `chapter_desc.py`. Measure `cover`'s cycles (`ti-cycles`, 360k frame budget).
2. **100-row hidden gray buffer (−1,690).** Copy-present mode never flips it; cut every
   clear from 3,840 to 3,000 bytes per plane (`rt_ti.c`).
3. **Ring backups by row span (−~2.5 KB of Work).** Keep the 800k-cycle re-decode fallback
   out of reach.
4. **`navbuf` envelope.** Size it for the core envelope (256 nodes) inside the budget, or
   reject a map that exceeds the measured budget at package time.
5. **A real message.** Replace "Missing data or memory" by "Not enough RAM: need X, free Y"
   (cold code), so a user knows what to delete.

Rejected: reading depth in place from the archive (−32 KB RAM but ~+120 KB archive per map:
impossible for 20 maps), shrinking the actor cache (rule 1), lowering `BK_N` (800k-cycle
stalls), sizing buffers to `sc_h` (no gain at the peak, Gariland is the 202-row maximum).
Measurements and references: the table above (`ti-mem heap`, 2026-11-08), ENGINE.md:1365-1379,
CORE_FINAL_COMBAT.md:80-128.

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

- `make qa-check`: `check_qa.c` calls `wait_arrow()`, which moved into the ORDER module
  (pre-existing link error, unrelated to the session).
- `check_title.py --variant b` needs its own title bank (`--bank`); the shipped variant 0 passes.
- God mode does not rewrite deferred weapon panel/line victims (only single blows and
  `W->cast` area batches).
- The live AI-pause animation is checked on PC/68000 (`core-ai-pause-check`); it was not yet
  reviewed visually in TiEmu.
