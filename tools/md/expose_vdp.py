#!/usr/bin/env python3
"""Expose read-only VDP snapshots in the pinned local Genesis Plus GX core.

ID 3 is libretro VIDEO_RAM; IDs 0x10000/0x10001 are this runner's extensions
for core CRAM and registers. Preserve upstream line endings; never commit it.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
path = ROOT / 'sources/md_core/libretro/libretro.c'
raw = path.read_bytes()
if b'/* TI port: read-only VDP snapshots */' not in raw:
    for name, cases in (
        (b'void *retro_get_memory_data(unsigned id)',
         b'      /* TI port: read-only VDP snapshots */\r\n'
         b'      case RETRO_MEMORY_VIDEO_RAM: return vram;\r\n'
         b'      case 0x10000: return cram;\r\n'
         b'      case 0x10001: return reg;\r\n'),
        (b'size_t retro_get_memory_size(unsigned id)',
         b'      /* TI port: read-only VDP snapshots */\r\n'
         b'      case RETRO_MEMORY_VIDEO_RAM: return sizeof(vram);\r\n'
         b'      case 0x10000: return sizeof(cram);\r\n'
         b'      case 0x10001: return sizeof(reg);\r\n')):
        pos = raw.index(name)
        anchor = raw.index(b'      case RETRO_MEMORY_SAVE_RAM:', pos)
        raw = raw[:anchor] + cases + raw[anchor:]
    path.write_bytes(raw)
    print('exposed VDP snapshots in local core; rebuild required')
