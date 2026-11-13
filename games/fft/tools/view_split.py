#!/usr/bin/env python3
"""Convert local chapter view banks (ffov0..3, ffgv0..3) to the ragged format of
extract.view_bank (R4: raw colours/width mask, compressed ragged depth), from the former
formats: one ZX0 stream of six planes, or V2/V3 (colours, then four full depth/mask planes
with row-XORed masks). Colour/mask pixels are preserved exactly; the contour tail of
view 0 is re-attached by tools/contour_assets.py. Idempotent: R4 banks are skipped.
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

def wrap(path):
    variable=path.with_suffix('.89y')
    if not variable.exists() or variable.stat().st_mtime_ns<path.stat().st_mtime_ns:
        subprocess.run([HERE/'../../tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',path.name,path.stem],
                       cwd=path.parent,check=True,capture_output=True)
        data=bytearray(variable.read_bytes());data[0x49]=3;variable.write_bytes(data)

for chapter in sys.argv[1:]:
    for rot in range(4):
        path = HERE / chapter / 'data' / ('%s%d.bin' % (PREFIX[chapter], rot))
        data = path.read_bytes()
        if data[:2] == b'R4':
            wrap(path);print(path.name, 'already R4'); continue
        if data[:2] in (b'R2',b'R3'):
            out=extract.archive_view(data);path.write_bytes(out);wrap(path)
            print(path.name,len(data),'->',len(out),'bytes');continue
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
        wrap(path)
        print(path.name, len(data), '->', len(out), 'bytes')
    first = HERE / chapter / 'data' / ('%s0.bin' % PREFIX[chapter])
    subprocess.run([sys.executable, HERE / 'tools/contour_assets.py', '--attach', first], check=True)
