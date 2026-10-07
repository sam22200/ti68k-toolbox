#!/usr/bin/env python3
"""Integration: local PAL 1-1 boot, live input/pokes, full-memory/video replay."""
import hashlib
from pathlib import Path
import sys
import tempfile

from snesrun import SNES, PAD, ROOT
sys.path.insert(0, str(ROOT / 'games/yoshi/tools'))
from reference import ROM, ROM_HASH, X, Y, VX, VY, MODE, STAGE, boot


def main():
    assert hashlib.sha256(ROM.read_bytes()).hexdigest() == ROM_HASH
    snes = SNES(ROM)
    try:
        boot(snes)
        assert 49.9 < snes.fps < 50.1, 'expected the local PAL revision'
        assert snes.frame[1:3] == (256, 224)
        assert [len(p) for p in snes.ppu()] == [65536, 512, 544, 64]
        assert len(snes.sram()) == 32768 and any(snes.ppu()[0])
        assert snes.options[b'snes9x_overclock_superfx'] == b'100%'
        assert snes.options[b'snes9x_overclock_cycles'] == b'disabled'
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory) / 'start.state'
            snes.save(state)
            x0, y0 = snes.read(X, 2), snes.read(Y, 2)

            def replay(pad, frames):
                snes.load(state)
                snes.pressed = pad
                samples = []
                for _ in range(frames):
                    snes.lib.retro_run()
                    samples.append(tuple(hashlib.sha256(data).digest() for data in
                        (snes.dump(), snes.sram(), *snes.ppu(), snes.image().tobytes())))
                return samples

            first = replay(1 << PAD['RIGHT'], 20)
            assert snes.read(X, 2) > x0 and snes.read(VX, 2, True) > 0
            assert first == replay(1 << PAD['RIGHT'], 20), 'every frame must match WRAM/SRAM/PPU/video'
            replay(1 << PAD['B'], 10)
            assert snes.read(Y, 2) < y0 and snes.read(VY, 2, True) < 0
            snes.load(state)
            snes.write(X, 160, 2)
            snes.pressed = 0
            snes.lib.retro_run()
            assert snes.read(X, 2) == 160, 'game must consume the candidate player X'
            assert snes.sram()[0x8c:0x8e] == b'\xa0\x00', 'cartridge words are little-endian'
            assert snes.read(MODE, 2) == 0x13 and snes.read(STAGE) == 0
            for address, size in ((0x7dffff, 1), (0x7fffff, 2), (0x707fff, 2)):
                try:
                    snes.read(address, size)
                except ValueError:
                    pass
                else:
                    raise AssertionError('out-of-range read accepted')
        print('Yoshi PAL 1-1: boot, movement, jump, live cartridge-RAM poke, PPU exports and per-frame deterministic replay passed')
    finally:
        snes.close()


if __name__ == '__main__':
    main()
