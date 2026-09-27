#!/usr/bin/env python3
"""Runs the benchmark build (make bench) under ti-cycles for scenarios 0, 1, 2 and prints the
cycles per frame of each zone. --save NAME stores the result, --ref NAME compares the cycles
with it, --screens NAME the screens (default: the --ref one).
The screen checksum of each scenario (last frame, the 128x100 view) tells whether an
optimisation changed the picture. It leaves out rows 45..47: at the horizon the original samples
outside its far texture (the window is narrower than the view there) and reads whatever memory
is next to it; "horizon px" counts the pixels of those rows that differ from the reference."""
import json, subprocess, sys, hashlib, os, shutil
from PIL import Image
import numpy as np
TIC = os.path.join(os.path.dirname(__file__), '../../../tools/bin/ti-cycles')
res = {}
for n in (0, 1, 2):
    png = '/tmp/m7bench_%d.png' % n
    out = subprocess.run([TIC, '--arg', str(n), '--png', png, 'build/m7bench.89z'],
                         capture_output=True, text=True).stdout.splitlines()
    z = {}
    frames = None
    for l in out:
        p = l.split()
        if p and p[0].isdigit():
            name = ' '.join(p[1:-3]); calls, total = int(p[-3]), int(p[-2])
            if name == 'frame': frames = calls
            z[name] = total
        if l.startswith('value:'): z['coverage px'] = int(p[1]) * (frames or 16)
    for k in z: z[k] //= frames
    img = np.array(Image.open(png))[:100, :128]
    z['screen'] = hashlib.md5(np.delete(img, [45, 46, 47], axis=0).tobytes()).hexdigest()[:8]
    refpng = None
    sref = sys.argv[sys.argv.index('--screens') + 1] if '--screens' in sys.argv else sys.argv[sys.argv.index('--ref') + 1] if '--ref' in sys.argv else None
    if sref: refpng = 'tools/bench_%s_%d.png' % (sref, n)
    if refpng and os.path.exists(refpng):
        z['horizon px'] = int((np.array(Image.open(refpng))[45:48, :128] != img[45:48]).sum())
    if '--save' in sys.argv: shutil.copy(png, 'tools/bench_%s_%d.png' % (sys.argv[sys.argv.index('--save') + 1], n))
    res[n] = z
ref = None
if '--ref' in sys.argv: ref = json.load(open('tools/bench_%s.json' % sys.argv[sys.argv.index('--ref') + 1]))
if '--save' in sys.argv: json.dump(res, open('tools/bench_%s.json' % sys.argv[sys.argv.index('--save') + 1], 'w'), indent=1)
sref_name = sys.argv[sys.argv.index('--screens') + 1] if '--screens' in sys.argv else sys.argv[sys.argv.index('--ref') + 1] if '--ref' in sys.argv else None
sref_json = json.load(open('tools/bench_%s.json' % sref_name)) if sref_name else None
names = list(res[0])
print('%-22s' % 'cycles / frame' + ''.join('%22s' % ('scenario %d' % n) for n in res))
for k in names:
    row = '%-22s' % k
    for n in res:
        v = res[n].get(k, '')
        r = ref[str(n)].get(k) if ref else None
        if isinstance(v, int) and isinstance(r, int) and r:
            row += '%22s' % ('%d (%+d%%)' % (v, round(100 * (v - r) / r)))
        elif isinstance(v, str) and sref_json:
            rs = sref_json[str(n)].get(k)
            row += '%22s' % (v + ('' if v == rs else ' CHANGED'))
        else:
            row += '%22s' % v
    print(row)
