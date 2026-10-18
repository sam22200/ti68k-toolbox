# FFT archive and RAM audit — 2026-10-17

**Historical baseline:** the tables below describe the inventory before the user's
storage cuts. Current applied changes reduce data to158,468 bytes; see
[STORAGE_CLEANUP.md](STORAGE_CLEANUP.md) and [BLADE_CACHE.md](BLADE_CACHE.md).
The audit tool retains the pre-cleanup
snapshot for reproducible candidates and reports the current manifest separately.

This is an audit, not a change to the game's asset formats. The reproducible tool is
`tools/storage_audit.py`; run it from `games/fft` with the project Python environment.
Results, hashes, exact candidates and native decoder logs are local in
`x/storage/audit.json`. No emulator is started and no shipped bank is changed.

The inventory combines the latest root actor/UI banks (including the floating chest)
with the selected contrast+edges native camera-review scenery/texture banks under
`x/config/data`. Historical builds, comparison atlases, PC executables, captures and
the original disc are excluded. This is a prospective current-content inventory,
not a newly linked/boot-tested full tactical+camera release. In particular, the
staged actor snapshot is older; it is replaced by the current root bank in this count.
Sizes below are payload bytes, before TI variable/container and archive overhead.
Ko in a French summary means 1,000 bytes; KiB means 1,024 bytes.

## Ten largest storage groups

| Rank | Data | Bytes | Sharing / purpose |
|---|---|---:|---|
| 1 | Rotation texture atlases, `fftat0..2` | 131,072 | Sixteen 128x128 grey/alpha pages, 4 bits/texel |
| 2 | Ordinary unit and loot sprites, `fftu` | 57,200 | Six actor types, mirrored poses, masks, interiors, objects |
| 3 | Interface, `ffti` | 50,020 | Font 30,400; names 6,120; portraits 4,176; banners 4,128; slopes 3,216; tiles 1,800; other 180 |
| 4 | Four selected ISO views, `fftv0..3` | 41,505 | Already ZX0-compressed; colour and two occlusion layers |
| 5 | Armed body poses and blades, `fftw` | 40,704 | Bodies 19,968; blades 20,736 |
| 6 | Merged surfaces and facing lists, `fftsurf` | 31,270 | Surface/header records 11,342; lists 19,928 |
| 7 | Shared detailed mesh and tables, `fftmesh` | 16,585 | Shared by perspective and textured rotations |
| 8 | Legacy attack fixture images, `fftshot` | 12,028 | Automatic attack cameras are disabled in the accepted configuration |
| 9 | Two fixed inspection fixture images, `fftlook` | 12,008 | One matching position/facing, close and wide; not general texturing |
| 10 | Triangle UVs, `fftuv` | 6,936 | Per-triangle texture coordinates/page |
| | **Total data** | **399,328** | **390.0 KiB**, excluding executable |

The current plain ISO executable payload is 63,775 bytes. The earlier bounded
configuration review payload is 65,407 bytes; it is identified by hash and was not
rebuilt here. Using that code size gives an approximate combined content footprint
of 464,735 bytes, not an integration result. The full tactical+camera native link
still exceeds the one-variable limit; see `CONFIGURATION.md`.

For comparison, the plain root ISO data set totals 186,793 bytes (original four-view
style, current actors, weapons and UI). The selected camera exploration is larger
because it includes the detailed mesh, textures, acceleration tables and fixtures.
Do not count `fftiso.bin` (139,528 B), `ffttex.bin` (138,532 B), `fftbg.bin`, multiple
style sets, duplicate `.bin`/`.89y` containers or experiment directories as TI assets.
The PC-only comparison files are already excluded from the native send set.

## Lossless savings candidates

