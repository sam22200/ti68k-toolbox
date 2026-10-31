#!/usr/bin/env python3
"""Compare original Charge fixtures with the shared engine on PC and 68000."""
from pathlib import Path
import argparse
import re
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
OUT = HERE / 'x/charge'
BANKS = 'ffbm ffbn ffgv0 ffgv1 ffgv2 ffgv3 ffga ffgi ffgd ffcp ffcu ffcw ffca ffcb ffcj'.split()


def run(args):
    p = subprocess.run(list(map(str, args)), cwd=HERE / 'data', text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if p.returncode:
        print(p.stdout); p.check_returncode()
    return p.stdout


def main():
    global OUT
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--out',type=Path,default=OUT)
    OUT=ap.parse_args().out.resolve()
    OUT.mkdir(parents=True, exist_ok=True)
    common = [HERE / 'check_charge.c', ROOT / 'runtime/core/rt_core.c']
    run(['cc', '-std=gnu99', '-O1', '-o', OUT / 'check', *common,
         ROOT / 'runtime/platform-sw/rt_sw.c'])
    pc = run([OUT / 'check'])
    run([ROOT / 'tools/bin/ti-cc', '-DRT_CYCLES', '-o', OUT / 'checkc',
         *common, ROOT / 'lib/unpack68k.s', ROOT / 'tools/extgraph/lib/tilemap.a'])
    command = [ROOT / 'tools/bin/ti-cycles', '--frames', 0, '--arg', 61, '--max', 1000]
    for bank in BANKS:
        command += ['--file', HERE / 'data' / (bank + '.89y')]
    ti = run([*command, OUT / 'checkc.89z'])
    (OUT / 'native.log').write_text(ti)
    match = re.search(r'charge ([0-9A-F]+) failure 0', pc); assert match, pc
    values = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', ti)]
    assert values[-2:] == [int(match[1], 16), 0], (pc, ti)
    print('Charge: 48 original previews/commits, all eight CT/tile/interruption cases, '
          'menu scrolling/cache pixels; PC/68000 hash ' + match[1])


if __name__ == '__main__':
    main()
