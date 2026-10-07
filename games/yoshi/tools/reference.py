#!/usr/bin/env python3
"""Regenerate the local PAL level 1-1 door, with a documented tutorial poke."""
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tools/snes'))
from snesrun import SNES, script, DEFAULT_CORE

ROM = ROOT / "roms/snes/Super Mario World 2 - Yoshi's Island (Europe) (En,Fr,De).sfc"
ROM_HASH = '91a4dc481c54b620cb3bccaffe5fa3f69db955ae600309414d18bb59307cba90'
OUT = ROOT / 'sources/yoshi_snes'
X, Y, VX, VY = 0x70008c, 0x700090, 0x7000b4, 0x7000aa
MODE, STAGE = 0x7e0118, 0x7e021a


def run_keys(snes, path, frames):
    keys = script(path)
    snes.pressed = 0
    for frame in range(frames):
        if frame in keys:
            snes.pressed = keys[frame]
        snes.lib.retro_run()


def boot(snes):
    boot_keys = ROOT / 'games/yoshi/keys/ref_boot.txt'
    enter_keys = ROOT / 'games/yoshi/keys/ref_enter.txt'
    run_keys(snes, boot_keys, 9900)
    assert snes.read(MODE, 2) == 0x13 and snes.read(STAGE) == 0x0b
    assert (snes.read(X, 2), snes.read(Y, 2)) == (135, 1888)
    # Only the opening tutorial is bypassed. The original exit handler,
    # overworld selection and level loader initialize level 1-1 normally.
    snes.write(X, 1000, 2)
    run_keys(snes, enter_keys, 620)
    assert snes.read(MODE, 2) == 0x13 and snes.read(STAGE) == 0
    assert snes.read(Y, 2) == 1904 and snes.read(VY, 2, True) == 0
    return {'boot_frames': 9900, 'enter_frames': 620,
            'boot_keys_sha256': hashlib.sha256(boot_keys.read_bytes()).hexdigest(),
            'enter_keys_sha256': hashlib.sha256(enter_keys.read_bytes()).hexdigest(),
            'tutorial_poke': {'address': '70008c', 'size': 2, 'value': 1000}}


def main():
    assert hashlib.sha256(ROM.read_bytes()).hexdigest() == ROM_HASH
    OUT.mkdir(parents=True, exist_ok=True)
    snes = SNES(ROM)
    try:
        provenance = boot(snes)
        snes.save(OUT / 'start.state')
        snes.image().save(OUT / 'start.png')
        (OUT / 'start.wram').write_bytes(snes.dump())
        for name, data in zip(('vram', 'cgram', 'oam', 'regs', 'sram'),
                              (*snes.ppu(), snes.sram())):
            (OUT / ('start.' + name)).write_bytes(data)
        (OUT / 'start.json').write_text(json.dumps({**snes.identity, **provenance,
            'rom_sha256': ROM_HASH,
            'core_sha256': hashlib.sha256(DEFAULT_CORE.read_bytes()).hexdigest(),
            'options': {k.decode(): v.decode() for k, v in snes.options.items()},
            'spawn': {'x': snes.read(X, 2), 'y': snes.read(Y, 2)},
            'video': list(snes.frame[1:3]),
            'core_revision': 'fae2fea08f74180759ef540ee94259213f503480'}, indent=2) + '\n')
        print('Level 1-1 reference door:', snes.read(X, 2), snes.read(Y, 2))
    finally:
        snes.close()


if __name__ == '__main__':
    main()
