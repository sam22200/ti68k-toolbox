# Handover: FFT core milestone (for Codex or any agent taking over)

Session of 2026-10-21/22, branch `feat/fft-shared-engine`, last commit `dba2fbc`. Read first:
`CLAUDE.md` (technical rules), `docs/working-rules.md` (collaboration rules), this file, then
`ENGINE.md` (the one-engine design and its milestones), `README.md` (build and checks) and
`../MANAGEMENT.md`.

## Where we want to go

The user's goal is a **"core" milestone**: one battle engine, one management-menu engine, one
scene engine, a world map and a campaign save, shared by every chapter. Chapters hold **only
data** (maps, scenes, descriptors). Titanium only, engine plus PIC modules. Once the core is
done, the next step is to **rip every Chapter I map and scene** as data.

The battle engine will grow a lot (many FFT jobs and abilities missing), so program size and
RAM are permanent constraints, not one-off problems (see "Limits").

## Results of this session (all committed and pushed)

| Commit | What |
|---|---|
| `579469c` | M4: one battle program `fftbat` plays every chapter (`fftbat(100+door)` Orbonne, `fftbat(200+door)` Gariland) |
| `f584d25`, `d1097c1` | Campaign save `ffsave` (now FSV2, `save.h`), shared by every program |
| `d6c09ad`, `7baa225` | Management menus `fftmng` (bank `ffmg`, `tools/management_data.py`) |
| `cb6c7ea`, `c4de233`, `13659e8`, `f5deb7d` | World map `fftmap` (bank `ffwm`, `tools/world_assets.py`), save keeps the location, motifs on land and sea |
| `d7dda58`, `cda534e` | One Status page for every chapter, with job names |
| `6ae104f` | Jobs and items shared by FFT id across chapters (bank `ffcj`, `tools/common_tables.py`) |
| `dba2fbc` | Battles take the player's party from the save |

The chain, `launcher.c` (`fftstory`): `fftscn(0)`, `fftbat(100)`, `fftscn(1)`, `fftbat(200)`,
then a loop on `fftmap()`, which returns 0 (quit), 1 (open `fftmng`) or a battle door (run
`fftbat`). It resumes after the last saved victory. `fftstory(1)` starts a new game; door 99
is the test chain.

TiEmu (Titanium) was checked once at the end:
- a full Gariland victory followed by door 216;
- a job change in `fftmng` that persists;
- the map loop: travel to Mandalia, the menus, then back, resuming at Mandalia.

## Decisions the user made (do not re-ask)

- Management menus: **layout A, no animation**. Four actions: Placer les capacités, Équiper,
  Apprendre, Changer de métier. All **20 generic jobs**.
