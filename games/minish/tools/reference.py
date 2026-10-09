#!/usr/bin/env python3
"""Minish Woods USA: offline ROM extraction and a reproducible study door.

This is reference preparation, not a native game. The door injects a minimal
study save into the original loader; it does not assert natural story progress.
"""
import argparse
import ctypes as C
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tools/gba'))
from gbarun import GBA, CORE_REV
from lz77 import decompress

ROM = ROOT / 'roms/gba/Legend of Zelda, The - The Minish Cap (USA).gba'
ROM_SHA256 = 'bedc74df62755f705398273de8ed3bc59be610cf55760d0b9aa277f1f5035e73'
READING_AID_REV = '6fb6dfb4a7efbe24d0fd1dda5097af6131faacde'
OUTPUT = ROOT / 'sources/minish_gba'
MAIN = 0x03001000
ROOM = 0x03000bf0
PLAYER = 0x03001160
SAVE = 0x02002a40
MAP_BOTTOM, MAP_TOP = 0x02025eb0, 0x0200b650
ORIGIN_X, ORIGIN_Y, WIDTH, HEIGHT = 2976, 2160, 1008, 1008

# USA offsets from assets/map.json, confirmed against the matching local ROM.
# name, compressed offset, compressed bytes, decompressed bytes
ASSETS = (
    ('gfx0', 0x324eb4, 11348, 16384),
    ('gfx1', 0x327b08, 9712, 16384),
    ('gfx2', 0x32a0f8, 8432, 16384),
    ('metatiles_bottom', 0x32c1e8, 7036, 16384),
    ('metatiles_top', 0x32dd64, 3468, 16368),
    ('types_bottom', 0x32eaf0, 1072, 4096),
    ('types_top', 0x32ef20, 708, 4092),
    ('map_bottom', 0x32f1e4, 4268, 7938),
    ('map_top', 0x330290, 1544, 7938),
)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def rom_bytes():
    data = ROM.read_bytes()
    if sha(data) != ROM_SHA256:
        raise ValueError('this reference requires the exact documented USA ROM')
    # Room header is independent evidence for dimensions/origin and tileset0.
    actual = [int.from_bytes(data[0x11c488 + i:0x11c48a + i], 'little')
              for i in range(0, 10, 2)]
    assert actual == [ORIGIN_X, ORIGIN_Y, WIDTH, HEIGHT, 0], actual
    return data


def extract():
    """No emulator, save or gameplay is needed for these ROM assets."""
    data = rom_bytes()
    result = {}
    for name, offset, compressed_size, size in ASSETS:
        result[name] = decompress(data[offset:offset + compressed_size], max_output=16384)
        assert len(result[name]) == size, name
    return result


def boot(gba, extra_writes=()):
    """Cold title -> original room loader -> walkable western Woods study spawn."""
    for _ in range(360):
        gba.step([])
    assert gba.read(MAIN + 2) == 0, 'expected cold title task'
    # Empty study save, initialized so FinalizeSave does not redirect to Link's house.
    for offset in range(0x4b4):
        gba.write(SAVE + offset, 0)
    writes = [(SAVE + 1, 1, 1), (SAVE + 2, 1, 1), (SAVE + 3, 1, 1),
              (SAVE + 8, 1, 1), (SAVE + 0xaa, 24, 1), (SAVE + 0xab, 24, 1),
              (SAVE + 0x88, 0, 1), (SAVE + 0x89, 0, 1),
              (SAVE + 0x8a, 4, 1), (SAVE + 0x8b, 0, 1),
              (SAVE + 0x8c, 32, 2), (SAVE + 0x8e, 88, 2), (SAVE + 0x90, 1, 1),
              # Reproduce the ordinary file-select black exit fade. The title's
              # retained white fade otherwise keeps the loaded forest invisible.
              (0x03000fd0, 1, 1), (0x03000fd2, 0, 1), (0x03000fd8, 5, 2),
              (0x03000fda, 256, 2), (0x03000fdc, 256, 2), (0x03000fde, 0, 2),
              (MAIN + 2, 2, 1), (MAIN + 3, 0, 1), (MAIN + 4, 0, 1)]
    writes += list(extra_writes)
    for address, value, size in writes:
        gba.write(address, value, size)
    for _ in range(180):
        gba.step([])
    assert [gba.read(MAIN + i) for i in (2, 3, 4)] == [2, 2, 2]
    assert [gba.read(ROOM + i) for i in (4, 5)] == [0, 0]
    assert [gba.read(ROOM + i, 2) for i in (6, 8, 0x1e, 0x20)] == [
        ORIGIN_X, ORIGIN_Y, WIDTH, HEIGHT]
    assert position(gba) == [32 * 65536, 88 * 65536]
    assert len(set(gba.image().tobytes())) > 32, 'forest must be visibly rendered'
    return writes


