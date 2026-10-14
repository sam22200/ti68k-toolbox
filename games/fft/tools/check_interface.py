#!/usr/bin/env python3
"""Cross-check every partial UI build and both pages, plus per-frame TI budgets."""
import subprocess
import check_blows as checks


def main():
    subprocess.run(['make', 'pc', 'cycles', 'CFLAGS_GAME=-DFFT_MEASURE'], check=True)
    count = 0
    for scenario in (*range(26, 41), *range(53, 59)):
        for frames in (1, 2, 4, 8, 12, 20, 40):
            result = checks.ti(scenario, frames)
            checks.equal(scenario, frames, result); count += 1
        checks.budget(scenario, 40, result)
    for frames in (1, 2, 4, 8, 12, 20, 40, 60, 90, 120, 180):
        result = checks.ti(31, frames, 'interface')
        checks.equal(31, frames, result, 'interface'); count += 1
    checks.budget(31, 180, result, 'interface')
    for frames in (20, 40, 60, 80, 100, 120, 140, 160, 180, 200, 220, 240, 260, 280, 300, 320, 340, 360, 380):
        result = checks.ti(32, frames, 'status')
        checks.equal(32, frames, result, 'status'); count += 1
    checks.budget(32, 380, result, 'status')
    print('%d interface PC/TI screens and all ordinary frame budgets passed' % count)


if __name__ == '__main__': main()