- World map: **simplified geography** (land and sea from the original texture, without the
  parchment's lines and lettering), travel, menus, enter a location. Then "un peu de motif"
  (sparse land stipple, sea waves, checkered shallows, black coast).
- One Status page for all chapters (Orbonne's text page and Gariland's page are unified).

## Open items

1. **Visual validation pending.** The user has not yet approved the screens of `fftmng`, of
   `fftmap` (with motifs) or of the unified Status page. Show before/after images and get an
   OK before building more on them.
2. **Step 3 of the core: abilities as shared data, plus an FFT formula interpreter.**
   - 512 abilities, 16-entry skill sets.
   - Wider indices and learned bits (today `SET_N` entries per set, `u8 learned`).
   - The interpreter must live in a **module** (`ffbm`): `fftbat` has no room left.
   - A question to the user is still unanswered:
     - (a) start with the formula families Chapter I needs; or
     - (b) first rip the next battle (Mandalia Plains, door 200 on the map, `STORY` in
       `world_assets.py`) to work from concrete needs.
   - Wait for the answer, or ask again in one short question.
3. After the core: rip Chapter I's maps and scenes. `world_assets.py STORY` lists which
   location shows after which story flag, and the battle door it enters.

## Architecture in one page

**Battle engine `fft.c`**
- `FFT_CHAPTER` builds read a descriptor `cdesc`/`CD(field)` (`tools/chapter_desc.py`
  generates `chapter.h`).
- PIC modules live in the bank `ffbm` (`tools/module.py`): MOD_AI 0, TURN 1, SCREENS 2,
  BEATS 3, SAVE 4.
  - Their context is `ModCtx` in `module.h`. A new core function a module needs goes there.
  - Parts are marked `MOD_PART(k)` / `MOD_CORE`.

**Campaign save `save.h` (FSV2)**
- Big-endian, FFT's own ids everywhere.
- Header and inventory: 256 story flags, `n`, `loc` (location id + 1), `inv[256]`.
- 16 `SaveUnit` records of 176 bytes:
  - job, gfx (0xFF none), type (0 male, 1 female, 2 Ramza), name, level, exp, brave, faith,
    zodiac;
  - secondary, raw[5][3], eq[5] (0 none), reaction, support, movement;
  - jp[20], tjp[20], learned[20][3].
- Learned bit = `0x800000 >> pos`: positions 0..15 are actions, 16..21 are
  reactions/supports/movement.
- Battle side: `save.inc` (`save_campaign`, `save_party`, `party_update`, `party_load`) runs as
  the SAVE module.

**Descriptor fields added this session**
- `PARTY`: bit 1 writes the party at the victory, bit 2 reads it at the start; test door 70
  reads it too.
- `WON` (flag + 1), `LOCATION` (id + 1).
- `SET_POS`, `SET_ID`, `BAG_ITEM`, `RSM_POS`: the chapter's indices to FFT's ids.
- `DEFEND_ID`, `EQUIP_CHANGE_ID`, `EQUIP_CHANGE_POS`.

**Descriptor fields removed**
- Now in `ffcj`: STATUS, JOBS, ITEMS, NJOB, NITEM, JOB_ID, JOB_NAME, WEAPON_RANGE, SHIELD_EVADE.

**Banks** (built from the local disc, **never committed**)

| Bank | Built by | Contents |
|---|---|---|
| `ffcj` | `common_tables.py` | "FCJ1": Job[160] (14 bytes), job names, job skill sets, Item[256] (26 bytes; `reach`, `sev`, `pad` after the name). Written to both `opening/data` and `campaign/data`. |
| `ffmg` | `management_data.py` | "FMG1", ~47.8 KB: 21 job records (careers 0..19, Ramza at 20), items, ability/set names, icons, 41 sprites, 41 portraits, font, generic names. |
| `ffwm` | `world_assets.py` | "FWM1", ~19.6 KB: 255 TileMap tiles, a 31×24 grid, 43 locations, 48 routes, the story list. |

## Discoveries (verified, save the time)

**Disc tables**
- Do not scan blindly: use the names in the public decomp **adamrt/fft_decomp** (GitHub).
- `WORLD/WLDCORE.BIN` is loaded at 0x80067000.
  - The locations' dots are at 0x2DDFC.
  - The route table is at 0x2E008.
  - A map position + (263, 186) is the texture pixel.
- `WORLD/WORLD.BIN` 0x8E1A0: the location names.
- Name lists in `BATTLE.BIN`: jobs 0xFAD41, items 0xFB2C3, abilities 0xFCB88, skill sets 0xFE93A.
- `WORLD/WLDTEX.TM2`: 2048-byte sectors, each a 12-byte header then 1010 words.
  - It holds four 8-bit pieces, each starting with its palette.
  - The pieces' heights are uneven (241 vs 242 rows): crop them.
  - Each piece's first 8 columns are not map.
  - Route ends can be 2 px off their location (Bariaus Hill): the check allows 3 px.

**Engine**
- Ramza keeps sprite 40 and `UG_RAMZA` in every job.
- Move is 5 with Battle Boots: this is not a bug in the management test.
- A unit's job and equipment are now FFT ids, so Gariland's state hashes changed. This was
  expected: the references `open_m3.json` and `camp_m3.json` were updated.
- The `Item` layout (26 bytes) is asserted in three places that must change together:
  - `opening/opening.inc`;
  - `tools/campaign_assets.py`;
  - `tools/common_tables.py`.
- The `font` symbol clashes with `rt_ti.c` (hence `menu_font`). TI builds of the menus and the
  map need `tilemap.a` in the link.
- The contour tables in `water_dump` need `--attach`.

## Limits (lasting)

- **TTPack input limit: 64,936 bytes. `fftbat` is 64,161 bytes: 775 bytes left.**
  - Every new feature goes into a module (`ffbm`) or into data, not into the core.
  - Measure the size after every change.
- RAM: the Titanium has ~188 KB free, and Gariland already needs ~183 KB. Prefer archived data
  read in place. Plan overlays and split programs for growth.
- Modules sit one at a time in `MOD_ZONE`, sized by the largest module image. A large formula
  interpreter raises that zone: check it.

## How to verify

```sh
export PATH=$PWD/tools/bin:$PATH
make -C games/fft/opening assets check      # 12 + 6 replays
make -C games/fft/campaign assets           # needs the local disc
make -C games/fft/campaign check bundle     # 26 cases incl. won, party, manage, worldmap
make -C games/fft/campaign save-test manage-test map-test
```

- A change counts as done when the PC tests and the PC/TI replays (ti-cycles) pass, with
  checksums equal to the references.
- If a reference changes, it must be a deliberate, explained change.
- The previous agent's wrapper (`verify.sh` rebuilding both chapters) was in its scratchpad
  and is not in the repo: use the make targets above.
- TiEmu is used last, once per milestone: Xvfb `:99`, and `ti-run` with a clean `.sav`.

## Do not

- Do not grow `fftbat`'s core: 775 bytes left.
- Do not commit anything derived from the disc (`*.bin` banks, sprites, texts).
- Do not build on an unvalidated visual change. Show side-by-side images first. Identical output
  needs no validation.
- Do not animate the management menus. Do not re-ask the decisions listed above.
- Do not write asm without asking the user. Do not use TiEmu before PC tests and ti-cycles.
- Do not use `pkill -f` with a pattern that matches your own command (it killed the scan
  itself). Never start or kill `kwin_x11` for Xvfb captures: it broke the user's desktop.
- Do not scan the disc by brute force for tables (false positives): find them by name in the
  decomp.
- Do not reintroduce per-chapter tables (jobs, items, Status) or `FFT_OPENING`-style `#if`s:
  chapter differences belong in the descriptor.
- Do not change Gariland's bit-identity silently. Its replays are the regression net.

## Working with the user

- Reply in French with correct accents; files, code and commits stay in English.
- Commits: Conventional Commits, one concern per commit, on this branch, then push
  (`ti-commit` skill).
- The user prefers short questions with options and sometimes answers with a free scope instead.
  Do not ask more than needed.
