#!/usr/bin/env python3
"""Controlled PAL movement probes: anchor X on the natural flat spawn tile.

The X integer is reset before each frame, not its fraction or velocity. This
isolates flat-ground/air mechanics from slopes, scrolling and streamed actors.
Each post-frame X still measures the original horizontal integration.
"""
import json
import sys
from reference import ROOT, OUT, ROM, X
from measure import FIELDS
sys.path.insert(0, str(ROOT / 'tools/snes'))
from snesrun import SNES, PAD

FIELDS = {**FIELDS, 'flutter': (0x7000d2, 2, False),
          'motion_vx': (0x7000a8, 2, True),
          'phase_timer': (0x7001d4, 2, False),
          'cooldown': (0x7001e6, 2, False),
          'skid': (0x7001d0, 2, False),
          'adjustment': (0x701e3a, 2, True)}
CASES = {
    'idle': [(0, '')],
    'right': [(0, 'RIGHT')], 'left': [(0, 'LEFT')],
    'brake': [(0, 'RIGHT'), (60, '')],
    'reverse': [(0, 'RIGHT'), (60, 'LEFT')],
    'jump_tap': [(0, 'B'), (1, '')],
    'jump_hold': [(0, 'B'), (35, '')],
    'flutter_hold': [(0, 'B')],
    'flutter_release': [(0, 'B'), (60, '')],
    'flutter_repress': [(0, 'B'), (25, ''), (35, 'B')],
    'flutter_restart': [(0, 'B'), (60, ''), (72, 'B')],
    'flutter_early_restart': [(0, 'B'), (60, ''), (63, 'B')],
    'flutter_rearm': [(0, 'B'), (88, ''), (93, 'B')],
    'flutter_fast_rearm': [(0, 'B'), (83, ''), (89, 'B')],
    'air_coast': [(0, 'B RIGHT'), (34, 'B')],
    'air_right': [(0, 'B'), (1, 'B RIGHT')],
    'air_brake': [(0, 'B'), (1, 'B RIGHT'), (60, 'B')],
    'air_reverse': [(0, 'B'), (1, 'B RIGHT'), (60, 'B LEFT')],
    'running_jump': [(0, 'RIGHT'), (60, 'RIGHT B')],
    'mixed_controls': [(0, 'RIGHT'), (12, 'RIGHT B'), (20, 'LEFT B'),
                       (26, 'LEFT'), (38, 'B'), (44, 'RIGHT B'), (62, ''),
                       (75, 'B'), (90, ''), (101, 'LEFT B'), (123, 'B'),
                       (160, ''), (190, 'RIGHT'), (206, 'RIGHT B'), (208, 'RIGHT')],
}
FRAMES = 240


def main():
    snes = SNES(ROM)
    try:
        result = {'source': json.loads((OUT / 'start.json').read_text()),
                  'intervention': 'Before every frame write X=119 at $70008C only; '
                  'fraction, velocity, Y, camera, collision and actors stay live.',
                  'sampling': 'frame -1 before input; frame 0 after first update',
                  'cases': {}}
        for name, events in CASES.items():
            snes.load(OUT / 'start.state')
            snes.pressed = 0
            keys = {f: sum(1 << PAD[k] for k in key.split()) for f, key in events}
            def sample(frame):
                return {'frame': frame, **{field: snes.read(*spec)
                        for field, spec in FIELDS.items()}}
            samples = [sample(-1)]
            for frame in range(FRAMES):
                snes.write(X, 119, 2)
                if frame in keys:
                    snes.pressed = keys[frame]
                snes.lib.retro_run()
                samples.append(sample(frame))
            assert all(s['mode'] == 0x13 and s['angle'] == 0 for s in samples), name
            result['cases'][name] = {'input': events, 'samples': samples}
            print(name, 'min Y', min(s['y'] for s in samples),
                  'VX', samples[-1]['vx'], 'flutter',
                  sorted(set(s['flutter'] for s in samples)))
        (OUT / 'movement.json').write_text(json.dumps(result, indent=2) + '\n')
    finally:
        snes.close()


if __name__ == '__main__':
    main()
