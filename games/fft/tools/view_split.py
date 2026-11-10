#!/usr/bin/env python3
"""Convert local chapter view banks (ffov0..3, ffgv0..3) to the ragged format of
extract.view_bank (R2/R3: colours, width mask, ragged depth stream), from the former
formats: one ZX0 stream of six planes, or V2/V3 (colours, then four full depth/mask planes
with row-XORed masks). Colour and mask streams are kept byte for byte; the contour tail of
view 0 is re-attached by tools/contour_assets.py. Idempotent: R2/R3 banks are skipped.
usage: view_split.py opening|campaign ...
"""
from pathlib import Path
import struct
import subprocess
import sys

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE / 'tools'))
import extract  # noqa: E402

PREFIX = {'opening': 'ffov', 'campaign': 'ffgv'}

for chapter in sys.argv[1:]:
    for rot in range(4):
        path = HERE / chapter / 'data' / ('%s%d.bin' % (PREFIX[chapter], rot))
        data = path.read_bytes()
        if data[:2] in (b'R2', b'R3'):
            print(path.name, 'already ragged'); continue
        if len(data) >= 10 and data[-10:-6] in (b'FC01', b'FC02'):
            data = data[:struct.unpack('>H', data[-6:-4])[0]]
        if data[:2] in (b'V2', b'V3'):
            head = 6 if data[:2] == b'V3' else 4
            a = int.from_bytes(data[2:4], 'big')
            b = int.from_bytes(data[4:6], 'big') if head == 6 else 0
            colours, stroke = data[head:head + a], data[head + a:head + a + b]
            depth = extract.unzx0(data[head + a + b:])
            raw = extract.unrowxor_masks(bytes(2 * len(depth) // 4) + depth)
            out = extract.view_bank(b'R' + data[1:2], colours, stroke, extract.ragged_depth(raw))
        else:
            out = extract.pack_view(extract.unrowxor_masks(extract.unzx0(data)))
        path.write_bytes(out)
        print(path.name, len(data), '->', len(out), 'bytes')
    first = HERE / chapter / 'data' / ('%s0.bin' % PREFIX[chapter])
    subprocess.run([sys.executable, HERE / 'tools/contour_assets.py', '--attach', first], check=True)
