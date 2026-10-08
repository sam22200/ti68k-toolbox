#!/usr/bin/env python3
"""Real-core integration: boot, native inputs, CPU bus/RAM and exact state replay.

This requires the user's local BIOS. A no-input boot tests the reference
instrument; it does not claim a gameplay door or measured player mechanics.
"""
import argparse
import json
from pathlib import Path
import tempfile

from neogeorun import DEFAULT_BIOS, DEFAULT_CORE, DEFAULT_ROM, NeoGeo, PAD, sha, write_file


def frame_fingerprint(neo):
    return {
        'ram': sha(neo.dump()),
        **{name: sha(data) for name, data in neo.video().items()},
        'nvram': sha(neo.region(4, 65536)),
        'card': sha(neo.region(5, 131072)),
        'z80': sha(neo.region(7, 2048)),
        'rgb': sha(neo.image().tobytes()),
        'status': neo.status(),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bios', type=Path, default=DEFAULT_BIOS)
    parser.add_argument('--rom', type=Path, default=DEFAULT_ROM)
    parser.add_argument('--core', type=Path, default=DEFAULT_CORE)
    parser.add_argument('--boot-frames', type=int, default=2400)
    parser.add_argument('--keys', type=Path, help='optional documented cold-boot script')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    if args.boot_frames < 1:
        parser.error('--boot-frames must be positive')
    from neogeorun import script
    keys = script(args.keys)
    if any(frame >= args.boot_frames for frame in keys):
        parser.error('boot script events exceed --boot-frames')
    try:
        neo = NeoGeo(args.rom, args.bios, args.core)
    except (OSError, ValueError) as error:
        parser.exit(1, f'Reference check cannot start: {error}\n')
    try:
        for frame in range(args.boot_frames):
            if frame in keys:
                neo.pressed[:] = keys[frame]
            neo.step()
        assert neo.fps > 0 and neo.frame is not None
        assert any(neo.video()['vram']), 'expected video memory to be populated'
        for port in (0, 1):
            descriptors = {d['description']: d['id'] for d in neo.descriptors
                           if d['port'] == port and d['device'] == 1}
            for button, description in (('A', 'Button A'), ('B', 'Button B'),
                                        ('C', 'Button C'), ('D', 'Button D'),
                                        ('COIN', 'Coin'), ('START', 'Start')):
                assert descriptors.get(description) == PAD[button], 'native input descriptor mapping'
        loaded_crc = neo.status()['bios_crc32']
        matching = [chip for chip in neo.bios_manifest if chip['crc32'] == loaded_crc]
        assert matching, 'actual selected BIOS must belong to the supplied archive'
        assert neo.options[b'fbneo-cpu-speed-adjust'] == b'100%'
        assert neo.options[b'fbneo-fixed-frameskip'] == b'0'
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory) / 'boot.state'
            saved_ram = neo.dump()
            neo.save(state)

            def replay(restore=True):
                if restore:
                    neo.load(state)
                    assert neo.pressed == [0, 0] and neo.frame is None
                frames = []
                for frame in range(120):
                    neo.pressed[:] = (
                        [(1 << PAD['RIGHT']) | (1 << PAD['A']),
                         (1 << PAD['LEFT']) | (1 << PAD['B'])] if frame < 30 else
                        [1 << PAD['COIN'], 1 << PAD['START']] if frame < 32 else [0, 0])
                    neo.step()
                    frames.append(frame_fingerprint(neo))
                    inputs = neo.region(6, 32)
                    if frame < 30:
                        assert inputs[0] & 0x18 == 0x18, 'P1 right/A mapping'
                        assert inputs[1] & 0x24 == 0x24, 'P2 left/B mapping'
                    elif frame < 32:
                        assert inputs[3] & 1, 'P1 coin mapping'
                        assert inputs[2] & 4, 'P2 start mapping'
                return frames

            first, second = replay(False), replay()
            assert first == second, 'every frame must reproduce RAM/palettes/video/status'
            # Compare normalized writes against the actual emulated CPU bus.
            neo.load(state)
            for address, size, value in ((0x10fffc, 4, 0x12345678),
                                         (0x10fffd, 2, 0xabcd), (0x10ffff, 1, 0x81)):
                neo.write(address, value, size)
                assert neo.read(address, size) == value
                assert neo.lib.retro_ti_neogeo_bus_read(address, size) == value
            neo.load(state)
            assert neo.dump() == saved_ram, 'restore must undo all diagnostic writes'
        results = dict(metadata=neo.metadata(), boot_frames=args.boot_frames,
                       boot_keys=str(args.keys) if args.keys else None,
                       boot_keys_sha256=sha(args.keys.read_bytes()) if args.keys else None,
                       bios_matches=matching, replay_frames=120, replay=first,
                       gameplay_door_verified=False)
        if args.report:
            write_file(args.report, json.dumps(results, indent=2) + '\n')
        print(f'Neo Geo: {args.boot_frames} boot frames at {neo.fps:.6f} Hz, '
              'two-controller inputs, CPU bus/RAM pokes and 120 exact replay frames passed')
        print('This checks the reference instrument; the Windjammers serve door is a separate probe.')
    finally:
        neo.close()
    try:
        NeoGeo(args.rom, args.bios, args.core)
    except ValueError as error:
        assert 'fresh process' in str(error), 'reinitialization must fail with the actionable guard'
    else:
        raise AssertionError('unsafe FBNeo reinitialization was not rejected')


if __name__ == '__main__':
    main()