| Candidate | Current → candidate bytes | Flash saved | RAM / CPU implications |
|---|---:|---:|---|
| Alias identical rotation visibility lists | 31,270 → 18,626 | **12,644** | Same records and ordered lists; no decompression or extra bulk RAM |
| Remove disabled attack fixtures | 12,028 → 0 | **12,028** | Remove obsolete load/startup guard too; do not simply delete the file |
| Keep one unshifted font instead of 16 shifted copies | 30,400 → 475 | **29,925** | Needs glyph shifts or a small cache; rebuilding all shifts needs 30,400 RAM bytes |
| Deduplicate exact blade frames with u16 indices | 20,736 → 17,376 | **3,360** | 48 frames, 40 unique; one index read, keep per-pose offsets separately |
| Remove unused original UI marker | 84 → 0 | **84** | `UI_MARKER` has no reader in the current game; update generated offsets |
| Texture tiles, 8x8 dictionary | 131,072 → 100,576 | **30,496** | Preserve every texel; dictionary lookup in the pixel loop |
| Texture 2-bit shade + 1-bit alpha | 131,072 → 98,304 | **32,768** | Extra alpha read/bit operations, no image loss |
| Combine tile dictionary and 3-bit tiles | 131,072 → 77,480 | **53,592** | Includes 8,192-byte index; CPU cost requires a native mapper benchmark |
| Generate ordinary/armed/portrait mirrors | 39,528 stored mirror bytes → generated | **39,528** | All mirrors at once cost 39,528 additional RAM bytes; one actor type costs 6,588 |

The facing-list candidate is actually rebuilt in `x/storage/fftsurf-aliased.bin`:
all 32 ordered list reads are byte-identical, including both resolutions. There are
only 12 unique lists. Existing readers can follow aliases, but their hard-coded file
size checks must change before the smaller bank can be used.

The canonical font reconstructs every byte of all sixteen shifts exactly. Atlas
dictionary counts include all 4,096 tiles, not just those believed visible:
2,887 are unique. All 5 texel symbols (four greys plus transparency) survive the
3-bit conversion. The combined dictionary size is derived from the same unique
tiles at 24 rather than 32 bytes each; the game sampler is not implemented here.

The 371 ordinary/armed/object frames are all distinct as complete 208-byte records.
Adding a duplicate-frame index there would increase storage by 742 bytes. Mirror
generation is a different operation: perform it before applying the fixed ground
shadow, as the existing offline generator does. Mirroring already-composited shadows
blindly is not the proposed implementation.

The first five candidates save 58,041 bytes together, subject to their implementation
and CPU checks. Adding the combined texture candidate gives 111,633 bytes, taking
data from 399,328 to 287,695 bytes. This is a lossless representation target, not a
new shipped format or a guaranteed frame budget. Do not additionally add overlapping
texture alternatives, whole-bank compression or mirror savings to that total.

## Compression measured with the existing ZX0 implementation

Every host stream round-trips exactly. Native tests call the existing verified
`zx0_asm` under `ti-cycles` and compare length and output Fletcher checksum with the
source. CPU times use MC68000 datasheet counts at 12 MHz, excluding allocation,
checksum, grayscale, interrupts and presentation; they are not TiEmu timings.

| Sample | Raw → ZX0 bytes | Decode CPU ms | Needed output RAM |
|---|---:|---:|---:|
| Whole unit bank | 57,200 → 23,422 | 342.92 | 57,200 |
| Whole weapons bank | 40,704 → 10,783 | 162.63 | 40,704 |
| Whole interface bank | 50,020 → 10,472 | 219.77 | 50,020 |
| Preshifted font | 30,400 → 3,206 | 96.43 | 30,400 |
| Shared mesh | 16,585 → 7,783 | 124.90 | 16,585 |
| Surface/list bank | 31,270 → 8,194 | 134.11 | 31,270 |
| Two inspection fixtures | 12,008 → 3,882 | 56.67 | 12,008 |
| One texture page | 8,192 → 2,501 | 49.39 | 8,192 |
| Ramza's full ordinary class, both mirrors | 9,152 → 4,282 | 57.43 | 9,152 |
| One ordinary frame, sample only | 208 → 159 | 1.29 | 208 |

The complete alternative 3-bit atlas also compresses from 98,304 to 23,571 bytes;
no native decode of that >64-KiB raw stream was attempted. This is not evidence that
it fits RAM. Other rows are representative samples, not averages over every page,
class or sprite pose. Per-frame/per-tile streams need directories and can compress
worse than whole banks.

