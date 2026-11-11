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

## Rule: nothing of the battle core is duplicated (user, 2026-11-10)

The companion holds only rules the core lacks. When a technique is a variant of a core
formula, the variant goes into the core's own code (a flag, a parameter), not a copy in the
companion; companion code reaches core arithmetic through `ModCtx`, never a second copy.

## Formula 0x4E (2026-11-10)

`battle_formula_damage_ma_times_y` is formula 0x08 without Faith (magical evade, MA x Y with
the same element strengthen, support/status, absorb/null stages and status-proc draw). So
it lives in the core: a magic formula (`cast_rules.h`: `magic_formula`,
`magic_damage_formula` at the six former `formula == 8` sites) whose power carries bit 7 to
`magic_amount` = no Faith (formula 0x08 powers are at most 96). No companion code.
Enabled (`common_tables.truth_magic_record`, single-target shapes): Choco Meteor, Small Bomb,
Flame Attack, Thunder/Aqua/Ice Soul, 0x126, Feather Bomb. Area and line shapes (0x117,
0x13B, 0x142, 0x144, 0x14E, 0x151-0x153) wait for their geometry.
Checked (`check_discovery.c`, 17,804 checks, PC = sanitizers = 68000): amount = formula
0x08's with both Faiths at 100, unchanged by Faith, 0 on a nullified element, the real CT-0
cast's HP change. The reference is the core's formula 0x08 (verified on the original); a
direct 0x4E original-function oracle is not yet written. Chapter I: 129 of 155 executable.

## Next

1. Done: preloaded at battle start, before the battle's buffers (`companion_reserve`), into
   a block of the `FFT_CORE_COMPANION_MAX` envelope (3,072 bytes, `core_limits.h`; module.py
   refuses a bigger image), counted in `BATTLE_RAM_BYTES` and the RAM message (held bit 8);
   no allocation during play. Native (TiEmu, Orbonne): the 3,074-byte block sits beside the
   zone (8,670). Cost: ~3 KB less free battle RAM. First call (rebind to the battle's
   Work/map) 7,424 cycles, next 1,078. Raise the envelope with the image.
2. A direct original-function oracle for 0x4E; its area/line shapes.
3. Done: Holy Sword 0x2D in the core (a weapon-strike variant, `HOLY_SWORD.md`), Holy Explosion on
   Earth Slash's line, Dark Sword 0x2F (a drain variant), Mighty Sword 0x2E (a strike plus
   an equipment break), the Ruins 0x1A/0x1B (`RUINS.md`): 142 of 155; Eye
   Gouge, Poison Nail, Beak (0x50 with evasion), Turn Punch, Mind Blast and Death Sentence
   (0x3D), Odd Soundwave, Beaking, Goblin Punch, Self Destruct
   (`MONSTER_STATUS_STRIKES.md`): 152 of 155; Black Ink, Look of Devil, Look of Fright
   (direct targeting): 155 of 155. The companion image is still empty: every Chapter I
   technique was a variant of a core family.
4. Families already in the core for humans (0x50, 0x38, Turn Punch), the rest.
