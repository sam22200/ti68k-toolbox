# The battle executable's resident budget

Measured 2026-11-24. Everything here was built and weighed, not estimated; where
a figure is an estimate it says so.

The chapter's battle program `fftbat` is the one at its ceiling, and it is the
reason the status pictograms and the transformation creatures are written,
checked on screen and still switched off in `fft.h`. This records what the
ceiling is, what was tried, and the two routes that remain, so none of it has to
be rediscovered.

## The numbers

| | bytes |
|---|---|
| battle executable, nothing added | 64601 |
| TTPack's input ceiling | **64936** |
| free | 335 |
| `core_budget.py` alert threshold | 512 |
| status pictograms, art already archived | 324 |
| transformation creatures, art already archived | 114 |
| both | 458, so **123 over** |

The program was already under the project's own alert line before either feature
existed. Either one fits alone; the pair does not.

## What is already out

**Data.** The pictures live in the archive tail (`tools/contour_assets.py`), the
same one that carries the engine's own tables: `picto_blob`, `picto_of_sid`,
`charge_cycle`, `chicken_gfx`, `frog_gfx`, 4.8 KB. The tail reads the switches in
`fft.h`, so a chapter built without the features carries none of it. What is left
in the program is 117 bytes of small tables in `fft.c` (`mir`, `pos`, `fx`, `dx`,
`dir_x`…), which is not worth the indexing it would cost.

**Cold code.** Twenty-eight module images, three banks. `MOD_ZONE` is 8932 and
equals the largest image.

**The zone is not a lever.** `mod_zone` is a `malloc(MOD_ZONE)`, so it costs RAM,
not program: built with `MOD_ZONE` cut to 4000 the executable is 64601 bytes
either way, byte for byte. Re-splitting the images would hand back heap and no
resident room at all.

## The resident map

Read by joining `objdump -h` section sizes to `objdump -t`'s symbol table on the
chapter object: 214 functions, 61976 bytes of `.text`.

| function | bytes | |
|---|---|---|
| `game_render` | 7038 | the renderer, cannot move |
| `update` | 5680 | the frame loop, cannot move |
| `magic_hit` | 2102 | shared formula, modules call back into it through `ModCtx` |
| `game_scenario` | 1992 | **once per battle** |
| `magic_heal` | 1940 | shared formula |
| `draw_monster_ko` | 1798 | drawing |
| `spread_go` | 1554 | exported through `ModCtx` |
| `ensure_scene` | 1398 | on camera change |
| `unit_card_battle` | 1204 | drawing |
| `battle_runtime_main` | 1014 | the entry |

## Moving `game_scenario` out: the analysis

**The prize is 2704 bytes**, not 1992: measured by replacing the body with
`{(void)n;}` and rebuilding, which takes its exclusive callees with it. That is
twenty-two times what the two features need.

**It cannot be a zone module.** `game_scenario` calls `mod_call`, and so do two of
its callees:

| | body | calls `mod_call` |
|---|---|---|
| `game_scenario` | 11106 chars | yes |
| `chapter_load` | 3539 chars | yes |
| `objective_bind` | 383 chars | yes |

A module running in the zone that calls `mod_call` loads another image over
itself. The battle's start sequence is the module system's conductor; it cannot
be one of the players.

**The only route is the companion block.** `comp_block` is a separate
`malloc(FFT_CORE_COMPANION_MAX)` that already hosts two mutually exclusive owners,
`MOD_COMPANION` and `MOD_SAVE_READER`, and is never the zone, so code there may
call `mod_call`. `comp_bind` already re-installs on an owner change, so a third
owner costs no new eviction logic.

What it would take:

- the chapter path of the three functions calls **42 distinct functions**, of
  which 3 are already reachable through `ModCtx` and about 14 exist only in the
  `#ifndef FFT_CHAPTER` test doors. So roughly **25 new `ModCtx` exports**, each a
  pointer and its assignment, about 200 bytes coming back. Net about 2500.
