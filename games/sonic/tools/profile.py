#!/usr/bin/env python3
"""Measure each TI frame by differences between ti-cycles prefix totals.

This uses the real compiled 68000 binary, not host timings or TiEmu timings.
No instrumentation is added to game hot paths. Output remains local in x/.
"""
import json
from pathlib import Path
import re
import subprocess

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]


def main():
    result = {}
    for name, scenario, count in [('play', 0, 250), ('damage', 10, 90), ('burst', 12, 90)]:
        previous = [0, 0]
        frames = []
        for n in range(1, count + 1):
            run = subprocess.run([str(ROOT / 'tools/bin/ti-cycles'), '--arg', str(scenario),
                                  '--file', 'sonterr.89y', '--file', 'sonart.89y', '--keys', f'keys/{name}.txt',
                                  '--frames', str(n), 'sonicc.89z'], cwd=GAME,
                                 text=True, capture_output=True, check=True)
            totals = []
            for zone in ('update', 'render'):
                match = re.search(r'^\d+\s+' + zone + r'\s+\d+\s+(\d+)\s+\d+',
                                  run.stdout, re.MULTILINE)
                if not match:
                    raise RuntimeError(f'missing {zone} cycle total: {run.stdout}')
                totals.append(int(match.group(1)))
            frame = [a - b for a, b in zip(totals, previous)]
            frames.append({'frame': n - 1, 'update': frame[0], 'render': frame[1],
                           'total': sum(frame)})
            previous = totals
        result[name] = {'scenario': scenario, 'frames': frames,
                        'average': sum(r['total'] for r in frames) // count,
                        'peak': max(frames, key=lambda r: r['total'])}
        print(name, 'average', result[name]['average'], 'peak', result[name]['peak'], flush=True)
    (GAME / 'x/cycles.json').write_text(json.dumps(result, indent=2) + '\n')
    if any(run['peak']['total'] > 360000 for run in result.values()):
        raise RuntimeError('a measured frame exceeds the 360k grayscale game budget')


if __name__ == '__main__':
    main()
