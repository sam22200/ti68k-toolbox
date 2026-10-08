#!/usr/bin/env python3
"""Replay no-write original dash inputs; export bounded motion fixtures."""
import json
from pathlib import Path
import sys
GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
sys.path.insert(0, str(ROOT / 'tools/neogeo'))
from check_gameplay import DIRECTIONS, mask, values
from check_reference import frame_fingerprint
from neogeorun import NeoGeo, sha
from measure_actions import PROGRAM_SHA

def main():
    n = NeoGeo()
    trials, refs, starts = [], [], []
    try:
        metadata = n.metadata()
        assert sha((ROOT / 'sources/windjammers_neogeo/program.be.bin').read_bytes()) == PROGRAM_SHA
        for port, name in ((0, 'serve'), (1, 'free')):
            door = ROOT / f'sources/windjammers_neogeo/gameplay/{name}.state'
            for direction in DIRECTIONS:
                for held in (1, 2, 20):
                    def run():
                        n.load(door); n.pressed[:] = [0, 0]
                        # Warm frontend surface before the measured input door.
                        n.step(); n.step()
                        initial = values(n); rows, hashes = [], []
                        for frame in range(36):
                            n.pressed[:] = [0, 0]
                            n.pressed[port] = mask(direction + ' A') if 4 <= frame < 4 + held else 0
                            n.step(); rows.append(values(n)); hashes.append(frame_fingerprint(n))
                        return dict(initial=initial, rows=rows, hashes=hashes)
                    first = run()
                    assert first == run(), (port, direction, held)
                    ident = len(starts)
                    first.update(port=port, direction=direction, held=held)
                    trials.append(first)
                    initial = first['initial']
                    starts.append((port, mask(direction), held, initial[f'p{port+1}_x'], initial[f'p{port+1}_y']))
                    # Compare every integrated dash position/velocity. The ROM's
                    # horizontal wall animation can extend Yoo's action; native
                    # wall recovery uses the ordinary dash duration instead.
                    for frame, row in enumerate(first['rows'][:4 + (14 if port else 12)]):
                        refs.append((ident, frame, *[row[f'p{port+1}_{k}'] for k in ('x', 'y', 'vx', 'vy')]))
        out = GAME / 'generated'
        lines = ['/* Local no-write dash motion fixtures; PC only. */']
        for title, rows in (('DashStart dash_start', starts), ('DashRef dash_ref', refs)):
            lines += ['static const ' + title + '[] = {'] + [' {' + ','.join(map(str, row)) + '},' for row in rows] + ['};']
        (out / 'dash.h').write_text('\n'.join(lines) + '\n')
        (out / 'dash.json').write_text(json.dumps(dict(metadata=metadata, trials=trials, program_sha256=PROGRAM_SHA,
            script_sha256=sha(Path(__file__).read_bytes()), native_steps=len(refs),
            adaptation='Horizontal wall action/recovery; moving collision poses; dash artwork'), indent=2) + '\n')
        print(f'Dash: {len(trials)} no-write trials replayed twice; {len(refs)} motion steps')
    finally:
        n.close()
if __name__ == '__main__': main()
