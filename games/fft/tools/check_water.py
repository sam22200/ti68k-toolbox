#!/usr/bin/env python3
"""Canal poses, entry/wading/exit and blows: PC/TI pixels and every frame's cost."""
import subprocess
import check_blows as checks


def main():
    subprocess.run(['make', 'pc', 'cycles', 'CFLAGS_GAME=-DFFT_MEASURE'], check=True)
    count = 0
    for scenario in range(41, 53):
        for frames in (1, 3, 5, 7, 9, 10, 12, 14, 16, 18, 24, 32, 40, 100):
            result = checks.ti(scenario, frames)
            checks.equal(scenario, frames, result); count += 1
        checks.budget(scenario, 100, result)
    print('%d water PC/TI screens and all ordinary frame budgets passed' % count)


if __name__ == '__main__': main()