Whole-bank expansion of the unit and interface data alone requires 107,220 extra
RAM bytes. Current read-in-place assets use Flash directly and consume no such
bulk RAM. Replacing that with whole-bank decompression is therefore a poor default.
An 8-KiB page miss already costs about 49 ms: repeated texture-page decoding inside
a moving frame would be costly. Prefer compact random-access layouts and bounded,
prepared sprite/glyph caches; measure their miss cases before changing the renderer.

## RAM: native ABI sizes, not host sizeof

GCC4TI compiles constant sizeof probes for the actual 68000 layout. No PC pointer
size assumptions are used. The following five large **data** categories describe two
different builds; they are not evidence of a full tactical+camera peak fitting AMS.

| Rank | Data in RAM | Bytes | Notes |
|---|---|---:|---|
| 1 | Current decoded ISO view | **52,320** | One view only: 17,440 colour + 34,880 occlusion bytes |
| 2 | Full tactical UI backing caches | **23,850** | Compact camera review uses 3,220, including pointer placeholders |
| 3 | Optional idle perspective cache | **20,284** | Four allocations; largest 13,600. Compact camera build only in this audit |
| 4 | Mutable ISO scene colour planes | **17,440** | Reach highlights/restoration/camera scrolling |
| 5 | Grayscale driver's allocated planes + double buffer | **15,376** | HW2+ driver allocation 7,688 and GrayDBuf 7,688; excludes LCD MMIO |

Additional important data: movement/topology arrays 5,050 B; water glints 4,004 B;
cached displayed background 4,000 B; main AI arrays/ranks 1,414 B; tactical State
1,014 B (1,046 with camera configuration). Those State bytes are embedded in the
program's writable data and must not be counted twice with the resident executable.

The full plain ISO Work is **43,192 bytes**, including the UI caches and smaller
arrays above. Its allocated view+scene+Work buffers total **112,952 bytes**.
The compact camera Work is **22,762**, giving **92,522 bytes** for those same three
blocks, or **112,806** with its optional idle cache. These totals exclude executable,
grayscale backend, startup/OS screen saves, stack, allocation headers and AMS.

The executable also needs RAM while running: approximately 63.8 KB for current ISO,
65.4 KB for the recorded compact camera binary. Packing the executable saves Flash
but does not remove its expanded execution RAM. The full UI+camera integration and
real maximum free-block headroom must still be measured on a clean AMS boot;
headless malloc success is not an AMS memory-fit test. Earlier real-TiEmu cache
allocation limits remain documented in `PREFETCH.md` and have not been retimed here.

Perspective depth (16,000 B), sprite scale pool (24,000 B), transformed vertices,
turn depth and inspection FWork already alias existing buffers depending on mode.
Do not add them as independent allocations. Keep those mode lifetimes intact when
introducing decompression caches. The idle cache is separate specifically to retain
the ISO scene while preparing inspection.

## Recommended order

1. Remove dead attack-fixture dependencies and the unused marker; alias facing lists
   and exact blade frames. Keep rendered output unchanged; check native replays.
2. Replace the full preshifted font with canonical glyphs and a bounded cache, then
   measure HUD rebuilds and the crowded defeat replay against the frame budget.
3. Prototype compact texture tiles with native per-pixel timing. Retain the current
   80x25 mapper as the control; no additional full atlas RAM copy.
4. Try lazy sprite mirroring or block/frame compression with a small eviction cache;
   check simultaneous actor animation, weapon poses and occluded silhouettes on misses.
5. Revisit UI buffer lifetimes to free RAM before expanding any compression scheme.
   Full-map colours and occlusion must remain coherent when reach marks are restored.

Keep one chosen ISO style; contrast+edges costs only 2,636 packed bytes more than the
original across all four views. Four arbitrary map orientations cannot be derived
by flipping one 2D image: visibility and asymmetric buildings change. Computing them
from geometry at battle start would require a measured native texture renderer and
would trade archive savings for setup time; it is not the current engine path.

For more battles, actor classes, font, generic UI, common terrain textures and math
tables should be global assets. Store only each map's actual geometry/view/UV data;
deduplicate terrain tiles across maps offline. The fixed `fftlook`/`fftshot` fixtures
are prototype-specific and must not multiply for every actor, tile and facing.
