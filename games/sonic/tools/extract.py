#!/usr/bin/env python3
"""Extract REV00 collision terrain and a diagnostic TileMap from the local ROM.

The real game decompresses its chunks at boot. This tool consumes that RAM,
plus ROM heightmaps; no third-party disassembly files are needed to build.
All emitted maps, profiles, tiles and reference samples remain ignored.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[3]
GAME = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/md'))
from mdrun import MD

ROM_HASH = '46160baa06362c711c9f1a5017cb7371026444936c8af5e93a78996cf32ff2a6'
WORLD_W = 1664
COLS = ROWS = 128


def main():
    rom = ROOT / 'roms/md/Sonic_1.md'
    if not rom.exists():
        sys.exit('missing local roms/md/Sonic_1.md (commercial, not distributed)')
    raw = rom.read_bytes()
    if hashlib.sha256(raw).hexdigest() != ROM_HASH:
        sys.exit('terrain offsets are verified for Sonic REV00 only')
    md = MD(rom)
    try:
        md.load(ROOT / 'sources/sonic1_md/start.state')
        if md.read(0xfff600) != 12 or md.read(0xfff796, 4) != 0x64a00:
            sys.exit('reference must be the verified GHZ1 start state; run make reference')
        profiles = [(tuple([0] * 16), 0, 0)]
        ids = {profiles[0]: 0}
        grid = bytearray(COLS * ROWS)
        for ty in range(ROWS):
            for tx in range(WORLD_W // 16):
                chunk = md.read(0xffa400 + (ty >> 4) * 128 + (tx >> 4)) & 127
                descriptor = (md.read(0xff0000 + (chunk - 1) * 512 +
                                     (((ty & 15) << 4) + (tx & 15)) * 2, 2) if chunk else 0)
                block, flags = descriptor & 2047, (descriptor >> 13) & 3
                height_id = raw[0x64a00 + block] if block and flags else 0
                if not height_id:
                    continue
                heights = []
                for x in range(16):
                    value = raw[0x62a00 + height_id * 16 + (15 - x if descriptor & 0x800 else x)]
                    value = value - 256 if value > 127 else value
                    if descriptor & 0x1000:
                        value = -value
                    heights.append(value)
                angle = raw[0x62900 + height_id]
                if descriptor & 0x800:
                    angle = -angle & 255
                if descriptor & 0x1000:
                    angle = (128 - angle) & 255
                # Odd angles are cardinal snap markers, not one-unit slopes.
                if angle & 1:
                    angle = (angle + 32) & 192
                key = (tuple(heights), angle, flags)
                if key not in ids:
                    ids[key] = len(profiles)
                    profiles.append(key)
                if ids[key] > 255:
                    sys.exit('too many collision profiles for byte grid')
                grid[(ty << 7) + tx] = ids[key]
    finally:
        md.close()

    def solid(x, y):
        heights, angle, flags = profiles[grid[((y >> 4) << 7) + (x >> 4)]]
        height = heights[x & 15]
        return height > 0 and (y & 15) >= 16 - height or height < 0 and (y & 15) < -height

    # Diagnostic silhouette at half scale: dark surface, light-gray interior.
    # Four source samples per destination pixel, preserving narrow solid edges.
    tile_ids, tiles, view = {}, [], bytearray()
    view_w, view_h = WORLD_W // 32, 2048 // 32
    for ty in range(view_h):
        for tx in range(view_w):
            words = []
            for y in range(16):
                light = dark = 0
                for x in range(16):
                    sx, sy = tx * 32 + x * 2, ty * 32 + y * 2
                    occupied = any(solid(sx + dx, sy + dy) for dx in (0, 1) for dy in (0, 1))
                    edge = occupied and sy > 1 and not solid(sx, sy - 2)
                    if occupied:
                        light |= 1 << (15 - x)
                    if edge:
                        dark |= 1 << (15 - x)
                words.extend((dark, light))  # ExtGraph TileMap plane order.
            key = tuple(words)
            if key not in tile_ids:
                tile_ids[key] = len(tiles)
                tiles.append(key)
            if tile_ids[key] > 255:
                sys.exit('too many diagnostic tiles for runtime TileMap')
            view.append(tile_ids[key])

    profile_offset = 32 + len(grid)
    view_offset = profile_offset + len(profiles) * 64
    tile_offset = view_offset + len(view)
    # Verified REV00 GHZ1 object layout: six-byte records, terminated by XFFFF.
    # Object IDs carry a remember-state flag in bit 7; Y carries flip flags.
    spacing = [(16, 0), (24, 0), (32, 0), (0, 16), (0, 24), (0, 32),
               (16, 16), (24, 24), (32, 32), (-16, 16), (-24, 24), (-32, 32),
               (16, 8), (24, 16), (-16, 8), (-24, 16)]
    objects = []
    for offset in range(0x6b096, 0x6b096 + 1290, 6):
        x, y, kind, subtype = struct.unpack_from('>HHBB', raw, offset)
        if x == 65535 or x >= 1536:
            break
        kind &= 127
        if kind == 0x25:
            dx, dy = spacing[subtype >> 4]
            for i in range(min(subtype & 7, 6) + 1):
                objects.append((x + dx * i, (y & 4095) + dy * i, kind, 0))
        elif kind in (0x40, 0x22, 0x2b):
            objects.append((x, y & 4095, kind, (y >> 14) & 1))
    assert sum(o[2] == 0x25 for o in objects) == 14
    assert sum(o[2] != 0x25 for o in objects) == 3
    object_offset = tile_offset + len(tiles) * 64
    header = b'SNC2' + struct.pack('>10H', COLS, ROWS, len(profiles), len(tiles),
                                view_w, view_h, profile_offset, view_offset, tile_offset, WORLD_W)
    header += struct.pack('>HH', object_offset, len(objects))
    header = header.ljust(32, b'\0')
    for endian, suffix in (('=', ''), ('>', '.be')):
        output = bytearray(header + grid)
        for heights, angle, flags in profiles:
            rows = []
            for y in range(16):
                row = sum(1 << (15 - x) for x, height in enumerate(heights)
                          if height > 0 and y >= 16 - height or height < 0 and y < -height)
                rows.append(row)
            output.extend(struct.pack(endian + '16H', *rows))
            output.extend(bytes(height & 255 for height in heights))
            output.extend(bytes((angle, flags)))
            output.extend(bytes(14))
        output.extend(view)
        for tile in tiles:
            output.extend(struct.pack(endian + '32H', *tile))
        for obj in objects:
            output.extend(struct.pack('>HHBB', *obj))
        assert len(output) < 65518
        (GAME / ('sonterr' + suffix + '.bin')).write_bytes(output)
    (GAME / 'x').mkdir(exist_ok=True)
    (GAME / 'x/terrain.json').write_text(json.dumps({'rom_sha256': ROM_HASH,
        'width_md': WORLD_W, 'profiles': len(profiles), 'tiles': len(tiles),
        'bytes': len(output), 'bridge': {'left': 1088, 'right': 1280, 'top': 896},
        'endpoint_md': 1536, 'objects': objects,
        'object_layout_offset': '0x6b096'}, indent=2) + '\n')
    print(f'terrain: {len(profiles)} profiles, {len(tiles)} tiles, {len(output)} bytes')


if __name__ == '__main__':
    main()
