# Chapter I readiness: next steps (2026-11-08, battle RAM updated 2026-11-12)

Latest steering (2026-11-13): finish the [user prerequisites](CAMPAIGN_PREREQUISITES.md)
before route automation. [Safe scene ESC and permanent Brave/Faith saves](SCENE_SAVE_FIXES.md)
and [four traps/level-down persistence](TRAPS.md) are delivered and approved.
ABI89/Work29310/Save3204/FCD5, input57621/headroom7315; matching private pilot
groups total701775 transport bytes. Monster KO art, targeted/protected objectives
and authored guest/chain/choice content remain. RAM figures below are previous
measurements, not a new16KiB pass; difficult recovery remains deferred.

Current integration (2026-11-12): all155 audited Chapter I actions are executable.
[Sweegy combat/save replays](CHAPTER1_MONSTER_BATTLE.md) now exercise the first
complete monster fights on PC/sanitizers (13 actors, source terrain, frozen
generation inputs); one selected68000 boot/save carrier covers the two cold
fixes. This does not ship the remaining chapter artwork or event content.
The RAM/archive measurements below are dated planning evidence: the11.4KB free
RAM predates the companion's3074-byte reservation and later program growth.
[Current archive-colour follow-up](CORE_REACH_ARCHIVE.md) saves8638 production
bytes after the1692-byte grayscale lot. Work29282, ABI87; true TTPack input57275
leaves7661 bytes. The real installed supervisor diagnostic has14058 bytes free
(largest13042); production free15171 is only an estimate and still below16KiB.
Installed-flow peak/contiguous certification remains open. Current pilot groups
are696764 transport bytes; a conservative full-chapter archive estimate remains
below its planning allowance, but full content has not been assembled.

User steering (2026-11-12): defer difficult RAM recovery and continue. A bounded
source audit selected no simple extra2KiB saving; keep the current buffer/cache
and companion capacities. [Archive projection](CHAPTER1_ARCHIVE_PLAN.md) now has
a host-only reproducible check:1869336 projected chapter bytes versus2163895
allowance,294559 planning margin. Full assembly remains open. Next is preparing
the installed chapter route, not another RAM refactor or TI sweep.

