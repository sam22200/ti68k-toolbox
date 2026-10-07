#!/usr/bin/env python3
"""Original collision traces, native PC/TI state equality and per-frame costs."""
import json
import re
import subprocess
import sys
from reference import ROOT, OUT

GAME = ROOT / 'games/yoshi'


def command(args):
    return subprocess.run(args, cwd=GAME, capture_output=True, text=True, check=True).stdout


def run(binary, keys, frames, scenario):
    return command([str(ROOT / 'tools/bin/ti-cycles'), '--arg', str(scenario),
                    '--file', str(GAME / 'yjterr.89y'), '--file', str(GAME / 'yjart.89y'), '--keys', str(keys),
                    '--frames', str(frames), str(GAME / binary)])


def values(text):
    return [int(v) & 0xffffffff for v in re.findall(r'^value: (-?\d+)', text, re.MULTILINE)]


def original_fields(packet):
    return {'y': packet[2] >> 16, 'sub': packet[2] & 65535,
            'vy': ((packet[1] & 65535) ^ 32768) - 32768,
            'jump': packet[3] & 255, 'flutter': packet[3] >> 24,
            'phase_timer': (packet[3] >> 16) & 255, 'cooldown': (packet[3] >> 8) & 255,
            'angle': packet[6] >> 24, 'head_timer': (packet[6] >> 8) & 255}


def check(keys, frames, scenario, original=None):
    ti = values(run('yjterrh.89z', keys, frames, scenario))
    pc = values(command([str(GAME / 'yjprobe_test'), '--trace', str(keys),
                         str(frames), str(scenario)]))
    assert len(ti) == 7 * frames + 1 and ti[-1] == frames, (scenario, len(ti))
    assert ti == pc, f'scenario {scenario}: native PC/TI state mismatch'
    if original:
        for frame, expected in enumerate(original):
            got = original_fields(ti[frame * 7:frame * 7 + 7])
            assert got == {key: expected[key] for key in got}, (scenario, frame, got, expected)
    return ti


def main():
    directory = GAME / 'x'
    directory.mkdir(exist_ok=True)
    idle = directory / 'idle.txt'
    idle.write_text('0\n')
    jump = directory / 'interaction.txt'
    jump.write_text('0 A\n35\n')
    drops = json.loads((OUT / 'terrain/drops.json').read_text())['cases']
    count = 0
    for n, case in enumerate(drops):
        samples = case['samples']
        check(idle, len(samples), 20 + n, samples)
        count += len(samples)
    interactions = json.loads((OUT / 'terrain/interactions.json').read_text())
    for n, name in enumerate(('one_way', 'ceiling')):
        samples = interactions[name]['samples']
        check(jump, len(samples), 12 + n, samples)
        count += len(samples)
    print(f'Original PAL collision: {len(drops)} drops + two interactions, {count} full states match PC/TI', flush=True)
    script = GAME / 'keys/terrain.txt'
    trace = check(script, 600, 59)
    finish = next(frame for frame in range(600) if trace[frame * 7 + 6] & 0x10000)
    assert trace[finish * 7] >> 16 == 1264 and trace[finish * 7 + 4] & 1
    print(f'Native traversal: 600 complete states PC=TI, first finish frame {finish}', flush=True)
    if '--states-only' in sys.argv:
        return
    previous, frames = [0, 0], []
    for count in range(1, 601):
        output = run('yjprobec.89z', script, count, 59)
        totals = []
        for zone in ('update', 'render'):
            m = re.search(r'^\d+\s+' + zone + r'\s+\d+\s+(\d+)', output, re.MULTILINE)
            assert m, output
            totals.append(int(m.group(1)))
        delta = [a - b for a, b in zip(totals, previous)]
        frames.append({'frame': count - 1, 'update': delta[0], 'render': delta[1], 'total': sum(delta)})
        previous = totals
        if count % 100 == 0: print('Profiled native frames:', count, flush=True)
    peak = max(frames, key=lambda f: f['total'])
    stats = {'frames': frames, 'peak': peak, 'average': sum(f['total'] for f in frames) // 600,
             'finish': finish, 'scope': 'static collision terrain + diagnostic actor/camera; hardware grayscale excluded'}
    (directory / 'terrain_cycles.json').write_text(json.dumps(stats, indent=2) + '\n')
    print('Terrain cycles: mean', stats['average'], 'peak', peak, flush=True)
    assert peak['total'] < 210000, 'terrain prototype exceeds its provisional 51.2 Hz rendering budget'


if __name__ == '__main__':
    main()
