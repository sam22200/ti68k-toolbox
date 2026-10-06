#!/usr/bin/env python3
"""Measure short Sonic REV00 actions on verified flat ground in the real ROM.

Run `make reference measure`; output stays under ignored sources/sonic1_md/.
This is reference preparation, not the target engine or its implementation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tools/md'))
from mdrun import MD, PAD  # noqa: E402

FIELDS = {'x': (0xffd008, 4, False), 'y': (0xffd00c, 4, False),
          'vx': (0xffd010, 2, True), 'vy': (0xffd012, 2, True),
          'ground_speed': (0xffd014, 2, True), 'status': (0xffd022, 1, False),
          'angle': (0xffd026, 1, False), 'radius_y': (0xffd016, 1, False)}


def sample(md):
    return {name: md.read(*spec) for name, spec in FIELDS.items()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'sources/sonic1_md/physics.json')
    args = parser.parse_args()
    rom = ROOT / 'roms/md/Sonic_1.md'
    start = ROOT / 'sources/sonic1_md/start.state'
    md = MD(rom)
    try:
        md.load(start)
        # Inject above known flat ground, then let the original resolver land.
        md.write(0xffd008, 192 << 16, 4)
        md.write(0xffd00c, 930 << 16, 4)
        for address in (0xffd010, 0xffd012, 0xffd014):
            md.write(address, 0, 2)
        md.write(0xffd022, 2)
        md.pressed = 0
        for _ in range(60):
            md.lib.retro_run()
        initial = sample(md)
        assert initial['angle'] == 0 and initial['status'] == 0
        assert initial['x'] >> 16 == 192 and initial['y'] >> 16 == 940
        assert initial['vx'] == initial['vy'] == initial['ground_speed'] == 0
        actions = {
            'accelerate_release': ({0: ['RIGHT'], 20: []}, 45),
            'reverse': ({0: ['RIGHT'], 20: ['LEFT'], 24: []}, 35),
            'jump_held': ({0: ['C'], 60: []}, 70),
            'jump_tap': ({0: ['C'], 1: []}, 60),
            'roll': ({0: ['RIGHT'], 20: ['DOWN'], 21: []}, 45),
        }
        result = {'provenance': {**md.identity,
                  'rom_sha256': hashlib.sha256(rom.read_bytes()).hexdigest(),
                  'start_state_sha256': hashlib.sha256(start.read_bytes()).hexdigest(),
                  'setup': 'x=192,y=930,velocities=0,status=2; 60 idle frames',
                  'units': 'x/y unsigned 16.16 original pixels; velocities signed 8.8 per original frame'},
                  'initial': initial, 'actions': {}}
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory) / 'flat.state'
            md.save(state)
            for name, (keys, frames) in actions.items():
                md.load(state)
                md.pressed = 0
                trace = []
                for frame in range(frames):
                    if frame in keys:
                        md.pressed = sum(1 << PAD[key] for key in keys[frame])
                    md.lib.retro_run()
                    trace.append({'frame': frame, **sample(md)})
                result['actions'][name] = {'keys': keys, 'trace': trace}
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, indent=2) + '\n')
        for name, action in result['actions'].items():
            trace = action['trace']
            print(name, 'first=', trace[0], 'last=', trace[-1])
            if name.startswith('jump'):
                airborne = [row for row in trace if row['status'] & 2]
                print('  apex_center_y=', min(row['y'] for row in airborne) / 65536,
                      'airborne_frames=', len(airborne))
        print('saved', args.output)
    finally:
        md.close()


if __name__ == '__main__':
    main()