def position(gba):
    return [gba.read(PLAYER + offset, 4, True) - origin * 65536
            for offset, origin in ((0x2c, ORIGIN_X), (0x30, ORIGIN_Y))]


def raw(gba, address, size):
    return C.string_at(gba.pointer(address, size), size)


def check_assets(gba, assets):
    """Compare offline decoding with bytes loaded by the unmodified original."""
    checked = 0
    for name, base in (('map_bottom', MAP_BOTTOM), ('map_top', MAP_TOP)):
        # ROM rows have 63 metatiles; the game's expanded RAM rows have 64.
        actual = b''.join(raw(gba, base + 0x3004 + y * 128, 126) for y in range(63))
        assert assets[name] == actual, name
        checked += len(actual)
    for name, address in (('metatiles_bottom', MAP_BOTTOM + 0x7004),
                          ('metatiles_top', MAP_TOP + 0x7004),
                          ('types_bottom', MAP_BOTTOM + 0x5004),
                          ('types_top', MAP_TOP + 0x5004)):
        assert assets[name] == raw(gba, address, len(assets[name])), name
        checked += len(assets[name])
    # Graphics banks are extracted, but animated tiles can replace ROM bytes in
    # VRAM during these 180 frames; RGB extraction fidelity needs a separate check.
    return checked


def digest(gba):
    return {name: sha(data) for name, data in
            {**gba.snapshot(), 'rgb': gba.image().tobytes()}.items()}


def replay(gba, state):
    gba.load(state)
    frames = []
    for frame in range(120):
        gba.step(['RIGHT'] if frame < 20 else ['DOWN'] if frame < 40 else [])
        if frame == 19:
            assert position(gba) == [57 * 65536, 88 * 65536], 'native walking must respond'
        frames.append(digest(gba))
    return frames


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--extract-only', action='store_true', help='read ROM assets without running it')
    args = parser.parse_args()
    assets = extract()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    for name, data in assets.items():
        (OUTPUT / (name + '.bin')).write_bytes(data)
    metadata = {'rom_sha256': ROM_SHA256, 'reading_aid_revision': READING_AID_REV,
                'room': {'area': 0, 'room': 0, 'origin': [ORIGIN_X, ORIGIN_Y],
                         'pixels': [WIDTH, HEIGHT], 'metatiles': [63, 63]},
                'assets': [{'name': name, 'offset': offset, 'compressed_bytes': packed,
                            'bytes': size, 'sha256': sha(assets[name])}
                           for name, offset, packed, size in ASSETS]}
    if not args.extract_only:
        state = OUTPUT / 'woods.state'
        gba = GBA(ROM)
        try:
            writes = boot(gba)
            initial = digest(gba)
            checked = check_assets(gba, assets)
            gba.save(state)
            gba.image().save(OUTPUT / 'entrance.png')
            for name, data in gba.snapshot().items():
                (OUTPUT / (name + '.bin')).write_bytes(data)
            first = replay(gba, state)
            assert first == replay(gba, state), 'restored forest replay differs'
            metadata.update({**gba.identity, 'pinned_revision': CORE_REV,
                'boot': {'title_frames': 360, 'load_frames': 180,
                         'save_reset': [hex(SAVE), 0x4b4],
                         'pokes': [{'address': hex(a), 'size': s, 'value': v} for a, v, s in writes],
                         'study_spawn': [32, 88], 'natural_story_progress': False},
                'checked_decoded_bytes': checked, 'replay_frames': 120, 'replays': 3,
                'initial_hashes': initial, 'state_sha256': sha(state.read_bytes())})
        finally:
            gba.close()
        gba = GBA(ROM)
        try:
            boot(gba)
            assert digest(gba) == initial, 'cold forest door differs'
            assert replay(gba, state) == first, 'fresh-core forest replay differs'
        finally:
            gba.close()
        print(f'Woods: direct original load, 25px/20-frame walk, {checked} decoded bytes and three 120-frame full-memory/RGB replays passed')
    (OUTPUT / 'reference.json').write_text(json.dumps(metadata, indent=2) + '\n')
    print(f'Extracted {sum(map(len, assets.values()))} bytes from nine ROM streams without gameplay')


if __name__ == '__main__':
    main()
