#!/usr/bin/env python3
"""Expose PPU snapshots in the pinned local Snes9x core; never copy its code."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
path = ROOT / 'sources/snes_core/libretro/libretro.cpp'
raw = path.read_bytes()
marker = b'/* TI port: PPU snapshots */'
if marker not in raw:
    for name, cases in (
        (b'void* retro_get_memory_data(unsigned type)',
         b'        /* TI port: PPU snapshots */\n'
         b'        case 0x10000: return PPU.CGDATA;\n'
         b'        case 0x10001: return PPU.OAMData;\n'
         b'        case 0x10002: return Memory.FillRAM + 0x2100;\n'),
        (b'size_t retro_get_memory_size(unsigned type)',
         b'        /* TI port: PPU snapshots */\n'
         b'        case 0x10000: return sizeof(PPU.CGDATA);\n'
         b'        case 0x10001: return sizeof(PPU.OAMData);\n'
         b'        case 0x10002: return 64;\n')):
        pos = raw.index(name)
        anchor = raw.index(b'        case RETRO_MEMORY_SNES_SUFAMI_TURBO_A_RAM:', pos)
        raw = raw[:anchor] + cases + raw[anchor:]
    path.write_bytes(raw)
    print('exposed PPU snapshots in local core; rebuild required')
