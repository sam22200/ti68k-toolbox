# Applied archive cleanup — 2026-10-17

The subsequent [single-facing portrait and bounded blade-cache change](BLADE_CACHE.md)
reduces data further to158,468 bytes. This note retains the first cleanup's baseline
and measurements; current UI/weapon banks are16,284 /22,992 bytes respectively.

The user retired textured rotations and disabled attack-camera assets after the
[initial inventory](STORAGE_AUDIT.md). Active camera data drops from **399,328 to
174,860 bytes**, a **224,468-byte / 56.21% reduction**, before TI variable/archive
overhead. Source controls and historical captures remain outside the active package.
The native configuration executable drops from65,497 to61,413 container bytes
(65,407 to61,323 payload bytes). Full tactical+camera packaging remains separate;
the calculator build here is the bounded interactive camera review.

## Applied changes and exact data savings

| Change | Saved bytes |
|---|---:|
| Remove rotation atlases | 131,072 |
| Remove rotation-only UVs | 6,936 |
| Remove rotation surface/facing acceleration bank | 31,270 |
| Remove disabled attack fixture backgrounds | 12,028 |
| Canonical font, including alignment difference | 29,924 |
| Remove unused source UI marker | 84 |
| Variable-length names with offset directories | 1,640 |
| Deduplicate exact blade frames, including byte index | 3,408 |
| Compress the two fixed perspective images | 8,106 |
| **Total** | **224,468** |

F1/F5 remain animated in native160x100 low poly, with the same signed camera sweep.
CATALOG now has only two perspective choices: close/wide and textured/filled mesh.
The normal-view textured-rotation option and its state byte are removed. The
configuration build rejects accidentally enabling `FFT_TURN_TEXTURE`; historical
research controls can still be built separately.

The native staging directory uses an explicit nine-bank manifest. Retired atlas,
UV, surface and attack banks move to sibling `retired-banks/` directories and cannot
be resent through the active wildcard command. `.bin` and `.89y` are two representations
of the same asset, not additive TI payloads. Adjacent PC-only `ffttex.bin`/`fftiso.bin`
remain excluded from the TI send list. The fixed inspection backgrounds retain their
existing position/facing restrictions; atlas removal does not make native general
perspective texturing available.

## Interface: why it was 50 KB, and what it is now

| Component | Before bytes | Current bytes |
|---|---:|---:|
| Small font, including alignment | 30,400 | 476 |
| 510 generic names | 6,120 | 4,480 |
| Six portraits, both horizontal facings | 4,176 | 4,176 |
| Four antialiased banner strips | 4,128 | 4,128 |
| Sloped reach/cursor markers | 3,216 | 3,216 |
| Map terrain/tile records | 1,800 | 1,800 |
| Character widths and alignment | 96 | 96 |
| Unused original source marker | 84 | 0 |
| **Total `ffti`** | **50,020** | **18,372** |

The font stored95 characters x16 horizontal alignments x5 rows x4 bytes. The new
format stores the same95x5 byte rows (475 bytes plus one alignment byte), shifts
five rows while rebuilding a UI part, then retains the existing HUD caches. There
is no30-KB generated-font allocation or new persistent glyph cache. Every one of
the1,520 original shifted glyphs reconstructs byte-for-byte.

Names retain all510 original entries and their unit indices. Each gender now has
255 big-endian u16 offsets followed by NUL-terminated strings; repeated strings
can share offsets. `unit_name` returns a Flash pointer without a decode buffer.
All510 names match the former padded12-byte records exactly.

Portraits retain colour, transparency and both source orientations. Banner strips
are small text images, not full-screen backgrounds. Terrain and slope data remain
because navigation, cursor drawing and tactical rules use them.

## Attack poses and weapons: why 40 KB

| Component | Before bytes | Current bytes |
|---|---:|---:|
| Armed body poses | 19,968 | 19,968 |
| Blade pictures | 20,736 | 17,280 |
| Blade frame index | 0 | 48 |
| **Total `fftw`** | **40,704** | **37,296** |

Bodies: six actor types x two mirrors x two front/back facings x four wind/swing
phases =96 frames. Each16x26 frame stores four52-byte planes: light, dark,
transparency and the eroded interior used by covered silhouettes. These are actual
body/hand poses, not just an icon of a sword.

