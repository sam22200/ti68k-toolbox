# Neo Geo reference tooling

The first local set is `roms/neogeo/wjammers/`: eleven Windjammers cartridge
chips. `romset.py` checks sizes and CRC32 against FBNeo's `wjammersRomDesc`
at revision `0282ae7e0dc647316043f2f6b2cd312d3164218a`, records SHA-256,
prepares canonical chip names and exports a big-endian program image.
The original files remain untouched. ROM preparation uses Python 3's standard
library; the runner uses `tools/pyenv` (Pillow for RGB captures).

```sh
make -C tools/neogeo audit
make -C tools/neogeo check
make -C tools/neogeo prepare
make -C tools/neogeo core
make -C tools/neogeo core-check
make -C tools/neogeo reference BIOS=/path/to/neogeo.zip
make -C tools/neogeo gameplay
make -C tools/neogeo rules
```

`prepare` writes `sources/windjammers_neogeo/{manifest.json,program.be.bin,
wjammers.zip}` (ignored). Override `ROM` and `OUT` as needed; the manifest
currently supports only the exact Windjammers set. A directory with `.bin`
chip names or a ZIP with driver chip names is accepted. Archive packaging
renames entries without converting their contents. The CPU image swaps
adjacent bytes in P1 only; never put that converted image into the game ZIP.

`test` runs eight frontend tests without a BIOS: memory byte order/signedness,
bounds, two-player input scripts/callbacks and dynamic core options. `check`
also verifies the actual local files, header/vectors, reversible program
conversion and lossless ZIP round-trip. Neither boots the game.

