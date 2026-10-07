#!/usr/bin/env python3
"""Check real TI state against controlled PAL traces and profile probe frames."""
import json
import re
import subprocess
import sys
from reference import ROOT, OUT

GAME = ROOT / 'games/yoshi'


def run(binary, keys, frames, scenario=10):
    return subprocess.run([str(ROOT / 'tools/bin/ti-cycles'), '--frames', str(frames),
                           '--arg', str(scenario), '--keys', str(keys), str(binary)], cwd=GAME,
                          text=True, capture_output=True, check=True).stdout


def main():
    directory = GAME / 'x'
    directory.mkdir(exist_ok=True)
    original = json.loads((OUT / 'movement.json').read_text())
    total = 0
    scripts = {}
    for name, case in original['cases'].items():
        keys = directory / (name + '.txt')
        keys.write_text(''.join(str(frame) + (' ' + buttons.replace('B', 'A')
                         if buttons else '') + '\n' for frame, buttons in case['input']))
        scripts[name] = keys
        count = len(case['samples']) - 1
        result = run(GAME / 'yjtrace.89z', keys, count)
        values = [int(n) & 0xffffffff for n in re.findall(r'^value: (-?\d+)', result, re.MULTILINE)]
        expected = []
        for s in case['samples'][1:]:
            expected += [(s['x'] << 16) | s['x_sub'],
                         ((s['vx'] & 65535) << 16) | (s['vy'] & 65535),
                         (s['y'] << 16) | s['y_sub'],
                         (s['flutter'] << 24) | (s['phase_timer'] << 16) |
                         (s['cooldown'] << 8) | s['jump'],
                         (s['skid'] << 8) | int(s['jump'] == 0)]
        assert values == expected + [count], f'{name}: TI state differs from original'
        total += count
        print(name + f': all {count} complete TI movement states match original PAL')
    print(f'TI: {total} original PAL states matched')
    if '--states-only' in sys.argv:
        return
    frames = []
    for name in ('flutter_hold', 'air_reverse', 'mixed_controls'):
        previous = [0, 0]
        for count in range(1, 241):
            result = run(GAME / 'yjprobec.89z', scripts[name], count)
            totals = []
            for zone in ('update', 'render'):
                match = re.search(r'^\d+\s+' + zone + r'\s+\d+\s+(\d+)', result, re.MULTILINE)
                if not match:
                    raise ValueError('missing measured zone: ' + result)
                totals.append(int(match.group(1)))
            frame = [a - b for a, b in zip(totals, previous)]
            frames.append({'case': name, 'frame': count - 1, 'update': frame[0],
                           'render': frame[1], 'total': sum(frame)})
            previous = totals
        print(name + ': 240 per-frame cycle measurements completed', flush=True)
    peak = max(frames, key=lambda frame: frame['total'])
    result = {'frames': frames, 'peak': peak,
              'average': sum(frame['total'] for frame in frames) // len(frames),
              'scope': 'controlled movement diagnostic only; not five-screen terrain/game costs'}
    (directory / 'cycles.json').write_text(json.dumps(result, indent=2) + '\n')
    print('Probe average', result['average'], 'peak', peak)
    # A 51.2 Hz probe has ~234k total cycles/frame before grayscale overhead.
    assert peak['total'] < 210000, 'probe exceeds its provisional rendering budget'


if __name__ == '__main__':
    main()
