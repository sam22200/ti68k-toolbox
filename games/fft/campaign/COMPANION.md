# Companion image: specials shared by every chapter

2026-11-10, ABI 85. User decision: the techniques of non-canonical jobs (Holy Sword, Dark
Sword, Wiegraf's and Zalbag's sets...), of monsters and of bosses recur across the four
chapters, so they belong to the core, in one **companion** image, not in chapter packs.
Chapter-pack code (`HW3PATCH_USES.md` § 1) stays an option for the rare one-off.

## Scope

Monster sets alone: 82 actions over the game, 20 enabled; of the 62 left, 19 reuse formulas
the core already has (families 0x29, 0x31, 0x35, 0x38, 0x50), 43 need 19 new formulas
(0x4E alone: 16 actions). The special human sets add Holy Sword (0x2D with status, area,
line), Dark Sword (0x2F), 0x2E, the Ruins (0x1A, 0x1B) and later chapters' sets.

## Mechanism

- Image `MOD_COMPANION` (27) in the core's FMB3 banks, built by `tools/module.py
  --companion 27`: excluded from the zone (`MOD_ZONE`), its size in `MOD_COMPANION_BYTES`;
  the bank validator lets this one id exceed the zone (`module_bank.h`, `big`).
- On the TI (`fft.c`, `special`): installed on first use into its own `malloc` block
  (HW3Patch), with the same context binding as a zone image (`mod_install`); reinstalled
  when the bindings it holds change (Work, map, unit count); freed with the battle's buffers
  (`release`). It never touches the zone: a zone image calling it keeps running.
- One entry for every caller, resident or zone image (`ModCtx.special`):
  `special(a, t, ab, query)`; arguments travel in the companion's own context
  (`cast_actor`, `cast_ref`, `cast_ability`), the query as the image's op.
- On the PC there are no modules: `companion.inc` compiles `companion_query` into the
  program and `special` calls it, so PC, sanitizer and 68000 checks compare the same code.

**Rules for companion code:** compute and return (kind, chance, amount or packed status);
the core keeps commit, status rules, animation, UI and AI. Never call `mod_call` or a
`ModCtx` entry that loads a zone image; only pure resident helpers.

## Measured (`check_banks.c`, `ti-cycles`, 68000 datasheet cycles)

| | cycles |
|---|---:|
| first call (malloc, copy, bind) | 5,682 |
| next calls (rebind test, arguments, probe) | 962 each |
| a zone image swap, for comparison | ~35,000 for an 8 KB image |

Empty companion: 390 bytes (mostly the context). Resident cost of the mechanism: 250 bytes
of TTPack input (headroom 8,743 → 8,493); zone 8,630 → 8,648 (the new context pointer).

## Next

1. Preload at battle start when the encounter needs it (monsters or special sets), and
   count its size in the battle's RAM requirement (`BATTLE_RAM_BYTES`, the RAM message):
   today the first call allocates, which can fail mid-battle (`special` then returns 0).
2. First content: formula 0x4E (16 monster actions), oracle against the original.
3. Families already in the core for humans (0x50, 0x38, Turn Punch), Holy Sword, the rest.