- `sizeof(ModCtx)` is part of `MOD_SIG`, so **the ABI bumps and all 28 images
  rebuild**. That is all-or-nothing: there is no half-applied state.
- about 30 PC checks call `game_scenario`, which is good news for catching
  mistakes and bad news for the size of a bad day.

The full call list, test-door entries included, for whoever picks this up:
`actor_reset, aim, attack, attack_range, attack_step, battle_units, bn_start,
cam_target, chapter_files, chapter_grid, chapter_load, chapter_route,
compute_reach, edges, end_battle, guest_load, guests_restore, make_path,
map_start, memset, mod_call, mod_context, monster_body_valid, monster_ko_valid,
movement_profile, nav_reserve, objective_bind, objective_load, opening_start,
perspective_enter, rand_seed, resource_file, rt_ticks, scenario_prepare,
scene_replace, skill, turn_end, turn_start, unit_stats`.

**What a load costs**, since that is the price of one more module:
`mod_install` is three `memcpy`s of the image into the zone, about 6 cycles a
byte (`experiments/heapbig/modcopy.c` under ti-cycles, which estimates AMS ROM
calls and sees flat memory where the real copy reads Flash, so treat it as a
floor). The largest image is about 54k cycles, a sixth of one 30 fps frame, once.
A one-shot module loaded at battle start is free.

## Raising the ceiling instead

**The platform wall is 65518 bytes** for any one contiguous code image
(`experiments/heapbig/`, verified): with 188982 bytes of heap free `HeapAllocPtr`
grants 65000 and refuses 65520. TTPack stops 582 bytes below that wall.

**TTPack's limit is one line.** `tools/gcc4ti/trunk/tigcc/ld-tigcc/export/pucrunch.c`:

```c
int startAddr = 0x258;                                   /* line 1579 */
if (startAddr + inlen - 1 > 0xffff) {                    /* line 1602 */
    Error(NULL, "File is too large to handle (>64936 Bytes)");
```

600 + 64936 - 1 = 65535 exactly. The 600 is pucrunch's C64 load address and means
nothing on a TI. Lowering it, or testing `inlen` alone, would raise the ceiling to
about 65535 - enough for both features with room over. The risk is that the
on-calc decompressor may assume the same 16-bit window including that offset, and
getting it wrong corrupts a program silently rather than failing. GCC4TI is
vendored and rebuilt by `tools/build-gcc4ti.sh`, so the patch would have to live
with that script. **Not attempted.**

**Our own ZX0 self-extractor cannot replace it.** The idea was a small stub that
reads the compressed image from an archived variable, decompresses it into a heap
block with `lib/unpack68k.s` and jumps. A 65 KB image decompressed to an address
nobody knew at link time has to be relocated, and the project's existing
position-independent images avoid relocation by being built `-mpcrel`, which the
68000 limits to a 32 KB displacement. A 65 KB `-mpcrel` image is not possible, so
the stub would have to apply ld-tigcc's relocations itself. That is reimplementing
the loader, not swapping a decompressor.

**Not packing `fftbat` at all works and costs too much.** Built plain with both
features it is 65059 bytes, under the 65518 variable limit with 459 to spare, so
it would ship today. But the packed form is a 1095-byte stub plus a 45414-byte
archived core, and AMS retains each archived executable's RAM copy until the
supervisor returns - the warning is in `Makefile` above the `packed` target.
Unpacked, `fftbat` would hold about 64 KB of RAM for the whole session instead of
a stub plus a heap block it frees, and cost about 18.5 KB more archive per
chapter. On a machine where RAM is the scarcest resource that is the wrong trade.

## Where this leaves it

In increasing order of what they cost and what they buy:

1. **Patch pucrunch's start address.** One line, about 600 bytes, unblocks both
   features. Needs someone to read the on-calc decompressor first.
2. **Move `game_scenario` to the companion block.** About 2500 bytes, room for
   several features and for the monsters still missing. One ABI bump, about 25
   exports, and a day that goes well or not at all.
3. Anything that adds resident drawing code from here needs one of the two first.
