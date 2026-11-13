#!/usr/bin/env python3
"""Prepared objective policy and original-source predicates on PC only."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def run(command, log):
    p = subprocess.run(list(map(str, command)), text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log.write_text(p.stdout)
    p.check_returncode()
    return p.stdout


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, default=HERE/'x/objectives/pc')
    out = ap.parse_args().out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    run([sys.executable, HERE.parent/'tools/objective_oracle.py', '--out', out/'oracle'], out/'oracle.log')
    logs = []
    for name, flags in [('pc', []), ('san', ['-fsanitize=address,undefined', '-fno-pie', '-no-pie'])]:
        binary = out/name
        run(['cc', '-std=gnu99', '-O1', '-g', *flags, '-o', binary, HERE/'test_objective_rules.c'], out/f'{name}-build.log')
        logs.append(run([binary], out/f'{name}.log'))
    assert logs[0] == logs[1]
    report = dict(result=logs[0].strip(), sanitizers_equal=True, native=False, integrated=False,
                  oracle_sha256=hashlib.sha256((out/'oracle/results.json').read_bytes()).hexdigest(),
                  sources={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                           for p in (HERE.parent/'objective_rules.h', HERE/'test_objective_rules.c')})
    (out/'results.json').write_text(json.dumps(report, indent=2)+'\n')
    print(logs[0].strip())


if __name__ == '__main__':
    main()