Chapter I has about 20 battle maps and 20 scenes; the pilot ships 2 of each. Tackle these in
order before producing Chapter I content. State of the 2026-11-08 session: saves as in FFT,
scene skip, hidden DEBUG page, APPS selection, live AI-pause animation, `fftmem.py`/`tilog`
instrumentation and the `fftcore` TiEmu state ([CORE_PACKAGES.md](CORE_PACKAGES.md), "Saves,
debug aids and logs" and "Native test loop").

## 1. Battle RAM margin (target; further refactoring deferred)

**Current status:** the100-row hidden target and archive-based reach restoration
are delivered; see [current measurements and limits](CORE_REACH_ARCHIVE.md).
Per the user decision above, proceed without further difficult RAM refactoring.
Production installed-flow measurement is still required before claiming16KiB.
The old ring-backup estimate below is historical.

**Historical status 2026-11-10 (before companion/program growth): ~11.4 KB free.** Measured natively (Titanium AMS 3.10, Orbonne, `ti-mem heap`): 187,554
bytes free when the supervisor calls `fftbat`, the battle reserves 176,158:

| Block | Bytes | Notes |
|---|---:|---|
| decompressed program | 56,298 | the resident only (64,934 with the module zone until 2026-11-10) |
| module zone | 8,632 | heap block since HW3Patch is required (§ 4): RAM unchanged overall |
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

**Historical proposals (the old4.6KB deficit is obsolete):**
1. **100-row hidden gray buffer (−1,688), delivered2026-11-12.** The following was the original plan; the current implementation and checks are in CORE_RAM_MARGIN.md. Copy-present mode never flips it, but every draw
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

**Resident headroom.** TTPack input 56,193 of 64,936: **8,743 bytes left** (`tools/headroom.sh`)
since the module zone left the program (2026-11-10, § 4; 149 bytes before, 71 after the actor
banks).

Rejected: reading depth in place from the archive (−32 KB RAM but ~+120 KB archive per map:
impossible for 20 maps), shrinking the actor cache (rule 1), lowering `BK_N` (800k-cycle
stalls), sizing buffers to `sc_h` (no gain at the peak, Gariland is the 202-row maximum),
sizing `vbuf` per chapter (RAM would vary per map; the envelope keeps one number to check).

## 2. Archive budget for 20 maps

**Current planning status:** [R4 projection](CHAPTER1_ARCHIVE_PLAN.md) is1869336
bytes against2163895 available, with294559 margin; real full content/Flash
validation is unfinished. `make chapter1-archive-budget PACKAGE_OUTPUT=...`
checks current package provenance without compiling or running TI.

**Historical estimate 2026-11-10:** the following R2 numbers predate raw R4
colours. The core reads up to six actor banks (~390KB).

Budget: 2,700,000 planning bytes − 131,072 reserve − core 396,473 = **2,172,455 bytes** for
the installed chapter (one chapter at a time, INSTALL.md). Pilot pack: 211,686 bytes.

Views of the 13 distinct Chapter I battle maps (`tools/map_budget.py --archive`, four greys
from the disc, first 202 content rows, R2 banks; × 1.12, the pilot's measured ratio of
shipped to estimated bytes: Gariland 37,980/35,328, Orbonne 34,762/31,024):

| Map | Views (estimate) | Map | Views (estimate) |
|---|---:|---|---:|
| 056 Orbonne | 31,024 (shipped 34,762) | 091 Thieves Fort | 27,748 |
| 022 Gariland | 35,328 (shipped 37,980) | 077 Lenalia Plateau | 24,336 |
| 085 Mandalia Plains | 37,384 | 072 Fovoham Plains | 20,087 |
| 074 Sweegy Woods | 36,170 | 073 Inside of Windmill Shed | 24,864 |
| 031 Dorter Trade City | 23,774 | 103 Windmill Shed | 22,727 |
| 034 Cellar of Sand Mouse | 24,955 | 049 Fort Zeakden | 27,405 |
| 076 Zeklaus Desert | 40,105 | **13 maps** | **375,907 → ~421,000** |

Projection for Chapter I (20 battles on the 13 maps, 20 scenes):

| Part | Pilot | Chapter I estimate | Basis |
|---|---:|---:|---|
| map views | 72,742 (2 maps) | ~421,000 | the table above |
| per-map banks (`a`, `i`, `d`, story) | 15,719 (2 maps) | ~105,000 | ~8 KB a map |
| extra battles on a map already shipped | – | ~35,000 | 7 × ~5 KB of encounter data |
| actor frames `ffca` + `ffcb` | 84,455 (597 frames) | ~170,000–250,000 | ~2–3× the pilot's job/monster sheets |
| scenes (`ffs*`, pools) | 10,344 (2 scenes) | ~100,000 | ~5 KB a scene; event scenes reuse map views |
| flow, dialogue, results | 9,742 | ~100,000 | ~40 steps of script |
| world map `ffwm` | 17,674 | ~18,000 | shared |
| **total** | **211,686** | **~0.95–1.03 MB** | **~45 % of the budget** |

**Decision: one installable Chapter I pack** (no split; the chained `previous`/`next` packs stay
for later chapters). Margin: ~1.1 MB for estimate errors and other programs.

**Core limits found (to fix before freezing the core, step 4):**
- **Actor banks (fixed 2026-11-10, ABI 82):** the core read only `ffca` and `ffcb` (~131 KB).
  It now reads `ffca`, `ffcb` (required) then `ffcc..ffcf` while present (`actors.h`
  `ACTOR_BANKS` 6, ~390 KB); frames are numbered across them in order (`actors.inc` walks the
  bank counts). `common_actors.py` writes as many banks as needed; `chapter_packages.py`
  rejects a gap. Checked: the cache test with three banks, and Gariland's manual and AI
  replays with the pilot's frames split 200/250/147 over three banks (same checksums).
- **Map letters:** one letter a map (`ff?v0`), `c` reserved: ~25 distinct maps a pack, enough.
- **Width:** 076 Zeklaus Desert is 336 px wide (scene 320): crop 16 px in its views.

## 3. One-command native chapter run

A script that boots `TI_STATE=fftcore`, sends only the chapter pack + `tilog.89y`, plays New
Game with scene skips, deploys a party, sets `fftmem.py set god on`/`fast on` or skips each
fight, and ends with the `tilog` and `fftmem watch` reports. Needs: a deployment log of
placements (`tilog` in `deploy.c`; today cells are placed blind) and a key script per step.

## 4. Freeze the core interface

**Zone and resident (2026-11-10, ABI 84).** The program is the resident plus the module zone
(the largest image). The cold start (`bootstrap_files`, `cj_valid`) moved from SCREENS to
RESOURCES (ops 13, 14; the catalogue now goes through RESOURCES' own context), the AI's
weapon cache (`weapon_ai_prepare`) from WEAPON to COMMAND op 6 (it already compiles
`weapon_plan.inc`; AI thinking already switches images every step). SCREENS 8,708 → 7,474,
WEAPON 8,690 → 6,686, COMMAND 3,338 → 7,520, RESOURCES 5,110: zone 8,708 → **8,630**, the AI
image, which keeps its own formula evaluator on purpose (no AI/FORMULA ping-pong). TTPack
input 64,787: **149 bytes left** (was 71), and 78 bytes of RAM. Going lower means splitting
the AI image. The large resident functions left are hot render paths or call modules.

**Chapter I coverage (2026-11-10, `tools/chapter_audit.py`, `x/chapter1-audit.json`).** The
units of the 18 story encounters (ATTACK rows up to Fort Zeakden, ENTD 0x180..0x191) and of
the first-tier random formations of Mandalia, Fovoham, Sweegy, Lenalia and Zeklaus (33
encounters, 297 units) use **155 action abilities; the core executes 105**. The 50 missing
ones are deliberately disabled families in `common_tables.py` (`kind` 255: "Monster/special
producers ... remain independently gated"); the "201/201 generic actions" count covers the
generic jobs only:
- **Special character sets:** Holy Sword (Agrias, already at Orbonne: Lightning Stab, Holy
  Explosion), Dark Sword (Gafgarion at Orbonne), **Holy Magic (Ovelia) done 2026-11-10**:
  Deathspell 2 cancels ten statuses (Esuna's family, `cleanse_oracle.py`); Mbarrier is adapted
  (user decision) to Regen, Protect, Shell and Haste, all or nothing, without Reraise: the
  status result holds one status byte (`(byte << 8) | bits`); its row is ffcj status row 0x7F,
  checked with the original functions by `buffs_oracle.py`), Wiegraf's sets (Split/Crush Punch; as Lune Knight Shellbust Stab, Blastar/
  Hellcry Punch, Icewolf Bite), Zalbag's Magic/Speed/Power/Mind Ruin.
- **Monsters (all):** Goblin (Tackle, Goblin/Turn Punch, Eye Gouge), Bomb/Grenade (Bite,
  Small Bomb, Self Destruct, Flame Attack), Red Panther (Scratch, Cat Kick, Poison Nail),
  Chocobos (Choco Ball/Meteor), Pisco Demon, Squidlarkin, Skeleton, Bone Snatch, Flotiball,
  Ahriman, Juravis, Cocatoris, Steel Hawk, Bull Demon, Dragon.
- R/S/M used: Auto Potion, Counter, Weapon Guard, Equip Crossbow, Concentrate, Move+1,
  Jump+1 (all generic: covered by the 78 R/S/M consumers).

**Monster copies done (2026-11-10): 122 of 155 executable, 33 missing.** Monster actions whose
source record copies an already verified one are enabled (`common_tables.py`): 11 basic attacks
(Tackle, Bite, Scratch... = Choco Attack, `MONSTER_ATTACK` replaces the id 265 hard-coded in
six core sites), Small Bomb and Choco Ball (Punch Art strikes, formula 0x31), Cat Kick and
Tail Swing (Dash's 0x37, evadeable), Shine Lover (Gil Taking), Gather Power (Accumulate, Y 2).
A monster uses only its own set's learned abilities. Checked in `check_discovery.c` against
the human action they copy (PC, sanitizers, 68000). Still needing family work with an oracle:
Turn Punch (area vertical 1 where Spin Fist has 0), the formula 0x50 status family (Eye
Gouge, Poison Nail, Black Ink, Look of Devil, Beak), Odd Soundwave (0x38), Holy Sword (0x2D:
status on hit, area, line), then the new formulas, in the core's companion image
(`COMPANION.md`; 0x4E done: 129 of 155 executable; Holy Sword 0x2D done, `HOLY_SWORD.md`:
133 of 155 with Holy Explosion; Dark Sword 0x2F: 134 of 155; Mighty Sword 0x2E: 138 of 155;
Ruins 0x1A/0x1B, `RUINS.md`: 142 of 155; monster status strikes,
`MONSTER_STATUS_STRIKES.md` (with Turn Punch, Mind Blast, Death Sentence, Odd Soundwave,
Beaking, Goblin Punch, Self Destruct): 152 of 155; Black Ink, Look of Devil, Look of Fright
(flags_4 0x20, direct targeting through the weapons' trajectory): **155 of 155 executable**,
headroom 7,203, zone 8,698.

Caveat: the random-formation tier is read as "first pair of each route" (WLDCORE); some
monsters listed (Dragon, Ahriman) may belong to later progress tiers. **The core cannot be
frozen for Chapter I before monster actions and these special sets exist** (plus monster
units themselves: no equipment, innate skills, sprites in the actor banks).

**Platform decision (user, 2026-11-10): HW3Patch is required on the Titanium; the TI-89 HW2
is dropped.** Unpatched, code only runs inside the program's block (`ti68k-c-patterns.md`:
`experiments/dll`, `experiments/heapcode`), which is why the module zone lives in the program
and why resident + zone must stay under TTPack's 64,936 bytes. With HW3Patch, modules can run
from the heap.

**Done (2026-11-10): the zone is a heap block.** Measured in TiEmu (Titanium AMS 3.10,
`experiments/heapcode/heaplow.c`): code in a block at 0x18F18, after 150 KB of allocations,
freezes unpatched and returns with HW3Patch, also after reloading the saved state (`hw3`).
`fft.c`: `mod_zone` is `malloc(MOD_ZONE)` in `mod_ready`, freed by `final_release`. Program
TTPack input 64,787 → 56,193 (**8,743 bytes of headroom**, was 149); RAM is unchanged (the
8,632-byte block replaces the same bytes of program). Native Orbonne run from the `fftcore`
state rebuilt on `hw3`: dialogue, AI turns, attacks and a charged action from the heap zone.
The supervisor (`launcher.c`) stops with "HW3Patch required" on an unpatched Titanium
(GCC4TI's test: a cleared word in `EX_stoBCD`, or 0x100 at 0xAC); `INSTALL.md` names the
step. Splitting the resident is now possible but no longer urgent. Other uses of HW3Patch
(code in chapter packs: study; parked options): `HW3PATCH_USES.md`.

Every core change still forces a new `fftcore` state. Chapter content must never require a
core rebuild: keep chapter data, scripts and encounters in packs.

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
