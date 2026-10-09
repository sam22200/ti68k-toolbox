#!/usr/bin/env python3
"""Require complete native PC/TI hashes, screens and bounded frame costs."""
import json
import argparse
from pathlib import Path
import re
import subprocess

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]


def run(args):
    return subprocess.run(list(map(str, args)), cwd=GAME, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True).stdout


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--name',default='minish')
    parser.add_argument('--zoom',action='store_true')
    args=parser.parse_args()
    out = GAME / 'captures' / ('zoom' if args.zoom else '')
    out.mkdir(parents=True,exist_ok=True)
    cases = [(n, 'keys/opening.txt', 400) for n in range(7)]
    cases += [(2, 'keys/walls.txt', 300), (0, 'keys/walls.txt', 300),
              (0, 'keys/reset.txt', 160)]
    cases += [(n, 'keys/idle.txt', 1) for n in (7,8,*range(16,60))]
    cases += [(n, 'keys/idle.txt', 1) for n in range(100,140)]
    cases += [(n,'keys/sword.txt',120) for n in (0,7,8,140,141,143)]
    cases += [(142,'keys/idle.txt',1),(141,'keys/reset.txt',160)]
    cases += [(n,'keys/idle.txt',1) for n in range(144,256)]
    cases += [(256,'keys/combat.txt',300),(257,'keys/idle.txt',90),
              (258,'keys/retry.txt',60),(258,'keys/idle.txt',12),(256,'keys/reset.txt',160)]
    cases += [(n,'keys/sword.txt',120) for n in range(260,272)]
    cases += [(n,'keys/idle.txt',1) for n in range(300,360)]
    cases += [(n,'keys/idle.txt',1) for n in range(360,420)]
    cases += [(420,'keys/idle.txt',120),(420,'keys/idle.txt',14)]
    cases += [(430,'keys/roll.txt',120),(432,'keys/kill.txt',90)]
    cases += [(n,'keys/sword.txt',120) for n in range(440,456)]
    fx_count=json.loads((GAME/'fixtures/effects.json').read_text())['poses']
    cases += [(n,'keys/idle.txt',1) for n in range(512,512+3*fx_count)]
    cases += [(n,'keys/idle.txt',1) for n in range(700,892)]
    cases += [(0,'keys/opening.txt',n) for n in (190,192,196)]
    if args.zoom:
        cases += [(n, 'keys/idle.txt', 1) for n in range(64,100)]
    banks=('mindat','mizscene','mizactor','mizact','mizfight','mizfx') if args.zoom else ('mindat','miscen','michar','miact','mifight','mifx')
    files = [arg for name in banks for arg in ('--file',name+'.89y')]
    results = []
    for n, script, frames in cases:
        ti = run([ROOT/'tools/bin/ti-cycles', *files,
                  '--arg', n, '--keys', script, '--frames', frames, args.name+'h.89z'])
        pc = run(['./'+args.name+'_test', '--hash', script, frames, str(n)])
        values = [int(h,16) for h in re.findall(r'^value: -?\d+ \(0x([0-9a-f]+)\)',ti,re.M)]
        assert len(values) == frames*2+1, (n,script,'missing hash or frame cost')
        assert values[-1] == frames, (n,script,'early termination')
        hashes, costs = values[:-1:2], values[1:-1:2]
        assert hashes == list(map(int,pc.splitlines())) and len(hashes) == frames, (n, script, 'state hashes')
        native_ti = run([ROOT/'tools/bin/ti-cycles', *files,
                         '--arg', n, '--keys', script, '--frames', frames,
                         '--png', out/f'ti_{n}_{Path(script).stem}.png', args.name+'c.89z'])
        native_pc = run(['./'+args.name+'_pc', '--headless', '--scenario', n,
                         '--keys', script, '--frames', frames,
                         '--shot', out/f'pc_{n}_{Path(script).stem}.png'])
        tc = re.search(r'checksum\s+(\w+)', native_ti)
        pc = re.search(r'checksum\s+(\w+)', native_pc)
        assert tc and pc and tc[1] == pc[1], (n, script, native_ti, native_pc)
        # The instrumented run samples every full update+render frame, including
        # cold TileMap draws and cache rebuilds. Hash/marker overhead is retained.
        peak_bound = max(costs)
        zones = [int(x) for x in re.findall(r'^\d+\s+(?:update|render)\s+\d+\s+\d+\s+(\d+)',native_ti,re.M)]
        assert len(zones) == 2, native_ti
        assert peak_bound < 360000, (n, script, peak_bound)
        results.append({'scenario': n, 'keys': script, 'frames': frames,
                        'checksum': tc[1], 'update_average': zones[0],
                        'render_average': zones[1], 'frame_bound': peak_bound})
        print(f'PC/TI {n} {script}: {frames} field hashes, screen {tc[1]}, frame <= {peak_bound} cycles')
    (out/'native_checks.json').write_text(json.dumps(results, indent=2)+'\n')
    print(f"Native: {sum(r['frames'] for r in results)} PC/TI state hashes, {len(results)} screens passed")


if __name__ == '__main__':
    main()
