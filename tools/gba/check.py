#!/usr/bin/env python3
"""Local-ROM integration checks; boot/input/replay, not a native TI port."""
import argparse
import hashlib
from pathlib import Path
import tempfile

from gbarun import GBA, ROOT, REGIONS


def digest(gba):
    return {name: hashlib.sha256(data).hexdigest()
            for name, data in {**gba.snapshot(), 'rgb': gba.image().tobytes()}.items()}


def buttons(frame):
    if frame % 40 < 10:
        return ['RIGHT', 'A']
    if frame % 40 < 20:
        return ['LEFT', 'B']
    return []


def boot(gba):
    for frame in range(360):
        gba.step(['START'] if frame in (120, 240) else [])


def check(rom):
    with tempfile.TemporaryDirectory() as directory:
        state = Path(directory) / 'boot.state'
        gba = GBA(rom)
        try:
            boot(gba)
            assert 59.7 < gba.fps < 59.8, gba.fps
            assert gba.frame[1:3] == (240, 160)
            assert len(set(gba.image().tobytes())) > 4, 'blank boot video'
            for name, (_, size) in REGIONS.items():
                assert len(gba.memory(name)) == size
            cold = digest(gba)
            gba.save(state)
            # Direct pokes check byte order and boundaries, not game semantics.
            original_word = gba.read(0x0203fffc, 4)
            original_short = gba.read(0x03007ffe, 2)
            gba.write(0x0203fffc, 0x89abcdef, 4)
            assert gba.memory('ewram')[-4:] == b'\xef\xcd\xab\x89'
            assert gba.read(0x0203fffc, 4, True) == -1985229329
            gba.write(0x03007ffe, -123, 2)
            assert gba.read(0x03007ffe, 2, True) == -123
            for address, size in ((0x02040000, 1), (0x03007fff, 2),
                                  (0x06017fff, 2), (0x08000000, 1),
                                  (0x02000000, 3)):
                try:
                    gba.read(address, size)
                except ValueError:
                    pass
                else:
                    raise AssertionError('out-of-range read accepted')
            try:
                gba.write(0x04000000, 0, 2)
            except ValueError:
                pass
            else:
                raise AssertionError('raw I/O poke accepted')
            savedata = gba.savedata()
            if savedata:
                # Assert cartridge save restoration even if gameplay never writes it.
                import ctypes as C
                C.memset(gba.lib.retro_get_memory_data(0), 0x5a, len(savedata))
            gba.load(state)
            assert gba.savedata() == savedata, 'cartridge save was not restored'
            assert gba.read(0x0203fffc, 4) == original_word
            assert gba.read(0x03007ffe, 2) == original_short

            def replay():
                gba.load(state)
                samples = []
                for frame in range(120):
                    gba.step(buttons(frame))
                    samples.append(digest(gba))
                return samples

            first = replay()
            assert first == replay(), 'RAM/save/PPU/RGB differs on a restored frame'
            gba.load(state)
            gba.step(['RIGHT', 'A'])
            # GBA KEYINPUT bits 0=A, 4=RIGHT are active low, unlike libretro IDs.
            assert gba.read(0x04000130, 2) & 0x3ff == 0x3ee
            gba.step([])
            assert gba.read(0x04000130, 2) & 0x3ff == 0x3ff
        finally:
            gba.close()
        gba = GBA(rom)
        try:
            boot(gba)
            assert digest(gba) == cold, 'cold boot is not reproducible'
            # A fresh process-like core load must support the same state container.
            gba.load(state)
            for frame, expected in enumerate(first):
                gba.step(buttons(frame))
                assert digest(gba) == expected, f'fresh-core replay differs at {frame}'
        finally:
            gba.close()
    return {'rom': rom.name, 'sha256': hashlib.sha256(rom.read_bytes()).hexdigest(),
            'cold_boot_frames': 360, 'restored_frames': 120,
            'replays': 3, 'regions': [*REGIONS, 'savedata', 'rgb']}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path, nargs='?')
    args = parser.parse_args()
    roms = [args.rom] if args.rom else sorted((ROOT / 'roms/gba').glob('*.gba'))
    if not roms:
        parser.error('provide a local GBA ROM, or put one in roms/gba/')
    import json
    for rom in roms:
        print(json.dumps(check(rom), sort_keys=True))
    print('GBA reference: cold boots, native keypad, RAM bounds/byte order, cartridge saves and per-frame replay passed')


if __name__ == '__main__':
    main()