`core` builds [FBNeo/libretro](https://github.com/libretro/FBNeo) revision
`a49cfac4b97cc62d0196c1d0cde8f5b14fde662c` under ignored
`sources/neogeo_core/`, using `SUBSET=neogeo USE_SPEEDHACKS=0 FASTMATH=0`.
The earlier driver revision used for chip auditing lacks the libretro
frontend; it is a separate source identity. `expose_video.py` adds read-only
memory/video/status exports, a CPU-bus read probe and an explicit initial-RTC
setter, without changing the emulation loop. The runner sets the calendar to
2000-01-01 Saturday 00:00:00 before the first CPU frame; ordinary emulated
ticks advance it afterward. Host local time otherwise changes BIOS-maintained
RAM/NVRAM bytes between boots. Metadata records this reference configuration.
`core-check` verifies the compiled ABI/build flags and RTC feature and runs
the genuine frontend/core rejection path with deliberately invalid BIOS
input. It requires the compiled core and original game ZIP, but no firmware.

`reference` is the positive integration check: cold boot, native two-player
inputs, normalized RAM versus the actual CPU bus, and every frame of a
120-frame uninterrupted-versus-restored RAM/VRAM/palette/Z80/RGB replay.
It writes a provenance/replay report under `sources/windjammers_neogeo/`.
This check **passes** with the user's `roms/neogeo/neogeo.zip`: 2400 cold-boot
frames into the game's intro, then 120 identical continuation/replay frames.
The selected BIOS is `sp-s3.sp1`, CRC32 `91b64be3` (MVS Asia/Europe ver.6);
the core reports 59.18 Hz and 304×224 output. The separate `gameplay` check
reaches a serve and measures walking and ordinary throws. The [FBNeo BIOS
documentation](https://docs.libretro.com/library/fbneo/#bios) describes lookup
locations. Record the actual loaded BIOS and settings, not just the archive.

## Runner

```sh
tools/pyenv/bin/python tools/neogeo/neogeorun.py \
  --bios /path/to/neogeo.zip --frames 2400 \
  --save sources/windjammers_neogeo/boot.state \
  --video 2399:sources/windjammers_neogeo/boot_video \
  --shot 2399:sources/windjammers_neogeo/boot.png \
  --metadata sources/windjammers_neogeo/boot.json
```

The ROM defaults to the prepared `wjammers.zip`. Each instance gets fresh
temporary system/save directories; only the supplied BIOS is exposed there.
No existing NVRAM, memory card, cheats or high scores are imported. Audio is
emulated and discarded. This pinned FBNeo core cannot safely reinitialize a
real Neo Geo driver in the same process; the runner rejects a second instance.
Use a fresh process for each cold boot and `load` for repeated probes of one
initialized core. `gameplay` compares cold boots in separate processes.
The runner retains registered defaults and overrides
CPU clock to 100%, fixed frameskip to 0, patches/hiscores/card to disabled and
diagnostic input to None. Mode/region/game DIPs remain recorded core defaults;
`--option core-key=value` can select an explicit reference configuration.

Key scripts use `<frame> <buttons...>`, held until the next event. Bare
buttons address P1; `P2:LEFT` addresses P2. Every event replaces **both**
held masks, so an unspecified player releases their buttons. A bare frame
releases both. Buttons are Neo Geo UP/DOWN/LEFT/RIGHT/A/B/C/D/COIN/START,
mapped to the pinned Classic RetroPad; metadata captures the actual input
descriptors. The positive check validates A/B/C/D/coin/start descriptors on
both ports and live right/A, left/B, P1 coin and P2 start input bytes.

`--trace 100000:2,100002:s2` prints CPU-order fields after each selected
frame. `--poke 10fffc:4=0x12345678` writes before frame 0. These are syntax
examples, **not** known player fields. Work RAM is strictly bounded to
`100000..10FFFF` and rejects mirrors/crossing accesses. Read/write normalize
the pinned little-endian core's swapped bytes; the positive integration
check compares writes against `SekReadByte` on the emulated CPU bus.

`--dump frame:path` exports CPU-order RAM. `--video frame:directory` exports
RAM, VRAM, both palettes (big-endian words), raw NVRAM/card storage, Z80 RAM,
input bytes and JSON status. Status includes actual BIOS name/CRC, palette
bank, animation counter, FIX selection and graphics/brightness flags.
These are post-frame snapshots, not a raster timeline or decoded scene.
`--gif`, `--every`, `--load` and `--metadata` follow the SNES tooling's flow.
Loading a state clears both frontend pads and invalidates the prior video;
the next run produces frame 0. The core can emit a duplicate (NULL video)
on that frame: RAM advances, but a PNG requires a subsequent non-NULL callback.
Replay checks retain the saved frontend surface explicitly for duplicate
presentation; it is separate from the opaque core/game/settings-bound state.

## Windjammers gameplay measurements

`make -C tools/neogeo gameplay` uses `keys/windjammers_serve.txt`: two humans,
H. Mita vs B. Yoo, Beach, default MVS v6 configuration. These are reference
probe choices. Two independent fresh boots must match all exported fingerprints
at post-frame 4350; no RAM writes are used to navigate menus or obtain possession.
Outputs stay in ignored `sources/windjammers_neogeo/gameplay/`: `serve.state`,
`serve.png`, snapshots, individual CSV timelines and `report.json` with identities,
field map, input-script hash, full per-frame RAM/video hashes and findings.

The check verifies all eight walking directions for each character, release,
reversal and integer-word court clamps against 92-frame original timelines.
It captures straight/up/down A throws, release timing, free-flight integration,
rebounds and subsequent automatic catches. All trials replay exactly; a separate
160-frame uninterrupted rally matches state-restored execution. Three labelled
diagnostic trials reposition the players/disc and change disc velocity to
validate the fields independently. They are excluded from normal mechanics evidence.

See the [measured field map and constants](../../.claude/skills/ti-port-neogeo/reference/windjammers.md#observed-first-service-walking-and-disc-reference).
The separate `rules` check boots its own first-service door and checks the
ordinary wall solver against four natural rebounds and 56 fractional-boundary
diagnostics. It checks 168 neutral-pose contact placements (overlap, catch/rear
deflection and next-frame consumption), eight goal-line crossings and sixteen
point-zone probes on both sides. Two no-write missed-shot sequences confirm
points persist and the loser receives service. All 1390 frames replay with
identical full fingerprints. Its states, CSVs and report live in ignored
`sources/windjammers_neogeo/rules/`; every diagnostic write is recorded.

On fresh Beach, the selected five-point Y interval is [120,168), with three
points outside; the table's first entry [136,152) is not the active interval.
Round points are BCD bytes, separate from arcade totals. The detailed notes
include the verified sweep quantization, contact boxes/arcs and goal timeline.
The contact check uses original pose selection and collision angles; it does
not claim a full animation/angle model. Corner/high-shot collisions, other
actions/content, round/time-out ending and the native TI engine remain ahead.
These checks supply original evidence; they do not build a TI game.

Read [the skill](../../.claude/skills/ti-port-neogeo/SKILL.md) for the port
workflow and [Windjammers notes](../../.claude/skills/ti-port-neogeo/reference/windjammers.md)
for observed file identity and the next gameplay probes. ROMs, BIOS, core
sources, states and generated art stay local under `roms/` and `sources/`.
