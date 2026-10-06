#!/usr/bin/env python3
"""Integration check: real Sonic REV00 boot, input, state replay and RAM pokes."""
import hashlib
from pathlib import Path
import tempfile

from mdrun import MD, PAD, ROOT, script

ROM_HASH = '46160baa06362c711c9f1a5017cb7371026444936c8af5e93a78996cf32ff2a6'


def main():
    rom = ROOT / 'roms/md/Sonic_1.md'
    assert hashlib.sha256(rom.read_bytes()).hexdigest() == ROM_HASH, 'expected local Sonic REV00'
    md = MD(rom)
    try:
        keys = script(ROOT / 'games/sonic/keys/ref_boot.txt')
        for frame in range(1000):
            if frame in keys:
                md.pressed = keys[frame]
            md.lib.retro_run()
        assert md.read(0xfff600) == 0x0c, 'boot must reach normal level mode'
        assert (md.read(0xffd008, 2), md.read(0xffd00c, 2)) == (80, 944), 'expected GHZ1 spawn'
        assert md.read(0xffd000) == 1, 'player must be Sonic'
        vram, cram, regs = md.vdp()
        assert (len(vram), len(cram), len(regs)) == (65536, 128, 32)
        assert regs[5] == 0x7c, 'expected GHZ sprite attribute table'
        assert vram != bytes(65536), 'VDP snapshot must contain loaded tiles'
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory) / 'start.state'
            md.save(state)

            def replay(pad, count):
                md.load(state)
                md.pressed = pad
                result = []
                for _ in range(count):
                    md.lib.retro_run()
                    result.append((hashlib.sha256(md.dump()).digest(),
                                   hashlib.sha256(md.image().tobytes()).digest()))
                return result

            first = replay(1 << PAD['RIGHT'], 20)
            assert md.read(0xffd008, 2) > 80, 'right input must move the real player'
            assert first == replay(1 << PAD['RIGHT'], 20), 'state replay must match RAM and video'
            replay(1 << PAD['C'], 10)
            assert md.read(0xffd012, 2, signed=True) < 0, 'jump must have negative Y velocity'
            assert md.read(0xffd00c, 2) < 944, 'jump must rise above the spawn'
            assert md.read(0xffd022) & 2, 'jump must set the airborne bit'
            md.load(state)
            # A poke that the running game visibly consumes, rather than a scratch byte.
            md.write(0xffd008, 120, 2)
            md.pressed = 0
            md.lib.retro_run()
            assert md.read(0xffd008, 2) == 120, 'player X poke must survive an idle update'
            dump = md.dump()
            assert int.from_bytes(dump[0xd008:0xd00a], 'big') == 120, 'dump must use CPU byte order'
        print('Sonic reference: boot, movement, jump, RAM poke, VDP snapshots and deterministic state replay passed')
    finally:
        md.close()


if __name__ == '__main__':
    main()
