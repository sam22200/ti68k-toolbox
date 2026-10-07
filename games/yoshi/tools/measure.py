#!/usr/bin/env python3
"""Measure first PAL movement/jump timelines, preserving raw observations."""
import json
import sys

from reference import ROOT, OUT, ROM, X, Y, VX, VY
sys.path.insert(0, str(ROOT / 'tools/snes'))
from snesrun import SNES, PAD

FIELDS = {'x': (X, 2, False), 'x_sub': (0x70008a, 2, False),
          'y': (Y, 2, False), 'y_sub': (0x70008e, 2, False),
          'vx': (VX, 2, True), 'vy': (VY, 2, True),
          'jump': (0x7000c0, 2, False), 'flutter': (0x7000d2, 1, False),
          'angle': (0x7000b6, 2, False),
          'game_frame': (0x7e0030, 2, False), 'mode': (0x7e0118, 1, False)}
CASES = {
    'idle': [(0, '')], 'right': [(0, 'RIGHT')], 'left': [(0, 'LEFT')],
    'brake': [(0, 'RIGHT'), (30, '')],
    'reverse': [(0, 'RIGHT'), (30, 'LEFT')],
    'jump_tap': [(0, 'B'), (1, '')],
    'jump_hold': [(0, 'B'), (35, '')],
    'flutter_hold': [(0, 'B')],
    'flutter_repress': [(0, 'B'), (25, ''), (35, 'B')],
}


def main():
    snes = SNES(ROM)
    try:
        result = {'source': json.loads((OUT / 'start.json').read_text()),
                  'sampling': 'frame -1 before input; frame 0 after first update', 'cases': {}}
        for name, events in CASES.items():
            snes.load(OUT / 'start.state')
            snes.pressed = 0
            # Preserve the loader's natural spawn and camera. Teleporting far
            # ahead gets clamped by the original scrolling logic and corrupts
            # a supposed flat-ground measurement.
            keys = {f: sum(1 << PAD[k] for k in key.split()) for f, key in events}
            def sample(frame):
                return {'frame': frame, **{field: snes.read(*spec)
                        for field, spec in FIELDS.items()}}
            samples = [sample(-1)]
            for frame in range(90):
                if frame in keys:
                    snes.pressed = keys[frame]
                snes.lib.retro_run()
                samples.append(sample(frame))
            result['cases'][name] = {'input': events, 'samples': samples}
            print(name, 'X', samples[-1]['x'], 'min Y', min(s['y'] for s in samples),
                  'VX range', min(s['vx'] for s in samples), max(s['vx'] for s in samples),
                  'flutter states', sorted(set(s['flutter'] for s in samples)))
        (OUT / 'physics.json').write_text(json.dumps(result, indent=2) + '\n')
    finally:
        snes.close()


if __name__ == '__main__':
    main()
