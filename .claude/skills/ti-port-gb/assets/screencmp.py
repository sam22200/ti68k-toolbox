#!/usr/bin/env python3
"""The port's playfield against the ROM's, pixel by pixel: screencmp.py SCRIPT:SCENARIO:F ...
The test binary replays keys/SCRIPT.txt (logic frames) from the ROM's door memory and draws the
screen at logic frame F (bghost_test --shot); the ROM under PyBoy the same, its screen taken
after the tick of logic frame F. GB rows 16..95 against TI rows 0..79, as 4 grey ranks.
Exit 1 on any difference.
Old note:
For each, the PC build runs N runtime frames (2 N GB logic frames) of keys/SCRIPT.txt headless
(read as runtime frames), the ROM the same script with its frame numbers doubled (logic frames)
and the ROM under PyBoy to logic frame 2 N, whose picture shows the sprites and BG of logic frame
2 N - 1 (the port's last); GB rows 16..95 (the
playfield) are compared with TI rows 0..79 as 4 grey ranks. Exit 1 on any difference."""
import os, subprocess, sys, warnings
warnings.filterwarnings('ignore')
from PIL import Image
TI = os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../..')
PY = os.path.join(TI, 'tools/pyenv/bin/python')
GB = os.path.join(TI, '.claude/skills/ti-port-gb/scripts/gbtrace.py')
ROM = os.path.join(TI, 'roms/gb/Bubble_Ghost.gb')


def ranks(im):
    px = list(im.getdata())
    order = {v: i for i, v in enumerate(sorted(set(px), reverse=True))}
    return [order[v] for v in px]


bad = 0
for spec in sys.argv[1:]:
    name, scen, n = spec.split(':')
    scen, n = int(scen), int(n)
    d = 7 if scen in (5, 6, 17, 18, 29, 30) else 3
    pokes = '' if scen == 0 else 'c0ac=2,c0b0=0,c0b3=0' if scen == 1 else 'c0ac=%d,c0b0=%d,c0b3=0' % (scen + 1, d)
    gpng, ppng = 'build/scr_gb_%d.png' % os.getpid(), 'build/scr_pc_%d.png' % os.getpid()   # per run
    subprocess.run([PY, GB, ROM, '--load', 'build/pre.state', '--boot-keys', 'A', '--poke', pokes,
                    '--poke-at', '0223', '--logic', '0283', '--frames', str(n + 1),
                    '--keys', 'keys/%s.txt' % name, '--shot', '%d:%s' % (n, gpng)],
                   check=True, stdout=subprocess.DEVNULL)
    subprocess.run(['./bghost_test', '--shot', name, str(scen), str(n), ppng], check=True,
                   stdout=subprocess.DEVNULL)
    g = Image.open(gpng).convert('L').crop((0, 16, 160, 96))
    p = Image.open(ppng).convert('L').crop((0, 0, 160, 80))
    a, b = ranks(g), ranks(p)
    diff = [i for i in range(len(a)) if a[i] != b[i]]
    print('%-10s frame %4d: %s' % (name, n, 'identical' if not diff else '%d pixels differ, rows %s'
          % (len(diff), sorted({i // 160 for i in diff})[:12])))
    bad |= bool(diff)
sys.exit(bad)
