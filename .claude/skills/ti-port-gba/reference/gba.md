# GBA reference details

Use this when studying memory, disassembly or art. Sources: the pinned
[mGBA memory definitions](https://github.com/mgba-emu/mgba/blob/26b7884bc25a5933960f3cdcd98bac1ae14d42e2/include/mgba/internal/gba/memory.h),
[libretro memory descriptors and state loading](https://github.com/mgba-emu/mgba/blob/26b7884bc25a5933960f3cdcd98bac1ae14d42e2/src/platform/libretro/libretro.c),
and [software renderer](https://github.com/mgba-emu/mgba/tree/26b7884bc25a5933960f3cdcd98bac1ae14d42e2/src/gba/renderers).

## Memory and execution

| Canonical CPU range | Export | Bytes |
|---|---|---:|
| `02000000..0203FFFF` | EWRAM | 262144 |
| `03000000..03007FFF` | IWRAM | 32768 |
| `04000000..040003FF` | raw I/O mirrors | 1024 |
| `05000000..050003FF` | palette RAM | 1024 |
| `06000000..06017FFF` | VRAM | 98304 |
| `07000000..070003FF` | OAM | 1024 |

Words are little-endian. A GBA core does not necessarily expose EWRAM through
libretro memory ID2: this revision uses `SET_MEMORY_MAPS`. The callback's
descriptors are stack-local; copy descriptor values immediately, retain only
the core-owned region pointers, and close the core before opening another.
Cartridge save memory comes from ID0 with the reported size; FLASH/EEPROM
protocols are not direct SRAM bus mappings. Do not interpret all saves as a
flat `0E000000` CPU array. ROM has several bus windows, normally beginning at
`08000000`; the runner intentionally does not resolve bus mirrors or open bus.

For Ghidra, load the confirmed ROM image at its actual mapping using a
little-endian ARM processor and the appropriate ARMv4T behavior. Thumb entry
addresses can have bit0 set as a mode marker; clear that bit for code placement
and set Thumb context instead of disassembling every byte as ARM. RAM code
copies, literal pools, indirect calls and overlays require runtime evidence.
Keep the original image and notes local; do not confuse executable pointers
with data solely from alignment. PC-relative ARM and Thumb reads use different
pipeline offsets; a decompiler's pseudocode is not a behavioral oracle.

Probe fractions, signed loads, carry, arithmetic shifts and truncation on the
source before choosing target types. Direct dump reads of unaligned words
are byte slices, not ARM CPU loads with hardware alignment behavior.

## Graphics extraction

Colors are 15-bit packed words (red in bits0..4, green5..9, blue10..14), with
BG and OBJ palette halves. Packed 4-bpp pixels occupy nibbles, low nibble first;
8-bpp pixels occupy bytes. Tile palette index zero is transparent where the
layer rules specify it; backdrop and bitmap pixels need separate treatment.
Exported words use the little-endian host/core layout in the validated build.

Text BG maps use 16-bit entries with tile, flip and palette fields; large maps
are composed of 32×32-entry screen blocks, not necessarily a single linear
row-major image. Affine BG maps use byte tile indices and require measured
transform/reference coordinates. Bitmap modes3/4/5 have distinct color/index,
size and page rules. Inspect DISPCNT/BG control registers before selecting a
decoder. OBJ tile layout depends on the 1D/2D mapping bit; OBJ shape/size,
signed screen wrapping, flips, affine/double-size and disabled bits must be
decoded separately. Affine matrix words share OAM storage with object entries.

Confirm a decoder with opaque pixels of the source video, including flipped
tiles, palette banks and overlapping sprites. Raw post-frame registers do not
capture HBlank DMA or every internal latch used during a completed frame;
add narrow core instrumentation when those effects matter. Flattening layers
is a target adaptation, not evidence of exact extraction.

The pinned libretro build presents RGB565 through the runner's Pillow
`BGR;16` conversion. Match its integer channel expansion/rounding when
checking RGB; shifting each five-bit channel left by three is not identical.
The Minish Woods mode-0 decoder in `games/minish/tools/art.py` passes complete
frame comparisons, including the measured zero-weight alpha BG layer. It
rejects unsupported affine, window, 8-bpp and other blend cases.

In this Minish Cap study, completed OAM/video uses the previous player pose,
allocation and coordinates. Hardware BG offsets also include an eight-pixel
vertical cache margin. Actor anchors and map crops therefore use the previous
entity and room camera. Confirm such relationships on moving frames for each
game; neither rule follows universally from GBA hardware.

The same forest study also measures a one-update delay between live metatile/
collision mutation and its completed background image. Ordinary sword art
uses overlapping player/weapon tile ranges with different palettes; attribute
both by allocation and palette before outlining the merged silhouette. Source
tile interactions use three frame-flag-selected point samples, distinct from
enemy hitboxes. A held button and repeated press edges produce different action
sequences, and a new press can restart and reorient an unfinished swing. Verify
these timing/input distinctions directly; a decompiled rectangle or a single
held-input trial is insufficient.

GBA BIOS LZ77/RLE/Huffman data can be extracted offline or observed after
decompression; identify the actual format before applying a decoder. Reuse
the project's verified TI codecs for target banks rather than carry BIOS or
ARM routines into the native engine.

## Current validation boundary

`tools/gba/check.py` passes on the three currently local revisions of Advance
Wars (Europe), Final Fantasy Tactics Advance (USA) and The Minish Cap (USA):
two cold 360-frame boots and three 120-frame restored sequences per ROM,
eight region/video hashes each frame, keypad press/release, RAM byte-order
and bounds, save-memory restoration and fresh-core replay. ROM hashes appear
in the command's JSON output. These are instrumentation trials near boot;
none establishes gameplay coordinates, combat rules or a native TI port.

The Minish Cap has a game-specific door in `games/minish/tools/reference.py`:
an explicit minimal study save routes through the original room loader to
Minish Woods without an adventure playthrough. Offline LZ77 maps/metatiles/
types match loaded RAM, and native walking, forest replay, original scenery,
actor animation and canopy occlusion are checked. Ordinary sword actions and
all 53 bush cuts additionally match 1947 source steps. Both native scales pass
complete PC/TI states/screens and frame budgets, including fully cleared beds.
M4 has a separate original enemy-enabled door: `TABIDACHI` gates the room's
enemy list. Two cold boots and twenty combat trials replay completely;
608 updates check targeted walking/contact/recoil/sword/shot behavior, with
twenty original enemy poses and960k independent oracle pixels per native scale.
The native encounter also checks death/retry, active-projectile save/load,
clipping and dense scenes under360k cycles. Local AI and rock impact/expiry
are adaptations; original falling/bouncing deflections and drops are deferred.
Read `games/minish/RE_NOTES.md` for the USA address recipe, white-fade trap,
63-to64 row stride and remaining boundaries. Do not silently reuse this
empty study save as normal quest progression or assume story flags/equipment.

The runner uses HLE BIOS and does not freeze RTC, import existing cartridge
saves or provide link/sensor scripts. Add and test only the dependencies the
chosen game requires. Firmware/peripheral configuration belongs in metadata
when support is added.
