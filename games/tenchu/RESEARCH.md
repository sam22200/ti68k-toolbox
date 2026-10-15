# Historical renderer research

Status terms: **INSPECTED** means actual source/page read; **MEASURED HERE** means
this project's TI binary under datasheet-cycle emulation; **REPORTED** means another
author's result; **UNTESTED** is a suggestion, not a performance claim.

## Highest-priority height raycaster / FAT

INSPECTED: [dzoba/ti-89-raycasting-with-z](https://github.com/dzoba/ti-89-raycasting-with-z),
local `sources/ti89_raycast_z`, including `src/fatd_dungeon.c`, `fatd_stairs.c`,
`src/includes/fat.h`, `tools/build_dungeon.sh`, and SDK `engine/fat.c`,
`scale_it.c`, `scalers.c`, `transscalers.c`, `tools/genscalers.c`, `readme.txt`.

FAT supplies fixed-angle tables, grid intersection tracing, masked texture strips,
depth information and generated assembly wall/sprite scalers. Descend's extension
owns `floor_heights`, eye Z, height-transition segments, stairs, camera-height movement,
column distance tests and custom textured billboards. Its 96x96 view uses paired
columns, a six-segment cap and expanded 64x64 shade tables. White is also discarded
in its sprite shade conversion; that loses opaque white outlines. Our independent
implementation retains opacity separately. No FAT/Descend code or textures are linked.

The extension's build uses Docker GCC4TI, `-O3`, packing, a FAT DLL/texture bank and
a launcher. Its `DRY_RUN` still requires a ROM path. The original SDK only documents
TIGCC 0.93 support. The repo's blanket Titanium limitation is narrower than the local
knowledge base: heap execution can work with HW3Patch. Our NOSTUB renderer needs
neither that DLL nor generated heap code. FAT's advertised 560–825 frames/minute is
REPORTED, about 9.3–13.75 FPS at its historical 10 MHz conditions, not our baseline.

## Doom89 is a FAT client, distinct from PC Doom

INSPECTED: [Doom89](https://www.ticalc.org/archives/files/fileinfo/405/40593.html),
[SDK](https://www.ticalc.org/archives/files/fileinfo/406/40627.html),
[FPS archive](https://www.ticalc.org/pub/89/asm/games/fps/), and downloaded
`sources/doom89_sdk/Source/Doom89.c`, `sys.h`, `rayline.h`, README/TPR.
The client calls `FAT_LoadEngine`, `FAT_Render`, animated texture updates and HUD
interfaces. Its published package has a 1,095-byte launcher, 29,209-byte packed data,
54,687-byte FAT bank and several approximately 64 KB graphics banks. Those are file
sizes, not peak live RAM. The archive explicitly requires HW3Patch on Titanium.
No reliable reproducible scene/clock/FPS dataset was found for Doom89; do not invent one.

INSPECTED: [Sanglard's Doom renderer account](https://www.fabiensanglard.net/doomIphone/doomClassicRenderer.php),
[BSP/visibility notes](https://afshari.lu/booklets/doom-source/bsp-visibility-and-rendering/).
Stock Doom traverses BSP near-first, clips solid horizontal column intervals,
collects floor/ceiling visplanes and defers masked objects. This is constrained 2.5D,
not free polygon rendering. The architecture's front-to-back visibility is transferable;
PC cache/prefetch explanations are not a 68000 optimization. We did not implement
or benchmark a full BSP/visplane renderer in addition to the five requested paths.

## Atari VCS and compact output

INSPECTED: [Javanaise's VCS raycaster](https://github.com/Javanaise/vcs-demo-raycaster),
its C++ study and assembly source under `sources/vcs_raycaster`. It uses 32 rays,
bit-packed colours and four-bit heights, then XOR row deltas, within the VCS's
128-byte RAM problem. The output representation is 21 bytes before conversion;
approximately 35 bytes of display data is the author's recollection, not our result.
Transferable: coarse columns, quantized heights, precomputed functions and compact
span descriptions. TIA cycle-scheduled scanout and multiplication lookup machinery
are not directly useful on a 68000 with word multiplies and a buffered LCD.

## Independent portals and modern suggestions

INSPECTED author discussions:
[portal wall prototype](https://www.reddit.com/r/GraphicsProgramming/comments/1kis3od/),
[portal precision](https://www.reddit.com/r/retrogamedev/comments/p3mfb9/),
[camera rotation](https://www.reddit.com/r/gameenginedevs/comments/1l6ggxk/),
[software rendering experiments](https://www.reddit.com/r/GraphicsProgramming/comments/1v1jvdb/).
These are primary reports/discussions, not verified 68000 results. Parent/child screen
span intersection, bounded traversal and early rejection are useful. Claims that
portals eliminate every overlapping pixel are not generally true. SIMD/SoA, tiles,
threads and Ryzen full-HD figures do not predict TI performance.

INSPECTED: [bnied's BSP renderer](https://github.com/bnied/BSPRenderer/blob/main/cpp/README.md)
uses a C++20/SDL3 Doom-style BSP and visplanes, at a much larger internal resolution;
its README says no sprites. [Lode's raycasting tutorial](https://lodev.org/cgtutor/raycasting.html)
is a useful DDA sanity reference, not the historical performance baseline.

Local prior research is more directly applicable: X3D's source and CodeWalrus account
in the project's game-techniques section 5 describe convex-room/frustum wire edges,
shared-plane rejection and a proposed screen-span method. The author's 15–20 FPS
report is not measured here. Our Phase 3 is a bounded row-span portal tree followed
by painter rendering, independently of BSP; no zero-overdraw claim is made.

## Voxel background and demoscene search

INSPECTED: [Kai Kostack's Voxel2Space / V2](https://kaikostack.com/?p=376), a heightfield
terrain renderer, not arbitrary volumes or one of the five priorities. V2 changes its
memory/address layout to reduce sampling arithmetic. Our already measured local
`experiments/demoscene/voxel.c` and verified asm variant are the appropriate cycle
reference; they were not reimplemented or relabeled as this work's results.

Searched Pouet, Demozoo, AtariAge, GameDev.net, Atari ST/68k discussions and the
provided Reddit communities. Relevant primary artifacts include
[TI-68k Pouet releases](https://www.pouet.net/prodlist.php?platform%5B%5D=TI-8x+%2868k%29),
[Demozoo's calculator catalog](https://demozoo.org/platforms/27/),
[ST-NICCC](https://www.pouet.net/prod.php?which=1251), and VCS author's AtariAge credits.
The local knowledge base already measures NICCC-style spans, voxels and effects.
Amiga blitter results require hardware the TI does not have. No additional AtariAge
or GameDev.net suggestion established a reproducible faster 68000 renderer here.
Precomputed draw commands, dirty camera updates and polygon depth/coverage grids remain
UNTESTED for this map. Static last-frame reuse and low resolution are MEASURED HERE.

## Original hardware facts and evidence

Target is original TI-89 HW2, not Titanium: nominal 12 MHz 68000; HW1 sensitivity
is shown separately at 10 MHz. No physical clock/battery measurement was available.
256 KB physical RAM, roughly 188 KB user RAM; original user archive maximum
702 KB versus Titanium's 2.7 MB, per [Texas Instruments' archive table](https://education.ti.com/en/customer-support/knowledge-base/ti-83-84-plus-family/general-information/12174).
Live free memory depends on AMS, apps and variables; 188 KB is not a guaranteed
allocatable contiguous block. The prototype's workspace allocation sizes are reported.

The visible LCD is 160x100, but runtime/ExtGraph plane pitch is 30 bytes and storage
240x128 = 3,840 bytes. Original HW2 grayscale is driven by plane copies/interrupts,
not four hardware grayscale pixels; its cost and ordering are already verified in
the project's C-patterns/performance references. TiEmu's `movem` and shift timing is
wrong. Use `ti-cycles`, and do not claim the emulator establishes real machine FPS.
The final private stock AMS 2.09 emulator run establishes program launch/archive
and LCD-path compatibility only. Actual four-tone drawing remains a later milestone.

## Licenses and local data

- Descend's custom source/tools are MIT according to its license; this does not
  license the original TiCT FAT SDK. Bundled SDK files retain TiCT copyright and
  no explicit open-source license was found in those files. None are incorporated.
- Doom89 has GPL notices. The root SDK README contains GPL v3 text; the separate
  source license file includes an older GPL notice. Do not assume these notices
  relicense FAT or graphics. No Doom89 code/data is incorporated.
- VCS source has an MIT license; bnied's example declares GPLv2. Neither is linked.
- Runtime/ExtGraph/GCC4TI are existing project dependencies. Only the established
  verified multiplication idiom is reused; no new asm has been authored.
- Tenchu ROM, extracted files, meshes, textures, states and native screenshots
  remain local/ignored. Scripts contain format descriptions and measured facts,
  not game code. Baked benchmark human silhouettes are original placeholder art.

Tenchu's AFS work was also informed by the author's
[archive analysis](https://boricj.net/tenchu1/2024/03/18/part-6.html),
[archive maker analysis](https://boricj.net/tenchu1/2024/03/25/part-7.html), and
[AFS200 format documentation](https://github.com/stedestro/AFS200Tools).
The extractor is independently written and checks the local USA archive identity.
