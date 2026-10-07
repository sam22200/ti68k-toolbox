#!/usr/bin/env python3
"""Decode the local PAL level's full Map16 collision workspace, offline."""
import hashlib
import json
import struct
from pathlib import Path
from reference import ROOT, OUT, ROM, ROM_HASH

TOP = 1536
WIDTH = 80
HEIGHT = 32
STRIDE = 128
PAGE_INFO = 0x53b12
SHAPES = 0x53d0e
GAME = ROOT / 'games/yoshi'


def tile(sram, wram, tx, ty):
    screen = sram[0xcaa + ((ty >> 4) << 4) + (tx >> 4)] & 63
    offset = 0x18000 + (screen << 9) + ((ty & 15) << 5) + ((tx & 15) << 1)
    return struct.unpack_from('<H', wram, offset)[0]


def properties(rom, number):
    flags, special, shape = rom[PAGE_INFO + (number >> 8) * 3:][:3]
    # Water and dynamic actor interactions are excluded from this terrain bank.
    flags &= 7
    if flags & 4:
        heights = [struct.unpack_from('b', rom, SHAPES + (shape << 7) + (x << 3) + 3)[0] + 1
                   for x in range(16)]
        angle = rom[SHAPES + (shape << 7) + 2]
    else:
        heights, angle = [0] * 16, 0
    return flags, angle, heights


def load_map():
    rom = ROM.read_bytes()
    assert hashlib.sha256(rom).hexdigest() == ROM_HASH
    assert rom[PAGE_INFO:PAGE_INFO + 18] == bytes.fromhex(
        '00 00 00 02 00 00 04 08 00 04 08 01 00 00 00 04 08 02')
    sram = (OUT / 'start.sram').read_bytes()
    wram = (OUT / 'start.wram').read_bytes()
    numbers = [[tile(sram, wram, x, TOP // 16 + y) for x in range(WIDTH)]
               for y in range(HEIGHT)]
    return rom, numbers


def surfaces(rom, numbers, x):
    result = []
    for row, cells in enumerate(numbers):
        flags, angle, heights = properties(rom, cells[x >> 4])
        if flags:
            result.append((TOP + (row << 4) + heights[x & 15], flags, angle, cells[x >> 4]))
    return result


def main():
    rom, numbers = load_map()
    pages = sorted({n >> 8 for row in numbers for n in row})
    result = {'source': json.loads((OUT / 'start.json').read_text()),
              'format': 'screen index at $700CAA; screen&63, 256 LE words at $7F8000+screen*512',
              'page_info_rom_offset': PAGE_INFO, 'slope_rom_offset': SHAPES,
              'bounds': {'x': [0, 1280], 'y': [TOP, TOP + HEIGHT * 16]},
              'pages': {hex(p): list(rom[PAGE_INFO + p * 3:PAGE_INFO + p * 3 + 3]) for p in pages}}
    (OUT / 'terrain.json').write_text(json.dumps(result, indent=2) + '\n')
    kinds = [(0, 0, [0] * 16)]
    cells = bytearray(STRIDE * HEIGHT)
    for y, row in enumerate(numbers):
        for x, number in enumerate(row):
            p = properties(rom, number)
            if p not in kinds:
                kinds.append(p)
            cells[(y << 7) + x] = kinds.index(p)
    profiles = bytearray()
    for flags, angle, heights in kinds:
        profiles.extend(bytes((flags, angle)) + bytes(h & 255 for h in heights) + bytes(14))
    # The actual four-grey scene has fewer than256 unique 16x16 tiles.
    # Deduplicate offline and use the already verified ExtGraph TileMap engine.
    # Retain tightly packed image bytes as an independent pixel oracle.
    scene = (GAME / 'generated/scene.bin').read_bytes()
    assert len(scene) == 49152
    view_w, view_h = 48, 24
    tiles = [tuple([0] * 32)]
    view = bytearray(view_w * view_h)
    for by in range(16):
        for bx in range(40):
            rows=[]
            for py in range(16):
                offset=(by*16+py)*96+bx*2
                light=struct.unpack_from('>H',scene,offset)[0]
                dark=struct.unpack_from('>H',scene,24576+offset)[0]
                rows.extend((dark,light))
            t=tuple(rows)
            if t not in tiles: tiles.append(t)
            view[by*view_w+bx]=tiles.index(t)
    assert len(tiles)<=256
    po, vo = 32 + len(cells), 32 + len(cells) + len(profiles)
    to=vo+len(view)
    image_offset=to+len(tiles)*64
    image_stride,image_height=80,256
    image_planes=[b''.join(scene[base+y*96:base+y*96+80] for y in range(256)) for base in (0,24576)]
    header = b'YTR4' + struct.pack('>14H', WIDTH, HEIGHT, TOP, STRIDE,
             len(kinds), view_w, view_h, len(tiles), po, vo, to,
             image_offset + 2 * image_stride * image_height, image_offset, image_stride)
    for endian, suffix in (('=', ''), ('>', '.be')):
        bank = bytearray(header + cells + profiles + view)
        for t in tiles: bank.extend(struct.pack(endian + '32H', *t))
        bank.extend(image_planes[0]); bank.extend(image_planes[1])
        (GAME / ('yjterr' + suffix + '.bin')).write_bytes(bank)
    result.update({'kinds': len(kinds), 'scene_tiles': len(tiles), 'bank_bytes': len(bank)})
    (OUT / 'terrain.json').write_text(json.dumps(result, indent=2) + '\n')
    print('Terrain:', len(cells), 'padded collision cells,', len(kinds), 'profiles,',
          len(tiles), 'scene tiles,', len(bank), 'bytes')


if __name__ == '__main__':
    main()
