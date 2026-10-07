#!/usr/bin/env python3
"""Compare original interaction fields and complete native PC/TI actor states."""
import json
import re
import sys
from reference import OUT
from check_terrain import GAME, command, run, values

WORDS = 30  # seven movement words, three action words, four words per actor


def dense():
    keys = GAME / 'x/dense.txt'
    keys.write_text('0 RIGHT\n')
    packets = check(keys, 80, 53)
    assert {p[5] >> 16 & 15 for p in packets} == set(range(16))
    previous, frames = [0, 0], []
    for n in range(1, 81):
        output = run('yjprobec.89z', keys, n, 53)
        totals = [int(re.search(r'^\d+\s+' + z + r'\s+\d+\s+(\d+)', output, re.MULTILINE).group(1))
                  for z in ('update', 'render')]
        delta = [a - b for a, b in zip(totals, previous)]
        frames.append({'frame': n - 1, 'update': delta[0], 'render': delta[1], 'total': sum(delta)})
        previous = totals
    peak = max(frames, key=lambda f: f['total'])
    (GAME / 'x/actors_dense_cycles.json').write_text(json.dumps({'peak': peak, 'frames': frames}, indent=2) + '\n')
    print('Dense scene: 80 complete states PC=TI, all16 camera shifts; peak', peak, flush=True)
    assert peak['total'] < 210000


def check(keys, frames, scenario):
    ti = values(run('yjactorh.89z', keys, frames, scenario))
    pc = values(command([str(GAME / 'yjprobe_test'), '--actortrace', str(keys),
                         str(frames), str(scenario)]))
    assert len(ti) == WORDS * frames + 1 and ti[-1] == frames
    assert ti == pc, f'actor scenario {scenario}: native PC/TI state mismatch'
    return [ti[f * WORDS:(f + 1) * WORDS] for f in range(frames)]


def main():
    original = json.loads((OUT / 'actors/reference.json').read_text())
    count = 0
    for case in original['cases']:
        packets = check(OUT / f'actors/{case["name"]}.txt', len(case['samples']), case['scenario'])
        for frame, (packet, expected) in enumerate(zip(packets, case['samples'])):
            action, held, flags = packet[7:10]
            got = {'mouth': action >> 24, 'length': (action >> 16) & 255,
                   'timer': (action >> 8) & 255, 'up': action & 255,
                   'swallow': held >> 16, 'slot': (held >> 8) & 255,
                   'holding': held & 255, 'eggs': flags & 255}
            assert got == {k: expected[k] for k in got}, (case['name'], frame, got, expected)
            if case['name'] == 'walk':
                actor = {'x': packet[10] >> 16, 'y': packet[10] & 65535,
                         'vx': ((packet[11] >> 16) ^ 32768) - 32768,
                         'sub': (packet[11] >> 8) & 255, 'actor_state': packet[11] & 255}
                assert actor == {k: expected[k] for k in actor}, ('walk', frame, actor, expected)
        count += len(packets)
    print(f'Original PAL: {count} complete interaction states PC=TI=ROM; 90 flat Shy Guy motion states exact', flush=True)
    script = GAME / 'keys/actors.txt'
    packets = check(script, 1300, 0)
    finish = next(f for f, p in enumerate(packets) if p[6] & 0x10000)
    assert packets[finish][0] >> 16 == 1264 and packets[finish][9] & 255 == 2
    assert all(p == packets[finish] for p in packets[finish:])
    print(f'Native actor traversal: 1300 complete PC/TI states; clear update {finish}, two eggs, frozen finish', flush=True)
    dense()
    if '--states-only' in sys.argv:
        return
    previous, frames = [0, 0], []
    for n in range(1, 1301):
        output = run('yjprobec.89z', script, n, 0)
        totals = []
        for zone in ('update', 'render'):
            match = re.search(r'^\d+\s+' + zone + r'\s+\d+\s+(\d+)', output, re.MULTILINE)
            assert match, output
            totals.append(int(match.group(1)))
        delta = [a - b for a, b in zip(totals, previous)]
        frames.append({'frame': n - 1, 'update': delta[0], 'render': delta[1], 'total': sum(delta)})
        previous = totals
        if n % 100 == 0: print('Profiled actor frames:', n, flush=True)
    peak = max(frames, key=lambda f: f['total'])
    stats = {'frames': frames, 'peak': peak, 'average': sum(f['total'] for f in frames) // len(frames),
             'finish': finish, 'original_states': count,
             'scope': 'native tongue/ingestion/actor traversal; ROM scenery and animated sprites; hardware grayscale excluded'}
    (GAME / 'x/actors_cycles.json').write_text(json.dumps(stats, indent=2) + '\n')
    print('Actors cycles: mean', stats['average'], 'peak', peak, flush=True)
    assert peak['total'] < 210000, 'actor traversal exceeds provisional 51.2 Hz rendering budget'


if __name__ == '__main__':
    if sys.argv[1:] == ['--dense-only']:
        dense()
    else:
        main()
