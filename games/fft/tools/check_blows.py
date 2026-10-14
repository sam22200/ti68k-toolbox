#!/usr/bin/env python3
"""PC/TI screens at every blow phase, plus complete frame costs on battle replays.

Run from games/fft. FFT_MEASURE emits (stall/mode/time/frame, cycles) pairs;
only initial/view loads, camera rotations and reach creation are allowed stalls.
Build outputs and the raw measurements stay local in x/.
"""
from pathlib import Path
import re
import subprocess

OUT = Path('x'); OUT.mkdir(exist_ok=True)
FILES = ['fftv0', 'fftv1', 'fftv2', 'fftv3', 'fftu', 'fftw', 'ffti']
BASE = ['../../tools/bin/ti-cycles', '--max', '8000']
for name in FILES: BASE += ['--file', name + '.89y']


def ti(scenario, frames, keys=None):
    args = BASE + ['--arg', str(scenario), '--frames', str(frames)]
    if keys: args += ['--keys', 'keys/' + keys + '.txt']
    result = subprocess.run(args + ['fftc.89z'], check=True, capture_output=True, text=True).stdout
    return result


def equal(scenario, frames, result, keys=None):
    args = ['./fft_pc', '--headless', '--scenario', str(scenario), '--frames', str(frames)]
    if keys: args += ['--keys', 'keys/' + keys + '.txt']
    pc = subprocess.run(args, check=True, capture_output=True, text=True).stdout
    assert re.search(r'checksum (\w+)', result)[1] == re.search(r'checksum (\w+)', pc)[1], (scenario, frames, keys)


def budget(scenario, frames, result, keys=None):
    values = [int(v, 16) for v in re.findall(r'value: -?\d+ \(0x([0-9a-f]+)\)', result)]
    pairs = list(zip(values[::2], values[1::2]))
    assert len(pairs) == frames
    ordinary = [(m & 65535, c, m >> 24 & 127) for m, c in pairs if not m >> 31]
    peak = max(ordinary, key=lambda p: p[1])
    over = [p for p in ordinary if p[1] > 360000]
    (OUT / ('m25_cycles_%d_%s.txt' % (scenario, keys or 'none'))).write_text(result)
    print('scenario %d %s: %d ordinary frames, peak %d at %d (mode %d), %d stalls, %d over budget'
          % (scenario, keys or '', len(ordinary), peak[1], peak[0], peak[2], frames - len(ordinary), len(over)), flush=True)
    assert not over, over


def main():
    subprocess.run(['make', 'pc', 'cycles', 'CFLAGS_GAME=-DFFT_MEASURE'], check=True)
    checkpoints = [13, 14, 15, 16, 18, 24, 26, 28, 40]
    for scenario in range(17, 26):
        for frames in checkpoints:
            result = ti(scenario, frames)
            equal(scenario, frames, result)
        budget(scenario, 40, result)
    for scenario, frames, keys in [(16, 14000, None), (16, 6000, 'fast'), (10, 260, 'attack'), (10, 1000, 'turn'),
                                   (11, 300, 'skills'), (14, 250, 'victory'), (15, 2400, 'defeat')]:
        result = ti(scenario, frames, keys)
        equal(scenario, frames, result, keys)
        budget(scenario, frames, result, keys)
    print('88 PC/TI screens and all replay frame budgets passed')


if __name__ == '__main__': main()