Blades: two animation families x dagger/sword x two mirrors x two facings x three
impact phases =48 logical frames. Their32x36 canvas stores three144-byte planes
(light, dark, transparency), so432 bytes per picture. There are only40 unique
records. A48-byte lookup now shares the duplicates while all per-pose horizontal
anchors stay unchanged. Every logical blade picture matches the original bank.

## Perspective image compression and RAM

The two6,000-byte fixed inspection images now occupy3,902 bytes including their
descriptor/header and offset directory, instead of12,008. Existing verified ZX0
decoding writes only the selected image directly into `scene_l`; it adds no output
allocation. Other cameras continue to render their own geometry.

Native CPU-only decoding is290,468 cycles /24.21 ms for close and388,112 /32.34 ms
for wide at12 MHz. This replaces a raw copy on entry; it is not a new per-animation
frame cost. Prepared texture entry in the native check is389.11 ms close and194.13 ms
wide for the matching original-facing fixtures. These are datasheet CPU counts,
not TiEmu or physical-calculator elapsed times. Cold construction remains roughly
1.2–1.7 seconds depending on camera, as before; compression does not solve that cost.

Game Work buffers are unchanged:43,192 bytes for full ISO and22,762 for the bounded
camera review. Its optional idle cache remains20,284 bytes. State alignment leaves
the removed preference byte without a net struct-size reduction. The smaller
executable also reduces resident execution data/code, but no fresh AMS free-memory
or largest-block measurement is claimed. There are no bulk asset expansions at
battle start.

## Further sprite-storage experiments, not integrated

`make sprite-storage-explore` compresses every current frame independently and
verifies round trips. Directories are included in these candidate sizes:

| Data | Current bytes | Per-frame ZX0 candidate | Further saving |
|---|---:|---:|---:|
| Ordinary characters and loot | 57,200 | 39,574 | 17,626 |
| Armed bodies | 19,968 | 12,716 | 7,252 |
| Forty unique blades | 17,280 | 2,976 | 14,304 |

Six actual native sample decodes take1.29–1.83 CPU ms, with208 or432 output bytes.
The largest encoded frame is sampled, but is not proven to have the worst decode
latency. A blade-only cache is the most promising next experiment:432 bytes plus
its key, one current attacker, and a miss only when its blade phase changes. A
cache holding one ordinary pose per eleven units needs2,288 bytes plus keys, but
simultaneous misses and perspective pose-pointer lifetimes still need game-level
measurement. Neither candidate is enabled in the game yet.

Generating every mirror at battle start would consume39,528 extra RAM bytes;
one actor type's ordinary/armed/portrait mirrors need6,588. Lazy mirroring can bound
this, but must mirror the body before reapplying the fixed ground shadow. Whole-bank
compression saves more Flash but its large output buffers remain unsuitable for
the current RAM budget. Rotation texture tiling and list deduplication are now
irrelevant to the active package because those banks were removed entirely.

## Verification and reproduction

```sh
make test events-test pc ti cycles CFLAGS_GAME=-DFFT_MEASURE
make events-check check-interface
make camera-prefetch-check camera-prefetch-filter-check
make storage-cleanup-check turn-direction-check
make storage-audit sprite-storage-explore
```

The unchanged-art checks compare137 actual game screens with a preserved pre-change
binary/bank snapshot,1,520 glyph shifts,510 names and48 blade pictures. Default
tactical tests and banner/loot/covered-silhouette tests pass. Native checks compare
101 event/replay screens,177 UI screens,484 configuration frames and34 cold/prepared
camera pictures. Direction checks cover42,240 projected corners and48 signed-sweep
cases. Full battle, fast-forward and defeat replays peak at350,614 /356,184 /359,294
ordinary cycles, all below360,000. The font change adds roughly2,200 cycles at the
previous defeat peak (about0.18 ms), without extra cache RAM.

Native test figures exclude grayscale/presentation/waits. No new TiEmu run was
performed. Local reports are in `x/storage/cleanup-checked.json`, `audit.json`,
`sprite-exploration.json`, `x/config/checked.json`, `x/prefetch/checked.json` and
`x/events/checked.json`. Resend the executable and **all nine current archived
variables** together: UI, names, weapons and inspection-image formats changed.
