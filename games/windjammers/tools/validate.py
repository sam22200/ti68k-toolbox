#!/usr/bin/env python3
"""Native PC/TI equality and per-frame datasheet cycle costs, without TiEmu."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
OUT = GAME / 'x'


def keys_for(scenario):
    if scenario == 27: return 'keys/dash.txt'
    if scenario == 28: return 'keys/dash_yoo.txt'
    if scenario >= 23: return f'keys/curve{scenario}.txt'
    if scenario >= 19: return 'keys/superlob.txt'
    if scenario >= 17: return 'keys/special.txt'
    if scenario >= 15: return 'keys/lob.txt'
    if scenario >= 13: return 'keys/timing_return.txt'
    if scenario >= 11: return 'keys/timing_lift.txt'
    return 'keys/idle.txt' if scenario >= 9 else 'keys/play.txt'


def run(args):
    if str(args[0]).endswith('/ti-cycles'):
        args = [args[0], '--file', 'wjart.89y', *args[1:]]
    return subprocess.run([str(a) for a in args], cwd=GAME, capture_output=True,
                          text=True, check=True).stdout


def check():
    codegen()
    run([ROOT / 'tools/bin/ti-cc', '-o', 'windjamh', '-DRT_CYCLES', '-DSTATE_HASH',
         '-DRT_FRAME_TICKS2=17', 'windjam.c', 'render.c', ROOT / 'runtime/core/rt_core.c',
         ROOT / 'runtime/platform-ti68k/rt_ti.c', ROOT / 'tools/extgraph/lib/tilemap.a'])
    cases = []
    # Extra uninterrupted training replay crosses the 30-second HUD clock's zero.
    for scenario, frames in [*[(n, 220) for n in range(29)], (4, 1000)]:
        keyfile = keys_for(scenario)
        args = [ROOT / 'tools/bin/ti-cycles', '--arg', scenario, '--keys', keyfile, '--frames', frames]
        ti = run([*args, 'windjamh.89z'])
        hashes = re.findall(r'^value: (\d+)', ti, re.MULTILINE)[:frames]
        pc = run(['./windjam_test', '--hash', keyfile, frames, scenario]).splitlines()
        assert len(hashes) == frames and hashes == pc, f'scenario {scenario}: state hashes differ'
        plain = run([*args, 'windjamc.89z'])
        screen = re.search(r'^shot \d+ checksum (\w+)', plain, re.MULTILINE).group(1)
        host = run(['./windjam_pc', '--headless', '--scenario', scenario, '--keys', keyfile, '--frames', frames])
        expected = re.search(r'checksum (\w+)', host).group(1)
        assert screen == expected, f'scenario {scenario}: planes differ'
        (OUT / f'scenario{scenario}_{frames}.ti.log').write_text(ti)
        cases.append(dict(scenario=scenario, frames=frames, checksum=screen, state_equal=True))
        print(f'scenario {scenario}: {frames} per-field state hashes and screen {screen}, TI = PC', flush=True)
    effect_cases = []
    for scenario in (13,14,17,18,21,22):
        keyfile = keys_for(scenario)
        kind = 1 if scenario < 15 else 2
        sample = run(['./windjam_test','--effect-frame',keyfile,scenario,kind]).split()
        frames, expected = int(sample[0]), sample[1]
        output = run([ROOT / 'tools/bin/ti-cycles','--arg',scenario,'--keys',keyfile,
                      '--frames',frames,'windjamc.89z'])
        screen = re.search(r'^shot \d+ checksum (\w+)',output,re.MULTILINE).group(1)
        assert screen == expected, f'scenario {scenario}: active trail planes differ'
        effect_cases.append(dict(scenario=scenario,frames=frames,kind=kind,checksum=screen))
        print(f'scenario {scenario}: active trail kind {kind}, frame {frames}, screen {screen}, TI = PC',flush=True)
    # Real SDL frontend state files, including an odd draw parity at save time.
    run(['./windjam_pc', '--headless', '--scenario', 4, '--frames', 301, '--save', 'x/whole.state'])
    run(['./windjam_pc', '--headless', '--scenario', 4, '--frames', 101, '--save', 'x/part.state'])
    run(['./windjam_pc', '--headless', '--load', 'x/part.state', '--frames', 200, '--save', 'x/resumed.state'])
    assert (OUT / 'whole.state').read_bytes() == (OUT / 'resumed.state').read_bytes(), 'SDL state continuation differs'
    (OUT / 'validation.json').write_text(json.dumps(dict(cases=cases, effect_cases=effect_cases,
        sdl_state_continuation=True), indent=2) + '\n')
    print('SDL state files: 301 uninterrupted frames = 101 saved + 200 restored', flush=True)


def codegen():
    compiler = ROOT / 'tools/gcc4ti-bin'
    environment = dict(os.environ, TIGCC=str(compiler), PATH=str(compiler / 'bin') + os.pathsep + os.environ['PATH'])
    flags = ['-Os', '-mshort', '-mregparm=5', '-fomit-frame-pointer', '-ffunction-sections',
             '-fdata-sections', '-mno-bss', '-DRT_FRAME_TICKS2=17', '-I' + str(compiler / 'include/c')]
    subprocess.run([str(compiler / 'bin/m68k-coff-tigcc-gcc'), *flags, '-S', 'windjam.c',
                    '-o', str(OUT / 'windjam.s')], cwd=GAME, env=environment, check=True)
    assembly = (OUT / 'windjam.s').read_text()
    forbidden = re.findall(r'\b(?:mulu|muls)\.l\b|\b(?:divu|divs)\.[wl]\b|\b_+(?:mul|[us]?div|[us]?mod)(?:hi|si|di)[234]\b|\b_+\w*(?:sf|df)[234]\b', assembly)
    assert not forbidden, f'physics codegen contains expensive arithmetic: {forbidden}'
    (OUT / 'codegen.json').write_text(json.dumps(dict(compiler=str(compiler), flags=flags,
                                                    forbidden_arithmetic=[]), indent=2) + '\n')
    print('68000 physics codegen: word products only; no division, long products, floats or arithmetic helpers', flush=True)


def profile():
    result = {}
    for name, scenario, frames in (('play', 0, 220), ('demo', 4, 220), ('goal', 3, 100), ('rear', 5, 60),
                                   ('mita_catch', 7, 40), ('yoo_catch', 8, 40), ('auto_yoo', 9, 320), ('auto_mita', 10, 320), ('mita_lift', 11, 220), ('yoo_lift', 12, 220),
                                   ('mita_immediate', 13, 220), ('yoo_immediate', 14, 220),
                                   *[(f'advanced_{n}', n, 220) for n in range(15, 23)],
                                   *[(f'curve_{n}', n, 220) for n in range(23, 27)],
                                   ('dash_mita', 27, 100), ('dash_yoo', 28, 100)):
        previous, rows = [0, 0], []
        for n in range(1, frames + 1):
            output = run([ROOT / 'tools/bin/ti-cycles', '--arg', scenario, '--keys', keys_for(scenario),
                          '--frames', n, 'windjamc.89z'])
            totals = []
            for zone in ('update', 'render'):
                match = re.search(r'^\d+\s+' + zone + r'\s+\d+\s+(\d+)\s+\d+', output, re.MULTILINE)
                if not match:
                    raise RuntimeError('missing cycle zone: ' + output)
                totals.append(int(match.group(1)))
            costs = [a - b for a, b in zip(totals, previous)]
            rows.append(dict(frame=n - 1, update=costs[0], render=costs[1], total=sum(costs)))
            previous = totals
        peak = max(rows, key=lambda row: row['total'])
        assert peak['total'] < 360000, (name, peak)
        result[name] = dict(scenario=scenario, frames=rows, peak=peak,
                            average=sum(row['total'] for row in rows) // frames)
        print(f'{name}: average {result[name]["average"]}, peak {peak} cycles', flush=True)
    (OUT / 'cycles.json').write_text(json.dumps(result, indent=2) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument('--profile', action='store_true')
    modes.add_argument('--codegen', action='store_true')
    args = parser.parse_args()
    OUT.mkdir(exist_ok=True)
    if args.profile:
        profile()
    elif args.codegen:
        codegen()
    else:
        check()


if __name__ == '__main__':
    main()
