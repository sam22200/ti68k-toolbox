#!/usr/bin/env python3
"""Confirm decoded geometry with original stationary drops and tile pokes."""
import json
import sys
from reference import ROOT, OUT, ROM, X, Y, VX, VY
from terrain import load_map, surfaces, tile, TOP
sys.path.insert(0, str(ROOT / 'tools/snes'))
from snesrun import SNES, PAD


def suppress(snes):
    for slot in range(24):
        snes.write(0x700f00 + (slot << 2), 0, 2)


def capture_doors(snes, directory):
    snes.load(OUT / 'start.state')
    snes.save(directory / 'door0.state')
    doors = [{'x': 119, 'camera_x': 0, 'state': 'door0.state'}]
    for frame in range(1000):
        suppress(snes)
        snes.pressed = (1 << PAD['RIGHT']) | ((1 << PAD['B']) if frame % 100 != 99 else 0)
        if snes.read(0x7e0d0f) == 9 and frame % 10 == 0:
            snes.pressed |= 1 << PAD['A']
        snes.lib.retro_run()
        camera = snes.read(0x7e0039, 2)
        if len(doors) < 5 and camera >= len(doors) * 256:
            name = f'door{len(doors)}.state'
            snes.save(directory / name)
            doors.append({'x': snes.read(X, 2), 'camera_x': camera, 'state': name})
        if snes.read(X, 2) >= 1280:
            break
    assert len(doors) == 5
    return doors


def drop(snes, state, x, y, directory, name, vy=0, held=False):
    snes.load(state)
    for address, value in ((X, x), (0x70008a, 0), (Y, y), (0x70008e, 0),
                           (VX, 0), (0x7000a8, 0), (VY, vy), (0x7000c0, 6),
                           (0x7000d2, 1), (0x7001d4, 0), (0x7001e6, 0), (0x7001dc, 0)):
        snes.write(address, value, 2)
    snes.pressed = 0
    samples = []
    for frame in range(90):
        snes.pressed = (1 << PAD['B']) if held and frame < 35 else 0
        suppress(snes)
        snes.write(X, x, 2)
        snes.lib.retro_run()
        s = {'frame': frame, 'x': snes.read(X, 2), 'y': snes.read(Y, 2),
             'sub': snes.read(0x70008e, 2), 'vy': snes.read(VY, 2, True),
             'jump': snes.read(0x7000c0, 2), 'angle': snes.read(0x7000b6, 2),
             'head_timer': snes.read(0x7001dc, 2),
             'flutter': snes.read(0x7000d2, 2), 'phase_timer': snes.read(0x7001d4, 2),
             'cooldown': snes.read(0x7001e6, 2),
             'mode': snes.read(0x7e0118, 2)}
        samples.append(s)
        if s['jump'] == 0 and frame > 0:
            snes.image().save(directory / (name + '.png'))
            break
    return {'x': x, 'start_y': y, 'samples': samples}


def main():
    directory = OUT / 'terrain'
    directory.mkdir(exist_ok=True)
    rom, numbers = load_map()
    snes = SNES(ROM)
    try:
        doors = capture_doors(snes, directory)
        tests = []
        # Within each restored camera's streamed region; includes all slope
        # kinds in this band, flat solids, and two one-way ledges.
        for x in (119, 32, 47, 160, 175, 195, 208, 240, 420, 480,
                  604, 632, 744, 790, 840, 860, 895, 925, 950, 980,
                  1008, 1040, 1118, 1152, 1184, 1240, 1264):
            door = max((d for d in doors if d['camera_x'] <= x), key=lambda d: d['camera_x'])
            # Ground route beneath the optional high platforms; separate
            # positions 744/1152 verify the one-way platforms from above.
            lower = 0 if x in (744, 1152) else 1800
            ground = min(s[0] for dx in (3, 8, 13)
                         for s in surfaces(rom, numbers, x + dx) if s[0] >= lower)
            case = drop(snes, directory / door['state'], x, ground - 32 - 40,
                        directory, f'drop{x}')
            case['surfaces'] = {str(dx): surfaces(rom, numbers, x + dx)[:3] for dx in (3, 8, 13)}
            tests.append(case)
            last = case['samples'][-1]
            print(x, 'prediction', ground - 32, 'land', last, flush=True)
        (directory / 'drops.json').write_text(json.dumps({'source': json.loads((OUT / 'start.json').read_text()),
            'intervention': 'Restore surveyed camera; inject drop position/zero velocities/jump6; '
            'suppress 24 actor state words and anchor X before each frame.',
            'doors': doors, 'cases': tests}, indent=2) + '\n')
        interactions = {}
        for name, x, y in (('one_way', 744, 1708), ('ceiling', 1008, 1672)):
            door = max((d for d in doors if d['camera_x'] <= x), key=lambda d: d['camera_x'])
            interactions[name] = drop(snes, directory / door['state'], x, y,
                                       directory, name, vy=-1328, held=True)
            samples = interactions[name]['samples']
            print(name, 'frames', len(samples), 'min Y', min(s['y'] for s in samples),
                  'head timer', max(s['head_timer'] for s in samples), 'last', samples[-1], flush=True)
        (directory / 'interactions.json').write_text(json.dumps(interactions, indent=2) + '\n')
    finally:
        snes.close()


if __name__ == '__main__':
    main()
